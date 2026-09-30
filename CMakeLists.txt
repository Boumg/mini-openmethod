cmake_minimum_required(VERSION 3.25)
project(mini_openmethod VERSION 0.1.0 LANGUAGES CXX)

add_library(mini_openmethod INTERFACE)
add_library(mini_openmethod::mini_openmethod ALIAS mini_openmethod)
target_compile_features(mini_openmethod INTERFACE cxx_std_23)
target_include_directories(mini_openmethod INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

option(MINI_OPENMETHOD_EXEMPLES "Compiler les exemples progressifs" ${PROJECT_IS_TOP_LEVEL})
option(MINI_OPENMETHOD_TESTS "Compiler et executer les tests" ${PROJECT_IS_TOP_LEVEL})

function(configurer_executable cible)
    target_link_libraries(${cible} PRIVATE mini_openmethod::mini_openmethod)
    set_target_properties(${cible} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${cible} PRIVATE /W4 /permissive- /utf-8 /EHsc /GR)
    else()
        target_compile_options(${cible} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

if(MINI_OPENMETHOD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
if(MINI_OPENMETHOD_EXEMPLES)
    foreach(etape IN ITEMS 01_dispatch_simple 02_heritage 03_dispatch_double 04_ambiguite_resolue)
        add_executable(${etape} exemples/${etape}.cpp)
        configurer_executable(${etape})
        if(MINI_OPENMETHOD_TESTS)
            add_test(NAME exemple_${etape} COMMAND ${etape})
        endif()
    endforeach()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
install(TARGETS mini_openmethod EXPORT mini_openmethodCibles)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT mini_openmethodCibles NAMESPACE mini_openmethod::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mini_openmethod)
configure_package_config_file(cmake/mini_openmethodConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/mini_openmethodConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mini_openmethod)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/mini_openmethodConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mini_openmethodConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/mini_openmethodConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mini_openmethod)
