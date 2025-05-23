/*
Copyright (C) 2012-2025 tim cotter. All rights reserved.
*/

/**
analyze guthrie voting.

assumptions:
1. voter engagement is limited.
they have enough capacity to pick their favorite candidate.
but that's it.
they don't have the time or energy or motivation to score or order candidates.
they are effectively stupid.
2. a candidate with a majority of the votes wins.
3. we want more than two parties.
4. third parties cannot split the vote.
5. protest votes should be safe.
6. voters should vote honestly.
7. candidates are not stupid.
they are informed, motivated, and have time and energy to fully engage the system.
they can score and order the other candidates.
8. candidates may vote strategically.
9. the winning candidate should maximize total satisfaction (utility) of the voters.
10. voters know how all the candidates score/rank each other.
11. candidates should not be harmed by getting more votes from voters.
monoticity and participation criteria.


guthrie voting works like this:

phase 1:
the primary reduces a large number of candidates to a manageable number.
but not less than 3.

phase 2:
voters cast a single vote for their favorite cancidate.
A,B,D received 35,30,25,10 votes respectively.
A prefers B > C > D.
B prefers A > C > D.
C prefers D > B > A
D prefers C > B > A

phase 3:
find the winner by coombs method.
round 1:
A casts 35 votes for A.
B casts 30 votes for B.
C casts 25 votes for C.
D casts 10 votes for D.
totals: A=35, B=30, C=25, D=10.
no one has a majority.
proceed to the elimination phase.
A casts 35 votes against D.
B casts 30 votes against D.
C casts 25 votes against A.
D casts 10 votes against A.
totals: A=35, B=0, C=0, D=65.
D is eliminated from the ballot but not the voting.
round 2:
A casts 35 votes for A.
B casts 30 votes for B.
C casts 25 votes for C.
D casts 10 votes for C.
totals: A=35, B=30, C=35, D=0.
no one has a majority.
proceed to the elimination phase.
A casts 35 votes against C.
B casts 30 votes against C.
C casts 25 votes against A.
D casts 10 votes against A.
totals: A=35, B=0, C=65, D=0.
C is eliminated from the ballot but not the voting.
round 3;
A casts 35 votes for A.
B casts 30 votes for B.
C casts 25 votes for B.
D casts 10 votes for B.
totals: A=35, B=65, C=35, D=0.
B has a majority.
B wins the election.

there really isn't much motivation for voters to vote strategically.
the only reason would be if the candidate's ranking is different from the voter's.

it doesn't look like the candidates have much incentive to vote strategically either.
though i could be wrong on this.

things to do:
1. start with 3 candidates.
increase later.
2. model the electorate so that utility is a function of distance between voter and candidate.
    a. uniform single dimension.
    b. clustered voters.
    c. multiple dimensions of progressively lower utility weighting.
3. ensure winner still wins if any other candidate drops out.
4. identify the optimal candidate.
5. check if a single candidate can vote strategically to get a better result.
**/

#include "guthrie.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>

namespace {

/** number of voters **/
//constexpr int kNVoters = 100;
constexpr int kNVoters = 10*1000;

/** options for distributing the electorate. **/
constexpr int kElectorateUniform = 0;
constexpr int kElectorateRandom = 1;
constexpr int kElectorateMethod = kElectorateRandom;

/** number of candidates **/
constexpr int kNCandidates = 3;
//constexpr int kNCandidates = 5;
//constexpr int kNCandidates = 7;

/** options for choosing candidates **/
constexpr int kCandidatesRandom = 0;
constexpr int kCanddiatesSingleTransferableVote = 1;
constexpr int kCandidateMethod = kCanddiatesSingleTransferableVote;

class RandomNumberGenerator {
public:
    RandomNumberGenerator() = default;
    ~RandomNumberGenerator() = default;

    std::mt19937_64 rng_;
    std::uniform_real_distribution<double> unif_;

    void init() noexcept {
        std::uint64_t seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::seed_seq ss{uint32_t(seed & 0xffffffff), uint32_t(seed>>32)};
        rng_.seed(ss);
        unif_ = std::uniform_real_distribution<double>(0.0, 1.0);
        LOG("Random seed: "<<seed);
    }

    double generate() noexcept {
        double x = unif_(rng_);
        return x;
    }

    int generate(int max) noexcept {
        double x = generate();
        int i = std::floor(x * double(max));
        return i;
    }
};
static RandomNumberGenerator *rng_ = nullptr;

class Voter {
public:
    /** position along the axis ranges from 0..1 **/
    double position_ = 0.0;
};
typedef std::vector<Voter> Voters;

class Candidate {
public:
    /** generated name **/
    char name_;

    /** position along the axis ranges from 0..1 **/
    double position_ = 0.0;

    /** vote total aka asset **/
    int support_ = 0;

    /** ranking of other candidates. **/
    std::vector<int> rankings_;

    /** for sorting **/
    bool operator < (const Candidate& other) const
    {
        return (position_ < other.position_);
    }
};
typedef std::vector<Candidate> Candidates;

class Electorate {
public:
    Electorate() = default;
    ~Electorate() = default;

    int nvoters_ = 0;
    Voters voters_;

    void init(int nvoters, int method) noexcept {
        nvoters_ = nvoters;

        /** allocate space for the voters and candidates **/
        voters_.resize(nvoters);

        /** distribute the voter by the chosen method. **/
        switch (method) {
        default:
        case kElectorateUniform:
            ranked();
            break;

        case kElectorateRandom:
            random();
            break;
        }

        /** show the distribution. **/
        show();
    }

    /**
    evenly distribute voters from 0 to 1 along a single axis.
    this is the simplest model of the electorate.
    **/
    void ranked() noexcept {
        LOG("Electorate is uniform ranked order.");
        double nvoters = double(nvoters_);
        double offset = 0.5 / nvoters;
        for (int i = 0; i < nvoters_; ++i) {
            auto& voter = voters_[i];
            voter.position_ = offset + double(i) / nvoters;
        }
    }

    /**
    randomly distribute voters along the axis.
    **/
    void random() noexcept {
        LOG("Electorate is random.");
        for (int i = 0; i < nvoters_; ++i) {
            auto& voter = voters_[i];
            voter.position_ = rng_->generate();
        }
    }

    void show() noexcept {
        LOG("Electorate distribution:");
        constexpr int kNBins = 10;
        std::vector<int> bins;
        bins.resize(kNBins);
        for (int i = 0; i < kNBins; ++i) {
            bins[i] = 0;
        }
        for (auto&& voter : voters_) {
            int i = std::floor(voter.position_ * kNBins);
            i = std::clamp(i, 0, kNBins - 1);
            ++bins[i];
        }
        double nvoters = voters_.size();
        for (int i = 0; i < kNBins; ++i) {
            double mx = double(i+1) / kNBins;
            double frac = 100.0 * double(bins[i]) / nvoters;
            LOG(mx<<": "<<bins[i]<<" "<<frac<<"%");
        }
    }
};

class GuthrieImpl {
public:
    GuthrieImpl() = default;
    GuthrieImpl(const GuthrieImpl &) = delete;
    GuthrieImpl(GuthrieImpl &&) = delete;
    ~GuthrieImpl() = default;

    Electorate electorate_;
    Candidates candidates_;
    int nvoters_ = kNVoters;
    int electorate_method_ = kElectorateMethod;
    int ncandidates_ = kNCandidates;
    int canddiate_method_ = kCandidateMethod;
    int winner_ = 0;

    void run() noexcept {
        LOG("Guthrie voting analysis:");
        LOG("Number voters: "<<nvoters_);
        LOG("Number candidates: "<<ncandidates_);
        rng_ = new(std::nothrow) RandomNumberGenerator();
        rng_->init();
        electorate_.init(nvoters_, electorate_method_);
        init_candidates();

        vote();
        find_winner();
        check_criteria();
    }

    void init_candidates() noexcept {

        /** choose the number of candidates based on the specified method. **/
        int n = ncandidates_;
        if (canddiate_method_ == kCanddiatesSingleTransferableVote) {
            /** choose a number of candidates in the primary. **/
            double cube_root = std::pow(double(nvoters_), 1.0/3.0);
            n = (int) std::round(cube_root);

            /** maybe increase it. **/
            n = std::max(n, ncandidates_);
        }

        if (n > ncandidates_) {
            LOG("Reducing the number of candidates from "<<n<<" to "<<ncandidates_<<".");
        }

        /** allocate space **/
        candidates_.resize(n);

        /**
        choose random voters as candidates.
        do not allow duplicates.
        **/
        std::vector<bool> duplicates;
        duplicates.resize(nvoters_);
        for (int i = 0; i < nvoters_; ++i) {
            duplicates[i] = false;
        }
        for (auto&& candidate : candidates_) {
            int i = 0;
            for(;;) {
                i = rng_->generate(nvoters_);
                if (duplicates[i] == false) {
                    break;
                }
            }
            duplicates[i] = true;
            candidate.position_ = electorate_.voters_[i].position_;
        }

        /**
        reduce the number of candidates.
        remove the ones with the lowest vote counts.
        **/
        for(;;) {
            n = candidates_.size();
            if (n <= ncandidates_) {
                break;
            }
            vote();
            int worst = 0;
            int worst_support = 2*nvoters_;
            for (int i = 0; i < n; ++i) {
                auto& candidate = candidates_[i];
                if (candidate.support_ < worst_support) {
                    worst = i;
                    worst_support = candidate.support_;
                }
            }
            candidates_.erase(candidates_.begin() + worst);
        }

        /** sort them. **/
        std::sort(candidates_.begin(), candidates_.end());

        /** every candidate rank orders the others. **/
        for (auto&& candidate : candidates_) {
            rank_other_candidates(candidate);
        }

        /** name them in sorted normalized order **/
        for (int i = 0; i < ncandidates_; ++i) {
            auto& candidate = candidates_[i];
            candidate.name_ = 'A' + i;
        }

        /** show candidate positions and rankings of the others. **/
        LOG("Candidate positions:");
        for (auto&& candidate : candidates_ ) {
            LOG(candidate.name_<<": "<<candidate.position_);
        }
        LOG("Candidate rankings of other candidates:");
        for (auto&& candidate : candidates_ ) {
            std::stringstream ss;
            ss << candidate.name_ << ":";
            for (int i = 1; i < ncandidates_; ++i) {
                int rank = candidate.rankings_[i];
                if (i > 1) {
                    ss << " >";
                }
                ss << " " << candidates_[rank].name_;
            }
            LOG(ss.str());
        }
    }

    void rank_other_candidates(
        Candidate& candidate
    ) noexcept {
        std::vector<double> distance;
        distance.reserve(ncandidates_);
        candidate.rankings_.reserve(ncandidates_);

        for (int i = 0; i < ncandidates_; ++i) {
            auto& other = candidates_[i];
            double d = std::abs(candidate.position_ - other.position_);
            int k = 0;
            for(;;) {
                if (k>=i) {
                    break;
                }
                if (distance[k] > d) {
                    break;
                }
                ++k;
            }
            distance.insert(distance.begin() + k, d);
            candidate.rankings_.insert(candidate.rankings_.begin() + k, i);
        }
    }

    void vote() noexcept {
        /**
        clear support.
        we may vote multiple times.
        **/
        for (auto&& candidate : candidates_) {
            candidate.support_ = 0;
        }

        /**
        for each voter...
        find the closest candidate.
        increment their support.
        **/
        for (auto&& voter : electorate_.voters_) {
            int favorite = find_closest_candidate(voter.position_);
            ++candidates_[favorite].support_;
        }
        /** report results **/
        /*LOG("Candidate vote totals:");
        for (auto&& candidate : candidates_) {
            LOG(candidate.name_<<": "<<candidate.support_);
        }*/
    }

    int find_closest_candidate(
        double position
    ) noexcept {
        int closest_candidate = 0;
        double closest_distance = 2.0;
        int ncandidates = candidates_.size();
        for (int i = 0; i < ncandidates; ++i) {
            auto& candidate = candidates_[i];
            double distance = std::abs(candidate.position_ - position);
            if (distance < closest_distance) {
                closest_candidate = i;
                closest_distance = distance;
            }
        }
        return closest_candidate;
    }

    void find_winner() noexcept {
        std::vector<int> counts;
        counts.resize(ncandidates_);

        for (int round = 1; round < ncandidates_; ++round) {
            LOG("Round "<<round<<":");

            /** phase 1: count first place votes. **/

            /** initialize the counts **/
            for (int i = 0; i < ncandidates_; ++i) {
                counts[i] = 0;
            }

            /** count first place votes. **/
            for (auto&& candidate : candidates_) {
                int favorite = candidate.rankings_[0];
                counts[favorite] += candidate.support_;
            }

            /** show first place vote counts. **/
            LOG("First place vote counts:");
            for (int i = 0; i < ncandidates_; ++i) {
                auto& candidate = candidates_[i];
                LOG(candidate.name_<<": "<<counts[i]);
            }

            /** check for majority. **/
            for (int i = 0; i < ncandidates_; ++i) {
                if (2*counts[i] > nvoters_) {
                    winner_ = i;
                    auto& candidate = candidates_[i];
                    LOG(candidate.name_<<" wins Guthrie voting in round "<<round<<".");
                    return;
                }
            }

            /**
            no candidate has a majority.
            proceed to phase 2: count last place votes.
            **/
            LOG("No candidate has a majority.");

            /** initialize the counts **/
            for (int i = 0; i < ncandidates_; ++i) {
                counts[i] = 0;
            }

            /** count last place votes. **/
            int last_index = ncandidates_ - round;
            for (auto&& candidate : candidates_) {
                int worst = candidate.rankings_[last_index];
                counts[worst] += candidate.support_;
            }

            /** show last place vote counts. **/
            LOG("Last place vote counts:");
            for (int i = 0; i < ncandidates_; ++i) {
                auto& candidate = candidates_[i];
                LOG(candidate.name_<<": "<<counts[i]);
            }

            /** find the candidate with the most last place votes. **/
            int loser = 0;
            int loser_count = 0;
            for (int i = 0; i < ncandidates_; ++i) {
                int count = counts[i];
                if (count > loser_count) {
                    loser = i;
                    loser_count = count;
                }
            }
            LOG("Candidate "<<candidates_[loser].name_<<" has the most last place votes - eliminated.");

            /** remove the loser from the candidate rankings. **/
            for (auto&& candidate : candidates_) {
                for (auto it = candidate.rankings_.begin(); it < candidate.rankings_.end(); ++it) {
                    if (*it == loser) {
                        candidate.rankings_.erase(it);
                        break;
                    }
                }
            }

            LOG("Updated rankings:");
            for (auto&& candidate : candidates_ ) {
                std::stringstream ss;
                ss << candidate.name_ << ":";
                for (int i = 0; i < last_index; ++i) {
                    int rank = candidate.rankings_[i];
                    if (i > 0) {
                        ss << " >";
                    }
                    ss << " " << candidates_[rank].name_;
                }
                LOG(ss.str());
            }
        }
    }

    void check_criteria() noexcept {
        LOG("");
        LOG("Checking voting criteria.");
        int max_utility = find_max_utility_candidate();
        int condorcet = find_condorcet_candidate();

        LOG("Voting criteria results:");
        const char *result = nullptr;
        result = result_to_string(winner_, max_utility);
        LOG("Maximizes total voter utility: "<<result);
        result = result_to_string(winner_, condorcet);
        LOG("Condorcet majority           : "<<result);
    }

    const char *result_to_string(int winner, int expected) noexcept {
        if (expected < 0) {
            return "n/a";
        }
        if (winner == expected) {
            return "pass";
        }
        return "=FAIL=";
    }

    int find_max_utility_candidate() noexcept {
        int winner = 0;
        double max = 0.0;
        double min = 10.0 * double(nvoters_);

        std::vector<double> utility;
        utility.resize(ncandidates_);

        /** sum the distance from each candidate to all voters. **/
        for (int i = 0; i < ncandidates_; ++i) {
            auto& candidate = candidates_[i];
            double sum = 0;
            for (auto&& voter : electorate_.voters_) {
                double d = std::abs(candidate.position_ - voter.position_);
                sum += d;
            }
            utility[i] = sum;

            if (max < sum) {
                max = sum;
            }
            if (min > sum) {
                winner = i;
                min = sum;
            }
        }

        /** scale utility **/
        for (int i = 0; i < ncandidates_; ++i) {
            utility[i] = 1.0 - utility[i] / max;
        }

        /** show results **/
        LOG(candidates_[winner].name_<<" maximizes total voter utility.");
        for (int i = 0; i < ncandidates_; ++i) {
            auto& candidate = candidates_[i];
            LOG(candidate.name_<<": "<<utility[i]);
        }

        return winner;
    }

    /**
    condorcet wins the most head to head races.
    **/
    int find_condorcet_candidate() noexcept {
        /** initialize number of wins for each candidate. **/
        std::vector<int> wins;
        wins.resize(ncandidates_);
        for (int i = 0; i < ncandidates_; ++i) {
            wins[i] = 0;
        }

        LOG("Condorcet results:");
        /** count head to head victories. **/
        for (int i = 0; i < ncandidates_; ++i) {
            for (int k = i + 1; k < ncandidates_; ++k) {
                bool result = head_to_head(i, k);
                if (result) {
                    LOG(candidates_[i].name_<<" > "<<candidates_[k].name_);
                    ++wins[i];
                } else {
                    LOG(candidates_[k].name_<<" > "<<candidates_[i].name_);
                    ++wins[k];
                }
            }
        }

        /** find the candidate with the most wins. **/
        int max = -1;
        int winner = 0;
        bool multiple = false;
        for (int i = 0; i < ncandidates_; ++i) {
            int w = wins[i];
            if (max == w) {
                multiple = true;
            }
            if (max < w) {
                winner = i;
                multiple = false;
                max = w;
            }
        }

        /** no winner if there's a cycle. **/
        if (multiple) {
            LOG("Condorcet cycle exists.");
            return -1;
        }

        return winner;
    }

    bool head_to_head(int a, int b) noexcept {
        int avotes = 0;
        int bvotes = 0;

        double apos = candidates_[a].position_;
        double bpos = candidates_[b].position_;

        for (auto&& voter : electorate_.voters_) {
            double vpos = voter.position_;
            double adist = std::abs(apos - vpos);
            double bdist = std::abs(bpos - vpos);
            if (adist < bdist) {
                ++avotes;
            }
        }
        bvotes = nvoters_ - avotes;
        if (avotes > bvotes) {
            return true;
        }
        return false;
    }
};

} // anonymous namespace

GuthrieVoting::GuthrieVoting() noexcept {
    impl_ = (void *) new GuthrieImpl;
}

GuthrieVoting::~GuthrieVoting() noexcept {
    auto impl = (GuthrieImpl *) impl_;
    delete impl;
}

void GuthrieVoting::run() noexcept {
    auto impl = (GuthrieImpl *) impl_;
    impl->run();
}
