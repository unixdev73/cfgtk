add_library(cfgtk_lexer STATIC lexer.cpp)
install(TARGETS cfgtk_lexer DESTINATION lib)

option(LEXER_TESTS_ENABLED "Enable lexer tests" ON)
if (NOT TESTS_ENABLED)
	message(STATUS "Skipping lexer tests")
	set(LEXER_TESTS_ENABLED 0)
endif()

if (LEXER_TESTS_ENABLED)
	message(STATUS "Enabled lexer tests")
	add_executable(test_lexer test/lexer.cpp)
	# Takes a table file, an expected output string, and a series of tokens,
	# runs the lexer, and returns successfully with 0 if the produced string
	# matches the expected one.
	target_link_libraries(test_lexer PRIVATE cfgtk_lexer)
	add_test(NAME lexer_test_001 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"" --
	)
	add_test(NAME lexer_test_002 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:--source" --source
	)
	add_test(NAME lexer_test_003 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:-s" -s
	)
	add_test(NAME lexer_test_004 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:--source :value" --source value
	)
	add_test(NAME lexer_test_005 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:-s :value" -s value
	)
	add_test(NAME lexer_test_006 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"length-tok:--length" --length
	)
	add_test(NAME lexer_test_007 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"length-tok:-l" -l
	)
	add_test(NAME lexer_test_008 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"length-tok:--length :value" --length value
	)
	add_test(NAME lexer_test_009 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"length-tok:-l :value" -l value
	)
	add_test(NAME lexer_test_010 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"help-tok:--help" --help
	)
	add_test(NAME lexer_test_011 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"help-tok:-h" -h
	)
	add_test(NAME lexer_test_012 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"pin-tok:--pin" --pin
	)
	add_test(NAME lexer_test_013 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"pin-tok:-p" -p
	)
	add_test(NAME lexer_test_014 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		":-- :--" -- -- -- --
	)
	add_test(NAME lexer_test_015 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:--source :--pin" --source -- --pin
	)
	add_test(NAME lexer_test_016 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:-s :--pin" -s -- --pin
	)
	add_test(NAME lexer_test_017 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"length-tok:--length :--source" --length -- --source
	)
	add_test(NAME lexer_test_018 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"length-tok:-l :-s" -l -- -s
	)
	add_test(NAME lexer_test_019 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:--source length-tok:--length" --source --length
	)
	add_test(NAME lexer_test_020 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:-s length-tok:-l" -s -l
	)
	add_test(NAME lexer_test_021 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:--source pin-tok:-p" --source -p
	)
	add_test(NAME lexer_test_022 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:--source pin-tok:--pin" --source --pin
	)
	add_test(NAME lexer_test_023 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:--source :val help-tok:-h pin-tok:-p :--strange :---spd :-o :v"
		--source val -hp --strange ---spd -o -- v
	)
	add_test(NAME lexer_test_024 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"source-tok:--source :val help-tok:-h pin-tok:-p :--strange :---spd :-o :v"
		--source val -hp --strange ---spd -o -- v
	)
	add_test(NAME lexer_test_025 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		":-pls :-slo :- :- source-tok:--source :-ols :-los"
		-pls -slo - - --source -ols -los
	)
	add_test(NAME lexer_test_026 COMMAND test_lexer
		"${TEST_DATA_DIR}/test_lexer_token_table.txt"
		"help-tok:-h pin-tok:-p" -hp
	)
endif()
