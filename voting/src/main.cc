/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
reverse rank order voting.
**/

#include "data.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <algorithm>

namespace {

class Result {
public:
  std::string who_;
  int idx_ = 0;
  int count_ = 0;
};
using Results = std::vector<Result>;

class Voting {
public:
  Voting() = default;
  Voting(const Voting&) = delete;
  Voting(Voting&&) = delete;
  ~Voting() = default;

  int nballots_ = 0;
  int ncandidates_ = 0;
  Ballots ballots_;
  Candidates candidates_;
  Results results_;

  void run() noexcept {
    restore_ballots();
    candidates_ = get_candidates();
    nballots_ = ballots_.size();
    ncandidates_ = candidates_.size();
    results_.resize(ncandidates_);

    LOG("");
    LOG("ballots.size()="<<nballots_);
    LOG("candidates_.size()="<<ncandidates_);

    show_candidates();
    first_past_post();
    ranked_choice_voting();
    reverse_rank_order();

    LOG("");
  }

  void show_candidates() noexcept {
    LOG("");
    LOG("Candidates:");
    init_results();
    for (int i = 1; i < ncandidates_; ++i) {
      results_[i].count_ = i;
    }
    print_results();
  }

  void first_past_post() noexcept {
    restore_ballots();
    init_results();
    for (auto ballot : ballots_) {
      int choice = ballot.choice_[0];
      auto& result = results_[choice];
      ++result.count_;
    }
    sort_results();
    LOG("");
    LOG("First Past the Post Results:");
    print_results();
    auto& winner = results_[1];
    LOG("Winner: "<<winner.who_);
  }

  void ranked_choice_voting() noexcept {
    LOG("");
    LOG("Ranked Choice Results:");
    restore_ballots();
    std::vector<int> in_race(ncandidates_, 1);
    for (int rank = ncandidates_ - 2; rank > 0; --rank) {
      init_results();
      for (auto&& ballot : ballots_) {
        int choice = ballot.choice_[0];
        if (choice > 0) {
          /** first place vote for one candidate. **/
          auto& result = results_[choice];
          ++result.count_;
        }
      }
      sort_results();
      /*LOG("Rank: "<<rank);
      print_results();*/
      auto& loser = results_[rank+1];
      LOG(loser.who_<<" is eliminated.");
      eliminate_from_ballots(loser.idx_);
      in_race[loser.idx_] = 0;
    }
    auto& winner = results_[1];
    LOG("Winner: "<<winner.who_);
  }

  void reverse_rank_order() noexcept {
    LOG("");
    LOG("Reverse Rank Order Results:");
    restore_ballots();
    std::vector<int> in_race(ncandidates_, 1);
    for (int rank = ncandidates_ - 2; rank > 0; --rank) {
      init_results();
      for (auto&& ballot : ballots_) {
        int choice = ballot.choice_[rank];
        if (choice > 0) {
          /** last place vote for one candidate. **/
          auto& result = results_[choice];
          ++result.count_;
        } else {
          /** last place votes for many candidates. **/
          auto is_last = in_race;
          for (int i = 0; i <= rank; ++i) {
            choice = ballot.choice_[i];
            is_last[choice] = 0;
          }
          for (int i = 1; i < ncandidates_; ++i) {
            if (is_last[i]) {
              auto& result = results_[i];
              ++result.count_;
            }
          }
        }
      }
      sort_results();
      //LOG("Rank: "<<rank);
      //print_results();
      auto& loser = results_[1];
      LOG(loser.who_<<" is eliminated.");
      eliminate_from_ballots(loser.idx_);
      in_race[loser.idx_] = 0;
    }
    auto& winner = results_[2];
    LOG("Winner: "<<winner.who_);
  }

  void eliminate_from_ballots(
    int loser_idx
  ) noexcept {
    int nchoices = ncandidates_ - 1;
    for (auto&& ballot : ballots_) {
      int dst = 0;
      for (int src = 0; src < nchoices; ++src) {
        if (ballot.choice_[src] != loser_idx) {
          ballot.choice_[dst++] = ballot.choice_[src];
        }
      }
      for (; dst < nchoices; ++dst) {
        ballot.choice_[dst] = 0;
      }
    }
  }

  void restore_ballots() noexcept {
    ballots_ = get_voting_data();
  }

  void init_results() noexcept {
    for (int i = 1; i < ncandidates_; ++i) {
      auto& result = results_[i];
      result.who_ = candidates_[i];
      result.idx_ = i;
      result.count_ = 0;
    }
  }

  void sort_results() noexcept {
    auto first = results_.begin();
    ++first;
    auto last = results_.end();
    auto cmp = [](const Result& a, const Result& b) -> bool {
      return (a.count_ > b.count_);
    };
    std::sort(first, last, cmp);
  }

  void print_results() noexcept {
    for (int i = 1; i < ncandidates_; ++i) {
      auto& result = results_[i];
      LOG(result.who_<<": "<<result.count_);
    }
  }

  void print_ballot(
    const Ballot& ballot
  ) noexcept {
    LOG("ballot={"
      <<ballot.choice_[0]<<", "
      <<ballot.choice_[1]<<", "
      <<ballot.choice_[2]<<", "
      <<ballot.choice_[3]<<", "
      <<ballot.choice_[4]<<", "
      <<ballot.choice_[5]<<"}");
  }
};

} // anonymous namespace

int main(
  int argc, char *argv[]
) noexcept {
  (void) argc;
  (void) argv;

  agm::log::init(AGM_TARGET_NAME ".log", false);

  Voting voting;
  voting.run();

  return 0;
}
