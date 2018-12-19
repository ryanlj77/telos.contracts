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
#include "test_symbol.hpp"

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

class eosio_fox_tester : public tester {

public:
	abi_serializer protocol_ser;
	abi_serializer tlos_token_ser;
	abi_serializer fox_token_ser;

    eosio_fox_tester() {

		produce_blocks( 2 );

		create_accounts({N(eosio.token), N(eosio.ram), N(eosio.ramfee), N(eosio.stake),
			N(eosio.bpay), N(eosio.vpay), N(eosio.saving), N(eosio.names), N(eosio.trail)});
		
        produce_blocks( 2 );

		set_code(N(eosio.token), contracts::token_wasm());
		set_abi(N(eosio.token), contracts::token_abi().data());
		{
			const auto &accnt = control->db().get<account_object, by_name>(N(eosio.token));
			abi_def abi;
			BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
			tlos_token_ser.set_abi(abi, abi_serializer_max_time);
		}

		tlos_create(N(eosio), asset::from_string("10000000000.0000 TLOS"));
		tlos_issue(N(eosio), N(eosio), asset::from_string("1000000000.0000 TLOS"), "Initial amount!");
		produce_blocks( 2 );

		create_accounts({N(foxprotocol1), N(foxtoken1), N(foxtoken2), N(foxtoken3)});

		deploy_foxprotocol_contract();
		
		deploy_foxtoken_contract(N(foxtoken1));
		produce_blocks( 2 );
		deploy_foxtoken_contract(N(foxtoken2));
		produce_blocks( 2 );
		deploy_foxtoken_contract(N(foxtoken3));
		
		produce_blocks( 2 );

		create_accounts({N(craig), N(peter), N(ed), N(james), N(douglas)});
		produce_blocks( 2 );

		//TODO: create and mint tokens to users

		//TODO: regdomain for each registry

		//TODO: 

		std::cout << "=======================END SETUP==============================" << std::endl;
	}

    action_result tlos_create(account_name issuer, asset maximum_supply) {
		return tlos_push_action(N(eosio.token), N(create), mvo()("issuer", issuer)("maximum_supply", maximum_supply));
	}

	action_result tlos_issue(account_name issuer, account_name to, asset quantity, string memo) {
		return tlos_push_action(issuer, N(issue), mvo()("to", to)("quantity", quantity)("memo", memo));
	}

    action_result tlos_push_action(const account_name &signer, const action_name &name, const variant_object &data) {
		string action_type_name = tlos_token_ser.get_action_type(name);

		action act;
		act.account = N(eosio.token);
		act.name = name;
		act.data = tlos_token_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

		return base_tester::push_action(std::move(act), uint64_t(signer));
	}

    void deploy_foxprotocol_contract() {
        set_code(N(foxprotocol1), contracts::fox_protocol_wasm());
        set_abi(N(foxprotocol1), contracts::fox_protocol_abi().data());
        {
            const auto& accnt = control->db().get<account_object,by_name>(N(foxprotocol1));
            abi_def abi;
            BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
            protocol_ser.set_abi(abi, abi_serializer_max_time);
        }
    }

	void deploy_foxtoken_contract(account_name target_contract) {
        set_code(target_contract, contracts::fox_token_wasm());
        set_abi(target_contract, contracts::fox_token_abi().data());
        {
            const auto& accnt = control->db().get<account_object,by_name>(target_contract);
            abi_def abi;
            BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
            fox_token_ser.set_abi(abi, abi_serializer_max_time);
        }
    }

    uint32_t now() {
	    return (control->pending_block_time().time_since_epoch().count() / 1000000);
    }

	//TODO: check permission
	transaction_trace_ptr foxtoken_mint(account_name target_contract, account_name recipient, asset tokens) {
		signed_transaction trx;
		trx.actions.emplace_back( get_action(target_contract, N(mint), vector<permission_level>{{target_contract, config::active_name}},
			mvo()
			("recipient", recipient)
			("tokens", tokens)
			)
		);
		set_transaction_headers(trx);
		trx.sign(get_private_key(target_contract, "active"), control->get_chain_id());
		return push_transaction( trx );
	}

    // fc::variant get_order(symbol sym) {
	// 	vector<char> data = get_row_by_account(N(eosio.trail), N(eosio.trail), N(registries), sym.to_symbol_code());
	// 	return data.empty() ? fc::variant() : abi_ser.binary_to_variant("registry", data, abi_serializer_max_time);
	// }

};