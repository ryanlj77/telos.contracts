/**
 * Implements a standard eosio.token contract with TIP-5 extension.
 * 
 * @author Craig Branscom
 */

#include <token.registry.hpp>
#include <eosiolib/print.hpp>

registry::registry(name self, name code, datastream<const char*> ds) : contract(self, code, ds), _config(self, self.value) {
    if (!_config.exists()) {

        // config = tokenconfig{
        //     get_self(), //publisher
        //     TOKEN_NAME, //token_name
        //     INITIAL_MAX_SUPPLY, //max_supply
        //     INITIAL_SUPPLY //supply
        // };

        //_config.set(config, get_self());
    } else {
        config = _config.get();
    }
}

registry::~registry() {
    if (_config.exists()) {
        _config.set(config, config.publisher);
    }
}


void registry::create(name issuer, asset maximum_supply) {
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    if (maximum_supply.symbol == symbol("VIITA", 4)) {
        //NOTE: migrates VIITA stats to new table
        statstable.emplace( _self, [&]( auto& s ) {
            s.supply        = config.supply;
            s.max_supply    = config.max_supply;
            s.issuer        = config.publisher;
        });
    } else {
        statstable.emplace( _self, [&]( auto& s ) {
            s.supply.symbol = maximum_supply.symbol;
            s.max_supply    = maximum_supply;
            s.issuer        = issuer;
        });
    }
}

void registry::issue(name to, asset quantity, string memo) {
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
        SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                          { st.issuer, to, quantity, memo }
        );
    }
}

void registry::retire(asset quantity, string memo) {
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must retire positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void registry::transfer(name from, name to, asset quantity, string memo) {
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.code();
    stats statstable( _self, sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    eosio_assert(quantity.is_valid(), "invalid quantity" );
    eosio_assert(quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = has_auth( to ) ? to : from;

    sub_balance(from, quantity);
    add_balance(to, quantity, payer);
}

void registry::open(name owner, const symbol& symbol, name ram_payer) {
    require_auth(ram_payer);

    auto sym_code_raw = symbol.code().raw();

    stats statstable( _self, sym_code_raw );
    const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
    eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch" );

    accounts acnts(_self, owner.value);
    auto it = acnts.find(sym_code_raw);
    
    if (it == acnts.end()) {
        acnts.emplace(ram_payer, [&]( auto& a ){
            a.balance = asset{0, symbol};
        });

        migrate_balance(owner);
    }
}

void registry::close(name owner, const symbol& symbol) {
   require_auth( owner );
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

void registry::allot(name sender, name recipient, asset tokens) {
    require_auth(sender);
    eosio_assert(is_account(recipient), "recipient account does not exist");
    eosio_assert(sender != recipient, "cannot allot tokens to self");
    eosio_assert(tokens.amount > 0, "must allot positive quantity");
    eosio_assert(tokens.is_valid(), "invalid tokens" );

    auto sym_code_raw = tokens.symbol.code().raw();
    stats statstable(get_self(), sym_code_raw);
    const auto& st = statstable.get(sym_code_raw);

    eosio_assert(tokens.symbol == st.supply.symbol, "symbol precision mismatch");

    eosio_assert(false, "allotments disabled during upgrade");

    sub_balance(sender, tokens);
    add_allot(sender, recipient, tokens);
}

void registry::unallot(name sender, name recipient, asset tokens) {
    require_auth(sender);
    eosio_assert(tokens.amount > 0, "must unallot positive quantity");
    eosio_assert(tokens.is_valid(), "invalid tokens" );

    sub_allot(sender, recipient, tokens);
    add_balance(sender, tokens, sender);
}

void registry::claimallot(name sender, name recipient, asset tokens) {
    require_auth(recipient);
    eosio_assert(tokens.amount > 0, "must claim positive quantity");
    eosio_assert(tokens.is_valid(), "invalid tokens");
    
    sub_allot(sender, recipient, tokens);
    add_balance(recipient, tokens, recipient);
}


void registry::sub_balance(name owner, asset value) {
    accounts from_acnts( _self, owner.value );

    const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
    eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

    from_acnts.modify( from, owner, [&]( auto& a ) {
        a.balance -= value;
    });
}

void registry::add_balance(name owner, asset value, name ram_payer) {
    accounts to_acnts( _self, owner.value );
    auto to = to_acnts.find( value.symbol.code().raw() );
    if(to == to_acnts.end()) {
        to_acnts.emplace( ram_payer, [&]( auto& a ){
            a.balance = value;
        });
    } else {
        to_acnts.modify( to, same_payer, [&]( auto& a ) {
            a.balance += value;
        });
    }
}

void registry::sub_allot(name sender, name recipient, asset tokens) {
    allotments_table allotments(get_self(), sender.value);
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

void registry::add_allot(name sender, name recipient, asset tokens) {
    allotments_table allotments(get_self(), sender.value);
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

//NOTE: migrates balance from balances table to accounts table
void registry::migrate_balance(name owner) {
    balances_table balances(get_self(), get_self().value);
    auto b_itr = balances.find(owner.value);

    //NOTE: no balance to migrate
    if (b_itr == balances.end()) {
        return;
    }

    asset amount_to_migrate = b_itr->tokens;
    auto sym_code_raw = symbol("VIITA", 4).code().raw();

    //NOTE: balance is 0 or less, no need to migrate
    if (amount_to_migrate <= asset(0, symbol("VIITA", 4))) {
        return;
    }

    accounts acnts(get_self(), owner.value);
    auto it = acnts.find(sym_code_raw);

    //NOTE: account will always be there bc being called right after open
    acnts.modify(it, same_payer, [&]( auto& a ) {
        a.balance += amount_to_migrate;
    });

    balances.erase(b_itr);
}


void registry::createwallet(name recipient) {
    require_auth(recipient);
    balances_table balances(config.publisher, config.publisher.value);
    auto itr = balances.find(recipient.value);
    eosio_assert(itr == balances.end(), "Account already owns a wallet");
    balances.emplace(recipient, [&]( auto& a ){
        a.owner = recipient;
        a.tokens = asset(int64_t(0), symbol("VIITA", 4));
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

EOSIO_DISPATCH(registry, (create)(issue)(retire)(transfer)(open)(close)
    (allot)(unallot)(claimallot)
    (createwallet)(deletewallet))