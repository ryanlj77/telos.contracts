#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

struct token_settings {
    bool is_destructible = false;
    bool is_proxyable = false; //NOTE: allows proxy system
    bool is_burnable = false; //NOTE: can only burn from own balance
    bool is_seizable = false;
    bool is_max_mutable = false;
    bool is_transferable = false; //NOTE: allows counerbalances and cb decay
    bool is_recastable = false;
    bool is_initialized = false;

    uint32_t counterbal_decay_rate = 300; //seconds to decay by 1 whole token
    bool lock_after_initialize = true;
};

struct legacy_ballot {
    uint64_t ballot_id;
    uint8_t table_id;
    uint64_t reference_id;

    uint64_t primary_key() const { return ballot_id; }
    EOSLIB_SERIALIZE(legacy_ballot, (ballot_id)(table_id)(reference_id))
};
typedef multi_index<name("ballots"), legacy_ballot> leg_ballots_table;

struct legacy_proposal {
    uint64_t prop_id;
    name publisher;
    string info_url;
    
    asset no_count;
    asset yes_count;
    asset abstain_count;
    uint32_t unique_voters;

    uint32_t begin_time;
    uint32_t end_time;
    uint16_t cycle_count;
    uint8_t status; // 0 = OPEN, 1 = PASS, 2 = FAIL

    uint64_t primary_key() const { return prop_id; }
    EOSLIB_SERIALIZE(legacy_proposal, (prop_id)(publisher)(info_url)
        (no_count)(yes_count)(abstain_count)(unique_voters)
        (begin_time)(end_time)(cycle_count)(status))
};
typedef multi_index<name("proposals"), legacy_proposal> leg_proposals_table;

struct legacy_registry {
    asset max_supply;
    asset supply;
    uint32_t total_voters;
    uint32_t total_proxies;
    name publisher;
    string info_url;
    token_settings settings;


    uint64_t primary_key() const { return max_supply.symbol.code().raw(); }
    EOSLIB_SERIALIZE(legacy_registry, (max_supply)(supply)(total_voters)(total_proxies)(publisher)(info_url)(settings))
};
typedef multi_index<name("registries"), legacy_registry> leg_registries_table;
