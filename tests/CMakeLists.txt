find_package(doctest CONFIG REQUIRED)
include("${doctest_DIR}/doctest.cmake")

add_executable(tests_mini_openmethod lanceur_tests.cpp
    test_methode.cpp test_arguments.cpp test_objets_modifiables.cpp
    test_indexation.cpp test_traits.cpp test_domaines.cpp test_resolution.cpp)
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
ajouter_refus(descripteurs_nombre "Un descripteur par argument est requis")
ajouter_refus(descripteur_invalide "Descripteur invalide")
ajouter_refus(sans_polymorphe "Une ou deux positions polymorphes sont requises")
ajouter_refus(trois_polymorphes "Une ou deux positions polymorphes sont requises")
ajouter_refus(ordinaire_type "Specialisation invalide")
ajouter_refus(ordinaire_reference "Specialisation invalide")
ajouter_refus(ordinaire_arite "Specialisation invalide")
ajouter_refus(preparation_ordinaire "Une position ordinaire ne peut pas etre preparee")
ajouter_refus(ordinaire_ambiguite "Ambiguite : plusieurs specialisations maximales")
ajouter_refus(ordinaire_sans_specialisation "Combinaison sans specialisation applicable")
ajouter_refus(modifiable_specialisation_constante "Specialisation invalide")
ajouter_refus(constante_specialisation_modifiable "Specialisation invalide")
ajouter_refus(preparation_constante_modifiable "Objet incompatible avec la racine ou la qualification const")
ajouter_refus(modifiable_preparation_temporaire "La preparation exige un objet persistant")
ajouter_refus(modifiable_ambiguite "Ambiguite : plusieurs specialisations maximales")
ajouter_refus(modifiable_sans_specialisation "Combinaison sans specialisation applicable")
foreach(forme IN ITEMS valeur pointeur rvalue volatile)
    ajouter_refus(polymorphe_${forme} "La signature exige des references lvalue non volatiles")
endforeach()
# Les mauvais appels sont rejetes par les surcharges C++, sans relais de diagnostic supplementaire.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(diagnostic_appel "C2665")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(diagnostic_appel "no matching function for call to object")
else()
    set(diagnostic_appel "no match for call")
endif()
ajouter_refus(appel_constant_modifiable "${diagnostic_appel}")
ajouter_refus(appel_prepare_constant_modifiable "${diagnostic_appel}")
