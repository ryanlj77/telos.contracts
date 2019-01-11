/**
 * Arbitration Contract Interface
 *
 * @author Craig Branscom, Peter Bue, Ed Silva, Douglas Horn
 * @copyright defined in telos/LICENSE.txt
 */

#pragma once
#include <trail.voting.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/types.h>

using namespace std;
using namespace eosio;

class[[eosio::contract("eosio.arbitration")]] arbitration : public eosio::contract {
public:
  using contract::contract;
  
  #pragma region Enums

  enum case_state : uint8_t {
    CASE_SETUP,         // 0
    AWAITING_ARBS,      // 1
    CASE_INVESTIGATION, // 2
    DISMISSED,          // 3
    HEARING,            // 4
    DELIBERATION,       // 5
    DECISION,           // 6 NOTE: No more joinders allowed
    ENFORCEMENT,        // 7
    COMPLETE            // 8
  };

  enum claim_class : uint8_t {
    UNDECIDED,           // 0
    LOST_KEY_RECOVERY,   // 1
    TRX_REVERSAL,        // 2
    EMERGENCY_INTER,     // 3
    CONTESTED_OWNER,     // 4
    UNEXECUTED_RELIEF,   // 5
    CONTRACT_BREACH,     // 6
    MISUSED_CR_IP,       // 7
    A_TORT,              // 8
    BP_PENALTY_REVERSAL, // 9
    WRONGFUL_ARB_ACT,    // 10
    ACT_EXEC_RELIEF,     // 11
    WP_PROJ_FAILURE,     // 12
    TBNOA_BREACH,        // 13
    MISC                 // 14
  };

  enum arb_status : uint8_t {
    AVAILABLE,   // 0
    UNAVAILABLE, // 1
    INACTIVE,    // 2
    SEAT_EXPIRED // 3
  };

  enum election_status : uint8_t {
    OPEN,   // 0
    PASSED, // 1
    FAILED, // 2
    CLOSED  // 3
  };

  enum lang_code : uint8_t {
    ENGL, //English 
    FRCH, //French
    GRMN, //German
    KREA, //Korean
    JAPN, //Japanese
    CHNA, //Chinese
    SPAN, //Spanish
    PGSE, //Portuguese
    SWED //Swedish
  };

#pragma endregion Enums

  arbitration(name s, name code, datastream<const char *> ds);
  
  ~arbitration();

  [[eosio::action]]
  void setconfig(uint16_t max_elected_arbs, uint32_t election_duration, uint32_t start_election, uint32_t arbitrator_term_length, vector<int64_t> fees);

#pragma region Arb_Elections

  [[eosio::action]]
  void initelection();

  [[eosio::action]]
  void regcand(name candidate, string creds_ipfs_url);

  [[eosio::action]]
  void unregcand(name candidate);

  [[eosio::action]]
  void candaddlead(name candidate, string creds_ipfs_url);
  
  [[eosio::action]]
  void candrmvlead(name candidate);

  [[eosio::action]]
  void endelection(name candidate);
                                                      
#pragma endregion Arb_Elections

#pragma region Case_Setup

  //NOTE: filing a case doesn't require a respondent
  [[eosio::action]]
  void filecase(name claimant, uint8_t class_suggestion, string ev_ipfs_url);

  //NOTE: adds subsequent claims to a case
  [[eosio::action]]
  void addclaim( uint64_t case_id, uint16_t class_suggestion, string ev_ipfs_url, name claimant);

  //NOTE: Claims can only be removed by a claimant during case setup. Enforce they have atleast one claim before awaiting arbs.
  [[eosio::action]]
  void removeclaim(uint64_t case_id, uint16_t claim_num, name claimant);

  //NOTE: member-level case removal, called during CASE_SETUP
  [[eosio::action]]
  void shredcase( uint64_t case_id, name claimant);
                                                                     
  [[eosio::action]]
  void readycase(uint64_t case_id, name claimant);

#pragma endregion Case_Setup

#pragma region Arb_Only

  //TODO: Set case respondant action
  //TODO: require rationale?
  [[eosio::action]]
  void dismisscase(uint64_t case_id, name arb, string ipfs_url);

  //TODO: require decision?
  [[eosio::action]]
  void closecase(uint64_t case_id, name arb, string ipfs_url);

  [[eosio::action]]
  void dismissev( uint64_t case_id, uint16_t claim_index, uint16_t ev_index, name arb, string ipfs_url);

  //NOTE: moves to evidence_table and assigns ID
  [[eosio::action]]
  void acceptev(uint64_t case_id, uint16_t claim_index, uint16_t ev_index, name arb, string ipfs_url);

  [[eosio::action]]
  void arbstatus(uint16_t new_status, name arb);

  [[eosio::action]]
  void casestatus(uint64_t case_id, uint16_t new_status, name arb);

  [[eosio::action]]
  void changeclass(uint64_t case_id, uint16_t claim_index, uint16_t new_class, name arb);

  [[eosio::action]]
  void joincases(vector<uint64_t> case_ids, name arb);

  //NOTE: member version is submitev()
  [[eosio::action]]
  void addevidence(uint64_t case_id, vector<uint64_t> ipfs_urls, name arb);

  [[eosio::action]]
  void recuse(uint64_t case_id, string rationale, name arb);

#pragma endregion Arb_Only

#pragma region BP_Multisig_Actions

  [[eosio::action]] 
  void dismissarb(name arb);

#pragma endregion BP_Multisig_Actions

#pragma region System Structs

  struct permission_level_weight {
    permission_level permission;
    uint16_t weight;

    EOSLIB_SERIALIZE(permission_level_weight, (permission)(weight))
  };

  struct key_weight {
    eosio::public_key key;
    uint16_t weight;

    EOSLIB_SERIALIZE(key_weight, (key)(weight))
  };

  struct wait_weight {
    uint32_t wait_sec;
    uint16_t weight;

    EOSLIB_SERIALIZE(wait_weight, (wait_sec)(weight))
  };

  struct authority {
    uint32_t threshold = 0;
    std::vector<key_weight> keys;
    std::vector<permission_level_weight> accounts;
    std::vector<wait_weight> waits;

    EOSLIB_SERIALIZE(authority, (threshold)(keys)(accounts)(waits))
  };

#pragma endregion System Structs

protected:
#pragma region Tables and Structs

  /**
   * Holds all arbitrator candidate applications.
   * @scope get_self().value
   * @key uint64_t candidate_name.value
   */
  struct[[eosio::table]] pending_candidate {
    name candidate_name;
    string credentials_link; //NOTE: ideally ipfs hash
    uint32_t application_time;

    uint64_t primary_key() const { return candidate_name.value; }
    EOSLIB_SERIALIZE(pending_candidate, (candidate_name)(credentials_link)(application_time))
  };

  /**
   * Holds all currently elected arbitrators.
   * @scope get_self().value
   * @key uint64_t arb.value
   */
  struct [[eosio::table]] arbitrator {
    name arb;
    uint8_t arb_status;
    vector<uint64_t> open_case_ids;
    vector<uint64_t> closed_case_ids;
    string credentials_link; //NOTE: ipfs_url of arbitrator credentials
    uint32_t elected_time;
    uint32_t term_expiration;
    vector<uint8_t> languages; //NOTE: language codes

    uint64_t primary_key() const { return arb.value; }
    EOSLIB_SERIALIZE(arbitrator, (arb)(arb_status)(open_case_ids)(closed_case_ids)
      (credentials_link)(elected_time)(term_expiration)(languages))
  };

  //NOTE: Stores all information related to a single claim.
  struct claim {
    uint16_t class_suggestion;
    vector<string> submitted_pending_evidence; //NOTE: submitted by claimant
    vector<uint64_t> accepted_ev_ids;          //NOTE: accepted and emplaced by arb
    uint16_t class_decision;                   //NOTE: initialized to UNDECIDED (0)

    EOSLIB_SERIALIZE(claim, (class_suggestion)(submitted_pending_evidence)(accepted_ev_ids)(class_decision))
  };

  /**
   * Case Files for all arbitration cases.
   * @scope get_self().value
   * @key case_id
   */
  struct[[eosio::table]] casefile {
    uint64_t case_id;
    uint8_t case_status;

    vector<name> claimants;
    vector<name> respondants; //NOTE: empty for no respondant
    vector<name> arbitrators; //CLARIFY: do arbitrators get added when joining?
    vector<uint8_t> required_langs;

    vector<claim> claims;
    vector<string> result_links; //NOTE: mapped key to key with claims vector i.e. result of claim[5] is results_link[5]
    
    string comment;
    uint32_t last_edit;

    //vector<asset> additional_fees; //NOTE: case by case?
    //bool is_joined; //NOTE: unnecessary?

    uint64_t primary_key() const { return case_id; }
    EOSLIB_SERIALIZE(casefile, (case_id)(case_status)
      (claimants)(respondants)(arbitrators)(required_langs)
      (claims)(result_links)
      (comment)(last_edit))
  };

  /**
   * Stores evidence accepted from arbitration cases.
   * @scope get_self().value
   * @key uint64_t ev_id
   */
  //TODO: evidence types?
  struct [[eosio::table]] evidence {
    uint64_t ev_id;
    string ipfs_url;
    uint32_t accept_time;
    name accepted_by;

    //vector<uint8_t> doc_langs; //maybe?

    uint64_t primary_key() const { return ev_id; }
    EOSLIB_SERIALIZE(evidence, (ev_id)(ipfs_url)(accept_time)(accepted_by))
  };

  /**
   * Singleton for global config settings.
   * @scope singleton scope (get_self().value)
   * @key table name
   */
  //TODO: make fee structure a constant?
  // NOTE: diminishing subsequent response (default) times
  // NOTE: initial deposit saved
  // NOTE: class of claim where neither party can pay fees, TF pays instead
  struct [[ eosio::table ]] config {
    name publisher;
    vector<int64_t> fee_structure; //NOTE: always in TLOS so only store asset.amount value

    uint16_t max_elected_arbs;
    uint32_t election_duration;
    uint32_t election_start;
    bool auto_start_election = false;
    uint64_t current_ballot_id = 0;
    uint32_t arb_term_length;

    //TODO: response times vector?
    //TODO: 

    uint64_t primary_key() const { return publisher.value; }
    EOSLIB_SERIALIZE(config, (publisher)(fee_structure)
    (max_elected_arbs)(election_duration)(election_start)(auto_start_election)(current_ballot_id)(arb_term_length))
  };

  /**
   * Holds instances of joinder cases.
   * @scope get_self().value
   * @key uint64_t join_id
   */
  struct [[eosio::table]] joinder {
    uint64_t join_id;
    vector<uint64_t> cases;
    uint32_t join_time;

    uint64_t primary_key() const { return join_id; }
    EOSLIB_SERIALIZE(joinder, (join_id)(cases)(join_time))
  };

  typedef multi_index<name("pendingcands"), pending_candidate> pending_candidates_table;

  typedef multi_index<name("arbitrators"), arbitrator> arbitrators_table;

  typedef multi_index<name("casefiles"), casefile> casefiles_table;

  typedef multi_index<name("joinedcases"), joinder> joinders_table;

  typedef multi_index<name("evidence"), evidence> evidence_table;

  typedef singleton<name("config"), config> config_singleton;
  config_singleton configs;
  config _config;

#pragma endregion Tables and Structs


  void validate_ipfs_url(string ipfs_url);
  
  config get_default_config();
  
  void start_new_election(uint8_t available_seats);
  
  bool has_available_seats(arbitrators_table &arbitrators, uint8_t &available_seats);

  void add_arbitrator(arbitrators_table &arbitrators, name arb_name, std::string credential_link);

};
