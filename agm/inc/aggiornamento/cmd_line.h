/*
Copyright (C) 2016 CastAR, Inc. All rights reserved.
*/

#pragma once

namespace agm {
    class CmdLineOptions
    {
    public:
        // map a long human readable option name to a single character option.
        // the character option must be specified the in options string.
        // agm::CmdLineOptions::LongFormat cmd_line_options[] = {
        //    { "help",  '?' },
        //    { "alice", 'a' },
        //    { "bob",   'b' },
        //    { nullptr, 0 },
        // usage like this: --help --alice required --bob optional
        class LongFormat {
        public:
            const char *long_name_;
            char  option_;
        };

        CmdLineOptions(int argc, char **argv, const char *options,
            const LongFormat *lf = nullptr) noexcept;
        ~CmdLineOptions() noexcept;

        // walk the list of arguments looking for properly formatted arguments.
        // this function is similar to gnu's getopt function.
        // options are single letters.
        // options can be separated like this: -a -b -c -d
        // options can be combined like this: -abcd
        // -- stops option parsing
        // options followed by : require a value
        //   foo -a hello
        //   foo -aworld
        // options followed by :: may have a value
        // parsing stops at an unknown option.
        // parsing stops if a required option is missing.
        // arg_index_ is the index of the first non-option argument.
        bool get() noexcept;

        // reset the internal state so we can parse the command line again.
        void reset() noexcept;

        char option_;
        char *value_;
        int arg_index_;
        bool error_;

    private:
        int argc_;
        char **argv_;
        const char *options_;
        const LongFormat *long_format_;
        char *place_;
    };
}
