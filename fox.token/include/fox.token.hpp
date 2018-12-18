/**
 * Fox Token Definition
 * 
 * @author Craig Branscom
 * @copyright defined in telos/LICENSE.txt
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/transaction.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract("fox.token")]] foxtoken : public contract {

public:

    foxtoken(name self, name code, datastream<const char*> ds);

    ~foxtoken();

    symbol const NATIVE_SYMBOL = symbol("FOX", 3);

    struct [[eosio::table]] balance {
        name owner;
        asset tokens;
        asset on_order;

        uint64_t primary_key() const { return owner.value; }
        EOSLIB_SERIALIZE(balance, (owner)(tokens)(on_order))
    };
    
    typedef multi_index<name("balances"), balance> balances_table;

    [[eosio::action]] void mint(name recipient, asset tokens);

    [[eosio::action]] void transfer(name sender, name recipient, asset tokens);

    [[eosio::action]] void createwallet(name user);

    [[eosio::action]] void deletewallet(name user);

};
