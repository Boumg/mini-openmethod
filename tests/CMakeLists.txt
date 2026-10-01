find_package(doctest CONFIG REQUIRED)
include("${doctest_DIR}/doctest.cmake")

add_executable(tests_mini_openmethod lanceur_tests.cpp
    test_methode.cpp test_indexation.cpp test_traits.cpp test_domaines.cpp test_resolution.cpp)
configurer_executable(tests_mini_openmethod)
target_link_libraries(tests_mini_openmethod PRIVATE doctest::doctest)
if(NOT DEFINED MINI_OPENMETHOD_REFLEXION_ATTENDUE)
    set(MINI_OPENMETHOD_REFLEXION_ATTENDUE ${MINI_OPENMETHOD_REFLEXION_ACTIVE})
endif()
target_compile_definitions(tests_mini_openmethod PRIVATE
    MINI_OPENMETHOD_REFLEXION_ATTENDUE=${MINI_OPENMETHOD_REFLEXION_ATTENDUE})
doctest_discover_tests(tests_mini_openmethod TEST_PREFIX "unitaire/")

# Exerce aussi la bascule AUTO sous GCC, avec les limites constexpr par defaut.
if(MINI_OPENMETHOD_REFLEXION_ACTIVE)
    target_sources(tests_mini_openmethod PRIVATE test_grand_domaine.cpp)
endif()

# Sans lier mini_openmethod : verifie l'autodetection des en-tetes avec leurs dependances Boost.
add_executable(tests_detection test_detection.cpp)
target_include_directories(tests_detection PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(tests_detection PRIVATE Boost::callable_traits Boost::mp11)
target_compile_features(tests_detection PRIVATE cxx_std_${MINI_OPENMETHOD_STANDARD})
set_target_properties(tests_detection PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tests_detection PRIVATE ${MINI_OPENMETHOD_OPTIONS_REFLEXION})
target_compile_definitions(tests_detection PRIVATE
    MINI_OPENMETHOD_REFLEXION_ATTENDUE=${MINI_OPENMETHOD_REFLEXION_ATTENDUE})
add_test(NAME detection_sans_cmake COMMAND tests_detection)

function(ajouter_refus nom motif)
    set(arguments_generateur -G "${CMAKE_GENERATOR}")
    if(CMAKE_GENERATOR_PLATFORM)
        list(APPEND arguments_generateur -A "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
        list(APPEND arguments_generateur -T "${CMAKE_GENERATOR_TOOLSET}")
    endif()
    add_test(NAME refus_${nom}
        COMMAND ${CMAKE_COMMAND}
        -S "${CMAKE_CURRENT_SOURCE_DIR}/refus"
        -B "${CMAKE_CURRENT_BINARY_DIR}/refus_${nom}"
        ${arguments_generateur}
        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
        "-DBoost_DIR=${Boost_DIR}"
        "-DSTANDARD_TEST=${MINI_OPENMETHOD_STANDARD}"
        "-DMINI_OPENMETHOD_REFLEXION=${MINI_OPENMETHOD_REFLEXION_ACTIVE}"
        "-DOPTIONS_REFLEXION=${MINI_OPENMETHOD_OPTIONS_REFLEXION}"
        "-DSOURCE_REFUSEE=${CMAKE_CURRENT_SOURCE_DIR}/refus/${nom}.cpp"
        "-DMOTIF_ATTENDU=${motif}")
endfunction()
ajouter_refus(ambiguite_double "Ambiguite : plusieurs specialisations maximales")
ajouter_refus(ambiguite_simple "Ambiguite : plusieurs specialisations maximales")
ajouter_refus(sans_specialisation "Combinaison sans specialisation applicable")
ajouter_refus(doublon "Ambiguite : plusieurs specialisations maximales")
ajouter_refus(domaine_duplique "Domaine invalide")
ajouter_refus(lambda_generique "Une signature explicite")
ajouter_refus(retour_incorrect "Specialisation invalide")
ajouter_refus(base_non_polymorphe "Les bases doivent etre polymorphes")
ajouter_refus(foncteur_qualifie "Une signature explicite")
ajouter_refus(fonction_variadique "Une signature explicite")
ajouter_refus(preparation_temporaire "La preparation exige un objet persistant")
ajouter_refus(preparation_position "Position de preparation hors de la signature")
