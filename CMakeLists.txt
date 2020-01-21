cmake_minimum_required(VERSION 3.2)
project(c4opt)

include(./ext/c4core/cmake/c4Project.cmake)

c4_declare_project(c4opt
    DESC "Command line option parsing"
    AUTHOR "Joao Paulo Magalhaes <dev@jpmag.me>"
    MAJOR 0 MINOR 0 RELEASE 1)

c4_require_subproject(c4core SUBDIRECTORY ${C4OPT_EXT_DIR}/c4core)

c4_add_library(c4opt
    SOURCE_ROOT ${C4OPT_SRC_DIR}
    SOURCES
        c4/opt/opt.cpp
        c4/opt/opt.hpp
        c4/opt/detail/optionparser.h
    LIBS
        c4core
    INC_DIRS
        $<BUILD_INTERFACE:${C4OPT_SRC_DIR}> $<INSTALL_INTERFACE:include>
)

c4_install_target(c4opt)
c4_install_exports()

c4_add_dev_targets()
