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
we pick a number of primary candidates equal to the cube root of the number of voters.
voters cast a single vote for their favorite primary candidate.
excess candidates are removed by single transferable vote.
other culling methods are acceptable.

phase 2:
voters cast a single vote for their favorite candidate.
working example: A,B,D received 35,30,25,10 votes respectively.
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


there are several ways to model the electorate:

1. single axis - simplest.
2. multiple axes with equal weights.
3. multiple axes where the weights decrease geometrically.

per axis:

A. uniform - distributed evenly from 0.0 to 1.0.
B. random - placed at random.
with large electorates, is indistinguishable from a uniform distribution.
C. clustered - voters are normally distributed around one of several points.
how many clusters? how much spread? to be determined.

so far i've implement 1,A,B,C.

for single axis and uniform or random distribution...
either there's a majority.
or there's a condorcet winner.
we're in a nash equilibrium.
ie there's no incentive for either the voters or the candidates to change their votes.
ie to vote strategically instead of honestly.
an exception would be when a candidate's preference isn't honest.
example: A=40 B=35 C=25 where C for whatever reason prefers A>B.
then A+C eliminate B and A wins in round 2 with votes from B.
but if enough  voters can see this coming before the election,
they will also vote dishonestly for their second choice, B.
giving B a majority win in round 1.
so C's strategy for getting a victory for A would be to lie to their constituents.
and vote against their wishes.

ties are difficult to handle.
the order of the candidates never changes.
in case of tie, the first candidate wins.
but there's a gotcha.
sometimes we vote to eliminate a candidate.
the candidate with the most last place votes is eliminated.
in other words, they lose.
in this case, the last candidate "wins" the tie.
when searching for a winner, we take the first candidate with the most votes.
when searching for a loser, we take the last candidate with the most votes.

in the art, the electorate is modeled as voters clustering around positions.
this is usually implemented as the chinese restaurant problem.
there's a dispersion parameter alpha.
N patrons are already seated at tables.
the probability the next patron sits at a table is proportional to how many
patrons are already at the table.
P(patron N+1 sits at table k) = n[k] / (N + alpha)
p(patron N+1 sits by himself) = alpha / (N + alpha)
we place voters this way.
clusters have a position and a standard deviation.
the position ranges from 0.0 to 1.0.
some outstanding questions:
how many clusters do we want?
how does that relate to alpha?

satisfaction is a rather unsatisfying metric.
the best candidate gets a 1.0.
the average candidate gets a 0.0.
what if all candidates are equally good?
if two candidates have equal nearly optimal utility...
and the third has slightly less utility...
then the third candidate has a satisfaction of -2.
even though the third candidate is just as good as the other two.
weird.
what if all candidates are equally horrible?
one of them is going to get satisfaction rating of 1.0.
even though they'd get stomped by a randomly selected voter.
weird.
satisfaction is calculated using maximum and average utility.
i tried using max and average for all voters (expensive),
and all candidates in the primary (not helpful).
so status quo it is.
and more ranting...
when you're doing a summary, the satisfaction isn't to the same scale.
so they can't really be averaged meaningfully.
could try using the worst possible candidate as the baseline.
could try using the median voter/candidate as the baseline.

we do not need check monotonicity when multiple candidates drop out,
proof by induction.
we assume we already verified we have monotonicity for N-1 candidates.
we don't need to do that work again.

where there are many candidates...
and they form a condorcet ordering...
we have a nash equilibrium.
"proof":
a majority coalition can efficiently eliminate all candidates not in the coalition.
thereby ensuring one of the coalition members wins.
in order to forma a majority coalition of the left (close to position 1)...
the guthrie winner must be included.
otherwise they don't have a majority of the voters.
same for the right.
the guthrie winner is the choice for the eliminated right minority coalition.
a right majority coalition forms that includes the guthrie winner.
all other candidates of the left coalition are eliminated.
okay.
a split coalition includes candidates left and right of the guthrie winner
and excludes the guthrie winner.
non-coalition members are eliminated.
including the guthrie winner.
a new winner is selected from the left (for example).
coalition members from the right get a worse result.
and therefore would not join such a coalition.
qed.
the interesting cases are when there are condorcet cycles.
which require two or more uncorrelated axes for the issues.


things done:

handle ties.
check if the winner is the condorcet winner if there is one.
report satisfaction (two different ways) and bayer regret.
find the voter that would be the optimal candidate.
check for monotonicity ie the winner should still win if any other candidate drops out.
check if the most satisfactory candidate wins.
they don't. but that's okay.
these are diabolical cases that no voting system can do better. except maybe range voting.
multiple trials with summarized results,
normalize electorate to range from 0.0 to 1.0.
clustered voters,

things to do:

multiple issue dimensions,
non-linear utility or piece-wise linear utility,
**/

#include "guthrie.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>

namespace {

/** number of trials. **/
constexpr int kNTrials = 1;
//constexpr int kNTrials = 30;
//constexpr int kNTrials = 300;
//constexpr int kNTrials = 1000;
//constexpr int kNTrials = 30*1000;

/** number of voters. **/
//constexpr int kNVoters = 20;
//constexpr int kNVoters = 50;
//constexpr int kNVoters = 100;
constexpr int kNVoters = 1000;
//constexpr int kNVoters = 10*1000;

/** number of candidates **/
constexpr int kNCandidates = 3;
//constexpr int kNCandidates = 4;
//constexpr int kNCandidates = 5;
//constexpr int kNCandidates = 6;
//constexpr int kNCandidates = 7;

/** options for distributing the electorate. **/
constexpr int kElectorateUniform = 0;
constexpr int kElectorateRandom = 1;
constexpr int kElectorateClusters = 2;
//constexpr int kElectorateMethod = kElectorateUniform;
constexpr int kElectorateMethod = kElectorateRandom;
//constexpr int kElectorateMethod = kElectorateClusters;

/** options for number of issue dimensions (axes). **/
//constexpr int kNAxes = 1;
constexpr int kNAxes = 2;

/** options for choosing candidates **/
constexpr int kCandidatesRandom = 0;
constexpr int kCanddiatesSingleTransferableVote = 1;
constexpr int kCandidateMethod = kCanddiatesSingleTransferableVote;

/**
with the single transferable vote primary...
we choose a number of voters to be candidates.
intutition says the number should be between the cube root (0.333)
and the square root (0.5) of the number of voters.
too few is not a representative statistical distribution of the voters.
too many takes too long and doesn't help.
might even hurt.
default compromise is about 0.4.
**/
constexpr double kPrimaryPower = 0.4;

/** option to use a fixed seed for testing. **/
constexpr std::uint64_t kFixedSeed = 0;
//constexpr std::uint64_t kFixedSeed = 1748463505672718893;

/**
option to find the theoretical best candidate from the voters.
this feature is expensive and not used by the art.
**/
//constexpr bool kFindTheoreticalBestCandidate = true;
constexpr bool kFindTheoreticalBestCandidate = false;

/**
option to show the electorate distribution.
this is a bit spammy.
**/
//constexpr bool kShowElectorateDistribution = true;
constexpr bool kShowElectorateDistribution = false;

/**
option to show details of all coombs rounds.
this is a bit spammy.
**/
//constexpr bool kShowCoombsRounds = true;
constexpr bool kShowCoombsRounds = false;


/** some functions should sometimes be quiet. **/
constexpr bool kQuiet = true;

class RandomNumberGenerator {
public:
    RandomNumberGenerator() = default;
    ~RandomNumberGenerator() = default;

    std::uint64_t seed_ = 0;
    std::mt19937_64 rng_;
    std::uniform_real_distribution<double> unif_;
    std::normal_distribution<double> norm_;

    void init() noexcept {
        seed_ = kFixedSeed;
        if (seed_ == 0) {
            seed_ = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        }
        std::seed_seq ss{uint32_t(seed_ & 0xffffffff), uint32_t(seed_>>32)};
        rng_.seed(ss);
        unif_ = std::uniform_real_distribution<double>(0.0, 1.0);
        norm_ = std::normal_distribution<double>(0.0, 1.0);
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

    double normal() noexcept {
        double x = norm_(rng_);
        return x;
    }
};
static RandomNumberGenerator *rng_ = nullptr;

class Position {
public:
    Position() = default;
    ~Position() = default;

    std::vector<double> axis_;

    /** return the distance to another position. **/
    double distance(
        const Position& other
    ) noexcept {
        int naxes = axis_.size();
        if (naxes == 1) {
            double dist = std::abs(axis_[0] - other.axis_[0]);
            return dist;
        }
        double sum2 = 0.0;
        for (int i = 0; i < naxes; ++i) {
            double dx = axis_[i] - other.axis_[i];
            sum2 += dx * dx;
        }
        double dist = std::sqrt(sum2);
        return dist;
    }

    /** format the position as a string. **/
    std::string to_string() {
        std::stringstream ss;
        int naxes = axis_.size();
        if (naxes == 1) {
            ss<<axis_[0];
        } else {
            ss<<"{";
            for (int i = 0; i < naxes; ++i) {
                if (i > 0) {
                    ss<<", ";
                }
                ss<<axis_[i];
            }
            ss<<"}";
        }
        return ss.str();
    }

    /** for sorting **/
    bool operator < (const Position& other) const
    {
        return (axis_[0] < other.axis_[0]);
    }
};

class Voter {
public:
    /** position along the axis ranges from 0..1 **/
    Position position_;
};
typedef std::vector<Voter> Voters;

class Utility {
public:
    /** candidate with the best utility. **/
    int which_ = 0;

    /** utility of the best candidate. **/
    double best_ = 0;

    /** average utility of the average candidate. **/
    double average_ = 0;
};

class Cluster {
public:
    /** size **/
    int count_ = 0;

    /** position **/
    double position_ = 0.0;

    /** for sorting **/
    bool operator < (const Cluster& other) const
    {
        return (position_ < other.position_);
    }
};
typedef std::vector<Cluster> Clusters;

class Candidate {
public:
    /** generated name **/
    char name_ = '?';

    /** position along the axis ranges from 0..1 **/
    Position position_;

    /** vote total aka asset **/
    int support_ = 0;

    /** ranking of other candidates. **/
    std::vector<int> rankings_;

    /** utility **/
    double utility_ = 0.0;

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
    int naxes_ = 0;
    Voters voters_;
    Clusters clusters_;

    void init(
        int nvoters,
        int method,
        int naxes
    ) noexcept {
        nvoters_ = nvoters;
        naxes_ = naxes;

        /** allocate space for the voters and candidates **/
        voters_.resize(nvoters);

        /** allocate space for the positions. **/
        size_position_axes();

        /** distribute the voter by the chosen method. **/
        switch (method) {
        default:
        case kElectorateUniform:
            ranked();
            break;

        case kElectorateRandom:
            random();
            break;

        case kElectorateClusters:
            clusters();
            break;
        }

        /** normalize the distribution. **/
        if (method != kElectorateUniform) {
            normalize();
        }

        /** show the distribution. **/
        show_distribution();
    }

    void size_position_axes() noexcept {
        for (auto&& voter : voters_) {
            voter.position_.axis_.resize(naxes_);
        }
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
            auto& axis = voters_[i].position_.axis_;
            axis[0] = offset + double(i) / nvoters;
        }
    }

    /**
    randomly distribute voters along the axes.
    **/
    void random() noexcept {
        LOG("Electorate is random.");
        for (int i = 0; i < nvoters_; ++i) {
            auto& axis = voters_[i].position_.axis_;
            for (int k = 0; k < naxes_; ++k) {
                axis[k] = rng_->generate();
            }
        }
    }

    /**
    cluster each axis independently.
    **/
    void clusters() noexcept {
        LOG("Electorate is clustered.");
        for (int axis = 0; axis < naxes_; ++axis) {
            clusters(axis);
        }
    }

    /**
    chinese restaurant process.
    **/
    void clusters(
        int axis
    ) noexcept {
        /** create empty clusters. **/
        int nclusters = kNCandidates * 2;
        clusters_.resize(nclusters);
        for (auto&& cluster : clusters_) {
            cluster.count_ = 0;
            cluster.position_ = rng_->generate();
        }
        std::sort(clusters_.begin(), clusters_.end());

        for (int i = 0; i < nvoters_; ++i) {
            seat_voter(i, axis);
        }
    }

    /**
    pick a random cluster and join it.
    or create a new one.

    maybe we fix the number of clusters.
    and the first voters fill empty clusters.
    how many clusters?
    maybe 2 times number of candidates.
    **/
    void seat_voter(
        int k,
        int axis
    ) noexcept {
        /**
        what should the standard deviation be?
        should it start small and grow with the size of the table?
        this number found by trial and error.
        **/
        constexpr double kStdDev = 0.07;

        /** pick a cluster. **/
        int where = 0;
        int nclusters = clusters_.size();
        if (k < nclusters) {
            /** seat the first n voters in their own cluster. **/
            where = k;
        } else {
            /** populare clusters attract more people. **/
            int rn = rng_->generate(k);
            for (auto&& cluster : clusters_) {
                rn -= cluster.count_;
                if (rn < 0) {
                    break;
                }
                ++where;
            }
            if (where >= nclusters) {
                LOG("=tsc= "<<__LINE__<<": uh oh! where="<<where<<" > "<<nclusters<<"="<<nclusters);
            }
        }

        /** add the voter to the cluster. **/
        auto& cluster = clusters_[where];
        cluster.count_ += 1;

        /** position the voter. **/
        auto& voter = voters_[k];
        double rn = rng_->normal() * kStdDev;
        double position = cluster.position_ + rn;
        voter.position_.axis_[axis] = position;
    }

    /**
    normalize all axes.
    **/
    void normalize() noexcept {
        for (int axis = 0; axis < naxes_; ++axis) {
            normalize(axis);
        }
    }

    /**
    normalize the voters so they range from 0 to 1 along this axis.
    **/
    void normalize(
        int axis
    ) noexcept {
        double mn = 1e99;
        double mx = -1e99;
        for (auto&& voter : voters_) {
            mn = std::min(mn, voter.position_.axis_[axis]);
            mx = std::max(mx, voter.position_.axis_[axis]);
        }

        double scale = 1.0 / (mx - mn);
        for (auto&& voter : voters_) {
            double pos = voter.position_.axis_[axis];
            pos -= mn;
            pos *= scale;
            pos = std::clamp(pos, 0.0, 1.0);
            voter.position_.axis_[axis] = pos;
        }
    }

    /**
    show all axes.
    **/
    void show_distribution() noexcept {
        if (kShowElectorateDistribution == false) {
            return;
        }
        LOG("Electorate distribution:");
        for (int axis = 0; axis < naxes_; ++axis) {
            if (naxes_ > 1) {
                LOG("Axis: "<<axis);
            }
            show_distribution(axis);
        }
    }

    /**
    show distribution for one axis.
    **/
    void show_distribution(
        int axis
    ) noexcept {
        constexpr int kNBins = 20;
        std::vector<int> bins;
        bins.resize(kNBins);
        for (int i = 0; i < kNBins; ++i) {
            bins[i] = 0;
        }
        for (auto&& voter : voters_) {
            double position = voter.position_.axis_[axis];
            int i = std::floor(position * kNBins);
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

    /** "constants" **/
    int ntrials_ = kNTrials;
    int nvoters_ = kNVoters;
    int naxes_ = kNAxes;
    int electorate_method_ = kElectorateMethod;
    int ncandidates_ = kNCandidates;
    int canddiate_method_ = kCandidateMethod;

    /** the electorate and the candidates. **/
    Electorate electorate_;
    Candidates candidates_;

    /** results from the trial. **/
    int winner_ = 0;
    Utility theoretical_;
    Utility actual_;

    /** summary **/
    double total_satisfaction_ = 0.0;
    bool won_first_round_ = 0;
    int majority_winners_ = 0;
    double min_satisfaction_ = 1.0;
    int winner_maximizes_satisfaction_ = 0;
    int winner_is_condorcet_ = 0;
    int monotonicity_ = 0;

    void run() noexcept {
        /** initialize the random number generators. **/
        rng_ = new(std::nothrow) RandomNumberGenerator();
        rng_->init();

        /** some sanity checks. **/
        sanity_checks();

        /** hello, world. **/
        LOG("Guthrie voting analysis:");
        show_header();

        /** run many trials. **/
        for (int trial = 1; trial <= ntrials_; ++trial) {
            if (ntrials_ > 1) {
                LOG("");
                LOG("Trial: "<<trial);
            }

            /** initialize the electorate and candidates. **/
            electorate_.init(nvoters_, electorate_method_, naxes_);
            find_best_candidate();
            init_candidates();

            /** vote, find winner, check results. **/
            vote();
            find_winner();
            /** for summary **/
            if (won_first_round_) {
                ++majority_winners_;
            }
            show_satisfaction();
            check_criteria();
        }

        /** log the results. **/
        show_summary();
    }

    void sanity_checks() noexcept {
        if (ntrials_ < 1) {
            LOG("Error: Need at least 1 trial ("<<ntrials_<<").");
            std::exit(1);
        }
        if (ncandidates_ < 3) {
            LOG("Error: Need at least 3 candidates ("<<ncandidates_<<").");
            exit(1);
        }
        if (nvoters_ < ncandidates_) {
            LOG("Error: Need more voters ("<<nvoters_<<") than candidates ("<<ncandidates_<<").");
            exit(1);
        }

        /** uniform electorate is a single axis. **/
        if (kElectorateMethod == kElectorateUniform) {
            if (naxes_ != 1) {
                LOG("Warning: Uniform electorate requires number of issue axes ("<<naxes_<<") to be 1, overriding.");
                naxes_ = 1;
            }
        }
    }

    /** show configuration. **/
    void show_header() noexcept {
        LOG("Configuration:");
        LOG("Number trials    : "<<ntrials_);
        LOG("Number voters    : "<<nvoters_);
        LOG("Number candidates: "<<ncandidates_);
        if (kFixedSeed == 0) {
            LOG("Random seed      : "<<rng_->seed_);
        } else {
            LOG("Fixed seed       : "<<rng_->seed_);
        }
    }

    /**
    find the voter that would make the best candidate.
    also find the average utility of all voters.
    **/
    void find_best_candidate() noexcept {
        if (kFindTheoreticalBestCandidate == false) {
            return;
        }

        int best = 0;
        double best_utility = 1e99;
        Position *best_position = nullptr;
        double total_utility = 0.0;
        for (int i = 0; i < nvoters_; ++i) {
            auto& ipos = electorate_.voters_[i].position_;
            double utility = 0.0;
            for (int k = 0; k < nvoters_; ++k) {
                auto& kpos = electorate_.voters_[k].position_;
                utility += ipos.distance(kpos);
            }
            if (utility < best_utility) {
                best = i;
                best_utility = utility;
                best_position = &ipos;
            }
            total_utility += utility;
        }

        /**
        save the best candidate and utility.
        and the average utility of all candidates.
        **/
        theoretical_.which_ = best;
        theoretical_.best_ = best_utility;
        theoretical_.average_ = total_utility / double(nvoters_);

        /** show results. **/
        LOG("Best candidate chosen from all voters:");
        LOG("  Position: "<<best_position->to_string());
        LOG("  Utility : "<<best_utility);
        LOG("  Average : "<<theoretical_.average_);
    }

    void init_candidates() noexcept {
        /**
        pick primary candidates from the voters.
        eliminate most of them.
        sort them by the major axis.
        name them
        figure out how candidates rank each other.
        calculate voter satisfactions.
        **/
        LOG("Selecting candidates from the electorate.");
        pick_candidates_from_electorate();
        single_transferable_vote_primary();
        /** sort them. **/
        std::sort(candidates_.begin(), candidates_.end());
        name_candidates();
        show_candidate_positions();
        rank_candidates();
        calculate_utilities(actual_);
    }

    void pick_candidates_from_electorate() noexcept {
        /** choose the number of candidates based on the specified method. **/
        int n = ncandidates_;
        if (canddiate_method_ == kCanddiatesSingleTransferableVote) {
            /** use the cube root of the numbe of voters. **/
            double cube_root = std::pow(double(nvoters_), kPrimaryPower);
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
    }

    void single_transferable_vote_primary() noexcept {
        /**
        reduce the number of candidates.
        remove the ones with the lowest vote counts.
        we don't care about ties.
        **/
        for(;;) {
            int n = candidates_.size();
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
    }

    void name_candidates() noexcept {
        /** name them in sorted normalized order **/
        for (int i = 0; i < ncandidates_; ++i) {
            auto& candidate = candidates_[i];
            candidate.name_ = 'A' + i;
        }
    }

    void show_candidate_positions() noexcept {
        LOG("Candidate positions:");
        for (auto&& candidate : candidates_ ) {
            LOG(candidate.name_<<": "<<candidate.position_.to_string());
        }
    }

    void rank_candidates(
        bool quiet = false
    ) noexcept {
        /** every candidate rank orders the others. **/
        for (auto&& candidate : candidates_) {
            rank_other_candidates(candidate);
        }

        if (quiet == false) {
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
    }

    void rank_other_candidates(
        Candidate& candidate
    ) noexcept {
        std::vector<double> distance;
        distance.reserve(ncandidates_);
        candidate.rankings_.reserve(ncandidates_);

        for (int i = 0; i < ncandidates_; ++i) {
            auto& other = candidates_[i];
            double d = candidate.position_.distance(other.position_);
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

    void calculate_utilities(
        Utility& result
    ) noexcept {
        /** this might be the primary with extra candidates. **/
        int n = candidates_.size();

        /**
        calculate candidate utility.
        remember who is best.
        **/
        int best_candidate = 0;
        double best_utility = 1e99;
        double total_utility = 0.0;
        for (int i = 0; i < n; ++i) {
            /** calculate the utility for this candidate. **/
            auto& candidate = candidates_[i];
            double utility = 0.0;
            for (auto&& voter : electorate_.voters_) {
                utility += candidate.position_.distance(voter.position_);
            }
            candidate.utility_ = utility;

            /** update best and sum **/
            if (best_utility > utility) {
                best_candidate = i;
                best_utility = utility;
            }
            total_utility += utility;
        }

        /** compute utility or random candidate. **/
        double random_utility = total_utility / double(n);

        result.which_ = best_candidate;
        result.best_ = best_utility;
        result.average_ = random_utility;
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
    }

    int find_closest_candidate(
        Position &position
    ) noexcept {
        int closest_candidate = 0;
        double closest_distance = double(ncandidates_);
        int ncandidates = candidates_.size();
        for (int i = 0; i < ncandidates; ++i) {
            auto& candidate = candidates_[i];
            double distance = candidate.position_.distance(position);
            if (distance < closest_distance) {
                closest_candidate = i;
                closest_distance = distance;
            }
        }
        return closest_candidate;
    }

    void find_winner(
        bool quiet = false
    ) noexcept {
        bool show_everything = !quiet;
        if (kShowCoombsRounds == false) {
            show_everything = false;
        }
        bool show_required = !quiet;

        std::vector<int> counts;
        counts.resize(ncandidates_);

        /** summary statistics. **/
        won_first_round_ = false;

        /**
        normally we can find the winner in N-1 rounds.
        unless there's a tie in the last round.
        **/
        for (int round = 1; /*round < ncandidates_*/; ++round) {
            if (show_everything) {
                LOG("Round "<<round<<":");
            }

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
            bool show_it = show_everything;
            if (show_required && round == 1) {
                show_it = true;
            }
            if (show_it) {
                LOG("First place vote counts:");
                for (int i = 0; i < ncandidates_; ++i) {
                    auto& candidate = candidates_[i];
                    LOG(candidate.name_<<": "<<counts[i]);
                }
            }

            /** check for majority. **/
            for (int i = 0; i < ncandidates_; ++i) {
                if (2*counts[i] > nvoters_) {
                    winner_ = i;
                    if (show_required) {
                        auto& candidate = candidates_[i];
                        LOG(candidate.name_<<" wins Guthrie voting in round "<<round<<".");
                    }
                    /** summary statistic. **/
                    if (round == 1) {
                        won_first_round_ = true;
                    }
                    return;
                }
            }

            /** initialize the counts **/
            for (int i = 0; i < ncandidates_; ++i) {
                counts[i] = 0;
            }

            /**
            count last place votes.
            **/
            int last_index = ncandidates_ - round;
            for (auto&& candidate : candidates_) {
                int worst = candidate.rankings_[last_index];
                counts[worst] += candidate.support_;
            }

            /** find the candidate with the most last place votes. **/
            int loser = 0;
            int loser_count = -1;
            for (int i = 0; i < ncandidates_; ++i) {
                int count = counts[i];
                /**
                we have this implied bias that the first candidate in the list wins ties.
                however in case, we're looking for the loser.
                if there's a tie we want to find the last candidate in the list.
                hence the comparison is greater than or equal to.
                instead of just greater than.
                **/
                if (count >= loser_count) {
                    loser = i;
                    loser_count = count;
                }
            }

            /** remove the loser from the candidate rankings. **/
            for (auto&& candidate : candidates_) {
                for (auto it = candidate.rankings_.begin(); it < candidate.rankings_.end(); ++it) {
                    if (*it == loser) {
                        candidate.rankings_.erase(it);
                        break;
                    }
                }
            }

            if (show_everything) {
                LOG("No candidate has a majority.");

                LOG("Last place vote counts:");
                for (int i = 0; i < ncandidates_; ++i) {
                    auto& candidate = candidates_[i];
                    LOG(candidate.name_<<": "<<counts[i]);
                }
                LOG("Candidate "<<candidates_[loser].name_<<" has the most last place votes - eliminated.");

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
    }

    /**
    voter satisfaction is a function of the utility of a candidate.
    the best candidate has satisfaction 1.0.
    the average candidate has a satisfaction of 0.0.
    worse candidates have negative satisfaction.

    show the voter satisfication for each candidate a number of ways.
    standard: considers just the candidates.
    primary: includes candidates eliminated in the primary.
    all possible: includes all voters.
    **/
    void show_satisfaction() noexcept {
        LOG("");
        LOG("Voter satisfaction (utility):");
        calculate_satisfaction(actual_);

        if (kFindTheoreticalBestCandidate) {
            LOG("Voter satisfaction (all possible):");
            calculate_satisfaction(theoretical_);
        }
    }

    void calculate_satisfaction(
        Utility utility
    ) noexcept {
        double best = utility.best_;
        double average = utility.average_;
        double denom = average - best;
        for (auto&& candidate : candidates_) {
            double dutility = average - candidate.utility_;
            double satisfaction = dutility / denom;
            LOG(candidate.name_<<": "<<satisfaction<<" ("<<dutility<<")");
        }
    }

    void check_criteria() noexcept {
        LOG("");
        LOG("Checking voting criteria.");
        int max_satisfaction = find_max_satisfaction_candidate();
        int condorcet = find_condorcet_candidate();
        int monotonicity = check_monotonicity();

        if (winner_ == max_satisfaction) {
            ++winner_maximizes_satisfaction_;
        }
        if (winner_ == condorcet) {
            ++winner_is_condorcet_;
        }
        if (winner_ == monotonicity) {
            ++monotonicity_;
        }

        LOG("");
        LOG("Voting criteria results:");
        const char *result = nullptr;
        result = result_to_string(winner_, max_satisfaction);
        LOG("Maximizes voter satisfaction: "<<result);
        result = result_to_string(winner_, condorcet);
        LOG("Condorcet majority          : "<<result);
        result = result_to_string(winner_, monotonicity);
        LOG("Monotonicity                : "<<result);
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

    int find_max_satisfaction_candidate() noexcept {
        /** show results **/
        int winner = actual_.which_;
        auto& best_candidate = candidates_[winner];
        LOG(best_candidate.name_<<" maximizes voter satisfaction.");

        /** satisfaction = (utility - average) / (best - average) **/
        auto& winning_candidate = candidates_[winner_];
        double satisfaction = (winning_candidate.utility_ - actual_.average_) / (actual_.best_ - actual_.average_);

        /** best candididate has satisfaction of 1.0. **/
        double regret = 1.0 - satisfaction;
        LOG("Voter bayesian regret (standard): "<<regret);

        /** update summary **/
        total_satisfaction_ += satisfaction;
        min_satisfaction_ = std::min(min_satisfaction_, satisfaction);

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

    /**
    return true if candidate a will beat candidate b
    in a head to head matchup.
    **/
    bool head_to_head(
        int a,
        int b
    ) noexcept {
        /** init counts **/
        int avotes = 0;
        int bvotes = 0;

        /** candidate positions. **/
        auto& apos = candidates_[a].position_;
        auto& bpos = candidates_[b].position_;

        /** count votes. **/
        for (auto&& voter : electorate_.voters_) {
            double adist = voter.position_.distance(apos);
            double bdist = voter.position_.distance(bpos);
            if (adist < bdist) {
                ++avotes;
            }
        }
        bvotes = nvoters_ - avotes;

        /**
        we assume that a comes before b in the candidate list.
        and that we're looking for a winner.
        in which case, by the rules, a wins ties.
        **/
        return (avotes >= bvotes);
    }

    int check_monotonicity() noexcept {
        /** assume we pass. **/
        int monotonicity = winner_;

        /** save the original winner **/
        int original_winner = winner_;

        /** save the name of the original winner. **/
        char original_winner_name = candidates_[original_winner].name_;

        /** save the original candidates. **/
        auto original_candidates = candidates_;

        /** decrement the number of candidates. **/
        int ncandidates = ncandidates_;
        ncandidates_ = ncandidates - 1;

        /** remove the first candidate. **/
        candidates_.erase(candidates_.begin());

        /** remove one of the non-winners and revote. **/
        for (int i = 0; i < ncandidates; ++i) {
            /** skip the winner. **/
            if (i != original_winner) {
                /** re-vote. **/
                rank_candidates(kQuiet);
                vote();
                find_winner(kQuiet);

                /** check by name, not index. **/
                char winner_name = candidates_[winner_].name_;
                if (winner_name != original_winner_name) {
                    monotonicity = i;
                    LOG(winner_name<<" wins if "<<original_candidates[i].name_<<" doesn't run.");
                }
            }

            /**
            update the list of candidates.
            avoid buffer overrun at end of loop.
            3 candidates.
            i ranges from 0,1,2.
            original size is 3.
            current size is 2.
            **/
            if (i < ncandidates_) {
                candidates_[i] = original_candidates[i];
            }
        }

        /** restore the original candidates, count, and winner. **/
        std::swap(candidates_, original_candidates);
        ncandidates_ = ncandidates;
        winner_ = original_winner;

        /** log that we passed the test. **/
        if (monotonicity == original_winner) {
            LOG(original_winner_name<<" wins if any other candidate doesn't run.");
        }

        return monotonicity;
    }

    void show_summary() noexcept {
        if (ntrials_ <= 1) {
            return;
        }

        double denom = double(ntrials_);
        double satisfaction = double(total_satisfaction_) / denom;
        double min_satisfaction = min_satisfaction_;
        double regret = 1.0 - satisfaction;
        double max_regret = 1.0 - min_satisfaction;
        double majority_winners = 100.0 * double(majority_winners_) / denom;
        double maximizes_satisfaction = 100.0 * double(winner_maximizes_satisfaction_) / denom;
        double is_condorcet = 100.0 * double(winner_is_condorcet_) / denom;
        double monotonicity = 100.0 * double(monotonicity_) / denom;

        LOG("");
        show_header();
        LOG("");
        LOG("Summary:");
        LOG("Voter satisfaction (min)     : "<<satisfaction<<" ("<<min_satisfaction<<")");
        LOG("Voter regret (max)           : "<<regret<<" ("<<max_regret<<")");
        LOG("Won outright by true majority: "<<majority_winners<<"%");
        LOG("Maximizes voter satisfaction : "<<maximizes_satisfaction<<"%");
        LOG("Agrees with Condorcet        : "<<is_condorcet<<"%");
        LOG("Monotonicity                 : "<<monotonicity<<"%");
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
