/**
 * @file
 * @copyright defined in telos/LICENSE.txt
 * 
 * @brief Recommended TIP-5 Implementation
 * @author Craig Branscom
 * 
 * This contract is a fully working example implementation of the TIP-5 interface.
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include "token.registry.hpp"

//#include <eosiolib/print.hpp>

using namespace eosio;
using namespace std;

/**
 * @brief Constructor
 */
registry::registry(account_name self) : contract(self), settings(self, self) {
    if (!settings.exists()) {

        _settings = setting{
            self,
            asset(int64_t(10000), S(2, TTT)),
            asset(int64_t(0), S(2, TTT)),
            "Telos Test Token",
            true
        };

        settings.set(_settings, self);
        _settings = settings.get();
    } else {

        _settings = settings.get();
    }
}

/**
 * @brief Destructor
 */
registry::~registry() {
    if (settings.exists()) {
        settings.set(_settings, _settings.issuer);
    }
}

/**
 * @brief Mints new tokens into circulation.
 */
void registry::mint(account_name recipient, asset tokens) {
    require_auth(_settings.issuer);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(_settings.supply + tokens <= _settings.max_supply, "minting would exceed max_supply");

    add_balance(recipient, tokens, _settings.issuer);

    _settings.supply = (_settings.supply + tokens);

    //print("\nToken Name: ", _settings.name);
    //print("\nCirculating Supply: ", _settings.supply);
    //print("\nMax Supply: ", _settings.max_supply);
    //print("\nIssuer: ", name{_settings.issuer});
}

/**
 * @brief Transfers tokens to another account.
 */
void registry::transfer(account_name owner, account_name recipient, asset tokens) {
    require_auth(owner);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(owner != recipient, "cannot transfer to self");
    eosio_assert(tokens.is_valid(), "invalid token symbol" );
    eosio_assert(tokens.amount > 0, "must transfer positive quantity" );
    
    add_balance(recipient, tokens, owner);
    sub_balance(owner, tokens);
}

/**
 * @brief Allots tokens to be claimed by recipient.
 */
void registry::allot(account_name owner, account_name recipient, asset tokens) {
    require_auth(owner);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(owner != recipient, "cannot allot tokens to self");
    eosio_assert(tokens.is_valid(), "invalid token symbol" );
    eosio_assert(tokens.amount > 0, "must allot positive quantity" );

    sub_balance(owner, tokens);
    add_allot(owner, recipient, tokens, owner);
}

/**
 * @brief Reclaims an allotment.
 */
void registry::reclaim(account_name owner, account_name recipient, asset tokens) {
    require_auth(owner);
    eosio_assert(tokens.is_valid(), "invalid token symbol" );
    eosio_assert(tokens.amount > 0, "must allot positive quantity" );

    sub_allot(owner, recipient, tokens);
    add_balance(owner, tokens, owner);
}

/**
 * @brief Transfers an allotment to the intended recipient.
 */
void registry::transferfrom(account_name owner, account_name recipient, asset tokens) {
    require_auth(recipient);
    eosio_assert(is_account(owner), "owner account does not exist");
    eosio_assert(owner != recipient, "cannot transfer from self to self");
    eosio_assert(tokens.is_valid(), "invalid quantity given" );
    eosio_assert(tokens.amount > 0, "must transfer positive quantity" );
    
    sub_allot(owner, recipient, tokens);
    add_balance(recipient, tokens, recipient);
}

/**
 * @brief Creates a new zero balance wallet entry.
 */
void registry::createwallet(account_name owner) {
    require_auth(owner);

    balances_table balances(_settings.issuer, owner);
    auto b = balances.find(owner);

    if(b == balances.end() ) {
        balances.emplace(owner, [&]( auto& a ){
            a.owner = owner;
            a.tokens = asset(int64_t(0), S(2, TTT));
        });

        //print("\nNew wallet created for ", name{owner});
    } else {
        //print("\nWallet already exists for given account");
    }
}

/**
 * @brief Deletes a zero balance wallet entry.
 */
void registry::deletewallet(account_name owner) {
    require_auth(owner);

    balances_table balances(_settings.issuer, owner);
    auto itr = balances.find(owner);
    auto b = *itr;

    if(b.tokens.amount == 0) {
        balances.erase(itr);

        //print("\nWallet deleted for ", name{owner});
    } else {
        //print("\nCannot delete wallet unless balance is 0");
    }
}

void registry::sub_balance(account_name owner, asset tokens) {
    balances_table balances(_settings.issuer, owner);
    auto itr = balances.find(owner);
    auto b = *itr;
    eosio_assert(b.tokens.amount >= tokens.amount, "transaction would overdraw balance");

    balances.modify(itr, 0, [&]( auto& a ) {
        a.tokens -= tokens;

        //print("\nBalance subtracted...");
    });
}

void registry::add_balance(account_name recipient, asset tokens, account_name payer) {
    balances_table balances(_settings.issuer, recipient);
    auto itr = balances.find(recipient);

    eosio_assert(itr != balances.end(), "No wallet found for recipient");

    balances.modify(itr, 0, [&]( auto& a ) {
        a.tokens += tokens;

        //print("\nBalance added...");
    });
}

void registry::sub_allot(account_name owner, account_name recipient, asset tokens) {
    allotments_table allotments(_settings.issuer, owner);
    auto itr = allotments.find(recipient);
    auto al = *itr;
    eosio_assert(al.tokens.amount >= tokens.amount, "transaction would overdraw balance");

    if(al.tokens.amount == tokens.amount ) {
        allotments.erase(itr);

        //print("\nErasing allotment entry...");
    } else {
        allotments.modify(itr, 0, [&]( auto& a ) {
            a.tokens -= tokens;

            //print("\nSubtracting from existing allotment...");
        });
    }
}

void registry::add_allot(account_name owner, account_name recipient, asset tokens, account_name payer) {
    allotments_table allotments(_settings.issuer, owner);
    auto itr = allotments.find(recipient);

    if(itr == allotments.end() ) {
        allotments.emplace(payer, [&]( auto& a ){
            a.recipient = recipient;
            a.owner = owner;
            a.tokens = tokens;
        });

        //print("\nEmplacing new allotment...");
   } else {
        allotments.modify(itr, 0, [&]( auto& a ) {
            a.tokens += tokens;
        });

        //print("\nAdding to existing allotment...");
   }
}

EOSIO_ABI(registry, (mint)(transfer)(allot)(reclaim)(transferfrom)(createwallet)(deletewallet))
