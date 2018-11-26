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
     void setperm();

     struct permission_level_weight {
       permission_level permission;
       uint16_t weight;

       // explicit serialization macro is not necessary, used here only to improve compilation time
       EOSLIB_SERIALIZE(permission_level_weight, (permission)(weight))
     };

     struct key_weight {
       eosio::public_key key;
       uint16_t weight;

       // explicit serialization macro is not necessary, used here only to improve compilation time
       EOSLIB_SERIALIZE(key_weight, (key)(weight))
     };

     struct wait_weight {
       uint32_t wait_sec;
       uint16_t weight;

       // explicit serialization macro is not necessary, used here only to improve compilation time
       EOSLIB_SERIALIZE(wait_weight, (wait_sec)(weight))
     };

     struct authority {
       uint32_t threshold = 0;
       std::vector<key_weight> keys;
       std::vector<permission_level_weight> accounts;
       std::vector<wait_weight> waits;

       // explicit serialization macro is not necessary, used here only to improve compilation time
       EOSLIB_SERIALIZE(authority, (threshold)(keys)(accounts)(waits))
     };
};
