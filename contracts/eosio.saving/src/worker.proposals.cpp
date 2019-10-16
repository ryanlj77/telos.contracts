#include <worker.proposals.hpp>

wps::wps(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

wps::~wps() {}

//======================== admin actions ========================

ACTION wps::newversion(string new_version) {
	//open wpsconfig singleton
    wpsconfig_singleton wpsconfigs(get_self(), get_self().value);

	//initialize
	wps_config new_config;

	if (wpsconfigs.exists()) {
		//get wpsconfig
		new_config = wpsconfigs.get();

		//authenticate
		require_auth(new_config.admin);
		
		//update wps_version
		new_config.version = new_version;
    } else {

		//authenticate
    	require_auth(get_self());

		//initialize
		map<name, asset> default_fees;
		default_fees["minfee"_n] = asset(500000, TLOS_SYM); //50 TLOS
		default_fees["stepamount"_n] = asset(100000000, TLOS_SYM); //10,000 TLOS

		map<name, double> default_thresholds;
		default_thresholds[name("passquorum")] = double(5.0);
		default_thresholds[name("passvotes")] = double(50.0);
		default_thresholds[name("refundquroum")] = double(4.0);
		default_thresholds[name("refundvotes")] = double(20.0);
		default_thresholds[name("minfeeperc")] = double(5.0);
		default_thresholds[name("steppercent")] = double(0.1);

		//set defaults
		new_config = {
			"v2.0.0", //version
			get_self(), //admin
			default_fees, //fees
			default_thresholds //thresholds
		};

    }

    //set new config
    wpsconfigs.set(new_config, get_self());
}

ACTION wps::newadmin(name new_admin_name) {
	//open wpsconfig singleton, get wps config
    wpsconfig_singleton wpsconfigs(get_self(), get_self().value);
	auto conf = wpsconfigs.get();

	//authenticate
	require_auth(conf.admin);

	//set new admin
	conf.admin = new_admin_name;

	//update singleton
	wpsconfigs.set(conf, get_self());
}

ACTION wps::upsertfee(name fee_name, asset new_fee) {
	//open wpsconfig singleton, get wps config
    wpsconfig_singleton wpsconfigs(get_self(), get_self().value);
	auto conf = wpsconfigs.get();

	//authenticate
	require_auth(conf.admin);

	//validate
	check(new_fee.symbol == TLOS_SYM, "fees must be in TLOS");
	check(new_fee.amount > 0, "fee must be greater than 0");

	//set fee
	conf.fees.at(fee_name) = new_fee;

	//update singleton
	wpsconfigs.set(conf, get_self());
}

ACTION wps::upsertthresh(name threshold_name, double new_threshold) {
	//open wpsconfig singleton, get wps config
    wpsconfig_singleton wpsconfigs(get_self(), get_self().value);
	auto conf = wpsconfigs.get();

	//authenticate
	require_auth(conf.admin);

	//validate
	check(new_threshold > 0, "fee must be greater than 0");

	//set threshold
	conf.thresholds.at(threshold_name) = new_threshold;

	//update singleton
	wpsconfigs.set(conf, get_self());
}

//======================== proposal actions ========================

ACTION wps::newproposal(string title, string description, string content, name proposal_name, name category, 
	name proposer, name recipient, asset funds_requested) {
	//authenticate
	require_auth(proposer);

	//open proposals table, search for existing proposal
	proposals_table proposals(get_self(), get_self().value);
	auto p_itr = proposals.find(proposal_name.value);

	//open wpsconfig singleton, get wps config
    wpsconfig_singleton wpsconfigs(get_self(), get_self().value);
	auto conf = wpsconfigs.get();

	//initialize
	asset total_fee = asset(0, TLOS_SYM);
	vector<name> initial_options = {"yes"_n, "no"_n, "abstain"_n};

	//calculate fee
	double final_fee_percent = conf.thresholds.at("minfeeperc"_n);
	total_fee.amount = uint64_t( double(funds_requested.amount) * final_fee_percent );

	//validate
	check(p_itr == proposals.end(), "proposal already exists");
	check(valid_category(category), "invalid category");
	check(is_account(recipient), "recipient account does not exist");
	check(funds_requested.amount > 0, "must request more than zero funds");
	check(funds_requested.symbol == TLOS_SYM, "can only request funds in TLOS");

	//charge fee
	require_fee(proposer, total_fee);

	//emplace new proposal
	proposals.emplace(get_self(), [&](auto& col) {
		col.proposal_name = proposal_name;
		col.category = category;
		col.proposer = proposer;
		col.recipient = recipient;
		col.status = "draft"_n;
		col.title = title;
		col.description = description;
		col.content = content;
		col.funds_requested = funds_requested;
		col.fee_collected = total_fee;
		col.paid = false;
		col.refunded = false;
		col.refunded_amount = asset(0, TLOS_SYM);
	});

	//send inline newballot to trailservice
	action(permission_level{ get_self(), "active"_n }, "trailservice"_n, "newballot"_n, make_tuple(
        proposal_name, //ballot_name
		name("proposal"), //category
		get_self(), //publisher
		VOTE_SYM, //treasury_symbol
		name("1tokennvote"), //voting_method
		initial_options //initial_options
    )).send();

	//send inline editdetails to trailservice
	// action(permission_level{ get_self(), "active"_n }, "trailservice"_n, "editdetails"_n, make_tuple(
    //     proposal_name, //ballot_name
	// 	title, //title
	// 	description, //description
	// 	content, //content
    // )).send();
}

ACTION wps::editprop(name proposal_name, optional<name> category, optional<name> recipient,
	optional<asset> funds_requested, optional<string> title, optional<string> description, 
	optional<string> content) {
	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

	//authorize
	require_auth(prop.proposer);

	//initialize
	name new_category = prop.category;
	name new_recipient = prop.recipient;
	asset new_funds_requested = prop.funds_requested;
	string new_title = prop.title;
	string new_description = prop.description;
	string new_content = prop.content;

	if (category) {
		new_category = *category;
	}

	if (recipient) {
		new_recipient = *recipient;
	}

	if (funds_requested) {
		new_funds_requested = *funds_requested;
	}

	if (title) {
		new_title = *title;
	}

	if (description) {
		new_description = *description;
	}

	if (content) {
		new_content = *content;
	}

	//validate
	check(prop.status == "draft"_n, "proposal must be in draft mode to edit");
	check(valid_category(new_category), "invalid category");
	check(is_account(new_recipient), "recipient account doesn't exist");
	check(new_funds_requested.amount > 0, "must request greater than 0 funds");
	check(new_funds_requested.symbol == TLOS_SYM, "funding symbol must be TLOS");

	proposals.modify(prop, same_payer, [&](auto& col) {
		col.category = new_category;
		col.recipient = new_recipient;
		col.funds_requested = new_funds_requested;
		col.title = new_title;
		col.description = new_description;
		col.content = new_content;
	});
}

ACTION wps::readyprop(name proposal_name) {
	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

	//authenticate
	require_auth(prop.proposer);

	//validate
	check(prop.status == "draft"_n, "proposal must be in draft mode to ready");

	//initialize
	time_point_sec end_time = time_point_sec(current_time_point()) + uint32_t(2592000); //now + 30 days

	//update wps proposal status
	proposals.modify(prop, same_payer, [&](auto& col) {
		col.status = "voting"_n;
	});

	//send inline readyballot to trailservice
	action(permission_level{get_self(), "active"_n }, "trailservice"_n, "readyballot"_n, make_tuple(
        proposal_name, //ballot_name
		end_time //end_time
    )).send();
}

ACTION wps::closeprop(name proposal_name) {
	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

	//authenticate
	require_auth(prop.proposer);

	//validate
	check(prop.status == "voting"_n, "proposal must be in voting mode to close");

	//send inline closeballot to trailservice
	action(permission_level{get_self(), "active"_n }, "trailservice"_n, "closeballot"_n, make_tuple(
        proposal_name, //ballot_name
		true //broadcast
    )).send();
}

ACTION wps::claimfunds(name proposal_name, name claimant) {
	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

	//authenticate
	require_auth(prop.recipient);

	//validate
	check(prop.status == "passed"_n, "proposal must have passed to claim funds");
	check(prop.paid == false, "recipient has already been paid");

	//update proposal
	proposals.modify(prop, same_payer, [&](auto& col) {
		col.paid = true;
	});

	//debit recipient account with funds
	debit_account(prop.recipient, prop.funds_requested);
}

ACTION wps::cancelprop(name proposal_name, string memo) {
	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

	//authenticate
	require_auth(prop.proposer);

	//validate
	check(prop.status == "draft"_n || 
		prop.status == "voting"_n, 
		"proposal must be in voting or draft mode to cancel");

	//update proposal
	proposals.modify(prop, same_payer, [&](auto& col) {
		col.status = "cancelled"_n;
	});

	//send inline cancelballot action to trailservice
	action(permission_level{get_self(), "active"_n }, "trailservice"_n, "cancelballot"_n, make_tuple(
        prop.proposal_name, //ballot_name
		memo //memo
    )).send();
}

ACTION wps::deleteprop(name proposal_name) {
	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

	//authenticate
	require_auth(prop.proposer);

	//validate
	check(prop.status == "passed"_n ||
		prop.status == "failed"_n ||
		prop.status == "cancelled"_n, 
		"proposal must be either passed, failed, or cancelled to delete");

	//delete proposal from wps
	proposals.erase(prop);

	//send inline deleteballot action to trailservice
	action(permission_level{get_self(), "active"_n }, "trailservice"_n, "deleteballot"_n, make_tuple(
        prop.proposal_name //ballot_name
    )).send();
}

ACTION wps::getrefund(name proposal_name) {
	//open proposals table, get proposal
	proposals_table proposals(get_self(), get_self().value);
	auto& prop = proposals.get(proposal_name.value, "proposal not found");

	//authenticate
	require_auth(prop.proposer);

	//validate
	check(prop.refunded == false, "refund already paid");

	//TODO: check refund qualification

	//open deposits table, search for deposit
	deposits_table deposits(get_self(), get_self().value);
	auto d_itr = deposits.find(prop.proposer.value);

	if (d_itr == deposits.end()) {
		//emplace new deposit
		deposits.emplace(get_self(), [&](auto& col) {
			col.owner = prop.proposer;
			col.escrow = prop.fee_collected;
		});
	} else {
		//update existing balance
		deposits.modify(d_itr, same_payer, [&](auto& col) {
			col.escrow += prop.fee_collected;
		});
	}

	//update proposal
	proposals.modify(prop, same_payer, [&](auto& col) {
		col.refunded = true;
	});

}

ACTION wps::withdraw(name account_owner, asset quantity) {
	//open deposits table, get deposit
	deposits_table deposits(get_self(), get_self().value);
	auto& dep = deposits.get(account_owner.value, "deposit not found");

	//authenticate
	require_auth(dep.owner);

	//validate
	check(quantity.amount > 0, "must withdraw quantity greater than zero");
	check(dep.escrow >= quantity, "insufficient funds");

	//erase deposit row if empty
	if (dep.escrow == quantity) {
		deposits.erase(dep);
	} else {
		deposits.modify(dep, same_payer, [&](auto& col) {
			col.escrow -= quantity;
		});
	}

	//send inline to eosio.token
	action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
		get_self(), //from
		dep.owner, //to
		quantity, //quantity
		std::string("wps withdrawal") //memo
	)).send();
}

//========== notification methods ==========

void wps::catch_transfer(name from, name to, asset quantity, string memo) {
	//get initial receiver contract
    name rec = get_first_receiver();

	//validate
	check(rec == "eosio.token"_n, "eosio.token must be first receiver");
	check(quantity.symbol == symbol("TLOS", 4), "WPS only accepts TLOS deposits");

	//skips transfers from self or when memo says "skip"
	if (from == get_self() || memo == "skip") {
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
		deposits.modify(*d_itr, same_payer, [&](auto& col) {
			col.escrow += quantity;
		});
	}
}

void wps::catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters) {
	//authenticate
	// require_auth("trailservice"_n);

	//open proposals table, search for proposal
	proposals_table proposals(get_self(), get_self().value);
	auto p_itr = proposals.find(ballot_name.value);

	//return if no proposal found
	if (p_itr == proposals.end()) {
		return;
	}

	//open wpsconfig singleton
	wpsconfig_singleton wpsconf(get_self(), get_self().value);
	auto conf = wpsconf.get();

	//cross-contract table lookup
	ts_treasuries_table treasuries("trailservice"_n, name("trailservice").value);
	auto& trs = treasuries.get(VOTE_SYM.code().raw(), "VOTE treasury not found");

	//initialize
	asset pass_thresh_votes = trs.supply * conf.thresholds.at("passvotes"_n);
	asset pass_thresh_quorum = trs.supply * conf.thresholds.at("passquorum"_n);
	// asset refund_thresh_votes = trs.supply * conf.thresholds.at("refundvotes"_n);
	// asset refund_thresh_quorum = trs.supply * conf.thresholds.at("refundquorum"_n);

	//determine approval
	if (final_results.at("yes"_n) >= pass_thresh_votes && final_results.at("yes"_n) >= pass_thresh_quorum) {
		//update proposal
		proposals.modify(*p_itr, same_payer, [&](auto& col) {
			col.status = "passed"_n;
		});
	} else {
		//update proposal
		proposals.modify(*p_itr, same_payer, [&](auto& col) {
			col.status = "failed"_n;
		});
	}
}

//========== utility methods ==========

void wps::require_fee(name account_owner, asset fee) {
    //open deposits table, search for deposit
	deposits_table deposits(get_self(), get_self().value);
	auto dep = deposits.get(account_owner.value, "require_fee: deposit not found");

    //validate
    check(dep.escrow >= fee, "insufficient funds to cover fee");

    //charge fee
    deposits.modify(dep, same_payer, [&](auto& col) {
        col.escrow -= fee;
    });
}

void wps::debit_account(name account_owner, asset quantity) {
	//open deposits table, search for deposit
	deposits_table deposits(get_self(), get_self().value);
	auto d_itr = deposits.find(account_owner.value);

	if (d_itr == deposits.end()) {
		//emplace new deposit
		deposits.emplace(get_self(), [&](auto& col) {
			col.owner = account_owner;
			col.escrow = quantity;
		});
	} else {
		//update existing balance
		deposits.modify(*d_itr, same_payer, [&](auto& col) {
			col.escrow += quantity;
		});
	}
}

bool wps::valid_category(name category) {
    switch (category.value) {
        case (name("marketing").value):
            return true;
        case (name("dapps").value):
            return true;
        case (name("merchandise").value):
            return true;
        case (name("research").value):
            return true;
        case (name("audiovideo").value):
            return true;
		case (name("events").value):
            return true;
		case (name("devtools").value):
            return true;
		case (name("other").value):
            return true;
        default:
            return false;
    }
}

//======================== legacy actions ========================

ACTION wps::openvoting(uint64_t sub_id) {
	leg_submissions_table submissions(get_self(), get_self().value);
	auto& s = submissions.get(sub_id, "submission not found");

	require_auth(s.proposer);

	leg_ballots_table ballots("eosio.trail"_n, "eosio.trail"_n.value);
	auto& b = ballots.get(s.ballot_id, "Ballot not found on eosio.trail ballots_table");

	leg_proposals_table proposals("eosio.trail"_n, "eosio.trail"_n.value);
	auto& p = proposals.get(b.reference_id, "Prosal not found on eosio.trail proposals_table");

	leg_wp_environment_singleton legenv(get_self(), get_self().value);
	auto leg_conf = legenv.get();

	check(p.cycle_count == uint16_t(0), "proposal is no longer in building stage");
    check(p.status == uint8_t(0), "Proposal is already closed");

	uint32_t begin_time = current_time_point().sec_since_epoch();
	uint32_t end_time = current_time_point().sec_since_epoch() + leg_conf.cycle_duration;
	
    action(permission_level{ _self, "active"_n }, "eosio.trail"_n, "nextcycle"_n, make_tuple(
        _self,
        s.ballot_id,
        begin_time,
        end_time
    )).send();
}

ACTION wps::cancelsub(uint64_t sub_id) {
	leg_submissions_table submissions(get_self(), get_self().value);
    auto& s = submissions.get(sub_id, "Submission not found");

	require_auth(s.proposer);

	leg_ballots_table ballots("eosio.trail"_n, "eosio.trail"_n.value);
	auto b = ballots.get(s.ballot_id, "Ballot not found on eosio.trail ballots_table");

	leg_proposals_table proposals("eosio.trail"_n, "eosio.trail"_n.value);
	auto p = proposals.get(b.reference_id, "Prosal not found on eosio.trail proposals_table");

	check(p.cycle_count == uint16_t(0), "proposal is no longer in building stage");
    check(p.status == uint8_t(0), "Proposal is already closed");

	action(permission_level{ _self, "active"_n }, "eosio.trail"_n, "unregballot"_n, make_tuple(
        _self,
		s.ballot_id
    )).send();

	submissions.erase(s);
}

ACTION wps::claim(uint64_t sub_id) {
    leg_submissions_table submissions(_self, _self.value);
    const auto& sub = submissions.get(sub_id, "Worker Proposal Not Found");

	require_auth(sub.proposer);

    leg_ballots_table ballots("eosio.trail"_n, "eosio.trail"_n.value);
    const auto& bal = ballots.get(sub.ballot_id, "Ballot ID doesn't exist");
	
	leg_proposals_table props_table("eosio.trail"_n, "eosio.trail"_n.value);
	const auto& prop = props_table.get(bal.reference_id, "Proposal Not Found");

    check(prop.end_time < current_time_point().sec_since_epoch(), "Cycle is still open");
    check(prop.status == uint8_t(0), "Proposal is closed");

    leg_registries_table registries("eosio.trail"_n, name("eosio.trail").value);
    auto e = registries.find(VOTE_SYM.code().raw());

	leg_wp_environment_singleton legenv(get_self(), get_self().value);
	auto leg_conf = legenv.get();

    asset outstanding = asset(0, symbol("TLOS", 4));
    asset total_votes = prop.yes_count + prop.no_count + prop.abstain_count; //total votes cast on proposal
    asset non_abstain_votes = prop.yes_count + prop.no_count; 

    //pass thresholds
    asset quorum = e->supply * leg_conf.threshold_pass_voters / 100;
    asset votes_pass_thresh = non_abstain_votes * leg_conf.threshold_pass_votes / 100;

    //fee refund thresholds
    asset voters_fee_thresh = e->supply * leg_conf.threshold_fee_voters / 100; 
    asset votes_fee_thresh = total_votes * leg_conf.threshold_fee_votes / 100; 

    auto updated_fee = sub.fee;

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

    if(prop.yes_count > votes_pass_thresh && total_votes >= quorum) {
        outstanding += asset(int64_t(sub.amount), symbol("TLOS", 4));
    }
    
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
		uint32_t end_time = current_time_point().sec_since_epoch() + leg_conf.cycle_duration;
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
