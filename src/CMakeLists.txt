add_compile_options(-Wall -Wextra -Wpedantic -fmodules-ts)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_compile_options(-O3)
else()
	add_compile_options(-O0 -g)
endif()

add_executable(run
	grammar.cpp
	cyk.cpp
	main.cpp
)
