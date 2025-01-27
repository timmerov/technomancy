/*
Copyright (C) 2012-2025 tim cotter. All rights reserved.
*/

/**
compare voting systems:
- condorcet
- first past the post
- ranked choice voting
- reverse rank order voting
- approval voting
- contingent proxy voting

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
//constexpr int kNVoteTrials = 1000*1000;

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

class Electorate {
public:
    double abc_ = 0.0;
    double acb_ = 0.0;
    double axx_ = 0.0;
    double bac_ = 0.0;
    double bca_ = 0.0;
    double bxx_ = 0.0;
    double cab_ = 0.0;
    double cba_ = 0.0;
    double cxx_ = 0.0;
    const char *a_ = "A";
    const char *b_ = "B";
    const char *c_ = "C";

    void print() noexcept {
        double first_a = abc_ + acb_ + axx_;
        double first_b = bac_ + bca_ + bxx_;
        double first_c = cab_ + cba_ + cxx_;
        double last_a = bca_ + cba_ + bxx_/2.0 + cxx_/2.0;
        double last_b = acb_ + cab_ + axx_/2.0 + cxx_/2.0;
        double last_c = bac_ + abc_ + axx_/2.0 + bxx_/2.0;
        LOG("  ABC="<<abc_<<" ACB="<<acb_<<" AXX="<<axx_<<" A1="<<first_a<<" A3="<<last_a);
        LOG("  BAC="<<bac_<<" BCA="<<bca_<<" BXX="<<bxx_<<" B1="<<first_b<<" B3="<<last_b);
        LOG("  CAB="<<cab_<<" CBA="<<cba_<<" CXX="<<cxx_<<" C1="<<first_c<<" C3="<<last_c);
    }

    void swap_ab() noexcept {
        std::swap(abc_, bac_);
        std::swap(acb_, bca_);
        std::swap(axx_, bxx_);
        std::swap(cab_, cba_);
        std::swap(a_, b_);
    }

    void swap_ac() noexcept {
        std::swap(abc_, cba_);
        std::swap(acb_, cab_);
        std::swap(axx_, cxx_);
        std::swap(bac_, bca_);
        std::swap(a_, c_);
    }

    void swap_bc() noexcept {
        std::swap(abc_, acb_);
        std::swap(bac_, cab_);
        std::swap(bxx_, cxx_);
        std::swap(bca_, cba_);
        std::swap(b_, c_);
    }

    Electorate normalize(
        Results& results
    ) noexcept {
        Electorate p = *this;
        int winner = results[2].idx_;
        int second = results[1].idx_;
        int loser = results[0].idx_;
        if (winner == 1) {
            p.swap_ab();
            std::swap(winner, second);
        } else if (winner == 2) {
            p.swap_ac();
            std::swap(winner, loser);
        }
        if (second == 2) {
            p.swap_bc();
            std::swap(second, loser);
        }
        return p;
    }
};

class VotingImpl {
public:
    VotingImpl() = default;
    VotingImpl(const VotingImpl &) = delete;
    VotingImpl(VotingImpl &&) = delete;
    ~VotingImpl() = default;

    /** probability distributions (9 of 12) **/
    Electorate p_;
    Electorate pnorm_;

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
    int result_con_alternate_ = 0;
    int result_con_neither_ = 0;
    int first_past_post_ = 0;
    int result_fpp_a_ = 0;
    int result_fpp_b_ = 0;
    int result_fpp_c_ = 0;
    int result_fpp_con_ = 0;
    int result_fpp_strategic_ = 0;
    int result_fpp_counter_= 0;
    int ranked_choice_ = 0;
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
    int approval_ = 0;
    int result_app_a_ = 0;
    int result_app_b_ = 0;
    int result_app_c_ = 0;
    int result_app_con_ = 0;
    int result_app_fpp_ = 0;
    int result_app_rcv_ = 0;
    int result_app_rev_ = 0;
    int result_app_strategic_ = 0;
    int result_app_alternate_ = 0;
    int result_app_collude_ = 0;
    int contingent_proxy_ = 0;
    int result_cpv_a_ = 0;
    int result_cpv_b_ = 0;
    int result_cpv_c_ = 0;
    int result_cpv_con_ = 0;
    int result_cpv_fpp_ = 0;
    int result_cpv_rcv_ = 0;
    int result_cpv_rev_ = 0;
    int result_cpv_app_ = 0;
    int result_nwinners_1_ = 0;
    int result_nwinners_2_ = 0;
    int result_nwinners_3_ = 0;
    int result_nwinners_4_ = 0;
    int result_nwinners_5_ = 0;
    int result_nwinners_6_ = 0;
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
            contingent_proxy_voting();
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
            //seed = 1602879358922335402;
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
            p_.abc_ = random_number();
            p_.acb_ = random_number();
            p_.axx_ = random_number();
            p_.bac_ = random_number();
            p_.bca_ = random_number();
            p_.bxx_ = random_number();
            p_.cab_ = random_number();
            p_.cba_ = random_number();
            p_.cxx_ = random_number();
            sum  = p_.abc_ + p_.acb_ + p_.axx_;
            sum += p_.bac_ + p_.bca_ + p_.bxx_;
            sum += p_.cab_ + p_.cba_ + p_.cxx_;
        } while (sum < 0.1);
        /** normalize **/
        p_.abc_ /= sum;
        p_.acb_ /= sum;
        p_.axx_ /= sum;
        p_.bac_ /= sum;
        p_.bca_ /= sum;
        p_.bxx_ /= sum;
        p_.cab_ /= sum;
        p_.cba_ /= sum;
        p_.cxx_ /= sum;

        if (kNVoteTrials == 1) {
            LOG("probability distribution ="
                <<" "<<p_.abc_
                <<" "<<p_.acb_
                <<" "<<p_.axx_
                <<" "<<p_.bac_
                <<" "<<p_.bca_
                <<" "<<p_.bxx_
                <<" "<<p_.cab_
                <<" "<<p_.cba_
                <<" "<<p_.cxx_);
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
        std::vector<double> v(3, 0.0);
        if (kNUtilityTrials > 0) {
            std::vector<double> sum(3, 0.0);
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

        u_a += p_.abc_ * u_abc_a_ + p_.acb_ * u_acb_a_ + p_.axx_ * u_axx_a_;
        u_a += p_.bac_ * u_bac_a_ + p_.bca_ * u_bca_a_ + p_.bxx_ * u_bxx_x_;
        u_a += p_.cab_ * u_cab_a_ + p_.cba_ * u_cba_a_ + p_.cxx_ * u_cxx_x_;

        u_b += p_.abc_ * u_abc_b_ + p_.acb_ * u_acb_b_ + p_.axx_ * u_axx_x_;
        u_b += p_.bac_ * u_bac_b_ + p_.bca_ * u_bca_b_ + p_.bxx_ * u_bxx_b_;
        u_b += p_.cab_ * u_cab_b_ + p_.cba_ * u_cba_b_ + p_.cxx_ * u_cxx_x_;

        u_c += p_.abc_ * u_abc_c_ + p_.acb_ * u_acb_c_ + p_.axx_ * u_axx_x_;
        u_c += p_.bac_ * u_bac_c_ + p_.bca_ * u_bca_c_ + p_.bxx_ * u_bxx_x_;
        u_c += p_.cab_ * u_cab_c_ + p_.cba_ * u_cba_c_ + p_.cxx_ * u_cxx_c_;

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
        std::swap(p_.abc_, p_.bac_);
        std::swap(p_.acb_, p_.bca_);
        std::swap(p_.axx_, p_.bxx_);
        std::swap(p_.cab_, p_.cba_);
    }

    void swap_probabilities_b_c() noexcept {
        std::swap(p_.abc_, p_.acb_);
        std::swap(p_.bac_, p_.cab_);
        std::swap(p_.bca_, p_.cba_);
        std::swap(p_.bxx_, p_.cxx_);
    }

    void condorcet() noexcept {
        double a_b = p_.abc_ + p_.acb_ + p_.axx_ + p_.cab_ - p_.bac_ - p_.bca_ - p_.bxx_ - p_.cba_;
        double a_c = p_.acb_ + p_.abc_ + p_.axx_ + p_.bac_ - p_.cab_ - p_.cba_ - p_.cxx_ - p_.bca_;
        double b_c = p_.bca_ + p_.bac_ + p_.bxx_ + p_.abc_ - p_.cba_ - p_.cab_ - p_.cxx_ - p_.acb_;
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

        if (condorcet_ != 3) {
            normalize_electorate(results);
            check_con_strategic_voting();
        }
    }

    void check_con_strategic_voting() noexcept {
        bool neither = true;

        /**
        strategic voting for condorcet means...
        BAC voters may have incentive to vote BCA.
        **/
        double a = pnorm_.acb_ + pnorm_.abc_ + pnorm_.axx_ + 0;
        double c = pnorm_.cab_ + pnorm_.cba_ + pnorm_.cxx_ + pnorm_.bca_ + pnorm_.bac_;
        if (c > a) {
            neither = false;
            ++result_con_strategic_;
            if (kVerbose) {
                LOG(pnorm_.b_<<pnorm_.a_<<pnorm_.c_<<" should vote strategically.");
            }
        }

        /**
        additionally...
        CBA voters may have incentive to vote BCA.
        **/
        double a2 = p_.abc_ + p_.acb_ + p_.axx_ + p_.cab_;
        double b2 = p_.bca_ + p_.bac_ + p_.bxx_ + p_.abc_ + p_.cba_;
        if (b2 > a2) {
            neither = false;
            ++result_con_alternate_;
            if (kVerbose) {
                LOG(pnorm_.c_<<pnorm_.b_<<pnorm_.a_<<" should vote strategically.");
            }
        }

        if (neither) {
            ++result_con_neither_;
        }
    }

    void first_past_post() noexcept {
        double a = p_.abc_ + p_.acb_ + p_.axx_;
        double b = p_.bac_ + p_.bca_ + p_.bxx_;
        double c = p_.cab_ + p_.cba_ + p_.cxx_;
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
        normalize_electorate(results);
        check_fpp_strategic_voting();
    }

    void check_fpp_strategic_voting() noexcept {
        double winner = pnorm_.abc_ + pnorm_.acb_ + pnorm_.axx_;
        double second = pnorm_.bac_ + pnorm_.bca_ + pnorm_.bxx_;
        double swing1 = pnorm_.cba_;
        double swing2 = pnorm_.cab_;
        double second_plus = second + swing1;
        double winner_plus = winner + swing2;
        if (second_plus > winner) {
            /**
            CBA can change the outcome from their last place choice
            to their second place choice by changing their vote.
            **/
            ++result_fpp_strategic_;
            if (kVerbose) {
                LOG(pnorm_.c_<<pnorm_.b_<<pnorm_.a_<<" should vote strategically: "
                    <<second<<"+"<<swing1<<"="<<second_plus<<" > "<<winner);
            }
            if (winner_plus > second_plus) {
                /**
                when CBA changes the outcome...
                CAB can change it back by also voting for their second choice.
                **/
                ++result_fpp_counter_;
                if (kVerbose) {
                    LOG(pnorm_.c_<<pnorm_.a_<<pnorm_.b_<<" should also vote strategically: "
                        <<second<<"+"<<swing1<<"="<<second_plus<<" < "
                        <<winner_plus<<"="<<winner<<"+"<<swing2);
                }
            }
        }
    }

    void ranked_choice_voting() noexcept {
        double a = p_.abc_ + p_.acb_ + p_.axx_;
        double b = p_.bac_ + p_.bca_ + p_.bxx_;
        double c = p_.cab_ + p_.cba_ + p_.cxx_;
        auto round1 = create_results(a, b, c);
        int loser = round1[0].idx_;
        double a_plus = a;
        double b_plus = b;
        double c_plus = c;
        if (loser == 2) {
            /** C was eliminated **/
            a_plus += p_.cab_;
            b_plus += p_.cba_;
            c_plus = 0.0;
        } else if (loser == 1) {
            /** B was eliminated **/
            a_plus += p_.bac_;
            c_plus += p_.bca_;
            b_plus = 0.0;
        } else {
            /** A was eliminated **/
            b_plus += p_.abc_;
            c_plus += p_.acb_;
            a_plus = 0.0;
        }
        auto round2 = create_results(a_plus, b_plus, c_plus);
        ranked_choice_ = round2[2].idx_;
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
        normalize_electorate(round2);
        check_rcv_strategic_voting();
    }

    void check_rcv_strategic_voting() noexcept {
        double a = pnorm_.abc_ + pnorm_.acb_ + pnorm_.axx_;
        double b = pnorm_.bac_ + pnorm_.bca_ + pnorm_.bxx_;
        double c = pnorm_.cab_ + pnorm_.cba_ + pnorm_.cxx_;
        /**
        winner=A, second=B, last=C.
        B might want to shift X votes to C.
        want B - X > A so B is not eliminated.
        want C + X > A so C is not eliminated.
        also need B - X + ABC > C + X + ACB so B doesn't lose in the second round.
        ergo X < B - A and X > A - C and X < (B + ABC - C - ACB) / 2.
        **/
        double bma = b - a;
        double amc = a - c;
        double bmc = (b + pnorm_.abc_ - c - pnorm_.acb_) / 2.0;
        double min_x = amc;
        double max_x = std::min(bma, bmc);
        if (max_x > min_x && min_x > 0.0) {
            ++result_rcv_strategic_;
            if (kVerbose) {
                LOG(pnorm_.b_<<" should vote strategically. min="<<min_x<<" max="<<max_x);
            }
            return;
        }

        /**
        B cannot win.
        BCA might want to shift to CXX.
        so that B is eliminated in the first round instead of C.
        then A picks up votes from BAC.
        C already as all the votes from BCA.
        **/
        double a2 = a;
        double b2 = b - pnorm_.bca_;
        double c2 = c + pnorm_.bca_;
        if (a2 > b2 && c2 > b2) {
            a2 += pnorm_.bac_;
            if (c2 > a2) {
                ++result_rcv_alternate_;
                if (kVerbose) {
                    LOG(pnorm_.b_<<pnorm_.c_<<pnorm_.a_<<" should vote alternate strategy. c2="<<c2<<" a2="<<a2<<" b2="<<b2);
                }
            }
        }
    }

    void reverse_rank_order() noexcept {
        /** first place votes **/
        double front_a = p_.abc_ + p_.acb_ + p_.axx_;
        double front_b = p_.bac_ + p_.bca_ + p_.bxx_;
        double front_c = p_.cab_ + p_.cba_ + p_.cxx_;
        auto front_results = create_results(front_a, front_b, front_c);
        int front_winner = front_results[2].idx_;
        double front_score = front_results[2].score_;

        /** count last place votes **/
        double last_a = p_.bca_ + p_.cba_ + p_.bxx_/2.0 + p_.cxx_/2.0;
        double last_b = p_.acb_ + p_.cab_ + p_.axx_/2.0 + p_.cxx_/2.0;
        double last_c = p_.bac_ + p_.abc_ + p_.axx_/2.0 + p_.bxx_/2.0;
        auto round1 = create_results(last_a, last_b, last_c);
        /** eliminate the candidate with the most last place votes. **/
        int loser = round1[2].idx_;
        double round2_a = front_a;
        double round2_b = front_b;
        double round2_c = front_c;
        if (loser == 2) {
            /** C was eliminated **/
            round2_a += p_.cab_;
            round2_b += p_.cba_;
            round2_c = 0.0;
        } else if (loser == 1) {
            /** B was eliminated **/
            round2_a += p_.bac_;
            round2_c += p_.bca_;
            round2_b = 0.0;
        } else {
            /** A was eliminated **/
            round2_b += p_.abc_;
            round2_c += p_.acb_;
            round2_a = 0.0;
        }
        auto round2 = create_results(round2_a, round2_b, round2_c);
        int rev_winner = round2[2].idx_;

        /** determine winner **/
        bool front_runner = false;
        if (front_score > kRevFrontRunner) {
            front_runner = true;
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
        if (front_runner == false) {
            normalize_electorate(round2);
            check_rev_strategic_voting();
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

    void check_rev_strategic_voting() noexcept {
        /**
        suppose A was eliminated.
        normalize so B wins.
        **/
        double b = pnorm_.bac_ + pnorm_.bca_ + pnorm_.bxx_ + pnorm_.abc_;
        double c = pnorm_.cab_ + pnorm_.cba_ + pnorm_.cxx_ + pnorm_.acb_;
        if (c > b) {
            pnorm_.swap_bc();
        }

        /**
        B has incentive to move last place votes from C to A
        so that A is eliminated instead of C.

        a = last_a + X
        b = last_b
        c = last_c - X
        a > b and a > c

        rearranging the above.

        last_a + X > last_b
        last_a + X > last_c - X

        more rearranging.

        X > last_b - last_a
        X > (last_c - last_a)/2

        the number of votes that can be shifted is limited.
        half of the p_bxx votes have already been cast for A last.

        X < p_bac + p_bxx/2
        **/
        double last_a = pnorm_.bca_ + pnorm_.cba_ + pnorm_.bxx_/2.0 + pnorm_.cxx_/2.0;
        double last_b = pnorm_.acb_ + pnorm_.cab_ + pnorm_.axx_/2.0 + pnorm_.cxx_/2.0;
        double last_c = pnorm_.bac_ + pnorm_.abc_ + pnorm_.axx_/2.0 + pnorm_.bxx_/2.0;
        double bma = last_b - last_a;
        double cma = (last_c - last_a) / 2.0;
        double max_x = pnorm_.bac_ + pnorm_.bxx_/2.0;
        double min_x = std::max(bma, cma);
        if (max_x > min_x && min_x > 0.0) {
            ++result_rev_strategic_;
            if (kVerbose) {
                    LOG(pnorm_.b_<<" should vote strategically. bma="<<bma<<" cma="<<cma<<" max="<<max_x<<" min="<<min_x);
                    LOG("  last_a="<<last_a<<" last_b="<<last_b<<" last_c="<<last_c);
                    LOG("  b="<<b<<" c="<<c);
                    pnorm_.print();
            }
            /**
            tk tsc todo: A should be able to counter this.
            tk tsc todo: C should be able to counter this.
            **/
        } else {
            if (kVerbose) {
                LOG(pnorm_.b_<<" should vote tactically.");
            }
        }
    }

    void approval_voting() noexcept {
        double a = p_.abc_ + p_.acb_ + p_.axx_ + p_.bac_ + p_.cab_;
        double b = p_.bac_ + p_.bca_ + p_.bxx_ + p_.abc_ + p_.cba_;
        double c = p_.cab_ + p_.cba_ + p_.cxx_ + p_.acb_ + p_.bca_;
        auto results = create_results(a, b, c);
        approval_ = results[2].idx_;
        if (kNVoteTrials == 1) {
            LOG("Approval Winner:"
                <<" "<<results[2].name_<<"="<<results[2].score_
                <<" "<<results[1].name_<<"="<<results[1].score_
                <<" "<<results[0].name_<<"="<<results[0].score_);
        }

        /** strategic voting **/
        normalize_electorate(results);
        check_app_strategic_voting();
    }

    void check_app_strategic_voting() noexcept {
        /**
        BAC voters should have voted BXX.
        CAB voters should have voted CXX.
        **/
        double a1 = p_.abc_ + p_.acb_ + p_.axx_ + 0       + p_.cab_;
        double b1 = p_.bac_ + p_.bca_ + p_.bxx_ + p_.abc_ + p_.cba_;
        if (b1 > a1) {
            ++result_app_strategic_;
            if (kVerbose) {
                LOG(pnorm_.b_<<pnorm_.a_<<"X should vote strategically.");
            }
        }
        double a2 = p_.abc_ + p_.acb_ + p_.axx_ + p_.bac_ + 0;
        double c2 = p_.cab_ + p_.cba_ + p_.cxx_ + p_.acb_ + p_.bca_;
        if (c2 > a2) {
            ++result_app_alternate_;
            if (kVerbose) {
                LOG(pnorm_.b_<<pnorm_.a_<<"X should vote strategically.");
            }
        }
        /**
        tk tsc todo: what does this mean?
        BAC and CAB voters could have colluded.
        **/
        double a3 = p_.abc_ + p_.acb_ + p_.axx_ + 0 + 0;
        double b3 = b1;
        double c3 = c2;
        if (b3 > a3 && c3 > a3) {
            double diff = std::abs(b3 - c3);
            if (diff < 0.03) {
                ++result_app_collude_;
                if (kVerbose) {
                    LOG(pnorm_.b_<<pnorm_.a_<<"X and "<<pnorm_.c_<<pnorm_.a_<<"X should vote strategically.");
                }
            }
        }

        /**
        tk tsc todo: how to model approval strategic voting fail?
        BCA voters who voted BXX should have voted BCA.
        CBA voters who voted BXX should have voted CBA.
        **/
    }

    void contingent_proxy_voting() noexcept {
        /** first place votes **/
        double front_a = p_.abc_ + p_.acb_ + p_.axx_;
        double front_b = p_.bac_ + p_.bca_ + p_.bxx_;
        double front_c = p_.cab_ + p_.cba_ + p_.cxx_;        auto front_results = create_results(front_a, front_b, front_c);
        int front_winner = front_results[2].idx_;
        double front_score = front_results[2].score_;

        /** if someone has a majority of the votes, they win. **/
        if (front_score > 0.50) {
            contingent_proxy_ = front_winner;
            if (kNVoteTrials == 1) {
                LOG("Contingent Proxy Front-runner Wins: "
                    <<front_results[2].name_<<" "<<front_score);
            }
            return;
        }

        /**
        contingent election. no candidate has a majority.
        every candidate becomes a proxy for their voters.
        candidate A will cast all of his votes against C if abc > acb.
            abc=front_a, acb=0.
        otherwise A will cast all of his votes against B.
            abc=0, acb=front_a
        **/
        double abc = 0.0;
        double acb = 0.0;
        if (p_.abc_ > p_.acb_) {
            abc = front_a;
        } else {
            acb = front_a;
        }
        double bac = 0.0;
        double bca = 0.0;
        if (p_.bac_ > p_.bca_) {
            bac = front_b;
        } else {
            bca = front_b;
        }
        double cab = 0.0;
        double cba = 0.0;
        if (p_.cab_ > p_.cba_) {
            cab = front_c;
        } else {
            cba = front_c;
        }

        /** count last place votes **/
        double last_a = bca + cba;
        double last_b = acb + cab;
        double last_c = bac + abc;
        auto round1 = create_results(last_a, last_b, last_c);
        /** eliminate the candidate with the most last place votes. **/
        int loser = round1[2].idx_;
        double round2_a = front_a;
        double round2_b = front_b;
        double round2_c = front_c;
        if (loser == 2) {
            /** C was eliminated **/
            round2_a += cab;
            round2_b += cba;
            round2_c = 0.0;
        } else if (loser == 1) {
            /** B was eliminated **/
            round2_a += bac;
            round2_c += bca;
            round2_b = 0.0;
        } else {
            /** A was eliminated **/
            round2_b += abc;
            round2_c += acb;
            round2_a = 0.0;
        }
        auto round2 = create_results(round2_a, round2_b, round2_c);
        int cpv_winner = round2[2].idx_;

        /** determine winner **/
        contingent_proxy_ = cpv_winner;
        if (kNVoteTrials == 1) {
            LOG("Contingent Proxy Round 1:"
                <<" "<<round1[0].name_<<"="<<round1[0].score_
                <<" "<<round1[1].name_<<"="<<round1[1].score_
                <<" "<<round1[2].name_<<"="<<round1[2].score_);
            LOG("Contingent Proxy Winner:"
                <<" "<<round2[2].name_<<"="<<round2[2].score_
                <<" "<<round2[1].name_<<"="<<round2[1].score_);
        }
    }

    void normalize_electorate(
        Results& results
    ) noexcept {
        pnorm_ = p_.normalize(results);
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
        switch (ranked_choice_) {
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
        switch (contingent_proxy_) {
        case 0:
            ++result_cpv_a_;
            break;
        case 1:
            ++result_cpv_b_;
            break;
        case 2:
            ++result_cpv_c_;
            break;
        }
        if (first_past_post_ == condorcet_) {
            ++result_fpp_con_;
        }
        if (ranked_choice_ == condorcet_) {
            ++result_rcv_con_;
        }
        if (reverse_rank_order_ == condorcet_) {
            ++result_rev_con_;
        }
        if (approval_ == condorcet_) {
            ++result_app_con_;
        }
        if (contingent_proxy_ == condorcet_) {
            ++result_cpv_con_;
        }
        if (condorcet_ == 3) {
            ++result_fpp_con_;
            ++result_rcv_con_;
            ++result_rev_con_;
            ++result_app_con_;
            ++result_cpv_con_;
        }
        if (ranked_choice_ == first_past_post_) {
            ++result_rcv_fpp_;
        }
        if (reverse_rank_order_ == first_past_post_) {
            ++result_rev_fpp_;
        }
        if (approval_ == first_past_post_) {
            ++result_app_fpp_;
        }
        if (contingent_proxy_ == first_past_post_) {
            ++result_cpv_fpp_;
        }
        if (reverse_rank_order_ == ranked_choice_) {
            ++result_rev_rcv_;
        }
        if (approval_ == ranked_choice_) {
            ++result_app_rcv_;
        }
        if (contingent_proxy_ == ranked_choice_) {
            ++result_cpv_rcv_;
        }
        if (approval_ == reverse_rank_order_) {
            ++result_app_rev_;
        }
        if (contingent_proxy_ == reverse_rank_order_) {
            ++result_cpv_rev_;
        }
        if (contingent_proxy_ == approval_) {
            ++result_cpv_app_;
        }
        std::vector<int> candidates(6, 0);
        ++candidates[condorcet_];
        ++candidates[first_past_post_];
        ++candidates[ranked_choice_];
        ++candidates[reverse_rank_order_];
        ++candidates[approval_];
        ++candidates[contingent_proxy_];
        int nwinners = 0;
        for (int i = 0; i < 6; ++i) {
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
        case 4:
            ++result_nwinners_4_;
            break;
        case 5:
            ++result_nwinners_5_;
            break;
        case 6:
            ++result_nwinners_6_;
            break;
        }
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
        double pct_cpv_a = 100.0 * result_cpv_a_ / kNVoteTrials;
        double pct_cpv_b = 100.0 * result_cpv_b_ / kNVoteTrials;
        double pct_cpv_c = 100.0 * result_cpv_c_ / kNVoteTrials;
        LOG("Contingent Proxy    : A="<<pct_cpv_a<<"% B="<<pct_cpv_b<<"% C="<<pct_cpv_c<<"%");
        LOG("");
        double pct_fpp_con = 100.0 * result_fpp_con_ / kNVoteTrials;
        double pct_rcv_con = 100.0 * result_rcv_con_ / kNVoteTrials;
        double pct_rev_con = 100.0 * result_rev_con_ / kNVoteTrials;
        double pct_app_con = 100.0 * result_app_con_ / kNVoteTrials;
        double pct_cpv_con = 100.0 * result_cpv_con_ / kNVoteTrials;
        double pct_rcv_fpp = 100.0 * result_rcv_fpp_ / kNVoteTrials;
        double pct_rev_fpp = 100.0 * result_rev_fpp_ / kNVoteTrials;
        double pct_app_fpp = 100.0 * result_app_fpp_ / kNVoteTrials;
        double pct_cpv_fpp = 100.0 * result_cpv_fpp_ / kNVoteTrials;
        double pct_rev_rcv = 100.0 * result_rev_rcv_ / kNVoteTrials;
        double pct_app_rcv = 100.0 * result_app_rcv_ / kNVoteTrials;
        double pct_cpv_rcv = 100.0 * result_cpv_rcv_ / kNVoteTrials;
        double pct_app_rev = 100.0 * result_app_rev_ / kNVoteTrials;
        double pct_cpv_rev = 100.0 * result_cpv_rev_ / kNVoteTrials;
        double pct_cpv_app = 100.0 * result_cpv_app_ / kNVoteTrials;
        LOG("Agreements          :  FPP     RCV     REV     APP     CPV");
        LOG("Condorcet           : "<<pct_fpp_con<<"%  "<<pct_rcv_con<<"%  "<<pct_rev_con<<"%  "<<pct_app_con<<"%  "<<pct_cpv_con<<"%");
        LOG("First Past Post     :         "<<pct_rcv_fpp<<"%  "<<pct_rev_fpp<<"%  "<<pct_app_fpp<<"%  "<<pct_cpv_fpp<<"%");
        LOG("Ranked Choice Voting:                 "<<pct_rev_rcv<<"%  "<<pct_app_rcv<<"%  "<<pct_cpv_rcv<<"%");
        LOG("Reverse Rank Order  :                         "<<pct_app_rev<<"%  "<<pct_cpv_rev<<"%");
        LOG("Approval            :                                 "<<pct_cpv_app<<"%");
        LOG("");
        double nwinners1 = 100.0 * result_nwinners_1_ / kNVoteTrials;
        double nwinners2 = 100.0 * result_nwinners_2_ / kNVoteTrials;
        double nwinners3 = 100.0 * result_nwinners_3_ / kNVoteTrials;
        double nwinners4 = 100.0 * result_nwinners_4_ / kNVoteTrials;
        double nwinners5 = 100.0 * result_nwinners_5_ / kNVoteTrials;
        double nwinners6 = 100.0 * result_nwinners_6_ / kNVoteTrials;
        LOG("Unique Winners      : 1:"<<nwinners1<<"% 2:"<<nwinners2<<"% 3:"<<nwinners3<<"% 4:"<<nwinners4<<"% 5:"<<nwinners5<<"% 6:"<<nwinners6<<"%");
        LOG("");
        LOG("Strategic Voting:");
        double pct_rcv_strategic = 100.0 * result_rcv_strategic_ / kNVoteTrials;
        double pct_rcv_alternate = 100.0 * result_rcv_alternate_ / kNVoteTrials;
        LOG("Ranked Choice Voting: "<<pct_rcv_strategic<<"% alternate: "<<pct_rcv_alternate<<"%");
        double pct_fpp_strategic = 100.0 * result_fpp_strategic_ / kNVoteTrials;
        double pct_fpp_counter = 100.0 * result_fpp_counter_ / kNVoteTrials;
        LOG("First Past Post     : "<<pct_fpp_strategic<<"% counter: "<<pct_fpp_counter<<"%");
        double pct_con_strategic = 100.0 * result_con_strategic_ / kNVoteTrials;
        double pct_con_alternate = 100.0 * result_con_alternate_ / kNVoteTrials;
        double pct_con_neither = 100.0 * result_con_neither_ / kNVoteTrials;
        LOG("Condorcet           : "<<pct_con_strategic<<"% alternate: "<<pct_con_alternate<<"% neither: "<<pct_con_neither<<"%");
        double pct_app_strategic = 100.0 * result_app_strategic_ / kNVoteTrials;
        double pct_app_alternate = 100.0 * result_app_alternate_ / kNVoteTrials;
        double pct_app_collude = 100.0 * result_app_collude_ / kNVoteTrials;
        LOG("Approval            : "<<pct_app_strategic<<"% alternate: "<<pct_app_alternate<<"% collude: "<<pct_app_collude<<"%");
        double pct_rev_strategic = 100.0 * result_rev_strategic_ / kNVoteTrials;
        LOG("Reverse Rank Order  : "<<pct_rev_strategic<<"%");
        LOG("Contingent Proxy    : TBD");
        LOG("");
        LOG("Oddities:");
        LOG("Ranked Choice Voting:");
        if (kRevFrontRunner < 1.0 && kRevFrontRunner >= 0.50) {
            double pct_rev_front_runner = 100.0 * result_rev_front_runner_ / kNVoteTrials;
            LOG("Won by Front-Runner Rule: "<<pct_rev_front_runner<<"%");
        }
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
