begin_task()

task_link_libraries(asio)

# Echo library
add_task_library(echo)

# Tests
add_task_library(tests/tests_support tests_support)
add_task_test_dir(tests)

end_task()
