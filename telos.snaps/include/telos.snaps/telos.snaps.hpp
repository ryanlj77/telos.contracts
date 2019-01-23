/**
 *  @file
 *  @copyright defined in telos/LICENSE.txt
 */

#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>

using namespace eosio;

class [[eosio::contract("telos.snaps")]] snapshots : public contract {
  public:
      using contract::contract;
    snapshots( name receiver, name code, datastream<const char*> ds )
        : contract(receiver, code, ds), 
     _snapshot_meta(receiver, receiver.value)
     {

     }

     ACTION define( uint64_t snapshot_id,
                  name owner,
                  uint64_t snapshot_block,
                  uint64_t expire_block );

     ACTION setbalance( uint64_t snapshot_id,
                      name account,
                      uint64_t amount );

     ACTION remove( uint64_t snapshot_id, uint32_t count );
     //ACTION clear( uint64_t snapshot_id );

  private:
     TABLE balance {
        name account;
        uint64_t amount;

        uint64_t primary_key()const { return account.value; }
     };

     TABLE snapshot {
         uint64_t snapshot_id;
         name owner;
         uint64_t snapshot_block;
         uint64_t expire_block;
         uint32_t last_updated;
         uint64_t max_supply;

         uint64_t primary_key()const { return snapshot_id; }
     };

     typedef eosio::multi_index<name("snapmeta"), snapshot> SnapMeta;
     SnapMeta _snapshot_meta;

     typedef eosio::multi_index<name("snapshots"), balance> SnapBalances;

};
