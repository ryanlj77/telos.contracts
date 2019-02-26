#include "eosio.trail_tester.hpp"

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

class eosio_arb_tester : public eosio_trail_tester {
public:

    abi_serializer abi_ser;

    eosio_arb_tester() {
        deploy_contract();
        produce_blocks(1);
    }

    void deploy_contract() {
        create_accounts({N(eosio.arb)});

        set_code(N(eosio.arb), contracts::eosio_arb_wasm());
        set_abi(N(eosio.arb), contracts::eosio_arb_abi().data());
        {
            const auto &accnt = control->db().get<account_object, by_name>(N(eosio.arb));
            abi_def abi;
            BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
            abi_ser.set_abi(abi, abi_serializer_max_time);
        }
    }

#pragma region get_tables

    fc::variant get_config() {
        vector<char> data = get_row_by_account(N(eosio.arb), N(eosio.arb), N(config), N(config));
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("config", data, abi_serializer_max_time);
    }

    fc::variant get_nominee(uint64_t nominee_id) {
        vector<char> data = get_row_by_account(N(eosio.arb), N(eosio.arb), N(nominees), nominee_id);
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("nominee", data, abi_serializer_max_time);
    }

    fc::variant get_arbitrator(uint64_t arbitrator_id) {
        vector<char> data = get_row_by_account(N(eosio.arb), N(eosio.arb), N(arbitrators), arbitrator_id);
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("arbitrator", data, abi_serializer_max_time);
    }

    fc::variant get_casefile(uint64_t casefile_id) {
        vector<char> data = get_row_by_account(N(eosio.arb), N(eosio.arb), N(casefiles), casefile_id);
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("casefile", data, abi_serializer_max_time);
    }

    fc::variant get_unread_claim(uint64_t casefile_id, uint8_t claim_id) {
        auto cf = get_casefile(casefile_id);

        BOOST_REQUIRE_EQUAL(false, cf.is_null());
        auto unread_claims = cf["unread_claims"].as < vector < mvo > > ();
        return unread_claims[claim_id];
        //vector<char> data = get_row_by_accounts(N(eosio.arb),N(eosio.arb), N(unread_claims), casefile_id);
        //return data.empty()? fc::variant() : abi_ser.binary_to_variant("unread_claim", data, abi_serializer_max_time);
    }

    fc::variant get_unread_claim(uint64_t casefile_id, string claim_link) {
        auto cf = get_casefile(casefile_id);

        BOOST_REQUIRE_EQUAL(false, cf.is_null());
        auto unread_claims = cf["unread_claims"].as < vector < mvo > > ();

        auto it = find_if(unread_claims.begin(), unread_claims.end(), [&](auto c) {
            return claim_link == c["claim_summary"];
        });
        return it == unread_claims.end() ? fc::variant() : *it;

    }

    fc::variant get_claim(uint64_t claim_id) {
        vector<char> data = get_row_by_account(N(eosio.arb), N(eosio.arb), N(claims), claim_id);
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("claim", data, abi_serializer_max_time);
    }



    #pragma endregion get_tables


    #pragma region actions

    transaction_trace_ptr setconfig(uint16_t max_elected_arbs, uint32_t election_duration, uint32_t start_election, uint32_t arbitrator_term_length, vector<int64_t> fees) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(setconfig), vector<permission_level>{{N(eosio), config::active_name}}, mvo()
            ("max_elected_arbs", max_elected_arbs)
            ("election_duration", election_duration)
            ("start_election", start_election)
            ("arbitrator_term_length", arbitrator_term_length)
            ("fees", fees))
        );

        set_transaction_headers(trx);
        trx.sign(get_private_key(N(eosio), "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr init_election() {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(initelection), vector<permission_level>{{N(eosio), config::active_name}}, mvo()));
        set_transaction_headers(trx);
        trx.sign(get_private_key(N(eosio), "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr regarb(name nominee, string credentials_link) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(regarb), vector<permission_level>{{nominee, config::active_name}}, mvo()
            ("nominee", nominee)
            ("credentials_link", credentials_link)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(nominee, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr unregnominee(name nominee) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(unregnominee), vector<permission_level>{{nominee, config::active_name}}, mvo()
            ("nominee", nominee)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(nominee, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr candaddlead(name nominee, string credentials_link) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(candaddlead), vector<permission_level>{{nominee, config::active_name}}, mvo()
            ("nominee", nominee)
            ("credentials_link", credentials_link)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(nominee, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr candrmvlead(name nominee) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(candrmvlead), vector<permission_level>{{nominee, config::active_name}}, mvo()
            ("nominee", nominee)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(nominee, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr endelection(name nominee) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(endelection), vector<permission_level>{{nominee, config::active_name}}, mvo()
            ("nominee", nominee)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(nominee, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    //note: case setup

    transaction_trace_ptr filecase(name claimant, string claim_link, vector <uint8_t> lang_codes, optional<name> respondant ) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(filecase), vector<permission_level>{{claimant, config::active_name}}, mvo()
            ("claimant", claimant)
            ("claim_link", claim_link)
            ("lang_codes", lang_codes)
            ("respondant", respondant)
            ));
        set_transaction_headers(trx);
        trx.sign(get_private_key(claimant, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr addclaim(uint64_t case_id, string claim_link, name claimant) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(addclaim), vector<permission_level>{{claimant, config::active_name}}, mvo()
            ("case_id", case_id)
            ("claim_link", claim_link)
            ("claimant", claimant)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(claimant, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr removeclaim(uint64_t case_id, string claim_hash, name claimant) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(removeclaim), vector<permission_level>{{claimant, config::active_name}}, mvo()
            ("case_id", case_id)
            ("claim_hash", claim_hash)
            ("claimant", claimant)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(claimant, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr shredcase(uint64_t case_id, name claimant) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(shredcase), vector<permission_level>{{claimant, config::active_name}}, mvo()
            ("case_id", case_id)
            ("claimant", claimant)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(claimant, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr readycase(uint64_t case_id, name claimant) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(readycase), vector<permission_level>{{claimant, config::active_name}}, mvo()
            ("case_id", case_id)
            ("claimant", claimant)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(claimant, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    //note: case progression

    transaction_trace_ptr assigntocase(uint64_t case_id, name arb_to_assign) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(assigntocase), vector<permission_level>{{arb_to_assign, config::active_name}}, mvo()
            ("case_id", case_id)
            ("arb_to_assign", arb_to_assign)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(arb_to_assign, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr dismissclaim(uint64_t case_id, name assigned_arb, string claim_hash, string memo) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(dismissclaim), vector<permission_level>{{assigned_arb, config::active_name}}, mvo()
            ("case_id", case_id)
            ("assigned_arb", assigned_arb)
            ("claim_hash", claim_hash)
            ("memo", memo)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(assigned_arb, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr acceptclaim(uint64_t case_id, name assigned_arb, string claim_hash, string decision_link, uint8_t decision_class) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(acceptclaim), vector<permission_level>{{assigned_arb, config::active_name}}, mvo()
            ("case_id", case_id)
            ("assigned_arb", assigned_arb)
            ("claim_hash", claim_hash)
            ("decision_link", decision_link)
            ("decision_class", decision_class)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(assigned_arb, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr dismisscase(uint64_t case_id, name assigned_arb, string ruling_link) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(dismisscase), vector<permission_level>{{assigned_arb, config::active_name}}, mvo()
            ("case_id", case_id)
            ("assigned_arb", assigned_arb)
            ("ruling_link", ruling_link)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(assigned_arb, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr advancecase(uint64_t case_id, name assigned_arb) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(advancecase), vector<permission_level>{{assigned_arb, config::active_name}}, mvo()
            ("case_id", case_id)
            ("assigned_arb", assigned_arb)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(assigned_arb, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr newcfstatus(uint64_t case_id, uint16_t new_status, name assigned_arb) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(newcfstatus), vector<permission_level>{{assigned_arb, config::active_name}}, mvo()
            ("case_id", case_id)
            ("new_status", new_status)
            ("assigned_arb", assigned_arb)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(assigned_arb, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr recuse(uint64_t case_id, string rationale, name assigned_arb) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(recuse), vector<permission_level>{{assigned_arb, config::active_name}}, mvo()
            ("case_id", case_id)
            ("rationale", rationale)
            ("assigned_arb", assigned_arb)));
        set_transaction_headers(trx);
        trx.sign(get_private_key(assigned_arb, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    // ! double check permissions on this !
    transaction_trace_ptr dismissarb(name arb) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(dismissarb), vector<permission_level>{{arb, config::active_name}},
                                            mvo()("arb", arb) ));
        set_transaction_headers(trx);
        trx.sign(get_private_key(arb, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

	transaction_trace_ptr fix() {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(fix), vector<permission_level>{{N(eosio.arb), config::active_name}},
                                            mvo() ));
        set_transaction_headers(trx);
        trx.sign(get_private_key(N(eosio.arb), "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    #pragma endregion actions


    #pragma region enums


    enum case_state : uint8_t
    {
        CASE_SETUP,			// 0
        AWAITING_ARBS,		// 1
        CASE_INVESTIGATION, // 2
        HEARING,			// 3
        DELIBERATION,		// 4
        DECISION,			// 5 NOTE: No more joinders allowed
        ENFORCEMENT,		// 6
        RESOLVED,			// 7
        DISMISSED			// 8 NOTE: Dismissed cases advance and stop here
    };

    enum claim_class : uint8_t
    {
        UNDECIDED			= 1,
        LOST_KEY_RECOVERY	= 2,
        TRX_REVERSAL		= 3,
        EMERGENCY_INTER		= 4,
        CONTESTED_OWNER		= 5,
        UNEXECUTED_RELIEF	= 6,
        CONTRACT_BREACH		= 7,
        MISUSED_CR_IP		= 8,
        A_TORT				= 9,
        BP_PENALTY_REVERSAL	= 10,
        WRONGFUL_ARB_ACT 	= 11,
        ACT_EXEC_RELIEF		= 12,
        WP_PROJ_FAILURE		= 13,
        TBNOA_BREACH		= 14,
        MISC				= 15
    };

    enum arb_status : uint8_t
    {
        AVAILABLE,    // 0
        UNAVAILABLE,  // 1
        REMOVED,	  // 2
        SEAT_EXPIRED  // 3
    };

    enum election_status : uint8_t
    {
        OPEN,   // 0
        PASSED, // 1
        FAILED, // 2
        CLOSED  // 3
    };

    enum lang_code : uint8_t
    {
        ENGL, //0 English
        FRCH, //1 French
        GRMN, //2 German
        KREA, //3 Korean
        JAPN, //4 Japanese
        CHNA, //5 Chinese
        SPAN, //6 Spanish
        PGSE, //7 Portuguese
        SWED  //8 Swedish
    };

    #pragma endregion enums

};