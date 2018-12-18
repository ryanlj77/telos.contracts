#include <fox.token.hpp>
//#include <eosiolib/print.hpp>

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
