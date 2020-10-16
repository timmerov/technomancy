/**
synthetic voting data.

results:
    First Past the Post Results: Left
    Head to Head Results: Center
    Head to Head Elimination Results: Center
    Ranked Choice Results: Left
    Reverse Rank Order Results: Center
**/

#include "data.h"

namespace synthetic_1 {

Candidates get_candidates() noexcept {
    Candidates candidates = {
        "n/a",
        "Left",
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
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},

        {2, 1, 3, 0, 0, 0},
        {2, 1, 3, 0, 0, 0},
        {2, 1, 3, 0, 0, 0},
        {2, 1, 3, 0, 0, 0},
        {2, 1, 3, 0, 0, 0},
        {2, 1, 3, 0, 0, 0},
        {2, 1, 3, 0, 0, 0},
        {2, 1, 3, 0, 0, 0},

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