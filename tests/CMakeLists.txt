add_subdirectory(cpptest)

file(GLOB  kstests_files
	${CMAKE_CURRENT_SOURCE_DIR}/kstest/*.ks
)
foreach(ks_tests_file ${kstests_files})
	get_filename_component(ks_testcase ${ks_tests_file} NAME_WE)
	add_test(
		NAME kstest_${ks_testcase}
		COMMAND ksrunkstest ${ks_tests_file})
endforeach()