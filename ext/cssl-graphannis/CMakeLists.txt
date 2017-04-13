project(CSSL)

cmake_minimum_required(VERSION 3.2)


if (MSVC OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_definitions(/arch:AVX)
else()
	add_definitions(-mavx)
endif()

add_library(cssl "skiplist.c")
set_property(TARGET cssl PROPERTY C_STANDARD 99)

add_executable(cssl_demo "skiplist.c" "cssl.c")
set_property(TARGET cssl_demo PROPERTY C_STANDARD 99)
target_link_libraries(cssl_demo m)

install(TARGETS cssl cssl_demo
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
       INCLUDES DESTINATION include
)
install(FILES skiplist.h DESTINATION include/cssl)

