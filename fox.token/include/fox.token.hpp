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


    
    //typedef multi_index<name("balances"), balance> balances_table;

};
