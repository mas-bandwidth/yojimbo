cmake_minimum_required(VERSION 3.22)

MESSAGE("**********************************")
MESSAGE("Consume Yojimbo -- START --")
MESSAGE("**********************************")

project(consume-yojimbo CXX)

find_package(yojimbo CONFIG REQUIRED)

add_executable(consume_yojimbo src/main.cpp)

target_link_libraries(consume_yojimbo yojimbo::yojimbo )

MESSAGE("**********************************")
MESSAGE("Consume Yojimbo -- DONE --")
MESSAGE("**********************************")