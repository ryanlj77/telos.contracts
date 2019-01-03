/**
 * This contract implements the TIP-5 Single Token Interface.
 * 
 * @author Craig Branscom
 */

#include <token.registry.hpp>
#include <eosiolib/print.hpp>

registry::registry(name self, name code, datastream<const char*> ds) : contract(self, code, ds), _config(self, self.value) {
    if (!_config.exists()) {

        config = tokenconfig{
            get_self(), //publisher
            TOKEN_NAME, //token_name
            INITIAL_MAX_SUPPLY, //max_supply
            INITIAL_SUPPLY //supply
        };

        _config.set(config, get_self());
    } else {
        config = _config.get();
    }
}

registry::~registry() {
    if (_config.exists()) {
        _config.set(config, config.publisher);
    }
}

void registry::mint(name recipient, asset tokens) {
    require_auth(config.publisher);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(tokens.is_valid(), "invalid token");
    eosio_assert(tokens.amount > 0, "must mint positive quantity");
    eosio_assert(tokens.symbol == registry::TOKEN_SYM, "only native tokens are mintable");
    eosio_assert(config.supply + tokens <= config.max_supply, "minting would exceed allowed maximum supply");

    config.supply = (config.supply + tokens);

    add_balance(recipient, tokens);
}

void registry::transfer(name sender, name recipient, asset tokens) {
    require_auth(sender);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(sender != recipient, "cannot transfer to self");
    eosio_assert(tokens.is_valid(), "invalid token");
    eosio_assert(tokens.amount > 0, "must transfer positive quantity");
    eosio_assert(tokens.symbol == registry::TOKEN_SYM, "only native tokens are transferable");
    
    add_balance(recipient, tokens);
    sub_balance(sender, tokens);
}

void registry::allot(name sender, name recipient, asset tokens) {
    require_auth(sender);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(sender != recipient, "cannot allot tokens to self");
    eosio_assert(tokens.is_valid(), "invalid token");
    eosio_assert(tokens.amount > 0, "must allot positive quantity");
    eosio_assert(tokens.symbol == registry::TOKEN_SYM, "only native tokens are allotable");

    sub_balance(sender, tokens);
    add_allot(sender, recipient, tokens);
}

void registry::unallot(name sender, name recipient, asset tokens) {
    require_auth(sender);
    eosio_assert(tokens.is_valid(), "invalid token");
    eosio_assert(tokens.amount > 0, "must unallot positive quantity");
    eosio_assert(tokens.symbol == registry::TOKEN_SYM, "only native tokens are unallotable");

    sub_allot(sender, recipient, tokens);
    add_balance(sender, tokens);
}

void registry::claimallot(name sender, name recipient, asset tokens) {
    require_auth(recipient);
    eosio_assert(tokens.is_valid(), "invalid token");
    eosio_assert(tokens.amount > 0, "must claim positive quantity");
    eosio_assert(tokens.symbol == registry::TOKEN_SYM, "only native tokens are claimable");
    
    sub_allot(sender, recipient, tokens);
    add_balance(recipient, tokens);
}

void registry::createwallet(name recipient) {
    require_auth(recipient);

    balances_table balances(config.publisher, config.publisher.value);
    auto itr = balances.find(recipient.value);

    eosio_assert(itr == balances.end(), "Account already owns a wallet");

    balances.emplace(recipient, [&]( auto& a ){
        a.owner = recipient;
        a.tokens = asset(int64_t(0), TOKEN_SYM);
    });
}

void registry::deletewallet(name wallet_owner) {
    require_auth(wallet_owner);

    balances_table balances(config.publisher, config.publisher.value);
    auto itr = balances.find(wallet_owner.value);

    eosio_assert(itr != balances.end(), "Account does not have a wallet to delete");

    auto b = *itr;

    eosio_assert(b.tokens.amount == 0, "Cannot delete wallet unless balance is zero");

    balances.erase(itr);
}

#pragma region Helper_Functions

void registry::add_balance(name recipient, asset tokens) {
    balances_table balances(config.publisher, config.publisher.value);
    auto itr = balances.find(recipient.value);

    eosio_assert(itr != balances.end(), "No wallet found for recipient.");

    balances.modify(itr, same_payer, [&]( auto& a ) {
        a.tokens += tokens;
    });
}

void registry::sub_balance(name sender, asset tokens) {
    balances_table balances(config.publisher, config.publisher.value);
    auto itr = balances.find(sender.value);
    auto b = *itr;

    eosio_assert(b.tokens.amount >= tokens.amount, "Transaction would overdraw balance of sender");

    balances.modify(itr, same_payer, [&]( auto& a ) {
        a.tokens -= tokens;
    });
}

void registry::add_allot(name sender, name recipient, asset tokens) {
    allotments_table allotments(config.publisher, sender.value);
    auto itr = allotments.find(recipient.value);

    if(itr == allotments.end()) { //NOTE: create new allotment
        allotments.emplace(sender, [&]( auto& a ){
            a.recipient = recipient;
            a.sender = sender;
            a.tokens = tokens;
        });
   } else { //NOTE: add to existing allotment
        allotments.modify(itr, same_payer, [&]( auto& a ) {
            a.tokens += tokens;
        });
   }
}

void registry::sub_allot(name sender, name recipient, asset tokens) {
    allotments_table allotments(config.publisher, sender.value);
    auto itr = allotments.find(recipient.value);
    auto al = *itr;

    eosio_assert(al.tokens.amount >= tokens.amount, "transaction would overdraw balance");

    if(al.tokens.amount == tokens.amount ) { //NOTE: erasing allotment since all tokens are being claimed
        allotments.erase(itr);
    } else { //NOTE: subtracting token amount from existing allotment
        allotments.modify(itr, same_payer, [&]( auto& a ) {
            a.tokens -= tokens;
        });
    }
}

#pragma endregion Helper_Functions

EOSIO_DISPATCH(registry, (mint)(transfer)(allot)(unallot)(claimallot)(createwallet)(deletewallet))