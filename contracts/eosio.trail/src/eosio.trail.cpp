#include "../include/eosio.trail.hpp"

trail::trail(name self, name code, datastream<const char*> ds) : contract(self, code, ds), environment(self, self.value) {

    //TODO: check globals, assert trail is live

    if (!environment.exists()) {

        vector<uint64_t> new_totals = {0,0,0};

        env_struct = env{
            self, //publisher
            new_totals, //totals
            now(), //time_now
            0 //last_ballot_id
        };

        environment.set(env_struct, self);
    } else {
        env_struct = environment.get();
        env_struct.time_now = now();
    }
}

trail::~trail() {
    if (environment.exists()) {
        environment.set(env_struct, env_struct.publisher);
    }
}

#pragma region Tokens

void trail::regtoken(asset max_supply, name publisher, string info_url) {
    require_auth(publisher);

    auto sym = max_supply.symbol;

    //cehck registry doesn't already exist
    registries_table registries(get_self(), get_self().value);
    auto r = registries.find(sym.code().raw());
    eosio_assert(r == registries.end(), "token registry already exists");

    token_settings default_settings;

    //emplace new registry
    registries.emplace(publisher, [&]( auto& a ){
        a.max_supply = max_supply;
        a.supply = asset(0, sym);
        a.total_voters = uint32_t(0);
        a.total_proxies = uint32_t(0);
        a.publisher = publisher;
        a.info_url = info_url;
        a.settings = default_settings;
    });

}

void trail::initsettings(name publisher, symbol token_symbol, token_settings new_settings) {
    require_auth(publisher);

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(token_symbol.code().raw(), "registry doesn't exist for that token");
    eosio_assert(reg.publisher == publisher, "cannot change settings of another account's registry");

    //assert not already initialized if lock is true
    if (reg.settings.is_initialized) {
        eosio_assert(!reg.settings.lock_after_initialize, "settings have been locked");
    } else {
        new_settings.is_initialized = true;
    }

    //update registry settings
    registries.modify(reg, same_payer, [&](auto& row) {
        row.settings = new_settings;
    });

}

void trail::unregtoken(symbol token_symbol, name publisher) {
    require_auth(publisher);
    
    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(token_symbol.code().raw(), "registry doesn't exist for that token");
    eosio_assert(reg.settings.is_destructible, "token registry has been set as indestructible");
    eosio_assert(reg.publisher == publisher, "only publisher can call unregtoken");

    registries.erase(reg);
}

void trail::issuetoken(name publisher, name recipient, asset tokens, bool airgrab) {
    require_auth(publisher);
    eosio_assert(tokens > asset(0, tokens.symbol), "must issue more than 0 tokens");

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(tokens.symbol.code().raw(), "registry doesn't exist for that token");
    eosio_assert(reg.publisher == publisher, "only publisher can issue tokens");

    asset new_supply = (reg.supply + tokens);
    eosio_assert(new_supply <= reg.max_supply, "Issuing tokens would breach max supply");

    //update supply
    registries.modify(reg, same_payer, [&](auto& row) {
        row.supply = new_supply;
    });

    //place in airgrabs table to be claimed by recipient
    if (airgrab) {
        airgrabs_table airgrabs(get_self(), publisher.value);
        auto g = airgrabs.find(recipient.value);

        if (g == airgrabs.end()) { //new airgrab
            airgrabs.emplace(publisher, [&](auto& row){
                row.recipient = recipient;
                row.tokens = tokens;
            });
        } else { //add to existing airgrab
            airgrabs.modify(g, same_payer, [&](auto& row) {
                row.tokens += tokens;
            });
        }
    } else { //publisher pays RAM cost if recipient has no balance entry (airdrop)
        balances_table balances(get_self(), tokens.symbol.code().raw());
        auto b = balances.find(recipient.value);

        if (b == balances.end()) { //new balance
            balances.emplace(publisher, [&]( auto& row){
                row.owner = recipient;
                row.tokens = tokens;
            });
        } else { //add to existing balance
            balances.modify(b, same_payer, [&]( auto& row) {
                row.tokens += tokens;
            });
        }
    }

}

void trail::claimairgrab(name claimant, name publisher, symbol token_symbol) {
    require_auth(claimant);

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(token_symbol.code().raw(), "token registry with that symbol doesn't exist");

    //check airgrab exists
    airgrabs_table airgrabs(get_self(), reg.publisher.value);
    auto grab = airgrabs.get(claimant.value, "no airgrab to claim");
    eosio_assert(grab.recipient == claimant, "cannot claim another account's airdrop");

    balances_table balances(get_self(), token_symbol.code().raw());
    auto b = balances.find(claimant.value);

    if (b == balances.end()) { //create a wallet, RAM paid by claimant
        balances.emplace(claimant, [&]( auto& a ){
            a.owner = claimant;
            a.tokens = grab.tokens;
        });
    } else { //add to existing balance
        balances.modify(b, same_payer, [&]( auto& a ) {
            a.tokens += grab.tokens;
        });
    }

    airgrabs.erase(grab);

    //print("\nAirgrab Claim: SUCCESS");
}

void trail::burntoken(name balance_owner, asset amount) {
    //TODO: change to require_auth(name("eosio.trail"));
    require_auth(balance_owner);
    eosio_assert(amount > asset(0, amount.symbol), "must claim more than 0 tokens");

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(amount.symbol.code().raw(), "registry doesn't exist for given token");
    eosio_assert(reg.settings.is_burnable, "token registry doesn't allow burning");

    //check balance exists
    balances_table balances(get_self(), amount.symbol.code().raw());
    auto bal = balances.get(balance_owner.value, "balance owner has no balance to burn");

    asset new_supply = (reg.supply - amount);
    asset new_balance = bal.tokens - amount;
    eosio_assert(new_balance >= asset(0, bal.tokens.symbol), "cannot burn more tokens than are owned");

    //update registry supply
    registries.modify(reg, same_payer, [&](auto& row) {
        row.supply = new_supply;
    });

    //subtract amount from balance
    balances.modify(bal, same_payer, [&](auto& row) {
        row.tokens -= amount;
    });
    
    //print("\nToken Burn: SUCCESS");
}

void trail::seizetoken(name publisher, name owner, asset tokens) {
    require_auth(publisher);
    eosio_assert(publisher != owner, "cannot seize your own tokens");
    eosio_assert(tokens > asset(0, tokens.symbol), "must seize greater than 0 tokens");

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(tokens.symbol.code().raw(), "registry doesn't exist for given token");
    eosio_assert(reg.publisher == publisher, "only publisher can seize tokens");
    eosio_assert(reg.settings.is_seizable, "token registry doesn't allow seizing");

    //check owner balance exists
    balances_table ownerbals(get_self(), tokens.symbol.code().raw());
    auto obal = ownerbals.get(owner.value, "user has no balance to seize");
    eosio_assert(obal.tokens - tokens >= asset(0, obal.tokens.symbol), "cannot seize more tokens than user owns");

    //subtract amount from balance
    ownerbals.modify(obal, same_payer, [&]( auto& row) {
        row.tokens -= tokens;
    });

    //check publisher balance exists
    balances_table publisherbal(get_self(), tokens.symbol.code().raw());
    auto pbal = publisherbal.get(publisher.value, "publisher has no balance to hold seized tokens");

    //add seized tokens to publisher balance
    publisherbal.modify(pbal, same_payer, [&]( auto& a ) {
        a.tokens += tokens;
    });

    //print("\nToken Seizure: SUCCESS");
}

void trail::seizeairgrab(name publisher, name recipient, asset amount) {
    require_auth(publisher);
    eosio_assert(amount > asset(0, amount.symbol), "must seize greater than 0 tokens");

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(amount.symbol.code().raw(), "registry doesn't exist for given token");

    //authorize
    eosio_assert(reg.publisher == publisher, "only publisher can seize airgrabs");
    eosio_assert(reg.settings.is_seizable, "token registry doesn't allow seizing");

    //check airgrab exists
    airgrabs_table airgrabs(get_self(), publisher.value);
    auto grab = airgrabs.get(recipient.value, "recipient has no airgrab");
    eosio_assert(grab.tokens - amount >= asset(0, grab.tokens.symbol), "cannot seize more tokens than airgrab holds");

    //remove or erase airgrab
    if (amount - grab.tokens == asset(0, grab.tokens.symbol)) { //all tokens seized :(
        airgrabs.erase(grab);
    } else { //airgrab still has balance after seizure
        airgrabs.modify(grab, same_payer, [&](auto& row) {
            row.tokens -= amount;
        });
    }

    balances_table publisherbal(get_self(), amount.symbol.code().raw());
    auto pbal = publisherbal.get(publisher.value, "publisher has no balance to hold seized tokens");

    //update publisher balance
    publisherbal.modify(pbal, same_payer, [&](auto& row) { //NOTE: add seized tokens to publisher balance
        row.tokens += amount;
    });

    //print("\nAirgrab Seizure: SUCCESS");
}

void trail::seizebygroup(name publisher, vector<name> group, asset tokens) {
    require_auth(publisher);
    //eosio_assert(publisher != owner, "cannot seize your own tokens");
    eosio_assert(tokens > asset(0, tokens.symbol), "must seize greater than 0 tokens");

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(tokens.symbol.code().raw(), "registry doesn't exist for given token");

    //authorize
    eosio_assert(reg.publisher == publisher, "only publisher can seize tokens");
    eosio_assert(reg.settings.is_seizable, "token registry doesn't allow seizing");

    //seize tokens from each member in group
    for (name n : group) {
        balances_table ownerbals(get_self(), tokens.symbol.code().raw());
        auto obal = ownerbals.get(n.value, "user has no balance to seize");
        eosio_assert(obal.tokens - tokens >= asset(0, obal.tokens.symbol), "cannot seize more tokens than user owns");

        //subtract amount from balance
        ownerbals.modify(obal, same_payer, [&](auto& row) {
            row.tokens -= tokens;
        });

        //TODO: instantiate outside loop, add at end
        balances_table publisherbal(get_self(), tokens.symbol.code().raw());
        auto pbal = publisherbal.get(publisher.value, "publisher has no balance to hold seized tokens");

        //add seized tokens to publisher balance
        publisherbal.modify(pbal, same_payer, [&](auto& row) {
            row.tokens += tokens;
        });
    }

    //print("\nToken Seizure: SUCCESS");
}

void trail::raisemax(name publisher, asset amount) {
    require_auth(publisher);
    eosio_assert(amount > asset(0, amount.symbol), "amount must be greater than 0");
    //TODO: check amount isn't invalid

    //check registry exists
    registries_table registries(_self, _self.value);
    auto reg = registries.get(amount.symbol.code().raw(), "registry doesn't exist for given token");

    //authorize
    eosio_assert(reg.publisher == publisher, "cannot raise another registry's max supply");
    eosio_assert(reg.settings.is_max_mutable, "token registry doesn't allow raising max supply");
    //TODO: check max isn't being raised to invalid amount

    //update max_supply
    registries.modify(reg, same_payer, [&](auto& row) {
        row.max_supply += amount;
    });

    //print("\nRaise Max Supply: SUCCESS");
}

void trail::lowermax(name publisher, asset amount) {
    require_auth(publisher);
    eosio_assert(amount > asset(0, amount.symbol), "amount must be greater than 0");

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(amount.symbol.code().raw(), "registry doesn't exist for that token");

    //authorize
    eosio_assert(reg.publisher == publisher, "cannot lower another account's max supply");
    eosio_assert(reg.settings.is_max_mutable, "token settings don't allow lowering max supply");
    eosio_assert(reg.supply <= reg.max_supply - amount, "cannot lower max supply below circulating supply");
    eosio_assert(reg.max_supply - amount >= asset(0, amount.symbol), "cannot lower max supply below 0");

    //update max_supply
    registries.modify(reg, same_payer, [&](auto& row) {
        row.max_supply -= amount;
    });

    //print("\nLower Max Supply: SUCCESS");
}

void trail::transfer(name sender, name recipient, asset amount) {
    require_auth(sender);
    eosio_assert(sender != recipient, "cannot send tokens to yourself");
    eosio_assert(amount > asset(0, amount.symbol), "must transfer grater than 0 tokens");
    //TODO: check amount.is_valid()

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(amount.symbol.code().raw(), "token registry doesn't exist");
    eosio_assert(reg.settings.is_transferable == true, "token registry disallows transfers");

    //check sender has sufficient balance
    balances_table sendbal(get_self(), amount.symbol.code().raw());
    auto sbal = sendbal.get(sender.value, "sender has no balance of token");
    eosio_assert(sbal.tokens - amount >= asset(0, amount.symbol), "insufficient funds in sender's balance");

    //check recipient has balance object to hold new funds
    balances_table recbal(get_self(), amount.symbol.code().raw());
    auto rbal = recbal.get(recipient.value, "recipient doesn't have a balance to hold transferred funds");

    //subtract amount from sender
    sendbal.modify(sbal, same_payer, [&](auto& row) {
        row.tokens -= amount;
    });

    //add amount to recipient
    recbal.modify(rbal, same_payer, [&](auto& row) {
        row.tokens += amount;
    });

    //print("\nTransferring " + amount + " : " + sender + " => " + recipient);
    //print("\nTransfer: SUCCESS");
}

#pragma endregion Tokens


#pragma region Voting

void trail::regvoter(name voter, symbol token_symbol) {
    require_auth(voter);

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(token_symbol.code().raw(), "registry doesn't exist for given token");

    //check balance doesn't already exist
    balances_table balances(get_self(), token_symbol.code().raw());
    auto b = balances.find(voter.value);
    eosio_assert(b == balances.end(), "voter already has a wallet for given token");

    //emplace new balance
    balances.emplace(voter, [&](auto& row){
        row.owner = voter;
        row.tokens = asset(0, token_symbol);
    });

    //update total voters in registry
    registries.modify(reg, same_payer, [&](auto& row) {
        row.total_voters += uint32_t(1);
    });

}

void trail::unregvoter(name voter, symbol token_symbol) {
    require_auth(voter);

    //check balance exists
    balances_table balances(get_self(), token_symbol.code().raw());
    auto bal = balances.get(voter.value, "voter balance with that symbol doesn't exist");

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(token_symbol.code().raw(), "token registry doesn't exist");

    //must check burnable status, deleting a non-empty wallet is equivalent to burning tokens from supply
    eosio_assert(reg.settings.is_burnable, "token registry disallows burning of funds");

    //update registry
    registries.modify(reg, same_payer, [&](auto& row) {
        row.supply -= bal.tokens;
        row.total_voters -= uint32_t(1);
    });

    //TODO: check that voter has zero proxied tokens?

    balances.erase(bal);

}

void trail::mirrorcast(name voter, symbol token_symbol) {
    require_auth(voter);

    //TODO: add ability to mirrorcast any token?
    eosio_assert(token_symbol == symbol("TLOS", 4), "Can only mirrorcast TLOS");

    //converts TLOS stake to VOTE tokens
    asset new_votes = asset(get_staked_tlos(voter).amount, VOTE_SYM);

    balances_table balances(get_self(), VOTE_SYM.code().raw());
    auto b_itr = balances.find(voter.value);

    //TODO: emplace balance of 0 VOTE if not found

    eosio_assert(b_itr != balances.end(), "Voter is not registered");
    auto bal = *b_itr;

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(VOTE_SYM.code().raw(), "token registry with that symbol doesn't exist");

    //subtract old balance from supply
    reg.supply -= bal.tokens;

    //update VOTE balance
    balances.modify(b_itr, same_payer, [&](auto& row) {
        row.tokens = new_votes;
    });

    //update supply
    reg.supply += new_votes;
    registries.modify(reg, same_payer, [&](auto& row) {
        row.supply = reg.supply;
    });

    //print("\nMirrorCast: SUCCESS");
}

void trail::castvote(name voter, uint64_t ballot_id, uint16_t direction) {
    require_auth(voter);

    //check ballot exists
    ballots_table ballots(get_self(), get_self().value);
    auto bal = ballots.get(ballot_id, "ballot with given ballot_id doesn't exist");

    //TODO: remove
    bool vote_success = false;

    //TODO: change to table_name
    switch (bal.table_id) {
        case 0 : 
            eosio_assert(direction >= uint16_t(0) && direction <= uint16_t(2), "Invalid Vote [0 = NO, 1 = YES, 2 = ABSTAIN]");
            vote_success = vote_for_proposal(voter, ballot_id, bal.reference_id, direction);
            break;
        case 1 : 
            eosio_assert(true == false, "feature still in development...");
            //vote_success = vote_for_election(voter, ballot_id, bal.reference_id, direction);
            break;
        case 2 : 
            vote_success = vote_for_leaderboard(voter, ballot_id, bal.reference_id, direction);
            break;
    }

    //TODO: case for polls

}

void trail::deloldvotes(name voter, uint16_t num_to_delete) {
    //require_auth(voter);
    eosio_assert(num_to_delete > uint16_t(0), "must clean more than 0 receipts");

    //start iterator at beginning
    votereceipts_table votereceipts(get_self(), voter.value);
    auto itr = votereceipts.begin();

    while (itr != votereceipts.end() && num_to_delete > 0) {
        if (itr->expiration < env_struct.time_now) { //vote_receipt has expired
            itr = votereceipts.erase(itr); //returns iterator to next element
            num_to_delete--;
        } else {
            itr++;
        }
    }

}

#pragma endregion Voting


#pragma region Proxies

// void regproxy() {
// }

// void unregproxy() {
// }

#pragma endregion Proxies


#pragma region Ballots

void trail::regballot(name publisher, uint8_t ballot_type, symbol voting_symbol, uint32_t begin_time, uint32_t end_time, string info_url) {
    require_auth(publisher);
    eosio_assert(ballot_type >= 0 && ballot_type <= 2, "invalid ballot type");
    eosio_assert(begin_time < end_time, "begin time must be less than end time");

    //check registry exists
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(voting_symbol.code().raw(), "token registry with that symbol doesn't exist");

    //TODO: change to name type
    uint64_t new_ref_id;

    switch (ballot_type) {
        case 0 : 
            new_ref_id = make_proposal(publisher, voting_symbol, begin_time, end_time, info_url);
            env_struct.totals[0]++;
            break;
        case 1 : 
            eosio_assert(true == false, "feature still in development...");
            //new_ref_id = make_election(publisher, voting_symbol, begin_time, end_time, info_url);
            //env_struct.totals[1]++;
            break;
        case 2 : 
            new_ref_id = make_leaderboard(publisher, voting_symbol, begin_time, end_time, info_url);
            env_struct.totals[2]++;
            break;
    }

    ballots_table ballots(get_self(), get_self().value);
    
    //TODO: remove
    uint64_t new_ballot_id = ballots.available_primary_key();
    env_struct.last_ballot_id = new_ballot_id;

    //emplace new ballot
    ballots.emplace(publisher, [&](auto& row) {
        row.ballot_id = new_ballot_id;
        row.table_id = ballot_type;
        row.reference_id = new_ref_id;
    });

}

void trail::unregballot(name publisher, uint64_t ballot_id) {
    require_auth(publisher);

    //check ballot exists
    ballots_table ballots(get_self(), get_self().value);
    auto bal = ballots.get(ballot_id, "ballot id doesn't exist");

    //TODO: remove
    bool del_success = false;

    switch (bal.table_id) {
        case 0 : 
            del_success = delete_proposal(bal.reference_id, publisher);
            env_struct.totals[0]--;
            break;
        case 1 : 
            eosio_assert(true == false, "feature still in development...");
            //del_success = delete_election(bal.reference_id, publisher);
            //env_struct.totals[1]--;
            break;
        case 2 : 
            del_success = delete_leaderboard(bal.reference_id, publisher);
            env_struct.totals[2]--;
            break;
    }

    if (del_success) {
        ballots.erase(bal);
    }

}

void trail::addcandidate(name publisher, uint64_t ballot_id, name new_candidate, string info_link) {
    require_auth(publisher);
    eosio_assert(is_account(new_candidate), "new candidate is not an account");

    //check ballot exists
    ballots_table ballots(get_self(), get_self().value);
    auto bal = ballots.get(ballot_id, "ballot with given ballot_id doesn't exist");
    eosio_assert(bal.table_id == 2, "ballot type doesn't support candidates");

    //TODO: refactor for elections and boards
    leaderboards_table leaderboards(get_self(), get_self().value);
    auto board = leaderboards.get(bal.reference_id, "leaderboard doesn't exist");
	eosio_assert(board.available_seats > 0, "num_seats must be a non-zero number");
    eosio_assert(board.publisher == publisher, "cannot add candidate to another account's leaderboard");
    eosio_assert(now() < board.begin_time , "cannot add candidates once voting has begun");

    auto existing_candidate = std::find_if(board.candidates.begin(), board.candidates.end(), [&new_candidate](const candidate &c) {
        return c.member == new_candidate; 
    });
    eosio_assert(existing_candidate == board.candidates.end(), "candidate already in leaderboard");

    candidate new_candidate_struct = candidate{
        new_candidate,
        info_link,
        asset(0, board.voting_symbol),
        0
    };

    //update list of candidates in leaderboard
    leaderboards.modify(board, same_payer, [&](auto& row) {
        row.candidates.push_back(new_candidate_struct);
    });

}

void trail::rmvcandidate(name publisher, uint64_t ballot_id, name candidate) {
    require_auth(publisher);

    //check ballot exists
    ballots_table ballots(get_self(), get_self().value);
    auto bal = ballots.get(ballot_id, "ballot with given ballot_id doesn't exist");
    eosio_assert(bal.table_id == 2, "ballot type doesn't support candidates");

    //check leaderboard exists
    leaderboards_table leaderboards(get_self(), get_self().value);
    auto board = leaderboards.get(bal.reference_id, "leaderboard doesn't exist");
    eosio_assert(board.publisher == publisher, "cannot remove candidate from another account's leaderboard");
    eosio_assert(now() < board.begin_time, "cannot remove candidates once voting has begun");

    //search and remove candidate from list
    auto new_candidates = board.candidates;
    bool found = false;
    for (auto itr = new_candidates.begin(); itr != new_candidates.end(); itr++) {
        auto cand = *itr;
        if (cand.member == candidate) {
            new_candidates.erase(itr);
            found = true;
            break;
        }
    }

    //roll back whole trx if not found
    eosio_assert(found, "candidate not found in leaderboard list");

    //update candidate list in leaderboard
    leaderboards.modify(board, same_payer, [&](auto& row) {
        row.candidates = new_candidates;
    });

}

void trail::setallcands(name publisher, uint64_t ballot_id, vector<candidate> new_candidates) {
    require_auth(publisher);

    //TODO: add validations

    //check ballot exists
    ballots_table ballots(get_self(), get_self().value);
    auto bal = ballots.get(ballot_id, "ballot with given ballot_id doesn't exist");
    eosio_assert(bal.table_id == 2, "ballot type doesn't support candidates");

    //check leaderboard exists
    leaderboards_table leaderboards(get_self(), get_self().value);
    auto board = leaderboards.get(bal.reference_id, "leaderboard doesn't exist");
    eosio_assert(board.publisher == publisher, "cannot change candidates on another account's leaderboard");
    eosio_assert(now() < board.begin_time , "cannot change candidates once voting has begun");

    //update candidates list in leaderboard
    leaderboards.modify(board, same_payer, [&](auto& row) {
        row.candidates = new_candidates;
    });

}

void trail::setallstats(name publisher, uint64_t ballot_id, vector<uint8_t> new_cand_statuses) {
    require_auth(publisher);

    ballots_table ballots(_self, _self.value);
    auto b = ballots.find(ballot_id);
    eosio_assert(b != ballots.end(), "ballot with given ballot_id doesn't exist");
    auto bal = *b;
    eosio_assert(bal.table_id == 2, "ballot type doesn't support candidates");

    leaderboards_table leaderboards(_self, _self.value);
    auto l = leaderboards.find(bal.reference_id);
    eosio_assert(l != leaderboards.end(), "leaderboard doesn't exist");
    auto board = *l;
    eosio_assert(board.publisher == publisher, "cannot change candidate statuses on another account's leaderboard");
    eosio_assert(now() > board.end_time , "cannot change candidate statuses until voting has ended");

    auto new_cands = set_candidate_statuses(board.candidates, new_cand_statuses);

    leaderboards.modify(l, same_payer, [&]( auto& a ) {
        a.candidates = new_cands;
    });

    //print("\nSet All Candidate Statuses: SUCCESS");
}

void trail::setseats(name publisher, uint64_t ballot_id, uint8_t num_seats) {
    require_auth(publisher);
    eosio_assert(num_seats > uint8_t(0), "num seats must be greater than 0");

    //check ballot exists
    ballots_table ballots(get_self(), get_self().value);
    auto bal = ballots.get(ballot_id, "ballot with given ballot_id doesn't exist");

    //check leaderboard exists
    leaderboards_table leaderboards(get_self(), get_self().value);
    auto board = leaderboards.get(bal.reference_id, "leaderboard doesn't exist");
    eosio_assert(now() < board.begin_time, "cannot set seats after voting has begun");
    eosio_assert(board.publisher == publisher, "cannot set seats for another account's leaderboard");

    //update seats in leaderboard
    leaderboards.modify(board, same_payer, [&]( auto& row) {
        row.available_seats = num_seats;
    });

}

void trail::closeballot(name publisher, uint64_t ballot_id, uint8_t pass) {
    require_auth(publisher);

    //check ballot exists
    ballots_table ballots(get_self(), get_self().value);
    auto bal = ballots.get(ballot_id, "ballot with given ballot_id doesn't exist");

    //TODO: remove
    bool close_success = false;

    switch (bal.table_id) {
        case 0 : 
            close_success = close_proposal(bal.reference_id, pass, publisher);
            break;
        case 1 : 
            eosio_assert(true == false, "feature still in development...");
            //close_success = close_election(bal.reference_id, pass);
            break;
        case 2: 
            close_success = close_leaderboard(bal.reference_id, pass, publisher);
            break;
    }

}

void trail::nextcycle(name publisher, uint64_t ballot_id, uint32_t new_begin_time, uint32_t new_end_time) {
    require_auth(publisher);
    eosio_assert(new_begin_time < new_end_time, "begin time must be less than end time");

    //check ballot exists
    ballots_table ballots(get_self(), get_self().value);
    auto bal = ballots.get(ballot_id, "ballot doesn't exist");
    eosio_assert(bal.table_id == 0, "ballot type doesn't support cycles");

    //check proposal exists
    proposals_table proposals(get_self(), get_self().value);
    auto prop = proposals.get(bal.reference_id, "proposal doesn't exist");

    //validate new cycle
	eosio_assert(env_struct.time_now < prop.begin_time || env_struct.time_now > prop.end_time, 
		"a proposal can only be cycled before begin_time or after end_time");

    auto sym = prop.no_count.symbol; //NOTE: uses same voting symbol as before

    //update proposal
    proposals.modify(prop, same_payer, [&](auto& row) {
        row.no_count = asset(0, sym);
        row.yes_count = asset(0, sym);
        row.abstain_count = asset(0, sym);
        row.unique_voters = uint32_t(0);
        row.begin_time = new_begin_time;
        row.end_time = new_end_time;
        row.cycle_count += 1;
        row.status = 0;
    });

}

#pragma endregion Ballots


#pragma region Helpers

uint64_t trail::make_proposal(name publisher, symbol voting_symbol, uint32_t begin_time, uint32_t end_time, string info_url) {

    proposals_table proposals(get_self(), get_self().value);
    uint64_t new_prop_id = proposals.available_primary_key();

    proposals.emplace(publisher, [&](auto& row) {
        row.prop_id = new_prop_id;
        row.publisher = publisher;
        row.info_url = info_url;
        row.no_count = asset(0, voting_symbol);
        row.yes_count = asset(0, voting_symbol);
        row.abstain_count = asset(0, voting_symbol);
        row.unique_voters = uint32_t(0);
        row.begin_time = begin_time;
        row.end_time = end_time;
        row.cycle_count = 0;
        row.status = 0;
    });

    return new_prop_id;
}

bool trail::delete_proposal(uint64_t prop_id, name publisher) {

    //check proposal exists
    proposals_table proposals(get_self(), get_self().value);
    auto prop = proposals.get(prop_id, "proposal doesn't exist");
    eosio_assert(now() < prop.begin_time, "cannot delete proposal once voting has begun");
	eosio_assert(prop.publisher == publisher, "cannot delete another account's proposal");
    eosio_assert(prop.cycle_count == 0, "proposal must be on initial cycle to delete");
    //TODO: eosio_assert that status > 0?

    proposals.erase(prop);

    return true;
}

bool trail::vote_for_proposal(name voter, uint64_t ballot_id, uint64_t prop_id, uint16_t direction) {
    
    //check proposal exists
    proposals_table proposals(get_self(), get_self().value);
    auto prop = proposals.get(prop_id, "proposal doesn't exist");

    //check registry exists
	registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(prop.no_count.symbol.code().raw(), "token registry with that symbol doesn't exist");
    eosio_assert(env_struct.time_now >= prop.begin_time && env_struct.time_now <= prop.end_time, "ballot voting window not open");

    votereceipts_table votereceipts(_self, voter.value);
    auto vr_itr = votereceipts.find(ballot_id);
    
    uint32_t new_voter = 1;
    asset vote_weight = get_vote_weight(voter, prop.no_count.symbol);
    eosio_assert(vote_weight > asset(0, prop.no_count.symbol), "vote weight must be greater than 0");

    if (vr_itr == votereceipts.end()) { //voter hasn't voted on ballot before

        vector<uint16_t> new_directions;
        new_directions.emplace_back(direction);

        votereceipts.emplace(voter, [&](auto& row){
            row.ballot_id = ballot_id;
            row.directions = new_directions;
            row.weight = vote_weight;
            row.expiration = prop.end_time;
        });
        
    } else { //vote for ballot_id already exists
        auto vr = *vr_itr;

        if (vr.expiration == prop.end_time) { //vote is for same cycle

			eosio_assert(reg.settings.is_recastable, "token registry disallows vote recasting");

            if (vr.directions[0] == direction) {
                vote_weight -= vr.weight;
            } else {

                switch (vr.directions[0]) { //remove old vote weight from proposal
                    case 0 : prop.no_count -= vr.weight; break;
                    case 1 : prop.yes_count -= vr.weight; break;
                    case 2 : prop.abstain_count -= vr.weight; break;
                }

                vr.directions[0] = direction;

                votereceipts.modify(vr_itr, same_payer, [&](auto& row) {
                    row.directions = vr.directions;
                    row.weight = vote_weight;
                });
            }
            
            new_voter = 0;

        } else if (vr.expiration < prop.end_time) { //vote is for new cycle on same proposal
            
            vr.directions[0] = direction;

            votereceipts.modify(vr_itr, same_payer, [&](auto& row) {
                row.directions = vr.directions;
                row.weight = vote_weight;
                row.expiration = prop.end_time;
            });

        }
    }

    switch (direction) { //update proposal with new weight
        case 0 : prop.no_count += vote_weight; break;
        case 1 : prop.yes_count += vote_weight; break;
        case 2 : prop.abstain_count += vote_weight; break;
    }

    proposals.modify(prop, same_payer, [&](auto& row) {
        row.no_count = prop.no_count;
        row.yes_count = prop.yes_count;
        row.abstain_count = prop.abstain_count;
        row.unique_voters += new_voter;
    });

    return true;
}

bool trail::close_proposal(uint64_t prop_id, uint8_t pass, name publisher) {

    //check proposal exists
    proposals_table proposals(get_self(), get_self().value);
    auto prop = proposals.get(prop_id, "proposal doesn't exist");
    eosio_assert(now() > prop.end_time, "can't close proposal while voting is still open");
	eosio_assert(prop.publisher == publisher, "cannot close another account's proposal");

    //update proposal with new status
    proposals.modify(prop, same_payer, [&](auto& row) {
        row.status = pass;
    });

    return true;
}


uint64_t trail::make_election(name publisher, symbol voting_symbol, uint32_t begin_time, uint32_t end_time, string info_url) {
    
    elections_table elections(get_self(), get_self().value);
    uint64_t new_elec_id = elections.available_primary_key();

    vector<candidate> empty_candidate_list;

    elections.emplace(publisher, [&](auto& row) {
        row.election_id = new_elec_id;
        row.publisher = publisher;
        row.info_url = info_url;
        row.candidates = empty_candidate_list;
        row.unique_voters = 0;
        row.voting_symbol = voting_symbol;
        row.begin_time = begin_time;
        row.end_time = end_time;
    });

    return new_elec_id;
}

bool trail::vote_for_election(name voter, uint64_t ballot_id, uint64_t elec_id, uint16_t direction) {
    //TODO: implement
    return false;
}

bool trail::close_election(uint64_t elec_id, uint8_t pass, name publisher) {
    //TODO: implement
    return false;
}

bool trail::delete_election(uint64_t elec_id, name publisher) {

    //check election exists
    elections_table elections(get_self(), get_self().value);
    auto elec = elections.get(elec_id, "election doesn't exist");
    eosio_assert(now() < elec.begin_time, "cannot delete election once voting has begun");
    eosio_assert(elec.publisher == publisher, "cannot delete another account's election");

    elections.erase(elec);

    return true;
}


uint64_t trail::make_leaderboard(name publisher, symbol voting_symbol, uint32_t begin_time, uint32_t end_time, string info_url) {
    require_auth(publisher);

    leaderboards_table leaderboards(get_self(), get_self().value);
    uint64_t new_board_id = leaderboards.available_primary_key();

    vector<candidate> candidates;

    leaderboards.emplace(publisher, [&]( auto& a ) {
        a.board_id = new_board_id;
        a.publisher = publisher;
        a.info_url = info_url;
        a.candidates = candidates;
        a.unique_voters = uint32_t(0);
        a.voting_symbol = voting_symbol;
        a.available_seats = 0;
        a.begin_time = begin_time;
        a.end_time = end_time;
        a.status = 0;
    });

    return new_board_id;
}

bool trail::delete_leaderboard(uint64_t board_id, name publisher) {

    //check leaderboard exists
    leaderboards_table leaderboards(get_self(), get_self().value);
    auto board = leaderboards.get(board_id, "leaderboard doesn't exist");
    eosio_assert(now() < board.begin_time, "cannot delete leaderboard once voting has begun");
    eosio_assert(board.publisher == publisher, "cannot delete another account's leaderboard");

    leaderboards.erase(board);

    return true;
}

bool trail::vote_for_leaderboard(name voter, uint64_t ballot_id, uint64_t board_id, uint16_t direction) {

    //check leaderboard exists
    leaderboards_table leaderboards(get_self(), get_self().value);
    auto board = leaderboards.get(board_id, "leaderboard doesn't exist");
	eosio_assert(direction < board.candidates.size(), "direction must map to an existing candidate in the leaderboard struct");
    eosio_assert(env_struct.time_now >= board.begin_time && env_struct.time_now <= board.end_time, "ballot voting window not open");

    //check registry exists
	registries_table registries(get_self(), get_self().value);
	auto reg = registries.get(board.voting_symbol.code().raw(), "token registry does not exist");

    votereceipts_table votereceipts(_self, voter.value);
    auto vr_itr = votereceipts.find(ballot_id);
    
    uint32_t new_voter = 1;
    asset vote_weight = get_vote_weight(voter, board.voting_symbol);
	eosio_assert(vote_weight > asset(0, board.voting_symbol), "vote weight must be greater than 0");

    if (vr_itr == votereceipts.end()) { //voter hasn't voted on ballot before

        vector<uint16_t> new_directions;
        new_directions.emplace_back(direction);

        //emplace new receipt
        votereceipts.emplace(voter, [&](auto& row){
            row.ballot_id = ballot_id;
            row.directions = new_directions;
            row.weight = vote_weight;
            row.expiration = board.end_time;
        });
        
    } else { //vote for ballot_id already exists

        auto vr = *vr_itr;

        if (vr.expiration == board.end_time && !has_direction(direction, vr.directions)) { //hasn't voted for candidate before
            new_voter = 0;
            vr.directions.emplace_back(direction);

            votereceipts.modify(vr, same_payer, [&](auto& row) {
                row.directions = vr.directions;
            });

        } else if (vr.expiration == board.end_time && has_direction(direction, vr.directions)) { //vote already exists for candidate (recasting)
			eosio_assert(reg.settings.is_recastable, "token registry disallows vote recasting");
            eosio_assert(true == false, "Feature currently disabled");
            new_voter = 0;
		}
        
    }

    //update candidate votes
    board.candidates[direction].votes += vote_weight;

    //update leaderboard
    leaderboards.modify(board, same_payer, [&](auto& row) {
        row.candidates = board.candidates;
        row.unique_voters += new_voter;
    });

    return true;
}

bool trail::close_leaderboard(uint64_t board_id, uint8_t pass, name publisher) {

    //check leaderboard exists
    leaderboards_table leaderboards(get_self(), get_self().value);
    auto board = leaderboards.get(board_id, "leaderboard doesn't exist");
    eosio_assert(now() > board.end_time, "cannot close leaderboard while voting is still open");
    eosio_assert(board.publisher == publisher, "cannot close another account's leaderboard");

    //update leaderboard status
    leaderboards.modify(board, same_payer, [&](auto& row) {
        row.status = pass;
    });

    return true;
}

asset trail::get_vote_weight(name voter, symbol voting_symbol) {
    
    balances_table balances(_self, voting_symbol.code().raw());
    auto b = balances.find(voter.value);

    if (b == balances.end()) { //no balance found, returning 0
        return asset(0, voting_symbol);
    } else {
        auto bal = *b;
        return bal.tokens;
    }
}

bool trail::has_direction(uint16_t direction, vector<uint16_t> direction_list) {
    for (uint16_t item : direction_list) {
        if (item == direction) {
            return true;
        }
    }
    return false;
}

vector<candidate> trail::set_candidate_statuses(vector<candidate> candidate_list, vector<uint8_t> new_status_list) {
    eosio_assert(candidate_list.size() == new_status_list.size(), "status list does not correctly map to candidate list");
    for (int idx = 0; idx < candidate_list.size(); idx++) {
        candidate_list[idx].status = new_status_list[idx];
    }
    return candidate_list;
}

#pragma endregion Helpers


#pragma region Reactions

// void trail::update_from_cb(name from, asset amount) {
//     counterbalances_table fromcbs(_self, amount.symbol.code().raw());
//     auto cb_itr = fromcbs.find(from.value);
//     if (cb_itr == fromcbs.end()) {
// 		uint32_t new_now = now();
//         fromcbs.emplace(_self, [&]( auto& a ){ //TODO: change ram payer to user? may prevent TLOS transfers
//             a.owner = from;
//             a.decayable_cb = asset(0, symbol("VOTE", 4));
// 			a.persistent_cb = asset(0, symbol("VOTE", 4));
//             a.last_decay = new_now;
//         });
//     } else {
//         auto from_cb = *cb_itr;
//         asset new_cb = from_cb.decayable_cb - amount;
//         if (new_cb < asset(0, symbol("VOTE", 4))) {
//             new_cb = asset(0, symbol("VOTE", 4));
//         }
//         fromcbs.modify(cb_itr, same_payer, [&]( auto& a ) {
//             a.decayable_cb = new_cb;
//         });
//     }
// }

// void trail::update_to_cb(name to, asset amount) {
//     counterbalances_table tocbs(_self, amount.symbol.code().raw());
//     auto cb_itr = tocbs.find(to.value);
//     if (cb_itr == tocbs.end()) {
// 		//print("\ntime_now: ", env_struct.time_now);
//         tocbs.emplace(_self, [&]( auto& a ){ //TODO: change ram payer to user? may prevent TLOS transfers
//             a.owner = to;
//             a.decayable_cb = asset(amount.amount, symbol("VOTE", 4));
// 			a.persistent_cb = asset(0, symbol("VOTE", 4));
//             a.last_decay = env_struct.time_now;
//         });
//     } else {
//         auto to_cb = *cb_itr;
//         asset new_cb = to_cb.decayable_cb + amount;
//         tocbs.modify(cb_itr, same_payer, [&]( auto& a ) {
//             a.decayable_cb = new_cb;
//         });
//     }
// }

// asset trail::get_decay_amount(name voter, symbol token_symbol, uint32_t decay_rate) {
//     counterbalances_table counterbals(_self, token_symbol.code().raw());
//     auto cb_itr = counterbals.find(voter.value);
//     uint32_t time_delta;
//     int prec = token_symbol.precision();
//     int val = 1;
//     for (int i = prec; i > 0; i--) {
//         val *= 10;
//     }
//     if (cb_itr != counterbals.end()) {
//         auto cb = *cb_itr;
//         time_delta = env_struct.time_now - cb.last_decay;
//         return asset(int64_t(time_delta / decay_rate) * val, token_symbol);
//     }
//     return asset(0, token_symbol);
// }

#pragma endregion Reactions


#pragma region Admin

void trail::pause() {
    require_auth(name("eosio.trail"));
    
    global_singleton globals(get_self(), get_self().value);
    global global_struct = globals.get();
    eosio_assert(global_struct.is_live, "trail is already paused");

    global_struct.is_live = false;

    globals.set(global_struct, get_self());
}

void trail::resume() {
    require_auth(name("eosio.trail"));
    
    global_singleton globals(get_self(), get_self().value);
    global global_struct = globals.get();
    eosio_assert(!global_struct.is_live, "trail is already live");
    
    global_struct.is_live = true;

    globals.set(global_struct, get_self());
}

#pragma endregion Admin


#pragma region Migration

void trail::releasecb(name voter) {
    counterbalances_table cbs(get_self(), VOTE_SYM.code().raw());
    auto cb = cbs.get(voter.value, "voter doesn't have a counterbalance");
    cbs.erase(cb);
}

void trail::resetreg() {
    registries_table registries(get_self(), get_self().value);
    auto reg = registries.get(VOTE_SYM.code().raw(), "token registry doesn't exist");

    registries.modify(reg, same_payer, [&](auto& row) {
        row.supply = asset(0, VOTE_SYM);;
    });
}

void trail::resetbal(name voter) {
    balances_table balances(get_self(), get_self().value);
    auto bal = balances.get(voter.value, "balance not found");

    balances.modify(bal, same_payer, [&](auto& row) {
        row.tokens = asset(0, VOTE_SYM);
    });
}

void trail::makeglobals() {

    global_singleton globals(get_self(), get_self().value);

    vector<uint16_t> new_totals = {0, 0, 0, 0};

    //TODO: initialize new_totals;

    global global_struct = global{
        get_self(), //publisher
        new_totals, //ballot_totals
        uint16_t(21), //max_vote_receipts
        true //is_live
    };

    globals.set(global_struct, get_self());

}

#pragma endregion Migration


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

		if (code == self && action == name("regtoken").value) {
            execute_action(name(self), name(code), &trail::regtoken);
        } else if (code == self && action == name("initsettings").value) {
            execute_action(name(self), name(code), &trail::initsettings);
        } else if (code == self && action == name("unregtoken").value) {
            execute_action(name(self), name(code), &trail::unregtoken);
        } else if (code == self && action == name("regvoter").value) {
            execute_action(name(self), name(code), &trail::regvoter);
        } else if (code == self && action == name("unregvoter").value) {
            execute_action(name(self), name(code), &trail::unregvoter);
        } else if (code == self && action == name("regballot").value) {
            execute_action(name(self), name(code), &trail::regballot);
        } else if (code == self && action == name("unregballot").value) {
            execute_action(name(self), name(code), &trail::unregballot);
        } else if (code == self && action == name("mirrorcast").value) {
            execute_action(name(self), name(code), &trail::mirrorcast);
        } else if (code == self && action == name("castvote").value) {
            execute_action(name(self), name(code), &trail::castvote);
        } else if (code == self && action == name("closeballot").value) {
            execute_action(name(self), name(code), &trail::closeballot);
        } else if (code == self && action == name("nextcycle").value) {
            execute_action(name(self), name(code), &trail::nextcycle);
        } else if (code == self && action == name("deloldvotes").value) {
            execute_action(name(self), name(code), &trail::deloldvotes);
        } else if (code == self && action == name("addcandidate").value) {
            execute_action(name(self), name(code), &trail::addcandidate);
        } else if (code == self && action == name("setallcands").value) {
            execute_action(name(self), name(code), &trail::setallcands);
        } else if (code == self && action == name("rmvcandidate").value) {
            execute_action(name(self), name(code), &trail::rmvcandidate);
        } else if (code == self && action == name("setseats").value) {
            execute_action(name(self), name(code), &trail::setseats);
        } else if (code == self && action == name("issuetoken").value) {
            execute_action(name(self), name(code), &trail::issuetoken);
        } else if (code == self && action == name("claimairgrab").value) {
            execute_action(name(self), name(code), &trail::claimairgrab);
        } else if (code == self && action == name("burntoken").value) {
            execute_action(name(self), name(code), &trail::burntoken);
        } else if (code == self && action == name("seizetoken").value) {
            execute_action(name(self), name(code), &trail::seizetoken);
        } else if (code == self && action == name("seizeairgrab").value) {
            execute_action(name(self), name(code), &trail::seizeairgrab);
        } else if (code == self && action == name("seizebygroup").value) {
            execute_action(name(self), name(code), &trail::seizebygroup);
        } else if (code == self && action == name("setallstats").value) {
            execute_action(name(self), name(code), &trail::setallstats);
        } else if (code == self && action == name("raisemax").value) {
            execute_action(name(self), name(code), &trail::raisemax);
        } else if (code == self && action == name("lowermax").value) {
            execute_action(name(self), name(code), &trail::lowermax);
        } else if (code == self && action == name("transfer").value) {
            execute_action(name(self), name(code), &trail::transfer);
        }
        // else if (code == name("eosio.token").value && action == name("transfer").value) { //NOTE: updates counterbals after transfers
        //     trail trailservice(name(self), name(code), ds);
        //     auto args = unpack_action_data<transfer_args>();
        //     trailservice.update_from_cb(args.from, asset(args.quantity.amount, symbol("VOTE", 4)));
        //     trailservice.update_to_cb(args.to, asset(args.quantity.amount, symbol("VOTE", 4)));
        // }
    } //end apply
}; //end dispatcher
