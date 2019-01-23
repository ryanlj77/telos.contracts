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

    #pragma region Get_Tables
    
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

    //TODO: make get_claim

    #pragma endregion Get_Tables


    #pragma region Actions

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

    transaction_trace_ptr regnominee(name nominee, string credentials_link) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(regnominee), vector<permission_level>{{nominee, config::active_name}}, mvo()
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

    //NOTE: Case Setup

    transaction_trace_ptr filecase(name claimant, string claim_link) {
        signed_transaction trx;
        trx.actions.emplace_back(get_action(N(eosio.arb), N(filecase), vector<permission_level>{{claimant, config::active_name}}, mvo()
            ("claimant", claimant)
            ("claim_link", claim_link)));
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
        trx.actions.emplace_back(get_action(N(eosio.arb), N(endelection), vector<permission_level>{{claimant, config::active_name}}, mvo()
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

    //NOTE: Case Progression

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

    #pragma endregion Actions


    #pragma region Enums


    enum case_state : uint8_t {
        CASE_SETUP,         // 0
        AWAITING_ARBS,      // 1
        CASE_INVESTIGATION, // 2
        HEARING,            // 3
        DELIBERATION,       // 4
        DECISION,           // 5 NOTE: No more joinders allowed
        ENFORCEMENT,        // 6
        RESOLVED,           // 7
        DISMISSED           // 8 NOTE: Dismissed cases advance and stop here
    };

    enum claim_class : uint8_t {
        UNDECIDED,           // 0
        LOST_KEY_RECOVERY,   // 1
        TRX_REVERSAL,        // 2
        EMERGENCY_INTER,     // 3
        CONTESTED_OWNER,     // 4
        UNEXECUTED_RELIEF,   // 5
        CONTRACT_BREACH,     // 6
        MISUSED_CR_IP,       // 7
        A_TORT,              // 8
        BP_PENALTY_REVERSAL, // 9
        WRONGFUL_ARB_ACT,    // 10
        ACT_EXEC_RELIEF,     // 11
        WP_PROJ_FAILURE,     // 12
        TBNOA_BREACH,        // 13
        MISC                 // 14
    };

    enum arb_status : uint8_t {
        AVAILABLE,   // 0
        UNAVAILABLE, // 1
        INACTIVE,    // 2
        SEAT_EXPIRED // 3
    };

    enum election_status : uint8_t {
        OPEN,   // 0
        PASSED, // 1
        FAILED, // 2
        CLOSED  // 3
    };

    enum lang_code : uint8_t {
        ENGL, //0 English 
        FRCH, //1 French
        GRMN, //2 German
        KREA, //3 Korean
        JAPN, //4 Japanese
        CHNA, //5 Chinese
        SPAN, //6 Spanish
        PGSE, //7 Portuguese
        SWED //8 Swedish
    };

    #pragma endregion Enums

};