/*
Copyright (C) 2012-2025 tim cotter. All rights reserved.
*/

/**
solve sudokus

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

row is 1x9 cells.
column is 9x1 cells.
box is 3x3 cells.
a region is a row, column, or box.
cells must contain 0 or a unique digit.

**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <iomanip>
#include <sstream>
#include <vector>


namespace {

static const int kAllValid = 0b1111111110;

class Region {
public:
    std::string name_;
    int cells_[9];
};
typedef std::vector<Region> Regions;

class Cell {
public:
    int value_;
    int known_;
    int valid_;
};

class Sudoku {
public:
    Cell board_[81];
    Regions regions_;

    void run() noexcept {
        init();
        std::cout<<"Givens:"<<std::endl;
        print_board();

        /** takes 10^45 years. **/
        //solve_brute_force();

        /** find the gimmes. then brute force. **/
        solve_gimmes();
    }

    void init() noexcept {
        init_board_row(0, {0, 0, 0,  0, 0, 0,  0, 6, 0});
        init_board_row(1, {2, 8, 0,  0, 0, 0,  0, 0, 4});
        init_board_row(2, {0, 0, 7,  0, 0, 5,  8, 0, 0});
        init_board_row(3, {5, 0, 0,  3, 4, 0,  0, 2, 0});
        init_board_row(4, {4, 0, 0,  5, 0, 1,  0, 0, 8});
        init_board_row(5, {0, 1, 0,  0, 7, 6,  0, 0, 3});
        init_board_row(6, {0, 0, 5,  1, 0, 0,  2, 0, 0});
        init_board_row(7, {3, 0, 0,  0, 0, 0,  0, 8, 1});
        init_board_row(8, {0, 9, 0,  0, 0, 0,  0, 0, 0});
        copy_board_to_known();
        init_regions();
        check_regions_exit();
    }

    void init_board_row(
        int row,
        std::vector<int> values
    ) noexcept {
        for (int col = 0; col < 9; ++col) {
            int val = values[col];
            if (val < 0 || val > 9) {
                LOG("Error: row "<<row<<" column "<<col<<" initialized to: "<<val);
                exit(1);
            }
            set_board(row, col, val);
        }
    }

    void set_board(
        int row,
        int col,
        int val
    ) noexcept {
        int idx = 9*row + col;
        board_[idx].value_ = val;
    }

    int get_board(
        int row,
        int col
    ) noexcept {
        int idx = 9*row + col;
        int val = board_[idx].value_;
        return val;
    }

    void copy_board_to_known() noexcept {
        for (int i = 0; i < 9*9; ++i) {
            board_[i].known_ = board_[i].value_;
        }
    }

    void init_regions() noexcept {
        regions_.resize(0);
        init_rows();
        init_columns();
        init_boxes();
    }

    void init_rows() noexcept {
        for (int row = 0; row < 9; ++row) {
            init_row(row);
        }
    }

    void init_row(
        int row
    ) noexcept {
        Region rgn;

        std::stringstream ss;
        ss<<"Row "<<row;
        rgn.name_ = std::move(ss.str());

        for (int col = 0; col < 9; ++col) {
            int idx = 9*row + col;
            rgn.cells_[col] = idx;
        }
        regions_.push_back(rgn);
    }

    void init_columns() noexcept {
        for (int col = 0; col < 9; ++col) {
            init_column(col);
        }
    }

    void init_column(
        int col
    ) noexcept {
        Region rgn;

        std::stringstream ss;
        ss<<"Column "<<col;
        rgn.name_ = std::move(ss.str());

        for (int row = 0; row < 9; ++row) {
            int idx= 9*row + col;
            rgn.cells_[row] = idx;
        }
        regions_.push_back(rgn);
    }

    void init_boxes() noexcept {
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 3; ++x) {
                init_box(x, y);
            }
        }
    }

    void init_box(
        int x,
        int y
    ) noexcept {
        Region rgn;

        std::stringstream ss;
        ss<<"Box "<<x<<","<<y;
        rgn.name_ = std::move(ss.str());

        int m = 0;
        for (int i = 0; i < 3; ++i) {
            int row = 3*y + i;
            for (int k = 0; k < 3; ++k) {
                int col = 3*x + k;
                int idx = 9*row + col;
                rgn.cells_[m++] = idx;
            }
        }
        regions_.push_back(rgn);
    }

    void check_regions_exit() noexcept {
        bool error = false;
        for (auto rgn : regions_) {
            int used[10];
            for (int i = 0; i < 10; ++i) {
                used[i] = 0;
            }
            for (int i = 0; i < 9; ++i) {
                int idx = rgn.cells_[i];
                int val = board_[idx].value_;
                ++used[val];
            }
            for (int i = 1; i <= 9; ++i) {
                if (used[i] > 1) {
                    LOG("Error: "<<i<<" used more than once in "<<rgn.name_<<".");
                    error = true;
                }
            }
        }
        if (error) {
            exit(1);
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
            int val = get_board(row, col);
            std::cout<<val<<" ";
        }
        std::cout<<std::endl;
    }

    /**
    it's clear this method cannot possibly work.
    there are 81 cells.
    at least 17 of which must be given.
    that leave 64 cells with 9 possible values.
    9^64 = 1e61.
    which is significantly larger than will fit in a 64 bit integer.
    10 billion=1e10 takes 2 minutes.
    brute forcing all possible solutions will take 1e45 years.
    the sun will be a cold dark cinder by then.
    **/
    void solve_brute_force() noexcept {
        init_brute_force();
        std::cout<<"Start Brute Force"<<std::endl;
        print_board();
        agm::int64 print_i = 3;
        for (agm::int64 i = 0; i < 10*1000*1000*1000LL; ++i) {
            bool solved = check_regions();
            if (solved) {
                std::cout<<"Solved: "<<solved<<std::endl;
                break;
            }
            bool done = increment_board();
            if (done) {
                std::cout<<"Unsolvable."<<std::endl;
                break;
            }
            if (i == print_i) {
                print_i *= 3;
                std::cout<<"Iteration: "<<i<<std::endl;
                print_board();
            }
        }
    }

    void init_brute_force() noexcept {
        for (int i = 0; i < 9*9; ++i) {
            int val = board_[i].known_;
            if (val == 0) {
                val = 1;
            }
            board_[i].value_ = val;
        }
    }

    bool check_regions() noexcept {
        for (auto &&rgn : regions_) {
            int used = 0;
            for (int i = 0; i < 9; ++i) {
                int idx = rgn.cells_[i];
                int val = board_[idx].value_;
                int bit = 1 << val;
                if (used & bit) {
                    return false;
                }
                used |= bit;
            }
        }
        return true;
    }

    /**
    increment cells from 1 to 9.
    then reset to 1 and increment the next cell.
    return true when we run out of cells to increment.
    **/
    bool increment_board() noexcept {
        /** find the first cell we can increment. **/
        for (int i = 0; i < 9*9; ++i) {
            /** skip cells with known values. **/
            int known = board_[i].known_;
            if (known != 0) {
                continue;
            }
            /** increment with rollover. **/
            int val = board_[i].value_ + 1;
            if (val > 9) {
                val = 1;
            }
            board_[i].value_ = val;
            /** if we didn't rollover then we're finished. **/
            if (val != 1) {
                return false;
            }
            /** this cell rolled over. go to the next one. **/
        }
        /** out of cells to increment. we're done. **/
        return true;
    }

    void solve_gimmes() noexcept {
        set_valid();
        std::cout<<"Valid values:"<<std::endl;
        print_valid();
    }

    void set_valid() noexcept {
        /** everything is valid. **/
        for (int i = 0; i < 9*9; ++i) {
            board_[i].valid_ = kAllValid;
        }
        /** remove invalid bits. **/
        for (auto &&rgn : regions_) {
            int invalid = 0;
            for (int i = 0; i < 9; ++i) {
                int cell = rgn.cells_[i];
                int known = board_[cell].known_;
                invalid |= 1 << known;
            }
            int valid = kAllValid & ~invalid;
            for (int i = 0; i < 9; ++i) {
                int cell = rgn.cells_[i];
                board_[cell].valid_ &= valid;
            }
        }
        /** restore known cells. **/
        for (int i = 0; i < 9*9; ++i) {
            int known = board_[i].known_;
            if (known > 0) {
                board_[i].valid_ = 1 << known;
            }
        }
    }

    void print_valid() noexcept {
        print_valid_separator();
        print_valid_row(0);
        print_valid_row(1);
        print_valid_row(2);
        print_valid_separator();
        print_valid_row(3);
        print_valid_row(4);
        print_valid_row(5);
        print_valid_separator();
        print_valid_row(6);
        print_valid_row(7);
        print_valid_row(8);
        print_valid_separator();
    }

    void print_valid_separator() noexcept {
        std::cout<<"+-------------------------------+-------------------------------+-------------------------------+"<<std::endl;
    }

    void print_valid_row(
        int row
    ) noexcept {
        for (int col = 0; col <= 9; ++col) {
            if ((col % 3) == 0) {
                std::cout<<"| ";
            }
            if (col == 9) {
                break;
            }
            int idx = 9*row + col;
            int valid = board_[idx].valid_;
            for (int i = 1; i <= 9; ++i) {
                int bit = 1 << i;
                if (valid & bit) {
                    std::cout<<i;
                } else {
                    std::cout<<".";
                }
            }
            std::cout<<" ";
        }
        std::cout<<std::endl;
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
