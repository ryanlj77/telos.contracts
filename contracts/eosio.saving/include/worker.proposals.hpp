// Telos Worker Proposal System
//
// @author Craig Branscom, Peter Bue
// @contract wps
// @version v2.0.0-RFC1
// @copyright see LICENSE.txt

#include <trail.voting.hpp>
#include <trail.tokens.hpp>
#include <trail.system.hpp>

#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/action.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;
using namespace std;

class[[eosio::contract("eosio.saving")]] wps : public contract {

	public:

	wps(name self, name code, datastream<const char *> ds);

	~wps();

	//constants
	const asset TLOS_SYM = symbol("TLOS", 4);
	const asset VOTE_SYM = symbol("VOTE", 4);

	//======================== wps actions ========================

	//update the wps contract version
	ACTION updatewps(string version);

	ACTION newproposal(name proposal_name, name category, string title, string description, string ipfs_cid, 
		name proposer, name recipient, asset total_funds_requested, optional<uint8_t> cycles);

	ACTION readyprop(name proposal_name);

	ACTION endcycle(name proposal_name);

	ACTION claimfunds(name proposal_name, name claimant);

	ACTION nextcycle(name proposal_name, string deliverable_report, name next_cycle_name);

	ACTION cancelprop(name proposal_name, string memo);

	ACTION deleteprop(name proposal_name);

	ACTION getrefund(name proposal_name);

	ACTION withdraw(name account_owner, asset quantity);

	//======================== legacy actions ========================

	ACTION openvoting(uint64_t sub_id);

	ACTION cancelsub(uint64_t sub_id);

	ACTION claim(uint64_t sub_id);

	//========== migration actions ==========

	ACTION cleantable(name uint64_t sub_id);

	//========== notification methods ==========

	[[eosio::on_notify("eosio.token::transfer")]]
	void catch_transfer(name from, name to, asset quantity, string memo);

	[[eosio::on_notify("trailservice::broadcast")]]
	void catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters);

	//========== utility methods ==========

	bool valid_category(name category);

	bool valid_ipfs_cid(string ipfs_cid);

	//======================== wps tables ========================

	//scope: singleton
	//ram:
	TABLE wps_config {
		string wps_version;
		name wps_manager;
		uint32_t cycle_duration;
		asset fee_min;
		map<name, double> thresholds;
		
		EOSLIB_SERIALIZE(wpsconfig, (wps_version)(wps_mananger)
			(cycle_duration)(fee_min)(thresholds))
	}
	typedef singleton<name("wpsconfig"), wps_config> wpsconfig_singleton;

	//scope: get_self().value
	//ram: 
	TABLE proposal {
		name proposal_name;
		name proposer;
		name recipient;
		uint8_t cycles;
		asset fee;
		bool approved;
		bool refund;
		bool refunded;
		uint8_t current_cycle;
		name current_ballot_name;
		asset total_funds_requested;
		map<name, string> deliverables; //ballot_name => deliverable_report

		uint64_t primary_key() const { return proposal_name.value; }
		EOSLIB_SERIALIZE(proposal, (proposal_name)(proposer)(recipient)
			(cycles)(fee)(approved)(refund)(refunded)(current_cycle)
			(current_ballot_name)(total_funds_requested)(deliverables))
	};
	typedef multi_index<name("proposals"), proposal> proposals_table;

	//scope: get_self().value
	//ram: 
	TABLE deposit {
		name owner;
		asset escrow;

		uint64_t primary_key() const { return owner.value; }
		EOSLIB_SERIALIZE(deposit, (owner)(escrow))
	};
	typedef eosio::multi_index<name("deposits"), deposit> deposits_table;

	//======================== legacy tables ========================

	TABLE submission {
		uint64_t id;
		uint64_t ballot_id;
		name proposer;
		name receiver;
		std::string title;
		std::string ipfs_location;
		uint16_t cycles;
		uint64_t amount;
		uint64_t fee;

		auto primary_key() const { return id; }
		EOSLIB_SERIALIZE(submission, (id)(ballot_id)(proposer)(receiver)(title)(ipfs_location)(cycles)(amount)(fee))
	};
	typedef eosio::multi_index<"submissions"_n, submission> submissions_table;

	struct [[ eosio::table("wpenv"), eosio::contract("eosio.saving") ]] wp_env {
		name publisher;
		uint32_t cycle_duration;
		uint16_t fee_percentage;
		uint64_t fee_min;
		uint32_t start_delay;
		double threshold_pass_voters;
		double threshold_pass_votes;
		double threshold_fee_voters;
		double threshold_fee_votes;

		uint64_t primary_key() const { return publisher.value; }
		EOSLIB_SERIALIZE(wp_env, (publisher)(cycle_duration)(fee_percentage)(fee_min)(start_delay)(threshold_pass_voters)(threshold_pass_votes)(threshold_fee_voters)(threshold_fee_votes))
	};
	typedef singleton<name("wpenv"), wp_env> wp_environment_singleton;
	
};
