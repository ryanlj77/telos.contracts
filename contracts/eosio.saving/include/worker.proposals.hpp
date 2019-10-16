// Telos Worker Proposal System
//
// @author Craig Branscom, Peter Bue
// @contract wps
// @version v2.0.0-RFC1
// @copyright see LICENSE.txt

// #include <trail.voting.hpp>
// #include <trail.tokens.hpp>
// #include <trail.system.hpp>
#include <legacy.hpp>

#include <trailservice.tables.hpp>

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

	//NOTE: WPS must always keep 30 TLOS of submission fees to cover Trail expenses

	//proposal statuses: draft, open, passed, failed, cancelled

	//constants
	const symbol TLOS_SYM = symbol("TLOS", 4);
	const symbol VOTE_SYM = symbol("VOTE", 4);

	//======================== admin actions ========================

	//update the wps contract version
	ACTION newversion(string new_version);

	//update wps admin
	ACTION newadmin(name new_admin_name);

	//upsert new fee
	ACTION upsertfee(name fee_name, asset new_fee);

	//upsert new threshold
	ACTION upsertthresh(name threshold_name, double new_threshold);

	//======================== proposal actions ========================

	//submits a new proposal in draft mode
	ACTION newproposal(string title, string description, string content, name proposal_name, name category, 
		name proposer, name recipient, asset funds_requested);

	//edits an existing proposal in draft mode
	ACTION editprop(name proposal_name, optional<name> category, optional<name> recipient,
		optional<asset> funds_requested, optional<string> title, optional<string> description, 
		optional<string> content);

	//opens proposal up for voting
	ACTION readyprop(name proposal_name);

	//notifies trail to close ballot and broadcast results
	//final results captured in catch_broadcast() and pass/fail status rendered there
	ACTION closeprop(name proposal_name);

	//claims funds from a passed proposal
	ACTION claimfunds(name proposal_name, name claimant);

	//cancels a proposal in draft or voting mode
	ACTION cancelprop(name proposal_name, string memo);

	//deletes a passed, failed, or cancelled proposal
	ACTION deleteprop(name proposal_name);

	//claims a refund on a proposal, if qualified
	ACTION getrefund(name proposal_name);

	//withdraws a deposit balance back to eosio.token
	ACTION withdraw(name account_owner, asset quantity);

	//======================== legacy actions ========================

	ACTION openvoting(uint64_t sub_id);

	ACTION cancelsub(uint64_t sub_id);

	ACTION claim(uint64_t sub_id);

	//========== migration actions ==========

	// ACTION cleantable(uint64_t sub_id);

	//========== notification methods ==========

	[[eosio::on_notify("eosio.token::transfer")]]
	void catch_transfer(name from, name to, asset quantity, string memo);

	[[eosio::on_notify("trailservice::broadcast")]]
	void catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters);

	//========== utility methods ==========

	void require_fee(name account_owner, asset fee);

	void debit_account(name account_owner, asset quantity);

	bool valid_category(name category);

	//======================== wps tables ========================

	//scope: singleton
	//ram:
	TABLE wps_config {
		string version;
		name admin;
		map<name, asset> fees;
		map<name, double> thresholds;
		
		EOSLIB_SERIALIZE(wps_config, (version)(admin)(fees)(thresholds))
	};
	typedef singleton<name("wpsconfig"), wps_config> wpsconfig_singleton;

	//scope: get_self().value
	//ram: 
	TABLE proposal {
		name proposal_name;
		name category;
		name reserved1 = name(0);
		name proposer;
		name recipient;
		name status; //draft, voting, passed, failed

		string title;
		string description;
		string content;

		asset funds_requested;
		asset fee_collected;

		bool paid;
		bool refunded;
		asset refunded_amount;

		uint64_t primary_key() const { return proposal_name.value; }
		EOSLIB_SERIALIZE(proposal, 
			(proposal_name)(category)(reserved1)(proposer)(recipient)(status)
			(title)(description)(content)
			(funds_requested)(fee_collected)
			(paid)(refunded)(refunded_amount))
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
	typedef eosio::multi_index<"submissions"_n, submission> leg_submissions_table;

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
	typedef singleton<name("wpenv"), wp_env> leg_wp_environment_singleton;
	
};
