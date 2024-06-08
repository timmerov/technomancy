/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
here's the problem:
you have 2 light bulbs.
there's a 30 story building.
you must find the highest floor from which a dropped bulb won't break.
and since you are lazy, you want an algorithm that minimizes the worst
case number of stairs you need to climb.

assume:
if a light bulb breaks when dropped from floor N it will break from all higher floors.
if a libht bulb does not break when dropped from floor N it will not break from all lower floors.
a light bulb that does not break is not damaged.
every time we go up a flight of stairs we have to climb back down.
ergo we only track climbing up.
coming back down is free.

setup:
number floors from 0 to N-1.

assumptions:
the cost of dropping a light bulb from floor 0 is 0.
so let's assume that we do that.
and further assume the bulb does not break.
cause if it does we're done.

it's pretty easy to find the number of stairs you need to climb.
but that's pretty useless.
you really need to know where to drop the bulbs.
a list of the floor at which you drop the bulbs is printed.

this problem lends itself well to dynamic programming.
it's recursive.
so we'd be calculating and recalculating functions with the same parameters.
which is dumb.
but...
the list of floor drops is not saved in the cache.
so...
you can either have super fast execution time.
or you can have the full answer.
**/

#include <sstream>
#include <vector>

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

namespace {
class LightBulb {
public:
    LightBulb() = default;
    ~LightBulb() = default;

    int n_floors_ = 30;
    bool verbose_ = false;
    bool use_cache_ = false;
    std::vector<int> floors_;
    std::vector<int> cache1_;
    std::vector<int> cache2_;

    void run() noexcept {
        init();

        LOG("How many stairs do you need to climb to find the breaking floor of a "<<n_floors_<<" story building?");
        int n_stairs = cost2LightBulbs(0);
        LOG("Answer: "<<n_stairs<<" flights of stairs.");
        print_floors();
        print_costs();
    }

    void print_floors() noexcept {
        /**
        if we are using the cache...
        then we don't have floor information.
        **/
        if (use_cache_) {
            return;
        }

        std::stringstream ss;
        ss<<"Drop bulbs from these floors:";
        int sz = floors_.size();
        for (int i = 0; i < sz; ++i) {
            ss<<" "<<floors_[i];
        }
        ss<<".";
        LOG(ss.str());
        LOG("Drop bulbs in pairs in the order specified.");
        LOG("If the first breaks, descend to the lowest unknown floor and drop the second.");
        LOG("When you have a broken bulb, test all unknown floors in ascending order.");
    }

    void print_costs() noexcept {
        std::stringstream ss2;
        ss2<<"cost2LightBulbs:   ";
        for (int i = 0; i < n_floors_; ++i) {
            ss2<<" "<<cache2_[i];
        }
        LOG(ss2.str());
        std::stringstream ss1;
        ss1<<"cost1of2LightBulbs:";
        for (int i = 0; i < n_floors_; ++i) {
            ss1<<" "<<cache1_[i];
        }
        LOG(ss1.str());
    }

    /**
    initialize the answer cache.
    **/
    void init() noexcept {
        cache1_.resize(n_floors_);
        cache2_.resize(n_floors_);
        for (int i = 0; i < n_floors_; ++i) {
            cache1_[i] = -1;
            cache2_[i] = -1;
        }
    }

    /**
    here we are on the ground floor with 2 light bulbs.
    we need to test all floors above highest_good.
    **/
    int cost2LightBulbs(
        int highest_good
    ) noexcept {
        /**
        check the cache.
        **/
        if (use_cache_) {
            int answer = cache2_[highest_good];
            if (answer >= 0) {
                return answer;
            }
        }

        int first_to_test = highest_good + 1;
        int floors_to_test = n_floors_ - first_to_test;

        /**
        special case: we're done.
        no floors remain to be tested.
        no more stairs remain to be climbed.
        **/
        if (floors_to_test <= 0) {
            /*LOG("("<<highest_good<<") floors_to_test="<<floors_to_test<<" answer=0");*/
            return 0;
        }

        /**
        special case: only one floor remains to test.
        **/
        if (floors_to_test == 1) {
            /*LOG("("<<highest_good<<") floors_to_test="<<floors_to_test<<" answer="<<first_to_test);*/
            return first_to_test;
        }

        /**
        special case: two floors remain to test.
        worst case we have to climb up to first_floor_to_test.
        bulb doesn't break.
        climb up one more floor.
        **/
        if (floors_to_test == 2) {
            /*LOG("("<<highest_good<<") floors_to_test="<<floors_to_test<<" answer="<<first_to_test+1);*/
            return first_to_test + 1;
        }

        /**
        we need to record the floors from which we dropped bulbs.
        **/
        int sz = floors_.size();
        floors_.push_back(-1);
        auto best_floors = floors_;

        /**
        we have 2 light bulbs.
        we know we have at least 3 floors to test.
        try dropping the first bulb from most floors.
        we start with first+1.
        we don't have to test first,
        because if the bulb breaks on first+1...
        then we can test the only remaining floor (first) for free.
        we also don't have to test the last floor.
        because if the bulb does not break on last-1...
        we can test last with the second bulb.
        **/
        int start_test = first_to_test + 1;
        int end_test = n_floors_ - 2;
        int best_answer = 0x7FFFFFFF;
        for (int i = start_test; i <= end_test; ++i) {
            /**
            drop the first bulb from floor i.
            case 1:
            if it breaks then we climb down (for free) to
            the first floor and drop the second bulb.
            if that breaks, then we're done at no additional cost.
            so we assume it doesn't break.
            then we climb down to the ground (for free).
            and we can now use the single bulb case.
            **/
            int cost_broke = i + cost1LightBulb(first_to_test+1, i);
            /**
            optimization: calculating this cost is cheap.
            the next cost is expensive.
            if this cost is worse than the best then we can skip it.
            this cost only goes up as i increases.
            so we can skip all the remaining floors.
            **/
            if (cost_broke >= best_answer) {
                break;
            }
            /**
            case 2:
            if it didn't break, then we climb up some of the stairs.
            and drop the second bulb.
            **/
            int cost_good = cost1of2LightBulbs(i);
            /**
            the test answer is the max of the two cases.
            **/
            int test_answer = std::max(cost_broke, cost_good);
            /**
            save the best answer so far.
            **/
            if (best_answer > test_answer) {
                best_answer = test_answer;
                if (verbose_) {
                    LOG("("<<highest_good<<") floors_to_test="<<floors_to_test
                        <<" i="<<i<<" cost_broke="<<cost_broke<<" cost_good="<<cost_good
                        <<" answer="<<best_answer);
                }

                /**
                update the local list of best floors.
                **/
                best_floors = floors_;
                best_floors[sz] = i;
            }
            /**
            truncate the global list of floors.
            **/
            floors_.resize(sz+1);
        }

        /**
        update the floors list.
        **/
        floors_ = std::move(best_floors);

        /**
        update the cache.
        **/
        cache2_[highest_good] = best_answer;

        return best_answer;
    }

    /**
    we are on floor highest_good with one light bulb.
    the other is intact on the ground.
    we have not yet paid the cost of climbing to the current floor.
    which we will combine with the cost of climbing to the drop floor.
    **/
    int cost1of2LightBulbs(
        int highest_good
    ) noexcept {
        /**
        check the cache.
        **/
        if (use_cache_) {
            int answer = cache1_[highest_good];
            if (answer >= 0) {
                return answer;
            }
        }

        /**
        we need to record the floors from which we dropped bulbs.
        **/
        int sz = floors_.size();
        floors_.push_back(-1);
        auto best_floors = floors_;

        /**
        try dropping the bulb from each floor.
        **/
        int first_to_test = highest_good + 1;
        int best_answer = 0x7FFFFFFF;
        for (int i = first_to_test; i < n_floors_; ++i) {
            /**
            drop the second bulb from floor i.
            case 1:
            if it breaks then climb down to the ground floor (for free).
            and we can use the single bulb case.
            **/
            int cost_broke = i + cost1LightBulb(first_to_test, i);
            /**
            optimization: calculating this cost is cheap.
            the next cost is expensive.
            if this cost is worse than the best then we can skip it.
            this cost only goes up as i increases.
            so we can skip all the remaining floors.
            **/
            if (cost_broke >= best_answer) {
                break;
            }
            /**
            case 2:
            if it didn't break, we climb down to ground floor (for free).
            and we can use the 2 bulb case.
            **/
            int cost_good = i + cost2LightBulbs(i);
            /**
            the test answer is the max of the two cases.
            **/
            int test_answer = std::max(cost_broke, cost_good);
            /**
            save the best answer so far.
            **/
            if (best_answer > test_answer) {
                best_answer = test_answer;
                if (verbose_) {
                    LOG("("<<highest_good<<") "
                        <<" i="<<i<<" cost_broke="<<cost_broke<<" cost_good="<<cost_good
                        <<" answer="<<best_answer);
                }

                /**
                update the local list of best floors.
                **/
                best_floors = floors_;
                best_floors[sz] = i;
            }
            /**
            truncate the global list of floors.
            **/
            floors_.resize(sz+1);
        }

        /**
        update the floors list.
        **/
        floors_ = std::move(best_floors);

        /**
        update the cache.
        **/
        cache1_[highest_good] = best_answer;

        return best_answer;
    }

    /**
    we broke a bulb.
    we have only one left.
    we must test all floors from lowest to highest.
    climbing down after each test.
    **/
    int cost1LightBulb(
        int first_to_test,
        int lowest_bad
    ) noexcept {
        /**
        our cost is testing all floors
        minus the cost of testing good floors.
        **/
        int all_floors = cost1LightBulb(lowest_bad);
        int good_floors = cost1LightBulb(first_to_test);
        int answer = all_floors - good_floors;
        return answer;
    }

    /**
    test all floors from the ground floor to highest.
    sum all digits from 0 to <N.
    this has a well-known numerical solution.
    **/
    int cost1LightBulb(
        int n_floors
    ) noexcept {
        /** special case: **/
        if (n_floors <= 1) {
            return 0;
        }
        int answer = n_floors * (n_floors - 1) / 2;
        return answer;
    }
};
}

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    LightBulb lb;
    lb.run();

    return 0;
}
