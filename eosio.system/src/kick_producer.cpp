#pragma once

#include <eosio.system/eosio.system.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/producer_schedule.hpp>
#include <eosio.token/eosio.token.hpp>
#include <eosiolib/chain.h>
#include <algorithm>
#include <cmath>  

#define RESET_BLOCKS_PRODUCED 0
#define MAX_BLOCK_PER_CYCLE 12

namespace eosiosystem {

using namespace eosio;

uint32_t active_schedule_size = 0;

bool system_contract::reach_consensus() {
    return _grotations.offline_bps.size() < (active_schedule_size / 3) - 1;
}

void system_contract::add_producer_to_kick_list(offline_producer producer) {
    //add unique producer to the list
    account_name bp_name = producer.name;
    auto bp = std::find_if(_grotations.offline_bps.begin(), _grotations.offline_bps.end(), [&bp_name](const offline_producer &op) {
        return op.name == bp_name; 
    }); 

    if(bp == _grotations.offline_bps.end())  _grotations.offline_bps.push_back(producer);
    else { // update producer missed blocks and total votes
       for(size_t i = 0; i < _grotations.offline_bps.size(); i++) {
           if(bp_name == _grotations.offline_bps[i].name){
               _grotations.offline_bps[i].total_votes = producer.total_votes;
               _grotations.offline_bps[i].missed_blocks = producer.missed_blocks;
               break;
           }
       }    
    }
     
    if(active_schedule_size > 1 && !reach_consensus()) kick_bp();
}

void system_contract::remove_producer_to_kick_list(offline_producer producer) {
  // verify if bp was missing blocks
    account_name bp_name = producer.name;
    auto bp = std::find_if(_grotations.offline_bps.begin(), _grotations.offline_bps.end(), [&bp_name](const offline_producer &op) {
        return op.name == bp_name; 
    });   
   
  // producer found
  if (bp != _grotations.offline_bps.end()) _grotations.offline_bps.erase(bp, _grotations.offline_bps.end());
}

void system_contract::kick_bp() {
    std::vector<offline_producer> o_bps = _grotations.offline_bps;
    std::sort(o_bps.begin(), o_bps.end(), [](const offline_producer &op1, const offline_producer &op2){
        if(op1.missed_blocks != op2.missed_blocks) return op1.missed_blocks > op2.missed_blocks;
        else return op1.total_votes < op2.total_votes;
    });
    for(uint32_t i = 0; i < o_bps.size(); i++) {
        auto obp = o_bps[i];
        auto bp = _producers.find(obp.name);

        _producers.modify(bp, 0, [&](auto &p) {
            p.deactivate();
            remove_producer_to_kick_list(obp);
        });

        if(reach_consensus()) break;
    }
}

bool system_contract::crossed_missed_blocks_threshold(uint32_t amountBlocksMissed) {
    if(active_schedule_size <= 1) return false;

    //6hrs
    auto timeframe = (_grotations.next_rotation_time.to_time_point() - _grotations.last_rotation_time.to_time_point()).to_seconds();
   
    //get_active_producers returns the number of bytes populated
    // account_name prods[21];
    auto totalProds = active_schedule_size;//get_active_producers(prods, sizeof(account_name) * 21) / 8;
    //Total blocks that can be produced in a cycle
    auto maxBlocksPerCycle = (totalProds - 1) * MAX_BLOCK_PER_CYCLE;
    //total block that can be produced in the current timeframe
    auto totalBlocksPerTimeframe = (maxBlocksPerCycle * timeframe) / (maxBlocksPerCycle / 2);
    //max blocks that can be produced by a single producer in a timeframe
    auto maxBlocksPerProducer = (totalBlocksPerTimeframe * MAX_BLOCK_PER_CYCLE) / maxBlocksPerCycle;
    //15% is the max allowed missed blocks per single producer
    auto thresholdMissedBlocks = maxBlocksPerProducer * 0.15;
    
    return amountBlocksMissed > thresholdMissedBlocks;
}

void system_contract::set_producer_block_produced(account_name producer, uint32_t amount) {
  auto pitr = _producers.find(producer);
  if (pitr != _producers.end()) {
    _producers.modify(pitr, 0, [&](auto &p) {
        if(amount == 0) p.blocks_per_cycle = amount;
        else p.blocks_per_cycle += amount;
        
        offline_producer op{p.owner, p.total_votes, p.missed_blocks};
        remove_producer_to_kick_list(op);
    });
  }
}

void system_contract::set_producer_block_missed(account_name producer, uint32_t amount) {
  auto pitr = _producers.find(producer);
  if (pitr != _producers.end() && pitr->active()) {
    _producers.modify(pitr, 0, [&](auto &p) {
        p.missed_blocks += amount;

        offline_producer op{p.owner, p.total_votes, p.missed_blocks};
        if(crossed_missed_blocks_threshold(p.missed_blocks)) {
            p.deactivate();
            remove_producer_to_kick_list(op);
        } else if(op.missed_blocks > 0) add_producer_to_kick_list(op);
    });
  }
}

void system_contract::update_producer_blocks(account_name producer, uint32_t amountBlocksProduced, uint32_t amountBlocksMissed) {
  auto pitr = _producers.find(producer);
  if (pitr != _producers.end() && pitr->active()) {
      _producers.modify(pitr, 0, [&](auto &p) { 
        p.blocks_per_cycle += amountBlocksProduced; 
        p.missed_blocks += amountBlocksMissed;

        offline_producer op{p.owner, p.total_votes, p.missed_blocks};
        if(crossed_missed_blocks_threshold(p.missed_blocks)) {
            p.deactivate();
            remove_producer_to_kick_list(op);
        } else if(op.missed_blocks > 0) add_producer_to_kick_list(op);
      });    
  }
}

void system_contract::check_missed_blocks(block_timestamp timestamp, account_name producer) { 
    if(_grotations.last_onblock_caller == 0) {
        _grotations.last_time_block_produced = timestamp;
        _grotations.last_onblock_caller = producer;
        set_producer_block_produced(producer, 1);
        return;
    }
  
    //12 == 6s
    auto producedTimeDiff = timestamp.slot - _grotations.last_time_block_produced.slot;

    if(producedTimeDiff == 1 && producer == _grotations.last_onblock_caller) set_producer_block_produced(producer, producedTimeDiff); 
    else if(producedTimeDiff == 1 && producer != _grotations.last_onblock_caller) {
        //set zero to last producer blocks_per_cycle 
        set_producer_block_produced(_grotations.last_onblock_caller, RESET_BLOCKS_PRODUCED);
        //update current producer blocks_per_cycle 
        set_producer_block_produced(producer, producedTimeDiff);
    } 
    else if(producedTimeDiff > 1 && producer == _grotations.last_onblock_caller) update_producer_blocks(producer, 1, producedTimeDiff - 1);
    else {
        auto lastPitr = _producers.find(_grotations.last_onblock_caller);
        if (lastPitr == _producers.end()) return;
            
        account_name producers_schedule[21];
        uint32_t total_prods = get_active_producers(producers_schedule, sizeof(account_name) * 21) / 8;
        
        active_schedule_size = total_prods;
        auto currentProducerIndex = std::distance(producers_schedule, std::find(producers_schedule, producers_schedule + total_prods, producer));
        auto totalMissedSlots = std::fabs(producedTimeDiff - 1 - lastPitr->blocks_per_cycle);

        //last producer didn't miss blocks    
        if(totalMissedSlots == 0.0) {
            //set zero to last producer blocks_per_cycle 
            set_producer_block_produced(_grotations.last_onblock_caller, RESET_BLOCKS_PRODUCED);
            
            account_name bp_offline = currentProducerIndex == 0 ? producers_schedule[total_prods - 1] : producers_schedule[currentProducerIndex - 1];
            
            update_producer_blocks(bp_offline, RESET_BLOCKS_PRODUCED, producedTimeDiff - 1);
            
            set_producer_block_produced(producer, 1);
        } else { //more than one producer missed blocks
            if(totalMissedSlots / MAX_BLOCK_PER_CYCLE > 0) {
                auto totalProdsMissedSlots = totalMissedSlots / 12;
                auto totalCurrentProdMissedBlocks = std::fmod(totalMissedSlots, 12);
                
                //Check if the last or the current bp missed blocks
                if(totalCurrentProdMissedBlocks > 0) {
                    auto lastProdTotalMissedBlocks = MAX_BLOCK_PER_CYCLE - lastPitr->blocks_per_cycle;
                    if(lastProdTotalMissedBlocks > 0) set_producer_block_missed(producers_schedule[currentProducerIndex - 1], lastProdTotalMissedBlocks);
                    
                    update_producer_blocks(producer, 1, uint32_t(totalCurrentProdMissedBlocks - lastProdTotalMissedBlocks));
                }  else set_producer_block_produced(producer, 1);
                
                for(int i = 0; i <= totalProdsMissedSlots; i++) {
                    auto lastProdIndex = currentProducerIndex - (i + 1);
                    lastProdIndex = lastProdIndex < 0 ? 21 + lastProdIndex : lastProdIndex;

                    auto prod = producers_schedule[lastProdIndex];
                    set_producer_block_missed(prod, MAX_BLOCK_PER_CYCLE);                       
                }

                set_producer_block_produced(_grotations.last_onblock_caller, RESET_BLOCKS_PRODUCED);
            } else {
                set_producer_block_produced(_grotations.last_onblock_caller, RESET_BLOCKS_PRODUCED);
                update_producer_blocks(producer, 1, uint32_t(totalMissedSlots) );
            }
        }
    }    

    _grotations.last_time_block_produced = timestamp;
    _grotations.last_onblock_caller = producer;
}

} // namespace eosiosystem
