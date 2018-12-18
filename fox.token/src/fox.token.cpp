#include <fox.token.hpp>
//#include <eosiolib/print.hpp>

foxtoken::foxtoken(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {
}

foxtoken::~foxtoken() {
}