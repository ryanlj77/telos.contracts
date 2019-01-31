/**
 * Telos Canopy Implementation
 * 
 * @author Craig Branscom
 * @copyright defined in telos/LICENSE.txt
 */

#include "../include/telos.canopy.hpp"

canopy::canopy(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

canopy::~canopy() {}

void canopy::addfile(uint64_t ipfs_cid, name payer) {
    require_auth(payer);

    harddrive harddrive(get_self(), get_self().value);
    auto f = harddrive.find(ipfs_cid);
    eosio_assert(f == harddrive.end(), "File already exists on Hard Drive");

    //TODO: assert not in filerequests table



}

void canopy::regprovider(name provider_name, string endpoint) {
    require_auth(provider_name);

    providers providers(get_self(), get_self().value);
    auto p_itr = providers.find(provider_name.value);
    eosio_assert(p_itr == providers.end(), "Provider is already registered");

    providers.emplace(get_self(), [&](auto& row) {
       row.account = provider_name;
       row.status = APPLIED;
       row.ipfs_endpoint = endpoint;
    });

}
