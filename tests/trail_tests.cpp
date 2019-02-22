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

    //make ballot, category proposal

    //setinfo

    //addoption x3 (yes, no, abstain)

    //ready ballot

    //vote/unvote/revote

    std::cout << "=======================END BALLOT_FLOW==============================" << std::endl;
	
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(vote_cleanup, trail_tester ) try {

    //make ballots x 21

    //vote on each

    //attempt to vote past max_vote_receipts

    //undelegatebw, trigger cleanup votes

    //revote
	
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()