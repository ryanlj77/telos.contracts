/**
 *  @author Jesse Schulman
 *  @copyright defined in telos/LICENSE.txt
 */

#include "../include/telos.snaps/telos.snaps.hpp"

void snapshots::define( uint64_t      snapshot_id,
                        uint64_t snapshot_block,
                        uint64_t expire_block )
{
    require_auth( _self );
    auto iter = _snapshot_meta.find( snapshot_id );
    eosio_assert(iter == _snapshot_meta.end(), "There is already a snapshot with the specified id." );
    _snapshot_meta.emplace( _self, [&](auto &s) {
        s.snapshot_id = snapshot_id;
        s.snapshot_block = snapshot_block;
        s.expire_block = expire_block;
        s.max_supply = 0;
    });
}


void snapshots::setbalance( uint64_t snapshot_id,
                     name account,
                     uint64_t amount )
{

    // TODO: should we allow anyone to create and own a snapshot, if so this should track an owner and not the contract itself
    require_auth( _self );

    // TODO: check that account exists?
    auto meta_iter = _snapshot_meta.find( snapshot_id );
    eosio_assert( meta_iter != _snapshot_meta.end(), "There is no snapshot with the specified id." );

    SnapBalances snap_balances( _self, snapshot_id );
    auto iter = snap_balances.find( account.value );
    if ( iter == snap_balances.end() ) {
        snap_balances.emplace( _self, [&](auto &b) {
            b.account = account;
            b.amount = amount;
        });
    } else {
        snap_balances.modify( iter, _self, [&](auto &b) {
            b.amount = amount;
        });
    }

    _snapshot_meta.modify( meta_iter, _self, [&](auto &s) {
        s.last_updated = now();
        s.max_supply += amount;
    });
}

void snapshots::remove( uint64_t snapshot )
{
    // TODO: figure out how many rows can be deleted in a single action
    // also decide if meta should stick around after expiration
}

EOSIO_DISPATCH( snapshots, (define)(setbalance)(remove) )
