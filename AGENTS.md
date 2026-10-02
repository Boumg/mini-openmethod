# Directives du projet

- Rédiger les réponses, la documentation, les commentaires, les tests et les commits en français.
- Nommer les identifiants en français, sauf les noms imposés par C++, CMake ou les API externes.
- Garder une bibliothèque d'en-têtes C++23, sans macro de déclaration ; utiliser Boost.CallableTraits et Boost.Mp11 via vcpkg pour les traits, avec Boost.OpenMethod facultatif pour les mesures.
- Préserver la différence entre opération externe et registre extensible à l'exécution.
- Tout changement de comportement doit mettre à jour les tests et la documentation correspondants.
- Utiliser doctest pour les tests d'exécution, comme dépendance vcpkg réservée aux tests ; conserver les assertions de compilation et les contrôles des diagnostics.
- Tester les cas négatifs de compilation avec leur diagnostic attendu, jamais le seul code d'échec.
- Exécuter la configuration CMake, la compilation et CTest avant de publier.
- Préférer le serveur MCP GitHub pour les opérations GitHub ; vérifier son authentification.
- Utiliser des commits atomiques, par exemple : `feat: ajouter la résolution du double dispatch`.
- Ne pas activer les greffons dynamiques sans définir leur contrat et leurs limites de validation.
- Pour comparer les appels directs, virtuels et mini-openmethod, consulter d'abord
  `documentation/comparaison_appels_cpp.md` et ses résultats datés. Ne pas relancer
  systématiquement les chronométrages ; les actualiser sur demande ou lorsqu'un
  changement du dispatch, du compilateur ou de la machine justifie une nouvelle mesure.
  Les vérifications fonctionnelles `--verifier` peuvent rester automatiques.
