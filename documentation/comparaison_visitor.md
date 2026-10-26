# Ajouter des opérations : Visitor et mini-openmethod

[Sommaire](README.md) · [Guide d'utilisation](utilisation.md)

Une classe décrit un objet et ses règles propres. Elle n'a pas nécessairement
à connaître tous les traitements qui pourront lui être appliqués : affichage,
export, facturation, etc. **Définir ces opérations à l'extérieur permet d'en
ajouter sans modifier les classes du domaine.**

Visitor et mini-openmethod poursuivent cet objectif. La différence porte sur
le mécanisme à prévoir dans les classes. Cette page compare le **Visitor
classique avec `accept` et des fonctions virtuelles**, pas `std::visit` ni
toutes les variantes du patron.

## Avec Visitor : les classes savent recevoir un visiteur

Dans cet exemple, `accepter` correspond à la fonction souvent nommée `accept`.
Chaque classe concrète transmet `*this` au visiteur, avec son type précis.
Le visiteur contient le traitement :

```cpp
#include <string_view>

struct Chien;
struct Chat;

struct Visiteur {
    virtual ~Visiteur() = default;
    virtual std::string_view visiter(const Chien&) const = 0;
    virtual std::string_view visiter(const Chat&) const = 0;
};

struct Animal {
    virtual ~Animal() = default;
    virtual std::string_view accepter(const Visiteur&) const = 0;
};

struct Chien final : Animal {
    std::string_view accepter(const Visiteur& visiteur) const override {
        return visiteur.visiter(*this);
    }
};

struct Chat final : Animal {
    std::string_view accepter(const Visiteur& visiteur) const override {
        return visiteur.visiter(*this);
    }
};

struct Parler : Visiteur {
    std::string_view visiter(const Chien&) const override { return "ouaf"; }
    std::string_view visiter(const Chat&) const override { return "miaou"; }
};

// Nouvelle opération : les classes Animal, Chien et Chat restent inchangées.
struct Decrire : Visiteur {
    std::string_view visiter(const Chien&) const override { return "un chien"; }
    std::string_view visiter(const Chat&) const override { return "un chat"; }
};

int main() {
    Chien chien;
    Chat chat;
    const Animal& animal = chien;
    return animal.accepter(Parler{}) == "ouaf"
        && chat.accepter(Parler{}) == "miaou"
        && animal.accepter(Decrire{}) == "un chien"
        && chat.accepter(Decrire{}) == "un chat" ? 0 : 1;
}
```

Pour `animal.accepter(Parler{})`, l'appel virtuel atteint `Chien::accepter`.
Dans cette fonction, `*this` est un `Chien` : la surcharge `visiter(const Chien&)`
est choisie, puis l'appel virtuel atteint son implémentation dans `Parler`.

**Une fois ce protocole installé**, ajouter `Decrire` ne demande aucune
modification des classes animales. Elles connaissent l'interface `Visiteur`,
mais pas les visiteurs concrets `Parler` et `Decrire`. Le retour `string_view`
est un choix de cet exemple ; d'autres interfaces de visite sont possibles.

## Avec mini-openmethod : les classes ne connaissent pas les opérations

Les classes n'ont ni `accepter`, ni interface `Visiteur`. Elles sont ici
polymorphes grâce au destructeur virtuel d'`Animal`. Les opérations déclarent
elles-mêmes les types qu'elles savent traiter :

```cpp
#include <mini_openmethod/methode.hpp>
#include <string_view>

struct Animal { virtual ~Animal() = default; };
struct Chien final : Animal {};
struct Chat final : Animal {};

int main() {
    using namespace mini_openmethod;
    auto parler = creer_methode<std::string_view(const Animal&)>(
        domaines<liste_types<Chien, Chat>>{},
        [](const Chien&) -> std::string_view { return "ouaf"; },
        [](const Chat&) -> std::string_view { return "miaou"; });

    // Nouvelle opération : les classes Animal, Chien et Chat restent inchangées.
    auto decrire = creer_methode<std::string_view(const Animal&)>(
        domaines<liste_types<Chien, Chat>>{},
        [](const Chien&) -> std::string_view { return "un chien"; },
        [](const Chat&) -> std::string_view { return "un chat"; });

    Chien chien;
    Chat chat;
    const Animal& animal = chien;
    return parler(animal) == "ouaf" && parler(chat) == "miaou"
        && decrire(animal) == "un chien" && decrire(chat) == "un chat" ? 0 : 1;
}
```

`decrire(animal)` identifie un `Chien` et appelle la lambda correspondante.
L'opération dépend des classes ; **les classes ne dépendent ni de l'opération,
ni de mini-openmethod**. On peut ainsi placer les traitements et leurs
dépendances dans un composant distinct, tout en conservant la hiérarchie.

## Ce qui change quand le programme évolue

| Besoin | Visitor classique montré ici | mini-openmethod actuel |
| --- | --- | --- |
| Ajouter une opération | Ajouter un visiteur concret, sans changer les classes déjà visitables. | Ajouter une méthode externe et ses spécialisations, sans changer les classes. |
| Préparer les classes | Déclarer `accepter` et l'implémenter pour chaque type concret. | Disposer d'une hiérarchie polymorphe compatible ; aucun protocole de visite à ajouter. |
| Ajouter un type, par exemple `Oiseau` | Ajouter son `accepter`, étendre `Visiteur` et compléter les visiteurs concrets. | Ajouter le type aux domaines concernés et vérifier la couverture des méthodes ; une spécialisation générale peut suffire. |
| Choisir selon deux objets | Organiser une composition de visites ou un mécanisme supplémentaire. | Déclarer deux positions de dispatch ; chaque paire de types est vérifiée. |
| Oublier un cas | Une surcharge virtuelle pure non implémentée empêche d'instancier le visiteur. | Une combinaison du domaine sans spécialisation applicable empêche de construire la méthode. |

Le Visitor classique convient notamment à une hiérarchie stable déjà équipée
du protocole de visite. Les opérations externes sont utiles pour des traitements
ajoutés par différents composants, ou pour une hiérarchie existante que l'on ne
souhaite pas équiper d'un visiteur. Cette motivation est également exposée dans
la [documentation de Boost.OpenMethod](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/motivation.html).

## Les limites du découplage

Les opérations doivent toujours connaître les types et utiliser leurs interfaces
accessibles. Elles n'obtiennent aucun accès automatique aux membres privés.
Si un traitement exige une information que la classe n'expose pas, son interface
peut devoir évoluer.

Ajouter une opération externe demande de compiler son code. Ajouter un type
ou une spécialisation à une méthode existante demande de recomposer et de
recompiler cette méthode. Un descendant absent de la liste reste refusé, même
si un traitement de sa base existe. Le domaine est fermé ; il n'y a pas de
registre extensible pendant l'exécution.

Cette comparaison porte sur l'organisation du code. Aucun banc Visitor n'est
fourni ici pour conclure sur sa vitesse relative. Les mesures disponibles
concernent les [appels C++ directs et virtuels](comparaison_appels_cpp.md) et
[Boost.OpenMethod](comparaison_boost.md).
