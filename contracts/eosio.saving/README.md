# Worker Proposal Developer Guide

The Worker Proposal System contract allows members of the Telos Blockchain to create worker proposals. Through the WPS contract members of the telos blockchain can outline improvements that they believe benefit Telos. Once a proposal is published, members of Telos can use the Trail Service contract to vote for a proposal. If a proposal passes the proposer can claim the Tokens needed to implement the work described in their proposal.

# Contract Setup 

The Worker Proposal System contract should be set to the `eosio.saving` account. Or to which ever account will receive the tokens needed to fund worker proposals. 

## Dependencies

The WPS contract requires that the Trail Service contract be set on the `trailservice` account, and Trail 2 integration complete for `eosio.contracts` (to be completed in upcoming Telos Pine Update). 

## WPS Actions

> ACTION `updatewps()`

Updates the WPS contract version.

Required Authority: `get_self()`

- string `version`: semver version number.

> ACTION `newproposal()`

Drafts a new worker proposal and creates a new ballot on Trail.

Required Authority: `proposer`

- name `proposal_name`: the name of the new proposal, will also be the `ballot_name` in Trail.

- name `category`: the category name the proposal most accurately fits.

- string `title`: the title of the proposal.

- string `description`: a description of the proposal.

- string `ipfs_cid`: an ipfs_cid (or any uri) pointing to more thorough documentation or presentation of the proposal.

- name `proposer`: the name of the account submitting the proposal (and being charged the submission fee).

- name `recipient`: the name of the account that will receive the funds, if approved.

- asset `total_funds_requested`: the total amount of `TLOS` that will be requested for the entire proposal. This amount will be divided by the number of cycles in the proposal to determine the amount due for each cycle.

- OPTIONAL uint8_t `cycles`: the total number of cycles requested in the proposal. If no value is supplied, the number of cycles will default to 1.

> ACTION readyprop()

Completes the draft proposal and calls Trail to open voting on the ballot.

Required Authority: `proposal.proposer`

- name `proposal_name`: the name of the proposal to open for voting.

> ACTION endcycle()

Ends a cycle of voting for a proposal and calls Trail to close the ballot.

Required Authority: `proposal.proposer`

- name `proposal_name`: the name of the proposal to end.

> ACTION claimfunds()

Claims funds for the current cycle, if approved by a vote.

Required Authority: `proposal.proposer`

- name `proposal_name`: the name of the proposal to claim funds from.

- name `claimant`: the name of the account claiming the funds.

> ACTION nextcycle()

Moves the proposal to the next cycle if more cycles remain.

- name `proposal_name`: the name of the proposal to move to the next cycle.

- string `deliverable_report`: an ipfs cid or uri pointing to the previous cycle's deliverable report.

- name `next_cycle_name`: the name of the next cycle's ballot.

> ACTION cancelprop()

Cancels a proposal and calls Trail to cancel voting.

Required Authority: `proposal.proposer`

- name `proposal_name`: the name of the proposal to cancel.

- string `memo`: a memo describing the reason for the cancellation.

> ACTION deleteprop()

Deletes a cancelled or completed proposal and calls Trail to delete the ballot.

- name `proposal_name`: the name of the proposal to delete.

> ACTION getrefund()

Retrieves a refund on a proposal, if refund conditions are met.

Required Authority: `proposal.proposer`

- name `proposal_name`: the name of the proposal holding the refund.

> ACTION withdraw()

Withdraws a given quantity of TLOS from the WPS platform.

Required Authority: `deposit.owner`

- name `account_owner`: the name of the account to withdraw TLOS from.

- asset `quantity`: the amount of TLOS to withdraw.

---

## Legacy Actions (available until Trail 2 migration is complete)

>`void cancelsub(uint64_t sub_id)`

The cancelsub action allows members who have called `submit` previously to cancel their submission before the voting starts. 

*CAUTION: This action does not refund your fee!*

`sub_id` the unique id of the submission to be cancelled.

>`void openvoting(uint64_t sub_id)`

The openvoting action starts the voting of the initial proposal.

`sub_id` the unique id of the submisision to be started.

>`void claim(uint64_t sub_id)`

The claim action allows the proposer of the submission with `sub_id` to claim the `amount` of `TLOS` described in the submission.
