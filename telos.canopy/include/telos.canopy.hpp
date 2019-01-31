/**
 * Telos Canopy Definition
 * 
 * @author Craig Branscom
 * @copyright defined in telos/LICENSE.txt
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract("telos.canopy")]] canopy : public contract {

    public:

    canopy(name self, name code, datastream<const char*> ds);

    ~canopy();

    const symbol NATIVE_SYM = symbol("DISK", 0);

    enum provider_status : uint8_t {
        APPLIED, //status until accepted and in compliance
        ACTIVE, //node is in service
        MAINTENANCE, //node is down for scheduled maintenance
        SUSPENDED, //provider is suspended from the service (temporary)
        EXPELLED //provider is expelled from the service (permanent)
    };

    struct [[eosio::table]] provider {
        name account;
        uint8_t status;
        string ipfs_endpoint;

        uint64_t primary_key() const { return account.value; }
        EOSLIB_SERIALIZE(provider, (account)(status)(ipfs_endpoint))
    };

    struct [[eosio::table]] file {
        uint64_t ipfs_cid; //NOTE: base32 encoding by IPFS
        name payer;

        uint64_t primary_key() const { return ipfs_cid; }
        uint64_t by_payer() const { return payer.value; }
        EOSLIB_SERIALIZE(file, (ipfs_cid)(payer))
    };

    typedef multi_index<name("filerequests"), file> file_requests; //TODO: different struct? could hold consensus data until pinned to harddrive

    typedef multi_index<name("harddrive"), file> harddrive;

    typedef multi_index<name("providers"), provider> providers;


    //Canopy User Actions

    [[eosio::action]]
    void addfile(uint64_t ipfs_cid, name payer);

    [[eosio::action]]
    void rmvfile(string ipfs_cid, name payer);

    
    //Canopy Provider Actions

    [[eosio::action]]
    void regprovider(name provider_name, string endpoint);

    [[eosio::action]]
    void acceptfile(string ipfs_cid, name provider_name);

    [[eosio::action]]
    void releasefile(string ipfs_cid, name provider_name);


    //Do Later

    // struct [[eosio::table]] account {
    //     name user;
    //     asset liquid;
    //     int64_t quota;
    //     int64_t quota_used;
    //     int16_t unique_files;
    //     uint64_t primary_key() const { return user.value; }
    //     //EOSLIB_SERIALIZE(account, )
    // };
    // int64_t size_in_bytes;
    // int32_t chunks; //NOTE: number of whole 256KiB chunks that make up the file
    // uint8_t file_type; //TODO: maybe change to string?
    // typedef multi_index<name("accounts"), account> accounts_table;
    // write enum for file types?
    //TODO: scope by get_self() and secondary index by name?
    //TODO: implement multibase, multicodec, multihash to transform CIDv1 to a uint64_t
    //NOTE: C++ impl: https://github.com/cpp-ipfs/cpp-multihash
    //NOTE: currently supports the IPFS CIDv1 standard

};
