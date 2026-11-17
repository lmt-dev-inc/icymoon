cmake_minimum_required(VERSION 3.30)

project(PackageTest CXX)

find_package(slang REQUIRED)

add_executable(example
    src/example.cpp
)

target_compile_features(example PRIVATE cxx_std_20)

target_link_libraries(example
  PRIVATE
    slang::slang
)
