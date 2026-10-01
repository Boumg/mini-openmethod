# mini-openmethod

Prototype pédagogique C++23 de méthodes externes à une hiérarchie polymorphe.
L'API n'utilise aucune macro : les concepts valident les signatures, les traits
extraient les paramètres des lambdas et les fonctions `consteval` construisent
une table de dispatch conservée en `constexpr`.

**Le dispatch dépend du type dynamique des objets. La résolution des
spécialisations et la détection des ambiguïtés ont lieu à la compilation,
pour un domaine explicitement fermé.**

La bibliothèque utilise les en-têtes Boost.CallableTraits et Boost.Mp11,
installés par vcpkg. Les tests utilisent doctest, installé uniquement lorsque
`MINI_OPENMETHOD_TESTS=ON` ; les utilisateurs de la bibliothèque n'en dépendent pas.
Un banc de comparaison facultatif ajoute Boost.OpenMethod via vcpkg.
Il ne cherche pas à reproduire toutes les possibilités de Boost.OpenMethod.
CMake sélectionne automatiquement deux implémentations : modèles C++23 ou
réflexion C++26 pour construire les tables. L'analyse des signatures par
Boost.CallableTraits est commune aux deux modes.
L'API et le chemin d'appel restent communs ; C++23 suffit toujours pour utiliser
la bibliothèque.

## Premier exemple : single dispatch

```cpp
#include <mini_openmethod/methode.hpp>
#include <string_view>

struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};

int main() {
    using namespace mini_openmethod;
    auto parler = creer_methode<std::string_view(const Animal&)>(
        domaines<liste_types<Chien, Chat>>{},
        [](const Chien&) -> std::string_view { return "ouaf"; },
        [](const Chat&) -> std::string_view { return "miaou"; });

    Chien chien;
    const Animal& animal = chien;
    return parler(animal) == "ouaf" ? 0 : 1;
}
```

Aucune opération n'est ajoutée à `Animal`. Les lambdas fournissent leurs propres
signatures : il n'est pas nécessaire de répéter `Chien` dans un appel
`override_for<Chien>`. Les captures, les lambdas `mutable`, les lambdas
`noexcept` et les pointeurs de fonctions sont pris en charge.

## Réutiliser une référence préparée

Quand le même objet est traité plusieurs fois, son type peut être validé une
seule fois. Dans l'exemple précédent :

```cpp
auto chien_prepare = parler.preparer(chien);
auto resultat = parler(chien_prepare);
```

Pour une méthode à deux arguments, préparer chaque position séparément :

```cpp
auto gauche = interaction.preparer<0>(chien);
auto droite = interaction.preparer<1>(chat);
auto resultat = interaction(gauche, droite);
```

`preparer` rejette les types inconnus et refuse les objets temporaires à la
compilation. L'appel préparé réutilise directement les indices. La référence
ne possède pas l'objet : celui-ci doit rester vivant à la même adresse, avec
le même type dynamique. Un remplacement de l'objet exige une nouvelle préparation.
Les changements de ses données restent visibles.

La racine et la liste ordonnée des types font partie du type de la référence.
Elle peut servir à plusieurs méthodes compatibles et reste utilisable après
déplacement de la méthode. L'appel accepte soit tous les objets ordinaires,
soit toutes les références préparées. Cette option est utile pour les appels
répétés ; elle n'est pas systématiquement plus rapide sur les petits domaines.

## Construire et tester

Prérequis : CMake 3.25 ou supérieur, compilateur avec mode C++23, RTTI et exceptions
activés, et vcpkg initialisé. La CI prévoit GCC 13, Clang 18 et MSVC.
Le manifeste fixe les versions des dépendances ; la chaîne vcpkg installe les composants
nécessaires pendant la configuration. Adapter le chemin vers vcpkg ci-dessous :

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/chemin/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Pour un générateur à configuration unique, ajouter
`-DCMAKE_BUILD_TYPE=Release` à la configuration. Sous Windows avec Visual Studio,
les exemples se trouvent dans `build/Release/` ; sous Linux avec Make ou Ninja,
ils se trouvent directement dans `build/`.
Pour reprendre un dossier configuré avant l'ajout de vcpkg, ajouter `--fresh`
à la première configuration afin que CMake charge la nouvelle chaîne.

Les tests d'exécution utilisent les assertions et les contrôles d'exceptions de
doctest, y compris en Release. CTest découvre automatiquement chaque scénario
nommé ; par exemple, pour ne lancer que les tests d'indexation :

```sh
ctest --test-dir build -C Release -R "unitaire/Indexation" --output-on-failure
```

Les `static_assert` continuent de vérifier les règles à la compilation. Des
programmes incorrects sont compilés séparément : un test négatif réussit
uniquement si le compilateur refuse le programme avec le diagnostic attendu.
`-DMINI_OPENMETHOD_TESTS=OFF` désactive les tests et leur dépendance doctest.
La fonctionnalité facultative `tests` du manifeste vcpkg est sélectionnée par
CMake lorsque les tests sont activés dans ce dépôt utilisé comme projet principal.

Boost.CallableTraits extrait les paramètres et le retour des signatures ;
Boost.Mp11 vérifie l'unicité des types des domaines. Ces opérations ont lieu
à la compilation. Les traits standards déjà suffisants restent dans `std`.
La cible exportée `mini_openmethod::mini_openmethod` transmet les dépendances
d'en-têtes à ses consommateurs, qui doivent aussi disposer de ces composants
Boost (par leur chaîne vcpkg ou leur `CMAKE_PREFIX_PATH`).

## Compilation locale avec LLVM sous Windows

Le préréglage `llvm` utilise `clang++.exe` dans
`%ProgramFiles%\LLVM\bin` (habituellement `C:\Program Files\LLVM\bin`).
Il génère les fichiers de compilation dans `build-llvm/`, séparément des
fichiers de Visual Studio. Il active les exemples et les tests en Release.

Prérequis : LLVM, Ninja, CMake et vcpkg accessibles sur la machine, ainsi que les outils
C++ de Visual Studio et le SDK Windows. Le compilateur est Clang ; les en-têtes
et bibliothèques standard restent ceux de l'environnement Microsoft installé.

Depuis la racine du dépôt, dans PowerShell :

```powershell
$env:VCPKG_ROOT = 'C:/chemin/vcpkg'
cmake --preset llvm
cmake --build --preset llvm
ctest --preset llvm
```

Cette configuration a été vérifiée avec LLVM 23.1.2 et Visual Studio
Professional 2026 sur Windows x64, y compris les tests de refus de compilation.
Le préréglage est disponible uniquement sous Windows ; la procédure générale
ci-dessus reste utilisable sur les autres systèmes.

Si LLVM est installé ailleurs, adapter le chemin lors de la configuration :
`cmake --preset llvm "-DCMAKE_CXX_COMPILER=D:/Outils/LLVM/bin/clang++.exe"`.

Les [préréglages CMake](https://cmake.org/cmake/help/v3.25/manual/cmake-presets.7.html)
conservent ces réglages afin de rendre les commandes reproductibles.

## Sélection automatique C++23 / C++26

`MINI_OPENMETHOD_REFLEXION` accepte trois valeurs dans CMake :

| Valeur | Comportement |
| --- | --- |
| `AUTO` (défaut) | Sélectionne la réflexion si une véritable table de double dispatch compile ; sinon utilise C++23 |
| `ON` | Exige la réflexion C++26 et signale une erreur si elle est indisponible |
| `OFF` | Force l'implémentation C++23, même avec un compilateur compatible C++26 |

La sonde vérifie ensemble le compilateur, la bibliothèque `<meta>` et les
opérations de réflexion utilisées. Elle ne se fonde pas sur un numéro de version.
CMake essaie d'abord sans option supplémentaire, puis avec `-freflection`
pour GCC et Clang. Il doit connaître le mode C++26 du compilateur choisi.
Une ancienne configuration garde sa valeur en cache : passer explicitement
`-DMINI_OPENMETHOD_REFLEXION=AUTO` pour activer la sélection automatique.

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/chemin/vcpkg/scripts/buildsystems/vcpkg.cmake -DMINI_OPENMETHOD_REFLEXION=AUTO -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Les deux générateurs sont séparés dans
[`resolution_cpp23.hpp`](include/mini_openmethod/detail/resolution_cpp23.hpp) et
[`resolution_cpp26.hpp`](include/mini_openmethod/detail/resolution_cpp26.hpp).
Le premier développe des paramètres de modèles ; le second parcourt les types
réfléchis dans des boucles `consteval`. Ils produisent les mêmes indices de
spécialisations, puis une couche commune construit la table de pointeurs.
Aucun changement des appels à `creer_methode` n'est nécessaire.

Sans CMake, [`configuration.hpp`](include/mini_openmethod/configuration.hpp)
détecte les capacités **déjà activées**. Ajouter aussi les en-têtes Boost au chemin
d'inclusion. Avec GCC 16.2, compiler avec
`-std=c++26 -freflection` active automatiquement la réflexion ; `-std=c++23`
conserve les modèles. Un en-tête ne peut pas activer les options du compilateur.
La définition explicite `MINI_OPENMETHOD_REFLEXION=0` ou `1` force le choix.
`mini_openmethod::reflexion_active` permet de connaître l'implémentation retenue.
Garder le même réglage dans toutes les unités de compilation ; la cible CMake
exportée propage le choix, le standard minimum et les options à ses consommateurs.

Sélection vérifiée localement le 1er octobre 2026 : GCC 16.2 dans le conteneur
choisit C++26 ; Clang 23.1.2 et MSVC 19.51 sous Windows choisissent C++23.

La réflexion simplifie la génération des tables à la compilation. Elle ne
supprime pas l'identification des objets par `typeid` à l'exécution. Les domaines
restent explicites et fermés. Les [mesures et leurs limites](documentation/performances.md)
distinguent le coût de compilation du coût d'appel.

## Vérifier avec le conteneur GCC 16.2

Les fichiers sont conservés dans [`outils/`](outils) :
[`Dockerfile.gcc`](outils/Dockerfile.gcc),
[`verifier-gcc.ps1`](outils/verifier-gcc.ps1) et
[`verifier-gcc.sh`](outils/verifier-gcc.sh).
Sous Windows, avec Docker Desktop démarré :

```powershell
./outils/verifier-gcc.ps1
```

Le script construit l'image `mini-openmethod-gcc:16.2`, puis configure, compile,
teste et installe les variantes `OFF`, `ON` et `AUTO`. Les sources sont montées
en lecture seule. Les exécutables Linux, installations, journaux de tests et
mesures restent dans `build-gcc/cpp23`, `build-gcc/cpp26` et `build-gcc/auto`.
Le conteneur est supprimé à la fin ; l'image et ces résultats sont conservés.
L'image contient vcpkg à la révision du manifeste. Les dépendances d'en-têtes
sont installées une fois dans `build-gcc/vcpkg_installed` et partagées par les
trois configurations ; Boost.OpenMethod n'est pas nécessaire à cette validation.

Les modes réfléchis `ON` et `AUTO` vérifient aussi un domaine de **16 × 16 types
avec 256 spécialisations**, sans relever les limites `constexpr` du compilateur.
Le générateur mémorise les relations entre types distincts et partage la sélection
du candidat maximal avec la voie C++23.

La CI utilise le même script pour GCC 16.2. GCC 13, Clang 18 et MSVC vérifient
le repli automatique C++23. Les tests négatifs emploient aussi le standard et
les options du mode choisi et contrôlent toujours le diagnostic attendu.

## Comparer avec Boost.OpenMethod via vcpkg

Le banc facultatif compare les deux bibliothèques sur les mêmes objets :
dispatch simple avec 2, 8 et 32 types, double dispatch avec 2 × 2 et 8 × 8 types,
puis héritage virtuel. Pour chaque bibliothèque, il sépare les appels ordinaires
et ceux depuis une référence préparée avant la mesure (`virtual_ptr` pour Boost).

Avec les outils C++ de Visual Studio, depuis PowerShell :

```powershell
./outils/comparer-boost.ps1
./outils/comparer-boost.ps1 -Compilateur llvm
```

Le script utilise `VCPKG_ROOT` si défini, sinon cherche le vcpkg fourni avec
Visual Studio. `-RacineVcpkg C:/chemin/vcpkg` permet de choisir une installation.
Le [manifeste](vcpkg.json) fixe le registre et rend Boost.OpenMethod facultatif via
`MINI_OPENMETHOD_COMPARAISON_BOOST=ON`. Les paquets restent dans
`build-comparaison-vcpkg/vcpkg_installed` ; les mesures dans
`build-boost-msvc/comparaison.txt` ou `build-boost-llvm/comparaison.txt`.

Voir les [résultats, le protocole et les commandes CMake](documentation/comparaison_boost.md).
L'indexation optimisée réduit fortement l'écart initial. Les résultats dépendent
du compilateur, de la taille du domaine et de la réutilisation des références.

## Progression proposée

| Étape | Exemple | Ce que l'on observe |
| --- | --- | --- |
| 1 | [Dispatch simple](exemples/01_dispatch_simple.cpp) | Sélection par le type réel derrière une référence de base |
| 2 | [Héritage](exemples/02_heritage.cpp) | Repli vers la spécialisation applicable la plus précise |
| 3 | [Double dispatch](exemples/03_dispatch_double.cpp) | Sélection par une paire ordonnée de types |
| 4 | [Ambiguïté résolue](exemples/04_ambiguite_resolue.cpp) | Ajout de l'intersection qui départage deux spécialisations |

Chaque exemple est autonome et retourne un code d'échec si son résultat est
incorrect. Les deux implémentations couvrent toute cette progression, du
dispatch simple au double dispatch.

## Héritage et ordre de spécialisation

Une fonction prenant `const Mammifere&` peut traiter un `Chat` dérivé de
`Mammifere`. Si une fonction prenant `const Chat&` existe, elle est plus précise.

Pour deux arguments, une spécialisation doit être au moins aussi précise
sur **chaque** position et strictement plus précise sur au moins une position.
L'ordre de déclaration ne sert jamais à départager les fonctions.

Par exemple, `(Chien, Animal)` et `(Animal, Chien)` sont incomparables pour
`(Chien, Chien)`. L'ajout d'une fonction `(Chien, Chien)` résout l'ambiguïté.
Sans elle, la construction de la méthode échoue à la compilation, même si elle
n'est jamais appelée. Voir le [programme volontairement invalide](tests/refus/ambiguite_double.cpp).

Les deux domaines peuvent avoir des tailles et des racines différentes,
par exemple `Animal × Support`. Le dispatch n'est pas automatiquement
symétrique : `Chien × Chat` et `Chat × Chien` désignent deux cases distinctes.

## Architecture

1. **Décrire** : `domaines<liste_types<...>, ...>` énumère les types dynamiques
   autorisés, une liste par argument.
2. **Analyser** : les traits lisent la signature explicite de chaque appelable.
   Les concepts et assertions vérifient les références et les relations d'héritage.
3. **Résoudre** : pour chaque combinaison du produit cartésien, le compilateur
   retient l'unique candidat maximal selon l'ordre de spécialisation.
4. **Vérifier** : absence de candidat ou plusieurs candidats maximaux provoquent
   une erreur. Le numéro de case apparaît dans l'instanciation diagnostiquée.
5. **Appeler** : `typeid` trouve les indices des types réels, ou les références
   préparées les fournissent directement, puis une table de
   pointeurs de fonctions sélectionne un relais. Ce relais ajuste les références
   avec `static_cast` lorsque cette conversion est autorisée, ou `dynamic_cast`
   pour une base virtuelle, et invoque la lambda stockée dans un `std::tuple`.

Il n'y a ni registre global, ni initialisation statique dispersée, ni
`std::function`, ni allocation réalisée par le moteur à l'exécution. Les captures
et les fonctions utilisateur peuvent naturellement allouer. Les vecteurs de
réflexion temporaires n'existent que pendant l'évaluation à la compilation.

Jusqu'à 8 types, l'indexation recherche la première adresse RTTI correspondante.
Au-delà, elle utilise une table hachée sans allocation, initialisée une fois au
premier accès au domaine. Les collisions sont résolues sans accepter de faux
positif. Si l'adresse est absente, une recherche par égalité des types préserve
le cas d'adresses RTTI distinctes pour un même type. Un type inconnu est refusé.
Le chemin haché a un coût moyen attendu constant, mais son pire cas reste O(N).
Sous GCC, la petite recherche est explicitement intégrée à l'appelant pour éviter
la régression mesurée sur le dispatch à deux types ; Clang et MSVC ne reçoivent
pas cette indication. Les [mesures de correction](documentation/performances.md)
comparent plusieurs formes de recherche sur l'appel complet.
L'appel préparé évite cette recherche ; les conversions RTTI éventuelles pour
les bases virtuelles et l'appel indirect restent présents.
La table contient N ou N × M cases. La résolution compare les K spécialisations
deux à deux, avec un travail de l'ordre de O(N × M × K²) en double dispatch.
Cette approche privilégie la lisibilité. La
[comparaison avec Boost.OpenMethod](documentation/comparaison_boost.md)
quantifie le coût de l'indexation sur plusieurs tailles de domaine.

Voir les [détails de conception](documentation/architecture.md).

## Open-world et closed-world

| Propriété | Prototype livré : closed-world | Extension open-world possible |
| --- | --- | --- |
| Opérations extérieures aux classes | Oui | Oui |
| Types dynamiques possibles | Liste explicite à la compilation | Enregistrements au chargement ou à l'exécution |
| Spécialisations possibles | Ensemble fourni à la construction | Contributions de plusieurs modules ou greffons |
| Ambiguïtés | Vérifiées à la compilation sur tout le domaine | Vérifiées à l'initialisation ou lors des modifications |
| Type ajouté après compilation | Exception `type_inconnu` | Reconstruction ou mise à jour du registre |
| Coût d'une extension | Recompiler la composition | Gérer registre, synchronisation et durée de vie |

« Méthode externe » ne signifie donc pas « extension dynamique illimitée ».
Ce projet garde les opérations hors des classes mais ferme explicitement
l'univers des types et des spécialisations pour obtenir la garantie statique.

Une spécialisation peut viser une classe intermédiaire absente de la liste :
cette liste décrit les **types dynamiques exacts acceptés**, pas toutes les
classes pouvant apparaître dans les signatures. Un descendant non listé est
refusé même si une spécialisation de sa base pourrait le traiter.

## Contrat et limites

- Une ou deux positions polymorphes, toutes sous forme `const Classe&`.
  Aucun argument ordinaire supplémentaire, pointeur nullable ou référence mutable.
- Racines polymorphes ; héritage public et non ambigu vers chaque racine.
  L'héritage multiple et les diamants virtuels sont testés.
- Les listes sont non vides, sans doublons et contiennent des classes sans
  qualification `const` ou référence. Toutes leurs combinaisons doivent être couvertes.
- Les lambdas génériques, ensembles surchargés et foncteurs à opérateurs qualifiés
  par référence ne sont pas analysés. Utiliser une lambda intermédiaire à signature explicite.
- Les types de retour doivent correspondre exactement à la signature annoncée.
  `void`, les valeurs et les références sont acceptés ; aucune covariance implicite.
- Les exceptions utilisateur sont propagées. Une méthode n'est pas déclarée
  `noexcept`, même si ses lambdas le sont.
- Les appelables sont possédés par valeur et peuvent être seulement déplaçables.
  Les objets et les captures par référence doivent rester vivants pendant l'appel.
- L'appel est non `const` pour permettre les captures mutables. Le partage entre
  threads exige que les appelables et les données capturées soient sûrs, ou une
  synchronisation externe.
- Pas de découverte automatique des classes, de greffons, de `next`,
  de désenregistrement, ni d'optimisation des tables clairsemées.

## Intégrer la bibliothèque

```cmake
add_subdirectory(chemin/vers/mini-openmethod)
target_link_libraries(mon_programme PRIVATE mini_openmethod::mini_openmethod)
```

Les exemples et tests sont désactivés par défaut en sous-projet. Ils peuvent
être pilotés par `MINI_OPENMETHOD_EXEMPLES` et `MINI_OPENMETHOD_TESTS`.
Si les tests sont activés en sous-projet, le projet parent doit aussi fournir doctest.

Une installation est également disponible :

```sh
cmake --install build --config Release --prefix installation
```

Le consommateur peut ensuite utiliser `find_package(mini_openmethod CONFIG REQUIRED)`
et la même cible, avec `CMAKE_PREFIX_PATH` pointant vers l'installation.

## Sources et suites possibles

La [documentation officielle de Boost.OpenMethod](https://www.boost.org/doc/libs/1_90_0/libs/openmethod/doc/html/openmethod/core_api.html)
décrit déjà une API sans macros. L'intérêt de ce prototype est d'expérimenter
une composition explicite par lambdas et la vérification statique d'un domaine fermé.

Les règles d'héritage utilisées reposent sur
[`std::derived_from`](https://eel.is/c++draft/concept.derived).
La variante réfléchie emploie les
[traits de réflexion](https://eel.is/c++draft/meta.reflection.traits) équivalents.
Les ajustements à l'exécution suivent
[`static_cast`](https://eel.is/c++draft/expr.static.cast) ou
[`dynamic_cast`](https://eel.is/c++draft/expr.dynamic.cast).

Évolutions envisagées, non implémentées : diagnostics nommant les types ambigus,
mesures sur de plus grands domaines, puis registre
ouvert séparé. La réflexion C++26 analyse les domaines et les relations
d'héritage pour construire les tables ; elle ne découvre pas automatiquement
toutes les classes d'un programme.
