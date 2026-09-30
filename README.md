# mini-openmethod

Prototype pédagogique C++23 de méthodes externes à une hiérarchie polymorphe.
L'API n'utilise aucune macro : les concepts valident les signatures, les traits
extraient les paramètres des lambdas et les fonctions `consteval` construisent
une table de dispatch conservée en `constexpr`.

**Le dispatch dépend du type dynamique des objets. La résolution des
spécialisations et la détection des ambiguïtés ont lieu à la compilation,
pour un domaine explicitement fermé.**

Ce projet expérimental ne dépend ni de Boost ni d'une bibliothèque de tests.
Il ne cherche pas à reproduire toutes les possibilités de Boost.OpenMethod.
La cible est C++23 ; le noyau emploie principalement des mécanismes déjà
disponibles en C++20, sans réflexion C++26.

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

## Construire et tester

Prérequis : CMake 3.25 ou supérieur, compilateur avec mode C++23, RTTI et exceptions
activés. La CI prévoit GCC 13, Clang 18 et MSVC. Aucun téléchargement de dépendance
n'est effectué par CMake.

```sh
cmake -S . -B build
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Pour un générateur à configuration unique, ajouter
`-DCMAKE_BUILD_TYPE=Release` à la configuration. Sous Windows avec Visual Studio,
les exemples se trouvent dans `build/Release/` ; sous Linux avec Make ou Ninja,
ils se trouvent directement dans `build/`.

Les tests vérifient les résultats à l'exécution, y compris en Release, et
compilent séparément des programmes incorrects. Un test négatif réussit
uniquement si le compilateur refuse le programme avec le diagnostic attendu.

## Progression proposée

| Étape | Exemple | Ce que l'on observe |
| --- | --- | --- |
| 1 | [Dispatch simple](exemples/01_dispatch_simple.cpp) | Sélection par le type réel derrière une référence de base |
| 2 | [Héritage](exemples/02_heritage.cpp) | Repli vers la spécialisation applicable la plus précise |
| 3 | [Double dispatch](exemples/03_dispatch_double.cpp) | Sélection par une paire ordonnée de types |
| 4 | [Ambiguïté résolue](exemples/04_ambiguite_resolue.cpp) | Ajout de l'intersection qui départage deux spécialisations |

Chaque exemple est autonome et retourne un code d'échec si son résultat est
incorrect. Le même moteur évolue du dispatch simple au double dispatch ;
il n'y a pas quatre implémentations concurrentes à entretenir.

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
5. **Appeler** : `typeid` trouve les indices des types réels, puis une table de
   pointeurs de fonctions sélectionne un relais. Ce relais ajuste les références
   avec `dynamic_cast` et invoque la lambda stockée dans un `std::tuple`.

Il n'y a ni registre global, ni initialisation statique dispersée, ni
`std::function`, ni allocation réalisée par le moteur. Les captures et les
fonctions utilisateur peuvent naturellement allouer.

L'indexation RTTI actuelle parcourt les types de chaque domaine : son coût est
O(N) en simple dispatch, O(N + M) en double dispatch, auquel s'ajoutent les
conversions RTTI et l'appel indirect. Seul l'accès à la table est constant.
La table contient N ou N × M cases. La résolution compare les K spécialisations
deux à deux, avec un travail de l'ordre de O(N × M × K²) en double dispatch.
Cette approche privilégie la lisibilité ; aucune supériorité de performance
sur Boost.OpenMethod n'est revendiquée.

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
Les ajustements à l'exécution suivent
[`dynamic_cast`](https://eel.is/c++draft/expr.dynamic.cast).

Évolutions envisagées, non implémentées : indexation RTTI plus efficace, diagnostics
nommant les types ambigus, mesures comparatives, puis registre ouvert séparé.
La réflexion pourrait simplifier la description de la hiérarchie ; elle ne
supprime pas la nécessité d'identifier les types dynamiques.
