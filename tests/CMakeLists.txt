include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

# If pkg-config is not installed on the system, then the
# CHECK_INSTALL_DIR variable must be set to the install
# location of Check. For example, on Windows, this may
# be: C:/Program Files/check
# set(CHECK_INSTALL_DIR "C:/Program Files/check")

find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

set(TEST_SOURCES
  check_pool_alloc.c
)

set(RUNTIME_INIT_SOURCES
  runtime_pool_init.c
)

set(RUNTIME_ALLOC_SOURCES
  runtime_pool_alloc.c
)

add_executable(check_pool_alloc ${TEST_SOURCES})
target_link_libraries(check_pool_alloc poolalloc ${CHECK_LIBRARIES})

add_executable(runtime_pool_init ${RUNTIME_INIT_SOURCES})
target_link_libraries(runtime_pool_init poolalloc ${CHECK_LIBRARIES})

add_executable(runtime_pool_alloc ${RUNTIME_ALLOC_SOURCES})
target_link_libraries(runtime_pool_alloc poolalloc ${CHECK_LIBRARIES})