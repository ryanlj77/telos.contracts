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

        //transfer tlos to test accounts
        transfer(N(eosio), N(testaccount1), asset::from_string("100.0000 TLOS"), "initial fund");
        transfer(N(eosio), N(testaccount2), asset::from_string("200.0000 TLOS"), "initial fund");
        transfer(N(eosio), N(testaccount3), asset::from_string("300.0000 TLOS"), "initial fund");
        transfer(N(eosio), N(testaccount4), asset::from_string("400.0000 TLOS"), "initial fund");
        transfer(N(eosio), N(testaccount5), asset::from_string("500.0000 TLOS"), "initial fund");

        //deploy trail
		deploy_trail_contract();
		produce_blocks(2);

        //init VOTE registry
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
        newtoken(N(trailservice), max_supply, _settings, info_url);
		produce_blocks();

		std::cout << "=======================END SETUP==============================" << std::endl;
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

    fc::variant get_vote(account_name voter, name ballot_name) {
		vector<char> data = get_row_by_account(N(trailservice), voter, N(votes), ballot_name);
		return data.empty() ? fc::variant() : abi_ser.binary_to_variant("vote", data, abi_serializer_max_time);
	}

    fc::variant get_voter(account_name voter, symbol sym) {
		vector<char> data = get_row_by_account(N(trailservice), voter, N(accounts), sym);
		return data.empty() ? fc::variant() : abi_ser.binary_to_variant("account", data, abi_serializer_max_time);
	}

	//ballots

	transaction_trace_ptr newballot(name ballot_name, name category, name publisher, string title, string description, string info_url, symbol voting_sym) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(newballot), vector<permission_level>{{publisher, config::active_name}},
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

	transaction_trace_ptr setinfo(name ballot_name, name publisher, string title, string description, string info_url) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(setinfo), vector<permission_level>{{publisher, config::active_name}},
			mvo()
			("ballot_name", ballot_name)
			("publisher", publisher)
			("title", title)
			("description", description)
			("info_url", info_url)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(publisher, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr addoption(name ballot_name, name publisher, name option_name, string option_info) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(addoption), vector<permission_level>{{publisher, config::active_name}},
			mvo()
			("ballot_name", ballot_name)
			("publisher", publisher)
			("option_name", option_name)
			("option_info", option_info)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(publisher, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr readyballot(name ballot_name, name publisher, uint32_t end_time) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(readyballot), vector<permission_level>{{publisher, config::active_name}},
			mvo()
			("ballot_name", ballot_name)
			("publisher", publisher)
			("end_time", end_time)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(publisher, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr closeballot(name ballot_name, name publisher, uint8_t new_status) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(closeballot), vector<permission_level>{{publisher, config::active_name}},
			mvo()
			("ballot_name", ballot_name)
			("publisher", publisher)
			("new_status", new_status)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(publisher, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr deleteballot(name ballot_name, name publisher) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(deleteballot), vector<permission_level>{{publisher, config::active_name}},
			mvo()
			("ballot_name", ballot_name)
			("publisher", publisher)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(publisher, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr vote(name voter, name ballot_name, name option) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(vote), vector<permission_level>{{voter, config::active_name}},
			mvo()
			("voter", voter)
			("ballot_name", ballot_name)
			("option", option)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(voter, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr unvote(name voter, name ballot_name, name option) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(unvote), vector<permission_level>{{voter, config::active_name}},
			mvo()
			("voter", voter)
			("ballot_name", ballot_name)
			("option", option)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(voter, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr cleanupvotes(name voter, uint16_t count, symbol voting_sym) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(cleanupvotes), vector<permission_level>{{voter, config::active_name}},
			mvo()
			("voter", voter)
			("count", count)
			("voting_sym", voting_sym)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(voter, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	//tokens

	transaction_trace_ptr newtoken(name publisher, asset max_supply, token_settings settings, string info_url) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(newtoken), vector<permission_level>{{publisher, config::active_name}},
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

	transaction_trace_ptr mint(name publisher, name recipient, asset amount_to_mint) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(mint), vector<permission_level>{{publisher, config::active_name}},
			mvo()
			("publisher", publisher)
			("recipient", recipient)
			("amount_to_mint", amount_to_mint)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(publisher, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr burn(name publisher, asset amount_to_burn) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(burn), vector<permission_level>{{publisher, config::active_name}},
			mvo()
			("publisher", publisher)
			("amount_to_burn", amount_to_burn)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(publisher, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

    transaction_trace_ptr send(name sender, name recipient, asset amount, string memo) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(send), vector<permission_level>{{sender, config::active_name}},
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

	transaction_trace_ptr seize(name publisher, name owner, asset amount_to_seize) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(seize), vector<permission_level>{{publisher, config::active_name}},
			mvo()
			("publisher", publisher)
			("owner", owner)
			("amount_to_seize", amount_to_seize)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(publisher, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr open(name owner, symbol token_sym) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(open), vector<permission_level>{{owner, config::active_name}},
			mvo()
			("owner", owner)
			("token_sym", token_sym)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(owner, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr close(name owner, symbol token_sym) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(trailservice), N(close), vector<permission_level>{{owner, config::active_name}},
			mvo()
			("owner", owner)
			("token_sym", token_sym)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(owner, "active"), control->get_chain_id());
		return push_transaction( trx );
	}
	

    //utilities

	uint32_t now() {
        return (control->pending_block_time().time_since_epoch().count() / 1000000);
    }

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