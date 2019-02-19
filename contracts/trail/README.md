# Trail Service User/Developer Guide

Trail offers a comprehensive suite of blockchain-based voting services available to any prospective voter or contract developer on the Telos Blockchain Network.

## Understanding Trail's Role

Trail was designed to allow maximum flexibility for smart contract developers, while at the same time consolidating many of the boilerplate functions of voting contracts into a single service. 

This paradigm essentially means contract writers can leave the "vote counting" up to Trail and instead focus on how they want to interpret the results of their ballot. Through cross-contract table lookup, any contract can view the results of a ballot and then have their contracts act based on those results.

## Ballot Lifecycle

Building a ballot on Trail is simple and gives great flexibility to the developer to build their ballots in a variety of ways. An example ballot and voting contract will be used throughout this guide, and is recommended as an established model for current Trail interface best practices.

### 1. Setting Up Your Contract (Optional)

While setting up an external contract is not required to utilize Trail's services, it is required if developers want to **contractually** act on ballot results. If you aren't interested in having a separate contract and just want to run a ballot or campaign, then proceed to step 2.

For our contract example, we will be making a simple document updater where updates are voted on by the community. The basic interface looks like this:

* `createdoc(namae doc_name, string text)`

    The createdoc action will create a new document, retrievable by the supplied `doc_name`. The text of the document is stored as a string (not ideal for production, but fine for this example).

* `updatedoc(name doc_name, string new_text)`

    The updatedoc action simply updates the document with the new text. This will be called after voting has completed to update our document with the text supplied in the proposal (if the proposal passes).

* `makeproposal(name doc_name, string new_text)`

    The makeproposal action stores the new text retrievable by the document name, after first making sure the associated document exists. In a more complex example,this action could send an inline action to Trail's `createballot()` action to make the whole process atomic.

* `closeprop(name ballot_name)`

    The closeprop action is important for performing custom logic on ballot results, and simply sends an inline action to Trail's `closeballot()` action upon completion. The `status` field of the ballot allows ballot operators the ability to assign any context to status codes they desire. For example, a contract could interpret a status code of `7` to mean `REFUNDED`. However, status codes `0`, `1`, and `2` are reserved for meaning `SETUP`, `OPEN`, and `CLOSED`, respectively.

Note that no vote counts are stored on this contract (remember, that's Trail's job).

### 2. Creating a Ballot

Ballot regisration allows any user or developer to create a public ballot that can be voted on by any registered voter that has a balance of the respective voting token.

* `createballot(name ballot_name, name category, name publisher, string title, string description, string info_url, symbol voting_sym)`

    The regballot action will register a new ballot with attributes reflecting the given parameters.

    `ballot_name` is the unique name of the ballot. No other ballot may concurrently share this name.

    `category` is the category the ballot falls under. The currently supported categories are as follows:

    * `proposal` : Users vote on a proposal by casting votes in either the YES, NO, or ABSTAIN options.

    * `election` : Users vote on a single candidate from a set of candidate options.

    * `poll` : Users vote on a custom set of options. Polls can also be deleted mid-vote.

    `publisher` is the account publishing the ballot. Only this account will be able to modify and close the ballot.

    `title` is a string representing the title of the ballot.

    `description` is a brief explanantion of the ballot (ideally in markdown format). 

    `info_url` is critical to ensuring voters are able to know exactly what they are voting on when they cast their votes. This parameter should be a url to a webpage (or even better, an IPFS hash) that explains what the ballot is for, and should provide sufficient information for voters to make an informed decision. However, in order to ensure knowledgable voting this field should never be left blank.

    `voting_symbol` is the symbol to be used for counting votes. In order to cast a vote on the ballot, the voter must own a balance of tokens with the voting symbol.
    
    Note that in order to eliminate potential confusion with tokens on the `eosio.token` contract, the core voting symbol used by Trail is `VOTE` instead of `TLOS`,however `VOTE` tokens are issued to all holders of `TLOS` at a 1:1 ratio based on the user's current stake.
    
    Additionally, custom voting tokens must exist on Trail (by calling createtoken) before being able to create ballots that use them.

* `setinfo(name ballot_name, name publisher, string title, string description, string info_url)`

    The setinfo action allows the ballot publisher to replace the title, description, and info_url supplied in the initial createballot action.

    `ballot_name` is the name of the ballot to change.

    `publisher` is the name of the account that published the ballot. Only this account is authorized to update the info on the ballot.

    `title` is the new ballot title.

    `description` is the new ballot description.

    `info_url` is the new ballot info url.


* `readyballot(name ballot_name, name publisher, uint32_t end_time)`

    The readyballot action is called by the ballot publisher when voting is ready to open. After calling this action no further changes may be made to the ballot.

    `ballot_name` is the name of the ballot to ready.

    `publisher` is the name of the account that published the ballot. Only this account is authorized to ready the ballot for voting.

    `end_time` is the time voting will close for the ballot. The end_time value is expressed as seconds since the Unix Epoch.


* `deleteballot(name ballot_name, name publisher)`

    The deleteballot action deletes an existing ballot. This action can only be performed before a ballot opens for voting, and only by the publisher of the ballot.

    `publisher` is the account that published the ballot. Only this account can unregister the ballot.

    `ballot_id` is the ballot ID of the ballot to unregister.

### 3. Running A Ballot 

After ballot setup is complete, the only thing left to do is wait for users to begin casting their votes. All votes cast on Trail are live, so it's easy to see the state of the ballot as votes roll in. There are also a few additional features available for ballot runners that want to operate a more complex campaign. This feature set will grow with the development of Trail and as more complex versions of ballots are introduced to the system.

* `sample action` blah blah blah

In our custom contract example, none of these actions are used (just to keep it simple). 

### 4. Closing A Ballot

After a ballot has reached it's end time, it will automatically stop accepting votes. The final tally can be seen by querying the `ballots` table with the `ballot_name`.

* `closeballot(name ballot_name, name publisher, uint8_t new_status)`

    The closeballot action is how publishers close out a ballot and render a decision based on the results.

    `ballot_name` is the name of the ballot to close.

    `publisher` is the publisher of the ballot. Only this account may close the ballot.

    `new_status` is the resultant status after reaching a descision on the end state of the ballot. This number can represent any end state desired, but `0`, `1`, and `2` are reserved for `SETUP`, `OPEN`, and `CLOSED` respectively.

In our custom contract example, the `closeprop()` action would be called by the ballot operator, where closeprop would perform a cross-contract table lookup to access the final ballot results. Then, based on the results of the ballot, the custom contract would determine whether the proposal passed or failed, and update it's own tables accordingly. Finally, the closeprop action would send an inline action to Trail's `closeballot()` action to close out the ballot and assign a final status code for the ballot. For ballots that also have a set of candidates each with their own status codes, the `setallstats()` action allows each candidate's final status code to be set.

## Voter Registration and Participation

All users on the Telos Blockchain Network can register their accounts and receive a VoterID card that's valid for any ballot that counts its votes through Trail's Token Registry system.

### 1. Registration

* `open()`

    The open action creates an empty wallet for the given token symbol if a token registry of the given symbol exists.

* `close()`

    The unregvoter action unregisters an existing voter from Trail.

### 2. Getting and Casting Votes

* `vote()`

    The vote action will cast all a user's VOTE tokens on the given ballot. Note that this does not **spend** the user's VOTE tokens, it only applies their full weight to the ballot.

### 3. Cleaning Old Votes

* `cleanupvotes(name voter, uint16_t count, symbol voting_sym)`

    The cleanupvotes action is a way to clear out old votes and allows voters to reclaim their RAM that has been allocated for storing vote receipts.

    `voter` is the account for which old receipts will be deleted.

    `count` is the number of receipts the voter wishes to delete. This action will run until it deletes specified number of receipts, or until it reaches the end of the list. Passing in a hard number allows voters to carefully manage their NET and CPU expenditure.

    Note that Trail has been designed to be tolerant of users deleting their vote receipts. Trail will never delete a vote receipt that is still applicable to an open ballot. Because of this, the `cleanupvotes` action may be called by anyone to clean up any user's list of vote receipts.

## Creating Custom Tokens

Trail allows any Telos Blockchain Network user to create and manage their own custom tokens, which can also be used to vote on any ballot that has been configured to count votes based on that token.

### About Token Registries

Trail stores metadata about tokens in a custom data structure called a Token Registry. This stucture automatically updates when actions are called that would modify any part of the registry's state.

### 1. Registering a New Token

* `createtoken(name publisher, asset max_supply, token_settings settings, string info_url)`

    The createtoken action creates a new token within Trail, and initializes it with the given token settings.

    `publisher` is the name of the account that is creating the token registry. Only this account will be able to issue new tokens, change settings, and execute other actions related to directly modifying the state of the registry.

    `max_supply` is the total number of tokens allowed to simultaneously exist in circulation. This action will fail if a token already exists with the same symbol.

    `settings` is a custom list of boolean settings designed for Trail's token registries. The currently supported settings are as follows:

    * `is_destructible` allows the registry to be erased completely by the publisher.
    * `is_proxyable` allows tokens to be proxied to users that have registered as a valid proxy for this registry.
    * `is_burnable` allows tokens to be burned from circulation by the publisher. 
    * `is_seizable` allows the tokens to be seized from token holders by the publisher. This setting is intended for publishers who want to operate a more controlled token and voting system.
    * `is_max_mutable` allows the registry's max_supply field to be adjusted through an action call. Only the publisher can call the `changemax` action.
    * `is_transferable` allows the tokens to be transferred to other users.

    `info_url` is a url link (ideally an IPFS link) that prospective voters can follow to get more information about the token and it's purpose.

### 2. Operating A Token Registry

Trail offers a wide range of token features designed to offer maximum flexibility and control for registry operators, while at the same time ensuring the transparency and security of registry operations for token holders.

* `mint(name publisher, name recipient, asset amount_to_mint)`

    The mint action is the primary way for new tokens to be introduced into circulation.

    `publisher` is the name of the account that published the registry. Only the registry publisher has the authority to mint new tokens.

    `recipient` is the name of the recipient intended to receive the minted tokens.

    `amount_to_mint` is the amount of tokens to mint for the recipient. This action will fail if the number of tokens to mint would breach the max supply allowed by the registry.

* `burn(name publisher, asset amount_to_burn)`

    The burn action is used to burn tokens from a user's wallet. This action is only callable by the registry publisher, and if the registry allows token burning.

    `publisher` is the publisher of the registry. The publisher may only burn tokens from their own balance.

    `amount_to_burn` is the number of tokens to burn from circulation.

* `seize()`

    The seize action is called by the publisher to seize a balance of tokens from another account. Only the publisher can seize tokens, and only if the registry's settings allow it.


* `changemax()`



* `transfer()`

    The transfer action is called by a token holder to send tokens to another account. This action is callable by any token holder, but only if the registry settings allow token transfers.



## In Development (in no particular order)

* Proxy Registration, Proxy Voting, Proxy Management

* Additional Token Settings
