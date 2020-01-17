#include <c4/opt/opt.hpp>
#include <gtest/gtest.h>
#include <set>

C4_BEGIN_HIDDEN_NAMESPACE
//bool got_an_error = false;
//void error_callback(const char *msg, size_t msg_sz)
//{
//    printf("got expected error: %.*s", (int)msg_sz, msg);
//    got_an_error = true;
//}
//inline c4::ScopedErrorSettings tmp_err()
//{
//    got_an_error = false;
//    return c4::ScopedErrorSettings(c4::ON_ERROR_CALLBACK, error_callback);
//}
C4_END_HIDDEN_NAMESPACE

struct Args
{
    std::vector<std::string> sbuf;
    std::vector<const char*> cbuf;

    Args(size_t dim=128)
    {
        sbuf.reserve(dim);
        cbuf.reserve(dim);
    }

    Args(std::initializer_list<const char*> il, size_t dim=128) : Args(dim)
    {
        for(auto s : il)
        {
            _push(s);
        }
    }

    int argc() { return (int)cbuf.size(); }
    const char ** argv() { return cbuf.data(); }

    void _push(const char* s)
    {
        sbuf.emplace_back(s);
        cbuf.emplace_back(sbuf.back().c_str());
    }

    void _opt(const char* s)
    {
        sbuf.emplace_back("-");
        sbuf.back() += s;
        cbuf.emplace_back(sbuf.back().c_str());
    }

    template<size_t N>
    void add(const option::Descriptor (&usg)[N], int which, int num_reps=1)
    {
        for(int i = 0; i < num_reps; ++i)
        {
            _opt(usg[which].shortopt);
            if(usg[which].check_arg == &c4::opt::optional)
            {
                _push("foo");
            }
            else if(usg[which].check_arg == &c4::opt::required)
            {
                _push(""); // allow empty
            }
            else if(usg[which].check_arg == &c4::opt::nonempty)
            {
                _push("foo");
            }
            else if(usg[which].check_arg == &c4::opt::numeric)
            {
                _push("123");
            }
        }
    }

};

template<size_t N>
void do_arg_test(const option::Descriptor (&usg)[N], std::initializer_list<int> opts, int num_reps=1)
{
    Args args;
    std::set<int> which(opts);

    for(auto w : which)
    {
        args.add(usg, w, num_reps);
    }

    auto p = c4::opt::make_parser(usg, args.argc(), args.argv());

    for(auto const& d : usg)
    {
        int w = d.index;
        if(which.find(w) == which.end())
        {
            EXPECT_FALSE(p[w]);
            EXPECT_EQ(p[w].count(), 0);
        }
        else
        {
            EXPECT_TRUE(p[w]);
            EXPECT_EQ(p[w].count(), num_reps);
        }
    }
}

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
    {UNKNOWN            , 0, "" , ""        , c4::opt::unknown  , "USAGE: app [options] [<arg> [<more args>]]\n\nOptions:" },
    {HELP               , 0, "h", "help"    , c4::opt::none     , "  -h, --help  \tPrint usage and exit." },
    {NONE               , 0, "e", "none"    , c4::opt::none     , "  -e, --none  \tNo value should be given." },
    {OPTIONAL           , 0, "o", "optional", c4::opt::optional , "  -o[ <val>], --optional[=<val>]  \tAccepts an optional value." },
    {REQUIRED           , 0, "r", "required", c4::opt::required , "  -r <val>, --required=<val>  \tOption must be given with a possibly empty value." },
    {NONEMPTY           , 0, "n", "nonempty", c4::opt::nonempty , "  -n <val>, --nonempty=<val>  \tOption must be given with a non empty value." },
    {NUMERIC            , 0, "u", "numeric" , c4::opt::numeric  , "  -u <val>, --numeric=<val>  \tOption must be given with a numeric value." },
    {0,0,0,0,0,0}
};

Args test_case_args({"-e", "-o", "val", "-r", "req", "-n", "foo", "-u", "123", "arg0", "arg1", "arg2", "arg3"});
auto test_case = c4::opt::make_parser(usage, test_case_args.argc(), test_case_args.argv());

TEST(opt, raw_args)
{
    auto raw_args = test_case.raw_args();
    EXPECT_STREQ(raw_args[0], "-e");
    EXPECT_STREQ(raw_args[1], "-o"); EXPECT_STREQ(raw_args[2], "val");
    EXPECT_STREQ(raw_args[3], "-r"); EXPECT_STREQ(raw_args[4], "req");
    EXPECT_STREQ(raw_args[5], "-n"); EXPECT_STREQ(raw_args[6], "foo");
    EXPECT_STREQ(raw_args[7], "-u"); EXPECT_STREQ(raw_args[8], "123");
    EXPECT_STREQ(raw_args[9], "arg0");
    EXPECT_STREQ(raw_args[10], "arg1");
    EXPECT_STREQ(raw_args[11], "arg2");
    EXPECT_STREQ(raw_args[12], "arg3");
    int count = 0; // start at 1 to skip over the executable name
    for(auto r : raw_args)
    {
        EXPECT_STREQ(raw_args[count], r);
        EXPECT_EQ(raw_args[count], r);
        EXPECT_STREQ(r, test_case_args.cbuf[count]);
        ++count;
    }
}

TEST(opt, opts)
{
    auto opts = test_case.opts();
    EXPECT_FALSE(test_case[HELP]); EXPECT_FALSE(opts[HELP]);
    EXPECT_TRUE(test_case[NONE]); EXPECT_TRUE(opts[NONE]);
    EXPECT_TRUE(test_case[OPTIONAL]); EXPECT_TRUE(opts[OPTIONAL]);
    EXPECT_TRUE(test_case[REQUIRED]); EXPECT_TRUE(opts[REQUIRED]);
    EXPECT_TRUE(test_case[NONEMPTY]); EXPECT_TRUE(opts[NONEMPTY]);
    EXPECT_TRUE(test_case[NUMERIC]); EXPECT_TRUE(opts[NUMERIC]);
    int count = 0;
    for(auto const& o : opts)
    {
        EXPECT_EQ(&opts[count], &o);
        switch(count)
        {
        case UNKNOWN:
        case HELP:
        case _IDX_COUNT:
            break;
        case NONE: EXPECT_EQ(o.arg, nullptr); break;
        case OPTIONAL: EXPECT_STREQ(o.arg, "val"); break;
        case REQUIRED: EXPECT_STREQ(o.arg, "req"); break;
        case NONEMPTY: EXPECT_STREQ(o.arg, "foo"); break;
        case NUMERIC: EXPECT_STREQ(o.arg, "123"); break;
        default:
            printf("unknown option index: %d\n", count);
            GTEST_FAIL();
        }
        ++count;
    }
}

TEST(opt, opts_args)
{
    auto opts_args = test_case.opts_args();
    int count = 0;
    for(auto const& o : opts_args)
    {
        EXPECT_EQ(&opts_args[count], &o);
        switch(count)
        {
        case 0: EXPECT_STREQ(o.name, "e"); EXPECT_EQ(o.arg, nullptr); break;
        case 1: EXPECT_STREQ(o.name, "o"); EXPECT_STREQ(o.arg, "val"); break;
        case 2: EXPECT_STREQ(o.name, "r"); EXPECT_STREQ(o.arg, "req"); break;
        case 3: EXPECT_STREQ(o.name, "n"); EXPECT_STREQ(o.arg, "foo"); break;
        case 4: EXPECT_STREQ(o.name, "u"); EXPECT_STREQ(o.arg, "123"); break;
        default:
            GTEST_FAIL();
        }
        ++count;
    }
}

TEST(opt, posn_args)
{
    auto posn_args = test_case.posn_args();
    int count = 0;
    for(auto const o : posn_args)
    {
        EXPECT_STREQ(o, test_case.parser.nonOption(count));
        EXPECT_EQ(o, test_case.parser.nonOption(count));
        EXPECT_STREQ(o, posn_args[count]);
        EXPECT_EQ(o, posn_args[count]);
        switch(count)
        {
        case 0: EXPECT_STREQ(o, "arg0"); break;
        case 1: EXPECT_STREQ(o, "arg1"); break;
        case 2: EXPECT_STREQ(o, "arg2"); break;
        case 3: EXPECT_STREQ(o, "arg3"); break;
        default:
            GTEST_FAIL();
        }
        ++count;
    }
}

TEST(opt, groups)
{
    Args args({"-r", "r0", "-r", "r1", "-r", "r2", "-r", "r3", "-r", "r4"});
    auto p = c4::opt::make_parser(usage, args.argc(), args.argv());
    EXPECT_EQ(p[REQUIRED].count(), 5);
    int count = 0;
    for(auto const* o = &p[REQUIRED]; o; o = o->next())
    {
        switch(count)
        {
        case 0: EXPECT_STREQ(o->arg, "r0"); break;
        case 1: EXPECT_STREQ(o->arg, "r1"); break;
        case 2: EXPECT_STREQ(o->arg, "r2"); break;
        case 3: EXPECT_STREQ(o->arg, "r3"); break;
        case 4: EXPECT_STREQ(o->arg, "r4"); break;
        default:
            GTEST_FAIL();
        }
        ++count;
    }
}

TEST(opt, no_args)
{
    do_arg_test(usage, {});
}

TEST(opt, help)
{
    do_arg_test(usage, {HELP});
    do_arg_test(usage, {HELP}, 2);
    do_arg_test(usage, {HELP}, 3);
    do_arg_test(usage, {HELP}, 4);
}

TEST(opt, optional)
{
    do_arg_test(usage, {OPTIONAL});
    do_arg_test(usage, {OPTIONAL}, 2);
    do_arg_test(usage, {OPTIONAL}, 3);
    do_arg_test(usage, {OPTIONAL}, 4);
}

TEST(opt, required)
{
    do_arg_test(usage, {REQUIRED});
    do_arg_test(usage, {REQUIRED}, 2);
    do_arg_test(usage, {REQUIRED}, 3);
    do_arg_test(usage, {REQUIRED}, 4);
}

TEST(opt, nonempty)
{
    do_arg_test(usage, {NONEMPTY});
    do_arg_test(usage, {NONEMPTY}, 2);
    do_arg_test(usage, {NONEMPTY}, 3);
    do_arg_test(usage, {NONEMPTY}, 4);
}

TEST(opt, numeric)
{
    do_arg_test(usage, {NUMERIC});
    do_arg_test(usage, {NUMERIC}, 2);
    do_arg_test(usage, {NUMERIC}, 3);
    do_arg_test(usage, {NUMERIC}, 4);
}
