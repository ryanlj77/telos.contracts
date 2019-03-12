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
        ("amount", "100.0000 VOTE")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(v2, mvo()
        ("ballot_name", BALLOT_NAME)
        ("option_names", vector<name>({name("no")}))
        ("amount", "200.0000 VOTE")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(v3, mvo()
        ("ballot_name", BALLOT_NAME)
        ("option_names", vector<name>({name("yes")}))
        ("amount", "300.0000 VOTE")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(v4, mvo()
        ("ballot_name", BALLOT_NAME)
        ("option_names", vector<name>({name("abstain")}))
        ("amount", "400.0000 VOTE")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(v5, mvo()
        ("ballot_name", BALLOT_NAME)
        ("option_names", vector<name>({name("yes")}))
        ("amount", "500.0000 VOTE")
        ("expiration", bal_end_time)
	);

    //check num_votes were incremented
    auto a1 = get_balance(N(testaccount1), VOTE_SYM);
    auto a2 = get_balance(N(testaccount2), VOTE_SYM);
    auto a3 = get_balance(N(testaccount3), VOTE_SYM);
    auto a4 = get_balance(N(testaccount4), VOTE_SYM);
    auto a5 = get_balance(N(testaccount5), VOTE_SYM);
    
    REQUIRE_MATCHING_OBJECT(a1, mvo()
        ("balance", "100.0000 VOTE")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(a2, mvo()
        ("balance", "200.0000 VOTE")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(a3, mvo()
        ("balance", "300.0000 VOTE")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(a4, mvo()
        ("balance", "400.0000 VOTE")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(a5, mvo()
        ("balance", "500.0000 VOTE")
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
        ("amount", "100.0000 VOTE")
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
    auto trx_pointer1 = cleanupvotes(N(testaccount1), 51, VOTE_SYM);
    std::cout << name("testaccount1") << " charged " << trx_pointer1->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount1") << " charged " << trx_pointer1->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount1") << " charged " << trx_pointer1->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer2 = cleanupvotes(N(testaccount2), 51, VOTE_SYM);
    std::cout << name("testaccount2") << " charged " << trx_pointer2->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount2") << " charged " << trx_pointer2->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount2") << " charged " << trx_pointer2->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer3 = cleanupvotes(N(testaccount3), 51, VOTE_SYM);
    std::cout << name("testaccount3") << " charged " << trx_pointer3->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount3") << " charged " << trx_pointer3->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount3") << " charged " << trx_pointer3->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer4 = cleanupvotes(N(testaccount4), 51, VOTE_SYM);
    std::cout << name("testaccount4") << " charged " << trx_pointer4->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount4") << " charged " << trx_pointer4->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount4") << " charged " << trx_pointer4->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer5 = cleanupvotes(N(testaccount5), 51, VOTE_SYM);
    std::cout << name("testaccount5") << " charged " << trx_pointer5->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount5") << " charged " << trx_pointer5->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount5") << " charged " << trx_pointer5->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
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
        // name("ballot1"),
        // name("ballot2"),
        // name("ballot3"),
        // name("ballot4"),
        // name("ballot5"),
        // name("ballot11"),
        // name("ballot12"),
        // name("ballot13"),
        // name("ballot14"),
        // name("ballot15"),
        // name("ballot21"),
        // name("ballot22"),
        // name("ballot23"),
        // name("ballot24"),
        // name("ballot25"),
        // name("ballot31"),
        // name("ballot32"),
        // name("ballot33"),
        // name("ballot34"),
        // name("ballot35"),
        // name("ballot41")
    };

    //generate ballot name x MAX_VOTES_PER_ACCOUNT
    for (int i = 0; i < MAX_VOTES_PER_ACCOUNT; i++) {
        string temp_name = "ballot" + toBase31(i);
        name bal_name = eosio::chain::name(temp_name);
        std::cout << "created " << bal_name << std::endl;
        ballots.emplace_back(bal_name);
    }
    
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

    //fast forward past chain activation time (testing is set to 1000 blocks)
    produce_blocks(1000);

    //make and ready all ballots
    for (int i = 0; i < MAX_VOTES_PER_ACCOUNT; i++) {
        newballot(ballots[i], CATEGORY, N(testaccount1), TITLE, DESC, URL, MAX_VOTABLE_OPTIONS, VOTE_SYM);
        //std::cout << ballots[i] << std::endl;
		produce_blocks();

        //add options to new ballot
		addoption(ballots[i], N(testaccount1), YES_NAME, "Yes");
		addoption(ballots[i], N(testaccount1), NO_NAME, "No");
		addoption(ballots[i], N(testaccount1), ABSTAIN_NAME, "Abstain");
        produce_blocks();

        //ready new ballot
        bal_end_time = now() + 86400;
        readyballot(ballots[i], N(testaccount1), bal_end_time);
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

    }

    //ready default ballot (use as 22nd ballot for testing max votes)
    readyballot(name("testballot"), name("testaccount1"), now() + 86400);
    produce_blocks();

    //attempt to cast 22nd vote, should fail
    //castvote(N(testaccount1), BALLOT_NAME, NO_NAME);

    std::cout << "executing undelegatebw/rebalances >>>>>" << std::endl;

    //undelegatebw, trigger cleanup votes
    auto trx_pointer1 = undelegatebw(N(testaccount1), N(testaccount1), asset::from_string("1.0000 TLOS"), asset::from_string("1.0000 TLOS"));
    std::cout << name("testaccount1") << " charged " << trx_pointer1->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount1") << " charged " << trx_pointer1->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount1") << " charged " << trx_pointer1->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer2 = undelegatebw(N(testaccount2), N(testaccount2), asset::from_string("1.0000 TLOS"), asset::from_string("1.0000 TLOS"));
    std::cout << name("testaccount2") << " charged " << trx_pointer2->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount2") << " charged " << trx_pointer2->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount2") << " charged " << trx_pointer2->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer3 = undelegatebw(N(testaccount3), N(testaccount3), asset::from_string("1.0000 TLOS"), asset::from_string("1.0000 TLOS"));
    std::cout << name("testaccount3") << " charged " << trx_pointer3->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount3") << " charged " << trx_pointer3->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount3") << " charged " << trx_pointer3->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer4 = undelegatebw(N(testaccount4), N(testaccount4), asset::from_string("1.0000 TLOS"), asset::from_string("1.0000 TLOS"));
    std::cout << name("testaccount4") << " charged " << trx_pointer4->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount4") << " charged " << trx_pointer4->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount4") << " charged " << trx_pointer4->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer5 = undelegatebw(N(testaccount5), N(testaccount5), asset::from_string("1.0000 TLOS"), asset::from_string("1.0000 TLOS"));
    std::cout << name("testaccount5") << " charged " << trx_pointer5->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount5") << " charged " << trx_pointer5->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount5") << " charged " << trx_pointer5->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    //check votes are rebalanced
    for (name b : ballots) {
        for (name v : voters) {
            auto vo = get_vote(v, b);
            //ensures votes were not deleted
            BOOST_REQUIRE_EQUAL(false, vo.is_null());
        }
    }

    //fast forward to ballot expirations
    produce_blocks(172800); //1 day in blocks

    std::cout << "executing cleanupvotes >>>>>" << std::endl;

    //cleanup votes (actually deletes them)
    auto trx_pointer11 = cleanupvotes(N(testaccount1), 51, VOTE_SYM);
    std::cout << name("testaccount1") << " charged " << trx_pointer11->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount1") << " charged " << trx_pointer11->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount1") << " charged " << trx_pointer11->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer22 = cleanupvotes(N(testaccount2), 51, VOTE_SYM);
    std::cout << name("testaccount2") << " charged " << trx_pointer22->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount2") << " charged " << trx_pointer22->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount2") << " charged " << trx_pointer22->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer33 = cleanupvotes(N(testaccount3), 51, VOTE_SYM);
    std::cout << name("testaccount3") << " charged " << trx_pointer33->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount3") << " charged " << trx_pointer33->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount3") << " charged " << trx_pointer33->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer44 = cleanupvotes(N(testaccount4), 51, VOTE_SYM);
    std::cout << name("testaccount4") << " charged " << trx_pointer44->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount4") << " charged " << trx_pointer44->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount4") << " charged " << trx_pointer44->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer55 = cleanupvotes(N(testaccount5), 51, VOTE_SYM);
    std::cout << name("testaccount5") << " charged " << trx_pointer55->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount5") << " charged " << trx_pointer55->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount5") << " charged " << trx_pointer55->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    //check that votes have been cleaned
    for (name v : voters) {
        std::cout << "cleaning votes for " << v << std::endl;
        for (name b : ballots) {
            auto vo = get_vote(v, b);
            //ensures votes were cleaned
            BOOST_REQUIRE_EQUAL(true, vo.is_null());
        }
    }

    //close ballots
    for (name b : ballots) {
        closeballot(b, N(testaccount1), 2);
        produce_blocks();
        //ensures ballots were closed
        auto bal = get_ballot(b);
        //BOOST_REQUIRE_EQUAL(false, bal.is_null());
    }

    //fast forward past ballot cooldown so they can be deleted
    produce_blocks(BALLOT_COOLDOWN * 2);

    //delete old ballots
    for (name b : ballots) {
        deleteballot(b, N(testaccount1));
        produce_blocks();
        //ensures ballots were closed
        auto bal = get_ballot(b);
        BOOST_REQUIRE_EQUAL(true, bal.is_null());
    }
    
    std::cout << "<<<<<<<<<<<<<<<<<<<<<<< END MULTI_BALLOT_FLOW <<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
	
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(custom_token_flow, trail_tester ) try {

    std::cout << ">>>>>>>>>>>>>>>>>>>>>>> BEGIN CUSTOM_TOKEN_FLOW >>>>>>>>>>>>>>>>>>>>>>>" << std::endl;

    //init CRAIG token
    const symbol CRAIG_SYM = symbol(2, "CRAIG"); //heh
    const asset custom_max_supply = asset::from_string("1000.00 CRAIG");
    token_settings craig_settings = {
        true, //is_descructible
        true, //is_proxyable
        true, //is_burnable
        true, //is_seizable
        true, //is_max_mutable
        true //is_transferable`
    };
    string info_url = "Craig Token Registry";
    newtoken(N(testaccount1), custom_max_supply, craig_settings, info_url);
    produce_blocks();

    //init CRAIG ballot
    const name CUSTOM_BALLOT_NAME = name("customballot");
    const name CUSTOM_PUBLISHER = name("testaccount1");
    const string CUSTOM_TITLE = "Custom Ballot";
    const string CUSTOM_DESC = "Custom Description";
    const string CUSTOM_URL = "/ipfs/customipfscid";
    newballot(CUSTOM_BALLOT_NAME, name("games"), CUSTOM_PUBLISHER, CUSTOM_TITLE, CUSTOM_DESC, CUSTOM_URL, MAX_VOTABLE_OPTIONS, CRAIG_SYM);
    produce_blocks();

    //addoptions
    addoption(CUSTOM_BALLOT_NAME, N(testaccount1), YES_NAME, "Yes");
    addoption(CUSTOM_BALLOT_NAME, N(testaccount1), NO_NAME, "No");
    addoption(CUSTOM_BALLOT_NAME, N(testaccount1), ABSTAIN_NAME, "Abstain");
    produce_blocks();

    //ready ballot
    auto bal_end_time = now() + BALLOT_LENGTH;
    readyballot(CUSTOM_BALLOT_NAME, N(testaccount1), bal_end_time);
    produce_blocks();

    //regvoter for each test account
    regvoter(N(testaccount1), CRAIG_SYM);
    regvoter(N(testaccount2), CRAIG_SYM);
    regvoter(N(testaccount3), CRAIG_SYM);
    regvoter(N(testaccount4), CRAIG_SYM);
    regvoter(N(testaccount5), CRAIG_SYM);
    produce_blocks();

    //mint CRAIG tokens to test accounts
    mint(CUSTOM_PUBLISHER, N(testaccount1), asset(10000, CRAIG_SYM));
    mint(CUSTOM_PUBLISHER, N(testaccount2), asset(10000, CRAIG_SYM));
    mint(CUSTOM_PUBLISHER, N(testaccount3), asset(10000, CRAIG_SYM));
    mint(CUSTOM_PUBLISHER, N(testaccount4), asset(10000, CRAIG_SYM));
    mint(CUSTOM_PUBLISHER, N(testaccount5), asset(10000, CRAIG_SYM));
    produce_blocks();

    //cast votes
    castvote(N(testaccount1), CUSTOM_BALLOT_NAME, YES_NAME);
    castvote(N(testaccount2), CUSTOM_BALLOT_NAME, NO_NAME);
    castvote(N(testaccount3), CUSTOM_BALLOT_NAME, YES_NAME);
    castvote(N(testaccount4), CUSTOM_BALLOT_NAME, ABSTAIN_NAME);
    castvote(N(testaccount5), CUSTOM_BALLOT_NAME, YES_NAME);
    produce_blocks();

    //get vote objects
    auto cv1 = get_vote(N(testaccount1), CUSTOM_BALLOT_NAME);
    auto cv2 = get_vote(N(testaccount2), CUSTOM_BALLOT_NAME);
    auto cv3 = get_vote(N(testaccount3), CUSTOM_BALLOT_NAME);
    auto cv4 = get_vote(N(testaccount4), CUSTOM_BALLOT_NAME);
    auto cv5 = get_vote(N(testaccount5), CUSTOM_BALLOT_NAME);

    //check votes exist and match casted votes
    REQUIRE_MATCHING_OBJECT(cv1, mvo()
        ("ballot_name", CUSTOM_BALLOT_NAME)
        ("option_names", vector<name>({name("yes")}))
        ("amount", "100.00 CRAIG")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(cv2, mvo()
        ("ballot_name", CUSTOM_BALLOT_NAME)
        ("option_names", vector<name>({name("no")}))
        ("amount", "100.00 CRAIG")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(cv3, mvo()
        ("ballot_name", CUSTOM_BALLOT_NAME)
        ("option_names", vector<name>({name("yes")}))
        ("amount", "100.00 CRAIG")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(cv4, mvo()
        ("ballot_name", CUSTOM_BALLOT_NAME)
        ("option_names", vector<name>({name("abstain")}))
        ("amount", "100.00 CRAIG")
        ("expiration", bal_end_time)
	);
    REQUIRE_MATCHING_OBJECT(cv5, mvo()
        ("ballot_name", CUSTOM_BALLOT_NAME)
        ("option_names", vector<name>({name("yes")}))
        ("amount", "100.00 CRAIG")
        ("expiration", bal_end_time)
	);

    //get voting accounts
    auto ca1 = get_balance(N(testaccount1), CRAIG_SYM);
    auto ca2 = get_balance(N(testaccount2), CRAIG_SYM);
    auto ca3 = get_balance(N(testaccount3), CRAIG_SYM);
    auto ca4 = get_balance(N(testaccount4), CRAIG_SYM);
    auto ca5 = get_balance(N(testaccount5), CRAIG_SYM);
    
    //check voting accounts match minted amount, num votes incremented
    REQUIRE_MATCHING_OBJECT(ca1, mvo()
        ("balance", "100.00 CRAIG")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(ca2, mvo()
        ("balance", "100.00 CRAIG")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(ca3, mvo()
        ("balance", "100.00 CRAIG")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(ca4, mvo()
        ("balance", "100.00 CRAIG")
        ("num_votes", 1)
	);
    REQUIRE_MATCHING_OBJECT(ca5, mvo()
        ("balance", "100.00 CRAIG")
        ("num_votes", 1)
	);

    //fast forward past ballot end time
    produce_blocks(172800); //1 day in blocks

    //close ballot
    closeballot(CUSTOM_BALLOT_NAME, N(testaccount1), 2);
    produce_blocks();

    //cleanupvotes
    auto trx_pointer1 = cleanupvotes(N(testaccount1), 51, CRAIG_SYM);
    std::cout << name("testaccount1") << " charged " << trx_pointer1->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount1") << " charged " << trx_pointer1->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount1") << " charged " << trx_pointer1->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer2 = cleanupvotes(N(testaccount2), 51, CRAIG_SYM);
    std::cout << name("testaccount2") << " charged " << trx_pointer2->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount2") << " charged " << trx_pointer2->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount2") << " charged " << trx_pointer2->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer3 = cleanupvotes(N(testaccount3), 51, CRAIG_SYM);
    std::cout << name("testaccount3") << " charged " << trx_pointer3->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount3") << " charged " << trx_pointer3->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount3") << " charged " << trx_pointer3->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer4 = cleanupvotes(N(testaccount4), 51, CRAIG_SYM);
    std::cout << name("testaccount4") << " charged " << trx_pointer4->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount4") << " charged " << trx_pointer4->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount4") << " charged " << trx_pointer4->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    auto trx_pointer5 = cleanupvotes(N(testaccount5), 51, CRAIG_SYM);
    std::cout << name("testaccount5") << " charged " << trx_pointer5->elapsed.count() << " us CPU" << std::endl;
    std::cout << name("testaccount5") << " charged " << trx_pointer5->net_usage << " bytes NET" << std::endl;
    std::cout << name("testaccount5") << " charged " << trx_pointer5->action_traces[0].account_ram_deltas.begin()->delta << " bytes RAM" << std::endl << std::endl;
    produce_blocks();

    //check all vote objects have been cleaned
    cv1 = get_vote(N(testaccount1), CUSTOM_BALLOT_NAME);
    cv2 = get_vote(N(testaccount2), CUSTOM_BALLOT_NAME);
    cv3 = get_vote(N(testaccount3), CUSTOM_BALLOT_NAME);
    cv4 = get_vote(N(testaccount4), CUSTOM_BALLOT_NAME);
    cv5 = get_vote(N(testaccount5), CUSTOM_BALLOT_NAME);
    BOOST_REQUIRE_EQUAL(true, cv1.is_null());
    BOOST_REQUIRE_EQUAL(true, cv2.is_null());
    BOOST_REQUIRE_EQUAL(true, cv3.is_null());
    BOOST_REQUIRE_EQUAL(true, cv4.is_null());
    BOOST_REQUIRE_EQUAL(true, cv5.is_null());

    //test send
    auto send_trx = send(N(testaccount1), N(testaccount2), asset(5000, CRAIG_SYM), "test send");
    produce_blocks();

    //check balances after send
    ca1 = get_balance(N(testaccount1), CRAIG_SYM);
    ca2 = get_balance(N(testaccount2), CRAIG_SYM);
    REQUIRE_MATCHING_OBJECT(ca1, mvo()
        ("balance", "50.00 CRAIG")
        ("num_votes", 0)
	);
    REQUIRE_MATCHING_OBJECT(ca2, mvo()
        ("balance", "150.00 CRAIG")
        ("num_votes", 0)
	);

    //test burn
    auto burn_trx = burn(N(testaccount1), asset(2500, CRAIG_SYM));
    produce_blocks();

    //check balances after burn
    ca1 = get_balance(N(testaccount1), CRAIG_SYM);
    REQUIRE_MATCHING_OBJECT(ca1, mvo()
        ("balance", "25.00 CRAIG")
        ("num_votes", 0)
	);

    //check registry after burn


    //test seize
    auto seize_trx = seize(N(testaccount1), N(testaccount2), asset(15000, CRAIG_SYM));
    produce_blocks();

    //check balances after seize
    ca1 = get_balance(N(testaccount1), CRAIG_SYM);
    ca2 = get_balance(N(testaccount2), CRAIG_SYM);
    REQUIRE_MATCHING_OBJECT(ca1, mvo()
        ("balance", "175.00 CRAIG")
        ("num_votes", 0)
	);
    REQUIRE_MATCHING_OBJECT(ca2, mvo()
        ("balance", "0.00 CRAIG")
        ("num_votes", 0)
	);

    //test changemax
    auto changemax_trx = changemax(N(testaccount1), asset(-10000, CRAIG_SYM));
    produce_blocks();

    //check registry after changemax
    //auto cr1 = get_registry(CRAIG_SYM);

    //test unregvoter
    auto close_trx = unregvoter(N(testaccount2), CRAIG_SYM);
    produce_blocks();

    //check account doesn't exist
    ca2 = get_balance(N(testaccount2), CRAIG_SYM);
    BOOST_REQUIRE_EQUAL(true, ca2.is_null());

    std::cout << "<<<<<<<<<<<<<<<<<<<<<<< END CUSTOM_TOKEN_FLOW <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
	
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(proxy_flow, trail_tester ) try {

    std::cout << ">>>>>>>>>>>>>>>>>>>>>>> BEGIN PROXY_FLOW >>>>>>>>>>>>>>>>>>>>>>>" << std::endl;

    

    std::cout << "<<<<<<<<<<<<<<<<<<<<<<< END PROXY_FLOW <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
	
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(bad_actor_flow, trail_tester ) try {

    std::cout << ">>>>>>>>>>>>>>>>>>>>>>> BEGIN BAD_ACTOR_FLOW >>>>>>>>>>>>>>>>>>>>>>>" << std::endl;

    //ready default ballot
    readyballot(name("testballot"), name("testaccount1"), now() + 86400);
    produce_blocks();

    //check emplacement
    auto bal = get_ballot(name("testballot"));
    BOOST_REQUIRE_EQUAL(false, bal.is_null());

    //cast vote on ballot
    castvote(name("testaccount1"), name("testballot"), name("yes"));
    produce_blocks();
    
    //check vote was emplaced
    auto v = get_vote(name("testaccount1"), name("testballot"));
    BOOST_REQUIRE_EQUAL(false, v.is_null());

    //

    std::cout << "<<<<<<<<<<<<<<<<<<<<<<< END BAD_ACTOR_FLOW <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
	
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()