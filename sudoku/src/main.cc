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
static const int kUniqueTableSize = kAllValid + 1;

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
    int unique_table_[kUniqueTableSize];

    void run() noexcept {
        init();
        std::cout<<"Givens:"<<std::endl;
        print_board();

        /** find gimmes and exclusions. stop brute force when board is unsolvable. **/
        solve();
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
        init_unique_table();
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

    void init_unique_table() noexcept {
        for (int i = 0; i < kUniqueTableSize; ++i) {
            unique_table_[i] = 0;
        }
        for (int i = 1; i <= 9; ++i) {
            int bit = 1 << i;
            unique_table_[bit] = i;
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

    /**
    smarter solver.
    it's still brute force.
    but it backtracks when the board is unsolvable.
    which should speed things up considerably.

    board values are initialized to known or 0.
    set valid bits from cell values.
    find all gimmes and exclusions. set their cell values. reset valid bits.
    A. set unknown cell to first valid value.
    B. reset valid bits.
    find all gimmes and exclusions. set their cell values. reset valid bits.
    if solved then done.
    if solvable then
        advance to next unknown cell. return to B.
    not solvable.
    set cell to next valid value. return to A.
    set cell to first valid value. go to next unknown cell. return to A.
    **/
    void solve() noexcept {
        /** assume initialized to known or 0. **/
        /** set all gimme and exclusive cells. **/
        find_gimmes_exclusions();

        /** save gimmes and exclusives as known. **/
        copy_board_to_known();

        /** start at the first cell. **/
        int current = 0;
        for(;;) {
/* A */
            /** advance to the next cell with no value. **/
            current = find_next_value_0_cell(current);
            check_board_index(current);

            /** set its value to the first valid value. **/
            set_cell_to_first_valid_value(current);
            print_board();

            bool loop = true;
            while (loop == true) {
                loop = false;
/* B */
                /** set all gimme and exclusive cells. **/
                find_gimmes_exclusions();

                /** is the board solved. **/
                bool solved = check_solved();
                if (solved) {
                    /** celebrate. **/
                    std::cout<<"Solved!"<<std::endl;
                    return;
                }

                /** is the board still solvable. **/
                bool solvable = check_solvable();
                if (solvable == false) {
                    /** not solvable. **/
                    for(;;) {
/* C */
                        bool rolled_over = advance_cell_value(current);
                        if (rolled_over) {
                            /** clear all cell values from current to end. **/
                            clear_values_from(current);

                            /** reset valid values. **/
                            set_valid();

                            /** backtrack **/
                            current = find_previous_unknown_cell(current);
                            check_board_index(current);

                            continue;
                            /* goto C */
                        }

                        std::cout<<"Not solvable. Resetting."<<std::endl;
                        print_board();
                        /* goto B */
                        loop = true;
                        break;
                    }
                    if (loop) {
                        /* goto B */
                        continue;
                    }
                }
            }

            /** solvable. advance to the next unknown cell. **/
            ++current;
            check_board_index(current);
            /* goto A */
        }
    }

    void find_gimmes_exclusions() noexcept {
        int nfound = 0;
        do {
            set_valid();
            nfound = find_gimmes();
            nfound += find_exclusions();
        }
        while (nfound > 0);
        std::cout<<"Set gimmes and exclusions:"<<std::endl;
        print_board();
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
                int value = board_[cell].value_;
                invalid |= 1 << value;
            }
            int valid = kAllValid & ~invalid;
            for (int i = 0; i < 9; ++i) {
                int cell = rgn.cells_[i];
                board_[cell].valid_ &= valid;
            }
        }
        /** restore cells with values. **/
        for (int i = 0; i < 9*9; ++i) {
            int value = board_[i].value_;
            if (value > 0) {
                board_[i].valid_ = 1 << value;
            }
        }
    }

    int find_gimmes() noexcept {
        int nfound = 0;
        for (int i = 0; i < 9*9; ++i) {
            /** skip cells that have a value. **/
            auto& cell = board_[i];
            int value = cell.value_;
            if (value > 0) {
                continue;
            }
            /** skip cells with multiple valid values. **/
            int valid = cell.valid_;
            int unique = unique_table_[valid];
            if (unique == 0) {
                continue;
            }
            ++nfound;
            cell.value_ = unique;
            cell.valid_ = 1 << unique;
            int row = i / 9;
            int col = i % 9;
            std::cout<<"Found gimme: cell["<<row<<","<<col<<"]="<<unique<<std::endl;
        }
        return nfound;
    }

    /**
    look for regions where a value appears in exactly one cell.
    **/
    int find_exclusions() noexcept {
        static const int kCellUnknown = -1;
        static const int kMultipleCells = -2;

        int nfound = 0;

        /** for each region. **/
        for (auto &&rgn : regions_) {
            /** start with every digit at an unknown cell. **/
            int excluded_cells[10];
            for (int i = 0; i < 10; ++i) {
                excluded_cells[i] = kCellUnknown;
            }

            /** for each cell in the region. **/
            for (int i = 0; i < 9; ++i) {
                int cell = rgn.cells_[i];
                /** get the cell's valid bits. **/
                int valid = board_[cell].valid_;
                /** for each value. **/
                for (int k = 1; k <= 9; ++k) {
                    /** skip invalid values of k. **/
                    int bit = 1 << k;
                    if ((valid & bit) == 0) {
                        continue;
                    }
                    /** value k is valid. **/
                    int prev = excluded_cells[k];
                    if (prev == kCellUnknown) {
                        /** first encounter, remember where. **/
                        excluded_cells[k] = cell;
                    } else {
                        /** we have seen this value before. **/
                        excluded_cells[k] = kMultipleCells;
                    }
                }
            }

            /** check for cells unique to the region. **/
            for (int i = 1; i <= 9; ++i) {
                /** skip digits at unknown or multiple locations. **/
                int idx = excluded_cells[i];
                if (idx < 0) {
                    continue;
                }
                /** skip cells with values. **/
                auto& cell = board_[idx];
                int value = board_[idx].value_;
                if (value > 0) {
                    continue;
                }
                /** we found an exclusive cell. **/
                ++nfound;
                cell.value_ = i;
                cell.valid_ = 1 << i;
                int row = idx / 9;
                int col = idx % 9;
                std::cout<<"Found exclusion: cell["<<row<<","<<col<<"]="<<i<<std::endl;
            }
        }

        return nfound;
    }

    int find_next_value_0_cell(
        int start
    ) noexcept {
        int idx = start;
        for (; idx < 9*9; ++idx) {
            int value = board_[idx].value_;
            if (value == 0) {
                break;
            }
        }
        return idx;
    }

    void check_board_index(
        int idx
    ) noexcept {
        if (idx < 0 || idx >= 9*9) {
            LOG("Cell index out of range: "<<idx);
            exit(1);
        }
    }

    void set_cell_to_first_valid_value(
        int idx
    ) noexcept {
        auto& cell = board_[idx];
        int valid = cell.valid_;
        for (int i = 1; i <= 9; ++i) {
            int bit = 1 << i;
            if (valid & bit) {
                int row = idx / 9;
                int col = idx % 9;
                std::cout<<"Guessing cell["<<row<<","<<col<<"]="<<i<<std::endl;
                cell.value_ = i;
                break;
            }
        }
    }

    bool check_solved() noexcept {
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

    bool check_solvable() noexcept {
        for (auto&& rgn : regions_) {
            int possible = 0;
            for (int i = 0; i < 9; ++i) {
                int cell = rgn.cells_[i];
                int valid = board_[cell].valid_;
                possible |= valid;
            }
            possible &= kAllValid;
            if (possible != kAllValid) {
                return false;
            }
        }
        return true;
    }

    /** return true if we roll over. **/
    bool advance_cell_value(
        int idx
    ) noexcept {
        auto& cell = board_[idx];
        int value = cell.value_ + 1;
        int valid = cell.valid_;
        for (int i = value; i <= 9; ++i) {
            int bit = 1 << i;
            if (valid & bit) {
                cell.value_ = i;
                return false;
            }
        }
        return true;
    }

    void clear_values_from(
        int start
    ) noexcept {
        for (int i = start; i < 9*9; ++i) {
            auto& cell = board_[i];
            cell.value_ = cell.known_;
        }
    }

    int find_previous_unknown_cell(
        int start
    ) noexcept {
        for (int i = start - 1; i >= 0; --i) {
            int known = board_[i].known_;
            if (known > 0) {
                return i;
            }
        }
        return -1;
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
