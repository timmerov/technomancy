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
#include <iomanip>
#include <random>

namespace {

/** number of trials to run. **/
//constexpr int kNVoteTrials = 1;
//constexpr int kNVoteTrials = 100;
constexpr int kNVoteTrials = 10*1000;

/** uniform utilities or not. **/
//constexpr int kNUtilityTrials = 0;  // no randomness. use expectation values.
constexpr int kNUtilityTrials = 1;
//constexpr int kNUtilityTrials = 3;

/** fixed seed or random seed **/
constexpr bool kRandomSeed = true;
//constexpr bool kRandomSeed = false;

/** extra logging **/
constexpr bool kVerbose = false;
//constexpr bool kVerbose = true;

/**
add the front-runner rule to reverse rank order voting.
if the front-runner gets this threshold of the votes they win.
even if they are simultaneously the least popular candidate.
**/
constexpr double kRevFrontRunner = 1.0;
//constexpr double kRevFrontRunner = 0.55;
//constexpr double kRevFrontRunner = 0.50;


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
    int result_con_a_ = 0;
    int result_con_b_ = 0;
    int result_con_c_ = 0;
    int result_con_cycle_ = 0;
    int result_con_strategic_ = 0;
    int first_past_post_ = 0;
    int result_fpp_a_ = 0;
    int result_fpp_b_ = 0;
    int result_fpp_c_ = 0;
    int result_fpp_con_ = 0;
    int result_fpp_strategic_ = 0;
    int result_fpp_counter_= 0;
    int ranked_choice_voting_ = 0;
    int result_rcv_a_ = 0;
    int result_rcv_b_ = 0;
    int result_rcv_c_ = 0;
    int result_rcv_con_ = 0;
    int result_rcv_fpp_ = 0;
    int result_rcv_strategic_ = 0;
    int result_rcv_alternate_ = 0;
    int reverse_rank_order_ = 0;
    int result_rev_a_ = 0;
    int result_rev_b_ = 0;
    int result_rev_c_ = 0;
    int result_rev_con_ = 0;
    int result_rev_fpp_ = 0;
    int result_rev_rcv_ = 0;
    int result_rev_strategic_ = 0;
    int result_rev_alternate_ = 0;
    int approval_ = 0;
    int result_app_a_ = 0;
    int result_app_b_ = 0;
    int result_app_c_ = 0;
    int result_app_con_ = 0;
    int result_app_fpp_ = 0;
    int result_app_rcv_ = 0;
    int result_app_rev_ = 0;
    int result_nwinners_1_ = 0;
    int result_nwinners_2_ = 0;
    int result_nwinners_3_ = 0;
    int result_rev_majority_lost_ = 0;
    int result_rev_front_runner_ = 0;
    double result_rev_majority_lost_max_ = 0.0;
    double result_rev_majority_lost_first_sum_ = 0.0;
    double result_rev_majority_lost_last_sum_ = 0.0;

    /** random number generation **/
    std::uint64_t trial_seed_;
    std::mt19937_64 rng_;
    std::uniform_real_distribution<double> unif_;

    /** internal state **/
    int trial_ = 0;

    void run() noexcept {
        LOG("Synthetic Electorate."<<std::fixed<<std::setprecision(2));
        init();

        for (trial_ = 0; trial_ < kNVoteTrials; ++trial_) {
            random_probabilities();
            random_utilities();
            group_utility();
            condorcet();
            first_past_post();
            ranked_choice_voting();
            reverse_rank_order();
            approval_voting();
            analyze();
        }

        if (kNVoteTrials > 1) {
            summarize();
        }
    }

    void init() noexcept {
        std::uint64_t seed;
        if (kRandomSeed) {
            seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        } else {
            /** standard seed **/
            //seed = 0x123456789ABCDEF0;
            /**
            A is both the most popular and least popular.
            nwinners=2 CON=0 FPP=0 RCV=0 REV=1
            ABC=0.123253 ACB=0.185715 AXX=0.188619 A1=0.497587 A3=0.340093
            BAC=0.0544597 BCA=0.185522 BXX=0.110022 B1=0.350003 B3=0.332875
            CAB=0.0459297 CBA=0.0926401 CXX=0.0138403 C1=0.15241 C3=0.327032
            **/
            //seed = 1602873873118686564;
            /**
            B wins first past post.
            A wins cordocet A > B > C.
            B voters have incentive to switch votes to CBA.
            Ranked Choice Voting Round 1: B=0.379835 A=0.320321 C=0.299844
            Ranked Choice Voting Winner: A=0.519308 B=0.475759
            **/
            seed = 1602879358922335402;
            /**
            Reverse Rank Order Round 1: A=0.280917 B=0.287955 C=0.431127
            Reverse Rank Order Winner: A=0.504648 B=0.3792
            **/
            seed = 1602891851701679826;
        }
        std::seed_seq ss{uint32_t(seed & 0xffffffff), uint32_t(seed>>32)};
        rng_.seed(ss);
        unif_ = std::uniform_real_distribution<double>(0.0, 1.0);
        LOG("Random Seed: "<<seed);
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
            if (kVerbose) {
                LOG("  A-B: "<<a_b<<" A-C: "<<a_c<<" B-C: "<<b_c);
            }
        }

        /**
        strategic voting for condorcet post means...
        assume the results are ABC.
        BAC voters have incentive to vote BCA.
        **/
        if (condorcet_ != 3) {
            int winner = results[2].idx_;
            int second = results[1].idx_;
            int loser = results[0].idx_;
            if (winner == 0 && second == 1 && loser == 2) {
                check_con_strategic_voting("BAC", a_c, p_bac_);
            }
            if (winner == 0 && second == 2 && loser == 1) {
                check_con_strategic_voting("CAB", a_b, p_cab_);
            }
            if (winner == 1 && second == 0 && loser == 2) {
                check_con_strategic_voting("ABC", b_c, p_abc_);
            }
            if (winner == 1 && second == 2 && loser == 0) {
                check_con_strategic_voting("CBA", - a_b, p_cba_);
            }
            if (winner == 2 && second == 0 && loser == 1) {
                check_con_strategic_voting("ACB", - b_c, p_acb_);
            }
            if (winner == 2 && second == 1 && loser == 0) {
                check_con_strategic_voting("BCA", - a_c, p_cab_);
            }
        }
    }

    void check_con_strategic_voting(
        const char *cls,
        double a_c,
        double swing
    ) noexcept {
        a_c -= 2.0 * swing;
        if (a_c < 0.0) {
            ++result_con_strategic_;
            if (kVerbose) {
                LOG(cls<<" should vote strategically.");
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

        /**
        strategic voting for first past the post means...
        voters for the candidate expected to finish last change their
        votes to the their second place choice to avoid being stuck with
        their last place choice.
        **/
        int winner = results[2].idx_;
        int second = results[1].idx_;
        int loser = results[0].idx_;
        double winner_score = results[2].score_;
        double second_score = results[1].score_;
        if (winner == 0 && second == 1 && loser == 2) {
            check_fpp_strategic_voting("CBA", "CAB", winner_score, second_score, p_cba_, p_cab_);
        }
        if (winner == 0 && second == 2 && loser == 1) {
            check_fpp_strategic_voting("BCA", "BAC", winner_score, second_score, p_bca_, p_bac_);
        }
        if (winner == 1 && second == 0 && loser == 2) {
            check_fpp_strategic_voting("CAB", "CBA", winner_score, second_score, p_cab_, p_cba_);
        }
        if (winner == 1 && second == 2 && loser == 0) {
            check_fpp_strategic_voting("ACB", "ABC", winner_score, second_score, p_acb_, p_abc_);
        }
        if (winner == 2 && second == 0 && loser == 1) {
            check_fpp_strategic_voting("BAC", "BCA", winner_score, second_score, p_bac_, p_bca_);
        }
        if (winner == 2 && second == 1 && loser == 0) {
            check_fpp_strategic_voting("ABC", "ACB", winner_score, second_score, p_abc_, p_acb_);
        }
    }

    void check_fpp_strategic_voting(
        const char *cls1,
        const char *cls2,
        double winner,
        double second,
        double swing1,
        double swing2
    ) noexcept {
        double second_plus = second + swing1;
        double winner_plus = winner + swing2;
        if (second_plus > winner) {
            /**
            class 1 can change the outcome from their last place choice
            to their second place choice by changing their vote.
            **/
            ++result_fpp_strategic_;
            if (kVerbose) {
                LOG(cls1<<" should vote strategically: "
                    <<second<<"+"<<swing1<<"="<<second_plus<<" > "<<winner);
            }
            if (winner_plus > second_plus) {
                /**
                when class 1 changes the outcome...
                class 2 can change it back by also voting for their second choice.
                **/
                ++result_fpp_counter_;
                if (kVerbose) {
                    LOG(cls2<<" should also vote strategically: "
                        <<second<<"+"<<swing1<<"="<<second_plus<<" < "
                        <<winner_plus<<"="<<winner<<"+"<<swing2);
                }
            }
        }
    }

    void ranked_choice_voting() noexcept {
        double a = p_abc_ + p_acb_ + p_axx_;
        double b = p_bac_ + p_bca_ + p_bxx_;
        double c = p_cab_ + p_cba_ + p_cxx_;
        auto round1 = create_results(a, b, c);
        int loser = round1[0].idx_;
        double a_plus = a;
        double b_plus = b;
        double c_plus = c;
        if (loser == 2) {
            /** C was eliminated **/
            a_plus += p_cab_;
            b_plus += p_cba_;
            c_plus = 0.0;
        } else if (loser == 1) {
            /** B was eliminated **/
            a_plus += p_bac_;
            c_plus += p_bca_;
            b_plus = 0.0;
        } else {
            /** A was eliminated **/
            b_plus += p_abc_;
            c_plus += p_acb_;
            a_plus = 0.0;
        }
        auto round2 = create_results(a_plus, b_plus, c_plus);
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

        /**
        strategic voting for ranked choice voting is different from
        strategic voting for first past the post.
        voters for the last place candidate automatically have their
        votes redirected to their second choice in the second round.
        which is an improvement over first past the post.
        voters can vote for their first choice without fear of being a spoiler.
        nor of being stuck with their last choice.

        however...
        voters for the second place candidate might want to vote strategically.
        for example: voter preference is A > B > C.
        B voters have incentive to vote C.
        it's risky because B may end up eliminated in the first round.
        and because C may end up winning in the second round.
        **/
        int winner = round2[2].idx_;
        int second = round2[1].idx_;
        if (winner == 0 && second == 1 && loser == 2) {
            check_rcv_strategic_voting("B", "BCA", a, b, c, p_abc_, p_acb_, p_bca_, p_bac_);
        }
        if (winner == 0 && second == 2 && loser == 1) {
            check_rcv_strategic_voting("C", "CBA", a, c, b, p_acb_, p_abc_, p_cba_, p_cab_);
        }
        if (winner == 1 && second == 0 && loser == 2) {
            check_rcv_strategic_voting("A", "ACB", b, a, c, p_bac_, p_bca_, p_acb_, p_abc_);
        }
        if (winner == 1 && second == 2 && loser == 0) {
            check_rcv_strategic_voting("C", "CAB", b, c, a, p_bca_, p_bac_, p_cab_, p_cba_);
        }
        if (winner == 2 && second == 0 && loser == 1) {
            check_rcv_strategic_voting("A", "ABC", c, a, b, p_cab_, p_cba_, p_abc_, p_acb_);
        }
        if (winner == 2 && second == 1 && loser == 0) {
            check_rcv_strategic_voting("B", "BAC", c, b, a, p_cba_, p_cab_, p_bac_, p_bca_);
        }
    }

    void check_rcv_strategic_voting(
        const char *cls1,
        const char *cls2,
        double a,
        double b,
        double c,
        double abc,
        double acb,
        double bca,
        double bac
    ) noexcept {
        /**
        winner=A, second=B, last=C.
        B might want to shift X votes to C.
        want B - X > A so B is not eliminated.
        want C + X > A so C is not eliminated.
        also need B - X + ABC > C + X + ACB so we don't lose in the second round.
        ergo X < B - A and X > A - C and X < (B + ABC - C - ACB) / 2.
        **/
        double bma = b - a;
        double amc = a - c;
        double bmc = (b + abc - c - acb) / 2.0;
        double min_x = amc;
        double max_x = std::min(bma, bmc);
        if (max_x > min_x && min_x > 0.0) {
            ++result_rcv_strategic_;
            if (kVerbose) {
                LOG(cls1<<" should vote strategically. min="<<min_x<<" max="<<max_x);
            }
            return;
        }

        /**
        B cannot win.
        BCA might want to shift to C.
        **/
        double a2 = a + bac;
        double b2 = b - bca;
        double c2 = c + bca;
        if (c2 > a2 && a2 > b2) {
            ++result_rcv_alternate_;
            if (kVerbose) {
                LOG(cls2<<" should vote alternate strategy. c2="<<c2<<" a2="<<a2<<" b2="<<b2);
            }
        }
    }

    void reverse_rank_order() noexcept {
        /** first place votes **/
        double front_a = p_abc_ + p_acb_ + p_axx_;
        double front_b = p_bac_ + p_bca_ + p_bxx_;
        double front_c = p_cab_ + p_cba_ + p_cxx_;
        auto front_results = create_results(front_a, front_b, front_c);
        int front_winner = front_results[2].idx_;
        double front_score = front_results[2].score_;

        /** count last place votes **/
        double last_a = p_bca_ + p_cba_ + p_bxx_/2.0 + p_cxx_/2.0;
        double last_b = p_acb_ + p_cab_ + p_axx_/2.0 + p_cxx_/2.0;
        double last_c = p_bac_ + p_abc_ + p_axx_/2.0 + p_bxx_/2.0;
        auto round1 = create_results(last_a, last_b, last_c);
        /** eliminate the candidate with the most last place votes. **/
        int loser = round1[2].idx_;
        double round2_a = p_abc_ + p_acb_ + p_axx_;
        double round2_b = p_bac_ + p_bca_ + p_bxx_;
        double round2_c = p_cab_ + p_cba_ + p_cxx_;
        if (loser == 2) {
            /** C was eliminated **/
            round2_a += p_cab_;
            round2_b += p_cba_;
            round2_c = 0.0;
        } else if (loser == 1) {
            /** B was eliminated **/
            round2_a += p_bac_;
            round2_c += p_bca_;
            round2_b = 0.0;
        } else {
            /** A was eliminated **/
            round2_b += p_abc_;
            round2_c += p_acb_;
            round2_a = 0.0;
        }
        auto round2 = create_results(round2_a, round2_b, round2_c);
        int rev_winner = round2[2].idx_;

        /** determine winner **/
        if (front_score > kRevFrontRunner) {
            reverse_rank_order_ = front_winner;
            ++result_rev_front_runner_;
            if (kNVoteTrials == 1) {
                LOG("Reverse Rank Order Front-runner Wins: "
                    <<front_results[2].name_<<" "<<front_score);
            }
        } else {
            reverse_rank_order_ = rev_winner;
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

        /** strategic voting **/
        int winner = round2[2].idx_;
        int second = round2[1].idx_;
        int last   = round2[0].idx_;
        if (winner == 0 && second == 1 && last == 2 && second != reverse_rank_order_) {
            double b = p_bac_ + p_bca_ + p_bxx_ + p_abc_;
            double c = p_cab_ + p_cba_ + p_cxx_ + p_acb_;
            check_rev_strategic_voting("B", "C", b, c, last_a, last_b, last_c, p_bac_, p_bxx_, p_cab_, p_cxx_);
        }
        if (winner == 0 && second == 2 && last == 1 && second != reverse_rank_order_) {
            double b = p_bac_ + p_bca_ + p_bxx_ + p_abc_;
            double c = p_cab_ + p_cba_ + p_cxx_ + p_acb_;
            check_rev_strategic_voting("C", "B", c, b, last_a, last_c, last_b, p_cab_, p_cxx_, p_bac_, p_bxx_);
        }
        if (winner == 1 && second == 0 && last == 2 && second != reverse_rank_order_) {
            double a = p_abc_ + p_acb_ + p_axx_ + p_bac_;
            double c = p_cab_ + p_cba_ + p_cxx_ + p_acb_;
            check_rev_strategic_voting("A", "C", a, c, last_b, last_a, last_c, p_abc_, p_axx_, p_cba_, p_cxx_);
        }
        if (winner == 1 && second == 2 && last == 0 && second != reverse_rank_order_) {
            double a = p_abc_ + p_acb_ + p_axx_ + p_bac_;
            double c = p_cab_ + p_cba_ + p_cxx_ + p_acb_;
            check_rev_strategic_voting("C", "A", c, a, last_b, last_c, last_a, p_cba_, p_cxx_, p_abc_, p_axx_);
        }
        if (winner == 2 && second == 0 && last == 1 && second != reverse_rank_order_) {
            double a = p_acb_ + p_abc_ + p_axx_ + p_cab_;
            double b = p_bca_ + p_bac_ + p_bxx_ + p_cba_;
            check_rev_strategic_voting("A", "B", a, b, last_c, last_a, last_b, p_acb_, p_axx_, p_bca_, p_bxx_);
        }
        if (winner == 2 && second == 1 && last == 0 && second != reverse_rank_order_) {
            double a = p_acb_ + p_abc_ + p_axx_ + p_cab_;
            double b = p_bca_ + p_bac_ + p_bxx_ + p_cba_;
            check_rev_strategic_voting("B", "A", b, a, last_c, last_b, last_a, p_bca_, p_bxx_, p_acb_, p_axx_);
        }

        /** track when the front runner loses with a majority of the popular vote. **/
        if (front_score > 0.50 && front_winner != reverse_rank_order_) {
            ++result_rev_majority_lost_;
            switch (front_winner) {
            case 0:
                result_rev_majority_lost_max_ = std::max(result_rev_majority_lost_max_, front_a);
                result_rev_majority_lost_first_sum_ += front_a;
                result_rev_majority_lost_last_sum_ += last_a;
                break;
            case 1:
                result_rev_majority_lost_max_ = std::max(result_rev_majority_lost_max_, front_b);
                result_rev_majority_lost_first_sum_ += front_b;
                result_rev_majority_lost_last_sum_ += last_b;
                break;
            case 2:
                result_rev_majority_lost_max_ = std::max(result_rev_majority_lost_max_, front_c);
                result_rev_majority_lost_first_sum_ += front_c;
                result_rev_majority_lost_last_sum_ += last_b;
                break;
            }
            if (kVerbose) {
                LOG(front_results[2].name_<<" had "<<front_score<<" first place votes and lost.");
            }
        }
    }

    void check_rev_strategic_voting(
        const char *nameb,
        const char *namec,
        double b,
        double c,
        double last_a,
        double last_b,
        double last_c,
        double p_bac,
        double p_cab,
        double p_bxx,
        double p_cxx
    ) noexcept {
        if (b > c) {
            /**
            B has incentive to move last place votes from C to A
            so that A is eliminated instead of C.

            a = last_a + X
            b = last_b
            c = last_c - X
            a > b and a > c

            last_a + X > last_b
            last_a + X > last_c - X

            X > last_b - last_a
            X > (last_c - last_a)/2
            X < p_bac + p_bxx/2
            **/
            double bma = last_b - last_a;
            double cma = (last_c - last_a) / 2.0;
            double max_x = p_bac + p_bxx/2.0;
            double min_x = std::max(bma, cma);
            if (max_x > min_x && min_x > 0.0) {
                ++result_rev_strategic_;
                if (kVerbose) {
                        LOG(nameb<<" should vote strategically. bma="<<bma<<" cma="<<cma<<" max="<<max_x<<" min="<<min_x);
                        LOG("  last_a="<<last_a<<" last_b="<<last_b<<" last_c="<<last_c);
                        LOG("  b="<<b<<" c="<<c);
                        print_electorate();
                }
            } else {
                if (kVerbose) {
                    LOG(nameb<<" should vote tactically.");
                }
            }
        } else {
            /**
            C has incentive to move last place votes from B to A
            so that A is eliminated instead of B.
            **/
            double cma = last_c - last_a;
            double bma = (last_b - last_a) / 2.0;
            double max_x = p_cab + p_cxx/2.0;
            double min_x = std::max(cma, bma);
            if (max_x > min_x && min_x > 0.0) {
                ++result_rev_alternate_;
                if (kVerbose) {
                    LOG(namec<<" should vote strategically. cma="<<cma<<" bma="<<bma<<" max="<<max_x<<" min="<<min_x);
                    LOG("  last_a="<<last_a<<" last_c="<<last_c<<" last_b="<<last_b);
                    LOG("  c="<<c<<" b="<<b);
                    print_electorate();
                }
            } else {
                if (kVerbose) {
                    LOG(namec<<" should vote tactically.");
                }
            }
        }
    }

    void approval_voting() noexcept {
        double a = p_abc_ + p_acb_ + p_axx_ + p_bac_ + p_cab_;
        double b = p_bac_ + p_bca_ + p_bxx_ + p_abc_ + p_cba_;
        double c = p_cab_ + p_cba_ + p_cxx_ + p_acb_ + p_bca_;
        auto results = create_results(a, b, c);
        approval_ = results[2].idx_;
        if (kNVoteTrials == 1) {
            LOG("Approval Winner:"
                <<" "<<results[2].name_<<"="<<results[2].score_
                <<" "<<results[1].name_<<"="<<results[1].score_
                <<" "<<results[0].name_<<"="<<results[0].score_);
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
            ++result_con_a_;
            break;
        case 1:
            ++result_con_b_;
            break;
        case 2:
            ++result_con_c_;
            break;
        case 3:
            ++result_con_cycle_;
            break;
        }
        switch (first_past_post_) {
        case 0:
            ++result_fpp_a_;
            break;
        case 1:
            ++result_fpp_b_;
            break;
        case 2:
            ++result_fpp_c_;
            break;
        }
        switch (ranked_choice_voting_) {
        case 0:
            ++result_rcv_a_;
            break;
        case 1:
            ++result_rcv_b_;
            break;
        case 2:
            ++result_rcv_c_;
            break;
        }
        switch (reverse_rank_order_) {
        case 0:
            ++result_rev_a_;
            break;
        case 1:
            ++result_rev_b_;
            break;
        case 2:
            ++result_rev_c_;
            break;
        }
        switch (approval_) {
        case 0:
            ++result_app_a_;
            break;
        case 1:
            ++result_app_b_;
            break;
        case 2:
            ++result_app_c_;
            break;
        }
        if (first_past_post_ == condorcet_) {
            ++result_fpp_con_;
        }
        if (ranked_choice_voting_ == condorcet_) {
            ++result_rcv_con_;
        }
        if (reverse_rank_order_ == condorcet_) {
            ++result_rev_con_;
        }
        if (approval_ == condorcet_) {
            ++result_app_con_;
        }
        if (condorcet_ == 3) {
            ++result_fpp_con_;
            ++result_rcv_con_;
            ++result_rev_con_;
            ++result_app_con_;
        }
        if (ranked_choice_voting_ == first_past_post_) {
            ++result_rcv_fpp_;
        }
        if (reverse_rank_order_ == first_past_post_) {
            ++result_rev_fpp_;
        }
        if (approval_ == first_past_post_) {
            ++result_app_fpp_;
        }
        if (reverse_rank_order_ == ranked_choice_voting_) {
            ++result_rev_rcv_;
        }
        if (approval_ == ranked_choice_voting_) {
            ++result_app_rcv_;
        }
        if (approval_ == reverse_rank_order_) {
            ++result_app_rev_;
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
        if (kVerbose
        &&  reverse_rank_order_ != first_past_post_
        &&  reverse_rank_order_ != ranked_choice_voting_) {

            LOG(trial_<<": nwinners="<<nwinners
                <<" CON="<<condorcet_
                <<" FPP="<<first_past_post_
                <<" RCV="<<ranked_choice_voting_
                <<" REV="<<reverse_rank_order_);
            print_electorate();
        }
    }

    void print_electorate() noexcept {
        double first_a = p_abc_ + p_acb_ + p_axx_;
        double first_b = p_bac_ + p_bca_ + p_bxx_;
        double first_c = p_cab_ + p_cba_ + p_cxx_;
        double last_a = p_bca_ + p_cba_ + p_bxx_/2.0 + p_cxx_/2.0;
        double last_b = p_acb_ + p_cab_ + p_axx_/2.0 + p_cxx_/2.0;
        double last_c = p_bac_ + p_abc_ + p_axx_/2.0 + p_bxx_/2.0;
        LOG("  ABC="<<p_abc_<<" ACB="<<p_acb_<<" AXX="<<p_axx_<<" A1="<<first_a<<" A3="<<last_a);
        LOG("  BAC="<<p_bac_<<" BCA="<<p_bca_<<" BXX="<<p_bxx_<<" B1="<<first_b<<" B3="<<last_b);
        LOG("  CAB="<<p_cab_<<" CBA="<<p_cba_<<" CXX="<<p_cxx_<<" C1="<<first_c<<" C3="<<last_c);
    }

    void summarize() noexcept {
        LOG("Number of Trials    : "<<kNVoteTrials);
        LOG("Utility Distribution: "<<kNUtilityTrials);
        LOG("");
        LOG("Agreement with Group Utility:");
        double pct_con_a = 100.0 * result_con_a_ / kNVoteTrials;
        double pct_con_b = 100.0 * result_con_b_ / kNVoteTrials;
        double pct_con_c = 100.0 * result_con_c_ / kNVoteTrials;
        double pct_con_cycle = 100.0 * result_con_cycle_ / kNVoteTrials;
        LOG("Condorcet           : A="<<pct_con_a<<"% B="<<pct_con_b<<"% C="<<pct_con_c<<"% Cycle="<<pct_con_cycle<<"%");
        double pct_fpp_a = 100.0 * result_fpp_a_ / kNVoteTrials;
        double pct_fpp_b = 100.0 * result_fpp_b_ / kNVoteTrials;
        double pct_fpp_c = 100.0 * result_fpp_c_ / kNVoteTrials;
        LOG("First Past Post     : A="<<pct_fpp_a<<"% B="<<pct_fpp_b<<"% C="<<pct_fpp_c<<"%");
        double pct_rcv_a = 100.0 * result_rcv_a_ / kNVoteTrials;
        double pct_rcv_b = 100.0 * result_rcv_b_ / kNVoteTrials;
        double pct_rcv_c = 100.0 * result_rcv_c_ / kNVoteTrials;
        LOG("Ranked Choice Voting: A="<<pct_rcv_a<<"% B="<<pct_rcv_b<<"% C="<<pct_rcv_c<<"%");
        double pct_rev_a = 100.0 * result_rev_a_ / kNVoteTrials;
        double pct_rev_b = 100.0 * result_rev_b_ / kNVoteTrials;
        double pct_rev_c = 100.0 * result_rev_c_ / kNVoteTrials;
        LOG("Reverse Rank Order  : A="<<pct_rev_a<<"% B="<<pct_rev_b<<"% C="<<pct_rev_c<<"%");
        double pct_app_a = 100.0 * result_app_a_ / kNVoteTrials;
        double pct_app_b = 100.0 * result_app_b_ / kNVoteTrials;
        double pct_app_c = 100.0 * result_app_c_ / kNVoteTrials;
        LOG("Approval            : A="<<pct_app_a<<"% B="<<pct_app_b<<"% C="<<pct_app_c<<"%");
        LOG("");
        double pct_fpp_con = 100.0 * result_fpp_con_ / kNVoteTrials;
        double pct_rcv_con = 100.0 * result_rcv_con_ / kNVoteTrials;
        double pct_rev_con = 100.0 * result_rev_con_ / kNVoteTrials;
        double pct_app_con = 100.0 * result_app_con_ / kNVoteTrials;
        double pct_rcv_fpp = 100.0 * result_rcv_fpp_ / kNVoteTrials;
        double pct_rev_fpp = 100.0 * result_rev_fpp_ / kNVoteTrials;
        double pct_app_fpp = 100.0 * result_app_fpp_ / kNVoteTrials;
        double pct_rev_rcv = 100.0 * result_rev_rcv_ / kNVoteTrials;
        double pct_app_rcv = 100.0 * result_app_rcv_ / kNVoteTrials;
        double pct_app_rev = 100.0 * result_app_rev_ / kNVoteTrials;
        LOG("Agreements          :  FPP     RCV     REV     APP");
        LOG("Condorcet           : "<<pct_fpp_con<<"%  "<<pct_rcv_con<<"%  "<<pct_rev_con<<"%  "<<pct_app_con<<"%");
        LOG("First Past Post     :         "<<pct_rcv_fpp<<"%  "<<pct_rev_fpp<<"%  "<<pct_app_fpp<<"%");
        LOG("Ranked Choice Voting:                 "<<pct_rev_rcv<<"%  "<<pct_app_rcv<<"%");
        LOG("Reverse Rank Order  :                         "<<pct_app_rev<<"%");
        LOG("");
        double nwinners1 = 100.0 * result_nwinners_1_ / kNVoteTrials;
        double nwinners2 = 100.0 * result_nwinners_2_ / kNVoteTrials;
        double nwinners3 = 100.0 * result_nwinners_3_ / kNVoteTrials;
        LOG("Unique Winners      : 1:"<<nwinners1<<"% 2:"<<nwinners2<<"% 3:"<<nwinners3<<"%");
        LOG("");
        LOG("Strategic Voting:");
        double pct_con_strategic = 100.0 * result_con_strategic_ / kNVoteTrials;
        LOG("Condorcet           : "<<pct_con_strategic<<"%");
        double pct_fpp_strategic = 100.0 * result_fpp_strategic_ / kNVoteTrials;
        double pct_fpp_counter = 100.0 * result_fpp_counter_ / kNVoteTrials;
        LOG("First Past Post     : "<<pct_fpp_strategic<<"% counter: "<<pct_fpp_counter<<"%");
        double pct_rcv_strategic = 100.0 * result_rcv_strategic_ / kNVoteTrials;
        double pct_rcv_alternate = 100.0 * result_rcv_alternate_ / kNVoteTrials;
        LOG("Ranked Choice Voting: "<<pct_rcv_strategic<<"% alternate: "<<pct_rcv_alternate<<"%");
        double pct_rev_strategic = 100.0 * result_rev_strategic_ / kNVoteTrials;
        double pct_rev_alternate = 100.0 * result_rev_alternate_ / kNVoteTrials;
        LOG("Reverse Rank Order  : "<<pct_rev_strategic<<"% alternate: "<<pct_rev_alternate<<"%");
        LOG("");
        LOG("Oddities:");
        LOG("Ranked Choice Voting:");
        double pct_rev_front_runner = 100.0 * result_rev_front_runner_ / kNVoteTrials;
        LOG("Won by Front-Runner Rule: "<<pct_rev_front_runner<<"%");
        double pct_rev_majority_lost = 100.0 * result_rev_majority_lost_ / kNVoteTrials;
        double pct_rev_majority_lost_max = 100.0 * result_rev_majority_lost_max_;
        double pct_rev_majority_lost_first = 100.0 * result_rev_majority_lost_first_sum_ / result_rev_majority_lost_;
        double pct_rev_majority_lost_last = 100.0 * result_rev_majority_lost_last_sum_ / result_rev_majority_lost_;
        LOG("Lost with Majority of Popular Vote: "<<pct_rev_majority_lost<<"%");
        LOG("  max first place: "<<pct_rev_majority_lost_max<<"% average first place: "<<pct_rev_majority_lost_first
            <<"% average last place: "<<pct_rev_majority_lost_last<<"%");
        LOG("");
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
