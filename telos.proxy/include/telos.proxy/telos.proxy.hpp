/**
 * Supports chain storage of proxies
 * 
 * @author Marlon Williams
 * @copyright defined in telos/LICENSE.txt
 */

#pragma once

#include <eosiolib/eosio.hpp>

#include <string>

using namespace std;
using namespace eosio;

class [[eosio::contract("telos.proxy")]] tlsproxyinfo : public contract {
    public:
      using contract::contract;

      [[eosio::action]]
      void set(const name proxy, std::string name, std::string slogan, std::string philosophy, std::string background, std::string website, std::string logo_256, std::string telegram, std::string steemit, std::string twitter, std::string wechat, std::string reserved_1, std::string reserved_2, std::string reserved_3);

      [[eosio::action]]
      void remove(const name proxy);

      struct [[eosio::table]] proxy {
            name owner;
            std::string name;
            std::string website;
            std::string slogan;
            std::string philosophy;
            std::string background;
            std::string logo_256;
            std::string telegram;
            std::string steemit;
            std::string twitter;
            std::string wechat;
            std::string reserved_1;
            std::string reserved_2;
            std::string reserved_3;

            auto primary_key()const { return owner.value; }
            EOSLIB_SERIALIZE(proxy, (owner)(name)(website)(slogan)(philosophy)(background)(logo_256)(telegram)(steemit)(twitter)(wechat)(reserved_1)(reserved_2)(reserved_3))
        };

        typedef multi_index<"proxies"_n, proxy> proxy_table;

        tlsproxyinfo(name self, name code, datastream<const char*> ds);

        ~tlsproxyinfo();

    protected:
        proxy_table proxies;
};