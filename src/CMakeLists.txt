add_subdirectory(${PROJECT_SOURCE_DIR}/extern/fmtlib extern/fmtlib)

option(CLANG_TIDY_FIX "Perform fixes for Clang-Tidy" OFF)
find_program(
    CLANG_TIDY_EXE
    NAMES "clang-tidy"
    DOC "Path to clang-tidy executable"
)

add_library(kipper
    allocator.hh allocator.cpp
    ast.hh ast.cpp
	ast_print.hh ast_print.cpp
    api.cpp
    compiler.hh compiler.cpp
    completion.hh
    context.hh context.cpp
    gc.hh gc.cpp
    handle.hh handle.cpp
    heap.hh heap.cpp
    interpreter.hh interpreter.cpp
    kipper.hh kipper.cpp
    list.hh
	location.hh
    log.hh
	parser.hh parser.cpp
	reference.hh reference.cpp
    runtime.hh runtime.cpp
    scanner.hh scanner.cpp
    space.hh space.cpp
    symbol_table.hh symbol_table.cpp
	token.hh token.cpp
    utils.hh
    value.hh value.cpp
)
target_link_libraries(kipper 
    PRIVATE
        fmt::fmt
)
target_compile_features(kipper PUBLIC cxx_std_17)
set_target_properties(kipper PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if(CLANG_TIDY_EXE)
    message("Found clang-tidy: ${CLANG_TIDY_EXE}")
    set_target_properties(kipper PROPERTIES
        CXX_CLANG_TIDY ${CLANG_TIDY_EXE}
    )
else()
    message("Cannot find executable clang-tidy")
endif()

target_include_directories(kipper
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
