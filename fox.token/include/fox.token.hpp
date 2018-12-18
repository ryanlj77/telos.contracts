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

    //NOTE: coded by BUY symbol, scoped by SELL symbol (scope by match rate instead?)
    //TODO: secondary index by match rate? (sec idx by SELL symbol?)
    struct [[eosio::action]] order {
        uint64_t order_id;
        symbol quote_symbol;
        double rate;

        uint64_t primary_key() const { return order_id; }
        uint64_t by_quote_sym() const { return quote_symbol.code().raw(); }
        EOSLIB_SERIALIZE(order, (order_id)(quote_symbol)(rate))
    };
    
    typedef multi_index<name("balances"), balance> balances_table;

    typedef multi_index<name("orders"), order, 
        indexed_by<name("byquotesym"), eosio::const_mem_fun<order, uint64_t, &order::by_quote_sym>>> orders_table;

    [[eosio::action]] void mint(name recipient, asset tokens);

    [[eosio::action]] void transfer(name sender, name recipient, asset tokens);

    [[eosio::action]] void createwallet(name user);

    [[eosio::action]] void deletewallet(name user);

    [[eosio::action]] void placeorder(name orderer, asset buy, asset sell);


    name find_domain(symbol sym);
};
