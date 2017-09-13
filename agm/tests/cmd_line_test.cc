/*
Copyright (C) 2017 CastAR, Inc. All rights reserved.
*/

/*
unit tests for ip addresses and udp sockets.
*/

#include <cutl/cmd_line.h>
#include <pal/pal.h>

#include <string.h>


#define LOG_GOOD_FAIL(x) \
    if (x) {\
        LOG(#x << " GOOD!"); \
    } else {\
        LOG(#x << " FAIL!"); \
    }

namespace {
    static void log_args(
        const char *argv[]
    ) throw() {
        std::string s;
        while (auto arg = *argv++) {
            s += arg;
            s += " ";
        }
        LOG(s);
    }

    static void all_tests() throw() {
        LOG("Begin cutl::CmdLineOption tests.");

        bool got;

        {
            // program.exe
            const char *argv[] = {"program.exe", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(1, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 1);
        }

        {
            // program.exe -a -b
            const char *argv[] = {"program.exe", "-a", "-b", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(3, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'a');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'b');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 3);
        }

        {
            // program.exe -ab
            const char *argv[] = {"program.exe", "-ab", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(2, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'a');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'b');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 2);
        }

        {
            // program.exe -c foo
            const char *argv[] = {"program.exe", "-c", "foo", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(3, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'c');
            LOG_GOOD_FAIL(clo.value_ != nullptr);
            LOG_GOOD_FAIL(0 == strcmp(clo.value_, "foo" ));
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 3);
        }

        {
            // program.exe -cfoo
            const char *argv[] = {"program.exe", "-cfoo", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(2, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'c');
            LOG_GOOD_FAIL(clo.value_ != nullptr);
            LOG_GOOD_FAIL(0 == strcmp(clo.value_, "foo" ));
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 2);
        }

        {
            // program.exe -c=foo
            const char *argv[] = {"program.exe", "-c=foo", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(2, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'c');
            LOG_GOOD_FAIL(clo.value_ != nullptr);
            LOG_GOOD_FAIL(0 == strcmp(clo.value_, "foo" ));
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 2);
        }

        {
            // program.exe arg1
            const char *argv[] = {"program.exe", "arg1", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(2, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 1);
        }

        {
            // program.exe -a arg1
            const char *argv[] = {"program.exe", "-a", "arg1", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(3, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'a');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 2);
        }

        {
            // program.exe -c foo arg1
            const char *argv[] = {"program.exe", "-c", "foo", "arg1", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(4, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'c');
            LOG_GOOD_FAIL(clo.value_ != nullptr);
            LOG_GOOD_FAIL(0 == strcmp(clo.value_, "foo" ));
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 3);
        }

        {
            // program.exe -a -- -b
            const char *argv[] = {"program.exe", "-a", "--", "-b", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(4, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'a');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 3);
        }

        {
            // program.exe -a -
            const char *argv[] = {"program.exe", "-a", "-", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(3, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'a');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 2);
        }

        {
            // program.exe -d
            const char *argv[] = {"program.exe", "-d", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(2, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'd');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 2);
        }

        {
            // program.exe -d foo
            const char *argv[] = {"program.exe", "-d", "foo", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(3, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'd');
            LOG_GOOD_FAIL(clo.value_ != nullptr);
            LOG_GOOD_FAIL(0 == strcmp(clo.value_, "foo" ));
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 3);
        }

        {
            // program.exe -dfoo
            const char *argv[] = {"program.exe", "-dfoo", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(2, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'd');
            LOG_GOOD_FAIL(clo.value_ != nullptr);
            LOG_GOOD_FAIL(0 == strcmp(clo.value_, "foo" ));
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 2);
        }

        {
            // program.exe -d=foo
            const char *argv[] = {"program.exe", "-d=foo", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(2, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'd');
            LOG_GOOD_FAIL(clo.value_ != nullptr);
            LOG_GOOD_FAIL(0 == strcmp(clo.value_, "foo" ));
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 2);
        }

        {
            // program.exe -d foo arg1
            const char *argv[] = {"program.exe", "-d", "foo", "arg1", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(4, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'd');
            LOG_GOOD_FAIL(clo.value_ != nullptr);
            LOG_GOOD_FAIL(0 == strcmp(clo.value_, "foo" ));
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 3);
        }

        {
            // program.exe -d -a
            const char *argv[] = {"program.exe", "-d", "-a", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(3, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'd');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'a');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 3);
        }

        cutl::CmdLineOptions::LongFormat c_cmd_line_options[] = {
            {"long-a", 'a'},
            {"long-b", 'b'},
            {"long-c", 'c'},
            {"long-d", 'd'},
            {nullptr, 0 }};

        {
            // program.exe --long-a --long-b --long-c foo --long-d
            const char *argv[] = {"program.exe", "--long-a", "--long-b", "--long-c", "foo", "--long-d", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(6, (char **)argv, "abc:d::", c_cmd_line_options);
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'a');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'b');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'c');
            LOG_GOOD_FAIL(clo.value_ != nullptr);
            LOG_GOOD_FAIL(0 == strcmp(clo.value_, "foo" ));
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'd');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 6);
        }

        {
            // program.exe --long-c=foo
            const char *argv[] = {"program.exe", "--long-c=foo", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(2, (char **)argv, "abc:d::", c_cmd_line_options);
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 'c');
            LOG_GOOD_FAIL(clo.value_ != nullptr);
            LOG_GOOD_FAIL(0 == strcmp(clo.value_, "foo" ));
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == false);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            LOG_GOOD_FAIL(clo.arg_index_ == 2);
        }

        {
            // program.exe -c
            const char *argv[] = {"program.exe", "-c", nullptr};
            log_args(argv);
            cutl::CmdLineOptions clo(2, (char **)argv, "abc:d::");
            LOG_GOOD_FAIL(clo.error_ == false);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == true);
            LOG_GOOD_FAIL(clo.option_ == 'c');
            LOG_GOOD_FAIL(clo.value_ == nullptr);
            got = clo.get();
            LOG_GOOD_FAIL(got == false);
            LOG_GOOD_FAIL(clo.error_ == true);
            LOG_GOOD_FAIL(clo.option_ == 0);
            LOG_GOOD_FAIL(clo.value_ == nullptr);
        }

        LOG("Completed cutl::CmdLineOption tests.");
    }
}

void cmd_line_tests() throw() {
    all_tests();
}
