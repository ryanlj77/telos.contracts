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
#include <eosiolib/singleton.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract("telos.canopy")]] canopy : public contract {

    public:

    canopy(name self, name code, datastream<const char*> ds);

    ~canopy();

    const symbol NATIVE_SYM = symbol("DISK", 0);

    const int64_t PER_HR_RATE = 1;
    const uint32_t SEC_IN_HR = 3600;
    const uint32_t BILL_INTERVAL = 3600;

    enum provider_status : uint8_t {
        APPLIED, //status until accepted and in compliance
        ACTIVE, //node is in service
        MAINTENANCE, //node is down for scheduled maintenance
        SUSPENDED, //provider is suspended from the service (temporary)
        EXPELLED //provider is expelled from the service (permanent)
    };

    enum user_status : uint8_t {
        NORMAL, //normal state
        IN_CLEANUP //in cleaning process
    };

    enum file_extension : uint8_t {
        MD, //markdown
        TXT, //text
        MP3, //audio
        MP4 //video
    };

    //IPFS CIDv1b32 : <mb><version><mc><mh> 
    struct multihash {
        uint8_t multibase;
        uint8_t version;
        uint8_t multicodec;
        uint8_t hash_function;
        uint8_t hash_size;
        checksum256 raw_hash;
    };

    struct [[eosio::table]] user {
        name account;
        uint8_t status;
        asset tlos_balance;
        asset disk_balance; //tokens chargeable by providers
        asset per_hour_rate; //rate charged per hour for requested file storage (calculated from cumulative storage requested by this account and accepted by providers)
        uint16_t unique_files; //number of unique files
        uint32_t last_bill_time;

        uint64_t primary_key() const {return account.value; }
        EOSLIB_SERIALIZE(user, (account)(status)(tlos_balance)(disk_balance)(per_hour_rate)(unique_files)(last_bill_time))
    };

    struct [[eosio::table]] provider {
        name account;
        uint8_t status;
        string ipfs_endpoint;
        asset disk_balance;

        //TODO: uint32_t last_action_time; //for guaging activity, inactive privders could be kicked or get smaller payment?

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

    struct [[eosio::table]] file_meta {
        name file_name;
        uint8_t file_ext;
        uint16_t ipfs_chunks;
        uint32_t pin_time;

        //TODO: move billing to per-file basis instead of aggregated by account? could be stored here, since meta is only available when in harddrive

        uint64_t primary_key() const { return file_name.value; }
        EOSLIB_SERIALIZE(file_meta, (file_name)(file_ext)(ipfs_chunks)(pin_time))
    };

    struct [[eosio::table]] global_state {
        name publisher;
        uint32_t unique_users;
        uint16_t unique_providers;
        uint32_t unique_files;
        asset provider_bucket;

        //TODO: add DISK_PER_HR?

        uint64_t primary_key() const { return publisher.value; }
        EOSLIB_SERIALIZE(global_state, (publisher)(unique_users)(unique_providers)(unique_files)(provider_bucket))
    };


    //scope filerequests, harddrive, and fileremovals into same table?

    //make new struct for filerequests and fileremovals, and merge file struct with file_meta (file_with_meta?)

    typedef multi_index<name("filerequests"), file,
    indexed_by<name("bypayer"), const_mem_fun<file, uint64_t, &file::by_payer>>,
    indexed_by<name("bycid"), const_mem_fun<file, checksum256, &file::by_cid>>
    > file_requests; //TODO: different struct? could hold consensus data until pinned to harddrive?

    typedef multi_index<name("harddrive"), file,
    indexed_by<name("bypayer"), const_mem_fun<file, uint64_t, &file::by_payer>>,
    indexed_by<name("bycid"), const_mem_fun<file, checksum256, &file::by_cid>>
    > harddrive;

    typedef multi_index<name("fileremovals"), file,
    indexed_by<name("bypayer"), const_mem_fun<file, uint64_t, &file::by_payer>>,
    indexed_by<name("bycid"), const_mem_fun<file, checksum256, &file::by_cid>>
    > file_removals;

    typedef multi_index<name("metadata"), file_meta> metadata; //TODO: add vector of providers? Could divide payment among them

    typedef multi_index<name("providers"), provider> providers;

    typedef multi_index<name("users"), user> users;

    typedef singleton<name("global"), global_state> globals;
    globals global;
    global_state global_data;


    //Canopy User Actions

    [[eosio::action]]
    void buydisk(name payer, name recipient, asset tlos_amount);

    [[eosio::action]]
    void selldisk(name seller, asset disk_amount);

    [[eosio::action]]
    void addfile(name payer, name file_name, checksum256 ipfs_cid);

    [[eosio::action]]
    void rmvfile(name payer, checksum256 ipfs_cid);

    //NOTE: leaves a flag if all files aren't able to be unpinned in time.
    //NOTE: Flag allows anyone to help clean, and pays 1 DISK per poke (pay from provider bucket or user?)
    [[eosio::action]]
    void cleanhouse(name user);

    [[eosio::action]]
    void helpclean(name helper, name account_to_clean);

    [[eosio::action]]
    void withdraw(name user, asset tlos_amount);

    
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

    [[eosio::action]]
    void suspendnode();

    [[eosio::action]]
    void expellnode();


    //Functions

    void process_transfer(name to, asset amt);

    asset calc_bill(int64_t per_sec_rate, uint32_t last_bill_time);

    void process_bill(name username);

};
