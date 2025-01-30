set(TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../data/test)
option(TESTS_ENABLED "Enable all tests" ON)

add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(test) # System Component Tests
