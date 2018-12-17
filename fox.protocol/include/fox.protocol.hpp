/**
 * Fox Protocol Definition
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

class [[eosio::contract("fox.protocol")]] fox : public contract {

public:

    fox(name self, name code, datastream<const char*> ds);

    ~fox();

    struct [[eosio::table]] domain {
        symbol native_symbol;
        name publisher;
        uint32_t reg_time;

        bool is_blacklisted;
        //TODO: add blacklist metrics
        
        uint64_t primary_key() const { return native_symbol.code().raw(); }
        EOSLIB_SERIALIZE(domain, (native_symbol)(publisher)(reg_time)
            (is_blacklisted))
    };

    // struct [[eosio::table]] config {
    //     name host_name;
    //     uint16_t domain_count;
    //     uint16_t blacklisted_count;
    //     uint64_t primary_key() const { return host_name.value; }
    //     EOSLIB_SERIALIZE(config, (host_name)(domain_count)(blacklisted_count))
    // };
    
    typedef multi_index<name("domains"), domain> domains_table;

    //typedef singleton<name("config"), config> config_table;
	//config_table configs;

    [[eosio::action]] void regdomain(name publisher, symbol native_symbol);

    [[eosio::action]] void unregdomain(name publisher, symbol native_symbol);

    [[eosio::action]] void blacklist(name host, symbol sym_to_blacklist);

    //[[eosio::action]] void setconfig(name publisher, config new_config);

};
