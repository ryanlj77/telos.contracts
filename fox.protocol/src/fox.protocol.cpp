#include <fox.protocol.hpp>
//#include <eosiolib/print.hpp>

fox::fox(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {
}

fox::~fox() {
}

void fox::regdomain(name publisher, symbol native_symbol) {
    require_auth(publisher);

    domains_table domains(get_self(), get_self().value);
    auto d_itr = domains.find(native_symbol.code().raw());
    eosio_assert(d_itr == domains.end(), "Domain with that symbol already exists");

    domains.emplace(get_self(), [&](auto& l) {
        l.native_symbol = native_symbol;
        l.publisher = publisher;
        l.reg_time = now();
    });
}

