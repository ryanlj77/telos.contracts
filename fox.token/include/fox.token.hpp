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
        uint32_t order_time;

        symbol buying;
        asset selling;
        double rate;

        uint64_t matched_order_id;

        uint64_t primary_key() const { return order_id; }
        double by_rate() const { return rate; }
        EOSLIB_SERIALIZE(order, (order_id)(order_time)
            (buying)(selling)(rate)
            (matched_order_id))
    };

    #pragma region Imported_Defs

    struct [[eosio::table]] domain {
        symbol native_symbol;
        name publisher;
        uint32_t reg_time;

        bool is_blacklisted;
        
        uint64_t primary_key() const { return native_symbol.code().raw(); }
        EOSLIB_SERIALIZE(domain, (native_symbol)(publisher)(reg_time)
            (is_blacklisted))
    };

    typedef multi_index<name("domains"), domain> domains_table;

    #pragma endregion Imported_Defs
    
    typedef multi_index<name("balances"), balance> balances_table;

    typedef multi_index<name("openorders"), order, 
        indexed_by<name("byrate"), eosio::const_mem_fun<order, double, &order::by_rate>>> orders_table;

    //TODO: add partialorders table? separate by order type?

    [[eosio::action]] void mint(name recipient, asset tokens);

    [[eosio::action]] void transfer(name sender, name recipient, asset tokens);

    [[eosio::action]] void createwallet(name user);

    [[eosio::action]] void deletewallet(name user);

    [[eosio::action]] void marketorder(name orderer, symbol buy, asset sell);


    name find_domain(symbol sym);

    double find_market_rate(symbol sym);
};
