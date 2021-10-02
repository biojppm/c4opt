#ifndef _C4_OPT_HPP_
#define _C4_OPT_HPP_

#include <c4/error.hpp>
#include <c4/allocator.hpp>

C4_SUPPRESS_WARNING_GCC_PUSH
C4_SUPPRESS_WARNING_GCC("-Wnon-virtual-dtor")
C4_SUPPRESS_WARNING_GCC("-Wsign-conversion")
#include "c4/opt/detail/optionparser.h"
C4_SUPPRESS_WARNING_GCC_POP

/** @file opt.hpp command line option parser utilities */

namespace c4 {
namespace opt {

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


// option value checkers

namespace detail {
template<const option::CheckArg Checker, const option::CheckArg... MoreCheckers>
struct multichecker
{
    static option::ArgStatus check(option::Option const& option, bool msg)
    {
        option::ArgStatus stat = option::ARG_OK;
        if(Checker(option, msg) == option::ARG_ILLEGAL)
            stat = option::ARG_ILLEGAL;
        if(multichecker<MoreCheckers...>::check(option, msg) == option::ARG_ILLEGAL)
            stat = option::ARG_ILLEGAL;
        return stat;
    }
};

template<const option::CheckArg Checker>
struct multichecker<Checker>
{
    static option::ArgStatus check(option::Option const& option, bool msg)
    {
        return Checker(option, msg);
    }
};
} // namespace detail

option::ArgStatus unknown(option::Option const &option, bool msg);
/** no value is expected */
option::ArgStatus none(option::Option const&, bool msg);
/** option value is optional */
option::ArgStatus optional(option::Option const& option, bool msg);
/** option value is mandatory, may be empty */
option::ArgStatus required(option::Option const& option, bool msg);
/** option value is mandatory, must not be empty */
option::ArgStatus nonempty(option::Option const& option, bool msg);
/** option value is mandatory, must be integer */
option::ArgStatus integer(option::Option const& option, bool msg);

template<const option::CheckArg... Checkers>
option::ArgStatus multicheck(option::Option const& option, bool msg)
{
    auto ch = detail::multichecker<Checkers...>();
    return ch.check(option, msg);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Parser
{
    int             argc;
    const char    **argv;

    c4::Allocator<option::Option> alloc;

    size_t          num_opts;
    option::Descriptor const *usage;
    option::Stats   stats;
    option::Option *options; ///< using a raw pointer here to avoid dependency on vector
    option::Option *buffer;  ///< using a raw pointer here to avoid dependency on vector
    option::Parser  parser;

public:

    Parser& operator= (Parser const& that) = delete;
    Parser& operator= (Parser     && that) = delete;

    Parser(Parser const& that) = delete;
    Parser(Parser     && that);

    ~Parser();

private:

    option::Option *_allocate(unsigned num);
    void _free(option::Option *ptr, unsigned num);

public:

    Parser(option::Descriptor const *usage_, size_t num_usage_entries, int argc_, const char **argv_, c4::Allocator<option::Option> a={});

    void check_mandatory(std::initializer_list<int> mandatory_options) const;
    void help() const;

    option::Option const& operator[] (int i) const { C4_CHECK(size_t(i) < num_opts); return options[i]; }
    const char* operator() (int i) const { C4_CHECK(size_t(i) < num_opts); C4_CHECK_MSG(options[i].arg, "error in option %d: '%.*s'", i, options[i].namelen, options[i].name); return options[i].arg; }

private:

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
    struct iterator_range
    {
        It begin_, end_;
        It begin() const { return begin_; }
        It end() const { return end_; }
        auto operator[] (int i) const -> decltype(*begin_) { C4_ASSERT(i >= 0 && i < end_ - begin_); return *(begin_ + i); }
    };

    using positional_arg_range = iterator_range<positional_arg_iterator>;
    using option_range = iterator_range<option::Option*>;
    using raw_arg_range = iterator_range<const char **>;
    using option_arg_range = iterator_range<option_arg_iterator>;

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
        C4_CHECK(i >= 0 && size_t(i) < num_opts);
        return {{options[i]}, {nullptr}};
    }

    /** iterate through the gathered options */
    option_range opts() const
    {
        return {options, options+num_opts};
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

    void _fix_counts();

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


Parser make_parser(option::Descriptor const *usage, size_t num_usage_entries,
                   int argc, const char **argv,
                   c4::Allocator<option::Option> alloc=c4::Allocator<option::Option>{});

template <size_t N>
Parser make_parser(option::Descriptor const (&usage)[N],
                   int argc, const char **argv,
                   c4::Allocator<option::Option> alloc=c4::Allocator<option::Option>{})
{
    return make_parser(usage, N, argc, argv, alloc);
}


//-----------------------------------------------------------------------------

Parser make_parser(option::Descriptor const *usage, size_t N,
                   int argc, const char **argv,
                   int help_index,
                   std::initializer_list<int> mandatory_indices=std::initializer_list<int>(),
                   c4::Allocator<option::Option> alloc=c4::Allocator<option::Option>{});

template <size_t N>
Parser make_parser(option::Descriptor const (&usage)[N],
                   int argc, const char **argv,
                   int help_index,
                   std::initializer_list<int> mandatory_indices=std::initializer_list<int>(),
                   c4::Allocator<option::Option> alloc=c4::Allocator<option::Option>{})
{
    return make_parser(usage, N, argc, argv, help_index, mandatory_indices, alloc);
}


} // namespace opt
} // namespace c4


#endif /* _C4_OPT_HPP_ */
