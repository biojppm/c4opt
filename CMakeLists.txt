cmake_minimum_required(VERSION 3.2)
project(c4opt)

include(./extern/c4core/cmake/c4Project.cmake)

c4_declare_project(C4OPT)

set(C4OPT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(C4OPT_EXT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern)

set(src
    c4/opt/opt.hpp
    c4/opt/detail/optionparser.h
)

c4_require_subproject(c4core SUBDIRECTORY ${C4OPT_EXT_DIR}/c4core)

c4_add_library(c4opt LIBRARY_TYPE headers
    SOURCES ${src} SRC_ROOT ${C4OPT_SRC_DIR}
    INC_DIRS ${CMAKE_CURRENT_LIST_DIR}
)

if(C4OPT_BUILD_TESTS)
    enable_testing() # this must be done here (and not inside the test dir)
                     # so that the test targets are available at the top level
    add_subdirectory(test)
endif()

if(C4OPT_BUILD_BENCHMARKS)
    add_subdirectory(bm)
endif()
