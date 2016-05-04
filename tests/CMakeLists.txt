enable_testing()
find_package(Check REQUIRED)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  ## @TODO - Check kernel version
  find_package(Subunit REQUIRED)
endif()

include_directories(${CHECK_INCLUDE_DIRS} ${SUBUNIT_INCLUDE_DIRS})
set(LIBS ${LIBS} ${CHECK_LIBRARIES} ${SUBUNIT_LIBRARIES} cutil m)
include_directories(. ../src)
add_executable(test_suites main.c check_cache.c check_hashmap.c check_slab.c check_lru.c)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(ADDITIONAL_LIBS pthread)
else()
  set(ADDITIONAL_LIBS pthread rt)
endif()

target_link_libraries(test_suites ${LIBS} ${ADDITIONAL_LIBS})
add_test(test_suites ${CMAKE_CURRENT_BINARY_DIR}/test_suites)
