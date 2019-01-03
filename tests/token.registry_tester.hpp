#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/name.hpp>
#include <iostream>

#include <eosio/chain/wast_to_wasm.hpp>
#include <Runtime/Runtime.h>
#include <math.h>

#include <fc/variant_object.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

class token_registry_tester : public tester {

public:

	abi_serializer token_ser;
	abi_serializer reg_ser;

	vector<account_name> test_accounts {
		N(username1111), 
		N(username1112), 
		N(username1113), 
		N(username1114), 
		N(username1115), 
		N(username1121),
		N(username1122),
		N(username1123),
		N(username1124),
		N(username1125),
		N(username1131)
	};

	token_registry_tester() {

		produce_blocks( 2 );
		create_accounts({
			N(eosio.token), 
			N(eosio.ram), 
			N(eosio.ramfee), 
			N(eosio.stake),
			N(eosio.bpay), 
			N(eosio.vpay), 
			N(eosio.saving), 
			N(eosio.names), 
			N(eosio.trail)});
		produce_blocks( 2 );

		//deploy eosio.token contract

		set_code(N(eosio.token), contracts::token_wasm());
		set_abi(N(eosio.token), contracts::token_abi().data());
		{
			const auto &accnt = control->db().get<account_object, by_name>(N(eosio.token));
			abi_def abi;
			BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
			token_ser.set_abi(abi, abi_serializer_max_time);
		}
		produce_blocks( 2 );

		create(N(eosio), asset::from_string("10000000000.0000 TLOS"));
		issue(N(eosio), N(eosio), asset::from_string("1000000000.0000 TLOS"), "Initial amount!");
		produce_blocks( 2 );

		//Token Registry Setup

		create_accounts({N(thepublisher)}); //account publishing the token
		create_accounts(test_accounts); //accounts testing the token
		produce_blocks();

		deploy_tokenreg_contract();
		produce_blocks();
		
		eosio_transfer(N(eosio), N(thepublisher), asset(100000000, symbol(4, "TLOS")), std::string("fund publishing account"));

        string test_token_name = "Viita";
		symbol test_sym = symbol(4, "VIITA");

		//TEST: execute createwallet() for publisher
        create_wallet(N(thepublisher));
		produce_blocks();

		//TEST: check for zero-balance wallet
        auto pub_balance = get_token_balance(N(thepublisher));
        REQUIRE_MATCHING_OBJECT(pub_balance, mvo()
			("owner", "thepublisher")
			("tokens", "0.0000 VIITA")
		);

		produce_blocks();
        
		auto token_config = get_token_config(N(thepublisher));
		BOOST_REQUIRE_EQUAL(false, token_config.is_null());
		// std::cout << "publisher: " << token_config["publisher"] 
		// 	<< " token_name: " << token_config["token_name"] 
		// 	<< " max_supply: " << token_config["max_supply"] 
		// 	<< " supply: " << token_config["supply"] 
		// 	<< std::endl;

		REQUIRE_MATCHING_OBJECT(token_config, mvo()
			("publisher", "thepublisher")
			("token_name", "Viita")
			("max_supply", "10000000000.0000 VIITA")
			("supply", "0.0000 VIITA")
		);

		produce_blocks();

		//NOTE: END SETUP
	}

    void deploy_tokenreg_contract() {
        set_code(N(thepublisher), contracts::tokenreg_wasm());
        set_abi(N(thepublisher), contracts::tokenreg_abi().data());
        {
            const auto &accnt = control->db().get<account_object, by_name>(N(thepublisher));
            abi_def abi;
            BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
            reg_ser.set_abi(abi, abi_serializer_max_time);
        }
    }

	//NOTE: only pushes to eosio.token contract
	action_result push_action(const account_name &signer, const action_name &name, const variant_object &data) {
		string action_type_name = token_ser.get_action_type(name);

		action act;
		act.account = N(eosio.token);
		act.name = name;
		act.data = token_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

		return base_tester::push_action(std::move(act), uint64_t(signer));
	}

	action_result create(account_name issuer, asset maximum_supply){
		return push_action(N(eosio.token), N(create), mvo()("issuer", issuer)("maximum_supply", maximum_supply));
	}

	action_result issue(account_name issuer, account_name to, asset quantity, string memo){
		return push_action(issuer, N(issue), mvo()("to", to)("quantity", quantity)("memo", memo));
	}

	transaction_trace_ptr eosio_transfer(account_name from, account_name to, asset amount, string memo) {
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

	transaction_trace_ptr token_mint(account_name recipient, asset amount, account_name signer) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(thepublisher), N(mint), vector<permission_level>{{signer, config::active_name}},
			mvo()
			("recipient", recipient)
			("tokens", amount)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(signer, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr token_transfer(account_name sender, account_name recipient, asset amount) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(thepublisher), N(transfer), vector<permission_level>{{sender, config::active_name}},
			mvo()
			("sender", sender)
			("recipient", recipient)
			("tokens", amount)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(sender, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr token_allot(account_name sender, account_name recipient, asset amount) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(thepublisher), N(allot), vector<permission_level>{{sender, config::active_name}},
			mvo()
			("sender", sender)
			("recipient", recipient)
			("tokens", amount)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(sender, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr token_unallot(account_name sender, account_name recipient, asset amount) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(thepublisher), N(unallot), vector<permission_level>{{sender, config::active_name}},
			mvo()
			("sender", sender)
			("recipient", recipient)
			("tokens", amount)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(sender, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr token_claimallot(account_name sender, account_name recipient, asset amount) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(thepublisher), N(claimallot), vector<permission_level>{{recipient, config::active_name}},
			mvo()
			("sender", sender)
			("recipient", recipient)
			("tokens", amount)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(recipient, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr create_wallet(account_name recipient) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(thepublisher), N(createwallet), vector<permission_level>{{recipient, config::active_name}},
			mvo()
			("recipient", recipient)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(recipient, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

	transaction_trace_ptr delete_wallet(account_name wallet_owner) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(N(thepublisher), N(deletewallet), vector<permission_level>{{wallet_owner, config::active_name}},
			mvo()
			("wallet_owner", wallet_owner)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(wallet_owner, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

    fc::variant get_token_config(account_name publisher) {
		vector<char> data = get_row_by_account(publisher, publisher, N(tokenconfig), N(tokenconfig));
		return data.empty() ? fc::variant() : reg_ser.binary_to_variant("tokenconfig", data, abi_serializer_max_time);
	}

    fc::variant get_token_balance(account_name owner) {
		vector<char> data = get_row_by_account(N(thepublisher), N(thepublisher), N(balances), owner);
		return data.empty() ? fc::variant() : reg_ser.binary_to_variant("balance", data, abi_serializer_max_time);
	}

    fc::variant get_token_allotment(account_name sender, account_name recipient) {
		vector<char> data = get_row_by_account(N(thepublisher), sender, N(allotments), recipient);
		return data.empty() ? fc::variant() : reg_ser.binary_to_variant("allotment", data, abi_serializer_max_time);
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