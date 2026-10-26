# Référence de l'API et des erreurs

[Sommaire](README.md) · [Guide d'utilisation](utilisation.md)

Cette page sert à retrouver une règle précise. Pour apprendre par l'exemple,
commencer par le guide d'utilisation. Tous les noms ci-dessous appartiennent
à `mini_openmethod` ; inclure `<mini_openmethod/methode.hpp>` suffit pour l'API.
Les noms de `mini_openmethod::detail` restent internes.

## Repères de l'API publique

| Élément | Rôle |
| --- | --- |
| `liste_types<Types...>` | Liste ordonnée des types dynamiques exacts acceptés à une position |
| `argument_ordinaire` | Marque une position transmise aux fonctions sans participer au dispatch |
| `domaines<Descripteurs...>` | Associe un descripteur à chaque argument de la signature |
| `creer_methode<Signature>(domaines<...>{}, fonctions...)` | Construit une méthode en déduisant les types des fonctions |
| `methode<Signature, Domaines, Fonctions...>` | Type de la méthode ; son constructeur reçoit les fonctions par valeur |
| `operation(arguments...)` | Appelle la spécialisation choisie, sur objets bruts ou références préparées |
| `operation.preparer<Position>(objet)` | Valide le type et retourne une référence préparée pour cette position |
| `reference_preparee<Base, Liste, Modifiable>` | Référence non propriétaire conservant l'objet et son indice validé |
| `reference.objet()` | Donne accès à l'objet d'origine, avec les droits de la référence préparée |
| `type_inconnu` | Exception signalant un type dynamique absent du domaine |
| `reflexion_active` | Booléen constant indiquant le générateur de tables sélectionné |

## Décrire la signature et les domaines

`Signature` est la signature complète, par exemple
`int(Contexte&, const Animal&, double, const Support&)`.
Il faut exactement un descripteur par argument, dans le même ordre :

```cpp
using Description = domaines<argument_ordinaire, Animaux, argument_ordinaire, Supports>;
```

`Animaux` et `Supports` sont des alias de `liste_types<...>`.

- Il faut **exactement une ou deux positions dispatchées**, désignées par des
  listes de types. Zéro ou trois positions dispatchées sont refusées.
- Le nombre d'arguments ordinaires n'est pas limité par cette règle. Un objet
  polymorphe peut lui-même être un argument ordinaire : son type réel ne
  participe alors pas au choix de la fonction.
- Une position dispatchée prend `Base&` ou `const Base&`. Sa racine doit être
  polymorphe. Les pointeurs, valeurs, références `&&` et références `volatile`
  ne sont pas acceptés à ces positions.
- Chaque liste est non vide, sans doublons et contient des classes sans
  qualification `const`/`volatile` ni référence. Chaque type doit être identique
  à la racine ou en dériver publiquement et sans ambiguïté.
- L'ordre de la liste fixe les indices. Deux listes permutées ne sont pas
  interchangeables pour les références préparées.

Les listes décrivent un **domaine fermé** : un descendant non listé reste
inconnu. Une spécialisation peut toutefois viser une classe intermédiaire
absente des listes. Il n'y a ni découverte automatique des classes, ni registre
extensible à l'exécution, ni greffons, ni désenregistrement, ni `next`.
Les tables couvrent toutes les combinaisons déclarées, sans optimisation
particulière pour des domaines clairsemés.

## Construire et appeler une méthode

Préférer `creer_methode<Signature>(description, fonctions...)` à l'écriture
explicite de `methode<Signature, Domaines, Fonctions...>` : la fabrique déduit
les types des fonctions et les conserve par valeur, sans qualification ni référence.
Il faut fournir au moins une spécialisation.

Chaque spécialisation respecte les règles suivantes :

| Partie de la signature | Règle |
| --- | --- |
| Nombre de paramètres | Identique à celui de `Signature` |
| Retour | Exactement le retour annoncé : `void`, valeur ou référence ; aucune conversion ou covariance implicite |
| Position dispatchée | Référence à la racine ou à un dérivé public non ambigu, avec la même qualification `const` |
| Position ordinaire | Exactement le même type, y compris la forme par valeur, `&`, `const &` ou `&&` |
| Appelable | Signature unique et analysable : lambda à paramètres explicites, foncteur compatible ou pointeur de fonction |

Les captures, lambdas `mutable` et fonctions `noexcept` sont acceptées.
Les lambdas génériques, ensembles surchargés non désambiguïsés, fonctions
variadiques et opérateurs d'appel qualifiés `volatile`, `&` ou `&&` ne le sont pas.
Une lambda à signature explicite permet d'adapter un traitement à cette API.
Un pointeur de fonction membre fourni seul est également refusé : il manque
l'objet receveur. Capturer cet objet dans une lambda, par exemple :

```cpp
Traitement traitement;
auto operation = creer_methode<int(const Animal&)>(
    domaines<liste_types<Chien>>{},
    [&traitement](const Chien& chien) { return traitement.executer(chien); });
```

Cet extrait suppose que `Traitement::executer(const Chien&)` retourne `int`.
L'objet `traitement` doit rester vivant pendant les appels.

Toutes les combinaisons du domaine doivent avoir une unique spécialisation
maximale. Une fonction domine une autre si ses paramètres dispatchés sont au
moins aussi précis à chaque position et strictement plus précis à l'une d'elles.
Une égalité restante, y compris entre deux signatures identiques maximales,
est une ambiguïté. L'ordre de déclaration ne départage jamais les fonctions.
La construction valide ces règles même si la méthode n'est jamais appelée.

`operation(arguments...)` retourne le résultat de la fonction retenue. L'appel
est **non `const`**, pour permettre les captures mutables, et n'est pas déclaré
`noexcept`. Les exceptions des fonctions utilisateur sont propagées sans modification.
Les conversions C++ habituelles restent possibles à l'entrée pour les arguments
ordinaires, par exemple `int` vers `double`. L'égalité stricte des types concerne
les signatures des spécialisations.

Les arguments ordinaires par référence conservent leur identité et leur catégorie.
Les valeurs sont transmises par déplacement lorsque possible, sinon par copie ;
les ressources seulement déplaçables et les valeurs copiables dont le déplacement
est explicitement supprimé sont acceptées.

Les fonctions sont possédées par la méthode ; une fonction seulement déplaçable
doit être transférée, par exemple avec `std::move`. La possibilité de copier ou
déplacer une méthode dépend de ses fonctions. Les objets et captures par
référence restent à la charge de l'appelant. Le partage entre threads exige
des traitements et données adaptés, ou une synchronisation externe.

## Préparer un objet

```cpp
auto reference = operation.preparer<Position>(objet);
```

- `Position` désigne la position dans la **signature complète**, à partir de zéro.
  `preparer(objet)` équivaut à `preparer<0>(objet)`.
- La position doit exister et avoir un descripteur `liste_types<...>`.
- L'objet doit être une lvalue compatible avec la racine et ses droits `const`.
  Les temporaires sont refusés, même pour une position en lecture seule.
- Le type dynamique exact est validé immédiatement ; s'il est absent de la
  liste, `type_inconnu` est levée. Aucune spécialisation n'est appelée.
- `preparer` est une fonction membre `const` : une méthode constante peut
  préparer un objet, même si elle ne peut pas effectuer l'appel de dispatch.

Pour une signature `(Contexte&, const Animal&)` décrite par
`domaines<argument_ordinaire, Animaux>`, utiliser `preparer<1>(animal)`.
La forme sans indice tenterait de préparer la position ordinaire 0.

L'appel reçoit soit tous les objets dispatchés bruts, soit toutes leurs
références préparées. Les arguments ordinaires restent présents et inchangés.
Mélanger un objet brut et une référence préparée dans un double dispatch n'est
pas une forme d'appel prise en charge.

## Utiliser une référence préparée

Le type retourné dépend de la position de la méthode :

| Position | Type préparé | Retour de `objet()` |
| --- | --- | --- |
| `const Base&` | `reference_preparee<Base, Liste>` | `const Base&` |
| `Base&` | `reference_preparee<Base, Liste, true>` | `Base&` |

`Modifiable` vaut `false` par défaut. La référence préparée est copiable et
assignable par copie. Elle n'a ni construction par défaut, ni constructeur
public depuis un objet et un indice arbitraire ; son indice est privé.

Une référence modifiable se convertit implicitement vers la référence constante
de même racine et de même liste ordonnée. La conversion inverse est interdite.
Une référence préparée elle-même déclarée `const` conserve ses droits :
`const reference_preparee<Base, Liste, true>` permet encore de modifier l'objet.

`objet()` est un accesseur `const noexcept` qui restitue la référence à la
racine, sans identification RTTI ni vérification de durée de vie.
Il ne retourne pas automatiquement une référence au type dérivé.

La référence préparée ne possède pas l'objet. Celui-ci doit rester vivant à
la même adresse et conserver son type dynamique. Son remplacement ou son
déplacement physique demande une nouvelle préparation. Modifier ses données
reste permis ; ces changements sont visibles lors des appels suivants.

Une référence peut servir à une autre méthode compatible avec sa racine,
sa liste ordonnée et ses droits d'accès. Elle n'est pas liée aux captures ni
à l'adresse de la méthode initiale et reste utilisable après déplacement de
celle-ci. Les éventuels ajustements d'héritage virtuel restent nécessaires ;
la préparation ne supprime pas tous les usages possibles de RTTI.

## Comprendre une erreur

Les messages ci-dessous sont des fragments des diagnostics du projet. Les
compléments dépendent du compilateur. Les programmes liés sont volontairement
incorrects et sont compilés séparément par les tests de refus.

| Diagnostic ou situation | Cause | Correction minimale | Exemple |
| --- | --- | --- | --- |
| `Un descripteur par argument est requis` | Le nombre de descripteurs diffère de la signature | Ajouter une liste ou `argument_ordinaire` pour chaque position | [Descripteurs manquants](../tests/refus/descripteurs_nombre.cpp) |
| `Descripteur invalide` | Un descripteur n'est ni une liste ni le marqueur ordinaire | Utiliser `liste_types<...>` ou `argument_ordinaire` | [Descripteur incorrect](../tests/refus/descripteur_invalide.cpp) |
| `Une ou deux positions polymorphes sont requises` | Zéro ou plus de deux listes de types | Garder une ou deux positions dispatchées ; marquer les autres ordinaires si leur type ne doit pas sélectionner la fonction | [Zéro position](../tests/refus/sans_polymorphe.cpp), [trois positions](../tests/refus/trois_polymorphes.cpp) |
| `La signature exige des references lvalue non volatiles` | Position dispatchée par valeur, pointeur, `&&` ou `volatile` | Employer `Base&` ou `const Base&` | [Pointeur](../tests/refus/polymorphe_pointeur.cpp) |
| `Les bases doivent etre polymorphes` | La racine n'a aucune fonction virtuelle | Utiliser une racine polymorphe, par exemple avec un destructeur virtuel | [Racine non polymorphe](../tests/refus/base_non_polymorphe.cpp) |
| `Domaine invalide` | Liste vide, doublon, type qualifié ou héritage incompatible | Donner une liste de classes uniques, sans qualification, compatibles avec la racine | [Doublon de type](../tests/refus/domaine_duplique.cpp) |
| `Une signature explicite` | Lambda générique, surcharge non résolue, fonction variadique ou opérateur qualifié non admis | Adapter le traitement avec une lambda à signature explicite | [Lambda générique](../tests/refus/lambda_generique.cpp), [opérateur qualifié](../tests/refus/foncteur_qualifie.cpp), [fonction variadique](../tests/refus/fonction_variadique.cpp) |
| `Un pointeur de fonction membre doit etre adapte` | Pointeur de membre fourni sans objet receveur | Capturer l'objet dans une lambda et y appeler le membre | [Pointeur de membre](../tests/refus/pointeur_membre.cpp) |
| `Specialisation invalide` — retour ou nombre de paramètres | Signature différente de celle annoncée | Fournir le retour exact et tous les paramètres | [Retour incorrect](../tests/refus/retour_incorrect.cpp), [paramètre manquant](../tests/refus/ordinaire_arite.cpp) |
| `Specialisation invalide` — argument ordinaire | Type ou référence de la spécialisation différent du contrat | Reprendre exactement le type annoncé | [Type ordinaire](../tests/refus/ordinaire_type.cpp), [référence ordinaire](../tests/refus/ordinaire_reference.cpp) |
| `Specialisation invalide` — qualification de l'objet | Référence spécialisée avec une qualification `const` différente | Conserver `Base& → Derive&` ou `const Base& → const Derive&` | [Constante vers modifiable](../tests/refus/constante_specialisation_modifiable.cpp), [modifiable vers constante](../tests/refus/modifiable_specialisation_constante.cpp) |
| `Combinaison sans specialisation applicable` | Une case du domaine n'a pas de traitement | Ajouter une spécialisation qui la couvre, éventuellement générale | [Combinaison non couverte](../tests/refus/sans_specialisation.cpp) |
| `Ambiguite : plusieurs specialisations maximales` | Fonctions incomparables, ou signatures identiques maximales | Ajouter l'intersection plus précise, ou supprimer le doublon | [Ambiguïté double](../tests/refus/ambiguite_double.cpp), [doublon de fonction](../tests/refus/doublon.cpp) |
| `La preparation exige un objet persistant` | Préparation d'un temporaire | Conserver d'abord l'objet dans une variable vivante pendant les appels | [Temporaire](../tests/refus/preparation_temporaire.cpp) |
| `Position de preparation hors de la signature` | Indice trop grand | Compter les arguments depuis zéro dans la signature complète | [Position incorrecte](../tests/refus/preparation_position.cpp) |
| `Une position ordinaire ne peut pas etre preparee` | La position choisie porte `argument_ordinaire` | Préparer une position dispatchée | [Position ordinaire](../tests/refus/preparation_ordinaire.cpp) |
| `Objet incompatible avec la racine ou la qualification const` | Objet constant pour une position modifiable, ou racine incompatible | Utiliser un objet compatible avec cette position | [Objet constant](../tests/refus/preparation_constante_modifiable.cpp) |
| Aucune surcharge d'appel compatible, notamment `C2665` sous MSVC | Objet constant ou référence préparée en lecture seule transmis à une position modifiable ; autres arguments incompatibles | Respecter les types et droits annoncés, ainsi que la forme tout brut ou tout préparé | [Appel constant](../tests/refus/appel_constant_modifiable.cpp), [appel préparé constant](../tests/refus/appel_prepare_constant_modifiable.cpp) |

Sans aucune spécialisation, le diagnostic est `Au moins une specialisation est requise`.
Ce cas se corrige en fournissant au moins un traitement lors de la construction.

Exemple de retour exact : une lambda qui renvoie `"ouaf"` déduit normalement
`const char*`. Pour une méthode annonçant `std::string_view`, écrire :

```cpp
[](const Chien&) -> std::string_view { return "ouaf"; }
```

À l'exécution, `type_inconnu`, dérivée de `std::runtime_error`, signale un type
absent de la liste pendant l'appel brut ou `preparer`. Son message est
`Type dynamique absent du domaine ferme`. Vérifier le type exact reçu ; si ce
type doit être accepté, l'ajouter au domaine et couvrir ses combinaisons avant
de recompiler. Les exceptions du traitement utilisateur restent distinctes.

L'utilisation d'une référence préparée dont l'objet n'est plus valide n'est
pas contrôlée et ne produit pas une erreur `type_inconnu` garantie.
Voir les [cas d'exécution et la simulation RTTI](exemples_indexation_rtti.md).

## Choisir le générateur de tables

`reflexion_active` est une constante `bool`, utilisable dans un `static_assert`
ou un `if constexpr`. Elle vaut `true` lorsque les tables sont construites par
réflexion C++26, `false` pour le générateur C++23. Le contrat public reste le même.

L'option CMake `MINI_OPENMETHOD_REFLEXION` utilise `AUTO`, `ON` ou `OFF` ; la
définition C++ du même nom utilise `0` ou `1`. Ne pas confondre ces deux réglages.
Conserver un mode cohérent entre les unités de compilation. Les capacités du
compilateur doivent être activées avant l'inclusion des en-têtes.
Voir le [guide de construction](construction.md#choisir-c23-ou-la-réflexion-c26)
pour la détection, les dépendances et l'intégration dans un autre projet.
