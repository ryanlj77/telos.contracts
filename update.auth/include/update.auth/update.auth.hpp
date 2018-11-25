#pragma once

#include <eosiolib/eosio.hpp>
// #include <eosiolib/ignore.hpp>
// #include <eosiolib/transaction.hpp>
using namespace eosio;

class [[eosio::contract("update.auth")]] uauth : public contract {
    public:
        using contract::contract;
        // uauth(name self, name code, datastream<const char*> ds);

     [[eosio::action]]
     void hello(name executer);
     
     [[eosio::action]]
     void hello1(name executer);

     [[eosio::action]]
     void hello2(name executer);

     [[eosio::action]]
     void hello3(name executer);

     [[eosio::action]]
     void hello4(name executer);

     [[eosio::action]]
     void hello5(name executer);

     [[eosio::action]]
     void update(name account);
};