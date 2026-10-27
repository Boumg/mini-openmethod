# Ajouter des opérations : Visitor, std::visit et mini-openmethod

[Sommaire](README.md) · [Guide d'utilisation](utilisation.md)

Une classe décrit un objet et ses règles propres. Elle n'a pas nécessairement
à connaître tous les traitements qui pourront lui être appliqués : affichage,
export, facturation, etc. **Définir ces opérations à l'extérieur permet d'en
ajouter sans modifier les classes du domaine.**

Trois approches permettent de séparer les objets et leurs traitements :
**Visitor classique**, **`std::variant` avec `std::visit`** et **mini-openmethod**.
Elles diffèrent surtout par la représentation des objets et la manière de
retrouver le traitement applicable.

Les trois programmes ci-dessous sont indépendants. Le Visitor classique
installe un protocole dans les classes. Le variant porte l'indice de sa valeur
active. mini-openmethod reçoit une référence polymorphe et identifie son type
réel dans un domaine déclaré. **`std::variant` et mini-openmethod reposent tous
deux sur un ensemble explicite de types, fixé à la compilation.**

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

## Avec std::variant : le conteneur connaît l'alternative active

Ce programme C++17 réalise les mêmes opérations. Les classes n'ont besoin
d'aucune base commune ni de fonction virtuelle. Le type `Animal` désigne ici
un conteneur qui possède un `Chien` ou un `Chat` :

```cpp
#include <string_view>
#include <variant>

struct Chien {};
struct Chat {};
using Animal = std::variant<Chien, Chat>;

struct Parler {
    std::string_view operator()(const Chien&) const { return "ouaf"; }
    std::string_view operator()(const Chat&) const { return "miaou"; }
};

struct Decrire {
    std::string_view operator()(const Chien&) const { return "un chien"; }
    std::string_view operator()(const Chat&) const { return "un chat"; }
};

int main() {
    const Animal chien = Chien{};
    const Animal chat = Chat{};
    return std::visit(Parler{}, chien) == "ouaf"
        && std::visit(Parler{}, chat) == "miaou"
        && std::visit(Decrire{}, chien) == "un chien"
        && std::visit(Decrire{}, chat) == "un chat" ? 0 : 1;
}
```

Dans cet exemple, `chien.index()` vaut 0 et `chat.index()` vaut 1. À chaque
appel, `std::visit` transmet l'alternative active au visiteur. Le compilateur
vérifie que celui-ci peut être appelé pour **toutes** les alternatives.
Avec le résultat déduit utilisé ici, toutes les invocations doivent produire
le même type et la même catégorie de valeur. Les deux surcharges renvoient
donc explicitement `std::string_view`.

Ajouter `Decrire` ne modifie ni `Chien`, ni `Chat`, ni l'alias `Animal`.
Les opérations sont déjà extérieures aux classes, comme avec mini-openmethod.
Les règles précises figurent dans les sections du standard sur
[l'indice du variant](https://eel.is/c++draft/variant.status) et
[la visite](https://eel.is/c++draft/variant.visit).

## Avec mini-openmethod : les classes ne connaissent pas les opérations

Ce programme utilise l'API C++23 de mini-openmethod.

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

| Besoin | Visitor classique montré ici | `std::variant` et `std::visit` | mini-openmethod actuel |
| --- | --- | --- | --- |
| Ajouter une opération | Définir un visiteur concret pour les classes déjà visitables. | Ajouter un callable compatible avec les alternatives. | Ajouter une méthode externe et ses spécialisations. |
| Préparer les classes | Déclarer `accepter` et l'implémenter pour chaque type concret. | Les valeurs sont représentées par un variant ; aucune hiérarchie requise. | Disposer d'une hiérarchie polymorphe compatible. |
| Ajouter `Oiseau` | Ajouter son `accepter`, étendre `Visiteur` et compléter les visiteurs concrets. | Étendre le type du variant et vérifier les visiteurs concernés. | Étendre les domaines concernés et vérifier la couverture des méthodes. |
| Choisir selon deux objets | Organiser une composition de visites ou un mécanisme supplémentaire. | Passer deux variants au même `std::visit`. | Déclarer deux positions de dispatch. |
| Oublier un cas | Une surcharge virtuelle pure non implémentée empêche d'instancier le visiteur. | Une invocation invalide pour une combinaison empêche de compiler la visite. | Une combinaison sans spécialisation applicable empêche de construire la méthode. |

Le Visitor classique convient notamment à une hiérarchie stable déjà équipée
du protocole de visite. Les opérations externes sont utiles pour des traitements
ajoutés par différents composants, ou pour une hiérarchie existante que l'on ne
souhaite pas équiper d'un visiteur. Cette motivation est également exposée dans
la [documentation de Boost.OpenMethod](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/motivation.html).

## Comparer std::visit et mini-openmethod

### Représentation des objets et origine de l'indice

`std::variant<Chien, Chat>` possède sa valeur et expose l'indice de
l'alternative active. mini-openmethod travaille sur des objets existants,
transmis par `Animal&` ou `const Animal&`. Son appel brut utilise `typeid`
pour retrouver l'indice du type dynamique exact dans `liste_types<Chien, Chat>`.

Un variant peut également emprunter des objets. Cet extrait utilise les types
et le visiteur du programme `std::variant` précédent, dans son `main` :

```cpp
Chien objet;
std::variant<const Chien*, const Chat*> vue = &objet;
const auto cri = std::visit([](const auto* animal) {
    return Parler{}(*animal);
}, vue); // "ouaf"
if (cri != "ouaf") return 1;
```

Un variant de pointeurs typés ou de `std::reference_wrapper` permet ainsi de
comparer les approches sur les mêmes objets, sans les copier dans le conteneur.
L'appelant reste responsable de leur durée de vie. Il doit aussi déterminer
la bonne alternative lors de la construction de cette vue.

À l'inverse, dans une hiérarchie polymorphe, `variant<Animal*>` possède
l'alternative `Animal*`, même si le pointeur désigne un `Chien`. `std::visit`
transmet alors un `Animal*` : il ne sélectionne pas automatiquement une
surcharge prenant `Chien*`. Le traitement peut ensuite appeler une fonction
virtuelle, mais cette opération est distincte de la sélection de l'alternative.
mini-openmethod prend directement en charge l'identification du type réel
depuis la référence de base.

### Un domaine fermé dans les deux approches

Les alternatives de `std::variant<Types...>` et les types exacts de
`liste_types<Types...>` sont déclarés à la compilation. Étendre l'un de ces
ensembles impose de vérifier les traitements sur les nouveaux cas.

Dans mini-openmethod, un descendant absent du domaine provoque `type_inconnu`,
même si une spécialisation de sa base semble applicable. **Le domaine définit
les types acceptés ; les spécialisations définissent leurs traitements.** Une
spécialisation peut viser une classe intermédiaire absente du domaine des types
exacts. La [référence API](reference.md#décrire-la-signature-et-les-domaines)
détaille cette distinction.

### Préparer les appels à la compilation

Les deux mécanismes vérifient les appels possibles avant l'exécution. Une
implémentation de `std::visit` peut générer une table de fonctions ou utiliser
des branchements ; le standard n'impose pas une représentation unique.
mini-openmethod construit explicitement une table `static constexpr` :

| Étape | `std::visit` | mini-openmethod |
| --- | --- | --- |
| Définir les cas | Alternatives des variants. | Types des domaines déclarés. |
| Déterminer le traitement | Résolution C++ de l'appel du visiteur pour chaque combinaison. | Applicabilité et dominance des signatures sur les positions dispatchées. |
| Obtenir les indices | Indices déjà portés par les variants. | Identification RTTI ou indices de références préparées. |
| Fournir les arguments | Références aux alternatives actives. | Ajustement vers les paramètres de la spécialisation retenue. |
| Exécuter | Invocation du visiteur. | Appel du relais et de la fonction conservée dans la méthode. |

Dans l'implémentation, à chaque combinaison correspond un gagnant dans `resolution`,
puis un pointeur `&relais<gagnant>` dans la table. Plusieurs cases peuvent
partager le relais d'une spécialisation générale. Le choix de la meilleure
spécialisation est déjà fait : l'appel ne recommence pas cette recherche.
Voir la [construction des relais](../include/mini_openmethod/methode.hpp), la
[résolution C++23](../include/mini_openmethod/detail/resolution_cpp23.hpp) et
les [tables de dispatch](tables_dispatch.md).

### Héritage, couverture et ambiguïtés

Supposons une autre hiérarchie où `Chien` et `Chat` dérivent de `Mammifere`,
qui dérive d'`Animal`, et où `Oiseau` dérive directement d'`Animal`.
Pour le domaine `Chien, Chat, Oiseau`, des spécialisations prenant
`const Chien&`, `const Mammifere&` et `const Animal&` peuvent couvrir tous les cas :
`Chien` choisit la première, `Chat` la deuxième et `Oiseau` la troisième.

Un visiteur surchargé passé à `std::visit` peut lui aussi proposer ces trois
paramètres : la résolution de surcharge C++ choisit alors le traitement.
**Une surcharge exacte par alternative n'est donc pas obligatoire.**
mini-openmethod analyse les signatures et applique sa propre règle : une
fonction domine une autre si elle est au moins aussi précise sur chaque
position dispatchée et strictement plus précise sur au moins une.

Pour deux objets, les spécialisations `(Chien, Animal)` et `(Animal, Chien)`
restent incomparables sur `(Chien, Chien)`. Il faut résoudre cette intersection,
par exemple avec `(Chien, Chien)`. mini-openmethod rejette les ambiguïtés à la
compilation ; l'ordre de déclaration des fonctions ne donne aucune priorité.
Un ensemble de surcharges ambigu empêcherait également de compiler un appel
de `std::visit`. Les mécanismes de sélection diffèrent, même lorsque leurs
résultats coïncident dans les exemples simples d'héritage.

### Plusieurs objets et arguments ordinaires

L'appel `std::visit(visiteur, premier, second)` visite deux variants et
transmet leurs alternatives actives au même visiteur. Celui-ci doit être
invocable pour toutes les combinaisons : deux alternatives de chaque côté
donnent quatre cas. `std::visit` accepte également davantage de variants.

mini-openmethod accepte actuellement **une ou deux positions dispatchées**,
plus des arguments ordinaires décrits par `argument_ordinaire`. Seules les
positions dispatchées créent des dimensions dans sa table. Pour deux domaines
de tailles `N` et `M`, le couple `(i, j)` sélectionne la case `i * M + j`.
Avec `std::visit`, les paramètres supplémentaires peuvent être transmis au
traitement par un visiteur ou une lambda d'adaptation.

L'API mini-openmethod demande des signatures explicites analysables : les
lambdas génériques et foncteurs surchargés ne sont pas acceptés comme
spécialisations. Les positions dispatchées sont `Base&` ou `const Base&`,
avec la même qualification `const` dans les spécialisations ; leur retour
doit être exactement celui du contrat annoncé. `std::visit` accepte des
callables génériques et transmet aussi les catégories de valeur du variant.
Sa forme à résultat déduit exige un résultat commun exact ; `std::visit<R>`,
disponible depuis C++20, permet d'imposer le type de résultat selon les règles
d'invocation correspondantes. Voir la [référence mini-openmethod](reference.md#construire-et-appeler-une-méthode)
et les [règles de visite du standard](https://eel.is/c++draft/variant.visit).

### Références préparées : conserver l'indice du type

Cet extrait s'insère dans le `main` du programme mini-openmethod, avant son retour :

```cpp
auto animal_prepare = parler.preparer(animal);
const auto cri = parler(animal_prepare);
const auto description = decrire(animal_prepare);
if (cri != "ouaf" || description != "un chien") return 1;
```

La référence conserve un pointeur vers l'objet et son indice de type validé.
Elle peut déjà servir à plusieurs méthodes compatibles : même racine, même
liste ordonnée de types et droits d'accès compatibles. Chaque méthode consulte
sa propre table avec cet indice. **Aucune fonction sélectionnée ni résultat
d'appel n'est mémorisé.**

L'objet doit rester vivant, à la même adresse et avec le même type dynamique.
Modifier ses données reste possible. Les appels préparés évitent une nouvelle
identification ; l'ajustement des références peut encore nécessiter
`dynamic_cast`, notamment en présence d'héritage virtuel.
Voir [les références préparées](reference.md#utiliser-une-référence-préparée).

Cette représentation se rapproche d'un variant de pointeurs typés ou de
`std::reference_wrapper` : les deux portent un discriminant disponible avant
l'appel. La préparation depuis un
domaine, décrite dans la [feuille de route](evolutions.md#1-préparer-les-références-depuis-le-domaine),
est une évolution d'API prévue ; le partage entre méthodes compatibles existe
déjà avec `methode::preparer`.

## Performances : distinguer le mécanisme et les mesures

Le variant porte déjà son indice. Un appel mini-openmethod brut doit identifier
le type réel puis retrouver cet indice : recherche courte jusqu'à huit types,
index haché au-delà, avec un repli sur l'égalité des types. La référence préparée
amortit cette étape pour les appels répétés. La table de dispatch et l'index
des types sont deux structures différentes, comme l'expliquent les
[tables de dispatch](tables_dispatch.md#distinguer-les-deux-recherches).

Pour un seul variant, le standard exige une sélection indépendante du nombre
d'alternatives ; il n'impose pas cette garantie avec plusieurs variants.
Le coût du traitement et les optimisations du compilateur s'ajoutent à ce
raisonnement. Retirer une identification ne garantit pas à lui seul un meilleur
temps total : les accès mémoire et le code produit peuvent changer.

**Aucun banc du dépôt ne compare actuellement mini-openmethod à `std::visit`
ou au Visitor classique.** Les mesures disponibles concernent les
[appels directs et virtuels](comparaison_appels_cpp.md) et
[Boost.OpenMethod](comparaison_boost.md). Elles ne permettent pas d'annoncer
un classement entre les trois approches présentées ici.

Une comparaison mesurée avec `std::visit` devrait utiliser les mêmes objets,
par exemple via un variant de pointeurs typés, les mêmes traitements et le
même ordre d'appel. La création des vues et la préparation doivent être mesurées
séparément des appels répétés. Il faut aussi distinguer simple et double
dispatch, et rendre identique la visibilité des corps pour le compilateur.

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

`std::variant` convient à des données naturellement représentées par une somme
explicite de types. mini-openmethod apporte des opérations externes à une
hiérarchie polymorphe existante, avec des interfaces exprimées en références
de base et une validation du domaine à la compilation. Le Visitor classique
reste pertinent pour des classes déjà équipées de son protocole. Le choix
dépend donc de la représentation des objets et des règles de sélection attendues.
