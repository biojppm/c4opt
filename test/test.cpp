#include <c4/opt/opt.hpp>
#include <gtest/gtest.h>
#include <set>

C4_BEGIN_HIDDEN_NAMESPACE
bool got_an_error = false;
void error_callback(const char *msg, size_t msg_sz)
{
    printf("got expected error: %.*s", (int)msg_sz, msg);
    got_an_error = true;
}
inline c4::ScopedErrorSettings tmp_err()
{
    got_an_error = false;
    return c4::ScopedErrorSettings(c4::ON_ERROR_CALLBACK, error_callback);
}
C4_END_HIDDEN_NAMESPACE


namespace test1 {

typedef enum {
    UNKNOWN,
    HELP,
    NONE,
    OPTIONAL,
    REQUIRED,
    NONEMPTY,
    NUMERIC,
    _IDX_COUNT
} UsageIndex_e;
static const option::Descriptor usage[] =
{
    {UNKNOWN            , 0, "" , ""        , c4::opt::unknown             , "USAGE: app [options] [<arg> [<more args>]]\n\nOptions:" },
    {HELP               , 0, "h", "help"    , c4::opt::none                , "  -h, --help  \tPrint usage and exit." },
    {NONE               , 0, "e", "none"    , c4::opt::none                , "  -e, --none  \tNo value should be given." },
    {OPTIONAL           , 0, "o", "optional", c4::opt::optional            , "  -o[=<val>], --optional[=<val>]  \tAccepts an optional value." },
    {REQUIRED           , 0, "r", "required", c4::opt::required            , "  -r=<val>, --required=<val>  \tOption must be given with a possibly empty value." },
    {NONEMPTY           , 0, "n", "nonempty", c4::opt::nonempty            , "  -n=<val>, --nonempty=<val>  \tOption must be given with a non empty value." },
    {NUMERIC            , 0, "u", "numeric" , c4::opt::numeric             , "  -u=<val>, --numeric=<val>  \tOption must be given with a numeric value." },
    {0,0,0,0,0,0}
};

struct Args
{
    std::vector<std::string> sbuf;
    std::vector<const char*> cbuf;

    void push(const char* s)
    {
        sbuf.emplace_back(s);
        cbuf.emplace_back(sbuf.back().c_str());
    }

    int argc() { return (int)cbuf.size(); }
    const char ** argv() { return cbuf.data(); }
};

template< size_t N >
void do_arg_test(const option::Descriptor (&usg)[N], std::initializer_list<UsageIndex_e> opts)
{
    Args args;
    std::set<UsageIndex_e> which(opts);

    for(auto w : which)
    {
        args.push(usg[w].shortopt);
        if(usg[w].check_arg == &c4::opt::optional)
        {
            args.push("foo");
            args.push(usg[w].shortopt); // allow none
        }
        else if(usg[w].check_arg == &c4::opt::required)
        {
            args.push(""); // allow empty
        }
        else if(usg[w].check_arg == &c4::opt::nonempty)
        {
            args.push("foo"); // allow empty
        }
        else if(usg[w].check_arg == &c4::opt::numeric)
        {
            args.push("123");
        }
    }

    auto p = c4::opt::make_parser(usg, args.argc(), args.argv());

    for(auto w : which)
    {
        args.push(usg[w].shortopt);
        if(usg[w].check_arg == &c4::opt::optional)
        {
            args.push("foo");
            args.push(usg[w].shortopt); // allow none
        }
        else if(usg[w].check_arg == &c4::opt::required)
        {
            args.push(""); // allow empty
        }
        else if(usg[w].check_arg == &c4::opt::nonempty)
        {
            args.push(""); // allow empty
        }
        else if(usg[w].check_arg == &c4::opt::numeric)
        {
            args.push("123");
        }
    }
}

TEST(test1, no_args)
{
    do_arg_test(usage, {});
}

} // namespace test1
