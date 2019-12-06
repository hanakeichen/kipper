# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(
	${PROJECT_SOURCE_DIR}/extern/googletest 
	extern/googletest 
	EXCLUDE_FROM_ALL)

add_executable(ks-api-test 
  unittest.hh unittest.cpp
  value_test.cpp
)
target_link_libraries(ks-api-test PRIVATE kipper gtest gtest_main)
target_compile_features(ks-api-test PUBLIC cxx_std_17)
set_target_properties(ks-api-test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

add_test(NAME ksapitest COMMAND ks-api-test)

add_executable(ksrunkstest
  runkstestmain.cpp
)
target_link_libraries(ksrunkstest PRIVATE kipper gtest gtest_main)
target_compile_features(ksrunkstest PUBLIC cxx_std_17)
set_target_properties(ksrunkstest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)