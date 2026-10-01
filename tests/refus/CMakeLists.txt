cmake_minimum_required(VERSION 3.25)
project(verification_refus LANGUAGES CXX)
find_package(Boost 1.92 CONFIG REQUIRED COMPONENTS callable_traits mp11)
if(NOT DEFINED STANDARD_TEST)
    set(STANDARD_TEST 23)
endif()
set(definitions_test -DMINI_OPENMETHOD_REFLEXION=0)
if(MINI_OPENMETHOD_REFLEXION)
    set(definitions_test -DMINI_OPENMETHOD_REFLEXION=1 ${OPTIONS_REFLEXION})
endif()
# Un test negatif doit echouer pour la raison attendue, pas pour une dependance absente.
try_compile(COMPILATION_REUSSIE
    SOURCES "${SOURCE_REFUSEE}"
    NO_CACHE
    CMAKE_FLAGS
        "-DCMAKE_CXX_STANDARD=${STANDARD_TEST}"
        "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
        "-DCMAKE_CXX_EXTENSIONS=OFF"
        "-DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    COMPILE_DEFINITIONS ${definitions_test}
    LINK_LIBRARIES Boost::callable_traits Boost::mp11
    OUTPUT_VARIABLE DIAGNOSTIC)
if(COMPILATION_REUSSIE)
    message(FATAL_ERROR "Le programme incorrect a ete accepte : ${SOURCE_REFUSEE}")
endif()
string(FIND "${DIAGNOSTIC}" "${MOTIF_ATTENDU}" POSITION_MOTIF)
if(POSITION_MOTIF EQUAL -1)
    message(FATAL_ERROR "Refus pour une raison inattendue :\n${DIAGNOSTIC}")
endif()
message(STATUS "Refus confirme : ${MOTIF_ATTENDU}")
