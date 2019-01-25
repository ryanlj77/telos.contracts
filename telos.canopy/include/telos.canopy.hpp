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
#include <eosiolib/singleton.hpp>
#include <eosiolib/transaction.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract("telos.canopy")]] canopy : public contract {

public:

    canopy(name self, name code, datastream<const char*> ds);

    ~canopy();

    enum file_extension : uint8_t {
        MD //0
    };

    struct link {
        name owner;
        char* eospath;
        char* ipfspath;
        uint32_t file_size;
        name storage_provider;
        uint8_t store_status; //a flag that signals producers to cache file
        uint8_t accept_status; //a flag that signals producers have accepted storage and cached file
        asset stake; //tokens staked per file
    };

    /**
     * 
     * 
     * @scope 
     * @key 
     */
    struct [[eosio::table]] account {
        name user;
        asset liquid;
        int64_t quota;
        int64_t quota_used;
        int16_t unique_files;

        uint64_t primary_key() const { return user.value; }
        //EOSLIB_SERIALIZE(account, )
    };

    /**
     * Represents a file in the IPFS Cluster identified by a unique IPFS CID.
     * 
     * @scope name.value
     * @key 
     */
    //TODO: scope by get_self() and secondary index by name?
    //TODO: implement multibase, multicodec, multihash to transform CIDv1 to a uint64_t
    //NOTE: C++ impl: https://github.com/cpp-ipfs/cpp-multihash
    //NOTE: currently supports the IPFS CIDv1 standard
    struct [[eosio::table]] disk_row {
        uint64_t ipfs_cid;
        name payer; //TODO: rename to owner?

        int64_t size_in_bytes;
        int32_t chunks; //NOTE: number of whole 256KiB chunks that make up the file
        uint8_t file_type; //TODO: maybe change to string?
        
        uint64_t primary_key() const { return ipfs_cid; }
        //TODO: make secondary index by name? instead of scoping by name
        //EOSLIB_SERIALIZE(disk_row, )
    };
    
    typedef multi_index<name("harddisk"), disk_row> nominees_table;

    typedef multi_index<name("accounts"), account> accounts_table;

    //Canopy User Actions

    [[eosio::action]]
    void addfile(string ipfs_cid, name payer);

    [[eosio::action]]
    void rmvfile(string ipfs_cid, name payer);

    [[eosio::action]]
    void buydisk(name buyer, name recipient, asset amount); //TODO: change asset amount to int64_t bytes/chunks?

    [[eosio::action]]
    void selldisk(name owner, asset amount); //TODO: change asset amount to int64_t bytes/chunks?

    

    //Canopy Node Operator Actions

    [[eosio::action]]
    void acceptfile(string ipfs_cid, name node_operator);

    [[eosio::action]]
    void rejectfile(string ipfs_cid, name node_operator);



};
