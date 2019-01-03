#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <Runtime/Runtime.h>
#include <iomanip>

#include <fc/variant_object.hpp>
#include "contracts.hpp"
#include "test_symbol.hpp"
#include "token.registry_tester.hpp"

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(token_registry_tests)

BOOST_FIXTURE_TEST_CASE(wallets, token_registry_tester) try {
	
    account_name publisher = name("thepublisher");
    symbol sym = symbol(4, "VIITA");
	asset max_supply = asset(100000000000000, sym); 
	asset supply = asset(0, sym);
	produce_blocks();

    //TEST: create and check zero-balance wallets for users
    create_wallet(N(username1111));
    auto usr1111_balance = get_token_balance(N(username1111));
    REQUIRE_MATCHING_OBJECT(usr1111_balance, mvo()
        ("owner", "username1111")
        ("tokens", "0.0000 VIITA")
    );

    create_wallet(N(username1112));
    auto usr1112_balance = get_token_balance(N(username1112));
    REQUIRE_MATCHING_OBJECT(usr1112_balance, mvo()
        ("owner", "username1112")
        ("tokens", "0.0000 VIITA")
    );

    create_wallet(N(username1113));
    auto usr1113_balance = get_token_balance(N(username1113));
    REQUIRE_MATCHING_OBJECT(usr1113_balance, mvo()
        ("owner", "username1113")
        ("tokens", "0.0000 VIITA")
    );

    produce_blocks();

    //TEST: delete zero-balance wallet
    delete_wallet(N(username1113));
    usr1113_balance = get_token_balance(N(username1113));
    BOOST_REQUIRE_EQUAL(true, usr1113_balance.is_null());

    produce_blocks();

    //TEST: attempt malicious wallet actions....

    //TEST: attempt to delete non-existent wallet
    BOOST_REQUIRE_EXCEPTION(
		delete_wallet(N(username1114)),
		eosio_assert_message_exception, 
		eosio_assert_message_is( "Account does not have a wallet to delete" ) 
	);

    //TEST: mint balance, attempt to delete wallet with balance
    token_mint(N(username1112), asset(500000, sym), publisher);
    BOOST_REQUIRE_EXCEPTION(
		delete_wallet(N(username1112)),
		eosio_assert_message_exception, 
		eosio_assert_message_is( "Cannot delete wallet unless balance is zero" ) 
	);

    produce_blocks();

}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(minting, token_registry_tester) try {
	
    account_name publisher = name("thepublisher");
    symbol sym = symbol(4, "VIITA");
	asset max_supply = asset(100000000000000, sym); 
	asset supply = asset(0, sym);
	produce_blocks();

    token_mint(publisher, asset(500000, sym), publisher);

    //TEST: check balance of publisher
    auto pub_balance = get_token_balance(publisher);
    BOOST_REQUIRE_EQUAL(pub_balance["owner"], "thepublisher");
    BOOST_REQUIRE_EQUAL(pub_balance["tokens"], "50.0000 VIITA");

    //TEST: check circulating supply
    auto token_config = get_token_config(publisher);
    REQUIRE_MATCHING_OBJECT(token_config, mvo()
        ("publisher", "thepublisher")
        ("token_name", "Viita")
        ("max_supply", "10000000000.0000 VIITA")
        ("supply", "50.0000 VIITA")
    );

    //TEST: attempting malicious mint actions.....

    //TEST: create and check non-publisher account exists
    create_wallet(N(username1111));
    auto usr1111_balance = get_token_balance(N(username1111));
    REQUIRE_MATCHING_OBJECT(usr1111_balance, mvo()
        ("owner", "username1111")
        ("tokens", "0.0000 VIITA")
    );

    //TEST: attempt to mint from non-publisher account
    // BOOST_REQUIRE_EXCEPTION(
	// 	token_mint(N(username1111), asset(500000, sym), N(username1111)),
	// 	eosio_assert_message_exception,
	// 	eosio_assert_message_is( "Missing required authority" ) 
	// );

    //TEST: attempt to mint over max_supply
    BOOST_REQUIRE_EXCEPTION(
		token_mint(publisher, asset(500000000000000, sym), publisher),
		eosio_assert_message_exception, 
		eosio_assert_message_is( "minting would exceed allowed maximum supply" ) 
	);

    //TEST: attempt to mint negative tokens
    BOOST_REQUIRE_EXCEPTION(
		token_mint(publisher, asset(-500000, sym), publisher),
		eosio_assert_message_exception, 
		eosio_assert_message_is( "must mint positive quantity" ) 
	);

    //TEST: attempt to mint different symbol
    BOOST_REQUIRE_EXCEPTION(
		token_mint(publisher, asset(500000, symbol(4, "TEST")), publisher),
		eosio_assert_message_exception, 
		eosio_assert_message_is( "only native tokens are mintable" ) 
	);

}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transfers, token_registry_tester) try {
	
    account_name publisher = name("thepublisher");
    symbol sym = symbol(4, "VIITA");
	asset max_supply = asset(100000000000000, sym); 
	asset supply = asset(0, sym);
	produce_blocks();

    token_mint(publisher, asset(500000, sym), publisher); //NOTE: mint 50

    create_wallet(N(username1111));
    create_wallet(N(username1112));

    //TEST: check usernames have zero-balance wallets
    auto usr1111_balance = get_token_balance(N(username1111));
    REQUIRE_MATCHING_OBJECT(usr1111_balance, mvo()
        ("owner", "username1111")
        ("tokens", "0.0000 VIITA")
    );

    auto usr1112_balance = get_token_balance(N(username1112));
    REQUIRE_MATCHING_OBJECT(usr1112_balance, mvo()
        ("owner", "username1112")
        ("tokens", "0.0000 VIITA")
    );

    token_transfer(publisher, N(username1111), asset(250000, sym)); //NOTE: send 25

    //TEST: check sender has amount removed from balance
    auto pub_balance = get_token_balance(N(thepublisher));
    REQUIRE_MATCHING_OBJECT(pub_balance, mvo()
        ("owner", "thepublisher")
        ("tokens", "25.0000 VIITA")
    );

    //TEST: check recipient has amount added to balance
    usr1111_balance = get_token_balance(N(username1111));
    REQUIRE_MATCHING_OBJECT(usr1111_balance, mvo()
        ("owner", "username1111")
        ("tokens", "25.0000 VIITA")
    );

    token_transfer(N(username1111), N(username1112), asset(50000, sym)); //NOTE: send 5

    //TEST: check sender has 20 remaining
    usr1111_balance = get_token_balance(N(username1111));
    REQUIRE_MATCHING_OBJECT(usr1111_balance, mvo()
        ("owner", "username1111")
        ("tokens", "20.0000 VIITA")
    );

    //TEST: check recipient has 5 in balance
    usr1112_balance = get_token_balance(N(username1112));
    REQUIRE_MATCHING_OBJECT(usr1112_balance, mvo()
        ("owner", "username1112")
        ("tokens", "5.0000 VIITA")
    );

    //TEST: attempting malicious transfer actions..... 

    //TEST: transfer to nonexistant account
    BOOST_REQUIRE_EXCEPTION(
		token_transfer(N(username1111), N(craig), asset(10000, sym)), //NOTE: send 1
		eosio_assert_message_exception, 
		eosio_assert_message_is( "recipient account does not exist" ) 
	);

    //TEST: transfer to self
    BOOST_REQUIRE_EXCEPTION(
		token_transfer(N(username1111), N(username1111), asset(10000, sym)), //NOTE: send 1
		eosio_assert_message_exception, 
		eosio_assert_message_is( "cannot transfer to self" ) 
	);

    //TEST: transfer negative quantity
    BOOST_REQUIRE_EXCEPTION(
		token_transfer(N(username1111), N(username1112), asset(-10000, sym)), //NOTE: send 1
		eosio_assert_message_exception, 
		eosio_assert_message_is( "must transfer positive quantity" ) 
	);

    //TEST: transfer other symbol
    BOOST_REQUIRE_EXCEPTION(
		token_transfer(N(username1111), N(username1112), asset(10000, symbol(4, "TEST"))), //NOTE: send 1
		eosio_assert_message_exception, 
		eosio_assert_message_is( "only native tokens are transferable" ) 
	);

}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(allotments, token_registry_tester) try {
	
    account_name publisher = name("thepublisher");
    symbol sym = symbol(4, "VIITA");
	asset max_supply = asset(100000000000000, sym); 
	asset supply = asset(0, sym);
	produce_blocks();

    token_mint(publisher, asset(500000, sym), publisher); //NOTE: mint 50

    //TEST: tokens minted to publisher balance
    auto pub_balance = get_token_balance(N(thepublisher));
    REQUIRE_MATCHING_OBJECT(pub_balance, mvo()
        ("owner", "thepublisher")
        ("tokens", "50.0000 VIITA")
    );

    create_wallet(N(username1111));
    create_wallet(N(username1112));

    produce_blocks();

    //TEST: check usernames have zero-balance wallets
    auto usr1111_balance = get_token_balance(N(username1111));
    REQUIRE_MATCHING_OBJECT(usr1111_balance, mvo()
        ("owner", "username1111")
        ("tokens", "0.0000 VIITA")
    );

    auto usr1112_balance = get_token_balance(N(username1112));
    REQUIRE_MATCHING_OBJECT(usr1112_balance, mvo()
        ("owner", "username1112")
        ("tokens", "0.0000 VIITA")
    );

    token_allot(publisher, N(username1111), asset(250000, sym)); //NOTE: allot 25

    produce_blocks();

    //TEST: check allotment removed from balance
    pub_balance = get_token_balance(N(thepublisher));
    REQUIRE_MATCHING_OBJECT(pub_balance, mvo()
        ("owner", "thepublisher")
        ("tokens", "25.0000 VIITA")
    );

    //TEST: allotment amount added to recipient's allotment
    auto usr1111_allot = get_token_allotment(publisher, N(username1111));
    REQUIRE_MATCHING_OBJECT(usr1111_allot, mvo()
        ("recipient", "username1111")
        ("sender", "thepublisher")
        ("tokens", "25.0000 VIITA")
    );

    //TEST: test multiple existing allotments
    token_allot(publisher, N(username1112), asset(50000, sym)); //NOTE: allot 5

    produce_blocks();

    //TEST: allotment removed from publisher balance
    pub_balance = get_token_balance(N(thepublisher));
    REQUIRE_MATCHING_OBJECT(pub_balance, mvo()
        ("owner", "thepublisher")
        ("tokens", "20.0000 VIITA")
    );

    //TEST: check allotment amount for second user
    auto usr1112_allot = get_token_allotment(publisher, N(username1112));
    REQUIRE_MATCHING_OBJECT(usr1112_allot, mvo()
        ("recipient", "username1112")
        ("sender", "thepublisher")
        ("tokens", "5.0000 VIITA")
    );

    token_claimallot(publisher, N(username1111), asset(100000, sym)); //NOTE: claim 10 of 25

    produce_blocks();

    //TEST: check allotment remains, minus amount claimed
    usr1111_allot = get_token_allotment(publisher, N(username1111));
    REQUIRE_MATCHING_OBJECT(usr1111_allot, mvo()
        ("recipient", "username1111")
        ("sender", "thepublisher")
        ("tokens", "15.0000 VIITA")
    );

    //TEST: check recipient balance has claimed amount added
    usr1111_balance = get_token_balance(N(username1111));
    REQUIRE_MATCHING_OBJECT(usr1111_balance, mvo()
        ("owner", "username1111")
        ("tokens", "10.0000 VIITA")
    );

    //TEST: unallot tokens back to sender balance
    token_unallot(publisher, N(username1111), asset(50000, sym)); //NOTE: unallot 5 of remining 15

    produce_blocks();

    //TEST: check allotment remains, minus amount unalloted
    usr1111_allot = get_token_allotment(publisher, N(username1111));
    REQUIRE_MATCHING_OBJECT(usr1111_allot, mvo()
        ("recipient", "username1111")
        ("sender", "thepublisher")
        ("tokens", "10.0000 VIITA")
    );

    //TEST: check sender balance has unalloted amount added
    pub_balance = get_token_balance(N(thepublisher));
    REQUIRE_MATCHING_OBJECT(pub_balance, mvo()
        ("owner", "thepublisher")
        ("tokens", "25.0000 VIITA")
    );

    token_claimallot(publisher, N(username1111), asset(100000, sym)); //NOTE: claim remaining 10

    produce_blocks();

    //TEST: check allotment is erased
    usr1111_allot = get_token_allotment(publisher, N(username1111));
    BOOST_REQUIRE_EQUAL(true, usr1111_allot.is_null());

    //TEST: check recipient balance for added amount
    usr1111_balance = get_token_balance(N(username1111));
    REQUIRE_MATCHING_OBJECT(usr1111_balance, mvo()
        ("owner", "username1111")
        ("tokens", "20.0000 VIITA")
    );

    //TEST: attempting malicious claimallot() actions.....

    //TEST: attempt to claim allotment when not intended recipient
    // BOOST_REQUIRE_EXCEPTION(
	// 	token_claimallot(publisher, N(username1113), asset(100000, sym)),
	// 	eosio_assert_message_exception, 
	// 	eosio_assert_message_is( "only native tokens are transferable" ) 
	// );
    
    //TEST: attempt to claim negative quantity
    BOOST_REQUIRE_EXCEPTION(
		token_claimallot(publisher, N(username1112), asset(-10000, sym)),
		eosio_assert_message_exception, 
		eosio_assert_message_is( "must claim positive quantity" ) 
	);

    //TEST: attempt to claim wrong symbol
    BOOST_REQUIRE_EXCEPTION(
		token_claimallot(publisher, N(username1112), asset(10000, symbol(4, "TEST"))),
		eosio_assert_message_exception, 
		eosio_assert_message_is( "only native tokens are claimable" ) 
	);

}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()