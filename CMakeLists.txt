cmake_minimum_required(VERSION 3.25)
# Les fonctionnalites vcpkg doivent etre choisies avant project().
set(tests_par_defaut OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(tests_par_defaut ON)
endif()
option(MINI_OPENMETHOD_TESTS "Compiler et executer les tests" ${tests_par_defaut})
if(MINI_OPENMETHOD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES tests)
endif()
option(MINI_OPENMETHOD_COMPARAISON_BOOST "Comparer avec Boost.OpenMethod installe par vcpkg" OFF)
if(MINI_OPENMETHOD_COMPARAISON_BOOST)
    list(APPEND VCPKG_MANIFEST_FEATURES comparaison-boost)
endif()
project(mini_openmethod VERSION 0.1.0 LANGUAGES CXX)

add_library(mini_openmethod INTERFACE)
add_library(mini_openmethod::mini_openmethod ALIAS mini_openmethod)
target_compile_features(mini_openmethod INTERFACE cxx_std_23)
target_include_directories(mini_openmethod INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

find_package(Boost 1.92 CONFIG REQUIRED COMPONENTS callable_traits mp11)
target_link_libraries(mini_openmethod INTERFACE Boost::callable_traits Boost::mp11)

include(cmake/choisir_implementation.cmake)
configurer_implementation(mini_openmethod)

option(MINI_OPENMETHOD_EXEMPLES "Compiler les exemples progressifs" ${PROJECT_IS_TOP_LEVEL})
option(MINI_OPENMETHOD_MESURES "Compiler la mesure comparative du dispatch" OFF)

function(configurer_executable cible)
    target_link_libraries(${cible} PRIVATE mini_openmethod::mini_openmethod)
    set_target_properties(${cible} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${cible} PRIVATE /W4 /permissive- /utf-8 /EHsc /GR)
    else()
        target_compile_options(${cible} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

if(MINI_OPENMETHOD_MESURES)
    add_executable(mesurer_dispatch mesures/mesurer_dispatch.cpp)
    configurer_executable(mesurer_dispatch)
    add_executable(mesurer_indexation mesures/mesurer_indexation.cpp mesures/jeux_comparaison.cpp)
    configurer_executable(mesurer_indexation)
endif()

if(MINI_OPENMETHOD_COMPARAISON_BOOST)
    find_package(Boost 1.92 CONFIG REQUIRED COMPONENTS openmethod)
    add_executable(comparer_boost mesures/comparer_boost.cpp mesures/jeux_comparaison.cpp)
    configurer_executable(comparer_boost)
    target_link_libraries(comparer_boost PRIVATE Boost::openmethod)
endif()

if(MINI_OPENMETHOD_MESURES OR MINI_OPENMETHOD_COMPARAISON_BOOST)
    add_executable(comparer_appels mesures/comparer_appels.cpp mesures/objets_appels.cpp)
    configurer_executable(comparer_appels)
endif()

# La fabrique d'objets reste opaque pour eviter de deduire les types dynamiques.
foreach(cible IN ITEMS comparer_boost mesurer_indexation comparer_appels)
    if(TARGET ${cible})
        set_target_properties(${cible} PROPERTIES INTERPROCEDURAL_OPTIMIZATION FALSE)
        if(MSVC)
            target_compile_options(${cible} PRIVATE /GL-)
        else()
            target_compile_options(${cible} PRIVATE -fno-lto)
            target_link_options(${cible} PRIVATE -fno-lto)
        endif()
    endif()
endforeach()

if(MINI_OPENMETHOD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    if(TARGET comparer_boost)
        add_test(NAME equivalence_boost COMMAND comparer_boost --verifier)
    endif()
    if(TARGET comparer_appels)
        add_test(NAME equivalence_appels COMMAND comparer_appels --verifier)
    endif()
endif()
if(MINI_OPENMETHOD_EXEMPLES)
    foreach(etape IN ITEMS 01_dispatch_simple 02_heritage 03_dispatch_double 04_ambiguite_resolue
                           05_arguments_ordinaires 06_objets_modifiables)
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
