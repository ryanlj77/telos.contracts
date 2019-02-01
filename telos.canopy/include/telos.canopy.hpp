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

    const uint32_t DAY_IN_SEC = 86400;

    enum provider_status : uint8_t {
        APPLIED, //status until accepted and in compliance
        ACTIVE, //node is in service
        MAINTENANCE, //node is down for scheduled maintenance
        SUSPENDED, //provider is suspended from the service (temporary)
        EXPELLED //provider is expelled from the service (permanent)
    };

    enum file_extension : uint8_t {
        MD, //markdown
        TXT, //text
        MP3,
        MP4
    };

    struct [[eosio::table]] user {
        name account;
        asset tlos_balance;
        asset disk_balance; //tokens chargeable by providers
        asset per_diem; //rate charged per day for requested file storage
        uint16_t unique_files; //number of unique files

        uint64_t primary_key() const {return account.value; }
        EOSLIB_SERIALIZE(user, (account)(tlos_balance)(disk_balance)(per_diem)(unique_files))
    };

    struct [[eosio::table]] provider {
        name account;
        uint8_t status;
        string ipfs_endpoint;
        asset disk_balance;

        uint64_t primary_key() const { return account.value; }
        EOSLIB_SERIALIZE(provider, (account)(status)(ipfs_endpoint)(disk_balance))
    };

    struct [[eosio::table]] file {
        name file_name;
        name payer; //TODO: rename bill_to?
        checksum256 ipfs_cid; //NOTE: base32 encoding by IPFS hashed with sha256

        uint64_t primary_key() const { return file_name.value; }
        uint64_t by_payer() const { return payer.value; }
        checksum256 by_cid() const { return ipfs_cid; }
        EOSLIB_SERIALIZE(file, (file_name)(payer)(ipfs_cid))
    };

    struct [[eosio::action]] file_meta {
        name file_name;
        uint8_t file_ext;
        uint16_t ipfs_chunks;
        uint32_t add_time;
        uint32_t last_charge_time;

        uint64_t primary_key() const { return file_name.value; }
        EOSLIB_SERIALIZE(file_meta, (file_name)(file_ext)(ipfs_chunks)(add_time)(last_charge_time))
    };

    typedef multi_index<name("filerequests"), file,
    indexed_by<name("bypayer"), const_mem_fun<file, uint64_t, &file::by_payer>>,
    indexed_by<name("bycid"), const_mem_fun<file, checksum256, &file::by_cid>>
    > file_requests; //TODO: different struct? could hold consensus data until pinned to harddrive?

    typedef multi_index<name("harddrive"), file,
    indexed_by<name("bypayer"), const_mem_fun<file, uint64_t, &file::by_payer>>,
    indexed_by<name("bycid"), const_mem_fun<file, checksum256, &file::by_cid>>
    > harddrive;

    typedef multi_index<name("providers"), provider> providers;

    typedef multi_index<name("users"), user> users;

    typedef multi_index<name("metadata"), file_meta> metadata;


    //Canopy User Actions

    [[eosio::action]]
    void buydisk(name payer, name recipient, asset tlos_amount);

    [[eosio::action]]
    void selldisk(name seller, asset disk_amount);

    [[eosio::action]]
    void addfile(name payer, name file_name, checksum256 ipfs_cid);

    [[eosio::action]]
    void rmvfile(name payer, checksum256 ipfs_cid);

    
    //Canopy Provider Actions

    [[eosio::action]]
    void regprovider(name provider_name, string endpoint);

    [[eosio::action]]
    void acceptfile(checksum256 ipfs_cid, name provider_name, name file_name, uint16_t chunks);

    [[eosio::action]]
    void releasefile(checksum256 ipfs_cid, name provider_name, name file_name);

    [[eosio::action]]
    void maintainnode(name provider_name);

    [[eosio::action]]
    void activatenode(name provider_name);

    //Functions

    void process_transfer(name to, asset amt);

    void process_bill(name from, name to, asset bill);

};
