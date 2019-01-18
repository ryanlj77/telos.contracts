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

  //TODO: describe each enum in README

  enum case_state : uint8_t {
    CASE_SETUP,         // 0
    AWAITING_ARBS,      // 1
    CASE_INVESTIGATION, // 2
    HEARING,            // 3
    DELIBERATION,       // 4
    DECISION,           // 5 NOTE: No more joinders allowed
    ENFORCEMENT,        // 6
    RESOLVED,           // 7
    DISMISSED           // 8 NOTE: Dismissed cases advance and stop here
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
    ENGL, //0 English 
    FRCH, //1 French
    GRMN, //2 German
    KREA, //3 Korean
    JAPN, //4 Japanese
    CHNA, //5 Chinese
    SPAN, //6 Spanish
    PGSE, //7 Portuguese
    SWED //8 Swedish
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
  void regcand(name candidate, string credentials_link);

  [[eosio::action]]
  void unregcand(name candidate);

  [[eosio::action]]
  void candaddlead(name candidate, string credentials_link);
  
  [[eosio::action]]
  void candrmvlead(name candidate);

  [[eosio::action]]
  void endelection(name candidate);
                                                      
#pragma endregion Arb_Elections

#pragma region Case_Setup

  //NOTE: filing a case doesn't require a respondent
  [[eosio::action]]
  void filecase(name claimant, string claim_link);

  //NOTE: adds subsequent claims to a case
  [[eosio::action]]
  void addclaim(uint64_t case_id, string claim_link, name claimant);

  //NOTE: claims can only be removed by a claimant during case setup
  [[eosio::action]]
  void removeclaim(uint64_t case_id, uint64_t claim_id, name claimant);

  //NOTE: member-level case removal, called during CASE_SETUP
  [[eosio::action]]
  void shredcase(uint64_t case_id, name claimant);

  //NOTE: enforce claimant has at least 1 claim before readying                                 
  [[eosio::action]]
  void readycase(uint64_t case_id, name claimant);

#pragma endregion Case_Setup

#pragma region Case_Actions

  [[eosio::action]]
  void assigntocase(uint64_t case_id, name arb);

  [[eosio::action]]
  void advancecase(uint64_t case_id, name arb);

  //TODO: require rationale?
  [[eosio::action]]
  void dismisscase(uint64_t case_id, name arb, string ipfs_link, string comment);

  [[eosio::action]]
  void dismissclaim(uint64_t case_id, name arb, string claim_hash);

  //NOTE: moves to evidence_table and assigns ID
  [[eosio::action]]
  void acceptclaim(uint64_t case_id, uint16_t claim_index, uint16_t ev_index, name arb, string ipfs_url);

  [[eosio::action]]
  void newarbstatus(uint16_t new_status, name arb);

  [[eosio::action]]
  void newcfstatus(uint64_t case_id, uint16_t new_status, name arb);

  [[eosio::action]]
  void recuse(uint64_t case_id, string rationale, name arb);




  //TODO: require decision?
  [[eosio::action]]
  void closecase(uint64_t case_id, name arb, string ipfs_url); //TODO: rename to resolvecase()?

  [[eosio::action]]
  void changeclass(uint64_t case_id, uint16_t claim_index, uint16_t new_class, name arb);

  [[eosio::action]]
  void joincases(vector<uint64_t> case_ids, name arb);

  //NOTE: member version is submitev()
  [[eosio::action]]
  void addevidence(uint64_t case_id, vector<uint64_t> ipfs_urls, name arb);

  [

#pragma endregion Case_Actions

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
  struct[[eosio::table]] candidate {
    name candidate_name;
    string credentials_link;
    uint32_t application_time;

    uint64_t primary_key() const { return candidate_name.value; }
    EOSLIB_SERIALIZE(candidate, (candidate_name)(credentials_link)(application_time))
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
  struct [[eosio::table]] claim {
    uint64_t claim_id;
    string claim_summary; //NOTE: ipfs link to claim document
    string decision_link; //NOTE: ipfs link to decision document

    uint64_t primary_key() const { return claim_id; }
    EOSLIB_SERIALIZE(claim, (claim_id)(claim_summary)(decision_link))
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
    vector<name> arbitrators;
    vector<uint8_t> required_langs;

    vector<uint64_t> unread_claims; //TODO: make vector of claims? don't need to save claims to table unless accepted
    vector<uint64_t> accepted_claims;
    //TODO: string case_ruling? //discrete document like claims?
    
    string arb_comment;
    uint32_t last_edit;

    uint64_t primary_key() const { return case_id; }
    EOSLIB_SERIALIZE(casefile, (case_id)(case_status)
      (claimants)(respondants)(arbitrators)(required_langs)
      (submitted_claims)(accepted_claims)
      (arb_comment)(last_edit))
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
    name joined_by;

    uint64_t primary_key() const { return join_id; }
    EOSLIB_SERIALIZE(joinder, (join_id)(cases)(join_time)(joined_by))
  };

  typedef multi_index<name("candidates"), candidate> candidates_table;

  typedef multi_index<name("arbitrators"), arbitrator> arbitrators_table;

  typedef multi_index<name("casefiles"), casefile> casefiles_table;

  typedef multi_index<name("joinedcases"), joinder> joinders_table;

  typedef multi_index<name("claims"), claim> claims_table;

  typedef singleton<name("config"), config> config_singleton;
  config_singleton configs;
  config _config;

#pragma endregion Tables and Structs

#pragma region Helpers

  bool is_claimant(name claimant, vector<name> list);

  void validate_ipfs_url(string ipfs_url);
  
  config get_default_config();
  
  void start_new_election(uint8_t available_seats);
  
  bool has_available_seats(arbitrators_table &arbitrators, uint8_t &available_seats);

  void add_arbitrator(arbitrators_table &arbitrators, name arb_name, std::string credential_link);

#pragma endregion Helpers

};
