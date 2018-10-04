/**
 * @file
 * @copyright defined in telos/LICENSE.txt
 * 
 * @brief TIP_5 Single Token Registry Interface
 * @author Craig Branscom
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/singleton.hpp>

using namespace eosio;
using namespace std;

class registry : public contract {

    public:
        registry(account_name self);

        ~registry();

        // ABI Actions
        void mint(account_name recipient, asset tokens);

        void transfer(account_name owner, account_name recipient, asset tokens);

        void allot(account_name owner, account_name recipient, asset tokens);

        void reclaim(account_name owner, account_name recipient, asset tokens);

        void transferfrom(account_name owner, account_name recipient, asset tokens);

        void createwallet(account_name owner);

        void deletewallet(account_name owner);

    protected:

        void sub_balance(account_name owner, asset tokens);

        void add_balance(account_name recipient, asset tokens, account_name payer);

        void sub_allot(account_name owner, account_name recipient, asset tokens);

        void add_allot(account_name owner, account_name recipient, asset tokens, account_name payer);
        
        //@abi table settings i64
        struct setting {
            account_name issuer;
            asset max_supply;
            asset supply;
            string name;
            bool is_initialized;

            uint64_t primary_key() const { return issuer; }
            EOSLIB_SERIALIZE(setting, (issuer)(max_supply)(supply)(name)(is_initialized))
        };
        
        //@abi table balances i64
        struct balance {
            account_name owner;
            asset tokens;

            uint64_t primary_key() const { return owner; }
            EOSLIB_SERIALIZE(balance, (owner)(tokens))
        };

        //@abi table allotments i64
        struct allotment {
            account_name recipient;
            account_name owner;
            asset tokens;

            uint64_t primary_key() const { return recipient; }
            EOSLIB_SERIALIZE(allotment, (recipient)(owner)(tokens))
        };

        typedef multi_index< N(balances), balance> balances_table;
        typedef multi_index< N(allotments), allotment> allotments_table;

        typedef eosio::singleton<N(settings), setting> settings_table;
        settings_table settings;
        setting _settings;
};