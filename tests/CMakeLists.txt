cmake_minimum_required(VERSION 3.1...3.26)

project(
  SPSCData
  VERSION 1.0
  LANGUAGES CXX)

set(SOURCES 
    main.cpp
)

add_executable(Test ${SOURCES})
target_compile_features(Test PRIVATE cxx_std_17)
target_compile_options(Test PRIVATE -fsanitize=thread)
target_link_options(Test PRIVATE -fsanitize=thread)
