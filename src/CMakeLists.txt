set(src
    c4/opt/opt.hpp
    c4/opt/detail/optionparser.h
)

c4_require_module(c4opt
    c4core
    REMOTE GIT_REPOSITORY https://github.com/biojppm/c4core GIT_TAG master
    )

c4_add_library(C4OPT c4opt
    LIBRARY_TYPE headers
    SOURCES ${src}
    INC_DIRS ${CMAKE_CURRENT_LIST_DIR}
)
