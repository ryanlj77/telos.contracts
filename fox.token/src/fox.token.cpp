#include <fox.token.hpp>
#include <eosiolib/print.hpp>

foxtoken::foxtoken(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {
}

foxtoken::~foxtoken() {
}

void foxtoken::mint(name recipient, asset tokens) {
    require_auth(get_self()); //NOTE: only contract owner can mint new tokens


}

void foxtoken::createwallet(name user) {
    require_auth(user);

    balances_table balances(get_self(), get_self().value);
    auto itr = balances.find(user.value);
    eosio_assert(itr == balances.end(), "Wallet already exists for user");

    balances.emplace(user, [&]( auto& l ){
        l.owner = user;
        l.tokens = asset(0, NATIVE_SYMBOL);
        l.on_order = asset(0, NATIVE_SYMBOL);
    });
}

void foxtoken::deletewallet(name user) {
    require_auth(user);

    balances_table balances(get_self(), user.value);
    auto itr = balances.find(user.value);
    eosio_assert(itr != balances.end(), "User doesn't have a wallet");

    auto b = *itr;
    eosio_assert(b.tokens.amount == 0, "Cannot delete wallet unless balance is zero");
    eosio_assert(b.on_order.amount == 0, "Cannot delete wallet unless all orders are fulfilled/canceled");

    balances.erase(itr);
}

void foxtoken::marketorder(name orderer, symbol buy_symbol, asset sell) {
    require_auth(orderer);
    eosio_assert(buy_symbol == NATIVE_SYMBOL, "Cannot buy that asset here");

    //TODO: find target domain

    //TODO: search for match

    //TODO: flag for match, validate

    //auto upper = orders_by_rate.upper_bound(100);

    //TODO: check a match in constructor?? have explicit action? match on order placement?
}

name foxtoken::find_domain(symbol sym) {
    domains_table domains(name("foxprotocol"), name("foxprotocol").value);
    auto d_itr = domains.find(sym.code().raw());
    
    if (d_itr != domains.end()) {
        auto dom = *d_itr;
        return dom.publisher;
    }

    return name{0};
}

double foxtoken::find_market_rate(symbol sym) {
    
    orders_table openorders(get_self(), sym.code().raw());
    auto orders_by_rate = openorders.get_index<name("byrate")>();
    //auto upper = orders_by_rate.upper_bound(100);

    int max_loops = 50;
    for (order o : orders_by_rate) {
        print("rate: ", o.rate);
        max_loops--;
        if (max_loops == 0) {
            break;
        }
    }

    return orders_by_rate.begin()->rate;
}

EOSIO_DISPATCH(foxtoken, (mint)(createwallet)(deletewallet)(marketorder))
