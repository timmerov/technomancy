/**
synthetic voting data.

results:
    First Past the Post Results: Populist
    Head to Head Results: Populist
    Head to Head Elimination Results: Populist
    Ranked Choice Results: Populist
    Reverse Rank Order Results: Center (!)
**/

#include "data.h"

namespace synthetic_3 {

Candidates get_candidates() noexcept {
    Candidates candidates = {
        "n/a",
        "Populist",
        "Center",
        "Right",
    };
    return std::move(candidates);
}

Ballots get_voting_data() noexcept {
    Ballots ballots = {
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},

        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},
        {1, 3, 2, 0, 0, 0},

        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},
        {2, 3, 1, 0, 0, 0},

        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
        {3, 2, 1, 0, 0, 0},
    };
    return std::move(ballots);
}

} // namespace minneapolis_park_1