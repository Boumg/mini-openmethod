# Construire, tester et intégrer

[Sommaire](README.md)

Sauf indication contraire, les commandes se lancent depuis la racine du projet.
Pour commencer, la procédure générale suffit. Les sections suivantes couvrent
LLVM sous Windows, les options avancées et l'intégration dans un autre projet.

## Prérequis

- CMake 3.25 ou supérieur et un compilateur avec un mode C++23.
- RTTI et exceptions activés.
- vcpkg initialisé ; son chemin doit être fourni à CMake.

Le [manifeste](../vcpkg.json) fixe les dépendances. La chaîne vcpkg installe
Boost.CallableTraits et Boost.Mp11 pendant la configuration. doctest est réservé
aux tests ; Boost.OpenMethod est facultatif et sert aux comparaisons.

## Configurer, compiler et tester

Adapter le chemin vers vcpkg :

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/chemin/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Avec un générateur à configuration unique, comme Ninja ou Make, ajouter
`-DCMAKE_BUILD_TYPE=Release` à la première commande. Les exemples sont produits
dans `build/Release/` avec Visual Studio, et directement dans `build/` avec
Ninja ou Make.

Pour reprendre un dossier configuré avant l'ajout de vcpkg, ajouter `--fresh`
à la configuration afin que CMake charge la nouvelle chaîne.

Les exemples et les tests sont activés par défaut lorsque ce dépôt est le projet
principal. `-DMINI_OPENMETHOD_EXEMPLES=OFF` désactive les exemples ;
`-DMINI_OPENMETHOD_TESTS=OFF` désactive les tests et leur dépendance doctest.

CTest découvre les scénarios doctest, dont les contrôles restent actifs en
Release. Pour ne lancer que les tests d'indexation :

```sh
ctest --test-dir build -C Release -R "unitaire/Indexation" --output-on-failure
```

Les `static_assert` vérifient aussi les règles à la compilation. Les programmes
volontairement incorrects sont compilés séparément : un test négatif réussit
uniquement si le compilateur les refuse avec le **diagnostic attendu**.

### Lancer le premier exemple

Après la compilation, sous PowerShell avec Visual Studio :

```powershell
./build/Release/01_dispatch_simple.exe
```

Avec Ninja ou Make sous Linux :

```sh
./build/01_dispatch_simple
```

Avec Ninja sous Windows, utiliser `./build/01_dispatch_simple.exe`.
Le programme affiche :

```text
ouaf
miaou
```

Le [code de cet exemple](../exemples/01_dispatch_simple.cpp) vérifie aussi les
deux résultats et retourne 1 si l'un d'eux est incorrect.

## Utiliser LLVM sous Windows

Le [préréglage `llvm`](../CMakePresets.json) configure Clang avec Ninja, les exemples
et les tests en Release dans `build-llvm/`. Il nécessite LLVM, Ninja, CMake,
vcpkg, les outils C++ de Visual Studio et le SDK Windows. Les en-têtes et la
bibliothèque standard proviennent de l'environnement Microsoft installé.

Depuis PowerShell :

```powershell
$env:VCPKG_ROOT = 'C:/chemin/vcpkg'
cmake --preset llvm
cmake --build --preset llvm
ctest --preset llvm
```

Pour voir le premier exemple : `./build-llvm/01_dispatch_simple.exe`.

Le préréglage cherche `clang++.exe` dans `%ProgramFiles%\LLVM\bin`, généralement
`C:\Program Files\LLVM\bin`. Si LLVM est installé ailleurs, remplacer la
commande de configuration par :

```powershell
cmake --preset llvm "-DCMAKE_CXX_COMPILER=D:/Outils/LLVM/bin/clang++.exe"
```

Ce préréglage est réservé à Windows. Sur les autres systèmes, utiliser la
procédure générale ci-dessus.

## Choisir C++23 ou la réflexion C++26

La bibliothèque conserve la même API et le même chemin d'appel dans les deux
modes. Seule la construction des tables à la compilation change.

`MINI_OPENMETHOD_REFLEXION` accepte trois valeurs :

| Valeur | Comportement |
| --- | --- |
| `AUTO` (défaut) | Utilise la réflexion si une table de double dispatch compile avec cette chaîne ; sinon utilise C++23 |
| `ON` | Exige la réflexion C++26 et signale une erreur si elle est indisponible |
| `OFF` | Force les modèles C++23, même si le compilateur possède la réflexion |

Ajouter par exemple `-DMINI_OPENMETHOD_REFLEXION=OFF` à la configuration.
Une ancienne configuration conserve sa valeur en cache : passer explicitement
`-DMINI_OPENMETHOD_REFLEXION=AUTO` pour revenir au choix automatique.

La sonde vérifie le compilateur, la bibliothèque `<meta>` et les opérations de
réflexion utilisées. Elle essaie d'abord sans option supplémentaire, puis avec
`-freflection` pour GCC et Clang. La version de CMake utilisée doit connaître
le mode C++26 du compilateur choisi.

Les générateurs [C++23](../include/mini_openmethod/detail/resolution_cpp23.hpp)
et [C++26](../include/mini_openmethod/detail/resolution_cpp26.hpp) produisent
les mêmes indices gagnants. Leur fonctionnement est détaillé dans
l'[architecture](architecture.md). La réflexion ne supprime pas l'identification
des objets par `typeid` à l'exécution et ne rend pas le domaine extensible.

### Sans CMake

Ajouter les en-têtes du projet et de Boost au chemin d'inclusion.
[`configuration.hpp`](../include/mini_openmethod/configuration.hpp) détecte
seulement les capacités **déjà activées** : un en-tête ne peut pas activer les
options du compilateur.

Avec GCC 16.2, `-std=c++26 -freflection` active la réflexion automatiquement ;
`-std=c++23` conserve les modèles. La définition
`MINI_OPENMETHOD_REFLEXION=0` ou `1` force le choix, et
`mini_openmethod::reflexion_active` permet de connaître le mode retenu.

Garder le même réglage dans toutes les unités de compilation. Avec CMake, la
cible exportée transmet le choix, le standard minimum et les options nécessaires
à ses consommateurs.

## Vérifier les deux modes avec GCC

Pour une vérification fonctionnelle seule sur une chaîne GCC installée,
utiliser les commandes CMake et CTest de la procédure générale dans des dossiers
distincts : `build-gcc-off` avec `-DMINI_OPENMETHOD_REFLEXION=OFF`, puis
`build-gcc-on` avec `-DMINI_OPENMETHOD_REFLEXION=ON`. Ce second mode exige une
chaîne compatible avec la réflexion. Laisser les mesures désactivées ; en cas
de réutilisation d'un cache, ajouter `-DMINI_OPENMETHOD_MESURES=OFF` et
`-DMINI_OPENMETHOD_COMPARAISON_BOOST=OFF`.

### Validation dans Docker

Le dépôt fournit un [conteneur GCC 16.2](../outils/Dockerfile.gcc), un
[script PowerShell](../outils/verifier-gcc.ps1) et un
[script Linux](../outils/verifier-gcc.sh). Par défaut, ils exécutent uniquement
les vérifications fonctionnelles. Sous Windows, avec Docker Desktop démarré :

```powershell
./outils/verifier-gcc.ps1
```

Le script construit l'image `mini-openmethod-gcc:16.2`, puis configure, compile,
teste et installe les variantes `OFF`, `ON` et `AUTO`. Les sources sont montées
en lecture seule. Les résultats restent dans `build-gcc/cpp23`,
`build-gcc/cpp26` et `build-gcc/auto`. Le conteneur est supprimé à la fin ;
l'image et les résultats sont conservés. La compilation réutilise les fichiers
déjà construits ; les bancs sont compilés pour conserver les contrôles
`--verifier` dans CTest, sans lancer leurs chronométrages.

Le script et la CI lisent la révision vcpkg dans le manifeste et la transmettent
à la construction de l'image. Les dépendances sont installées
dans `build-gcc/vcpkg_installed` et partagées par les trois configurations.
Boost.OpenMethod n'est pas nécessaire à cette validation.

Les modes `ON` et `AUTO` vérifient également 256 spécialisations sur un domaine
de 16 × 16 types, sans relever les limites `constexpr`. Les tests négatifs
emploient le standard et les options du mode choisi. La CI utilise ce même
script ; GCC 13, Clang 18 et MSVC vérifient le repli automatique vers C++23.

Pour actualiser les mesures, ajouter explicitement `-Mesurer` sous PowerShell,
ou `--mesurer` à l'appel du script Bash dans le conteneur. Ce mode reconfigure
avec `--fresh`, reconstruit avec `--clean-first`, puis mesure le temps de
compilation et les appels de `mesurer_dispatch`. Voir le
[protocole de mesure](performances.md#reproduire-les-mesures).

## Intégrer la bibliothèque

### Préparer le projet consommateur

Créer un dossier pour le programme consommateur et y enregistrer le
[premier exemple du README](../README.md#premier-exemple) dans `main.cpp`.
Il retourne 0 lorsque le résultat est correct, sans afficher de texte.

Le consommateur doit fournir Boost.CallableTraits et Boost.Mp11, en version
1.92 ou supérieure. Avec vcpkg en mode manifeste, les dépendances du sous-projet
ne sont pas installées automatiquement : les déclarer dans le `vcpkg.json`
du consommateur. Voici un manifeste minimal utilisant la même révision que ce dépôt :

```json
{
  "builtin-baseline": "eb2d3a3279fd019cb7733072d86900d0ad2a1aef",
  "dependencies": ["boost-callable-traits", "boost-mp11"]
}
```

### Depuis les sources

Placer le dépôt mini-openmethod dans `externes/mini-openmethod` du consommateur,
puis créer ce `CMakeLists.txt` complet :

```cmake
cmake_minimum_required(VERSION 3.25)
project(mon_programme LANGUAGES CXX)

add_subdirectory(externes/mini-openmethod)
add_executable(mon_programme main.cpp)
target_link_libraries(mon_programme PRIVATE mini_openmethod::mini_openmethod)
```

Depuis la racine du **consommateur**, adapter le chemin vers vcpkg :

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/chemin/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
```

Les exemples et les tests sont désactivés par défaut en sous-projet. Ils restent
pilotables avec `MINI_OPENMETHOD_EXEMPLES` et `MINI_OPENMETHOD_TESTS`. Si les
tests sont activés, ajouter également `doctest` au manifeste du consommateur
et activer les tests avec `enable_testing()` dans son `CMakeLists.txt`.

### Depuis une installation

Après la compilation de mini-openmethod, depuis la racine de son dépôt :

```sh
cmake --install build --config Release --prefix installation
```

Le consommateur conserve `main.cpp` et son manifeste. Son `CMakeLists.txt`
utilise cette fois le paquet installé :

```cmake
cmake_minimum_required(VERSION 3.25)
project(mon_programme LANGUAGES CXX)

find_package(mini_openmethod CONFIG REQUIRED)
add_executable(mon_programme main.cpp)
target_link_libraries(mon_programme PRIVATE mini_openmethod::mini_openmethod)
```

Depuis la racine du **consommateur**, fournir le chemin absolu de l'installation :

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/chemin/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=/chemin/mini-openmethod/installation -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
```

La cible transmet les chemins d'en-têtes, le standard C++ et les dépendances
Boost au programme. Une installation existante de Boost peut aussi être rendue
accessible avec `CMAKE_PREFIX_PATH`, sans employer vcpkg.

Les en-têtes sont installés dans `include` par défaut. Pour choisir un autre
sous-répertoire, passer par exemple `-DCMAKE_INSTALL_INCLUDEDIR=entetes` lors de
la configuration de mini-openmethod ; la cible exportée suit ce choix.
Le paquet d'en-têtes est indépendant de l'architecture du producteur
(`ARCH_INDEPENDENT`). Le compilateur et les dépendances du consommateur doivent
toutefois convenir à sa propre architecture.
La [licence Boost Software License 1.0](../LICENSE.md) est également installée
dans `share/licenses/mini_openmethod` par défaut, sous `CMAKE_INSTALL_DATADIR`.

Pendant la phase `0.x`, les correctifs d'une même version mineure conservent la
compatibilité de l'API. Le paquet utilise `SameMinorVersion` : une demande de
`0.1.0` peut accepter `0.1.1`, mais pas `0.2.0`. Une évolution incompatible doit
être annoncée dans une nouvelle version mineure.

La sélection `AUTO`, `ON` ou `OFF` est refaite avec le compilateur et la
bibliothèque standard du consommateur, indépendamment du mode ayant produit
le paquet. `-DMINI_OPENMETHOD_REFLEXION=OFF` force donc C++23 même si le paquet
a été installé avec la réflexion activée.

Pour consulter les performances, voir les [résultats conservés](performances.md)
et le [protocole de comparaison avec Boost.OpenMethod](comparaison_boost.md).
Les commandes CMake et CTest de la procédure générale ne lancent pas de
chronométrages. Les scripts GCC en lancent uniquement avec l'option de mesure.
