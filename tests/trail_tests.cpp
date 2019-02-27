#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include "contracts.hpp"
#include "trail_tester.hpp"

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(trail_tests)

BOOST_FIXTURE_TEST_CASE(simple_flow, trail_tester ) try {

    std::cout << ">>>>>>>>>>>>>>>>>>>>>>> BEGIN SIMPLE_FLOW >>>>>>>>>>>>>>>>>>>>>>>" << std::endl;

    //ready ballot
    auto bal_end_time = now() + BALLOT_LENGTH;
    readyballot(BALLOT_NAME, N(testaccount1), bal_end_time);
    produce_blocks();

    // vector<option> emplaced_options = {
    //     option{YES_NAME, "Yes", asset(0, VOTE_SYM)},
    //     option{NO_NAME, "No", asset(0, VOTE_SYM)},
    //     option{ABSTAIN_NAME, "Abstain", asset(0, VOTE_SYM)}
    // };

    // struct option {
    //     name option_name;
    //     string info;
    //     asset votes;
    // };

    // vector<mvo> emplaced_options = {
    //     mvo() ("option_name", YES_NAME) ("info", "Yes") ("votes", asset(0, VOTE_SYM).to_string()),
    //     mvo() ("option_name", NO_NAME) ("info", "No") ("votes", asset(0, VOTE_SYM).to_string()),
    //     mvo() ("option_name", ABSTAIN_NAME) ("info", "Abstain") ("votes", asset(0, VOTE_SYM).to_string())
    // };
    
    //check ballot emplacement
    auto bal = get_ballot(BALLOT_NAME);
    // REQUIRE_MATCHING_OBJECT(bal, mvo()
    //     ("ballot_name", BALLOT_NAME)
    //     ("category", CATEGORY)
    //     ("publisher", name("testaccount1"))
    //     ("title", TITLE)
    //     ("description", DESC)
    //     ("info_url", URL)
    //     ("options", fc::variant(std::move(emplaced_options)))
    //     ("unique_voters", 0)
    //     ("max_votable_options", MAX_VOTABLE_OPTIONS)
    //     ("voting_symbol", VOTE_SYM)
    //     ("begin_time", now())
    //     ("end_time", bal_end_time)
    //     ("status", 1)
	// );

    //vote
    castvote(N(testaccount1), BALLOT_NAME, YES_NAME);
    castvote(N(testaccount2), BALLOT_NAME, NO_NAME);
    castvote(N(testaccount3), BALLOT_NAME, YES_NAME);
    castvote(N(testaccount4), BALLOT_NAME, ABSTAIN_NAME);
    castvote(N(testaccount5), BALLOT_NAME, YES_NAME);
    produce_blocks();
    

    //check vote objects exist
    auto v1 = get_vote(N(testaccount1), BALLOT_NAME);
    auto v2 = get_vote(N(testaccount2), BALLOT_NAME);
    auto v3 = get_vote(N(testaccount3), BALLOT_NAME);
    auto v4 = get_vote(N(testaccount4), BALLOT_NAME);
    auto v5 = get_vote(N(testaccount5), BALLOT_NAME);

    //check votes exist
    REQUIRE_MATCHING_OBJECT(v1, mvo()
        ("ballot_name", BALLOT_NAME)
        ("option_names", vector<name>({name("yes")}))
        ("amount", "1000000 VOTE")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(v2, mvo()
        ("ballot_name", BALLOT_NAME)
        ("option_names", vector<name>({name("no")}))
        ("amount", "2000000 VOTE")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(v3, mvo()
        ("ballot_name", BALLOT_NAME)
        ("option_names", vector<name>({name("yes")}))
        ("amount", "3000000 VOTE")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(v4, mvo()
        ("ballot_name", BALLOT_NAME)
        ("option_names", vector<name>({name("abstain")}))
        ("amount", "4000000 VOTE")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(v5, mvo()
        ("ballot_name", BALLOT_NAME)
        ("option_names", vector<name>({name("yes")}))
        ("amount", "5000000 VOTE")
        ("expiration", bal_end_time)
	);

    //check num_votes were incremented
    auto a1 = get_balance(N(testaccount1), VOTE_SYM);
    auto a2 = get_balance(N(testaccount2), VOTE_SYM);
    auto a3 = get_balance(N(testaccount3), VOTE_SYM);
    auto a4 = get_balance(N(testaccount4), VOTE_SYM);
    auto a5 = get_balance(N(testaccount5), VOTE_SYM);
    
    REQUIRE_MATCHING_OBJECT(a1, mvo()
        ("balance", "1000000 VOTE")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(a2, mvo()
        ("balance", "2000000 VOTE")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(a3, mvo()
        ("balance", "3000000 VOTE")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(a4, mvo()
        ("balance", "4000000 VOTE")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(a5, mvo()
        ("balance", "5000000 VOTE")
        ("num_votes", 1)
	);

    //check ballot updated with casted votes


    //unvote v1
    unvote(N(testaccount1), BALLOT_NAME, YES_NAME);

    //check vote change
    v1 = get_vote(N(testaccount1), BALLOT_NAME);
    // REQUIRE_MATCHING_OBJECT(v1, mvo()
    //     ("ballot_name", BALLOT_NAME)
    //     ("option_names", vector<name>({}))
    //     ("amount", "1000000 VOTE")
    //     ("expiration", bal_end_time)
	// );

    //revote for different option
    castvote(N(testaccount1), BALLOT_NAME, ABSTAIN_NAME);
    produce_blocks();

    //check revote exists
    v1 = get_vote(N(testaccount1), BALLOT_NAME);
    REQUIRE_MATCHING_OBJECT(v1, mvo()
        ("ballot_name", BALLOT_NAME)
        ("option_names", vector<name>({name("abstain")}))
        ("amount", "1000000 VOTE")
        ("expiration", bal_end_time)
	);

    //fast forward past expiration time
    produce_block(fc::seconds(86400)); //seconds in a day
    produce_blocks();

    //close ballot
    closeballot(BALLOT_NAME, N(testaccount1), CLOSED);
    produce_blocks();

    //check ballot status is closed
    bal = get_ballot(BALLOT_NAME);
    // REQUIRE_MATCHING_OBJECT(bal, mvo()
    //     ("ballot_name", BALLOT_NAME)
    //     ("category", CATEGORY)
    //     ("publisher", name("testaccount1"))
    //     ("title", TITLE)
    //     ("description", DESC)
    //     ("info_url", URL)
    //     ("options", emplaced_options)
    //     ("unique_voters", 0)
    //     ("max_votable_options", MAX_VOTABLE_OPTIONS)
    //     ("voting_symbol", VOTE_SYM)
    //     ("begin_time", now())
    //     ("end_time", bal_end_time)
    //     ("status", 1)
	// );

    //cleanupvotes
    auto trx_pointer = cleanupvotes(N(testaccount1), 21, VOTE_SYM);
    std::cout << trx_pointer->elapsed.count() << std::endl;

    cleanupvotes(N(testaccount2), 21, VOTE_SYM);
    cleanupvotes(N(testaccount3), 21, VOTE_SYM);
    cleanupvotes(N(testaccount4), 21, VOTE_SYM);
    cleanupvotes(N(testaccount5), 21, VOTE_SYM);
    produce_blocks();

    //check all vote objects have been cleaned
    v1 = get_vote(N(testaccount1), BALLOT_NAME);
    v2 = get_vote(N(testaccount2), BALLOT_NAME);
    v3 = get_vote(N(testaccount3), BALLOT_NAME);
    v4 = get_vote(N(testaccount4), BALLOT_NAME);
    v5 = get_vote(N(testaccount5), BALLOT_NAME);
    BOOST_REQUIRE_EQUAL(true, v1.is_null());
    BOOST_REQUIRE_EQUAL(true, v2.is_null());
    BOOST_REQUIRE_EQUAL(true, v3.is_null());
    BOOST_REQUIRE_EQUAL(true, v4.is_null());
    BOOST_REQUIRE_EQUAL(true, v5.is_null());

    std::cout << "<<<<<<<<<<<<<<<<<<<<<<< END SIMPLE_FLOW <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
	
} FC_LOG_AND_RETHROW()



BOOST_FIXTURE_TEST_CASE(multi_ballot_flow, trail_tester ) try {

    std::cout << ">>>>>>>>>>>>>>>>>>>>>>> BEGIN MULTI_BALLOT_FLOW >>>>>>>>>>>>>>>>>>>>>>>" << std::endl;

    //create vectors of names for use in testing
    vector<name> ballots = {
        name("ballot1"),
        name("ballot2"),
        name("ballot3"),
        name("ballot4"),
        name("ballot5"),
        name("ballot11"),
        name("ballot12"),
        name("ballot13"),
        name("ballot14"),
        name("ballot15"),
        name("ballot21"),
        name("ballot22"),
        name("ballot23"),
        name("ballot24"),
        name("ballot25"),
        name("ballot31"),
        name("ballot32"),
        name("ballot33"),
        name("ballot34"),
        name("ballot35"),
        name("ballot41")
    };
    
    vector<name> voters = {
        name("testaccount1"),
        name("testaccount2"),
        name("testaccount3"),
        name("testaccount4"),
        name("testaccount5"),
    };

    vector<name> poll_options = {
        name("yes"),
        name("no"),
        name("abstain")
    };

    //fast forward past chain activation time
    //produce_block(fc::seconds(500000)); //1 million blocks
    produce_blocks(1000000);

    //make and ready ballots x 21
    for (int i = 0; i < 21; i++) {
        newballot(ballots[i], CATEGORY, voters[i % 5], TITLE, DESC, URL, MAX_VOTABLE_OPTIONS, VOTE_SYM);
        //std::cout << ballots[i] << std::endl;
		produce_blocks();

        //add options to new ballot
		addoption(ballots[i], voters[i % 5], YES_NAME, "Yes");
		addoption(ballots[i], voters[i % 5], NO_NAME, "No");
		addoption(ballots[i], voters[i % 5], ABSTAIN_NAME, "Abstain");
        produce_blocks();

        //ready new ballot
        readyballot(ballots[i], voters[i % 5], now() + 86400);
        produce_blocks();

        //check emplacement
        auto bal = get_ballot(ballots[i]);
        BOOST_REQUIRE_EQUAL(false, bal.is_null());

        //loop over each voter, vote on new ballot
        for (int j = 0; j < voters.size(); j++) {
            castvote(voters[j], ballots[i], poll_options[i % 3]);
            produce_blocks();
            
            //check vote was emplaced
            auto v = get_vote(voters[j], ballots[i]);
            BOOST_REQUIRE_EQUAL(false, v.is_null());
            // REQUIRE_MATCHING_OBJECT(v, mvo()
            //     ("ballot_name", ballots[i])
            //     ("option_names", vector<name>({name("abstain")}))
            //     ("amount", "1000000 VOTE")
            //     ("expiration", bal_end_time)
            // );
        }

        //rebalance some accounts

    }

    //ready default ballot (use as 22nd ballot for testing max votes)
    readyballot(name("testballot"), name("testaccount1"), now() + 86400);
    produce_blocks();

    //attempt to cast 22nd vote, should fail
    //castvote(N(testaccount1), BALLOT_NAME, NO_NAME);

    // auto trx_pointer = cleanupvotes(N(testaccount1), 21, VOTE_SYM);
    // std::cout << trx_pointer->elapsed.count() << std::endl;

    //undelegatebw, trigger cleanup votes
    auto trx_pointer = undelegatebw(N(testaccount1), N(testaccount1), asset::from_string("1.0000 TLOS"), asset::from_string("1.0000 TLOS"));
    std::cout << trx_pointer->elapsed.count() << std::endl;

    std::cout << "<<<<<<<<<<<<<<<<<<<<<<< END MULTI_BALLOT_FLOW <<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
	
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()