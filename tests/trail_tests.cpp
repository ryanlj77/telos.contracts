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

BOOST_FIXTURE_TEST_CASE(ballot_flow, trail_tester ) try {

    std::cout << "=======================BEGIN BALLOT_FLOW==============================" << std::endl;

    const name BALLOT_NAME = name("testballot");
    const name CATEGORY = name("poll");
    const string NEW_TITLE = "# New Ballot Title";
    const string NEW_DESC = "## New Description";
    const string NEW_URL = "/ipfs/newipfslink";
    const uint8_t MAX_VOTABLE_OPTIONS = 3;
    const uint32_t BALLOT_LENGTH = 86400; //1 day in seconds

    const name YES_OPTION = name("yes");
    const name NO_OPTION = name("no");
    const name ABSTAIN_OPTION = name("abstain");

    //make ballot poll
    newballot(BALLOT_NAME, CATEGORY, N(testaccount1), "# Ballot", "## Description", "/ipfs/someipfslink", MAX_VOTABLE_OPTIONS, VOTE_SYM);
    //check ballot is emplaced

    //setinfo
    setinfo(BALLOT_NAME, N(testaccount1), NEW_TITLE, NEW_DESC, NEW_URL);
    //check info is changed

    //addoption x3 (yes, no, abstain)
    addoption(BALLOT_NAME, N(testaccount1), YES_OPTION, "Yes");
    addoption(BALLOT_NAME, N(testaccount1), NO_OPTION, "No");
    addoption(BALLOT_NAME, N(testaccount1), ABSTAIN_OPTION, "Abstain");
    //check options are emplaced

    //ready ballot
    readyballot(BALLOT_NAME, N(testaccount1), now() + BALLOT_LENGTH);
    //check ballot status and times

    //vote
    castvote(N(testaccount1), BALLOT_NAME, YES_OPTION);
    castvote(N(testaccount2), BALLOT_NAME, NO_OPTION);
    castvote(N(testaccount3), BALLOT_NAME, YES_OPTION);
    castvote(N(testaccount4), BALLOT_NAME, ABSTAIN_OPTION);
    castvote(N(testaccount5), BALLOT_NAME, YES_OPTION);
    //check votes exist, num_votes ware incremented, ballot updated with casted votes

    //unvote

    //revote

    //close ballot

    //cleanupvotes

    std::cout << "=======================END BALLOT_FLOW==============================" << std::endl;
	
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(scaled_ballot_flow, trail_tester ) try {

    //make ballots x 21

    //vote on each

    //attempt to vote past max_vote_receipts

    //undelegatebw, trigger cleanup votes

    //revote
	
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()