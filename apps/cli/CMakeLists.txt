add_executable(ks
	main.cpp
)
target_compile_features(ks PUBLIC cxx_std_17)
set_target_properties(ks PROPERTIES
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

target_include_directories(ks
    PRIVATE 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_link_libraries(ks PRIVATE kipper)
