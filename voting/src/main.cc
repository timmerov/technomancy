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

/** choose a data set. **/
//using namespace burlington;
//using namespace minneapolis_park_1;
using namespace minneapolis_park_3;

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
    head_to_head();
    head_to_head_elimination();
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

  void head_to_head() noexcept {
    LOG("");
    LOG("Head to Head Results:");
    restore_ballots();
    init_results();
    auto rankings = ballots_;
    for (int i = 0; i < nballots_; ++i) {
      auto& src = ballots_[i];
      auto& dst = rankings[i];
      for (int k = 0; k < ncandidates_-1; ++k) {
        dst.choice_[k] = ncandidates_;
      }
      for (int k = 0; k < ncandidates_-1; ++k) {
        int choice = src.choice_[k];
        if (choice > 0) {
          dst.choice_[choice-1] = k+1;
        }
      }
    }
    int max_wins = 0;
    int winner = 0;
    for (int i = 0; i < ncandidates_-1; ++i) {
      int wins = 0;
      for (int k = 0; k < ncandidates_-1; ++k) {
        int fori = 0;
        int fork = 0;
        for (auto&& ranking : rankings) {
          int ranki = ranking.choice_[i];
          int rankk = ranking.choice_[k];
          if (ranki < rankk) {
            ++fori;
          }
          if (rankk < ranki) {
            ++fork;
          }
        }
        if (fori > fork) {
          LOG(candidates_[i+1]<<" beat "<<candidates_[k+1]<<" "<<fori<<" to "<<fork<<".");
          ++wins;
        }
      }
      LOG(candidates_[i+1]<<" has "<<wins<<" wins.");
      if (max_wins < wins) {
        max_wins = wins;
        winner = i + 1;
      }
    }
    LOG("Winner: "<<candidates_[winner]);
  }

  void head_to_head_elimination() noexcept {
    LOG("");
    LOG("Head to Head Elimination Results:");
    restore_ballots();
    init_results();
    std::vector<int> in_race(ncandidates_, 1);
    for (int round = 1; round < ncandidates_-1; ++round) {
      auto rankings = ballots_;
      for (int i = 0; i < nballots_; ++i) {
        auto& src = ballots_[i];
        auto& dst = rankings[i];
        for (int k = 0; k < ncandidates_-1; ++k) {
          dst.choice_[k] = ncandidates_;
        }
        for (int k = 0; k < ncandidates_-1; ++k) {
          int choice = src.choice_[k];
          if (choice > 0) {
            dst.choice_[choice-1] = k+1;
          }
        }
      }
      int min_wins = ncandidates_;
      int loser = 0;
      for (int i = 0; i < ncandidates_-1; ++i) {
        if (in_race[i] == 0) {
          continue;
        }
        int wins = 0;
        for (int k = 0; k < ncandidates_-1; ++k) {
          if (in_race[k] == 0) {
            continue;
          }
          int fori = 0;
          int fork = 0;
          for (auto&& ranking : rankings) {
            int ranki = ranking.choice_[i];
            int rankk = ranking.choice_[k];
            if (ranki < rankk) {
              ++fori;
            }
            if (rankk < ranki) {
              ++fork;
            }
          }
          if (fori > fork) {
            //LOG(i+1<<" beat "<<k+1<<" "<<fori<<" to "<<fork);
            ++wins;
          }
        }
        //LOG(i+1<<" has "<<wins<<" wins");
        if (min_wins > wins) {
          min_wins = wins;
          loser = i;
        }
      }
      LOG(candidates_[loser+1]<<" is eliminated.");
      eliminate_from_ballots(loser+1);
      in_race[loser] = 0;
    }
    int winner = -1;
    for (int i = 0; i < ncandidates_-1; ++i) {
      if (in_race[i]) {
        winner = i + 1;
        break;
      }
    }
    LOG("Winner: "<<candidates_[winner]);
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
      int lcm = get_lcm(rank+1);
      for (auto&& ballot : ballots_) {
        int choice = ballot.choice_[rank];
        if (choice > 0) {
          /** last place vote for one candidate. **/
          auto& result = results_[choice];
          result.count_ += lcm;
        } else {
          /**
          last place votes for many candidates.
          divide the lcm evenly between them.
          **/
          auto is_last = in_race;
          for (int i = 0; i <= rank; ++i) {
            choice = ballot.choice_[i];
            is_last[choice] = 0;
          }
          int nlasts = 0;
          for (int i = 1; i < ncandidates_; ++i) {
            if (is_last[i]) {
              ++nlasts;
            }
          }
          for (int i = 1; i < ncandidates_; ++i) {
            if (is_last[i]) {
              auto& result = results_[i];
              result.count_ += lcm / nlasts;
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

  int get_lcm(int max) noexcept {
    static int g_table[] = {
      0, 1, 2,
      2*3,
      1*3*4,
      1*3*4*5,
      2*1*1*5*6,
      2*1*1*5*6*7,
      1*3*1*5*3*7*8
    };
    if (max < 0 || max > 8) {
      max = 0;
    }
    int lcm = g_table[max];
    return lcm;
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
