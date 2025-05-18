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

/** number of candidates **/
constexpr int kNCandidates = 3;


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
        LOG("Random Seed: "<<seed);
    }

    double generate() noexcept {
        double x = unif_(rng_);
        return x;
    }
};

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

    Voters voters_;

    void init() noexcept {
        /** allocate space for the voters and candidates **/
        voters_.resize(kNVoters);

        ranked();
    }

    /**
    evenly distribute voters from 0 to 1 along a single axis.
    this is the simplest model of the electorate.
    **/
    void ranked() noexcept {
        LOG("Electorate is uniform ranked order.");
        constexpr double kOffset = 0.5 / double(kNVoters);
        for (int i = 0; i < kNVoters; ++i) {
            auto& voter = voters_[i];
            voter.position_ = kOffset + double(i) / double(kNVoters);
        }
    }
};

class GuthrieImpl {
public:
    GuthrieImpl() = default;
    GuthrieImpl(const GuthrieImpl &) = delete;
    GuthrieImpl(GuthrieImpl &&) = delete;
    ~GuthrieImpl() = default;

    RandomNumberGenerator rng_;
    Electorate electorate_;
    Candidates candidates_;

    void run() noexcept {
        LOG("Guthrie Voting Analysis");
        LOG("Number Voters: "<<kNVoters);
        LOG("Number Candidates: "<<kNCandidates);
        rng_.init();
        electorate_.init();
        init_candidates();

        vote();
        find_winner();
    }

    void init_candidates() noexcept {
        /** allocate space **/
        candidates_.resize(kNCandidates);

        /**
        chose random position for each candidate.
        sort them.
        name them in sorted order.
        **/
        LOG("Candidate Positions:");
        for (auto&& candidate : candidates_) {
            candidate.position_ = rng_.generate();
        }
        std::sort(candidates_.begin(), candidates_.end());
        for (int i = 0; i < kNCandidates; ++i) {
            auto& candidate = candidates_[i];
            candidate.name_ = 'A' + i;
            LOG(candidate.name_<<": "<<candidate.position_);
        }
    }

    void vote() noexcept {
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
        LOG("Candidate Vote Totals:");
        for (auto&& candidate : candidates_) {
            LOG(candidate.name_<<": "<<candidate.support_);
        }
    }

    int find_closest_candidate(
        double position
    ) noexcept {
        int closest_candidate = 0;
        double closest_distance = 2.0;
        for (int i = 0; i < kNCandidates; ++i) {
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
        for (auto&& candidate : candidates_) {
            if (2*candidate.support_ > kNVoters) {
                LOG(candidate.name_<<" wins with a majority.");
                return;
            }
        }
        LOG("No candidate has a majority.");
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
