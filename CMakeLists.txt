cmake_minimum_required(VERSION 3.2)
include(./ext/c4core/cmake/c4Project.cmake)
project(c4opt
    DESCRIPTION "Command line option parsing"
    HOMEPAGE_URL "https://github.com/biojppm/c4opt"
    LANGUAGES CXX)
c4_project(VERSION 0.1.1
    AUTHOR "Joao Paulo Magalhaes <dev@jpmag.me>")

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
c4_pack_project()

c4_add_dev_targets()

c4_set_default_pack_properties(TYPE LIBRARY)
include(CPack)
