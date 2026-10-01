cmake_minimum_required(VERSION 3.25)
project(verifier_installation LANGUAGES CXX)
find_package(mini_openmethod CONFIG REQUIRED)
add_executable(verifier_installation ../test_detection.cpp)
target_link_libraries(verifier_installation PRIVATE mini_openmethod::mini_openmethod)
target_compile_definitions(verifier_installation PRIVATE
    MINI_OPENMETHOD_REFLEXION_ATTENDUE=${MINI_OPENMETHOD_REFLEXION_ATTENDUE})
enable_testing()
add_test(NAME installation COMMAND verifier_installation)
