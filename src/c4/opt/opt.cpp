#include "c4/opt/opt.hpp"
#include "c4/platform.hpp"
#include <stdlib.h>
#include <stdio.h>

#ifndef C4_WIN
#include <alloca.h>
#endif


namespace c4 {
namespace opt {

namespace {
void _arg_val_err(const char* msg1, option::Option const& opt, const char* msg2)
{
    fprintf(stderr, "%s", msg1);
    fwrite(opt.name, (size_t)opt.namelen, 1, stderr);
    fprintf(stderr, "%s\n", msg2);
}
} // anon

option::ArgStatus unknown(option::Option const &option, bool msg)
{
    if (msg)
        _arg_val_err("Unknown option '", option, "'");
    return option::ARG_ILLEGAL;
}

option::ArgStatus none(option::Option const&, bool /*msg*/)
{
    return option::ARG_NONE;
}

option::ArgStatus optional(option::Option const& option, bool /*msg*/)
{
    return (option.arg/* && option.name[option.namelen] != 0*/) ?
        option::ARG_OK : option::ARG_IGNORE;
}

option::ArgStatus required(option::Option const& option, bool msg)
{
    if(option.arg != 0)
        return option::ARG_OK;
    if(msg)
        _arg_val_err("Option '", option, "' requires an argument");
    return option::ARG_ILLEGAL;
}

option::ArgStatus nonempty(option::Option const& option, bool msg)
{
    if(option.arg != 0 && option.arg[0] != 0)
        return option::ARG_OK;
    if(msg)
        _arg_val_err("Option '", option, "' requires a non-empty argument");
    return option::ARG_ILLEGAL;
}

option::ArgStatus integer(option::Option const& option, bool msg)
{
    char* endptr = 0;
    if(option.arg != 0)
        strtol(option.arg, &endptr, 10);
    if(endptr != option.arg && *endptr == 0)
        return option::ARG_OK;
    if(msg)
        _arg_val_err("Option '", option, "' requires an integer argument");
    return option::ARG_ILLEGAL;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

Parser::Parser(Parser && that) : usage(that.usage)
{
    argc = that.argc;
    argv = that.argv;
    alloc = std::move(that.alloc);
    stats = that.stats;
    num_opts = that.num_opts;
    options = that.options;
    buffer = that.buffer;
    parser = that.parser;
    that.options = nullptr;
    that.buffer = nullptr;
}

Parser::~Parser()
{
    if(options)
    {
        _free(options, stats.options_max + stats.buffer_max);
        options = nullptr;
        buffer = nullptr;
    }
}

option::Option *Parser::_allocate(unsigned num)
{
    auto ptr = alloc.allocate(num);
    for(unsigned i = 0; i < num; ++i)
        new (ptr+i) option::Option();
    return ptr;
}

void Parser::_free(option::Option *ptr, unsigned num)
{
    for(unsigned i = 0; i < num; ++i)
    {
        buffer[i].~Option();
    }
    alloc.deallocate(ptr, num);
}

Parser::Parser(option::Descriptor const *usage_, size_t num_usage_entries, int argc_, const char **argv_, c4::Allocator<option::Option> a)
    :
    argc(argc_),
    argv(argv_),
    alloc(a),
    num_opts(num_usage_entries),
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

void Parser::help() const
{
    option::printUsage(fwrite, stdout, usage, /*columns*/80);
}

void Parser::check_mandatory(std::initializer_list<int> mandatory_options) const
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

void Parser::_fix_counts()
{
    #ifdef C4_WIN
    int *counts = (int*) _alloca(num_opts * sizeof(int));
    #else
    int *counts = (int*) alloca(num_opts * sizeof(int));
    #endif
    memset(counts, 0, num_opts * sizeof(int));

    for(int i = 0; i < parser.optionsCount(); ++i)
    {
        auto & opt = buffer[i];
        if(opt.index() < 0 || size_t(opt.index()) >= num_opts)
            continue;
        ++counts[opt.index()];
    }
    for(size_t j = 0; j < num_opts; ++j)
    {
        if(counts[j] == options[j].count())
            continue;
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


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

Parser make_parser(option::Descriptor const *usage, size_t num_usage_entries,
                   int argc, const char **argv,
                   c4::Allocator<option::Option> alloc)
{
    return Parser(usage, num_usage_entries, argc, argv, alloc);
}

Parser make_parser(option::Descriptor const *usage, size_t N,
                   int argc, const char **argv,
                   int help_index,
                   std::initializer_list<int> mandatory_indices,
                   c4::Allocator<option::Option> alloc)
{
    auto p = Parser(usage, N, argc, argv, alloc);
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
