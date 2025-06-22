cmake_minimum_required(VERSION 3.5)

project(mniam_player C)

add_executable(mniam_player main.c amcom.c)
set_target_properties(mniam_player PROPERTIES LINK_FLAGS "-mconsole")
target_link_libraries(mniam_player Ws2_32)
