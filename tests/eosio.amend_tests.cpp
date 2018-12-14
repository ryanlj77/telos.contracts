#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <Runtime/Runtime.h>
#include <iomanip>

#include <fc/variant_object.hpp>
#include "contracts.hpp"
#include "test_symbol.hpp"
#include "eosio.amend_tester.hpp"

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(eosio_amend_tests)

BOOST_FIXTURE_TEST_CASE( deposit_system, eosio_amend_tester) try {
	asset local_sum = get_currency_balance(N(eosio.token), symbol(4, "TLOS"), N(eosio.amend));
	for(uint16_t i = 0; i < test_voters.size(); i++) {
		auto deposit_info = get_deposit(test_voters[i].value);
		BOOST_REQUIRE_EQUAL(true, deposit_info.is_null());
		asset sum = asset::from_string("20.0000 TLOS");
		auto voter_balance = get_currency_balance(N(eosio.token), symbol(4, "TLOS"), test_voters[i].value);
		
		BOOST_REQUIRE_EQUAL(voter_balance, asset::from_string("200.0000 TLOS"));
		// std::cout << "transfer " << "1" << " account " << i << std::endl;
		transfer(test_voters[i].value, N(eosio.amend), sum, "Ratify/Amend deposit");
		produce_blocks( 2 );
		auto contract_balance = get_currency_balance(N(eosio.token), symbol(4, "TLOS"), N(eosio.amend));
		BOOST_REQUIRE_EQUAL(contract_balance, local_sum + sum);
		local_sum += sum;

		deposit_info = get_deposit(test_voters[i].value);
		BOOST_REQUIRE_EQUAL(false, deposit_info.is_null());
		REQUIRE_MATCHING_OBJECT(deposit_info, mvo()
			("owner", test_voters[i].to_string())
			("escrow", sum.to_string())
		);
		// std::cout << "transfer " << " 2 " << " account " << i << std::endl;
		transfer(test_voters[i].value, N(eosio.amend), sum, "Ratify/Amend deposit");
		produce_blocks( 2 );
		contract_balance = get_currency_balance(N(eosio.token), symbol(4, "TLOS"), N(eosio.amend));
		BOOST_REQUIRE_EQUAL(contract_balance, (local_sum + sum));
		local_sum += sum;

		deposit_info = get_deposit(test_voters[i].value);
		REQUIRE_MATCHING_OBJECT(deposit_info, mvo()
			("owner", test_voters[i].to_string())
			("escrow", (sum + sum).to_string())
		);

		asset pre_refund = get_currency_balance(N(eosio.token), symbol(4, "TLOS"), test_voters[i].value);
		asset escrow = asset::from_string(get_deposit(test_voters[i].value)["escrow"].as_string());
		getdeposit(test_voters[i].value);
            local_sum -= escrow;
		asset post_refund = get_currency_balance(N(eosio.token), symbol(4, "TLOS"), test_voters[i].value);

		BOOST_REQUIRE_EQUAL((pre_refund + escrow), post_refund);
	}
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( docs_and_clauses, eosio_amend_tester ) try {

   // insert two docs, first with 0 clauses and second with 3 clauses
   mvo doc0 = mvo()("document_id", 0)("document_title", std::string("Document 0"))("clauses", std::vector<std::string>({}))("last_amend", 0);
   mvo doc1 = mvo()("document_id", 1)("document_title", std::string("Document 1"))("clauses", std::vector<std::string>({std::string("ipfs_url_clause_1.0"), std::string("ipfs_url_clause_1.1"), std::string("ipfs_url_clause_1.2")}))("last_amend", 0);

   insertdoc(doc0["document_title"].as<std::string>(), doc0["clauses"].as<std::vector<std::string>>());
   insertdoc(doc1["document_title"].as<std::string>(), doc1["clauses"].as<std::vector<std::string>>());
   produce_blocks(1);

   // test documents integrity
   REQUIRE_MATCHING_OBJECT(get_document(0), doc0);
   REQUIRE_MATCHING_OBJECT(get_document(1), doc1);


   // register voters
   int total_voters = test_voters.size();
	symbol vote_symbol = symbol(4, "VOTE");

   register_voters(test_voters, 0, total_voters - 1, vote_symbol);
   auto registry_info = get_registry(symbol(4, "VOTE"));
   BOOST_REQUIRE_EQUAL(registry_info["total_voters"], total_voters - 1);


   name proposer = test_voters[total_voters - 1];
   transfer(N(eosio), proposer.value, asset::from_string("100.0000 TLOS"), "Blood Money");
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("300.0000"), get_balance( proposer ) ); // Fee for 3 proposals


   // set env and get thresholds from env
   set_env(uint32_t(2500000), uint64_t(1000000), uint32_t(864000000), double(5), double(66.67), double(4), double(25));
   produce_blocks(1);
   auto env = get_env();
   
   double threshold_pass_voters = env["threshold_pass_voters"].as<double>();
   unsigned int quorum_voters_size_pass = (total_voters * threshold_pass_voters) / 100;   
   auto quorum = vector<account_name>(test_voters.begin(), test_voters.begin() + quorum_voters_size_pass); 

   // submit proposals
   transfer(proposer, eosio::chain::name("eosio.amend"), core_sym::from_string("300.0000"), "Fee for Proposal 0 & 1 & 2");
   produce_blocks(1);

   makeproposal(std::string("Proposal 0"), doc0["document_id"].as<uint64_t>(), uint8_t(0), std::string("ipfs_url_clause_0.0"), proposer);
   makeproposal(std::string("Proposal 1"), doc1["document_id"].as<uint64_t>(), uint8_t(0), std::string("ipfs_url_clause_1.0.1"), proposer);
   makeproposal(std::string("Proposal 2"), doc1["document_id"].as<uint64_t>(), uint8_t(3), std::string("ipfs_url_clause_1.3.2"), proposer);
   produce_blocks(1);

   BOOST_REQUIRE_EXCEPTION( addclause(uint64_t(3), uint8_t(0), std::string("clause"), proposer), eosio_assert_message_exception, 
      eosio_assert_message_is( "proposal doesn't exist" ));

   // test that in proposal 0 can be added only one clause and modify none
   BOOST_REQUIRE_EXCEPTION( addclause(uint64_t(0), uint8_t(1), std::string("ipfs_url_clause_0.1"), proposer), eosio_assert_message_exception, 
      eosio_assert_message_is( "new clause num is not valid" ));

   BOOST_REQUIRE_EXCEPTION( addclause(uint64_t(0), uint8_t(0), std::string("ipfs_url_clause_0.0"), proposer), eosio_assert_message_exception, 
      eosio_assert_message_is( "clause number to add already exists in proposal" ));

   mvo expected_doc0 = mvo()
         ("document_id", doc0["document_id"])
         ("document_title", doc0["document_title"])
         ("clauses", std::vector<std::string>({std::string("ipfs_url_clause_0.0")}))
         ("last_amend", 0);

   // test proposal 1: change all clauses + add one more
   addclause(uint64_t(1), uint8_t(1), std::string("ipfs_url_clause_1.1.1"), proposer);
   addclause(uint64_t(1), uint8_t(2), std::string("ipfs_url_clause_1.2.1"), proposer);
   addclause(uint64_t(1), uint8_t(3), std::string("ipfs_url_clause_1.3.1"), proposer);
   
   mvo expected_doc1_p1 = mvo()
         ("document_id", doc1["document_id"])
         ("document_title", doc1["document_title"])
         ("clauses", std::vector<std::string>({std::string("ipfs_url_clause_1.0.1"), std::string("ipfs_url_clause_1.1.1"), std::string("ipfs_url_clause_1.2.1"), std::string("ipfs_url_clause_1.3.1")}))
         ("last_amend", 0);

   // test proposal 2: add one more clause + modify clause #0
   addclause(uint64_t(2), uint8_t(0), std::string("ipfs_url_clause_1.0.2"), proposer);
   
   mvo expected_doc1_p2 = mvo()
         ("document_id", doc1["document_id"])
         ("document_title", doc1["document_title"])
         ("clauses", std::vector<std::string>({std::string("ipfs_url_clause_1.0.2"), std::string("ipfs_url_clause_1.1.1"), std::string("ipfs_url_clause_1.2.1"), std::string("ipfs_url_clause_1.3.2")}))
         ("last_amend", 0);


   // open voting and test that no clause can be added anymore
   openvoting(0, proposer);
   openvoting(1, proposer);
   openvoting(2, proposer);
   produce_blocks(1);

   BOOST_REQUIRE_EXCEPTION( addclause(uint64_t(0), uint8_t(0), std::string("ipfs_url_clause_0.0"), proposer), eosio_assert_message_exception, 
      eosio_assert_message_is( "proposal is no longer in building stage" ));

   // vote proposal 0 to fail and 1, 2 to pass and skip to the end of the cycle
   for(int i = 0; i < quorum_voters_size_pass; i++) {
      mirrorcast(quorum[i].value, symbol(4, "TLOS"));

      castvote(quorum[i].value, 0, 0);
      castvote(quorum[i].value, 1, 1);
      castvote(quorum[i].value, 2, 1);
      produce_blocks(1);
   }

   produce_block(fc::seconds(2500000));

   // close proposals and test that docs were updated as expected
   closeprop(0, proposer);
   produce_blocks(1);
   REQUIRE_MATCHING_OBJECT(get_document(0), doc0);

   closeprop(1, proposer);
   produce_blocks(1);
   REQUIRE_MATCHING_OBJECT(get_document(1), expected_doc1_p1);

   closeprop(2, proposer);
   produce_blocks(1);
   REQUIRE_MATCHING_OBJECT(get_document(1), expected_doc1_p2);
   
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( amend_complete_flow, eosio_amend_tester ) try {

   insert_default_docs();

   // register voters
   int total_voters = test_voters.size();
	symbol vote_symbol = symbol(4, "VOTE");

   register_voters(test_voters, 0, total_voters - 1, vote_symbol);
   auto registry_info = get_registry(symbol(4, "VOTE"));
   BOOST_REQUIRE_EQUAL(registry_info["total_voters"], total_voters - 1);


   name proposer = test_voters[total_voters - 1];
   transfer(N(eosio), proposer.value, asset::from_string("101.0000 TLOS"), "Blood Money");
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("301.0000"), get_balance( proposer ) ); // Fee for 3 proposals

   // set env and get thresholds from env
   set_env(uint32_t(2500000), uint64_t(1000000), uint32_t(864000000), double(5), double(66.67), double(4), double(25));
   produce_blocks(1);
   auto env = get_env();
   
   double threshold_pass_voters = env["threshold_pass_voters"].as<double>();
   double threshold_fee_voters = env["threshold_fee_voters"].as<double>();
   unsigned int quorum_voters_size_pass = (total_voters * threshold_pass_voters) / 100;
   unsigned int quorum_voters_size_fail = quorum_voters_size_pass - 1;
   unsigned int fee_voters = (total_voters * threshold_fee_voters) / 100;
   
   double threshold_pass_votes = env["threshold_pass_votes"].as<double>();
   double threshold_fee_votes = env["threshold_fee_votes"].as<double>();

   
   // submit 3 proposals
   transfer(proposer, eosio::chain::name("eosio.amend"), core_sym::from_string("100.0000"), "Proposal 0 fee");
   makeproposal(std::string("Proposal 0"), documents[0]["document_id"].as<uint64_t>(), uint8_t(0), std::string("ipfs_url_clause_0"), proposer);
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("201.0000"), get_balance( proposer ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("100.0000"), get_balance( eosio::chain::name("eosio.amend") ) );

   transfer(proposer, eosio::chain::name("eosio.amend"), core_sym::from_string("200.0000"), "Proposal 1&2 fees");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1.0000"), get_balance( proposer ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("300.0000"), get_balance( eosio::chain::name("eosio.amend") ) );
   produce_blocks(1);
   makeproposal(std::string("Proposal 1"), documents[1]["document_id"].as<uint64_t>(), uint8_t(1), std::string("ipfs_url_clause_1"), proposer);
   makeproposal(std::string("Proposal 2"), documents[2]["document_id"].as<uint64_t>(), uint8_t(3), std::string("ipfs_url_clause_3"), proposer);
   produce_blocks(1);


   // test voting asserts
	BOOST_REQUIRE_EXCEPTION( castvote(test_voters[0].value, 0, 1), eosio_assert_message_exception, 
      eosio_assert_message_is( "ballot voting window not open" ));


	openvoting(0, proposer);
	openvoting(1, proposer);
   openvoting(2, proposer);
   produce_blocks(1);
   
   BOOST_REQUIRE_EXCEPTION( openvoting(0, proposer), eosio_assert_message_exception, 
      eosio_assert_message_is( "proposal is no longer in building stage" ));

   // validate vote integrity
	BOOST_REQUIRE_EXCEPTION( castvote(test_voters[0].value, 0, 3), eosio_assert_message_exception, 
      eosio_assert_message_is( "Invalid Vote. [0 = NO, 1 = YES, 2 = ABSTAIN]" ));

	BOOST_REQUIRE_EXCEPTION( castvote(test_voters[0].value, 3, 1), eosio_assert_message_exception, 
      eosio_assert_message_is( "ballot with given ballot_id doesn't exist" ));

   // mirrorcast on the other hand will throw an exception 
	BOOST_REQUIRE_EXCEPTION( mirrorcast(proposer.value, symbol(4, "TLOS")), eosio_assert_message_exception, 
      eosio_assert_message_is( "voter is not registered" ));

   auto quorum = vector<account_name>(test_voters.begin(), test_voters.begin() + quorum_voters_size_pass); 

   
   auto calculateTippingPoint = [&](int quorum_size, int threshold, bool fee = false) {
      if( fee )
         return std::ceil( double(quorum_size * threshold) / 100);

      return std::floor( double(quorum_size * threshold) / 100) + 1;
   };
 
   int vote_tipping_point = calculateTippingPoint(quorum_voters_size_fail, threshold_pass_votes);
   int fee_tipping_point = calculateTippingPoint(quorum_voters_size_fail, threshold_fee_votes, true);

   // std::cout<<"conditions: [ "<<vote_tipping_point<<" ] / [ i < "<<fee_tipping_point<<" - 1 ] "<<std::endl;

   // vote proposal 0 to fail, proposal 1 to fail but get fee back and proposal 2 to pass and get fee back
   for(int i = 0; i < quorum_voters_size_fail; i++) {
      mirrorcast(quorum[i].value, symbol(4, "TLOS"));

      // fail vote from lack of voters and the fee from no votes
      uint16_t vote_direction_0 = ( i < fee_tipping_point - 1 ) ? uint16_t(1) : uint16_t(0);
      // fail vote from lack of voters, but get fee
      uint16_t vote_direction_1 = 1;

      castvote(quorum[i].value, 0, vote_direction_0);
      castvote(quorum[i].value, 1, vote_direction_1);
      produce_blocks(1);
   }
   for(int i = 0; i < quorum_voters_size_pass; i++) {
      if(i >= quorum_voters_size_fail) { // voters which didn't staked for VOTE before
         mirrorcast(quorum[i].value, symbol(4, "TLOS"));         
      }

      uint16_t vote_direction_2 = ( i < vote_tipping_point ) ? uint16_t(1) : uint16_t(0);
      castvote(quorum[i].value, 2, vote_direction_2);
      produce_blocks(1);
   }

   BOOST_REQUIRE_EXCEPTION( closeprop(0, proposer), eosio_assert_message_exception, 
      eosio_assert_message_is( "Proposal is still open" ));
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1.0000"), get_balance( proposer ) );

   // end voting period
   produce_block(fc::seconds(2500000));
   produce_blocks(1);

   // close proposal 0: fail & nothing to refund
   closeprop(0, proposer);
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1.0000"), get_balance( proposer ));
   BOOST_REQUIRE_EQUAL(get_proposal(0)["status"].as<uint8_t>(), uint8_t(2));

   // close proposal 1: fail & refund fee
   closeprop(1, proposer);
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("101.0000"), get_balance( proposer ));
   BOOST_REQUIRE_EQUAL(get_proposal(1)["status"].as<uint8_t>(), uint8_t(2));

   // close proposal 1: fail & refund fee
   closeprop(2, proposer);
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("201.0000"), get_balance( proposer ));
   BOOST_REQUIRE_EQUAL(get_proposal(2)["status"].as<uint8_t>(), uint8_t(1));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( ballot_id_and_fee, eosio_amend_tester ) try {
   register_voters(test_voters, 0, 1, symbol(4, "VOTE"));
   
   insert_default_docs();

   auto proposer = test_voters[0];
   transfer(N(eosio), proposer.value, asset::from_string("2000.0000 TLOS"), "Blood Money");
   transfer(N(eosio), eosio::chain::name("eosio.amend"), asset::from_string("0.0001 TLOS"), "Init Amend env");

   auto title = std::string("my ratify test title");
   auto cycles = 1;
   auto ipfs_location = std::string("32662273CFF99078EC3BFA5E7BBB1C369B1D3884DEDF2AF7D8748DEE080E4B9");

   int num_proposals = 10;
   auto env = get_env();
   uint64_t fee = env["fee"].as<uint64_t>();
   
   asset expected_total = core_sym::from_string("0.0001");
   signed_transaction trx;
   for( int i = 0; i < num_proposals; i++){
      std::stringstream ssf;
      ssf << std::fixed << std::setprecision(4) << (double(fee)/10000);
      asset _fee = core_sym::from_string(ssf.str());
      expected_total += _fee;

      transfer(proposer, eosio::chain::name("eosio.amend"), _fee, std::string("fee for ratify "+std::to_string(i)));

      uint64_t doc_num = i % documents.size();
      uint8_t clause_num = i % documents[doc_num]["clauses"].size(); 
	   makeproposal(std::string(title+std::to_string(i)), uint64_t(doc_num), uint8_t(clause_num), ipfs_location, proposer);
   }
   produce_blocks(1);
	
	BOOST_REQUIRE_EXCEPTION( 
      makeproposal(std::string(title+std::to_string(num_proposals)), uint64_t(documents.size()), uint8_t(0), ipfs_location, proposer), 
      eosio_assert_message_exception, 
      eosio_assert_message_is( "Document Not Found" )
   );

	BOOST_REQUIRE_EXCEPTION( 
      makeproposal(std::string(title+std::to_string(num_proposals)), uint64_t(0), uint8_t(documents[0]["clauses"].size()), ipfs_location, proposer),
      eosio_assert_message_exception, 
      eosio_assert_message_is( "Deposit not found, please transfer your TLOS fee" )
   );
   
   asset saving_balance = get_balance(N(eosio.amend));

   BOOST_REQUIRE_EQUAL(saving_balance, expected_total);

   for( int i = 0; i < num_proposals; i++){
      auto proposal = get_proposal(i);
      auto submission = get_submission(i);
      auto ballot = get_ballot(i);

      // since only wps here, there should be same ids for props and subs
      BOOST_REQUIRE_EQUAL(proposal["prop_id"], submission["proposal_id"]);
      BOOST_REQUIRE_EQUAL(proposal["info_url"], submission["new_ipfs_urls"].as<vector<std::string>>()[0]);
      
      // prop id and ballot ref id should be same
      BOOST_REQUIRE_EQUAL(proposal["prop_id"], ballot["reference_id"]);

      // submission should have correct ballot_id
      BOOST_REQUIRE_EQUAL(submission["ballot_id"], ballot["ballot_id"]);
   }
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( set_environment, eosio_amend_tester ) try {
   set_env(111111, 2222, 33333, 1.1, 11.1, 1.2, 11.2);
   produce_blocks(1);

   // transfer(N(eosio), N(eosio.saving), core_sym::from_string("1.0000"), "memo");
   // getdeposit(N(eosio));

   auto env = get_env();
   REQUIRE_MATCHING_OBJECT(
      env, 
      mvo()
         ("publisher", eosio::chain::name("eosio.amend"))
         ("expiration_length", 111111)
         ("fee", 2222)
         ("start_delay", 33333)
         ("threshold_pass_voters", 1.1)
         ("threshold_pass_votes", 11.1)
         ("threshold_fee_voters", 1.2)
         ("threshold_fee_votes", 11.2)
   );

   insert_default_docs();
   name proposer = test_voters[0];
   transfer(proposer, eosio::chain::name("eosio.amend"), core_sym::from_string("0.2221"), "ratify 1 fee");
   produce_blocks();

	BOOST_REQUIRE_EXCEPTION( 
      makeproposal(
         std::string("test ratify 1"), 
         uint64_t(0), 
         uint8_t(0), 
         std::string("32662273CFF99078EC3BFA5E7BBB1C369B1D3884DEDF2AF7D8748DEE080E4B99"), 
         proposer
      ), 
      eosio_assert_message_exception, 
      eosio_assert_message_is( "Deposit amount is less than fee, please transfer more TLOS" )
   );

   transfer(proposer, eosio::chain::name("eosio.amend"), core_sym::from_string("0.0001"), "ratify 1 fee diff"); 
   makeproposal(
      std::string("test ratify 1"), 
      uint64_t(0), 
      uint8_t(0), 
      std::string("32662273CFF99078EC3BFA5E7BBB1C369B1D3884DEDF2AF7D8748DEE080E4B99"), 
      proposer
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( create_proposal_and_cancel, eosio_amend_tester ) try {
   int total_voters = test_voters.size();
   name proposer = test_voters[total_voters - 1];
   transfer(N(eosio), proposer.value, core_sym::from_string("300.0000"), "Blood Money");
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("500.0000"), get_balance( proposer ) );

   set_env(uint32_t(2500000), uint64_t(1000000), uint32_t(864000000), double(5), double(66.67), double(4), double(25));
   produce_blocks(1);

   insert_default_docs();

	BOOST_REQUIRE_EXCEPTION( 
      makeproposal(
         std::string("test ratify 1"), 
         uint64_t(0), 
         uint8_t(0), 
         std::string("32662273CFF99078EC3BFA5E7BBB1C369B1D3884DEDF2AF7D8748DEE080E4B99"), 
         proposer
      ), 
      eosio_assert_message_exception, eosio_assert_message_is( "Deposit not found, please transfer your TLOS fee" )
   );

   transfer(proposer, eosio::chain::name("eosio.amend"), core_sym::from_string("50.0000"), "ratify 1 fee");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("450.0000"), get_balance( proposer ) );
   produce_blocks(1);

	BOOST_REQUIRE_EXCEPTION( 
      makeproposal(
         std::string("test ratify 1"), 
         uint64_t(0), 
         uint8_t(0), 
         std::string("32662273CFF99078EC3BFA5E7BBB1C369B1D3884DEDF2AF7D8748DEE080E4B99"), 
         proposer
      ), 
      eosio_assert_message_exception, 
      eosio_assert_message_is( "Deposit amount is less than fee, please transfer more TLOS" )
   );

   transfer(proposer, eosio::chain::name("eosio.amend"), core_sym::from_string("50.0000"), "ratify 1 fee");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("400.0000"), get_balance( proposer ) );
   produce_blocks(1);

   makeproposal(
      std::string("test ratify 1"), 
      uint64_t(0), 
      uint8_t(0), 
      std::string("32662273CFF99078EC3BFA5E7BBB1C369B1D3884DEDF2AF7D8748DEE080E4B99"), 
      proposer
   );
   produce_blocks(1);

   transfer(proposer, eosio::chain::name("eosio.amend"), core_sym::from_string("100.0000"), "amend 2 fee");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("300.0000"), get_balance( proposer ) );

   makeproposal(
      std::string("test amend 2"), 
      uint64_t(0), 
      uint8_t(0), 
      std::string("32662273CFF99078EC3BFA5E7BBB1C369B1D3884DEDF2AF7D8748DEE080E4B99"), 
      proposer
   );
   produce_blocks(1);

   auto deposit_info = get_deposit(proposer.value);
   BOOST_REQUIRE(deposit_info.is_null());

   cancelsub(0, proposer);
   cancelsub(1, proposer);

   BOOST_REQUIRE_EQUAL( core_sym::from_string("300.0000"), get_balance( proposer ) );
   
   BOOST_REQUIRE(get_proposal(0).is_null());
   BOOST_REQUIRE(get_proposal(1).is_null());
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()