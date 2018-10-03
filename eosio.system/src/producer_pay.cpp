#include <eosio.system/eosio.system.hpp>

#include <eosio.token/eosio.token.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/producer_schedule.hpp>
#include <eosiolib/chain.h>
#include <algorithm>
#include <cmath>

#define RESET_BLOCKS_PRODUCED 0
#define MAX_BLOCK_PER_CYCLE 12

namespace eosiosystem
{
using namespace eosio;
const int64_t min_activated_stake = 150'000'000'0000;
const double continuous_rate = 0.025;                    // 2.5% annual inflation rate
const uint32_t blocks_per_year = 52 * 7 * 24 * 2 * 3600; // half seconds per year
const uint32_t seconds_per_year = 52 * 7 * 24 * 3600;
const uint32_t blocks_per_day = 2 * 24 * 3600;
const uint32_t blocks_per_hour = 2 * 3600;
const int64_t useconds_per_day = 24 * 3600 * int64_t(1000000);
const int64_t useconds_per_year = seconds_per_year * 1000000ll;

bool system_contract::crossed_missed_blocks_threshold(uint32_t amountBlocksMissed)
{
      //6hrs
      auto timeframe = (_grotations.next_rotation_time.to_time_point() - _grotations.last_rotation_time.to_time_point()).to_seconds();

      //get_active_producers returns the number of bytes populated
      account_name prods[21];
      uint32_t totalProds = get_active_producers(prods, sizeof(account_name) * 21) / 8;
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

void system_contract::set_producer_block_produced(account_name producer, uint32_t amount)
{
      auto pitr = _producers.find(producer);
      if (pitr != _producers.end())
      {
            if (amount == 0)
                  _producers.modify(pitr, 0, [&](auto &p) { p.blocks_per_cycle = amount; });
            else
                  _producers.modify(pitr, 0, [&](auto &p) { p.blocks_per_cycle += amount; });
      }
}

void system_contract::set_producer_block_missed(account_name producer, uint32_t amount)
{
      auto pitr = _producers.find(producer);
      if (pitr != _producers.end() && pitr->active())
      {
            _producers.modify(pitr, 0, [&](auto &p) {
                  p.missed_blocks += amount;
                  if (crossed_missed_blocks_threshold(p.missed_blocks))
                        p.deactivate();
            });
      }
}

void system_contract::update_producer_blocks(account_name producer, uint32_t amountBlocksProduced, uint32_t amountBlocksMissed)
{
      auto pitr = _producers.find(producer);
      if (pitr != _producers.end() && pitr->active())
      {
            _producers.modify(pitr, 0, [&](auto &p) {
                  p.blocks_per_cycle += amountBlocksProduced;
                  p.missed_blocks += amountBlocksMissed;
                  if (crossed_missed_blocks_threshold(p.missed_blocks))
                        p.deactivate();
            });
      }
}

void system_contract::check_missed_blocks(block_timestamp timestamp, account_name producer)
{
      if (_grotations.current_bp == 0)
      {
            _grotations.last_time_block_produced = timestamp;
            _grotations.current_bp = producer;
            set_producer_block_produced(producer, 1);
            return;
      }

      //12 == 6s
      auto producedTimeDiff = timestamp.slot - _grotations.last_time_block_produced.slot;
      if (producedTimeDiff == 1 && producer == _grotations.current_bp)
            set_producer_block_produced(producer, producedTimeDiff);
      else if (producedTimeDiff == 1 && producer != _grotations.current_bp)
      {
            //set zero to last producer blocks_per_cycle
            set_producer_block_produced(_grotations.current_bp, RESET_BLOCKS_PRODUCED);
            //update current producer blocks_per_cycle
            set_producer_block_produced(producer, producedTimeDiff);
      }
      else if (producedTimeDiff > 1 && producer == _grotations.current_bp)
            update_producer_blocks(producer, 1, producedTimeDiff - 1);
      else
      {
            auto lastPitr = _producers.find(_grotations.current_bp);
            if (lastPitr == _producers.end())
                  return;

            account_name producers_schedule[21];
            uint32_t bytes_populated = get_active_producers(producers_schedule, sizeof(account_name) * 21);

            auto currentProducerIndex = std::distance(producers_schedule, std::find(producers_schedule, producers_schedule + 21, producer));

            auto totalMissedSlots = std::fabs(producedTimeDiff - 1 - lastPitr->blocks_per_cycle);

            //last producer didn't miss blocks
            if (totalMissedSlots == 0)
            {
                  //set zero to last producer blocks_per_cycle
                  set_producer_block_produced(_grotations.current_bp, RESET_BLOCKS_PRODUCED);

                  update_producer_blocks(producers_schedule[currentProducerIndex - 1], RESET_BLOCKS_PRODUCED, producedTimeDiff - 1);

                  set_producer_block_produced(producer, 1);
            }
            else
            { //more than one producer missed blocks
                  if (totalMissedSlots / MAX_BLOCK_PER_CYCLE > 0)
                  {
                        auto totalProdsMissedSlots = totalMissedSlots / 12;
                        auto totalCurrentProdMissedBlocks = std::fmod(totalMissedSlots, 12);

                        //Check if the last or the current bp missed blocks
                        if (totalCurrentProdMissedBlocks > 0)
                        {
                              auto lastProdTotalMissedBlocks = MAX_BLOCK_PER_CYCLE - lastPitr->blocks_per_cycle;
                              if (lastProdTotalMissedBlocks > 0)
                                    set_producer_block_missed(producers_schedule[currentProducerIndex - 1], lastProdTotalMissedBlocks);

                              update_producer_blocks(producer, 1, totalCurrentProdMissedBlocks - lastProdTotalMissedBlocks);
                        }
                        else
                              set_producer_block_produced(producer, 1);

                        for (int i = 0; i <= totalProdsMissedSlots; i++)
                        {
                              auto lastProdIndex = currentProducerIndex - (i + 1);
                              lastProdIndex = lastProdIndex < 0 ? 21 + lastProdIndex : lastProdIndex;

                              auto prod = producers_schedule[lastProdIndex];
                              set_producer_block_missed(prod, MAX_BLOCK_PER_CYCLE);
                        }

                        set_producer_block_produced(_grotations.current_bp, RESET_BLOCKS_PRODUCED);
                  }
                  else
                  {
                        set_producer_block_produced(_grotations.current_bp, RESET_BLOCKS_PRODUCED);
                        update_producer_blocks(producer, 1, totalMissedSlots);
                  }
            }
      }

      _grotations.last_time_block_produced = timestamp;
      _grotations.current_bp = producer;
}

void system_contract::onblock(block_timestamp timestamp, account_name producer)
{
      require_auth(N(eosio));

      recalculate_votes();

      /** until activated stake crosses this threshold no new rewards are paid */
      if (_gstate.total_activated_stake < min_activated_stake)
            return;

      if (_gstate.last_pervote_bucket_fill == 0) /// start the presses
            _gstate.last_pervote_bucket_fill = current_time();

      /**
       * At startup the initial producer may not be one that is registered / elected
       * and therefore there may be no producer object for them.
       */
      auto prod = _producers.find(producer);
      if (prod != _producers.end())
      {
            _gstate.total_unpaid_blocks++;
            _producers.modify(prod, 0, [&](auto &p) {
                  p.unpaid_blocks++;
            });
      }

      check_missed_blocks(timestamp, producer);

      /// only update block producers once every minute, block_timestamp is in half seconds
      if (timestamp.slot - _gstate.last_producer_schedule_update.slot > 120)
      {
            update_elected_producers(timestamp);

            if ((timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day)
            {
                  name_bid_table bids(_self, _self);
                  auto idx = bids.get_index<N(highbid)>();
                  auto highest = idx.begin();

                  if (highest != idx.end() &&
                        highest->high_bid > 0 &&
                        highest->last_bid_time < (current_time() - useconds_per_day) &&
                        _gstate.thresh_activated_stake_time > 0 &&
                        (current_time() - _gstate.thresh_activated_stake_time) > 14 * useconds_per_day)
                  {
                        _gstate.last_name_close = timestamp;
                        idx.modify(highest, 0, [&](auto &b) {
                              b.high_bid = -b.high_bid;
                        });
                  }
            }
      }
}
void system_contract::recalculate_votes(){
   // fix type 1 : fixes just total vote weight
   // iterate and fix the total_producer_vote_weight = _producers(sum) if < 0
   // for the current issue on the testnet : this should be removed once the fix is applied
   // if (_gstate.total_producer_vote_weight <= -0.1){ // -0.1 threshold for floating point calc ?
   //     print("\n Negative total_weight_vote fix applied !");
   //     _gstate.total_producer_vote_weight = 0;
   //     for (const auto &prod : _producers) {
   //         _gstate.total_producer_vote_weight += prod.total_votes;
   //     }
   // }

   // fix type 2 : fixes proxied weights too
   if (_gstate.total_producer_vote_weight <= -0.1){ // -0.1 threshold for floating point calc ?
       _gstate.total_producer_vote_weight = 0;
       _gstate.total_activated_stake = 0;
       for(auto producer = _producers.begin(); producer != _producers.end(); ++producer){
           _producers.modify(producer, 0, [&](auto &p) {
               p.total_votes = 0;
           });
       }
       boost::container::flat_map<account_name, bool> processed_proxies;
       for (auto voter = _voters.begin(); voter != _voters.end(); ++voter) {
           if(voter->proxy && !processed_proxies[voter->proxy]){
               auto proxy = _voters.find(voter->proxy);
               _voters.modify( proxy, 0, [&]( auto& av ) {
                   av.last_vote_weight = 0;
                   av.last_stake = 0;
                   av.proxied_vote_weight = 0;
               });
               processed_proxies[voter->proxy] = true;
           }
           if(!voter->is_proxy || !processed_proxies[voter->owner]){
               _voters.modify( voter, 0, [&]( auto& av ) {
                   av.last_vote_weight = 0;
                   av.last_stake = 0;
                   av.proxied_vote_weight = 0;
               });
               processed_proxies[voter->owner] = true;
           }
           update_votes(voter->owner, voter->proxy, voter->producers, true);
       }
   }
}

void system_contract::claimrewards(const account_name &owner)
{
     // TODO: Implement
}

} //namespace eosiosystem
