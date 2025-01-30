add_library(cfgtk_parser STATIC parser.cpp)
install(TARGETS cfgtk_parser DESTINATION lib)

option(PARSER_TESTS_ENABLED "Enable parser tests" ON)
if (NOT TESTS_ENABLED)
	message(STATUS "Skipping parser tests")
	set(PARSER_TESTS_ENABLED 0)
endif()

if (PARSER_TESTS_ENABLED)
	message(STATUS "Enabled parser tests")
	add_executable(test_start_converter test/start_converter.cpp)
	# Takes an expected output grammar file, an input grammar file,
	# and checks if after applying the start rule the two grammars match
	target_link_libraries(test_start_converter PRIVATE cfgtk_parser)
	add_test(NAME conversion_to_cnf_start_test_001 COMMAND test_start_converter
		"${TEST_DATA_DIR}/test_start_converter_expected_001.txt"
		"${TEST_DATA_DIR}/test_start_converter_input_001.txt"
	)
	add_test(NAME conversion_to_cnf_start_test_002 COMMAND test_start_converter
		"${TEST_DATA_DIR}/test_start_converter_expected_002.txt"
		"${TEST_DATA_DIR}/test_start_converter_input_002.txt"
	)
	add_test(NAME conversion_to_cnf_start_test_003 COMMAND test_start_converter
		"${TEST_DATA_DIR}/test_start_converter_expected_003.txt"
		"${TEST_DATA_DIR}/test_start_converter_input_003.txt"
	)
	add_test(NAME conversion_to_cnf_start_test_004 COMMAND test_start_converter
		"${TEST_DATA_DIR}/test_start_converter_expected_004.txt"
		"${TEST_DATA_DIR}/test_start_converter_input_004.txt"
	)

	add_executable(test_term_converter test/term_converter.cpp)
	# Takes an expected output grammar file, an input grammar file,
	# and checks if after applying the term rule the two grammars match
	target_link_libraries(test_term_converter PRIVATE cfgtk_parser)
	add_test(NAME conversion_to_cnf_term_test_001 COMMAND test_term_converter
		"${TEST_DATA_DIR}/test_term_converter_expected_001.txt"
		"${TEST_DATA_DIR}/test_start_converter_expected_004.txt"
	)

	add_executable(test_bin_converter test/bin_converter.cpp)
	# Takes an expected output grammar file, an input grammar file,
	# and checks if after applying the bin rule the two grammars match
	target_link_libraries(test_bin_converter PRIVATE cfgtk_parser)
	add_test(NAME conversion_to_cnf_bin_test_001 COMMAND test_bin_converter
		"${TEST_DATA_DIR}/test_bin_converter_expected_001.txt"
		"${TEST_DATA_DIR}/test_term_converter_expected_001.txt"
	)

	add_executable(test_del_converter test/del_converter.cpp)
	# Takes an expected output grammar file, an input grammar file,
	# and checks if after applying the del rule the two grammars match
	# This test expects that rules only have 1 or 2 symbols
	# on their right hand sides
	target_link_libraries(test_del_converter PRIVATE cfgtk_parser)
	add_test(NAME conversion_to_cnf_del_test_001 COMMAND test_del_converter
		"${TEST_DATA_DIR}/test_del_converter_expected_001.txt"
		"${TEST_DATA_DIR}/test_del_converter_input_001.txt"
	)
	add_test(NAME conversion_to_cnf_del_test_002 COMMAND test_del_converter
		"${TEST_DATA_DIR}/test_del_converter_expected_002.txt"
		"${TEST_DATA_DIR}/test_del_converter_input_002.txt"
	)
	add_test(NAME conversion_to_cnf_del_test_003 COMMAND test_del_converter
		"${TEST_DATA_DIR}/test_del_converter_expected_003.txt"
		"${TEST_DATA_DIR}/test_del_converter_input_003.txt"
	)
	add_test(NAME conversion_to_cnf_del_test_004 COMMAND test_del_converter
		"${TEST_DATA_DIR}/test_del_converter_expected_004.txt"
		"${TEST_DATA_DIR}/test_del_converter_input_004.txt"
	)

	add_executable(test_unit_converter test/unit_converter.cpp)
	# Takes an expected output grammar file, an input grammar file,
	# and checks if after applying the unit rule the two grammars match
	target_link_libraries(test_unit_converter PRIVATE cfgtk_parser)
	add_test(NAME conversion_to_cnf_unit_test_001 COMMAND test_unit_converter
		"${TEST_DATA_DIR}/test_unit_converter_expected_001.txt"
		"${TEST_DATA_DIR}/test_del_converter_expected_004.txt"
	)

	add_executable(test_cnf_converter test/cnf_converter.cpp)
	# Takes an expected output grammar file, an input grammar file,
	# and checks if after converting the input to CNF the two grammars match
	target_link_libraries(test_cnf_converter PRIVATE cfgtk_parser)
	add_test(NAME conversion_to_cnf_test_001 COMMAND test_cnf_converter
		"${TEST_DATA_DIR}/test_cnf_converter_expected_001.txt"
		"${TEST_DATA_DIR}/test_cnf_converter_input_001.txt"
	)
endif()
