enable_language(ASM)

begin_task()
set_task_sources(jump.hpp jump.S)
add_task_test(tests tests.cpp)
end_task()
