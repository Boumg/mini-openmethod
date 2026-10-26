# Journal des modifications

[Présentation](README.md) · [Documentation](documentation/README.md)

## 0.1.0 — 26 octobre 2026

### Fonctionnalités

- Bibliothèque d'en-têtes C++23 : opérations externes sur un domaine fermé,
  avec une ou deux positions dispatchées et des arguments ordinaires supplémentaires.
- Spécialisations par lambdas, objets constants ou modifiables et références préparées.
- Contrôle des combinaisons absentes ou ambiguës à la compilation ; rejet des
  types dynamiques inconnus à l'exécution.
- Génération des tables en C++23 ou, facultativement, par réflexion C++26.
- Documentation progressive, schémas et exemples exécutables, dont l'indexation RTTI.
- Comparaison avec Visitor et exemples d'ajout d'opérations sans modifier les classes.
- Feuille de route des évolutions envisagées, distincte de l'API disponible.

### Corrections avant publication

- Paquet CMake indépendant de l'architecture du producteur ; répertoire des
  en-têtes personnalisable avec `CMAKE_INSTALL_INCLUDEDIR`.
- Diagnostic explicite pour un pointeur de fonction membre fourni sans adaptation.
- Lecture stricte et commune des paramètres des bancs : entiers complets,
  bornes vérifiées, refus des itérations nulles et des arguments supplémentaires.

### Construction et distribution

- Validation GCC sans chronométrage par défaut ; mesures sur demande explicite.
- Révision vcpkg définie dans le manifeste et réutilisée par les scripts et la CI.
- Boost Software License 1.0 fournie avec les sources et le paquet installé.

## Compatibilité pendant la phase 0.x

Les correctifs d'une même version mineure conservent la compatibilité de l'API :
par exemple, `0.1.1` peut remplacer `0.1.0`. Une nouvelle version mineure peut
introduire des ruptures, qui seront décrites dans ses notes de version.
Le paquet CMake applique cette règle avec `SameMinorVersion`.

La compatibilité ne porte pas sur les noms internes de `mini_openmethod::detail`.
Les dépendances et options de compilation restent celles du programme consommateur.
