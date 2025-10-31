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

class Region {
public:
    std::string name_;
    int cells[9];
};

typedef std::vector<Region> Regions;

class Sudoku {
public:
    int board_[81];
    Regions regions_;

    void run() noexcept {
        init();
        print_board();
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
        init_regions();
        check_regions();
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
        board_[idx] = val;
    }

    int get_board(
        int row,
        int col
    ) noexcept {
        int idx = 9*row + col;
        int val = board_[idx];
        return val;
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
            rgn.cells[col] = idx;
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
            rgn.cells[row] = idx;
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
                rgn.cells[m++] = idx;
            }
        }
    }

    void check_regions() noexcept {
        bool error = false;
        for (auto rgn : regions_) {
            int used[10];
            for (int i = 0; i < 10; ++i) {
                used[i] = 0;
            }
            for (int i = 0; i < 9; ++i) {
                int idx = rgn.cells[i];
                int val = board_[idx];
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
