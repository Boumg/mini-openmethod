# Utiliser la bibliothèque

[Sommaire](README.md)

Commencer par le [premier exemple](../README.md#premier-exemple) : la signature
`const Animal&` est commune, mais la fonction appelée dépend du type réel,
`Chien` ou `Chat`.

## Cinq mots pour lire les exemples

| Terme | Dans le premier exemple |
| --- | --- |
| Dispatch | Choisir la fonction qui renvoie `"ouaf"` ou `"miaou"` |
| Signature | `std::string_view(const Animal&)` : le retour et les paramètres de l'opération |
| Domaine | `liste_types<Chien, Chat>` : les types réels autorisés à cette position |
| Spécialisation | Une fonction fournie pour un cas, par exemple la lambda prenant `const Chien&` |
| Position polymorphe | Un argument dont le type réel participe au choix de la fonction |

Les fonctions sont conservées dans l'objet méthode. Aucune opération n'est
ajoutée aux classes de la hiérarchie.

Cette séparation permet d'ajouter un traitement sans modifier les classes :
elles ne connaissent pas les opérations qui les utilisent. La
[comparaison avec Visitor](comparaison_visitor.md) illustre cet avantage et ses
limites, en ajoutant une deuxième opération aux mêmes classes.

## Combien d'arguments ?

Une méthode possède **exactement une ou deux positions participant au dispatch**,
avec autant d'arguments ordinaires supplémentaires que nécessaire. Chaque
descripteur `liste_types<...>` désigne une position dispatchée ;
`argument_ordinaire` désigne un paramètre transmis au traitement.

| Paramètres de l'opération | Positions dispatchées | Accepté |
| --- | ---: | --- |
| `(Animal&, Animal&)`, deux listes de types | 2 | Oui |
| `(Animal&, Animal&, int)`, deux listes puis `argument_ordinaire` | 2 | Oui |
| `(Animal&, Animal&, Animal&)`, trois listes de types | 3 | Non |

Une méthode sans position dispatchée est également refusée. C'est le descripteur
qui choisit le rôle d'une position : un argument ordinaire peut lui-même être
un objet polymorphe, sans que son type intervienne dans le dispatch.

## Choisir un exemple

Les trois premiers suffisent pour découvrir le fonctionnement :

| Étape | Code complet | Ce que l'on apprend |
| --- | --- | --- |
| 1 | [Dispatch simple](../exemples/01_dispatch_simple.cpp) | Choisir une fonction selon un objet |
| 2 | [Héritage](../exemples/02_heritage.cpp) | Utiliser une spécialisation plus générale |
| 3 | [Double dispatch](../exemples/03_dispatch_double.cpp) | Choisir selon une paire ordonnée d'objets |

Continuer ensuite selon le besoin :

- [Résoudre une ambiguïté](../exemples/04_ambiguite_resolue.cpp).
- [Transmettre un montant ou un flux](../exemples/05_arguments_ordinaires.cpp).
- [Modifier les objets](../exemples/06_objets_modifiables.cpp).
- [Observer l'indexation et ses erreurs](../exemples/07_indexation_rtti.cpp), puis
  [simuler plusieurs fiches RTTI](../exemples/08_simulation_fiches_rtti.cpp) après
  l'explication des tables.

Chaque exemple retourne un code d'échec si son résultat est incorrect.
Pour les lancer, voir [construire et tester](construction.md).

## Héritage et ordre de spécialisation

Une fonction prenant `const Mammifere&` peut traiter un `Chat` dérivé de
`Mammifere`. Si une fonction prenant `const Chat&` existe, elle est plus précise.

Pour deux arguments, une spécialisation doit être au moins aussi précise
sur **chaque** position et strictement plus précise sur au moins une position.
L'ordre de déclaration ne sert jamais à départager les fonctions.

Par exemple, `(Chien, Animal)` et `(Animal, Chien)` sont incomparables pour
`(Chien, Chien)`. L'ajout d'une fonction `(Chien, Chien)` résout l'ambiguïté.
Sans elle, la construction de la méthode échoue à la compilation, même si elle
n'est jamais appelée. Voir le [programme volontairement invalide](../tests/refus/ambiguite_double.cpp).

Les deux domaines peuvent avoir des tailles et des racines différentes,
par exemple `Animal × Support`. Le dispatch n'est pas automatiquement
symétrique : `Chien × Chat` et `Chat × Chien` désignent deux cases distinctes.

## Transmettre des arguments ordinaires

Un contexte, un montant ou un flux peut accompagner les objets polymorphes.
Indiquer `argument_ordinaire` à chaque position qui ne participe pas au dispatch :

Cet extrait reprend les classes, `animal` et `using namespace mini_openmethod`
du premier exemple. Ajouter l'en-tête `<iostream>` pour le flux de sortie.

```cpp
auto facturer = creer_methode<void(const Animal&, double, std::ostream&)>(
    domaines<liste_types<Chien, Chat>, argument_ordinaire, argument_ordinaire>{},
    [](const Chien&, double montant, std::ostream& sortie) { sortie << "Chien : " << montant; },
    [](const Chat&, double montant, std::ostream& sortie) { sortie << "Chat : " << montant; });

facturer(animal, 12.5, std::cout);
```

Voir l'[exemple autonome](../exemples/05_arguments_ordinaires.cpp).
Les paramètres ordinaires peuvent précéder, séparer ou suivre les positions
polymorphes. Ils conservent exactement leur type dans chaque spécialisation :
valeur, `T&`, `const T&` ou `T&&`. Leur valeur et leur éventuel type dynamique
ne changent pas la sélection. Les règles détaillées de copie et de déplacement
sont regroupées dans la [référence API](reference.md).

## Modifier les objets polymorphes

Déclarer une position en `Classe&` pour autoriser sa modification, et en
`const Classe&` pour un accès en lecture seule. Par exemple, avec un membre
`energie` dans la hiérarchie `Animal` :

```cpp
auto nourrir = creer_methode<void(Animal&, int)>(
    domaines<liste_types<Chien, Chat>, argument_ordinaire>{},
    [](Chien& chien, int quantite) { chien.energie += 2 * quantite; },
    [](Chat& chat, int quantite) { chat.energie += quantite; });

nourrir(chien, 3);
```

Les spécialisations modifient l'objet d'origine, sans copie. Chaque position
garde exactement sa qualification `const` dans toutes les spécialisations :
`Animal&` devient `Chien&`, `const Animal&` devient `const Chien&`.
Une méthode peut mélanger les deux, par exemple `void(Animal&, const Support&)`.
Un objet constant ne peut pas être passé à une position modifiable.
Voir l'[exemple autonome](../exemples/06_objets_modifiables.cpp).

## Réutiliser une référence préparée

Quand le même objet est traité plusieurs fois, son type peut être validé une
seule fois. En reprenant `parler` et `chien` du premier exemple :

```cpp
auto chien_prepare = parler.preparer(chien);
auto resultat = parler(chien_prepare);
```

`preparer(objet)` signifie `preparer<0>(objet)`. Les positions commencent à zéro
et désignent les arguments de la **signature complète**.

Avec `rencontrer` de l'exemple de double dispatch, préparer chaque position séparément :

```cpp
auto gauche = rencontrer.preparer<0>(chien);
auto droite = rencontrer.preparer<1>(chat);
auto resultat = rencontrer(gauche, droite);
```

Un appel utilise soit tous les objets polymorphes bruts, soit toutes leurs
références préparées. Les arguments ordinaires restent transmis normalement.
Avec `facturer` et `animal` définis plus haut :

```cpp
auto animal_prepare = facturer.preparer(animal);
facturer(animal_prepare, 18.0, std::cout);
```

Si la signature commence par un argument ordinaire, par exemple
`void(Contexte&, const Animal&)`, préparer l'animal avec `preparer<1>(animal)`.
L'appel sans indice viserait le contexte en position 0 et serait refusé.

`preparer` rejette les types inconnus et refuse les objets temporaires à la
compilation. L'appel préparé réutilise directement les indices. La référence
ne possède pas l'objet : celui-ci doit rester vivant à la même adresse, avec
le même type dynamique. Un remplacement de l'objet exige une nouvelle préparation.
Les changements de ses données restent visibles.

La préparation peut être utile pour les appels répétés ; elle n'est pas
systématiquement plus rapide sur les petits domaines. La [référence API](reference.md)
précise les règles de copie, de conversion et de réutilisation entre méthodes.

## Domaine fermé et erreurs

Les listes déclarent les types dynamiques exacts acceptés. Une spécialisation
peut viser une classe intermédiaire absente des listes, mais un descendant non
listé est refusé, même si une fonction de sa base pourrait le traiter.

Une combinaison déclarée sans spécialisation, ou ambiguë, est rejetée à la
compilation. Un type absent de la liste provoque `type_inconnu` pendant l'appel
ou la préparation. Ajouter un type exige de recompiler la composition ; il n'y
a pas de registre extensible à l'exécution.

Pour les autres contraintes de signature ou un message du compilateur,
consulter la [référence et les corrections proposées](reference.md#comprendre-une-erreur).

**Suite :** [comprendre la construction et l'utilisation des tables](tables_dispatch.md).
Le [guide de compilation](construction.md) explique aussi l'intégration dans un autre projet.
