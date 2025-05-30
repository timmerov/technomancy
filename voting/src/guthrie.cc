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
see electorate.h for details.

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
multiple issue dimensions,

things to do:

non-linear utility or piece-wise linear utility,
**/

#include "electorate.h"
#include "guthrie.h"
#include "random.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <vector>


namespace {

/** number of trials. **/
//constexpr int kNTrials = 1;
//constexpr int kNTrials = 10;
//constexpr int kNTrials = 30;
//constexpr int kNTrials = 300;
constexpr int kNTrials = 1000;
//constexpr int kNTrials = 10*1000;
//constexpr int kNTrials = 30*1000;

/** number of voters. **/
//constexpr int kNVoters = 20;
//constexpr int kNVoters = 50;
//constexpr int kNVoters = 100;
constexpr int kNVoters = 1000;
//constexpr int kNVoters = 10*1000;

/** number of candidates **/
//constexpr int kNCandidates = 3;
constexpr int kNCandidates = 3;
//constexpr int kNCandidates = 5;
//constexpr int kNCandidates = 6;
//constexpr int kNCandidates = 7;

/** options for distributing the electorate. **/
//constexpr int kElectorateMethod = kElectorateUniform;
//constexpr int kElectorateMethod = kElectorateRandom;
constexpr int kElectorateMethod = kElectorateClusters;

/** options for number of issue dimensions (axes). **/
//constexpr int kNAxes = 1;
constexpr int kNAxes = 2;
//constexpr int kNAxes = 3;

/** options for clustered method. **/
constexpr int kNClusters = kNCandidates * 2;

/**
option for relative weighting of the axes.
the major axis has a weight of 1.0.
successive axes are scaled by this factor.
should be >0.0 and <=1.0.
intutition says 0.4 would be reasonable.
this means most real world issues are reasonably correlated.
and/or the minor issues are less important than the major one.
which seems to be the case.
1.0 means we have many uncorrelated issues.
and they are all equally important.
which seems a bit of a stretch.
**/
constexpr double kAxisWeightDecay = 0.4;

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
constexpr std::uint64_t kSeedChoice = 0;
//constexpr std::uint64_t kSeedChoice = 1748632174640245442;

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
option to show the voter blocs.
this is a bit spammy.
**/
constexpr bool kShowVoterBlocs = true;
//constexpr bool kShowVoterBlocs = false;

/**
option to show details of all coombs rounds.
this is a bit spammy.
**/
//constexpr bool kShowCoombsRounds = true;
constexpr bool kShowCoombsRounds = false;


/** some functions should sometimes be quiet. **/
constexpr bool kQuiet = true;


class Utility {
public:
    /** candidate with the best utility. **/
    int which_ = 0;

    /** utility of the best candidate. **/
    double best_ = 0;

    /** average utility of the average candidate. **/
    double average_ = 0;
};

typedef std::vector<int> Rankings;
typedef std::vector<double> Distances;

class Candidate {
public:
    /** generated name **/
    char name_ = '?';

    /** position along the axis ranges from 0..1 **/
    Position position_;

    /** vote total aka asset **/
    int support_ = 0;

    /** ranking of other candidates. **/
    Rankings rankings_;
    Distances distances_;

    /** utility **/
    double utility_ = 0.0;

    /** for sorting **/
    bool operator < (const Candidate& other) const
    {
        return (position_ < other.position_);
    }
};
typedef std::vector<Candidate> Candidates;

class Bloc {
public:
    /** voters in this bloc. **/
    int size_ = 0;

    /** distances to candidates in ranked order. **/
    Distances distances_;
};

typedef std::map<Rankings, Bloc> BlocMap;

class GuthrieImpl {
public:
    GuthrieImpl() = default;
    GuthrieImpl(const GuthrieImpl &) = delete;
    GuthrieImpl(GuthrieImpl &&) = delete;
    ~GuthrieImpl() = default;

    /** "constants" **/
    int ntrials_ = kNTrials;
    int ncandidates_ = kNCandidates;
    int canddiate_method_ = kCandidateMethod;

    /** the electorate and the candidates. **/
    Electorate electorate_;
    Candidates candidates_;
    BlocMap bloc_map_;

    /** results from the trial. **/
    int winner_ = 0;
    Utility theoretical_;
    Utility actual_;

    /** summary **/
    double total_satisfaction_ = 0.0;
    double total_satisfaction_plurality_ = 0.0;
    double total_satisfaction_condorcet_ = 0.0;
    int winner_plurality_ = 0;
    bool won_first_round_ = 0;
    int majority_winners_ = 0;
    double min_satisfaction_ = 1.0;
    int winner_maximizes_satisfaction_ = 0;
    int winner_is_condorcet_ = 0;
    int monotonicity_ = 0;

    void run() noexcept {
        /** initialize the random number generators. **/
        RandomNumberGenerator::init(kSeedChoice);

        /** configure the electorate. **/
        electorate_.nvoters_ = kNVoters;
        electorate_.method_ = kElectorateMethod;
        electorate_.naxes_ = kNAxes;
        electorate_.axis_weight_decay_ = kAxisWeightDecay;
        electorate_.nclusters_ = kNClusters;

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
            electorate_.init();
            if (kShowElectorateDistribution) {
                electorate_.show_distribution();
            }
            find_best_candidate();
            init_candidates();

            /** vote, find winner, check results. **/
            create_blocs();
            vote();
            find_winner();
            winner_summaries();
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
        if (electorate_.nvoters_ < ncandidates_) {
            LOG("Error: Need more voters ("<<electorate_.nvoters_<<") than candidates ("<<ncandidates_<<").");
            exit(1);
        }

        /** uniform electorate is a single axis. **/
        if (kElectorateMethod == kElectorateUniform) {
            if (electorate_.naxes_ != 1) {
                LOG("Warning: Uniform electorate requires number of issue axes ("<<electorate_.naxes_<<") to be 1, overriding.");
                electorate_.naxes_ = 1;
            }
        }
    }

    /** show configuration. **/
    void show_header() noexcept {
        auto seed = Rng::get_seed();

        LOG("Configuration:");
        LOG("Number trials    : "<<ntrials_);
        LOG("Number voters    : "<<electorate_.nvoters_);
        LOG("Number candidates: "<<ncandidates_);
        if (kSeedChoice == 0) {
            LOG("Random seed      : "<<seed);
        } else {
            LOG("Fixed seed       : "<<seed);
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
        for (int i = 0; i < electorate_.nvoters_; ++i) {
            auto& ipos = electorate_.voters_[i].position_;
            double utility = 0.0;
            for (int k = 0; k < electorate_.nvoters_; ++k) {
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
        theoretical_.average_ = total_utility / double(electorate_.nvoters_);

        /** show results. **/
        LOG("Best candidate chosen from all voters:");
        LOG(" Position: "<<best_position->to_string());
        LOG(" Utility : "<<best_utility);
        LOG(" Average : "<<theoretical_.average_);
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
            double cube_root = std::pow(double(electorate_.nvoters_), kPrimaryPower);
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
        duplicates.resize(electorate_.nvoters_);
        for (int i = 0; i < electorate_.nvoters_; ++i) {
            duplicates[i] = false;
        }
        for (auto&& candidate : candidates_) {
            int i = 0;
            for(;;) {
                i = Rng::generate(electorate_.nvoters_);
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
            create_blocs(kQuiet);
            vote();
            int worst = 0;
            int worst_support = 0x7FFFFFFF;
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
            LOG(" "<<candidate.name_<<": "<<candidate.position_.to_string());
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
                ss<<" "<<candidate.name_<<":";
                for (int i = 1; i < ncandidates_; ++i) {
                    int rank = candidate.rankings_[i];
                    double dist = candidate.distances_[i];
                    if (i > 1) {
                        ss<<" >";
                    }
                    ss<<" "<<candidates_[rank].name_<<" ("<<dist<<")";
                }
                LOG(ss.str());
            }
        }
    }

    void rank_other_candidates(
        Candidate& candidate
    ) noexcept {
        candidate.rankings_.reserve(ncandidates_);
        candidate.distances_.reserve(ncandidates_);
        candidate.rankings_.clear();
        candidate.distances_.clear();

        for (int i = 0; i < ncandidates_; ++i) {
            auto& other = candidates_[i];
            double d = candidate.position_.distance(other.position_);
            int k = 0;
            for (; k < i; ++k) {
                if (candidate.distances_[k] > d) {
                    break;
                }
            }
            candidate.rankings_.insert(candidate.rankings_.begin() + k, i);
            candidate.distances_.insert(candidate.distances_.begin() + k, d);
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

    /**
    divide the electorate up into blocks.
    computing the distance between voters and candidates is expensive.
    will get worse when we convert from distance to utility.
    **/
    void create_blocs(
        bool quiet = (kShowVoterBlocs==false)
    ) noexcept {
        /**
        when there are 3 candidates...
        the voters can only be ABC, ACB, BAC, BCA, CAB, CBA.
        **/
        bloc_map_.clear();
        Rankings rankings;
        Distances distances;
        int n = candidates_.size();

        for (auto&& voter : electorate_.voters_) {
            rankings.reserve(n);
            distances.reserve(n);
            rankings.clear();
            distances.clear();

            for (int i = 0; i < n; ++i) {
                double distance = voter.position_.distance(candidates_[i].position_);
                int k = 0;
                for (; k < i; ++k) {
                    if (distances[k] > distance) {
                        break;
                    }
                }
                rankings.insert(rankings.begin() + k, i);
                distances.insert(distances.begin() + k, distance);
            }

            auto it = bloc_map_.find(rankings);
            if (it == bloc_map_.end()) {
                Bloc bloc;
                bloc.size_ = 1;
                bloc.distances_ = std::move(distances);
                bloc_map_.insert({std::move(rankings), std::move(bloc)});
            } else {
                auto& found_bloc = it->second;
                ++found_bloc.size_;
                for (int i = 0; i < n; ++i) {
                    found_bloc.distances_[i] += distances[i];
                }
            }
        }

        if (quiet == false) {
            int n = bloc_map_.size();
            LOG("Voter blocs ("<<n<<"):");
            for (auto&& it : bloc_map_) {
                auto& rankings = it.first;
                auto& bloc = it.second;
                std::stringstream ss;
                ss<<" ";
                int nrankings = rankings.size();
                for (int k = 0; k < nrankings; ++k) {
                    int which = rankings[k];
                    ss<<candidates_[which].name_;
                }
                ss<<" size: "<<bloc.size_<<" distances:";
                for (int i = 0; i < nrankings; ++i) {
                    ss<<" "<<bloc.distances_[i];
                }
                LOG(ss.str());
            }
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
        for each voter bloc...
        give the support to that candidate.
        **/
        for (auto&& it : bloc_map_) {
            auto& rankings = it.first;
            auto& bloc = it.second;
            int favorite = rankings[0];
            candidates_[favorite].support_ += bloc.size_;
        }
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

            /** keep stats for plurality winners. **/
            if (round == 1) {
                winner_plurality_ = 0;
                int mx = -1;
                for (int i = 0; i < ncandidates_; ++i) {
                    int count = counts[i];
                    if (count > mx) {
                        mx = count;
                        winner_plurality_ = i;
                    }
                }
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
                    LOG(" "<<candidate.name_<<": "<<counts[i]);
                }
            }

            /** check for majority. **/
            for (int i = 0; i < ncandidates_; ++i) {
                if (2*counts[i] > electorate_.nvoters_) {
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
                    LOG(" "<<candidate.name_<<": "<<counts[i]);
                }
                LOG(" Candidate "<<candidates_[loser].name_<<" has the most last place votes - eliminated.");

                LOG("Updated rankings:");
                for (auto&& candidate : candidates_ ) {
                    std::stringstream ss;
                    ss<<" "<<candidate.name_<<":";
                    for (int i = 0; i < last_index; ++i) {
                        int rank = candidate.rankings_[i];
                        if (i > 0) {
                            ss<<" >";
                        }
                        ss<<" "<<candidates_[rank].name_;
                    }
                    LOG(ss.str());
                }
            }
        }
    }

    /**
    called after find_winner to collect data for the summary.
    **/
    void winner_summaries() noexcept {
        /** accumulate the satisfaction for the plurality winner. **/
        auto& candidate = candidates_[winner_plurality_];
        LOG(candidate.name_<<" is the plurality winner.");
        double sat = (actual_.average_ - candidate.utility_) / (actual_.average_ - actual_.best_);
        total_satisfaction_plurality_ += sat;

        /** track how many times we have win by majority. **/
        if (won_first_round_) {
            ++majority_winners_;
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
        const Utility& utility
    ) noexcept {
        double best = utility.best_;
        double average = utility.average_;
        double denom = average - best;
        for (auto&& candidate : candidates_) {
            double dutility = average - candidate.utility_;
            double satisfaction = dutility / denom;
            LOG(" "<<candidate.name_<<": "<<satisfaction<<" ("<<dutility<<")");
        }
    }

    double calculate_satisfaction(
        double candidate_utility,
        const Utility& utility
    ) noexcept {
        double best = utility.best_;
        double average = utility.average_;
        double denom = average - best;
        double satisfaction = (average - candidate_utility) / denom;
        return satisfaction;
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
        LOG("Voter bayesian regret: "<<regret);

        /** update summary **/
        total_satisfaction_ += satisfaction;
        min_satisfaction_ = std::min(min_satisfaction_, satisfaction);

        return winner;
    }

    class HeadToHead {
    public:
        int winner_ = 0;
        int loser_ = 0;
        int winner_votes_ = 0;
        int loser_votes_ = 0;
    };

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
                HeadToHead result;
                head_to_head(i, k, result);
                LOG(" "<<candidates_[result.winner_].name_<<" ("<<result.winner_votes_<<") > "
                    <<candidates_[result.loser_].name_<<" ("<<result.loser_votes_<<")");
                ++wins[result.winner_];
            }
        }

        /**
        find the candidate with the most wins.
        find the average utility while we're here.
        **/
        int max = -1;
        int winner = 0;
        int nwinners = 0;
        double total_utility = 0.0;
        for (int i = 0; i < ncandidates_; ++i) {
            int w = wins[i];
            bool sum_it = false;
            if (max == w) {
                ++nwinners;
                sum_it = true;
            } else if (max < w) {
                winner = i;
                nwinners = 1;
                max = w;
                total_utility = 0;
                sum_it = true;
            }
            if (sum_it) {
                total_utility += candidates_[i].utility_;
            }
        }

        /** no winner if there's a cycle. **/
        if (nwinners > 1) {
            LOG("Condorcet cycle exists.");
            winner = -1;
        }

        /**
        average the utility for all candidates in the cycle if any.
        accumulate for the summary.
        **/
        double utility = total_utility / double(nwinners);
        total_satisfaction_condorcet_ += calculate_satisfaction(utility, actual_);

        return winner;
    }

    /**
    return true if candidate a will beat candidate b
    in a head to head matchup.
    **/
    void head_to_head(
        int a,
        int b,
        HeadToHead &result
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
        bvotes = electorate_.nvoters_ - avotes;

        /**
        we assume that a comes before b in the candidate list.
        and that we're looking for a winner.
        in which case, by the rules, a wins ties.
        **/
        if (avotes >= bvotes) {
            result.winner_ = a;
            result.loser_ = b;
            result.winner_votes_ = avotes;
            result.loser_votes_ = bvotes;
        } else {
            result.winner_ = b;
            result.loser_ = a;
            result.winner_votes_ = bvotes;
            result.loser_votes_ = avotes;
        }
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
                create_blocs(kQuiet);
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
        double satisfaction = total_satisfaction_ / denom;
        double min_satisfaction = min_satisfaction_;
        double regret = 1.0 - satisfaction;
        double max_regret = 1.0 - min_satisfaction;
        double satisfaction_plurality = total_satisfaction_plurality_ / denom;
        double satisfaction_condorcet = total_satisfaction_condorcet_/ denom;
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
        LOG("Voter satisfaction plurality : "<<satisfaction_plurality);
        LOG("Voter satisfaction condorcet : "<<satisfaction_condorcet);
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
