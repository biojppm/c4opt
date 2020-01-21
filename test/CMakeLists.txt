c4_setup_testing(C4OPT ON)

function(c4opt_add_test name)
    c4_add_executable(c4opt-test-${name}
        SOURCES ${ARGN} main.cpp
        INC_DIRS ${CMAKE_CURRENT_LIST_DIR}
        LIBS c4opt gtest c4core
        FOLDER test)
    c4_add_test(c4opt-test-${name} ON)
endfunction(c4opt_add_test)

c4opt_add_test(basic test.cpp)
