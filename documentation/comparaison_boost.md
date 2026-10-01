# Comparaison avec Boost.OpenMethod et vcpkg

L'indexation optimisée réduit fortement le coût des appels de mini-openmethod.
À 32 types, l'appel ordinaire rejoint l'ordre de grandeur de Boost.OpenMethod ;
une référence préparée réduit encore le coût des deux bibliothèques.
Sur les petits domaines, les résultats dépendent fortement du compilateur.
La réflexion C++26 ne remplace pas l'identification des objets à l'exécution.

## Résultats locaux du 1er octobre 2026

Machine : AMD Ryzen 9 9950X, Windows x64, Boost.OpenMethod 1.92.0 installé
avec vcpkg 2026-07-27, triplet `x64-windows`. Les deux exécutables utilisent
la bibliothèque standard de Visual Studio. Le mode `AUTO` choisit C++23
pour les deux compilateurs installés. Chaque colonne d'une ligne utilise
les mêmes objets, la même séquence et le même calcul.

Temps médians en nanosecondes par appel ; les plus petits sont les meilleurs.
La préparation des références mini-openmethod et des `virtual_ptr` Boost est
exclue du temps. « Avant » conserve la mesure du moteur qui calculait toutes
les égalités RTTI ; les quatre autres colonnes proviennent du banc actualisé.

### MSVC 19.51.36260, Release `/O2 /Ob2`, sans LTO

| Cas | Types par position | Mini avant | Mini optimisé | Mini préparé | Boost référence | Boost préparé |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Simple | 2 | 12,16 | 6,77 | 3,71 | 6,97 | 4,00 |
| Simple | 8 | 36,92 | 3,61 | 6,07 | 11,26 | 5,35 |
| Simple | 32 | 141,26 | 11,99 | 6,02 | 11,56 | 5,86 |
| Double | 2 | 21,93 | 6,66 | 5,24 | 11,36 | 6,09 |
| Double | 8 | 78,40 | 5,89 | 6,43 | 13,58 | 7,36 |
| Héritage virtuel | 2 | 14,44 | 10,25 | 7,73 | 10,28 | 7,81 |

### Clang 23.1.2, Release `-O3`, sans LTO

| Cas | Types par position | Mini avant | Mini optimisé | Mini préparé | Boost référence | Boost préparé |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Simple | 2 | 10,30 | 1,28 | 0,31 | 7,08 | 3,64 |
| Simple | 8 | 35,21 | 2,42 | 0,30 | 11,02 | 5,12 |
| Simple | 32 | 144,27 | 11,10 | 6,59 | 11,58 | 6,05 |
| Double | 2 | 19,51 | 4,15 | 0,46 | 11,31 | 5,83 |
| Double | 8 | 77,76 | 6,50 | 6,73 | 13,49 | 7,22 |
| Héritage virtuel | 2 | 13,33 | 9,06 | 4,25 | 10,35 | 7,30 |

Sorties complètes actuelles : [MSVC](resultats/boost_msvc_optimise.txt) et
[Clang](resultats/boost_clang_optimise.txt). Les sorties antérieures restent
conservées : [MSVC avant](resultats/boost_msvc.txt) et
[Clang avant](resultats/boost_clang.txt).
Une nouvelle exécution après correction des points de revue est conservée pour
[MSVC](resultats/boost_msvc_apres_revue.txt) et
[Clang](resultats/boost_clang_apres_revue.txt). Les tableaux ci-dessus conservent
la première mesure de l'indexation optimisée ; les
[corrections GCC et réflexion](performances.md#corrections-vérifiées-après-la-revue)
ont leurs mesures distinctes.
Un [contrôle MSVC sur dix paires d'exécutions](performances.md#vérification-répétée-des-appels-préparés-sous-msvc)
ne confirme pas les ralentissements de 9 % et 5 % initialement observés sur les
doubles appels préparés : les écarts des médianes sont −0,04 % et +0,42 %.
Lors de ces mesures, les 22 tests passaient avec chacun des deux compilateurs, dont le contrôle
d'équivalence des six scénarios et les nouveaux tests de préparation.
GCC 16.2 validait aussi les 21 tests sans Boost.OpenMethod en C++23, et 22 en réflexion C++26
et en sélection automatique, ainsi qu'un consommateur de chaque installation.
Le test supplémentaire vérifie un domaine 16 × 16 avec 256 spécialisations.

Après migration vers doctest, CTest distingue chaque scénario nommé : la validation
locale compte 32 tests sous MSVC et Clang avec Boost.OpenMethod, 31 sous GCC en C++23
et 32 sous GCC en réflexion ou en sélection automatique. Cette différence de compte
reflète le découpage des scénarios ; elle ne représente pas autant de nouveaux cas couverts.

## Ce qui est mesuré

- Cinq millions d'appels par passage, sept passages par variante, médiane.
  L'ordre des quatre variantes tourne à chaque passage (trois dans le banc antérieur).
- Séquence de 4 096 entrées, graine 42, répartition équilibrée des types et
  de toutes les paires en double dispatch, puis mélange. Quatre objets sont
  créés par type, avec des valeurs déterminées à l'exécution.
- Une spécialisation par type ou par paire de types, même traitement
  arithmétique pour les quatre variantes. Le double dispatch est asymétrique.
- La fabrique d'objets est dans une unité de compilation séparée et la LTO
  est désactivée. Le compilateur des boucles ne peut donc pas déduire les types
  dynamiques à partir des expressions de construction.
- Avant le chronométrage, chaque résultat est comparé à une valeur attendue
  produite indépendamment. Chaque passage contrôle aussi sa somme finale.
- Création des objets, enregistrements, initialisation de Boost et préparation
  des pointeurs et références, ainsi que le premier remplissage de l'index
  haché mini-openmethod, sont hors chronométrage. Aucune allocation n'est demandée
  dans la boucle mesurée ; les impressions sont faites après les passages.

La première variante appelle mini-openmethod depuis `const Animal&`, avec
des lambdas à signature explicite. La deuxième utilise les mêmes lambdas avec
des `reference_preparee` construites par `preparer` avant la boucle.
La troisième utilise l'API Boost sans macros,
avec `virtual_<const Animal&>` et une identification dynamique à chaque appel.
La quatrième emploie une méthode Boost prenant des `virtual_ptr` construits
depuis ces mêmes références avant la boucle : elle représente leur réutilisation.
Voir l'[API principale](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/core_api.html)
et l'[explication des coûts de Boost](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/performance.html).

Chaque scénario possède son propre registre Boost, contenant sa racine et ses
types dérivés. Il utilise les politiques de `default_registry`, sans activer
les vérifications d'exécution supplémentaires. Une politique vide sert
uniquement à donner une identité distincte au registre. Les enregistrements
ont une durée de vie statique.

## Interprétation et limites

À 32 types, l'appel ordinaire mini-openmethod est environ 12 fois plus rapide
qu'avant sous MSVC, et 13 fois sous Clang. Il emploie maintenant un index haché
des adresses RTTI, avec vérification des collisions et repli par égalité des
types. Jusqu'à 8 types, une recherche linéaire avec arrêt anticipé est conservée.
Le [banc d'indexation](performances.md#choix-du-seuil-dindexation) détaille le seuil.

Une référence préparée évite l'identification répétée, mais ajoute des données
à parcourir (pointeur et indice). Elle n'améliore pas systématiquement les petits
domaines : l'appel ordinaire gagne notamment sous MSVC avec 8 types. Les temps
très bas de certains cas Clang reflètent les optimisations possibles du traitement
et de sa boucle ; ils ne constituent pas un coût universel d'appel indirect.
L'absence de LTO n'empêche pas les simplifications dans une unité de compilation.

Le coût supplémentaire de préparation doit être pris en compte si les références
ne sont utilisées qu'une fois. Le banc ne mesure ni cette préparation ni le coût
de construction des tables. L'appel préparé ne supprime pas le `dynamic_cast`
nécessaire aux ajustements d'héritage virtuel.

Les contrats restent différents : mini-openmethod vérifie exhaustivement un
domaine fermé à la compilation et rejette les types inconnus à chaque appel
ordinaire, ou au moment de la préparation. Les références préparées exigent que
l'objet reste vivant à la même adresse, avec le même type dynamique.
Boost construit son registre à l'initialisation et les appels Release utilisés
ici supposent que les classes sont enregistrées. Le test compare les résultats
sur des entrées valides ; il ne prétend pas rendre ces garanties identiques.

Il s'agit de petits traitements, de données réutilisées et de six scénarios,
pas d'une mesure de charge applicative. La fréquence du processeur, les caches,
le compilateur et la disposition des objets peuvent changer les chiffres.
Les mesures antérieures GCC de `performances.md` emploient un autre banc et
ne doivent pas être comparées directement à ces valeurs Windows.
Les chronométrages présentés ici concernent C++23 sous Windows.

## Reproduire sous Windows

Depuis la racine du dépôt, avec les outils C++ de Visual Studio installés :

```powershell
./outils/comparer-boost.ps1
./outils/comparer-boost.ps1 -Compilateur llvm
```

Le script utilise `VCPKG_ROOT` si défini, sinon détecte le vcpkg fourni avec
Visual Studio via `vswhere`. Pour choisir explicitement une installation :

```powershell
./outils/comparer-boost.ps1 -RacineVcpkg C:/chemin/vcpkg -Iterations 5000000 -Graine 42
```

Un vcpkg récent est nécessaire pour les ports et les outils Visual Studio
actuels. La vérification locale utilise celui de Visual Studio 2026.
Clang est recherché dans `%ProgramFiles%/LLVM/bin` et utilise Ninja.

Le manifeste racine `vcpkg.json` fixe le registre à
`eb2d3a3279fd019cb7733072d86900d0ad2a1aef`. Ses dépendances de base sont
`boost-callable-traits` et `boost-mp11`, utilisés par les en-têtes de la bibliothèque.
La fonctionnalité `comparaison-boost` déclare `boost-openmethod` et ses
dépendances transitives sont résolues par vcpkg. L'option CMake
`MINI_OPENMETHOD_COMPARAISON_BOOST=ON` active cette fonctionnalité avant
`project()` et lie `Boost::openmethod` au seul programme `comparer_boost`.

Les paquets sont conservés dans `build-comparaison-vcpkg/vcpkg_installed`.
Les dossiers `build-boost-msvc` et `build-boost-llvm` contiennent chacun
`configuration.log`, `compilation.log`, `tests.log` et `comparaison.txt`.

## Commandes CMake portables

Avec `VCPKG_ROOT` pointant vers une installation récente :

```sh
cmake -S . -B build-boost -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DMINI_OPENMETHOD_COMPARAISON_BOOST=ON
cmake --build build-boost --config Release --parallel 2
ctest --test-dir build-boost -C Release --output-on-failure
./build-boost/comparer_boost 5000000 42
```

Le dernier chemin correspond à un générateur à configuration unique sous
Linux ; Visual Studio place l'exécutable dans `Release/comparer_boost.exe`.
`comparer_boost --verifier` exécute seulement le contrôle des résultats.
La CI utilise ce mode avec GCC 13 et vcpkg ; aucun seuil de temps n'est imposé.
L'[intégration CMake de vcpkg](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration)
décrit les réglages de chaîne et de fonctionnalités utilisés ici.
