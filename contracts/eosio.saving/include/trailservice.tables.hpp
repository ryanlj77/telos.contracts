#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

//======================== trailservice tables ========================

//scope: get_self().value
struct treasury_struct {
    asset supply; //current supply
    asset max_supply; //maximum supply
    name access; //public, private, invite, membership
    name manager; //treasury manager

    string title;
    string description;
    string icon;

    uint32_t voters;
    uint32_t delegates;
    uint32_t committees;
    uint32_t open_ballots;

    bool locked; //locks all settings
    name unlock_acct; //account name to unlock
    name unlock_auth; //authorization name to unlock
    map<name, bool> settings; //setting_name -> on/off

    uint64_t primary_key() const { return supply.symbol.code().raw(); }
    EOSLIB_SERIALIZE(treasury_struct, 
        (supply)(max_supply)(access)(manager)
        (title)(description)(icon)
        (voters)(delegates)(committees)(open_ballots)
        (locked)(unlock_acct)(unlock_auth)(settings))
};
typedef multi_index<name("treasuries"), treasury_struct> ts_treasuries_table;
