# mini-openmethod

Bibliothèque pédagogique C++23 qui choisit une fonction selon le **type réel
d'un ou de deux objets**. Les opérations sont définies à l'extérieur des classes,
sans macro de déclaration.

On peut ainsi ajouter de nouveaux traitements sans modifier les classes du
domaine : elles ne connaissent ni ces opérations, ni la bibliothèque.
La [comparaison avec Visitor et `std::visit`](documentation/comparaison_visitor.md)
présente ce découplage, les variantes avec `std::variant` et les différences
entre leurs mécanismes de sélection.

La limite porte sur les arguments qui choisissent la fonction : une méthode peut
aussi transmettre des arguments ordinaires supplémentaires, comme un montant.

Les types possibles sont listés explicitement. Le compilateur choisit la bonne
fonction pour chaque combinaison et refuse les cas non couverts ou ambigus.
À l'exécution, la méthode identifie les objets puis consulte cette table.

## Premier exemple

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

La référence est de type `Animal`, mais l'objet est un `Chien` : la méthode
appelle la fonction qui renvoie `"ouaf"`. Aucune méthode `parler` n'est ajoutée
à la classe `Animal`.

Le domaine reste fermé : un type absent des listes provoque `type_inconnu`.
Ajouter un nouveau type ou une spécialisation demande une nouvelle compilation.

## Essayer

Il faut CMake 3.25 ou supérieur, un compilateur C++23 avec RTTI et exceptions,
et vcpkg. La bibliothèque est constituée d'en-têtes ; Boost.CallableTraits et
Boost.Mp11 sont ses dépendances. doctest est réservé aux tests.

Depuis la racine du dépôt, adapter le chemin vers vcpkg :

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/chemin/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Le [guide de compilation](documentation/construction.md) détaille le préréglage
LLVM sous Windows, les autres configurations et l'intégration dans un projet.

## Continuer la lecture

Le **[sommaire de la documentation](documentation/README.md)** propose un parcours progressif :

1. [Utiliser la bibliothèque](documentation/utilisation.md) avec les exemples 01 à 03.
2. [Comprendre les tables](documentation/tables_dispatch.md) à l'aide des schémas.
3. [Explorer le hachage, les collisions et les erreurs](documentation/exemples_indexation_rtti.md).

L'architecture détaillée et les résultats de performance restent accessibles
depuis le sommaire, pour approfondir ensuite.
La [référence API](documentation/reference.md) regroupe les règles complètes et
l'[aide aux diagnostics](documentation/reference.md#comprendre-une-erreur).

## Évolutions prévues

Les évolutions prévues sont de préparer les références depuis un domaine,
d'hériter d'un domaine et de ses méthodes, d'accepter les fonctions membres avec
la forme `methode(objet, arguments…)`, puis de permettre la préparation et les
appels admissibles à la compilation, avec un indice intégré aux classes.

La préparation portée par le domaine reprend la
[proposition de jll63](https://github.com/Boumg/mini-openmethod/issues/6#issuecomment-6301317986).

La [feuille de route](documentation/evolutions.md) donne des exemples et propose
d'autres pistes inspirées de Boost.OpenMethod, dont le dispatch sur trois objets
ou davantage. Ces évolutions ne sont pas encore disponibles.

## Licence et versions

La première version publiée est [0.1.0](https://github.com/Boumg/mini-openmethod/releases/tag/v0.1.0).
Le projet est distribué sous [Boost Software License 1.0](LICENSE.md).
Le [journal des modifications](CHANGELOG.md) présente les évolutions et les
règles de compatibilité des versions `0.x`.
