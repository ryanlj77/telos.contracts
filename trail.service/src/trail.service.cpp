#include "<trail.service/trail.service.hpp">

trail::trail(account_name self) : contract(self), env_singleton(self, self) {
    if (!env_singleton.exists()) {

        env_struct = environment{
            self, //publisher
            0, //total_tokens
            0, //total_voters
            0 //total_ballots
        };

        env_singleton.set(env_struct, self);
    } else {
        env_struct = env_singleton.get();
    }
}

trail::~trail() {
    if (env_singleton.exists()) {
        env_singleton.set(env_struct, env_struct.publisher);
    }
}

void trail::regtoken(asset native, account_name publisher) {
    require_auth(publisher);

    auto sym = native.symbol.name();
    registries_table registries(_self, sym);
    auto r = registries.find(sym);

    eosio_assert(r == registries.end(), "Token Registry with that symbol already exists.");

    registries.emplace(publisher, [&]( auto& a ){
        a.native = native;
        a.publisher = publisher;
    });

    env_struct.total_tokens++;

    print("\nToken Registration: SUCCESS");
}

void trail::unregtoken(asset native, account_name publisher) {
    require_auth(publisher);
    
    auto sym = native.symbol.name();
    registries_table registries(_self, sym);
    auto r = registries.find(sym);

    eosio_assert(r != registries.end(), "Token Registry does not exist.");

    registries.erase(r);

    env_struct.total_tokens--;

    print("\nToken Unregistration: SUCCESS");
}

void trail::regvoter(account_name voter) {
    require_auth(voter);

    voters_table voters(_self, voter);
    auto v = voters.find(voter);

    eosio_assert(v == voters.end(), "Voter already exists");

    voters.emplace(voter, [&]( auto& a ){
        a.voter = voter;
    });

    env_struct.total_voters++;

    print("\nVoterID Registration: SUCCESS");
}

void trail::unregvoter(account_name voter) {
    require_auth(voter);

    voters_table voters(_self, voter);
    auto v = voters.find(voter);

    eosio_assert(v != voters.end(), "Voter Doesn't Exist");

    auto vid = *v;

    eosio_assert(vid.receipt_list.size() == 0, "Can't delete until all vote receipts removed");    

    voters.erase(v);

    env_struct.total_voters--;

    print("\nVoterID Unregistration: SUCCESS");
}

void trail::addreceipt(uint64_t vote_code, uint64_t vote_scope, uint64_t vote_key, uint16_t direction, uint32_t expiration, account_name voter) {
    require_auth(voter);

    voters_table voters(_self, voter);
    auto v = voters.find(voter);

    eosio_assert(v != voters.end(), "Voter doesn't exist");

    auto vid = *v;
    int64_t new_weight = get_liquid_tlos(voter);

    votereceipt new_vr = votereceipt{
        vote_code,
        vote_scope,
        vote_key,
        direction,
        new_weight,
        expiration
    };

    auto itr = vid.receipt_list.begin();

    //search for existing receipt
    for (votereceipt r : vid.receipt_list) {
        if (r.vote_code == vote_code && r.vote_scope == vote_scope && r.vote_key == vote_key) {
            print("\nExisting receipt found. Updating...");

            auto h = vid.receipt_list.erase(itr);
            auto i = vid.receipt_list.insert(itr, new_vr);

            break;
        }
        itr++;
    }

    //if receipt not found in list
    if (itr == vid.receipt_list.end()) {
        vid.receipt_list.push_back(new_vr);
    } 

    voters.modify(v, 0, [&]( auto& a ) {
        a.receipt_list = vid.receipt_list;
    });

    print("\nVoteReceipt Addition: SUCCESS");
}

void trail::rmvexpvotes(account_name voter) {
    require_auth(voter);

    voters_table voters(_self, voter);
    auto v = voters.find(voter);

    eosio_assert(v != voters.end(), "Voter doesn't exist");
    auto vid = *v;

    uint32_t now_time = now(); //better name than now_time?

    auto itr = vid.receipt_list.begin();

    uint64_t votes_removed = 0;

    for (votereceipt r : vid.receipt_list) {
        if (now_time > r.expiration) {
            vid.receipt_list.erase(itr);
            votes_removed++;
        }
        itr++;
    }

    voters.modify(v, 0, [&]( auto& a ) {
        a.receipt_list = vid.receipt_list;
    });

    print("\nVotes Removed: ", votes_removed);
}

/*
void trail::rmvreceipt(uint64_t vote_code, uint64_t vote_scope, uint64_t vote_key, account_name voter) {
    require_auth(voter);

    voters_table voters(_self, voter);
    auto v = voters.find(voter);

    eosio_assert(v != voters.end(), "Voter doesn't exist");

    auto vid = *v;

    auto itr = vid.receipt_list.begin();
    bool found = false;

    //search for existing receipt
    for (votereceipt r : vid.receipt_list) {

        if (r.vote_code == vote_code && r.vote_scope == vote_scope && r.vote_key == vote_key) {
            print("\nExisting receipt found. Removing...");
            found = true;

            auto vr = vid.receipt_list.erase(itr);

            break;
        }

        itr++;
    }

    eosio_assert(found, "Receipt not found");

    voters.modify(v, 0, [&]( auto& a ) {
        a.receipt_list = vid.receipt_list;
    });

    print("\nVoteReceipt Removal: SUCCESS");
}
*/

void trail::regballot(account_name publisher) {
    require_auth(publisher);

    ballots_table ballots(_self, publisher);
    auto b = ballots.find(publisher);

    eosio_assert(b == ballots.end(), "Ballot already exists");

    ballots.emplace(publisher, [&]( auto& a ){
        a.publisher = publisher;
    });

    env_struct.total_ballots++;

    print("\nBallot Registration: SUCCESS");
}

void trail::unregballot(account_name publisher) {
    require_auth(publisher);

    ballots_table ballots(_self, publisher);
    auto b = ballots.find(publisher);

    eosio_assert(b != ballots.end(), "Ballot Doesn't Exist");

    ballots.erase(b);

    env_struct.total_ballots--;

    print("\nBallot Unregistration: SUCCESS");
}

/*
extern "C" {
    void apply(uint64_t self, uint64_t code, uint64_t action) {
        trail _trail(self);

        //has_auth(N(accountname)) //?
        //ballots_table ballots(self, code); //check that code is from a ballot

        if (code == self && action == N(regtoken)) {
            execute_action(&_trail, &trail::regtoken);
        } else if (code == self && action == N(unregtoken)) {
            execute_action(&_trail, &trail::unregtoken);
        } else if (code == self && action == N(regvoter)) {
            execute_action( &_trail, &trail::regvoter);
        } else if (code == self && action == N(unregvoter)) {
            execute_action(&_trail, &trail::unregvoter);
        } else if (code == self && action == N(addreceipt)) { //add code condition
            //print("\nreceiver: ", name{self});
            //print("\ncode: ", name{code});
            //print("\naction: ", name{action});
            execute_action(&_trail, &trail::addreceipt);
        } else if (code == self && action == N(regballot)) {
            execute_action(&_trail, &trail::regballot);
        } else if (code == self && action == N(unregballot)) {
            execute_action(&_trail, &trail::unregballot);
        }
    }
};
*/

/*
extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        switch(action) {
            case N(trailvote): return addreceipt(receiver, code);
        }
    }
}
*/

/*
extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        auto self = receiver;
        if(code == self){
            trail _trail(self);
            switch(action){
      	        case N(regtoken): execute_action(&_trail, &trail::regtoken);
                case N(unregtoken): execute_action(&_trail, &trail::unregtoken);
                case N(regvoter): execute_action(&_trail, &trail::regvoter);
                case N(unregvoter): execute_action(&_trail, &trail::unregvoter);
                case N(addreceipt): execute_action(&_trail, &trail::addreceipt);
                case N(rmvreceipt): execute_action(&_trail, &trail::rmvreceipt);
                case N(regballot): execute_action(&_trail, &trail::regballot);
                case N(unregballot): execute_action(&_trail, &trail::unregballot);
            }
        }
    }
};
*/

EOSIO_ABI(trail, (regtoken)(unregtoken)(regvoter)(unregvoter)(addreceipt)(rmvexpvotes))