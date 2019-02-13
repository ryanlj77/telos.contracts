/**
 * This file includes all definitions necessary to interact with Trail's voting system. Developers who want to
 * utilize the system simply must include this file in their implementation to interact with the information
 * stored by Trail.
 * 
 * @author Craig Branscom
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/singleton.hpp>

using namespace std;
using namespace eosio;

#pragma region Structs

//@scope voter (easy to loop over for deletion)
struct [[eosio::table, eosio::contract("eosio.trail")]] vote_receipt {
    uint64_t ballot_id;
    vector<uint16_t> directions; //TODO: change to vector of uint8_t
    asset weight; //TODO: make vector of weights? directions[i] => weights[i]
    uint32_t expiration;

    uint64_t primary_key() const { return ballot_id; }
    EOSLIB_SERIALIZE(vote_receipt, (ballot_id)(directions)(weight)(expiration))
};

// struct [[eosio::table, eosio::contract("eosio.trail")]] temp_receipt {
//     uint64_t ballot_id;
//     vector<uint16_t> directions; //TODO: change to vector of uint8_t
//     asset weight; //TODO: make vector of weights? directions[i] => weights[i]
//     uint32_t expiration;
//     uint64_t primary_key() const { return ballot_id; }
//     EOSLIB_SERIALIZE(vote_receipt, (ballot_id)(directions)(weight)(expiration))
// };

enum candidate_status : uint8_t {
    REGISTERED, //0
    RUNNING, //1
    RESIGN //2
};

struct candidate {
    name member;
    string info_link;
    asset votes;
    uint8_t status;
};

//@scope name("eosio.trail").value
struct [[eosio::table, eosio::contract("eosio.trail")]] ballot {
    uint64_t ballot_id; //TODO: change to ballot_name?
    uint8_t table_id; //TODO: change to table_name
    uint64_t reference_id; //TODO: rename local_ballot_name?

    uint64_t primary_key() const { return ballot_id; }
    EOSLIB_SERIALIZE(ballot, (ballot_id)(table_id)(reference_id))
};

enum proposal_status : uint8_t {
    UNDECIDED, //0
    PASSED, //1
    FAILED, //2
    RESERVED_PROP_1, //3 reserved for development
    RESERVED_PROP_2 //4 //reserved for development
};

//@scope name("eosio.trail").value
struct [[eosio::table, eosio::contract("eosio.trail")]] proposal {
    uint64_t prop_id; //TODO: change to prop_name
    name publisher;
    //TODO: string prop_title;
    //TODO: string description; //recommend markdown
    string info_url;
    
    asset no_count;
    asset yes_count;
    asset abstain_count;
    uint32_t unique_voters;

    uint32_t begin_time;
    uint32_t end_time;
    uint16_t cycle_count; //TODO: remove if wps cycles change
    uint8_t status; //TODO: rename prop_status?

    uint64_t primary_key() const { return prop_id; }
    EOSLIB_SERIALIZE(proposal, (prop_id)(publisher)(info_url)
        (no_count)(yes_count)(abstain_count)(unique_voters)
        (begin_time)(end_time)(cycle_count)(status))
};

enum election_status : uint8_t {
    CAMPAIGN, //0
    VOTING_OPEN, //1
    VOTING_CLOSED, //2
    RESERVED_ELEC_1, //3 reserved for development
    RESERVED_ELEC_2 //4 //reserved for development
};

//@scope name("eosio.trail").value
struct [[eosio::table, eosio::contract("eosio.trail")]] election {
    uint64_t election_id; //TODO: change to election_name
    name publisher;
    //TODO: string election_title;
    //TODO: string description; //recommend markdown
    string info_url;

    vector<candidate> candidates;
    uint32_t unique_voters;
    symbol voting_symbol;
    
    uint32_t begin_time;
    uint32_t end_time;
    //TODO: uint8_t election_status;

    uint64_t primary_key() const { return election_id; }
    EOSLIB_SERIALIZE(election, (election_id)(publisher)(info_url)
        (candidates)(unique_voters)(voting_symbol)
        (begin_time)(end_time))
};

enum leaderboard_status : uint8_t {
    SETUP, //0
    BOARD_OPEN, //1
    BOARD_CLOSED, //2
    RESERVED_BOARD_1, //3
    RESERVED_BOARD_2 //4
};

//@scope name("eosio.trail").value
struct [[eosio::table, eosio::contract("eosio.trail")]] leaderboard {
    uint64_t board_id; //TODO: change to board_name;
    name publisher;
    //TODO: string board_title;
    //TODO: string desciption; //recommend markdown
    string info_url;

    vector<candidate> candidates;
    uint32_t unique_voters;
    symbol voting_symbol;
    uint8_t available_seats; //TODO: rename available_slots?

    uint32_t begin_time;
    uint32_t end_time;
    uint8_t status; //TODO: rename board_status?

    uint64_t primary_key() const { return board_id; }
    EOSLIB_SERIALIZE(leaderboard, (board_id)(publisher)(info_url)
        (candidates)(unique_voters)(voting_symbol)(available_seats)
        (begin_time)(end_time)(status))
};

/**
 * totals vector mappings:
 *     totals[0] => total proposals
 *     totals[1] => total elections
 *     totals[2] => total leaderboards
 *     totals[3] => total polls
 */
//TODO: consider renaming to globals
struct [[eosio::table("environment"), eosio::contract("eosio.trail")]] env {
    name publisher;
    vector<uint64_t> totals; //TODO: change to vector of table names?
    uint32_t time_now; //TODO: remove?
    uint64_t last_ballot_id; //TODO: remove?

    uint64_t primary_key() const { return publisher.value; }
    EOSLIB_SERIALIZE(env, (publisher)(totals)(time_now)(last_ballot_id))
};

//NOTE: proxy receipts are scoped by voter (proxy)
// struct [[eosio::table, eosio::contract("eosio.trail")]] proxy_receipt {
//     uint64_t ballot_id;
//     vector<uint16_t> directions;
//     asset weight;
//     uint64_t primary_key() const { return ballot_id; }
//     EOSLIB_SERIALIZE(proxy_receipt, (ballot_id)(directions)(weight))
// };

//TODO: should proxies also be registered voters? does it matter?
//TODO: scope by proxied token symbol? or proxy name?
// struct [[eosio::table, eosio::contract("eosio.trail")]] proxy_id {
//     name proxy;
//     asset proxied_votes;
//     string info_url;
//     uint32_t num_constituants;
//     uint64_t primary_key() const { return proxy.value; }
//     EOSLIB_SERIALIZE(proxy_id, (proxy)(proxied_votes)(info_url)(num_constituants))
// };

#pragma endregion Structs


#pragma region Tables

//typedef multi_index<name("proxies"), proxy_id> proxies_table; //TODO: necessary? 

typedef multi_index<name("ballots"), ballot> ballots_table;
//typedef multi_index<name("tempballots"), ballot> temp_ballots;

typedef multi_index<name("proposals"), proposal> proposals_table;
//typedef multi_index<name("tempprops"), proposal> temp_proposals;

typedef multi_index<name("elections"), election> elections_table;
//typedef multi_index<name("tempelecs"), election> temp_elections;

typedef multi_index<name("leaderboards"), leaderboard> leaderboards_table;
//typedef multi_index<name("tempboards"), leaderboard> temp_leaderboards;

typedef multi_index<name("votereceipts"), vote_receipt> votereceipts_table;
//typedef multi_index<name("tempvotes"), vote_receipt> temp_votereceipts;

//typedef multi_index<name("proxreceipts"), proxy_receipt> proxyreceipts_table;

typedef singleton<name("environment"), env> environment_singleton;

#pragma endregion Tables


#pragma region Helper_Functions

bool is_ballot(uint64_t ballot_id) {
    ballots_table ballots(name("eosio.trail"), name("eosio.trail").value);
    auto b = ballots.find(ballot_id);

    if (b != ballots.end()) {
        return true;
    }

    return false;
}

// for when ballots switch to name keys
// bool is_ballot(name ballot_name) {
//     ballots ballots(name("eosio.trail"), name("eosio.trail").value);
//     auto bal_itr = ballots.find(ballot_name.value);
//     if (bal_itr != ballots.end()) {
//         return true;
//     }
//     return false;
// }

#pragma endregion Helper_Functions
