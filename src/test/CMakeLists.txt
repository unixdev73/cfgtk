option(SYSTEM_COMPONENT_TESTS_ENABLED "Enable system component tests" ON)
if (NOT TESTS_ENABLED)
	message(STATUS "Skipping system component tests")
	set(SYSTEM_COMPONENT_TESTS_ENABLED 0)
endif()

if (SYSTEM_COMPONENT_TESTS_ENABLED)
	message(STATUS "Enabled system component tests")
	add_executable(test_cyk_parser cyk_parser.cpp)
	# Takes a grammar file, a token table file, some input,
	# and checks if the start symbol derives the input sequence
	target_link_libraries(test_cyk_parser PRIVATE cfgtk_parser cfgtk_lexer)
	add_test(NAME cyk_parser_test_001 COMMAND test_cyk_parser
		"${TEST_DATA_DIR}/test_cyk_parser_grammar_001.txt"
		"${TEST_DATA_DIR}/test_cyk_parser_toktbl_001.txt"
		--some-unknown-flag
	)
	set_property(TEST cyk_parser_test_001 PROPERTY WILL_FAIL 1)
	add_test(NAME cyk_parser_test_002 COMMAND test_cyk_parser
		"${TEST_DATA_DIR}/test_cyk_parser_grammar_001.txt"
		"${TEST_DATA_DIR}/test_cyk_parser_toktbl_001.txt"
		--augment
	)
	set_property(TEST cyk_parser_test_002 PROPERTY WILL_FAIL 1)
	add_test(NAME cyk_parser_test_003 COMMAND test_cyk_parser
		"${TEST_DATA_DIR}/test_cyk_parser_grammar_001.txt"
		"${TEST_DATA_DIR}/test_cyk_parser_toktbl_001.txt"
		--augment value
	)
	add_test(NAME cyk_parser_test_004 COMMAND test_cyk_parser
		"${TEST_DATA_DIR}/test_cyk_parser_grammar_001.txt"
		"${TEST_DATA_DIR}/test_cyk_parser_toktbl_001.txt"
		--augment value --augment another --length text
	)
	set_property(TEST cyk_parser_test_004 PROPERTY WILL_FAIL 1)
	add_test(NAME cyk_parser_test_005 COMMAND test_cyk_parser
		"${TEST_DATA_DIR}/test_cyk_parser_grammar_001.txt"
		"${TEST_DATA_DIR}/test_cyk_parser_toktbl_001.txt"
		--augment value --augment another --length 10 --charset ascii
	)
endif()
