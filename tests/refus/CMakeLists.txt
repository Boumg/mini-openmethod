cmake_minimum_required(VERSION 3.25)
project(verification_refus LANGUAGES CXX)
# Un test negatif doit echouer pour la raison attendue, pas pour une dependance absente.
try_compile(COMPILATION_REUSSIE
    SOURCES "${SOURCE_REFUSEE}"
    NO_CACHE
    CMAKE_FLAGS
        "-DCMAKE_CXX_STANDARD=23"
        "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
        "-DCMAKE_CXX_EXTENSIONS=OFF"
        "-DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    OUTPUT_VARIABLE DIAGNOSTIC)
if(COMPILATION_REUSSIE)
    message(FATAL_ERROR "Le programme incorrect a ete accepte : ${SOURCE_REFUSEE}")
endif()
string(FIND "${DIAGNOSTIC}" "${MOTIF_ATTENDU}" POSITION_MOTIF)
if(POSITION_MOTIF EQUAL -1)
    message(FATAL_ERROR "Refus pour une raison inattendue :\n${DIAGNOSTIC}")
endif()
message(STATUS "Refus confirme : ${MOTIF_ATTENDU}")
