# Telos Canopy User/Developer Guide

Telos Canopy is a document storage solution designed to supplement EOS.IO software. Telos Canopy provides an IPFS Cluster with a collective pinset managed by a smart contract table.

Telos Canopy incentivizes node operators to run the nodeos Canopy Plugin and Canopy API Plugin through a native tokenomics model. By running the Canopy Plugins, node operators supplement the IPFS Cluster with extra resources and additional redundancy replication. In return for providing this service, node operators will be paid out in the native HDD token (Hard Disk Drive) upon removal of the file from the cluster, in addition to small recurring fees for maintaining the service.


1. transfer to teloscanopy
2. buydisk
3. addfile

## A La Carte Billing

uint32_t last_bill_time;
asset per_diem_bill;

When user pins or unpins file, calc days of billing * per diem and process charges.
Then increase per_diem by value of new file being pinned (or decrease if unpinning) and update last_bill_time to now().

uint32_t insufficient_balance_time;

Time point at which user will run out of funds
update target time with each pin/unpin or update of DISK balance
have table with sec idx in order of insufficient_balance_time being the soonest
periodic billing by providers will help keep the table fresh and users being billed correctly and on time

have plugin calculate instead? Could send notifications/warnings to accounts

if user selling DISK back, process all pending charges first. Fail action if attempting to sell enough DISK that would cause an insufficient balance. maybe have a minimum buffer on sellable DISK? maybe have a cleanhouse() action that processing all outstanding charges and unpins everything?


## Stephanie's Model

Account requests object be stored.  Current rate is $V TLOS/block (256KiB).

When object goes to store, the amount of replicas amongst the field is calculated
and a random list (from TIPFS) is created of storage nodes (returned in a json array)
that were used to store the block.  This list of storage nodes + the storage price are used
to create an escrow account governed by a smart contract.

We can do a few things here.  One would be reduced redundancy ( half the replicas for half the price ).
Another is we could calculate the total price to store as a function ( block price * starting replicas )  

The contract creates an escrow account where the amount provided for the block is deposited
along with the list of storage nodes.

When the unpin for that block is finally sent, the contract queries every node directly in the list
to verify they still have the block(tbd exactly how, something more than hashes maybe?) and those 
that respond correctly are rewarded with their slice of the escrow pool divided equally amongst
the correct responders.

## Rafiki Oracle

Telos Canopy also offers a dedicated Oracle service for feeding document data into smart contract actions.