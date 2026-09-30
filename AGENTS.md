# Directives du projet

- Rédiger les réponses, la documentation, les commentaires, les tests et les commits en français.
- Nommer les identifiants en français, sauf les noms imposés par C++, CMake ou les API externes.
- Garder une bibliothèque d'en-têtes C++23, sans dépendance obligatoire ni macro de déclaration.
- Préserver la différence entre opération externe et registre extensible à l'exécution.
- Tout changement de comportement doit mettre à jour les tests et la documentation correspondants.
- Tester les cas négatifs de compilation avec leur diagnostic attendu, jamais le seul code d'échec.
- Exécuter la configuration CMake, la compilation et CTest avant de publier.
- Préférer le serveur MCP GitHub pour les opérations GitHub ; vérifier son authentification.
- Utiliser des commits atomiques, par exemple : `feat: ajouter la résolution du double dispatch`.
- Ne pas activer les greffons dynamiques sans définir leur contrat et leurs limites de validation.
