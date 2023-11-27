##################
# Target
get_filename_component(DIRECTORY_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" TARGET_MAIN ${DIRECTORY_NAME})

####################
# Sources & headers
aux_source_directory(. SRC_LIST)
file(GLOB HEADERS_LIST "*.h" "*.hpp")

find_package(Threads REQUIRED)

add_executable(${TARGET_MAIN} ${SRC_LIST} ${HEADERS_LIST})
target_link_libraries(${TARGET_MAIN} PRIVATE Threads::Threads)

add_subdirectory(src)

#----------------------------------------
# Application
#----------------------------------------
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} Threads::Threads thread_safe_queue_lib)


#----------------------------------------
# Tests
#----------------------------------------
enable_testing(true)
add_subdirectory(tests)
add_test(unit_tests tests/thread_safe_queue_tests)