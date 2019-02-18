#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/name.hpp>
#include <iostream>

#include <eosio/chain/wast_to_wasm.hpp>
#include <Runtime/Runtime.h>
#include <math.h>

#include <fc/variant_object.hpp>

#include <contracts.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

#define NUM_VOTERS 128

using mvo = fc::mutable_variant_object;

class trail_tester : public tester
{
  public:
	abi_serializer abi_ser;
	abi_serializer token_ser;

	vector<name> test_voters;

	trail_tester()
	{
		produce_blocks(2);
        //make system accounts
		create_accounts({N(eosio.token), N(eosio.ram), N(eosio.ramfee),
            N(eosio.stake), N(eosio.bpay), N(eosio.vpay),
            N(eosio.saving), N(eosio.names), N(trailservice)});

        //make test accounts
        create_accounts({N(testaccount1), N(testaccount2), N(testaccount3), N(testaccount4), N(testaccount5)});
        produce_blocks(2);

        //deploy eosio.token
		deploy_token_contract();

        //init eosio.token
		create(N(eosio), asset::from_string("10000000000.0000 TLOS"));
		issue(N(eosio), N(eosio), asset::from_string("1000000000.0000 TLOS"), "Initial amount!");
		produce_blocks(2);

        //deploy trail
		deploy_trail_contract();
		produce_blocks(2);

        //init trail
		asset max_supply = asset::from_string("10000000000.0000 VOTE");
		string info_url = "Telos Governance Registry";
        settings _settings = {
            bool is_destructible = false;
            bool is_proxyable = false;
            bool is_burnable = false;
            bool is_seizable = false;
            bool is_max_mutable = false;
            bool is_transferable = false;
        };
        create_trail_token( N(trailservice), max_supply, _settings, info_url);
		produce_blocks(1);

        //

        //

		std::cout << "=======================END SETUP==============================" << std::endl;
	}
	
	

    uint32_t now() {
        return (control->pending_block_time().time_since_epoch().count() / 1000000);
    }

    string base31 = "abcdefghijklmnopqrstuvwxyz12345";

    string toBase31(uint32_t in) {
        vector<uint32_t> out = { 0, 0, 0, 0, 0, 0, 0 };
        uint32_t remainder = in;
        uint32_t divisor = 0;
        uint32_t quotient = 0;
        for (int i = 0; i < out.size(); i++) {
            divisor = pow(31, out.size() - 1 - i);
            quotient = remainder / divisor;
            remainder = remainder - (quotient * divisor);
            out[i] = quotient;
        }
        string output = "aaaaaaa";
        for (int i = 0; i < out.size(); i++) {
            output[i] = base31[out[i]];
        }
        return output;
    }

   //push action

    action_result push_action(const account_name &signer, const action_name &name, const variant_object &data) {
		string action_type_name = token_ser.get_action_type(name);
		action act;
		act.account = N(eosio.token);
		act.name = name;
		act.data = token_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);
		return base_tester::push_action(std::move(act), uint64_t(signer));
	}

	action_result trail_push_action(const account_name &signer, const action_name &name, const variant_object &data) {
		string action_type_name = abi_ser.get_action_type(name);
		action act;
		act.account = N(trailservice);
		act.name = name;
		act.data = abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);
		return base_tester::push_action(std::move(act), uint64_t(signer));
	}

    //eosio.token 

    void deploy_token_contract() {
        set_code(N(eosio.token), contracts::token_wasm());
		set_abi(N(eosio.token), contracts::token_abi().data());
		{
			const auto &accnt = control->db().get<account_object, by_name>(N(eosio.token));
			abi_def abi;
			BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
			token_ser.set_abi(abi, abi_serializer_max_time);
		}
    }

	action_result create(account_name issuer, asset maximum_supply) {
		return push_action(N(eosio.token), N(create), mvo()("issuer", issuer)("maximum_supply", maximum_supply));
	}

	action_result issue(account_name issuer, account_name to, asset quantity, string memo) {
		return push_action(issuer, N(issue), mvo()("to", to)("quantity", quantity)("memo", memo));
	}

    transaction_trace_ptr transfer(account_name from, account_name to, asset amount, string memo) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(eosio.token), N(transfer), vector<permission_level>{{from, config::active_name}},
			mvo()
			("from", from)
			("to", to)
			("quantity", amount)
			("memo", memo)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(from, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	fc::variant get_account(account_name acc, const string &symbolname) {
		auto symb = eosio::chain::symbol::from_string(symbolname);
		auto symbol_code = symb.to_symbol_code().value;
		vector<char> data = get_row_by_account(N(eosio.token), acc, N(accounts), symbol_code);
		return data.empty() ? fc::variant() : abi_ser.binary_to_variant("account", data, abi_serializer_max_time);
	}

    //trailservice

    void deploy_trail_contract() {
        set_code( N(trailservice), contracts::new_trail_wasm() );
        set_abi( N(trailservice), contracts::new_trail_abi().data() );
        {
            const auto& accnt = control->db().get<account_object,by_name>( N(trailservice) );
            abi_def abi;
            BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
            abi_ser.set_abi(abi, abi_serializer_max_time);
        }
    }

	fc::variant get_ballot(name ballot_name) {
		vector<char> data = get_row_by_account(N(trailservice), N(trailservice), N(ballots), ballot_name);
		return data.empty() ? fc::variant() : abi_ser.binary_to_variant("ballot", data, abi_serializer_max_time);
	}

	fc::variant get_registry(symbol sym) {
		vector<char> data = get_row_by_account(N(trailservice), N(trailservice), N(registries), sym.to_symbol_code());
		return data.empty() ? fc::variant() : abi_ser.binary_to_variant("registry", data, abi_serializer_max_time);
	}

    fc::variant get_vote_receipt(account_name voter, name ballot_name) {
		vector<char> data = get_row_by_account(N(trailservice), voter, N(votereceipts), ballot_name);
		return data.empty() ? fc::variant() : abi_ser.binary_to_variant("vote_receipt", data, abi_serializer_max_time);
	}

    fc::variant get_voter(account_name voter, symbol sym) {
		vector<char> data = get_row_by_account(N(trailservice), voter, N(accounts), sym);
		return data.empty() ? fc::variant() : abi_ser.binary_to_variant("account", data, abi_serializer_max_time);
	}

	transaction_trace_ptr create_trail_token(name publisher, asset max_supply, token_settings settings, string info_url) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(createtoken), vector<permission_level>{{publisher, config::active_name}},
			mvo()
			("publisher", publisher)
			("max_supply", max_supply)
			("settings", settings)
            ("info_url", info_url)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(publisher, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

    transaction_trace_ptr trail_transfer(name sender, name recipient, asset amount, string memo) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(transfer), vector<permission_level>{{from, config::active_name}},
			mvo()
			("sender", sender)
			("recipient", recipient)
			("amount", amount)
            ("memo", memo)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(sender, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr createballot(name ballot_name, name category, name publisher, string title, string description, string info_url, symbol voting_sym) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(createballot), vector<permission_level>{{publisher, config::active_name}},
			mvo()
			("ballot_name", ballot_name)
			("category", category)
			("publisher", publisher)
			("title", title)
			("description", description)
			("info_url", info_url)
            ("voting_sym", voting_sym)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(publisher, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

    //utilities

	void dump_trace(transaction_trace_ptr trace_ptr) {
		std::cout << std::endl << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
		for(auto trace : trace_ptr->action_traces) {
			std::cout << "action_name trace: " << trace.act.name.to_string() << std::endl;
			//TODO: split by new line character, loop and indent output
			std::cout << trace.console << std::endl << std::endl;
		}
		std::cout << std::endl << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl << std::endl;
	}
};