/**
 * This contract defines the TIP-5 Single Token Interface.
 * 
 * For a full developer walkthrough, see the README.md
 * 
 * @author Craig Branscom
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/singleton.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract("token.registry")]] registry : public contract {

public:

    using contract::contract;

    registry(name self, name code, datastream<const char*> ds);

    ~registry();

    //string const TOKEN_NAME = "Viita";
    //symbol const TOKEN_SYM = symbol("VIITA", 4);
    //asset const INITIAL_MAX_SUPPLY = asset(int64_t(100000000000000), TOKEN_SYM); //10 billion
    //asset const INITIAL_SUPPLY = asset(int64_t(0), TOKEN_SYM);

    [[eosio::action]]
    void create(name issuer, asset  maximum_supply);

    [[eosio::action]]
    void issue(name to, asset quantity, string memo);

    [[eosio::action]]
    void retire(asset quantity, string memo);

    [[eosio::action]]
    void transfer(name from, name to, asset quantity, string memo);

    [[eosio::action]]
    void open(name owner, const symbol& symbol, name ram_payer);

    [[eosio::action]]
    void close(name owner, const symbol& symbol);



    [[eosio::action]]
    void allot(name sender, name recipient, asset tokens);

    [[eosio::action]]
    void unallot(name sender, name recipient, asset tokens);

    [[eosio::action]]
    void claimallot(name sender, name recipient, asset tokens);

    //@scope sender.value (discoverable by sender)
    struct [[eosio::table]] allotment {
        name recipient;
        name sender;
        asset tokens;

        uint64_t primary_key() const { return recipient.value; }
        EOSLIB_SERIALIZE(allotment, (recipient)(sender)(tokens))
    };

    typedef multi_index<name("allotments"), allotment> allotments_table;

    static asset get_supply(name token_contract_account, symbol_code sym_code) {
        stats statstable( token_contract_account, sym_code.raw() );
        const auto& st = statstable.get( sym_code.raw() );
        return st.supply;
    }

    static asset get_balance(name token_contract_account, name owner, symbol_code sym_code) {
        accounts accountstable( token_contract_account, owner.value );
        const auto& ac = accountstable.get( sym_code.raw() );
        return ac.balance;
    }

    //TODO: get_allot()

    //Phase out after migration is done >>>

    struct [[eosio::table]] tokenconfig {
        name publisher;
        string token_name;
        asset max_supply;
        asset supply;

        uint64_t primary_key() const { return publisher.value; }
        EOSLIB_SERIALIZE(tokenconfig, (publisher)(token_name)(max_supply)(supply))
    };

    struct [[eosio::table]] balance {
        name owner;
        asset tokens;

        uint64_t primary_key() const { return owner.value; }
        EOSLIB_SERIALIZE(balance, (owner)(tokens))
    };

    [[eosio::action]] void createwallet(name recipient);

    [[eosio::action]] void deletewallet(name wallet_owner);

    void migrate_balance(name owner);

    typedef eosio::singleton<name("tokenconfig"), tokenconfig> config_singleton;
    config_singleton _config;
    tokenconfig config;

    typedef multi_index<name("balances"), balance> balances_table;

    //<<<

private:
    struct [[eosio::table]] account {
        asset    balance;

        uint64_t primary_key()const { return balance.symbol.code().raw(); }
    };

        struct [[eosio::table]] currency_stats {
        asset    supply;
        asset    max_supply;
        name     issuer;

        uint64_t primary_key()const { return supply.symbol.code().raw(); }
    };

    typedef eosio::multi_index< name("accounts"), account> accounts;
    typedef eosio::multi_index< name("stat"), currency_stats> stats;

    void sub_balance(name owner, asset value);
    void add_balance(name owner, asset value, name ram_payer);

    void add_allot(name sender, name recipient, asset tokens);
    void sub_allot(name sender, name recipient, asset tokens);
};