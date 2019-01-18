/**
 * Arbitration Contract Implementation. See function bodies for further notes.
 * 
 * @author Craig Branscom, Peter Bue, Ed Silva, Douglas Horn
 * @copyright defined in telos/LICENSE.txt
 */

#include <eosio.arbitration/eosio.arbitration.hpp>


arbitration::arbitration(name s, name code, datastream<const char *> ds) : eosio::contract(s, code, ds), configs(_self, _self.value) {
  _config = configs.exists() ? configs.get() : get_default_config();
}

arbitration::~arbitration() {
  if (configs.exists()) configs.set(_config, get_self());
}

void arbitration::setconfig(uint16_t max_elected_arbs, uint32_t election_duration, uint32_t election_start, uint32_t arbitrator_term_length, vector<int64_t> fees) {
  require_auth(name("eosio"));
  
  eosio_assert(max_elected_arbs > uint16_t(0), "Arbitrators must be greater than 0");
  _config = config{
                    get_self(), //publisher
                    fees, //fee_structure
                    max_elected_arbs, //max_elected_arbs
                    election_duration, //election_duration
                    election_start, //election_start
                    _config.auto_start_election,
                    _config.current_ballot_id,
                    arbitrator_term_length
                  };

  //print("\nSettings Configured: SUCCESS");
}


#pragma region Arb_Elections

void arbitration::initelection() {
  require_auth(name("eosio"));
  eosio_assert(!_config.auto_start_election, "Election is on auto start mode.");

  ballots_table ballots(name("eosio.trail"), name("eosio.trail").value);
  _config.current_ballot_id = ballots.available_primary_key();
  _config.auto_start_election = true;

  arbitrators_table arbitrators(_self, _self.value);

  uint8_t available_seats = 0;
  if (has_available_seats(arbitrators, available_seats)) {
    start_new_election(available_seats);
  }

  // print("\nElection started: SUCCESS");
}

void arbitration::regcand(name candidate, string credentials_link) {
  require_auth(candidate);
  validate_ipfs_url(credentials_link);

  candidates_table candidates(get_self(), get_self().value);
  auto cand_itr = candidates.find(candidate.value);
  eosio_assert(cand_itr == candidates.end(), "Candidate is already an applicant");
  
  arbitrators_table arbitrators(get_self(), get_self().value);
  auto arb_itr = arbitrators.find(candidate.value);

  if (arb_itr != arbitrators.end()) {
    eosio_assert(now() > arb_itr->term_expiration, "Candidate is already an Arbitrator and the seat isn't expired");

    //NOTE: set arb_status to SEAT_EXPIRED until re-election
    arbitrators.modify(arb_itr, same_payer, [&](auto &row) {
      row.arb_status = SEAT_EXPIRED;
    });
  }

  candidates.emplace(get_self(), [&](auto &row) {
    row.candidate_name = candidate;
    row.credentials_link = credentials_link;
    row.application_time = now();
  });
}

void arbitration::unregcand(name candidate) {
  require_auth(candidate);

  candidates_table candidates(get_self(), get_self().value);
  auto cand_itr = candidates.find(candidate.value);
  eosio_assert(cand_itr != candidates.end(), "Candidate isn't an applicant");

  ballots_table ballots(name("eosio.trail"), name("eosio.trail").value);
  auto bal = ballots.get(_config.current_ballot_id, "Ballot doesn't exist");

  leaderboards_table leaderboards(name("eosio.trail"), name("eosio.trail").value);
  auto board = leaderboards.get(bal.reference_id, "Leaderboard doesn't exist");

  //TODO: assert candidate is on leaderboard?
  eosio_assert(now() < board.begin_time, "Cannot unregister while election is in progress");

  candidates.erase(cand_itr);
}

void arbitration::candaddlead(name candidate, string credentials_link) {
  require_auth(candidate);
  validate_ipfs_url(credentials_link);
  eosio_assert(_config.auto_start_election, "there is no active election");

  candidates_table candidates(_self, _self.value);
  auto cand_itr = candidates.find(candidate.value);
  eosio_assert(cand_itr != candidates.end(), "Candidate isn't an applicant. Use regcand action to register as a candidate");

  ballots_table ballots(name("eosio.trail"), name("eosio.trail").value);
  auto bal = ballots.get(_config.current_ballot_id, "Ballot doesn't exist");

  leaderboards_table leaderboards(name("eosio.trail"), name("eosio.trail").value);
  auto board = leaderboards.get(bal.reference_id, "Leaderboard doesn't exist");
  eosio_assert(board.status != CLOSED, "A new election hasn't started. Use initelection action to start a new election.");
  
  action(permission_level{get_self(), name("active")}, name("eosio.trail"), name("addcandidate"),
    make_tuple(get_self(), 
      _config.current_ballot_id, 
      candidate, 
      credentials_link
    )
  ).send();
  
  //print("\nArb Application: SUCCESS");
}

void arbitration::candrmvlead(name candidate) {
  require_auth(candidate);
  
  candidates_table candidates(get_self(), get_self().value);
  auto cand_itr = candidates.find(candidate.value);
  eosio_assert(cand_itr != candidates.end(), "Candidate isn't an applicant.");

  action(permission_level{get_self(), name("active")}, name("eosio.trail"), name("rmvcandidate"),
    make_tuple(get_self(), 
      _config.current_ballot_id, 
      candidate
    )
  ).send();
  
  //print("\nCancel Application: SUCCESS");
}

void arbitration::endelection(name candidate) {
  require_auth(candidate);
   
  ballots_table ballots(name("eosio.trail"), name("eosio.trail").value);
  auto bal = ballots.get(_config.current_ballot_id, "Ballot doesn't exist");

  leaderboards_table leaderboards(name("eosio.trail"), name("eosio.trail").value);
  auto board = leaderboards.get(bal.reference_id, "Leaderboard doesn't exist");
  
  eosio_assert(now() > board.end_time, 
    std::string("Election hasn't ended. Please check again after the election is over in"
    + std::to_string(uint32_t(board.end_time - now())) //TODO: convert to minutes for error message?
    + " seconds").c_str()
  );
  
  //sort board candidates by votes
  auto board_candidates = board.candidates;
  sort(board_candidates.begin(), board_candidates.end(), [](const auto &c1, const auto &c2) { return c1.votes > c2.votes; });
  
  //resolve tie clonficts
  if(board_candidates.size() > board.available_seats) {      
    auto first_cand_out = board_candidates[board.available_seats];
    board_candidates.resize(board.available_seats);
    
    //count candidates that are tied with first_cand_out
    uint8_t tied_cands = 0;
    for(int i = board_candidates.size() - 1; i >= 0; i--) {
      if(board_candidates[i].votes == first_cand_out.votes) 
        tied_cands++;
    }

    //remove all tied candidates
    if(tied_cands > 0) board_candidates.resize(board_candidates.size() - tied_cands);
  }

  candidates_table candidates(get_self(), get_self().value);
  auto cand_itr = candidates.find(candidate.value);
  eosio_assert(cand_itr != candidates.end(), "Candidate isn't an applicant.");
  
  arbitrators_table arbitrators(get_self(), get_self().value);
  std::vector<permission_level_weight> arbs_perms;  

  //in case there are still candidates (not all tied)
  if(board_candidates.size() > 0) {
    for (int i = 0; i < board_candidates.size(); i++) {
        name cand_name = board_candidates[i].member;  
        auto c = candidates.find(cand_name.value);
        
        if (c != candidates.end()) {
          auto cand_credential = board_candidates[i].info_link;
          auto cand_votes = board_candidates[i].votes;
        
          if(cand_votes == asset(0, cand_votes.symbol)) continue;
          
          //remove candidates from candidates table / arbitration contract
          candidates.erase(c);
          
          //add candidates to arbitration table / arbitration contract
          add_arbitrator(arbitrators, cand_name, cand_credential);
        } else {
          print("\ncandidate: ", name{cand_name}, " was not found."); //TODO: remove?
        }
    }

    //add current arbitrators to permission list
    for(const auto &a : arbitrators) {
      if(a.arb_status != SEAT_EXPIRED) {
        arbs_perms.emplace_back(permission_level_weight{ permission_level{a.arb, name("active") }, 1 });
      }
    }

    //permissions need to be sorted 
    sort(arbs_perms.begin(), arbs_perms.end(), [](const auto &first, const auto &second) { return first.permission.actor.value < second.permission.actor.value; });

    //review update auth permissions and weights.
    if(arbs_perms.size() > 0) {
      uint32_t weight = arbs_perms.size() > 3 ? ((( 2 * arbs_perms.size() ) / uint32_t(3)) + 1) : 1;
  
      action(permission_level{get_self(), name("owner")}, name("eosio"), name("updateauth"),
        std::make_tuple(
          get_self(), 
          name("major"), 
          name("owner"),
          authority {
            weight, 
            std::vector<key_weight>{},
            arbs_perms,
            std::vector<wait_weight>{}
          }
        )
      ).send();
    }
  }

  //close ballot action.
  action(permission_level{get_self(), name("active")}, name("eosio.trail"), name("closeballot"), 
    make_tuple(
      get_self(), 
      _config.current_ballot_id, 
      CLOSED
    )
  ).send(); 

  //start new election with remaining candidates 
  //and new candidates that registered after past election had started.
  uint8_t available_seats = 0;
  auto remaining_candidates = distance(candidates.begin(), candidates.end());

  if (remaining_candidates > 0 && has_available_seats(arbitrators, available_seats)) {
    _config.current_ballot_id = ballots.available_primary_key();
    
    start_new_election(available_seats);

    for (const auto &c : candidates) {
      action(permission_level{get_self(), name("active")}, name("eosio.trail"), name("addcandidate"),
        make_tuple(
          get_self(), 
          _config.current_ballot_id, 
          c, 
          c.credentials_link
        )
      ).send(); 
    }
    
    //print("\nA new election has started.");
  } else {
    for(auto i = candidates.begin(); i != candidates.end(); i = candidates.erase(i));
    _config.auto_start_election = false;
    //print("\nThere aren't enough seats available or candidates to start a new election.\nUse init action to start a new election.");
  }
}

#pragma endregion Arb_Elections


#pragma region Case_Setup

void arbitration::filecase(name claimant, string claim_link) {
  require_auth(claimant);
	//eosio_assert(class_suggestion >= UNDECIDED && class_suggestion <= MISC, "class suggestion must be between 0 and 14");
	validate_ipfs_url(claim_link);

	action(permission_level{ claimant, name("active")}, name("eosio.token"), name("transfer"), make_tuple(
		claimant,
		get_self(),
		asset(int64_t(1000000), symbol("TLOS", 4)),
		std::string("Arbitration Case Filing Fee")
	)).send();

  claims_table claims(get_self(), get_self().value);
  auto claim_id = claims.available_primary_key();

  claims.emplace(claimant, [&](auto& row){
    row.claim_id = claim_id;
    row.claim_summary = claim_link;
    row.decision_link = "";
  });

	casefiles_table casefiles(get_self(), get_self().value);
  auto case_id = casefiles.available_primary_key();
  vector<name> arbs;
  vector<name> respondants;
  vector<uint8_t> lang_codes;
	vector<uint64_t> unr_claims = {claim_id};
  vector<uint64_t> acc_claims;

  casefiles.emplace(claimant, [&](auto& row){
    row.case_id = case_id;
    row.case_status = CASE_SETUP;
    row.claimants = claimants;
    row.respondants = respondants;
    row.arbitrators = arbs;
    row.required_langs = lang_codes;
    row.unread_claims = unr_claims;
    row.accepted_claims = acc_claims;
    row.arb_comment = "";
    row.last_edit = now();
  });

  //print("\nCased Filed");
} 

void arbitration::addclaim(uint64_t case_id, string claim_link, name claimant) { 
  require_auth(claimant);
	//eosio_assert(class_suggestion >= UNDECIDED && class_suggestion <= MISC, "class suggestion must be between 0 and 14");
	validate_ipfs_url(claim_link);

  claims_table claims(get_self(), get_self().value);
  auto claim_id = claims.available_primary_key();

  claims.emplace(claimant, [&](auto& row){
    row.claim_id = claim_id;
    row.claim_summary = claim_link;
    row.decision_link = "";
  });

	casefiles_table casefiles(get_self(), get_self().value);
	auto cf = casefiles.get(case_id, "Case Not Found");
	eosio_assert(cf.case_status == CASE_SETUP, "claims cannot be added after CASE_SETUP is complete.");
  eosio_assert(is_claimant(claimant, cf.claimants), "not in list of claimants for casefile");

	auto new_claims = cf.unread_claims;
	new_claims.emplace_back(claim_id);
  casefiles.modify(cf, same_payer, [&](auto& row) { 
		row.claims = new_claims;
	});
}

void arbitration::removeclaim(uint64_t case_id, uint64_t claim_id, name claimant) {
  require_auth(claimant);

	casefiles_table casefiles(get_self(), get_self().value);
	auto cf = casefiles.get(case_id, "Case Not Found");
	eosio_assert(cf.case_status == CASE_SETUP, "Claims cannot be removed after CASE_SETUP is complete");
  eosio_assert(cf.unread_claims.size() > 0, "No claims to remove");
  eosio_assert(is_claimant(claimant, cf.claimants), "Not in list of claimants for casefile");
  
  vector<uint64_t> new_claims = cf.unread_claims;

  bool found = false;
  auto itr = new_claims.begin();
  while (itr != new_claims.end()) {
    if (*itr == claim_id) {
      new_claims.erase(itr);
      found = true;
      break;
    }
    itr++;
  }
  eosio_assert(found, "Claim ID not found in casefile");

	casefiles.modify(cf, same_payer, [&](auto& row) {
		row.claims = new_claims;
	});

  claims_table claims(get_self(), get_self().value);
  auto cl = claims.get(claim_id, "Claim not found in table");
  claims.erase(cl);

	//print("\nClaim Removed");
}

void arbitration::shredcase(uint64_t case_id, name claimant) {
  require_auth(claimant);

  casefiles_table casefiles(get_self(), get_self().value);
  auto c_itr = casefiles.find(case_id);
  eosio_assert(c_itr != casefiles.end(), "Case Not Found");
  eosio_assert(is_claimant(claimant, c_itr->claimants), "not in list of claimants for casefile");

  //TODO: erase all claims for case as well

  eosio_assert(c_itr->case_status == CASE_SETUP, "cases can only be shredded during CASE_SETUP");

  casefiles.erase(c_itr);
}

void arbitration::readycase(uint64_t case_id, name claimant) {
  casefiles_table casefiles(get_self(), get_self().value);
  auto cf = casefiles.get(case_id, "Case Not Found");
  eosio_assert(cf.case_status == CASE_SETUP, "Cases can only be readied during CASE_SETUP");
  eosio_assert(cf.unread_claims.size() >= 1, "Cases must have atleast one claim");
  eosio_assert(is_claimant(claimant, cf.claimants), "not in list of claimants for casefile");

  casefiles.modify(cf, same_payer, [&](auto &row) { 
    row.case_status = AWAITING_ARBS;
    row.last_edit = now();
  });
}

#pragma endregion Case_Setup


#pragma region Case_Actions

void arbitration::assigntocase(uint64_t case_id, name arb) {
  require_auth(arb);

  //TODO: add arb to list of arbs in casefile
}

void arbitration::advancecase(uint64_t case_id, name arb) {
  require_auth(arb);
}

void arbitration::dismisscase(uint64_t case_id, name arb, string ipfs_link, string comment) {
  require_auth(arb);
  validate_ipfs_url(ipfs_link);

  casefiles_table casefiles(get_self(), get_self().value);
  auto cf = casefiles.get(case_id, "No case found with given case_id");

  auto arb_case = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), arb);
  eosio_assert(arb_case != cf.arbitrators.end(), "Arbitrator isn't selected for this case");
  eosio_assert(cf.case_status == CASE_INVESTIGATION, "Case is already dismissed or complete");

  casefiles.modify(cf, same_payer, [&](auto &row) {
    row.case_status = DISMISSED;
    row.arb_comment = comment;
    row.last_edit = now();
  });

  //print("\nCase Dismissed: SUCCESS");
}

void arbitration::dismissclaim(uint64_t case_id, name arb, string claim_hash) {
  require_auth(arb);
  validate_ipfs_url(claim_hash);

  casefiles_table casefiles(get_self(), get_self().value);
  auto cf = casefiles.get(case_id, "Case not found");
  auto arb_case = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), arb);
  eosio_assert(arb_case != cf.arbitrators.end(), "Only an assigned arbitrator can dismiss a claim");

  //vector<claim> new_claims = cf.unread_claims;
  // vector<uint64_t> new_accepted_ev_ids = clm.accepted_ev_ids;
  // new_accepted_ev_ids.erase(clm.accepted_ev_ids.begin() + ev_index);
  // clm.accepted_ev_ids = new_accepted_ev_ids;
  // new_claims[claim_index] = clm;

  casefiles.modify(cf, same_payer, [&](auto &cf) {
    cf.claims = new_claims;
    cf.last_edit = now();
  });

  // dismissed_evidence_table d_evidences(_self, _self.value);
  // auto dev_id = d_evidences.available_primary_key();
  // d_evidences.emplace(_self, [&](auto &dev) {
  //   dev.ev_id = dev_id;
  //   dev.ipfs_url = ipfs_url;
  // });

  //print("\nClaim dismissed");
}

void arbitration::acceptclaim(uint64_t case_id, uint16_t claim_index, uint16_t ev_index, name arb, string ipfs_url) {
  require_auth(arb);
  validate_ipfs_url(ipfs_url);

  casefiles_table casefiles(_self, _self.value);
  auto c = casefiles.get(case_id, "Case not found");
  auto arb_case = std::find(c.arbitrators.begin(), c.arbitrators.end(), arb);

  eosio_assert(arb_case != c.arbitrators.end(), "only arbitrator can accept");
  eosio_assert(claim_index < 0 || claim_index >= c.claims.size(), "claim_index is out of range");

  vector<claim> new_claims = c.claims;
  auto clm = c.claims[claim_index];

  eosio_assert(ev_index < 0 || ev_index >= clm.submitted_pending_evidence.size(), "ev_index is out of range");

  vector<string> new_submitted_pending_evidence = clm.submitted_pending_evidence;
  new_submitted_pending_evidence.erase(clm.submitted_pending_evidence.begin() +
                                       ev_index);
  clm.submitted_pending_evidence = new_submitted_pending_evidence;
  new_claims[claim_index] = clm;

  casefiles.modify(c, same_payer, [&](auto &cf) {
    cf.claims = new_claims;
    cf.last_edit = now();
  });

  evidence_table evidences(_self, _self.value);
  auto ev_id = evidences.available_primary_key();

  evidences.emplace(_self, [&](auto &ev) {
    ev.ev_id = ev_id;
    ev.ipfs_url = ipfs_url;
  });

  print("\nEvidence accepted");
}

void arbitration::newarbstatus(uint16_t new_status, name arb) {
  require_auth(arb);

  arbitrators_table arbitrators(_self, _self.value);
  auto ptr_arb = arbitrators.get(arb.value, "Arbitrator not found");

  eosio_assert(new_status >= 0 || new_status <= 3, "arbitrator status doesn't exist");

  arbitrators.modify(ptr_arb, same_payer, [&](auto &a) { 
      a.arb_status = new_status; 
    });

  print("\nArbitrator status updated: SUCCESS");
}

void arbitration::newcfstatus(uint64_t case_id, uint16_t new_status, name arb) {
  require_auth(arb);

  casefiles_table casefiles(_self, _self.value);
  auto c = casefiles.get(case_id, "no case found for given case_id");
  auto arb_case = std::find(c.arbitrators.begin(), c.arbitrators.end(), arb);

  eosio_assert(arb_case != c.arbitrators.end(), "arbitrator isn't selected for this case.");
  eosio_assert(c.case_status != DISMISSED || c.case_status != COMPLETE, "case is dismissed or complete");

  casefiles.modify(c, same_payer, [&](auto &cf) {
    cf.case_status = new_status;
    cf.last_edit = now();
  });

  print("\nCase updated: SUCCESS");
}

void arbitration::recuse(uint64_t case_id, string rationale, name arb) {
  // rationale ???
  require_auth(arb);

  casefiles_table casefiles(_self, _self.value);
  auto c = casefiles.get(case_id, "no case found for given case_id");
  auto arb_case = std::find(c.arbitrators.begin(), c.arbitrators.end(), arb);

  eosio_assert(arb_case != c.arbitrators.end(), "arbitrator isn't selected for this case.");

  vector<name> new_arbs = c.arbitrators;
  remove(new_arbs.begin(), new_arbs.end(), arb);

  casefiles.modify(c, same_payer, [&](auto &cf) {
    cf.arbitrators = new_arbs;
    cf.last_edit = now();
  });

  print("\nArbitrator was removed from the case");
}

// void arbitration::changeclass(uint64_t case_id, uint16_t claim_index, uint16_t new_class, name arb) {
//   require_auth(arb);
//   casefiles_table casefiles(_self, _self.value);
//   auto c = casefiles.get(case_id, "no case found for given case_id");
//   auto arb_case = std::find(c.arbitrators.begin(), c.arbitrators.end(), arb);
//   eosio_assert(arb_case != c.arbitrators.end(), "arbitrator isn't selected for this case.");
//   eosio_assert(claim_index < 0 || claim_index >= c.claims.size(), "claim_index is out of range");
//   vector<claim> new_claims = c.claims;
//   new_claims[claim_index].class_suggestion = new_class;
//   casefiles.modify(c, same_payer, [&](auto &cf) {
//     cf.claims = new_claims;
//     cf.last_edit = now();
//   });
//   print("\nClaim updated: SUCCESS");
// }

// void arbitration::dismissarb(name arb) {
//   require_auth2(arb.value, name("active").value); // REQUIRES 2/3+1 Vote from eosio.prods for MSIG
//   // require_auth2("eosio.prods"_n.value, "active"_n.value); //REQUIRES 2/3+1
//   // Vote from eosio.prods for MSIG
//   arbitrators_table arbitrators(_self, _self.value);
//   auto a = arbitrators.get(arb.value, "Arbitrator Not Found")
//   eosio_assert(a.arb_status != INACTIVE, "Arbitrator is already inactive");
//   arbitrators.modify(a, same_payer, [&](auto &a) {
//        a.arb_status = INACTIVE; 
//     });
//   print("\nArbitrator Dismissed!");
// }

// void arbitration::closecase(uint64_t case_id, name arb, string ipfs_url) {
//   require_auth(arb);
//   validate_ipfs_url(ipfs_url);
//   casefiles_table casefiles(_self, _self.value);
//   auto c = casefiles.get(case_id, "no case found for given case_id");
//   eosio_assert(c.case_status == ENFORCEMENT, "case hasn't been enforced");
//   auto arb_case = std::find(c.arbitrators.begin(), c.arbitrators.end(), arb);
//   eosio_assert(arb_case != c.arbitrators.end(), "arbitrator isn't selected for this case.");
//   auto new_ipfs_list = c.result_links;
//   new_ipfs_list.emplace_back(ipfs_url);
//   casefiles.modify(c, same_payer, [&](auto &cf) {
//     cf.result_links = new_ipfs_list;
//     cf.case_status = COMPLETE;
//     cf.last_edit = now();
//   });
//   print("\nCase Close: SUCCESS");
// }

#pragma endregion Case_Actions


#pragma region Helpers

bool arbitration::is_claimant(name claimant, vector<name> list) {
  auto itr = list.begin();
  while (itr != list.end()) {
    if (*itr == claimant) {
      return true;
    }
    itr++;
  }
  return false;
}

void arbitration::validate_ipfs_url(string ipfs_url) {
	eosio_assert(ipfs_url.length() == 53, "invalid ipfs string, valid schema: /ipfs/<hash>/");
}

arbitration::config arbitration::get_default_config() {
  vector<int64_t> fees{100000, 200000, 300000};
  auto c = config{
      get_self(),  // publisher
      fees,        // fee_structure
      uint16_t(0), // max_elected_arbs
      uint32_t(0), // election_duration
      uint32_t(0), // election_start
      bool(0),     // auto_start_election
      uint64_t(0), // current_ballot_id
      uint32_t(0)  // arb_term_length
  };

  configs.set(c, get_self());

  return c;
}

void arbitration::start_new_election(uint8_t available_seats) {
  uint32_t begin_time = now() + _config.election_start;
  uint32_t end_time = begin_time + _config.election_duration;

  action(permission_level{get_self(), name("active")}, name("eosio.trail"), name("regballot"),
         make_tuple(get_self(),         // publisher
                    uint8_t(2),         // ballot_type (2 == leaderboard)
                    symbol("VOTE", 4),  // voting_symbol
                    begin_time,         // begin_time
                    end_time,           // end_time
                    std::string("")     // info_url
                  )
                ).send();

  action(permission_level{get_self(), name("active")}, name("eosio.trail"), name("setseats"),
         make_tuple(get_self(),
          _config.current_ballot_id, 
          available_seats
        )
      ).send();

      print("\nNew election has started.");
}

bool arbitration::has_available_seats(arbitrators_table &arbitrators, uint8_t &available_seats) {
  uint8_t occupied_seats = 0;
  
  for (auto &arb : arbitrators) {
    // check if arb seat is expired
    if (now() > arb.term_expiration && arb.arb_status != uint16_t(SEAT_EXPIRED)) {
      arbitrators.modify(arb, same_payer, [&](auto &a) {
        a.arb_status = uint16_t(SEAT_EXPIRED);
      });
    }

    if(arb.arb_status != uint16_t(SEAT_EXPIRED)) occupied_seats++;
  }
  available_seats = uint8_t(_config.max_elected_arbs - occupied_seats);
  
  return available_seats > 0;
}

void arbitration::add_arbitrator(arbitrators_table &arbitrators, name arb_name, std::string credential_link) {
  auto arb = arbitrators.find(arb_name.value);
 
  if (arb == arbitrators.end()) {
    arbitrators.emplace(_self, [&](auto &a) {
      a.arb = arb_name;
      a.arb_status = uint16_t(UNAVAILABLE);
      a.term_expiration = now() + _config.arb_term_length;
      a.open_case_ids = vector<uint64_t>();
      a.closed_case_ids = vector<uint64_t>();
      a.credentials_link = credential_link;
    });
  } else {
    arbitrators.modify(arb, same_payer, [&](auto &a){
      a.arb_status = uint16_t(UNAVAILABLE);
      a.term_expiration = now() + _config.arb_term_length;
      a.credentials_link = credential_link;
    });
  }
}

#pragma endregion Helpers

EOSIO_DISPATCH(arbitration, (setconfig)(initelection)(candaddlead)(regcand)
                             (unregcand)(candrmvlead)(endelection)
                             (filecase)(addclaim)(removeclaim)(shredcase)(readycase)
                             (dismisscase)(closecase)(dismissev)(acceptev)
                             (arbstatus)(casestatus)(changeclass)(recuse)(dismissarb))
