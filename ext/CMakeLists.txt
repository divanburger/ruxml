cmake_minimum_required(VERSION 3.17)
project(ruxml)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES ruxml/array.cpp ruxml/memory.cpp ruxml/str.cpp ruxml/parser.cpp)

add_executable(ruxml test.cpp ${SOURCE_FILES})



find_package(Ruby 2.7 REQUIRED)

add_library(ruxml_ext SHARED ruxml/ruxml.cpp ${SOURCE_FILES})
target_include_directories(ruxml_ext PRIVATE ${RUBY_INCLUDE_DIRS})
target_link_libraries(ruxml_ext ${RUBY_LIBRARIES})