# Controle l'entree reelle du programme ; une regression ne doit pas lancer une mesure immense.
execute_process(COMMAND "${PROGRAMME}" -1
    RESULT_VARIABLE resultat OUTPUT_VARIABLE sortie ERROR_VARIABLE erreur TIMEOUT 5)
if(NOT resultat STREQUAL "1")
    message(FATAL_ERROR "L'argument negatif doit etre refuse immediatement : ${resultat}\n${sortie}\n${erreur}")
endif()
if(NOT erreur MATCHES "Un entier decimal non signe est attendu")
    message(FATAL_ERROR "Refus pour une raison inattendue : ${erreur}")
endif()
