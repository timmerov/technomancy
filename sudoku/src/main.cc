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

#include <sstream>
#include <vector>

namespace {

class Sudoku {
public:
    int board_[9][9];

    void init() noexcept {
        init_row(0, {0, 0, 0,  0, 0, 0,  0, 6, 0});
        init_row(1, {2, 8, 0,  0, 0, 0,  0, 0, 4});
        init_row(2, {0, 0, 7,  0, 0, 5,  8, 0, 0});
        init_row(3, {5, 0, 0,  3, 4, 0,  0, 2, 0});
        init_row(4, {4, 0, 0,  5, 0, 1,  0, 0, 8});
        init_row(5, {0, 1, 0,  0, 7, 6,  0, 0, 3});
        init_row(6, {0, 0, 5,  1, 0, 0,  2, 0, 0});
        init_row(7, {3, 0, 0,  0, 0, 0,  0, 8, 1});
        init_row(8, {0, 9, 0,  0, 0, 0,  0, 0, 0});
        print_board();
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

    void run() noexcept {
        LOG("Hello, World!");
        LOG("Goodbye, World!");
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
    s.init();
    s.run();

    return 0;
}
