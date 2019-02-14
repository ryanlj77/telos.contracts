/**
 * Trail is an EOSIO-based voting service that allows users to create ballots that
 * are voted on by the network of registered voters. It also offers custom token
 * features that let any user to create their own token and configure token settings 
 * to match a wide variety of intended use cases. 
 * 
 * @author Craig Branscom
 */

#include "trail.voting.hpp"
#include "trail.system.hpp"
#include "trail.tokens.hpp"

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/dispatcher.hpp>
#include <string>

using namespace eosio;

class [[eosio::contract("eosio.trail")]] trail : public contract {
    
public:

    trail(name self, name code, datastream<const char*> ds);

    ~trail();

    environment_singleton environment;
    env env_struct;

    #pragma region Constants

    uint64_t const VOTE_ISSUE_RATIO = 1; //indicates a 1:1 TLOS/VOTE issuance

    uint32_t const MIN_LOCK_PERIOD = 86400; //86,400 seconds is ~1 day

    uint32_t const DECAY_RATE = 120; //number of seconds to decay by 1 VOTE

    const symbol VOTE_SYM = symbol("VOTE", 4); //TODO: should be 0 precision?

    //TODO: add constants for totals vector mappings?

    #pragma endregion Constants


    #pragma region Tokens
    
    /**
     * Token Registration
     * 
     * 
     */

    [[eosio::action]]
    void regtoken(asset max_supply, name publisher, string info_url);

    //TODO: void settokenurl(name publisher, string info_url);

    [[eosio::action]]
    void initsettings(name publisher, symbol token_symbol, token_settings new_settings);

    [[eosio::action]]
    void unregtoken(symbol token_symbol, name publisher);

    /**
     * Token Actions
     * 
     * 
     */

    [[eosio::action]]
    void issuetoken(name publisher, name recipient, asset tokens, bool airgrab);

    [[eosio::action]] //TODO: remove pulisher as param? is findable through token symbol (implemented, just need to remove from signature)
    void claimairgrab(name claimant, name publisher, symbol token_symbol);

    [[eosio::action]] //TODO: make is_burnable_by_publisher/is_burnable_by_holder?
    void burntoken(name balance_owner, asset amount);

    [[eosio::action]]
    void seizetoken(name publisher, name owner, asset tokens); //TODO: add memo?

    [[eosio::action]]
    void seizeairgrab(name publisher, name recipient, asset amount); //TODO: add memo?

    [[eosio::action]]
    void seizebygroup(name publisher, vector<name> group, asset amount);

    [[eosio::action]]
    void raisemax(name publisher, asset amount);

    [[eosio::action]]
    void lowermax(name publisher, asset amount);

    [[eosio::action]] //TODO: rename to something else? may be confused with eosio.token transfer
    void transfer(name sender, name recipient, asset amount); //TODO: add memo?

    #pragma endregion Tokens


    #pragma region Voting

    /**
     * Voter Registration
     * 
     * 
     */

    [[eosio::action]] //effectively createwallet()
    void regvoter(name voter, symbol token_symbol);

    [[eosio::action]] //effectively deletewallet()
    void unregvoter(name voter, symbol token_symbol);

    /**
     * Voter Actions
     * 
     * 
     */

    //TODO: make into regular function?
    [[eosio::action]]
    void mirrorcast(name voter, symbol token_symbol);

    [[eosio::action]] //TODO: change to name ballot_name, uint8_t direction
    void castvote(name voter, uint64_t ballot_id, uint16_t direction);

    //TODO: void unvote();

    [[eosio::action]] //TODO: add begin_point param? then we could start iterating anywhere in the table
    void deloldvotes(name voter, uint16_t num_to_delete); //TODO: rename to cleanupvotes

    #pragma endregion Voting


    #pragma region Proxies

    /**
     * Proxy Registration
     * 
     * 
     */

    //TODO: void regproxy(name proxy, symbol voting_token);

    //TODO: void unregproxy(name proxy, symbol voting_token);

    /**
     * Proxy Actions
     * 
     * 
     */

    #pragma endregion Proxies


    #pragma region Ballots

    /**
     * Ballot Registration
     * 
     * 
     */

    [[eosio::action]]
    void regballot(name publisher, uint8_t ballot_type, symbol voting_symbol,
        uint32_t begin_time, uint32_t end_time, string info_url);

    [[eosio::action]] //TODO: change to name ballot_name
    void unregballot(name publisher, uint64_t ballot_id);

    //TODO: void withdrawbal(); //maybe?

    //action to replace ballot publisher's RAM with Trail's RAM. Could require TLOS payment?
    //TODO: archivebal()

    /**
     * Ballot Actions
     * 
     * 
     */

    [[eosio::action]] //TODO: allow in elections
    void addcandidate(name publisher, uint64_t ballot_id, name new_candidate, string info_link);

    [[eosio::action]] //TODO: allow in elections
    void rmvcandidate(name publisher, uint64_t ballot_id, name candidate);

    [[eosio::action]] //TODO: allow in elections
    void setallcands(name publisher, uint64_t ballot_id, vector<candidate> new_candidates);

    [[eosio::action]] //TODO: allow in elections
    void setallstats(name publisher, uint64_t ballot_id, vector<uint8_t> new_cand_statuses);

    [[eosio::action]]
    void setseats(name publisher, uint64_t ballot_id, uint8_t num_seats);

    [[eosio::action]] //TODO: remove if wps cycles change
    void nextcycle(name publisher, uint64_t ballot_id, uint32_t new_begin_time, uint32_t new_end_time);

    [[eosio::action]] //TODO: rename pass to final_status
    void closeballot(name publisher, uint64_t ballot_id, uint8_t pass);

    #pragma endregion Ballots


    #pragma region Helpers

    //TODO: turn into discrete actions? would make ABI much larger

    uint64_t make_proposal(name publisher, symbol voting_symbol, uint32_t begin_time, uint32_t end_time, string info_url);

    bool delete_proposal(uint64_t prop_id, name publisher);

    bool vote_for_proposal(name voter, uint64_t ballot_id, uint64_t prop_id, uint16_t direction);

    //TODO: rename pass to something else
    bool close_proposal(uint64_t prop_id, uint8_t pass, name publisher);


    uint64_t make_election(name publisher, symbol voting_symbol, uint32_t begin_time, uint32_t end_time, string info_url);

    bool delete_election(uint64_t elec_id, name publisher);

    bool vote_for_election(name voter, uint64_t ballot_id, uint64_t elec_id, uint16_t direction);
    
    bool close_election(uint64_t elec_id, uint8_t pass, name publisher);


    uint64_t make_leaderboard(name publisher, symbol voting_symbol, uint32_t begin_time, uint32_t end_time, string info_url);

    bool delete_leaderboard(uint64_t board_id, name publisher);

    bool vote_for_leaderboard(name voter, uint64_t ballot_id, uint64_t board_id, uint16_t direction);

    bool close_leaderboard(uint64_t board_id, uint8_t pass, name publisher);


    // uint64_t make_poll();
    // bool delete_poll();
    // bool vote_for_poll();
    // bool close_poll();


    asset get_vote_weight(name voter, symbol voting_token);

    //TODO: change to vector of uint8_t
    bool has_direction(uint16_t direction, vector<uint16_t> direction_list);

    //TODO: rename to map_cand_stats?
    vector<candidate> set_candidate_statuses(vector<candidate> candidate_list, vector<uint8_t> new_status_list);

    #pragma endregion Helpers


    #pragma region Reactions

    //Reactions are regular functions called only as a trigger from the dispatcher.

    // void update_from_cb(name from, asset amount);
    // void update_to_cb(name to, asset amount);
    // asset get_decay_amount(name voter, symbol token_symbol, uint32_t decay_rate);

    #pragma endregion Reactions


    #pragma region Admin

    [[eosio::action]]
    void pause();

    [[eosio::action]]
    void resume();

    #pragma endregion Admin


    #pragma region Migration

    [[eosio::action]]
    void releasecb(name voter);

    [[eosio::action]]
    void resetreg();

    [[eosio::action]]
    void resetbal(name voter);

    [[eosio::action]]
    void makeglobals();

    #pragma endregion Migration

};
