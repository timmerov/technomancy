/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
reverse rank order voting.

generate data sets.

for N=3...
these are the possible ways to rank order:

ABC ACB BAC BCA CAB CBA
AXX BXX CXX

ignore for now:
XXA XXB XXC

generate a random distribution of the above rank orders.
the shape of the distribution can be varied.
N=0 use the expectation values: 0.75, 0.50, 0.25.
N=1 uniform distribution.
N>1 uniform distribution averaged over N samples.

assign random utilities for a,b,c for each group.
sort them from highest to lowest.

compute the group utility for a,b,c.
sort the random distributions so a>b>c.

run one trial and log the details.
run many trials and log only the summary.

results:

10k trials, expectation utility:
Condorcet           : A=84.45% B=8.86% C=6.69%
First Past Post     : A=80.16% B=16.58% C=3.26%
Ranked Choice Voting: A=85.61% B=13.07% C=1.32%
Reverse Rank Order  : A=83.41% B=15.14% C=1.45%

10k trials, bell-shaped utility N=3:
Condorcet           : A=69.12% B=19.4% C=11.48%
First Past Post     : A=68.87% B=23.19% C=7.94%
Ranked Choice Voting: A=71.48% B=21.6% C=6.92%
Reverse Rank Order  : A=70.2% B=22.68% C=7.12%

10k trials, flat utility:
Condorcet           : A=58.49% B=24.67% C=16.84%
First Past Post     : A=59.06% B=27.26% C=13.68%
Ranked Choice Voting: A=60.65% B=27.11% C=12.24%
Reverse Rank Order  : A=59.59% B=27.97% C=12.44%


todo:
model partisan, swing, and non- voters.

todo:
assign axx distribution based on closeness of the b,c utilities.

todo:
(anti-)correlate a's utility for b,c with not-a's utility for b,c.

todo:
assign distributions based on closeness of the utilities.

todo:
model and/or estimate benefit from strategic voting.
**/

#include "data.h"
#include "generated.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <algorithm>
#include <chrono>
#include <random>

namespace {

/** number of trials to run. **/
//constexpr int kNVoteTrials = 1;
//constexpr int kNVoteTrials = 1000;
constexpr int kNVoteTrials = 10*1000;
//constexpr int kNVoteTrials = 100*1000;

/** uniform utilities or not. **/
//constexpr int kNUtilityTrials = 0;  // no randomness. use expectation values.
constexpr int kNUtilityTrials = 1;
//constexpr int kNUtilityTrials = 2;
//constexpr int kNUtilityTrials = 3;
//constexpr int kNUtilityTrials = 10;
//constexpr int kNUtilityTrials = 100;

class Result {
public:
    double score_ = 0.0;
    int idx_ = -1;
    std::string name_;

    Result(double score, int idx, const char *name) noexcept :
        score_(score), idx_(idx), name_(name)
    {
    }

    bool operator < (const Result& rhs) const {
        return score_ < rhs.score_;
    }
};
using Results = std::vector<Result>;

class VotingImpl {
public:
    VotingImpl() = default;
    VotingImpl(const VotingImpl &) = delete;
    VotingImpl(VotingImpl &&) = delete;
    ~VotingImpl() = default;

    /** probability distributions (9 of 12) **/
    double p_abc_ = 0.0;
    double p_acb_ = 0.0;
    double p_axx_ = 0.0;
    double p_bac_ = 0.0;
    double p_bca_ = 0.0;
    double p_bxx_ = 0.0;
    double p_cab_ = 0.0;
    double p_cba_ = 0.0;
    double p_cxx_ = 0.0;

    /**
    generated utility values (24).
    these are not valid after group_utility.
    **/
    double u_abc_a_ = 0.0;
    double u_abc_b_ = 0.0;
    double u_abc_c_ = 0.0;
    double u_acb_a_ = 0.0;
    double u_acb_c_ = 0.0;
    double u_acb_b_ = 0.0;
    double u_axx_a_ = 0.0;
    double u_axx_x_ = 0.0;
    double u_bac_b_ = 0.0;
    double u_bac_a_ = 0.0;
    double u_bac_c_ = 0.0;
    double u_bca_b_ = 0.0;
    double u_bca_c_ = 0.0;
    double u_bca_a_ = 0.0;
    double u_bxx_b_ = 0.0;
    double u_bxx_x_ = 0.0;
    double u_cab_c_ = 0.0;
    double u_cab_a_ = 0.0;
    double u_cab_b_ = 0.0;
    double u_cba_c_ = 0.0;
    double u_cba_b_ = 0.0;
    double u_cba_a_ = 0.0;
    double u_cxx_c_ = 0.0;
    double u_cxx_x_ = 0.0;

    /** results **/
    int condorcet_ = 0;
    int result_con_a = 0;
    int result_con_b = 0;
    int result_con_c = 0;
    int result_con_cycle = 0;
    int first_past_post_ = 0;
    int result_fpp_a = 0;
    int result_fpp_b = 0;
    int result_fpp_c = 0;
    int result_fpp_con = 0;
    int ranked_choice_voting_ = 0;
    int result_rcv_a = 0;
    int result_rcv_b = 0;
    int result_rcv_c = 0;
    int result_rcv_con = 0;
    int reverse_rank_order_ = 0;
    int result_rro_a = 0;
    int result_rro_b = 0;
    int result_rro_c = 0;
    int result_rro_con = 0;
    int result_nwinners_1_ = 0;
    int result_nwinners_2_ = 0;
    int result_nwinners_3_ = 0;

    /** random number generation **/
    std::mt19937_64 rng_;
    std::uniform_real_distribution<double> unif_;

    void run() noexcept {
        init();

        for (int i = 0; i < kNVoteTrials; ++i) {
            random_probabilities();
            random_utilities();
            group_utility();
            condorcet();
            first_past_post();
            ranked_choice_voting();
            reverse_rank_order();
            analyze();
        }

        if (kNVoteTrials > 1) {
            summarize();
        }
    }

    void init() noexcept {
        std::uint64_t seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        //std::uint64_t seed = 0x123456789ABCDEF0;
        std::seed_seq ss{uint32_t(seed & 0xffffffff), uint32_t(seed>>32)};
        rng_.seed(ss);
        unif_ = std::uniform_real_distribution<double>(0.0, 1.0);
    }

    double random_number() noexcept {
        double x = unif_(rng_);
        return x;
    }

    void random_probabilities() noexcept {
        double sum = 0.0;
        do {
            p_abc_ = random_number();
            p_acb_ = random_number();
            p_axx_ = random_number();
            p_bac_ = random_number();
            p_bca_ = random_number();
            p_bxx_ = random_number();
            p_cab_ = random_number();
            p_cba_ = random_number();
            p_cxx_ = random_number();
            sum  = p_abc_ + p_acb_ + p_axx_;
            sum += p_bac_ + p_bca_ + p_bxx_;
            sum += p_cab_ + p_cba_ + p_cxx_;
        } while (sum < 0.1);
        /** normalize **/
        p_abc_ /= sum;
        p_acb_ /= sum;
        p_axx_ /= sum;
        p_bac_ /= sum;
        p_bca_ /= sum;
        p_bxx_ /= sum;
        p_cab_ /= sum;
        p_cba_ /= sum;
        p_cxx_ /= sum;

        if (kNVoteTrials == 1) {
            LOG("probability distribution ="
                <<" "<<p_abc_
                <<" "<<p_acb_
                <<" "<<p_axx_
                <<" "<<p_bac_
                <<" "<<p_bca_
                <<" "<<p_bxx_
                <<" "<<p_cab_
                <<" "<<p_cba_
                <<" "<<p_cxx_);
        }
    }

    void random_utilities() noexcept {
        random_utilities(u_abc_a_, u_abc_b_, u_abc_c_);
        random_utilities(u_acb_a_, u_acb_c_, u_acb_b_);
        random_utilities(u_axx_a_, u_axx_x_);

        random_utilities(u_bac_b_, u_bac_a_, u_bac_c_);
        random_utilities(u_bca_b_, u_bca_c_, u_bca_a_);
        random_utilities(u_bxx_b_, u_bxx_x_);

        random_utilities(u_cab_c_, u_cab_a_, u_cab_b_);
        random_utilities(u_cba_c_, u_cba_b_, u_cba_a_);
        random_utilities(u_cxx_c_, u_cxx_x_);
    }

    void random_utilities(
        double &a,
        double &b
    ) noexcept {
        double c;
        double d;
        random_utilities(a, c, d);
        b = (c + d) / 2.0;
        if (kNVoteTrials == 1) {
            LOG("random utilities ="
                <<" "<<a
                <<" "<<b);
        }
    }

    void random_utilities(
        double &a,
        double &b,
        double &c
    ) noexcept {
        std::vector<double> sum(3, 0.0);
        std::vector<double> v(3, 0.0);
        if (kNUtilityTrials > 0) {
            for (int k = 0; k < kNUtilityTrials; ++k) {
                for (int i = 0; i < 3; ++i) {
                    v[i] = random_number();
                }
                std::sort(v.begin(), v.end());
                for (int i = 0; i < 3; ++i) {
                    sum[i] += v[i];
                }
            }
            for (int i = 0; i < 3; ++i) {
                v[i] = sum[i] / double(kNUtilityTrials);
            }
        } else {
            v[0] = 0.25;
            v[1] = 0.50;
            v[2] = 0.75;
        }

        a = v[2];
        b = v[1];
        c = v[0];
        if (kNVoteTrials == 1) {
            LOG("random utilities ="
                <<" "<<a
                <<" "<<b
                <<" "<<c);
        }
    }

    void group_utility() noexcept {
        double u_a = 0.0;
        double u_b = 0.0;
        double u_c = 0.0;

        u_a += p_abc_ * u_abc_a_ + p_acb_ * u_acb_a_ + p_axx_ * u_axx_a_;
        u_a += p_bac_ * u_bac_a_ + p_bca_ * u_bca_a_ + p_bxx_ * u_bxx_x_;
        u_a += p_cab_ * u_cab_a_ + p_cba_ * u_cba_a_ + p_cxx_ * u_cxx_x_;

        u_b += p_abc_ * u_abc_b_ + p_acb_ * u_acb_b_ + p_axx_ * u_axx_x_;
        u_b += p_bac_ * u_bac_b_ + p_bca_ * u_bca_b_ + p_bxx_ * u_bxx_b_;
        u_b += p_cab_ * u_cab_b_ + p_cba_ * u_cba_b_ + p_cxx_ * u_cxx_x_;

        u_c += p_abc_ * u_abc_c_ + p_acb_ * u_acb_c_ + p_axx_ * u_axx_x_;
        u_c += p_bac_ * u_bac_c_ + p_bca_ * u_bca_c_ + p_bxx_ * u_bxx_x_;
        u_c += p_cab_ * u_cab_c_ + p_cba_ * u_cba_c_ + p_cxx_ * u_cxx_c_;

        if (u_a < u_b) {
            std::swap(u_a, u_b);
            swap_probabilities_a_b();
        }
        if (u_b < u_c) {
            std::swap(u_b, u_c);
            swap_probabilities_b_c();
        }
        if (u_a < u_b) {
            std::swap(u_a, u_b);
            swap_probabilities_a_b();
        }

        if (kNVoteTrials == 1) {
            LOG("group utility ="
                <<" A="<<u_a
                <<" B="<<u_b
                <<" C="<<u_c);
            LOG("Group Utility Winner: A");
        }
    }

    void swap_probabilities_a_b() noexcept {
        std::swap(p_abc_, p_bac_);
        std::swap(p_acb_, p_bca_);
        std::swap(p_axx_, p_bxx_);
        std::swap(p_cab_, p_cba_);
    }

    void swap_probabilities_b_c() noexcept {
        std::swap(p_abc_, p_acb_);
        std::swap(p_bac_, p_cab_);
        std::swap(p_bca_, p_cba_);
        std::swap(p_bxx_, p_cxx_);
    }

    void condorcet() noexcept {
        double a_b = p_abc_ + p_acb_ + p_axx_ + p_cab_ - p_bac_ - p_bca_ - p_bxx_ - p_cba_;
        double a_c = p_acb_ + p_abc_ + p_axx_ + p_bac_ - p_cab_ - p_cba_ - p_cxx_ - p_bca_;
        double b_c = p_bca_ + p_bac_ + p_bxx_ + p_abc_ - p_cba_ - p_cab_ - p_cxx_ - p_acb_;
        int a = 0;
        int b = 0;
        int c = 0;
        if (a_b >= 0) {
            ++a;
        } else {
            ++b;
        }
        if (a_c >= 0) {
            ++a;
        } else {
            ++c;
        }
        if (b_c >= 0) {
            ++b;
        } else {
            ++c;
        }
        auto results = create_results(a, b, c);
        condorcet_ = results[2].idx_;
        if (a == 1 && b == 1 && c == 1) {
            condorcet_ = 3;
        }
        if (kNVoteTrials == 1) {
            if (condorcet_ == 3) {
                LOG("Condorcet Winner: Cycle");
            } else {
                LOG("Condorcet Winner:"
                    <<" "<<results[2].name_<<"="<<results[2].score_
                    <<" "<<results[1].name_<<"="<<results[1].score_
                    <<" "<<results[0].name_<<"="<<results[0].score_);
            }
        }
    }

    void first_past_post() noexcept {
        double a = p_abc_ + p_acb_ + p_axx_;
        double b = p_bac_ + p_bca_ + p_bxx_;
        double c = p_cab_ + p_cba_ + p_cxx_;
        auto results = create_results(a, b, c);
        first_past_post_ = results[2].idx_;
        if (kNVoteTrials == 1) {
            LOG("First Past Post Winner:"
                <<" "<<results[2].name_<<"="<<results[2].score_
                <<" "<<results[1].name_<<"="<<results[1].score_
                <<" "<<results[0].name_<<"="<<results[0].score_);
        }
    }

    void ranked_choice_voting() noexcept {
        double a = p_abc_ + p_acb_ + p_axx_;
        double b = p_bac_ + p_bca_ + p_bxx_;
        double c = p_cab_ + p_cba_ + p_cxx_;
        auto round1 = create_results(a, b, c);
        int loser = round1[0].idx_;
        if (loser == 2) {
            /** C was eliminated **/
            a += p_cab_;
            b += p_cba_;
            c = 0.0;
        } else if (loser == 1) {
            /** B was eliminated **/
            a += p_bac_;
            c += p_bca_;
            b = 0.0;
        } else {
            /** A was eliminated **/
            b += p_abc_;
            c += p_acb_;
            a = 0.0;
        }
        auto round2 = create_results(a, b, c);
        ranked_choice_voting_ = round2[2].idx_;
        if (kNVoteTrials == 1) {
            LOG("Ranked Choice Voting Round 1:"
                <<" "<<round1[2].name_<<"="<<round1[2].score_
                <<" "<<round1[1].name_<<"="<<round1[1].score_
                <<" "<<round1[0].name_<<"="<<round1[0].score_);
            LOG("Ranked Choice Voting Winner:"
                <<" "<<round2[2].name_<<"="<<round2[2].score_
                <<" "<<round2[1].name_<<"="<<round2[1].score_);
        }
    }

    void reverse_rank_order() noexcept {
        double a = p_bca_ + p_cba_ + p_bxx_/2.0 + p_cxx_/2.0;
        double b = p_acb_ + p_cab_ + p_axx_/2.0 + p_cxx_/2.0;
        double c = p_bac_ + p_abc_ + p_axx_/2.0 + p_bxx_/2.0;
        auto round1 = create_results(a, b, c);
        int loser = round1[2].idx_;
        a = p_abc_ + p_acb_ + p_axx_;
        b = p_bac_ + p_bca_ + p_bxx_;
        c = p_cab_ + p_cba_ + p_cxx_;
        if (loser == 2) {
            /** C was eliminated **/
            c = 0.0;
        } else if (loser == 1) {
            /** B was eliminated **/
            b = 0.0;
        } else {
            /** A was eliminated **/
            a = 0.0;
        }
        auto round2 = create_results(a, b, c);
        reverse_rank_order_ = round2[2].idx_;
        if (kNVoteTrials == 1) {
            LOG("Reverse Rank Order Round 1:"
                <<" "<<round1[0].name_<<"="<<round1[0].score_
                <<" "<<round1[1].name_<<"="<<round1[1].score_
                <<" "<<round1[2].name_<<"="<<round1[2].score_);
            LOG("Reverse Rank Order Winner:"
                <<" "<<round2[2].name_<<"="<<round2[2].score_
                <<" "<<round2[1].name_<<"="<<round2[1].score_);
        }
    }

    Results create_results(
        double a,
        double b,
        double c
    ) noexcept {
        Results results;
        results.emplace_back(a, 0, "A");
        results.emplace_back(b, 1, "B");
        results.emplace_back(c, 2, "C");
        std::sort(results.begin(), results.end());
        return results;
    }

    void analyze() noexcept {
        switch (condorcet_) {
        case 0:
            ++result_con_a;
            break;
        case 1:
            ++result_con_b;
            break;
        case 2:
            ++result_con_c;
            break;
        case 3:
            ++result_con_cycle;
            break;
        }
        switch (first_past_post_) {
        case 0:
            ++result_fpp_a;
            break;
        case 1:
            ++result_fpp_b;
            break;
        case 2:
            ++result_fpp_c;
            break;
        }
        switch (ranked_choice_voting_) {
        case 0:
            ++result_rcv_a;
            break;
        case 1:
            ++result_rcv_b;
            break;
        case 2:
            ++result_rcv_c;
            break;
        }
        switch (reverse_rank_order_) {
        case 0:
            ++result_rro_a;
            break;
        case 1:
            ++result_rro_b;
            break;
        case 2:
            ++result_rro_c;
            break;
        }
        if (first_past_post_ == condorcet_) {
            ++result_fpp_con;
        }
        if (ranked_choice_voting_ == condorcet_) {
            ++result_rcv_con;
        }
        if (reverse_rank_order_ == condorcet_) {
            ++result_rro_con;
        }
        if (condorcet_ == 3) {
            ++result_fpp_con;
            ++result_rcv_con;
            ++result_rro_con;
        }
        std::vector<int> candidates(4, 0);
        ++candidates[condorcet_];
        ++candidates[first_past_post_];
        ++candidates[ranked_choice_voting_];
        ++candidates[reverse_rank_order_];
        int nwinners = 0;
        for (int i = 0; i < 3; ++i) {
            if (candidates[i] > 0) {
                ++nwinners;
            }
        }
        switch (nwinners) {
        case 1:
            ++result_nwinners_1_;
            break;
        case 2:
            ++result_nwinners_2_;
            break;
        case 3:
            ++result_nwinners_3_;
            break;
        }
    }

    void summarize() noexcept {
        LOG("Agreement with Group Utility:");
        double pct_con_a = int(10000.0 * result_con_a / kNVoteTrials) / 100.0;
        double pct_con_b = int(10000.0 * result_con_b / kNVoteTrials) / 100.0;
        double pct_con_c = int(10000.0 * result_con_c / kNVoteTrials) / 100.0;
        double pct_con_cycle = int(10000.0 * result_con_cycle / kNVoteTrials) / 100.0;
        LOG("Condorcet           : A="<<pct_con_a<<"% B="<<pct_con_b<<"% C="<<pct_con_c<<"% Cycle="<<pct_con_cycle<<"%");
        double pct_fpp_a = int(10000.0 * result_fpp_a / kNVoteTrials) / 100.0;
        double pct_fpp_b = int(10000.0 * result_fpp_b / kNVoteTrials) / 100.0;
        double pct_fpp_c = int(10000.0 * result_fpp_c / kNVoteTrials) / 100.0;
        LOG("First Past Post     : A="<<pct_fpp_a<<"% B="<<pct_fpp_b<<"% C="<<pct_fpp_c<<"%");
        double pct_rcv_a = int(10000.0 * result_rcv_a / kNVoteTrials) / 100.0;
        double pct_rcv_b = int(10000.0 * result_rcv_b / kNVoteTrials) / 100.0;
        double pct_rcv_c = int(10000.0 * result_rcv_c / kNVoteTrials) / 100.0;
        LOG("Ranked Choice Voting: A="<<pct_rcv_a<<"% B="<<pct_rcv_b<<"% C="<<pct_rcv_c<<"%");
        double pct_rro_a = int(10000.0 * result_rro_a / kNVoteTrials) / 100.0;
        double pct_rro_b = int(10000.0 * result_rro_b / kNVoteTrials) / 100.0;
        double pct_rro_c = int(10000.0 * result_rro_c / kNVoteTrials) / 100.0;
        LOG("Reverse Rank Order  : A="<<pct_rro_a<<"% B="<<pct_rro_b<<"% C="<<pct_rro_c<<"%");
        LOG("");
        double nwinners1 = int(10000.0 * result_nwinners_1_ / kNVoteTrials) / 100.0;
        double nwinners2 = int(10000.0 * result_nwinners_2_ / kNVoteTrials) / 100.0;
        double nwinners3 = int(10000.0 * result_nwinners_3_ / kNVoteTrials) / 100.0;
        LOG("Unique Winners      : 1:"<<nwinners1<<"% 2:"<<nwinners2<<"% 3:"<<nwinners3<<"%");
        LOG("");
        LOG("Agreement with Condorcet (includes cycles):");
        double pct_fpp_con = int(10000.0 * result_fpp_con / kNVoteTrials) / 100.0;
        double pct_rcv_con = int(10000.0 * result_rcv_con / kNVoteTrials) / 100.0;
        double pct_rro_con = int(10000.0 * result_rro_con / kNVoteTrials) / 100.0;
        LOG("First Past Post     : "<<pct_fpp_con<<"%");
        LOG("Ranked Choice Voting: "<<pct_rcv_con<<"%");
        LOG("Reverse Rank Order  : "<<pct_rro_con<<"%");
    }
};

} // anonymous namespace

GeneratedDataVoting::GeneratedDataVoting() noexcept {
    impl_ = (void *) new VotingImpl;
}

GeneratedDataVoting::~GeneratedDataVoting() noexcept {
    auto impl = (VotingImpl *) impl_;
    delete impl;
}

void GeneratedDataVoting::run() noexcept {
    auto impl = (VotingImpl *) impl_;
    impl->run();
}
