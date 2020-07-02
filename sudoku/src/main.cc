/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
solve sudoku by simulated annealing.

specifically, this sudoku:

+-------+-------+-------+
| . . . | . . . | . 6 . |
| 2 8 . | . . . | . . 4 |
| . . 7 | . . 5 | 8 . . |
+-------+-------+-------+
| 5 . . | 3 4 . | . 2 . |
| 4 . . | 5 . 1 | . . 8 |
| . 1 . | . 7 6 | . . 3 |
+-------+-------+-------+
| . . 5 | 1 . . | 2 . . |
| 3 . . | . . . | . 8 1 |
| . 9 . | . . . | . . . |
+-------+-------+-------+

**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <vector>

namespace {

class Move {
public:
    int row_;
    int col1_;
    int col2_;
};
using Moves = std::vector<Move>;

class Sudoku {
public:
    int board_[9][9];
    Moves moves_;
    std::mt19937_64 rng_;
    std::uniform_real_distribution<double> unif_;

    void run() noexcept {
        init();
        print_board();

        double best_cost = 1.0;
        double old_cost = evaluate();
        static const int kIterations = 1000*1000;
        for (int i = 0; i < kIterations; ++i) {
            int move = choose_move();
            swap_move(move);
            double new_cost = evaluate();
            bool keep_it = false;
            if (new_cost < old_cost) {
                keep_it = true;
            } else {
                double temp = 1.0 - double(i) / double(kIterations);
                double delta = old_cost - new_cost;
                double prob = std::exp(delta/temp);
                //std::cout<<"=TSC= temp="<<temp<<" prob="<<prob<<" costs: "<<old_cost<<" "<<new_cost<<std::endl;
                double r = unif_(rng_);
                if (r < prob) {
                    keep_it = true;
                }
            }
            if (keep_it) {
                old_cost = new_cost;
                if (best_cost > old_cost) {
                    best_cost = new_cost;
                    LOG(i<<": cost: "<<std::fixed<<std::setprecision(3)<<best_cost);
                }
                if (old_cost == 0.0) {
                    break;
                }
            } else {
                swap_move(move);
            }
        }
        print_board();
    }

    void init() noexcept {
        std::srand(std::time(nullptr));
        auto now = std::chrono::high_resolution_clock::now();
        std::uint64_t seed = now.time_since_epoch().count();
        std::uint64_t seed_lo = seed & 0xFFFFFFFF;
        std::uint64_t seed_hi = seed >> 32;
        std::seed_seq ss{seed_lo, seed_hi};
        rng_.seed(ss);
        unif_ = std::uniform_real_distribution<double>(0, 1);

        init_board_moves();

        int nmoves = moves_.size();
        LOG("number of moves: "<<nmoves);
    }

    void init_board_moves() noexcept {
        moves_.clear();
        init_row(0, {0, 0, 0,  0, 0, 0,  0, 6, 0});
        init_row(1, {2, 8, 0,  0, 0, 0,  0, 0, 4});
        init_row(2, {0, 0, 7,  0, 0, 5,  8, 0, 0});
        init_row(3, {5, 0, 0,  3, 4, 0,  0, 2, 0});
        init_row(4, {4, 0, 0,  5, 0, 1,  0, 0, 8});
        init_row(5, {0, 1, 0,  0, 7, 6,  0, 0, 3});
        init_row(6, {0, 0, 5,  1, 0, 0,  2, 0, 0});
        init_row(7, {3, 0, 0,  0, 0, 0,  0, 8, 1});
        init_row(8, {0, 9, 0,  0, 0, 0,  0, 0, 0});
    }

    void init_row(
        int row,
        std::vector<int> vec
    ) noexcept {
        int count[10];
        for (int i = 0; i < 10; ++i) {
            count[i] = 0;
        }
        for (int col = 0; col < 9; ++col) {
            int x = vec[col];
            if (x < 0 || x > 9) {
                LOG("Error: row "<<row<<" column "<<col<<" initialized to: "<<x);
                exit(1);
            }
            board_[row][col] = x;
            ++count[x];
        }
        int unused[9];
        int idx = 0;
        for (int i = 1; i <= 9; ++i) {
            int c = count[i];
            if (c > 1) {
                LOG("Error: row "<<row<<" value "<<i<<" initialized "<<c<<" times.");
                exit(1);
            }
            if (c == 0) {
                unused[idx] = i;
                ++idx;
            }
        }
        idx = 0;
        for (int col = 0; col < 9; ++col) {
            if (board_[row][col] == 0) {
                board_[row][col] = unused[idx];
                ++idx;
            }
        }
        for (int i = 0; i < 9; ++i) {
            if (vec[i] != 0) {
                continue;
            }
            for (int k = i+1; k < 9; ++k) {
                if (vec[k] != 0) {
                    continue;
                }
                moves_.push_back({row, i, k});
            }
        }
    }

    void print_board() noexcept {
        print_separator();
        print_row(0);
        print_row(1);
        print_row(2);
        print_separator();
        print_row(3);
        print_row(4);
        print_row(5);
        print_separator();
        print_row(6);
        print_row(7);
        print_row(8);
        print_separator();
    }

    void print_separator() noexcept {
        std::cout<<"+-------+-------+-------+"<<std::endl;
    }

    void print_row(
        int row
    ) noexcept {
        for (int col = 0; col <= 9; ++col) {
            if ((col % 3) == 0) {
                std::cout<<"| ";
            }
            if (col == 9) {
                break;
            }
            std::cout<<board_[row][col]<<" ";
        }
        std::cout<<std::endl;
    }

    double evaluate() noexcept {
        int cost = 0;
        int count[10];
        count[0] = 0;
        for (int col = 0; col < 9; ++col) {
            for (int i = 1; i <= 9; ++i) {
                count[i] = 0;
            }
            for (int row = 0; row < 9; ++row) {
                int x = board_[row][col];
                ++count[x];
            }
            for (int i = 1; i <= 9; ++i) {
                if (count[i] == 0) {
                    ++cost;
                }
            }
        }
        for (int block_row = 0; block_row < 9; block_row += 3) {
            for (int block_col = 0; block_col < 9; block_col += 3) {
                for (int i = 1; i <= 9; ++i) {
                    count[i] = 0;
                }
                for (int row = 0; row < 3; ++row) {
                    int r = block_row + row;
                    for (int col = 0; col < 3; ++col) {
                        int c = block_col + col;
                        int x = board_[r][c];
                        ++count[x];
                    }
                }
                for (int i = 1; i <= 9; ++i) {
                    if (count[i] == 0) {
                        ++cost;
                    }
                }
            }
        }

        /**
        what's the worst possible cost?
        123|456|789
        123|456|789
        123|456|789
        ---+---+---
        123|456|789
        123|456|789
        123|456|789
        ---+---+---
        123|456|789
        123|456|789
        123|456|789
        every column has cost 8.
        every block has cost 6.
        so 8*9 + 6*9 = 126
        **/
        double scaled = double(cost) / 126.0;
        return scaled;
    }

    int choose_move() noexcept {
        int nmoves = moves_.size();
        for(;;) {
            int x = 1 + std::rand()/((RAND_MAX + 1u)/nmoves);
            if (x < nmoves) {
                return x;
            }
        }
    }

    void swap_move(
        int idx
    ) noexcept {
        auto& move = moves_[idx];
        std::swap(board_[move.row_][move.col1_], board_[move.row_][move.col2_]);
    }
};

}

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    Sudoku s;
    s.run();

    return 0;
}
