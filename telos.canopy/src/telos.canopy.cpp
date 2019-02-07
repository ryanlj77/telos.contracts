/**
 * Telos Canopy Implementation
 * 
 * @author Craig Branscom
 * @copyright defined in telos/LICENSE.txt
 */

#include "../include/telos.canopy.hpp"

canopy::canopy(name self, name code, datastream<const char*> ds) : contract(self, code, ds), global(get_self(), get_self().value) {
    if (!global.exists()) {

        global_data = global_state{
            get_self(), //publisher
            0, //unique_users
            0, //unique_providers
            0, //unique_files
            asset(0, NATIVE_SYM) //provider_bucket
        }; 

        global.set(global_data, get_self());
    } else {
        global_data = global.get();
    }
}

canopy::~canopy() {
    if (global.exists()) {
        global.set(global_data, global_data.publisher);
    }
}

void canopy::buydisk(name payer, name recipient, asset tlos_amount) {
    
    require_auth(payer);
    eosio_assert(is_account(recipient), "Recipient account does not exist");
    eosio_assert(tlos_amount.symbol == symbol("TLOS", 4), "Only TLOS can be used to purchase DISK");
    eosio_assert(tlos_amount.amount > int64_t(0), "Must buy with a positive amount");

    users users(get_self(), get_self().value);
    auto u = users.find(payer.value);
    eosio_assert(u != users.end(), "Payer account has no Canopy balance");
    
    users.modify(u, same_payer, [&](auto& row) {
        row.tlos_balance -= tlos_amount;
        row.disk_balance += asset(int64_t(tlos_amount.amount), NATIVE_SYM); //NOTE: buying at 10000 DISK / TLOS for easy testing
    });
    
}

void canopy::addfile(name payer, name file_name, checksum256 ipfs_cid) {
    require_auth(payer);

    users users(get_self(), get_self().value);
    auto u = users.find(payer.value);
    eosio_assert(u != users.end(), "User has no balance to pay for storage");

    file_requests filerequests(get_self(), get_self().value);
    auto files_by_cid = filerequests.get_index<name("bycid")>();

    auto fr = files_by_cid.find(ipfs_cid);
    eosio_assert(fr == files_by_cid.end(), "File has already been requested");
    // assert file_name is unique, will fail anyway if not but allows for a custom failure message

    //harddrive harddrive(get_self(), get_self().value);
    // auto files_by_cid = harddrive.get_index<name("bycid")>();
    // auto f = files_by_cid.find(ipfs_cid);
    // eosio_assert(f == harddrive.end(), "File already exists on Hard Drive");

    filerequests.emplace(get_self(), [&](auto& row) {
        row.file_name = file_name;
        row.payer = payer;
        row.ipfs_cid = ipfs_cid;
    });

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
        row.disk_balance = asset(int64_t(0), NATIVE_SYM);
    });

}

void canopy::acceptfile(checksum256 ipfs_cid, name provider_name, name file_name, uint16_t chunks) {
    require_auth(provider_name);

    //NOTE: check ipfs_cid IS in requests table
    file_requests filerequests(get_self(), get_self().value);
    auto fr_by_cid = filerequests.get_index<name("bycid")>();
    auto fr = fr_by_cid.find(ipfs_cid);
    eosio_assert(fr != fr_by_cid.end(), "File hasn't been requested");
    name req = fr->payer;

    //TODO: check ipfs_cid is NOT in harddrive table
    harddrive harddrive(get_self(), get_self().value);
    auto files_by_cid = harddrive.get_index<name("bycid")>();
    auto f = files_by_cid.find(ipfs_cid);
    eosio_assert(f == files_by_cid.end(), "File already exists on Hard Drive");

    metadata metadata(get_self(), get_self().value);
    auto m = metadata.find(file_name.value);
    eosio_assert(m == metadata.end(), "Metadata already exists for File");

    //TODO: calc DISK bill, charge requester
    asset pin_bill = asset(int64_t(10), NATIVE_SYM);
    asset per_hour_bill = asset(int64_t(chunks), NATIVE_SYM);
    asset total_bill = (pin_bill + per_hour_bill);

    //NOTE: processes inital bill (pin bill + per hour bill)
    process_bill(req);

    //NOTE: save metadata
    metadata.emplace(provider_name, [&](auto& row) {
        row.file_name = file_name;
        row.file_ext = 0; //TODO: add real ext later
        row.ipfs_chunks = chunks;
        row.pin_time = now();
    });

    //NOTE: save file
    harddrive.emplace(provider_name, [&](auto& row) {
        row.file_name = file_name;
        row.payer = req;
        row.ipfs_cid = ipfs_cid;
    });

    //NOTE: erase file request
    filerequests.erase(*fr);

}

void canopy::maintainnode(name provider_name) {
    require_auth(provider_name);

    providers providers(get_self(), get_self().value);
    auto p = providers.get(provider_name.value, "Provider is not registered");
    eosio_assert(p.status == ACTIVE, "Producer must be active to enter maintenance");

    providers.modify(p, same_payer, [&](auto& row) {
        row.status = MAINTENANCE;
    });
}

void canopy::activatenode(name provider_name) {
    require_auth(provider_name);

    providers providers(get_self(), get_self().value);
    auto p = providers.get(provider_name.value, "Provider is not registered");
    eosio_assert(p.status == MAINTENANCE, "Provider must be in maintenance to reactivate");

    providers.modify(p, same_payer, [&](auto& row) {
        row.status = MAINTENANCE;
    });
}

void canopy::process_transfer(name to, asset amt) {
    users users(get_self(), get_self().value);
    auto u = users.find(to.value);

    if (u == users.end()) { //User not found, initialize a new user account
        users.emplace(to, [&](auto& row) {
            row.account = to;
            row.status = NORMAL;
            row.tlos_balance = amt;
            row.disk_balance = asset(0, NATIVE_SYM);
            row.per_hour_rate = asset(int64_t(0), NATIVE_SYM);
            row.unique_files = uint16_t(0);
        });
    } else { //Add to existing account
        users.modify(u, same_payer, [&](auto& row) {
            row.tlos_balance += amt;
        });
    }

}

void canopy::process_bill(name username) {
    
    users users(get_self(), get_self().value);
    auto u = users.get(username.value, "User has no balance to pay for storage");

    asset bill = calc_bill(PER_HR_RATE, u.last_bill_time);
    eosio_assert(u.disk_balance >= bill, "User has insufficient balance to cover bill");
    
    //TODO: need to unpin files for insufficient balance? create a debt? put files in separate table to process unpins?

    //charge bill to account and update last billed time
    users.modify(u, same_payer, [&](auto& row) {
        row.disk_balance -= bill;
        row.last_bill_time = now();
    });

    //put bill in provider bucket
    global_data.provider_bucket += bill;

}

asset canopy::calc_bill(int64_t rate, uint32_t last_bill_time) {
    int64_t bill = int64_t((now() - last_bill_time) / SEC_IN_HR) * rate; //TODO: gameable currently, remainder isn't billed
    return asset(bill, NATIVE_SYM);
}

extern "C" {
    void apply(uint64_t self, uint64_t code, uint64_t action) {

        size_t size = action_data_size();
        constexpr size_t max_stack_buffer_size = 512;
        void* buffer = nullptr;

        if( size > 0 ) {
            buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);
            read_action_data(buffer, size);
        }

        datastream<const char*> ds((char*)buffer, size);

        struct transfer_args {
            name from;
            name to;
            asset quantity;
            string memo;
        };

        if(code == self && action == name("buydisk").value) {
            execute_action(name(self), name(code), &canopy::buydisk);
        } else if (code == self && action == name("addfile").value) {
            execute_action(name(self), name(code), &canopy::addfile);
        } else if (code == self && action == name("regprovider").value) {
            execute_action(name(self), name(code), &canopy::regprovider);
        } else if (code == name("eosio.token").value && action == name("transfer").value) {
            canopy canopy(name(self), name(code), ds);
            auto args = unpack_action_data<transfer_args>();
            canopy.process_transfer(args.to, args.quantity);
        }
    } //end apply
}; //end dispatcher
