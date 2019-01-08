# TIP-5 Interface and Sample Implementation Walkthrough

This walkthrough follows the TIP-5 eosio.token extension outlined in `token.registry.hpp` and implemented in `token.registry.cpp`.

### Actions

* `allot()` Allot is called to make an allotment to another account.

* `unallot()` Unallot is called to cancel an allotment and return the remaining allotted tokens to the sender's wallet.

* `claimallot()` ClaimAllot is called by the intended recipient to collect an allotment.

    It's important to note is that allotments can be claimed in increments or all at once; the choice is entirely at the discretion of the recipient.

    For example, Account A makes an allotment of 5.00 TEST tokens for Account B. Account B can then call claimallot() to retrieve any number of the allotted tokens up to the allotted amount. Account B calls claimallot() and retrieves 3.50 TEST tokens, which are then deposited into Account B's entry on the balances table. Later, Account B makes a second call to claimallot() and retrieves 1.00 TEST tokens, leaving 0.50 TEST in the allotment. Later, Account A calls unallot() and chooses to reclaim 0.25 TEST, moving those funds back into their wallet. Finally, Account B retrieves the remaining 0.25 TEST tokens, and the ram allocated for the allotment is returned to the sender.

### Tables and Structs

* `allotments_table` The allotments_table is a table separate from balances that allows users to make allotments to a desired recipient. The recipient can then collect their allotment at any time, provided the user who made the allotment has not reclaimed it.

    The allotments_table typedef is defined in the interface, allowing developers to instantiate the table with any code or scope as needed. The allotments table stores `allotment` structs, indexed by the `recipient` member.

    * `recipient` is the account that is intended to receive the allotment.
    * `sender` is the account that made the allotment.
    * `tokens` is the asset allotted, where `tokens.amount` is the quantity allotted.

### Teclos Example Commands

*Replace positionals such as contract-name, account-name, and your-auth with the appropriate identifiers for your environment.*

* `teclos get table contract-name account-name allotments`

    Returns all allotments made by the account name.

* `teclos push action contract-name allot '{"sender": "account-name", "recipient": "account-name", "tokens": "5.00 TEST"}' -p your-auth`

    Makes an allotment for the recipient from the sender.

* `teclos push action contract-name unallot '{"sender": "account-name", "recipient": "recipient-name", "tokens": "5.00 TEST"}' -p your-auth`

    Reclaims an allotment made by the sender.

* `teclos push action contract-name claimallot '{"sender": "account-name", "recipient": "account-name", "tokens": "5.00 TEST"}' -p your-auth`

    Claims an allotment made for the recipient.