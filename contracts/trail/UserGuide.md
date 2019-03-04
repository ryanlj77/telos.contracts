# Trail Service User Guide

Trail offers a comprehensive suite of blockchain-based voting services available to any prospective voter on the Telos Blockchain Network.

### Recommended User Resources

* 9 TLOS to CPU

* 1 TLOS to NET

* 10 KB of RAM

## Voter Registration

All users on the Telos Blockchain Network can register their Telos accounts within Trail.

### 1. Registration

* `open(name owner, symbol token_sym)`

    The open action creates an zero-balance wallet for the given token symbol if a token registry of the given symbol exists.

    `owner` is the name of the user opening the new account.

    `token_sym` is the symbol of tokens being stored in the new balance.

All users who want to participate in core voting **must** sign the  `open()` action and supply `VOTE,0` as the token symbol.

* `close(name owner, symbol token_sym)`

    The close action cancels an existing account balance from Trail and returns the RAM cost back to the user.

    `owner` is the nameof the user closing the account.

    `token_sym` is the symbol of the token balance to close.

Note that an account must be empty of all tokens before being allowed to close. Closed accounts can be created again at any time by simply signing another `open()` action.

## Voter Participation



### 1. Getting and Casting Votes

* `castvote(name voter, name ballot_name, name option)`

    The castvote action will cast user's full `VOTE` balance on the given ballot. Note that this **does not spend** the user's `VOTE` tokens, it only **copies** their full token weight onto the ballot.

### 2. Cleaning Old Votes

* `cleanupvotes(name voter, uint16_t count, symbol voting_sym)`

    The cleanupvotes action is a way to clear out old votes and allows voters to reclaim their RAM that has been allocated for storing vote receipts.

    `voter` is the account for which old receipts will be deleted.

    `count` is the number of receipts the voter wishes to delete. This action will run until it deletes specified number of receipts, or until it reaches the end of the list. Passing in a hard number allows voters to carefully manage their NET and CPU expenditure.

    Note that Trail has been designed to be tolerant of users deleting their vote receipts. Trail will never delete a vote receipt that is still applicable to an open ballot. Because of this, the `cleanupvotes()` action may be called by anyone to clean up any user's list of vote receipts.

* `cleanhouse(name voter)`

    The cleanhouse action will attempt to clean out a voter's entire history of votes in one sweep. If a user has an exceptionally large backlog of uncleaned votes this action may fail to clean all of them within the minimum transaction time. If that happens it's better to call `cleanupvotes()` several times first to reduce the number of votes `cleanhouse()` will attempt to clean. 

    `voter` is the name of the account to clean votes for.

Note that this action requires no account authorization, meaning any user can call `cleanhouse()` for any other user to assist in cleaning their vote backlog. There is no security risk to doing this and is a really nice way to help the platform generally run smoother.