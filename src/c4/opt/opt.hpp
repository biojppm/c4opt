#ifndef _C4_OPT_HPP_
#define _C4_OPT_HPP_

#include <c4/error.hpp>
#include <c4/allocator.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "c4/opt/detail/optionparser.h"

/** @file opt.hpp command line option parser utilities */

namespace c4 {
namespace opt {

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


// option value checkers

void _arg_val_err(const char* msg1, option::Option const& opt, const char* msg2)
{
    fprintf(stderr, "%s", msg1);
    fwrite(opt.name, opt.namelen, 1, stderr);
    fprintf(stderr, "%s\n", msg2);
}

option::ArgStatus unknown(option::Option const& option, bool msg)
{
    if(msg) _arg_val_err("Unknown option '", option, "'");
    return option::ARG_ILLEGAL;
}

inline option::ArgStatus none(option::Option const&, bool /*msg*/)
{
    return option::ARG_NONE;
}

inline option::ArgStatus optional(option::Option const& option, bool /*msg*/)
{
    if(option.arg/* && option.name[option.namelen] != 0*/)
    {
        return option::ARG_OK;
    }
    else
    {
        return option::ARG_IGNORE;
    }
}

inline option::ArgStatus required(option::Option const& option, bool msg)
{
    if(option.arg != 0) return option::ARG_OK;
    if(msg) _arg_val_err("Option '", option, "' requires an argument");
    return option::ARG_ILLEGAL;
}

inline option::ArgStatus nonempty(option::Option const& option, bool msg)
{
    if(option.arg != 0 && option.arg[0] != 0) return option::ARG_OK;
    if(msg) _arg_val_err("Option '", option, "' requires a non-empty argument");
    return option::ARG_ILLEGAL;
}

inline option::ArgStatus numeric(option::Option const& option, bool msg)
{
    char* endptr = 0;
    if(option.arg != 0) strtol(option.arg, &endptr, 10);
    if(endptr != option.arg && *endptr == 0) return option::ARG_OK;
    if(msg) _arg_val_err("Option '", option, "' requires a numeric argument");
    return option::ARG_ILLEGAL;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template <size_t N, class Alloc=c4::Allocator<option::Option>>
struct Parser
{
    int             argc;
    const char    **argv;

    Alloc           alloc;

    option::Descriptor const (&usage)[N];
    option::Stats   stats;
    option::Option *options; ///< using a raw pointer here to avoid dependency on vector
    option::Option *buffer;  ///< using a raw pointer here to avoid dependency on vector
    option::Parser  parser;

public:

    Parser& operator= (Parser const& that) = delete;
    Parser& operator= (Parser     && that) = delete;

    Parser(Parser const& that) = delete;
    Parser(Parser     && that) : usage(that.usage)
    {
        argc = that.argc;
        argv = that.argv;
        alloc = std::move(that.alloc);
        stats = that.stats;
        options = that.options;
        buffer = that.buffer;
        parser = that.parser;
        that.options = nullptr;
        that.buffer = nullptr;
    }

    ~Parser()
    {
        if(options)
        {
            _free(options, stats.options_max + stats.buffer_max);
            options = nullptr;
            buffer = nullptr;
        }
    }

private:

    option::Option *_allocate(unsigned num)
    {
        auto ptr = alloc.allocate(num);
        for(unsigned i = 0; i < num; ++i)
        {
            new (ptr+i) option::Option();
        }
        return ptr;
    }

    void _free(option::Option *ptr, unsigned num)
    {
        for(unsigned i = 0; i < num; ++i)
        {
            buffer[i].~Option();
        }
        alloc.deallocate(ptr, num);
    }

public:

    Parser(option::Descriptor const (&usage_)[N], int argc_, const char **argv_, Alloc a={})
        :
        argc(argc_),
        argv(argv_),
        alloc(a),
        usage(usage_),
        stats(usage_, argc, argv),
        options(_allocate(stats.options_max + stats.buffer_max)), // allocate a single block for both options and buffer
        buffer(options + stats.options_max),
        parser(usage, argc, argv, options, buffer)//, /*min_abbr_len*/2, /*single_minus_longopt*/true, /*bufmax*/-1);
    {
        _fix_counts();
        if(parser.error())
        {
            help();
            C4_ERROR("parser error");
        }
    }

    void check_mandatory(std::initializer_list<int> mandatory_options) const
    {
        int ok = 1;
        for(int index : mandatory_options)
        {
            if(options[index].count() == 0)
            {
                _arg_val_err("Option '", options[index], "' is mandatory and was not given");
                ok &= 0;
            }
        }
        if( ! ok)
        {
            C4_ERROR("mandatory options were missing");
        }
    }

    void help() const { option::printUsage(fwrite, stdout, usage, /*columns*/80); }

    option::Option const& operator[] (int i) const { C4_CHECK(i < N); return options[i]; }
    const char* operator() (int i) const { C4_CHECK(i < N); C4_CHECK_MSG(options[i].arg, "error in option %d: '%.*s'", i, options[i].namelen, options[i].name); return options[i].arg; }

public:

    struct positional_arg_iterator
    {
        option::Parser const* p;
        int i;
        using value_type = const char*;
        const char* operator* () const { C4_CHECK(i < p->nonOptionsCount()); return p->nonOption(i); }
        positional_arg_iterator& operator++ () { ++i; return *this; }
        positional_arg_iterator& operator-- () { --i; return *this; }
        positional_arg_iterator& operator+= (int d) { i += d; return *this; }
        positional_arg_iterator& operator-= (int d) { i -= d; return *this; }
        friend positional_arg_iterator operator+ (positional_arg_iterator it, int d) { return {it.p, it.i+d}; }
        friend positional_arg_iterator operator- (positional_arg_iterator it, int d) { return {it.p, it.i-d}; }
        friend int operator- (positional_arg_iterator l, positional_arg_iterator r) { return l.i - r.i; }
        bool operator!= (positional_arg_iterator that) { C4_ASSERT(p == that.p); return i != that.i; }
        bool operator== (positional_arg_iterator that) { C4_ASSERT(p == that.p); return i == that.i; }
    };

    struct option_arg_iterator
    {
        option::Option const *o;
        using value_type = option::Option const*;
        option::Option const* operator-> () const { return o; }
        option::Option const& operator* () const { C4_CHECK(o != nullptr); return *o; }
        option_arg_iterator& operator++ () { C4_CHECK(o != nullptr); o = o->next(); return *this; }
        option_arg_iterator& operator-- () { C4_CHECK(o != nullptr); o = o->prev(); return *this; }
        bool operator!= (option_arg_iterator that) { return o != that.o; }
        bool operator== (option_arg_iterator that) { return o == that.o; }
    };

    template <class It>
    struct it_range
    {
        It begin_, end_;
        It begin() const { return begin_; }
        It end() const { return end_; }
        auto operator[] (int i) const -> decltype(*begin_) { C4_ASSERT(i >= 0 && i < end_ - begin_); return *(begin_ + i); }
    };

    using positional_arg_range = it_range<positional_arg_iterator>;
    using option_range = it_range<option::Option*>;
    using raw_arg_range = it_range<const char **>;
    using option_arg_range = it_range<option_arg_iterator>;

public:

    /** iterate through the raw (argc,argv) arguments */
    raw_arg_range raw_args() const
    {
        raw_arg_range r{argv, &argv[0]+argc};
        return r;
    }

    /** iterate through the arguments given to an option */
    option_arg_range opts(int i) const
    {
        C4_CHECK(i >= 0 && i < N);
        return {{options[i]}, {nullptr}};
    }

    /** iterate through the gathered options */
    option_range opts() const
    {
        return {options, options+N};
    }

    /** iterate through the options in order, as given through argv */
    option_range opts_args() const
    {
        return {buffer, buffer+parser.optionsCount()};
    }

    /** iterate through the positional arguments (ie, those without options) */
    positional_arg_range posn_args() const
    {
        return {{&parser, 0}, {&parser, parser.nonOptionsCount()}};
    }

    void _fix_counts()
    {
        int counts[N] = {0};
        for(int i = 0; i < parser.optionsCount(); ++i)
        {
            auto & opt = buffer[i];
            if(opt.index() < 0 || opt.index() >= N) continue;
            ++counts[opt.index()];
        }
        for(size_t j = 0; j < N; ++j)
        {
            if(counts[j] == options[j].count()) continue;
            for(int i = 0; i < parser.optionsCount(); ++i)
            {
                auto & opt = buffer[i];
                if(opt.index() == i && options[j].first() != &opt)
                {
                    options[j].append(&opt);
                }
            }
        }
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template <size_t N, class Alloc=c4::Allocator<option::Option>>
c4::opt::Parser<N, Alloc> make_parser(option::Descriptor const (&usage)[N], int argc, const char **argv, Alloc alloc={})
{
    return Parser<N, Alloc>(usage, argc, argv, alloc);
}


template <size_t N, class Alloc=c4::Allocator<option::Option>
c4::opt::Parser<N, Alloc> make_parser(option::Descriptor const (&usage)[N], int argc, const char **argv, int help_index, std::initializer_list<int> mandatory_indices={}, Alloc alloc={})
{
    auto p = Parser<N, Alloc>(usage, argc, argv, alloc);
    if(p[help_index])
    {
        p.help();
        std::exit(0);
    }
    else
    {
        p.check_mandatory(mandatory_indices);
    }
    return p;
}


} // namespace opt
} // namespace c4


#endif /* _C4_OPT_HPP_ */
