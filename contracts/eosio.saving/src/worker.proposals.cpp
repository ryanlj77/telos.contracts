#include <worker.proposals.hpp>

// #include <eosio/print.hpp>

wps::wps(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

wps::~wps() {}

//======================== wps actions ========================

ACTION wps::updatewps(string version) {
	//authenticate
    require_auth(get_self());

	//initialize
	config new_config;

	//open wpsconfig singleton
    wpsconfig_singleton wpsconfigs(get_self(), get_self().value);

	if (wpsconfigs.exists()) {
		//get wpsconfig
		new_config = wpsconfigs.get();
		
		//update wps_version
		new_configs.wps_version = version;
    } else {
		//initialize
		map<name, double> default_thresholds;
		default_thresholds[name("passvoters")] = double(5.0);
		default_thresholds[name("passvotes")] = double(50.0);
		default_thresholds[name("refundvoters")] = double(4.0);
		default_thresholds[name("refundvotes")] = double(20.0);

		//set defaults
		new_config = {
			"2.0.0", //wps_version
			name("eosio.saving"), //manager
			uint32_t(2500000), //cycle_duration
			asset(50, TLOS_SYM),
			default_thresholds //thresholds
		};
    }

    //set new config
    configs.set(new_config, get_self());
}

ACTION wps::newproposal(name proposal_name, name category, string title, string description, string ipfs_cid, 
		name proposer, name recipient, asset total_funds_requested, optional<uint8_t> cycles) {

	//authenticate
	require_auth(proposer);

	//open proposals table, search for existing proposal
	proposals_table proposals(get_self(), get_self().value);
	auto p_itr = proposals.find(proposal_name.value);

	//validate
	check(p_itr == proposals.end(), "proposal already exists");
	check(valid_category(category), "invalid category");
	check(valid_ipfs_cid(ipfs_cid), "invalid ipfs cid");
	check(is_account(recipient), "recipient account does not exist");
	check(total_funds_requested.amount > 0, "must request more than zero funds");
	check(total_funds_requested.symbol == TLOS_SYM, "can only request TLOS funds");

	//initialize
	uint8_t total_cycles = 1;
	asset total_fee = asset(0, TLOS_SYM);
	map<name, string> initial_deliverables;
	vector<name> initial_options = {"yes"_n, "no"_n, "abstain"_n};

	if (cycles) {
		total_cycles = *cycles;
	}

	//emplace new proposal
	proposals.emplace(get_self(), [&](auto& col) {
		col.proposal_name = proposal_name;
		col.proposer = proposer;
		col.recipient = recipient;
		col.cycles = total_cycles;
		col.fee = total_fee;
		col.approved = false;
		col.refund = false;
		col.refunded = false;
		col.current_cycle = 0;
		col.current_ballot_name = proposal_name;
		col.total_funds_requested = total_funds_requested;
		col.deliverables = initial_deliverables;
	});

	//send inline newballot to trailservice
	action(permission_level{get_self(), "active"_n }, "trailservice"_n, "newballot"_n, make_tuple(
        proposal_name, //ballot_name
		name("proposal"), //category
		get_self(), //publisher
		VOTE_SYM, //treasury_symbol
		name("1tokennvote"), //voting_method
		initial_options //initial_options
    )).send();

	//send inline editdetails to trailservice
	action(permission_level{get_self(), "active"_n }, "trailservice"_n, "editdetails"_n, make_tuple(
        proposal_name, //ballot_name
		title, //title
		description, //description
		ipfs_cid, //content
    )).send();

}

ACTION wps::readyprop(name proposal_name) {

	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name, "proposal not found");

	//authenticate
	require_auth(prop.proposer);

	//validate
	check(prop.current_cycle == 0, "proposal already started");

	//initialize
	time_point_sec cycle_end_time = time_point_sec(current_time_point()) + uint32_t(2592000); //now + 30 days

	//send inline readyballot to trailservice
	action(permission_level{get_self(), "active"_n }, "trailservice"_n, "readyballot"_n, make_tuple(
        proposal_name, //ballot_name
		cycle_end_time //end_time
    )).send();

}

ACTION wps::endcycle(name proposal_name) {

	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

	//authenticate
	require_auth(prop.proposer);

	//validate
	check(!prop.approved, "cycle was already approved and ended");

	//send inline closeballot to trailservice
	action(permission_level{get_self(), "active"_n }, "trailservice"_n, "closeballot"_n, make_tuple(
        prop.current_ballot_name, //ballot_name
		true //broadcast
    )).send();

	//NOTE: final vote tally will be determined in catch_broadcast()

	

}

ACTION wps::claimfunds(name proposal_name, name claimant) {
	//TODO: implement

	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

}

ACTION wps::nextcycle(name proposal_name, string deliverable_report, name next_cycle_name) {

	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

	//authenticate
	require_auth(prop.proposer);

	//validate
	check(prop.approved, "next cycle is not approved");
	check(prop.current_cycle < prop.cycles, "proposal has no cycles left");

	//initialize
	auto new_deliverables = prop.deliverables;
	new_deliverables[next_cycle_name] = deliverable_report;

	//update proposal
	proposals.modify(prop, same_payer, [&](auto& col) {
		col.current_cycle += 1;
		col.current_ballot_name = next_cycle_name;
		col.deliverables = new_deliverables;
	});

	//TODO: send inline newballot to trail

}

ACTION wps::cancelprop(name proposal_name, string memo) {

	//open proposals table, get proposal
	proposals_table propsals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

	//authenticate
	require_auth(prop.proposer);

	//validate
	check(!prop.approved, "cannot cancel an approved proposal");

	//send inline cancelballot action to trailservice
	action(permission_level{get_self(), "active"_n }, "trailservice"_n, "cancelballot"_n, make_tuple(
        prop.current_ballot_name, //ballot_name
		memo //memo
    )).send();

}

ACTION wps::deleteprop(name proposal_name) {
	//TODO: implement
}

ACTION wps::getrefund(name proposal_name) {

	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

	//authenticate
	require_auth(prop.proposer);

	//validate
	check(prop.refund, "refund not approved");
	check(!prop.refunded, "refund already paid");

	//open deposits table, search for deposit
	deposits_table deposits(get_self(), get_self().value);
	auto d_itr = deposits.find(prop.proposer.value);

	if (d_itr == deposits.end()) {
		//emplace new deposit
		deposits.emplace(get_self(), [&](auto& col) {
			col.owner = prop.proposer;
			col.escrow = prop.fee;
		});
	} else {
		//update existing balance
		deposits.modify(d_itr, same_payer, [&](auto& col) {
			col.escrow += prop.fee;
		});
	}

	//update proposal
	proposals.modify(prop, same_payer, [&](auto& col) {
		col.refunded = true;
	});

}

ACTION wps::withdraw(name account_owner, asset quantity) {

	//authenticate
	require_auth(account_owner);

	//open deposits table, get deposit
	deposits_table deposits(get_self(), get_self().value);
	auto& dep = deposits.get(accont_owner.value, "deposit not found");

	//validate
	check(quantity.amount > 0, "must withdraw quantity greater than zero");
	check(dep.escrow >= quantity, "insufficient funds");

	//send inline to eosio.token
	action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
		get_self(), //from
		dep.owner, //to
		quantity, //quantity
		std::string("wps withdrawal") //memo
	)).send();

	//erase deposit row if empty
	if (dep.escrow == quantity) {
		deposits.erase(dep);
	}

}

//========== notification methods ==========

void wps::catch_transfer(name from, name to, asset quantity, string memo) {
	
	//authenticate
	require_auth(from);

	//validate
	check(quantity.symbol == symbol("TLOS", 4), "WPS only accepts TLOS deposits");

	//skips transfers from self or when memo says "skip"
	if (to != get_self() || memo == "skip") {
		return;
	}

	//open deposits table, search for deposit
	deposits_table deposits(get_self(), get_self().value);
	auto d_itr = deposits.find(from.value);

	if (d_itr == deposits.end()) {
		//emplace new deposit
		deposits.emplace(get_self(), [&](auto& col) {
			col.owner = from;
			col.escrow = quantity;
		});
	} else {
		//update existing balance
		deposits.modify(d_itr, same_payer, [&](auto& col) {
			col.escrow += quantity;
		});
	}

}

void wps::catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters) {
	//TODO: implement


}

//======================== legacy actions ========================

ACTION wps::openvoting(uint64_t sub_id) {
	print("\n look for sub id");
	submissions_table submissions(_self, _self.value);
	auto s = submissions.get(sub_id, "Submission not found");

	require_auth(s.proposer);

	print("\n getting ballot");
	ballots_table ballots("eosio.trail"_n, "eosio.trail"_n.value);
	auto b = ballots.get(s.ballot_id, "Ballot not found on eosio.trail ballots_table");

	print("\n getting proposal");
	proposals_table proposals("eosio.trail"_n, "eosio.trail"_n.value);
	auto p = proposals.get(b.reference_id, "Prosal not found on eosio.trail proposals_table");

	check(p.cycle_count == uint16_t(0), "proposal is no longer in building stage");
    check(p.status == uint8_t(0), "Proposal is already closed");

	uint32_t begin_time = current_time_point().sec_since_epoch();
	uint32_t end_time = current_time_point().sec_since_epoch() + wp_env_struct.cycle_duration;
	print("\n sending inline trx");
    action(permission_level{ _self, "active"_n }, "eosio.trail"_n, "nextcycle"_n, make_tuple(
        _self,
        s.ballot_id,
        begin_time,
        end_time
    )).send();
	print("\n after inline trx");

    print("\nReady Proposal: SUCCESS");
}

ACTION wps::cancelsub(uint64_t sub_id) {
	submissions_table submissions(_self, _self.value);
    auto s_itr = submissions.find(sub_id);
    check(s_itr != submissions.end(), "Submission not found");
	auto s = *s_itr;

	require_auth(s.proposer);
	ballots_table ballots("eosio.trail"_n, "eosio.trail"_n.value);
	auto b = ballots.get(s.ballot_id, "Ballot not found on eosio.trail ballots_table");

	proposals_table proposals("eosio.trail"_n, "eosio.trail"_n.value);
	auto p = proposals.get(b.reference_id, "Prosal not found on eosio.trail proposals_table");

	check(p.cycle_count == uint16_t(0), "proposal is no longer in building stage");
    check(p.status == uint8_t(0), "Proposal is already closed");

	action(permission_level{ _self, "active"_n }, "eosio.trail"_n, "unregballot"_n, make_tuple(
        _self,
		s.ballot_id
    )).send();

	submissions.erase(s_itr);
}

ACTION wps::claim(uint64_t sub_id) {
    submissions_table submissions(_self, _self.value);
    const auto& sub = submissions.get(sub_id, "Worker Proposal Not Found");

	require_auth(sub.proposer);

    ballots_table ballots("eosio.trail"_n, "eosio.trail"_n.value);
    const auto& bal = ballots.get(sub.ballot_id, "Ballot ID doesn't exist");
	
	proposals_table props_table("eosio.trail"_n, "eosio.trail"_n.value);
	const auto& prop = props_table.get(bal.reference_id, "Proposal Not Found");

    check(prop.end_time < current_time_point().sec_since_epoch(), "Cycle is still open");
    check(prop.status == uint8_t(0), "Proposal is closed");

    registries_table registries("eosio.trail"_n, "eosio.trail"_n.value);
    auto e = registries.find(symbol("VOTE", 4).code().raw());

    asset outstanding = asset(0, symbol("TLOS", 4));
    asset total_votes = prop.yes_count + prop.no_count + prop.abstain_count; //total votes cast on proposal
    asset non_abstain_votes = prop.yes_count + prop.no_count; 

    //pass thresholds
    asset quorum = e->supply * wp_env_struct.threshold_pass_voters / 100;
    asset votes_pass_thresh = non_abstain_votes * wp_env_struct.threshold_pass_votes / 100;

    //fee refund thresholds
    asset voters_fee_thresh = e->supply * wp_env_struct.threshold_fee_voters / 100; 
    asset votes_fee_thresh = total_votes * wp_env_struct.threshold_fee_votes / 100; 

    auto updated_fee = sub.fee;

    // print("\n GET FEE BACK WHEN <<<< ", prop.yes_count, " >= ", votes_fee_thresh," && ", total_votes, " >= ", voters_fee_thresh);
    if(sub.fee && prop.yes_count.amount > 0 && prop.yes_count >= votes_fee_thresh && total_votes >= voters_fee_thresh) {
        asset the_fee = asset(int64_t(sub.fee), symbol("TLOS", 4));
        if(sub.receiver == sub.proposer) {
            outstanding += the_fee;
        } else {
            action(permission_level{ _self, "active"_n }, "eosio.token"_n, "transfer"_n, make_tuple(
                _self,
                sub.proposer,
                the_fee,
                std::string("Worker proposal funds")
            )).send();
        }
        updated_fee = 0;
    }

    // print("\n GET MUNI WHEN <<<< ", prop.yes_count, " > ", votes_pass_thresh, " && ", total_votes, " >= ", voters_pass_thresh);
    if(prop.yes_count > votes_pass_thresh && total_votes >= quorum) {
        outstanding += asset(int64_t(sub.amount), symbol("TLOS", 4));
    }
    
    // print("\n numbers : ", total_votes, " * ", wp_env_struct.threshold_pass_voters, " | ", wp_env_struct.threshold_fee_voters, " ---- ", total_votes, " ", prop.yes_count, " ", prop.no_count);
    if(outstanding.amount > 0) {
        action(permission_level{ _self, "active"_n }, "eosio.token"_n, "transfer"_n, make_tuple(
            _self,
            sub.receiver,
            outstanding,
            std::string("Worker proposal funds") //TODO: improve memo messaging
        )).send();
    }

    if(prop.cycle_count == uint16_t(sub.cycles - 1)) { //Close ballot because it was the last cycle for the submission.
        uint8_t new_status = 1;
		action(permission_level{ _self, "active"_n }, "eosio.trail"_n, "closeballot"_n, make_tuple(
			_self,
			sub.ballot_id,
			new_status
		)).send();
    } else if(prop.cycle_count < uint16_t(sub.cycles - 1)) { //Start next cycle because number of cycles hasn't been reached.
		uint32_t begin_time = current_time_point().sec_since_epoch();
		uint32_t end_time = current_time_point().sec_since_epoch() + wp_env_struct.cycle_duration;
		action(permission_level{ _self, "active"_n }, "eosio.trail"_n, "nextcycle"_n, make_tuple(
			_self,
			sub.ballot_id,
			begin_time,
			end_time
		)).send();
	}

    submissions.modify(sub, same_payer, [&]( auto& a ) {
        a.fee = updated_fee;
    });
}
