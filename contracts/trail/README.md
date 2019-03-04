# Trail Voting Platform

Trail is a fully on-chain voting platform for the Telos Blockchain Network. It offers a suite of voting services for both users and developers and is maintained as a completely open source project for maximum transparency and auditability.

## Purpose of VOTE Token

Trail manages an internal token named `VOTE` that is used in place of the native system token. This is done to avoid confusion that a user's `TLOS` are being held on the platform, and to allow for more efficient actions and therefore lower resource usage.

## Services

* `Public Ballot Creation`

    Any user on the Telos Blockchain Network can create a ballot that is publicly viewable by all users.

    During Ballot creation, the Ballot publisher may specify which token they would like to use for counting votes. In most cases this will be the default `VOTE` token, but Trail offers additional services for creating controlled voting tokens that Ballot publishers can elect to use instead.

* `Casting Votes`

    All users who own a balance of `TLOS` are able to mirror their balance and receive an equal number of `VOTE` tokens, which are castable on any Ballot that is created for use with that token.

    Additionally, if users have balance of custom tokens and there are open ballots for that token, they may cast their votes on that Ballot as well.

* `Custom Voting Token Creation`

    Any user may create a custom voting token hosted by Trail that automatically inherits Trail's entire suite of voting services.

    For instance, a user could create a `BOARD` token to represent one board seat at their company. They would then own the entire `BOARD` token namespace within Trail and have the full authority to `mint` or `send` those board tokens to their respective owners. Trail's verbose token interface even allows for additional token features (set at the time of token creation) such as `burning`, `seizing`, or `mutable_max`.

* `Custom Token Ballots`

    Once a user has created a custom voting token, any holder of that token may publish Ballots that are votable on by any holder of the custom token.

    Continuing with the `BOARD` example, a board member could propose a vote for all `BOARD` holders. Ballots created for custom tokens operate exactly the same as Ballots running on the system-backed `VOTE` token.

* `Fraud Prevention`

    Trail has implemented a powerful Rebalance system that detects changes to a user's staked resources and automatically updates all open votes for the user.

* `System Efficiency`

    Trail has several efficiency actions that are callable by any user on the network to help clean up old votes and maintain a lightweight voting system.

## In Development (in no particular order)

* Proxy Registration, Proxy Voting, Proxy Management

* Cleanup Rewards (Task Market)

* Additional Categories

* Additional Token Settings
