cmake_minimum_required(VERSION 3.2)
project(c4opt)

include(./ext/c4core/cmake/c4Project.cmake)

c4_declare_project(c4opt
    DESC "Command line option parsing"
    AUTHOR "Joao Paulo Magalhaes <dev@jpmag.me>"
    MAJOR 0 MINOR 0 RELEASE 1)

set(C4OPT_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(C4OPT_EXT_DIR ${CMAKE_CURRENT_LIST_DIR}/ext)

c4_require_subproject(c4core SUBDIRECTORY ${C4OPT_EXT_DIR}/c4core)

c4_add_library(c4opt
    LIBRARY_TYPE INTERFACE
    SOURCE_ROOT ${C4OPT_SRC_DIR}
    INTERFACE_SOURCES
        c4/opt/opt.hpp
        c4/opt/detail/optionparser.h
    INC_DIRS
        $<BUILD_INTERFACE:${C4OPT_SRC_DIR}> $<INSTALL_INTERFACE:include>
)

c4_install_target(c4opt)
#c4_install_exports()  # cmake does not allow interface libraries to be installed
                       # so there's nothing to export

if(C4OPT_BUILD_TESTS)
    enable_testing() # this must be done here (and not inside the test dir)
                     # so that the test targets are available at the top level
    add_subdirectory(test)
endif()

if(C4OPT_BUILD_BENCHMARKS)
    add_subdirectory(bm)
endif()
