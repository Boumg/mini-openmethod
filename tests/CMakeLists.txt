add_executable(tests_methode test_methode.cpp)
configurer_executable(tests_methode)
add_test(NAME comportements COMMAND tests_methode)

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
