#include "../include/trail.hpp"

trail::trail(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

trail::~trail() {}



void trail::createballot(name ballot_name, name category, name publisher, string title, string description, string info_url, symbol voting_sym) {
    require_auth(publisher);

    ballots ballots(get_self(), get_self().value);
    auto b = ballots.find(ballot_name.value);
    check(b == ballots.end(), "ballot name already exists");

    vector<option> blank_options;

    ballots.emplace(publisher, [&](auto& row){
        row.ballot_name = ballot_name;
        row.category = category;
        row.publisher = publisher;
        row.title = title;
        row.description = description;
        row.info_url = info_url;
        row.options = blank_options;
        row.unique_voters = 0;
        row.voting_symbol = voting_sym;
        row.begin_time = 0;
        row.end_time = 0;
        row.status = SETUP;
    });

}

void trail::addoption(name ballot_name, name publisher, name option_name, string option_info) {
    require_auth(publisher);

    ballots ballots(get_self(), get_self().value);
    auto bal = ballots.get(ballot_name.value, "ballot name doesn't exist");
    check(bal.publisher == publisher, "only ballot publisher can add options");
    check(is_existing_option(option_name, bal.options) == false, "option is already in ballot");

    option new_option = {
        option_name,
        option_info,
        asset(0, bal.voting_symbol)
    };

    ballots.modify(bal, same_payer, [&](auto& row) {
        row.options.emplace_back(new_option);
    });
}

void trail::readyballot(name ballot_name, name publisher, uint32_t end_time) {
    require_auth(publisher);

    ballots ballots(get_self(), get_self().value);
    auto bal = ballots.get(ballot_name.value, "ballot name doesn't exist");
    check(bal.publisher == publisher, "only ballot publisher can ready ballot");
    check(bal.options.size() >= 2, "ballot must have at least 2 options");
    check(end_time - now() >= MIN_BALLOT_LENGTH, "ballot must be open for at least 1 day");

    ballots.modify(bal, same_payer, [&](auto& row) {
        row.begin_time = now();
        row.end_time = end_time;
        row.status = OPEN;
    });

}

void trail::vote(name voter, name ballot_name, name option) {
    require_auth(voter);

    //attempt to clean up an old vote

    //check max vote receipts

    //undo previous vote, if applicable

    //check ballot exists
    ballots ballots(get_self(), get_self().value);
    auto bal = ballots.get(ballot_name.value, "ballot name doesn't exist");

    //check option exists
    int idx = get_option_index(option, bal.options);
    check(idx != -1, "option not found on ballot");

    //get voting account
    asset votes = get_voting_balance(voter, bal.voting_symbol);
    check(votes.amount > int64_t(0), "cannot vote with a balance of 0");

    //update ballot
    ballots.modify(bal, same_payer, [&](auto& row) {
        row.options[idx].votes += votes;
    });
}

void trail::cleanupvotes(name voter, uint16_t count, symbol voting_sym) {
    vote_receipts votereceipts(get_self(), get_self().value);
    //auto by_exp = votereceipts.get_index<name("byexp")>();

    //get exp by lowest value

    //delete oldest receipt if expired
}



void trail::createtoken(name publisher, asset max_supply, token_settings settings, string info_url) {
    require_auth(publisher);

    symbol new_sym = max_supply.symbol;

    //check registry doesn't already exist
    registries registries(get_self(), get_self().value);
    auto reg = registries.find(new_sym.code().raw());
    check(reg == registries.end(), "registry with symbol not found");

    registries.emplace(publisher, [&](auto& row){
        row.supply = asset(0, new_sym);
        row.max_supply = max_supply;
        row.publisher = publisher;
        row.total_voters = uint32_t(0);
        row.total_proxies = uint32_t(0);
        row.settings = settings;
        row.info_url = info_url;
    });

}

void trail::mint(name publisher, name recipient, asset amount_to_mint) {
    require_auth(publisher);
    check(is_account(recipient), "recipient account doesn't exist");

    symbol token_sym = amount_to_mint.symbol;

    //check registry exists
    registries registries(get_self(), get_self().value);
    auto reg = registries.get(token_sym.code().raw(), "registry with symbol not found");
    check(reg.publisher == publisher, "only registry publisher can mint new tokens");
    check(reg.supply + amount_to_mint <= reg.max_supply, "cannot mint tokens beyond max_supply");

    //check account balance exists
    accounts accounts(get_self(), recipient.value);
    auto acc = accounts.get(token_sym.code().raw(), "account balance not found");

    //update recipient balance
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.balance += amount_to_mint;
    });

    //update registry supply
    registries.modify(reg, same_payer, [&](auto& row) {
        row.supply += amount_to_mint;
    });

}

void trail::open(name owner, symbol token_sym) {
    require_auth(owner);

    //check registry exists
    registries registries(get_self(), get_self().value);
    auto reg = registries.get(token_sym.code().raw(), "registry with symbol not found");

    //check account balance doesn't already exist
    accounts accounts(get_self(), owner.value);
    auto acc = accounts.find(token_sym.code().raw());
    check(acc == accounts.end(), "account balance already exists");

    //emplace account with zero balance
    accounts.emplace(owner, [&](auto& row){
        row.balance = asset(0, token_sym);
    });
}

void trail::close(name owner, symbol token_sym) {
    require_auth(owner);

    //check account exists and is empty
    accounts accounts(get_self(), owner.value);
    auto acc = accounts.get(token_sym.code().raw(), "account balance doesn't exist");
    check(acc.balance == asset(0, token_sym), "cannot close an account still holding tokens");

    accounts.erase(acc);
}



bool trail::is_existing_option(name option_name, vector<option> options) {
    for (option opt : options) {
        if (option_name == opt.option_name) {
            return true;
        }
    }

    return false;
}

int trail::get_option_index(name option_name, vector<option> options) {
    for (int i = 0; i < options.size(); i++) {
        if (option_name == options[i].option_name) {
            return i;
        }
    }

    return -1;
}

asset trail::get_voting_balance(name voter, symbol token_symbol) {
    accounts accounts(get_self(), voter.value);
    auto acc = accounts.get(token_symbol.code().raw(), "account with symbol doesn't exist");
    return acc.balance;
}
