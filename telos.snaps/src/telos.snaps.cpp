/**
 *  @author Jesse Schulman
 *  @copyright defined in telos/LICENSE.txt
 */

#include "../include/telos.snaps/telos.snaps.hpp"

void snapshots::define( uint64_t      snapshot_id,
                        name owner,
                        uint64_t snapshot_block,
                        uint64_t expire_block )
{
    eosio_assert( is_account( owner ), "Owner must be an account that exists" );
    require_auth( _self );
    auto iter = _snapshot_meta.find( snapshot_id );
    eosio_assert( iter == _snapshot_meta.end(), "There is already a snapshot with the specified id." );
    _snapshot_meta.emplace( _self, [&](auto &s) {
        s.snapshot_id = snapshot_id;
        s.owner = owner;
        s.snapshot_block = snapshot_block;
        s.expire_block = expire_block;
        s.max_supply = 0;
    });
}


void snapshots::setbalance( uint64_t snapshot_id,
                     name account,
                     uint64_t amount )
{
    auto meta_iter = _snapshot_meta.find( snapshot_id );
    eosio_assert( meta_iter != _snapshot_meta.end(), "There is no snapshot with the specified id." );
    eosio_assert( is_account(account), "Account does not exist" );

    snapshot thisSnap = *meta_iter;
    require_auth( thisSnap.owner );

    SnapBalances snap_balances( _self, snapshot_id );
    auto iter = snap_balances.find( account.value );
    uint64_t old_amount = 0;
    if ( iter == snap_balances.end() ) {
        snap_balances.emplace( _self, [&](auto &b) {
            b.account = account;
            b.amount = amount;
        });
    } else {
        balance old_balance = *iter;
        old_amount = old_balance.amount;
        snap_balances.modify( iter, _self, [&](auto &b) {
            b.amount = amount;
        });
    }

    _snapshot_meta.modify( meta_iter, _self, [&](auto &s) {
        s.last_updated = now();
        s.max_supply = (s.max_supply - old_amount) + amount;
    });
}

/*
void snapshots::clear( uint64_t snapshot_id )
{
    auto ite = _snapshot_meta.begin();
    while(ite != _snapshot_meta.end()) {
        ite = _snapshot_meta.erase(ite);
    }
}
*/

void snapshots::remove( uint64_t snapshot_id, uint32_t count )
{
    auto meta_iter = _snapshot_meta.find( snapshot_id );
    eosio_assert( meta_iter != _snapshot_meta.end(), "There is no snapshot with the specified id." );

    snapshot thisSnap = *meta_iter;
    require_auth( thisSnap.owner ); 

    uint32_t deleted = 0;
    SnapBalances snap_balances( _self, snapshot_id );
    auto ite = snap_balances.begin();
    while(ite != snap_balances.end()) {
        if (++deleted > count)
            break;

        ite = snap_balances.erase(ite);
    }
}

EOSIO_DISPATCH( snapshots, (define)(setbalance)(remove) )
