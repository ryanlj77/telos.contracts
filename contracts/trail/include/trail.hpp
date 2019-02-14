/**
 * Trail is an EOSIO-based voting service that allows users to create ballots that
 * are voted on by a network of registered voters. It also offers custom token
 * features that let any user to create their own voting token and configure settings 
 * to match a wide variety of intended use cases. 
 * 
 * @author Craig Branscom
 * @copyright
 */

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/dispatcher.hpp>
#include <string>

using namespace eosio;
using namespace std;

class [[eosio::contract("trail")]] trail : public contract {
    
    public:

    trail(name self, name code, datastream<const char*> ds);

    ~trail();

    struct option {
        name option_name;
        string info;
        asset votes;
    };

    TABLE ballot {
        name ballot_name;
        name ballot_type;
        name publisher;

        string title;
        string description;
        string info_url;

        vector<option> candidates;
        uint32_t unique_voters;
        symbol voting_symbol;
        uint8_t available_seats;

        uint32_t begin_time;
        uint32_t end_time;
        uint8_t status;

        uint64_t primary_key() const { return ballot_name.value; }
        EOSLIB_SERIALIZE(ballot, (ballot_name)(publisher)
            (title)(description)(info_url))
    };


    ACTION regballot();

    ACTION addoption();

    ACTION unregballot();


    ACTION castvote(name voter, name ballot_name, name option);

    ACTION cleanupvotes(name voter, uint16_t count);


};
