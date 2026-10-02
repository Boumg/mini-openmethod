# Architecture et choix de conception

## Organisation des en-têtes

- `mini_openmethod/domaines.hpp` déclare `liste_types`, `argument_ordinaire`, `domaines` et l'exception
  `type_inconnu`.
- `mini_openmethod/detail/traits.hpp` regroupe les assistants internes :
  les concepts de signature, `types_uniques` et `traits_liste`.
  Il inclut `domaines.hpp` et `detail/traits_fonction.hpp`.
- `mini_openmethod/detail/traits_fonction.hpp` analyse les signatures avec
  Boost.CallableTraits, dans les deux modes.
- `mini_openmethod/detail/arguments.hpp` valide les descripteurs, distingue les
  paramètres ordinaires et projette les signatures sur les positions polymorphes.
- `mini_openmethod/detail/index_types.hpp` fournit les recherches RTTI linéaire
  et hachée, avec validation du type exact.
- `mini_openmethod/reference_preparee.hpp` décrit une référence non propriétaire
  et son indice déjà validé, liés à une racine et une liste ordonnée de types.
- `mini_openmethod/configuration.hpp` détecte les capacités activées et expose
  `reflexion_active` ; une définition explicite permet de forcer le choix.
- `mini_openmethod/detail/resolution_cpp23.hpp` et `resolution_cpp26.hpp`
  calculent les indices des spécialisations gagnantes. `resolution.hpp`
  sélectionne l'un des deux générateurs.
- `mini_openmethod/detail/selection.hpp` choisit le candidat maximal unique
  parmi les seules spécialisations applicables, dans les deux modes.
- `mini_openmethod/methode.hpp` valide le contrat, transforme les indices en
  pointeurs de relais et gère l'appel. Il fournit aussi la fabrique `creer_methode`.

Les en-têtes publics peuvent être inclus seuls. L'inclusion de `methode.hpp`
reste suffisante pour utiliser toute l'API publique ; les noms de `detail`
restent internes. Seul `resolution_cpp26.hpp` exige directement une chaîne
compatible réflexion ; la sélection évite son inclusion en C++23.

## Arguments ordinaires : choix d'API et comparaison avec Boost

L'[issue #2](https://github.com/Boumg/mini-openmethod/issues/2) introduit les
paramètres transmis aux traitements sans participer à leur sélection :
contexte, montant, flux ou ressource possédée. Chaque position de la signature
possède exactement un descripteur dans `domaines<...>` :

- `liste_types<...>` désigne une position polymorphe, toujours `const Classe&` ;
- `argument_ordinaire` conserve le type annoncé, valeur ou référence, sans
  indexation RTTI ni conversion vers une classe spécialisée.

Par exemple, `int(Contexte&, const Animal&, double, const Support&)` emploie
`domaines<argument_ordinaire, Animaux, argument_ordinaire, Supports>`.
Les seules coordonnées de la table sont les positions 1 et 3. Le contexte et
le montant ne créent aucune dimension supplémentaire, même si le contexte est
lui-même polymorphe.

| Choix | mini-openmethod | Boost.OpenMethod 1.92 |
| --- | --- | --- |
| Positions participant au dispatch | Listes de types dans `domaines` ; marqueur explicite pour chaque argument ordinaire | `virtual_<T>` ou `virtual_ptr<T>` dans la signature ; paramètres ordinaires sans marqueur |
| Description de la signature | Signature C++ inchangée, descripteurs séparés | Décorateurs dans la signature de `method`, retirés pour l'appel utilisateur |
| Construction des tables | Résolution à la compilation sur les domaines fermés | Enregistrements de classes et de spécialisations, puis `initialize()` construit les tables |
| Appel | Indexer les seules positions polymorphes, choisir le relais et transmettre tous les arguments | Résoudre sur les paramètres virtuels, puis appeler la fonction avec tous les paramètres |
| Réutilisation d'une identification | `reference_preparee`, liée à la racine et à la liste ordonnée | `virtual_ptr`, associé au registre |
| Extensibilité | Recompilation de la composition | Modèle d'enregistrement plus général ; pas la même garantie statique sur un domaine fermé |

Cette comparaison porte sur l'[API sans macros de Boost.OpenMethod](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/core_api.html)
et ses [signatures mixtes](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/basics.html).
Les macros ne sont donc pas un critère différenciant. Notre choix préserve les
anciens appels à `creer_methode` et les domaines explicites. En contrepartie,
une signature contenant beaucoup d'arguments ordinaires répète davantage de
marqueurs que celle de Boost. La simplicité dépend de l'usage ; elle n'est pas
un avantage universel.

### Projection commune aux deux générateurs

`description_arguments` utilise Boost.Mp11 pour filtrer les indices des
positions polymorphes et leurs domaines. Les signatures des spécialisations
sont projetées sur ces positions, sous forme de types `void(Polymorphes...)`.
Le retour et la signature complète sont contrôlés auparavant ; le retour fictif
`void` n'intervient pas dans la sélection. Les deux générateurs existants
reçoivent ainsi exactement leur ancien format, sans modifier leur algorithme.
Ni l'applicabilité ni la dominance ne prennent en compte les arguments ordinaires.

L'arité totale sert à valider et transmettre les paramètres. L'arité de dispatch,
limitée à un ou deux, détermine les dimensions et l'aplatissement de la table.
Pour N × M types, celle-ci conserve N × M pointeurs, quel que soit le nombre
d'arguments ordinaires. La réflexion C++26 porte uniquement sur les signatures
projetées ; C++23 fournit la même sémantique.

Le type ordinaire d'une spécialisation doit être identique à celui de la
signature, références et qualifications comprises selon les règles des types
de fonctions C++. Les conversions habituelles restent possibles à l'entrée
de l'opérateur public, par exemple un entier vers un paramètre `double`.
Une spécialisation ne peut pas remplacer ce `double` par `int`.

### Transmission et références préparées

Pour une signature mixte, l'identification parcourt un tuple de références et
ne lit que les positions polymorphes. Sans argument ordinaire, un `if constexpr`
conserve l'accès direct aux paramètres : la projection par tuple dégradait
l'optimisation du petit double dispatch sous Clang. Ce choix n'ajoute aucune
branche à l'exécution. Les relais internes reçoivent les arguments par références et
`std::forward` conserve leur catégorie. Une référence mutable reste mutable,
une référence constante garde son identité, et une valeur seulement déplaçable
telle que `std::unique_ptr<T>` atteint la spécialisation sans copie.
Pour une classe passée par valeur depuis une lvalue, le contrat actuel comporte
une copie dans le paramètre public puis un déplacement dans le paramètre du
traitement ; les relais n'ajoutent aucun transfert.

`preparer<Position>` emploie la position absolue dans la signature :
`preparer<1>` et `preparer<3>` dans l'exemple précédent. Préparer une position
ordinaire est une erreur de compilation. L'appel préparé remplace toutes les
positions polymorphes par leurs références préparées ; les paramètres ordinaires
restent inchangés. Une référence préparée reste réutilisable entre méthodes
compatibles, indépendamment du placement de leurs paramètres ordinaires.

Le moteur n'ajoute aucune allocation, recherche RTTI ou dimension de table pour
ces paramètres. Leur transmission peut néanmoins modifier l'ABI, l'inlining et
le nombre de registres nécessaires. Ce constat architectural ne prouve donc
ni un coût d'appel nul, ni une égalité de performance avec Boost. Les
[mesures et leur protocole](comparaison_boost.md) séparent les anciens scénarios
et le nouveau scénario à arguments ordinaires.

## Deux générations de tables, une API commune

L'analyse des signatures est commune : `boost::callable_traits::return_type_t`
fournit le retour et `args_t` forme le tuple des paramètres. Les quelques
adaptateurs normalisent les pointeurs et opérateurs d'appel avant cette analyse.
Les contraintes du projet refusent toujours les variadiques et les fonctions
qualifiées `volatile`, `&` ou `&&`. En particulier, l'objet implicite d'un pointeur
de membre n'est pas ajouté aux paramètres. Voir la
[référence de Boost.CallableTraits](https://www.boost.org/doc/libs/latest/libs/callable_traits/doc/html/callable_traits/reference.html).

`types_uniques` est un alias de `boost::mp11::mp_is_set` : le contrôle récursif
interne est remplacé par l'[opération d'ensemble de Boost.Mp11](https://www.boost.org/doc/libs/latest/libs/mp11/doc/html/mp11.html).
Les autres traits déjà exprimés simplement avec `std` restent standards.

Le générateur C++23 utilise des `index_sequence`, des développements de
paramètres et `std::derived_from` pour évaluer l'applicabilité et la dominance.

Le générateur C++26 convertit les domaines et les signatures en collections de
`std::meta::info`. `dealias` enlève les alias avant `template_arguments_of`.
Les types des domaines et des paramètres sont ensuite normalisés et dédupliqués.
Une matrice mémorise la relation d'héritage public pour chaque paire de types
distincts. Les signatures conservent seulement les indices de ces types.
La compatibilité de chaque position est calculée une fois, puis réutilisée
dans toutes les cases du produit cartésien. Une position déjà incompatible
n'effectue pas la recherche aux positions suivantes.

La sélection commune compacte les candidats applicables. Elle rend directement
l'unique candidat lorsqu'il n'y en a qu'un ; sinon elle compare leur dominance.
La voie réfléchie utilise la matrice des types pour ces comparaisons, sans
construire une matrice de toutes les paires de fonctions. La voie C++23 conserve
sa matrice de dominance calculée par modèles. Les requêtes de réflexion ne sont
donc plus répétées pour chaque paire de fonctions et chaque case de dispatch.
Ces collections temporaires sont détruites avant la fin de l'évaluation
constante ; elles ne deviennent pas des données d'exécution.

Les générateurs renvoient le même format : un tableau d'indices, avec K pour
une absence de candidat et K + 1 pour une ambiguïté, K étant le nombre de
spécialisations. La couche commune vérifie ces sentinelles par `static_assert`
et produit une table `constexpr` de pointeurs de fonctions. Les diagnostics et
les règles de sélection sont ainsi partagés. La réflexion réduit le recours
aux développements de modèles ; elle ne promet pas une compilation plus rapide.

Les adaptateurs de pointeurs et de foncteurs restent communs. Pour un pointeur
de membre, la forme `Fonction Classe::*` extrait le type de fonction sans
énumérer chaque combinaison de qualifications. Dans les deux modes, les traits
reconnaissent également un type de fonction `noexcept` fourni directement.

La voie C++26 exclut explicitement les fonctions variadiques C, les fonctions
`volatile` et celles qualifiées par référence, afin de conserver les limites
de la voie C++23. La réflexion n'effectue aucune sélection dans un ensemble
surchargé et ne déduit pas la signature d'une lambda générique.

La sélection est protégée par le préprocesseur : la voie C++23 n'analyse aucune
syntaxe de réflexion et ne charge pas `<meta>`. Sans définition explicite,
`configuration.hpp` active la réflexion lorsque `__cpp_impl_reflection` et
`__cpp_lib_reflection` valent au moins `202506L`. Une définition de
`MINI_OPENMETHOD_REFLEXION` à 0 ou 1 force le choix ; 1 exige ces capacités.

CMake propose `AUTO` par défaut, `ON` pour exiger la réflexion et `OFF` pour
imposer les modèles. Sa sonde construit une véritable table de double dispatch
avec la bibliothèque, d'abord sans option puis avec `-freflection` sous GCC ou
Clang. L'échec de la sonde fait choisir C++23 en `AUTO`, mais arrête la
configuration en `ON`. Le test porte sur la chaîne complète, pas sur sa version.
Tous les fichiers d'un programme doivent employer le même réglage. La cible
CMake propage ce choix, le standard minimum et les options, y compris installée ;
un consommateur d'une installation ne refait pas cette sélection.

## Séparer les trois moments

Une fonction virtuelle C++ choisit son traitement selon l'objet receveur.
mini-openmethod conserve l'opération hors de la hiérarchie et peut sélectionner
sur deux objets. Un appel direct non virtuel suffit quand le traitement est
déjà connu statiquement. La [comparaison native conservée](comparaison_appels_cpp.md)
distingue ces contrats, la visibilité des corps et les coûts mesurés ; elle
ne nécessite pas un nouveau chronométrage à chaque modification.

La signature de l'opération, les listes de types et les types des lambdas sont
connus pendant la compilation. Les captures sont des valeurs fournies lors de
la construction de l'objet méthode. Les arguments et leurs types dynamiques
ne sont connus qu'au moment de l'appel.

La table est `static constexpr` pour chaque instanciation de méthode. Deux objets
de la même instanciation utilisent la même table mais conservent leurs propres
captures. Le pointeur de relais reçoit la méthode par référence : il retrouve
ainsi la bonne lambda, sans stocker un pointeur vers un objet qui pourrait être
déplacé.

La construction force l'instanciation de la résolution. Un programme ne peut
donc pas « cacher » une ambiguïté simplement en ne faisant aucun appel.

## Applicabilité et dominance

Pour un tuple de types dynamiques D et une spécialisation S, S est applicable
si chaque type Dᵢ dérive publiquement et sans ambiguïté du paramètre Sᵢ, ou lui
est identique. La version C++23 utilise `std::derived_from` ; la version C++26
combine `std::meta::is_base_of_type` et la convertibilité des pointeurs avec
`std::meta::is_convertible_type`. Tester seulement l'héritage accepterait à tort
des bases privées ou ambiguës.

A domine B si Aᵢ dérive de Bᵢ pour chaque position et qu'au moins une position
diffère. C'est un ordre partiel : certains candidats sont incomparables.
Une somme des profondeurs d'héritage ou une priorité à l'argument de gauche
produirait une autre sémantique et n'est pas employée.

La résolution compte les candidats applicables non dominés :

- zéro : combinaison non couverte, rejet à la compilation ;
- un : son indice remplit la table ;
- plusieurs : ambiguïté, rejet à la compilation.

Deux signatures identiques restent incomparables au sens strict ; si elles sont
maximales pour une combinaison, elles sont donc refusées. La vérification porte
sur les combinaisons du domaine déclaré, pas sur toutes les classes C++ qui
pourraient un jour être définies.

## Aplatir le produit cartésien

Avec deux domaines de tailles N et M, la case (i, j) correspond à i × M + j.
La construction parcourt les cases aplaties et retrouve leurs coordonnées
par division et modulo. Le runtime applique exactement la même numérotation.

Les tests utilisent aussi une matrice 3 × 2 avec deux racines différentes :
une matrice uniquement carrée pourrait masquer une erreur de diviseur.

## Identification dynamique et ajustement des références

Les traits ne découvrent pas le type réel derrière une référence de base.
Le prototype utilise `typeid` pour cette identification, puis exige une égalité
exacte avec un type déclaré. Accepter silencieusement un descendant non listé
invaliderait le caractère exhaustif de la vérification.

Après cette validation du type exact, le relais connaît une spécialisation
applicable. Il emploie `static_cast` lorsque cette conversion est bien formée,
y compris pour ajuster les décalages d'un héritage multiple non virtuel.
Cette conversion est sûre ici parce que le domaine et la résolution prouvent
l'existence d'un sous-objet cible public et non ambigu, et que l'indexation a
vérifié le type dynamique avant tout ajustement.

La conversion inverse depuis une base virtuelle ne permet pas `static_cast` :
le relais conserve alors `dynamic_cast`. Le choix est fait par `if constexpr`
pour chaque type de paramètre, sans branche supplémentaire à l'exécution.
Cette optimisation est commune aux deux versions. La réflexion ne change pas
le chemin d'appel ; les deux générateurs utilisent la même indexation.

L'absence de candidat est une erreur de compilation. Le type inconnu est une
erreur d'exécution distincte. Les exceptions des traitements utilisateur sont
propagées ; elles ne sont pas transformées en erreurs du moteur.

## Indexation et références préparées

Pour au plus 8 types, une expression OU à court-circuit recherche la première
adresse RTTI correspondante. Si aucune adresse ne correspond, une deuxième
recherche utilise `std::type_info::operator==`. Comparer seulement les adresses
ne suffirait pas : un même type peut avoir plusieurs objets RTTI.

Sous GCC uniquement, `[[gnu::always_inline]]` maintient cette petite recherche
visible dans l'appelant. Sans cette indication, GCC 16.2 produisait un dispatch
sensiblement plus lent sur le banc à deux types. Clang et MSVC conservent leur
choix d'inlining habituel. Le seuil de 8 types, le repli par égalité et le rejet
des inconnus restent identiques. Les comparaisons d'algorithmes doivent mesurer
l'appel complet en plus de la recherche isolée.

Au-delà de 8 types, `traits_liste` construit au premier accès un `index_types`
local statique constant, partagé par les méthodes utilisant la même liste.
L'initialisation est synchronisée par C++ ; les recherches ultérieures ne
modifient rien. Ce n'est pas un registre extensible. La table des spécialisations
reste calculée à la compilation, tandis que cet index auxiliaire est construit
à l'exécution car les adresses RTTI ne sont pas connues par `consteval`.

Un hachage multiplicatif des adresses choisit une case dans un tableau de taille
puissance de deux, au moins deux fois le nombre de types. Le sondage linéaire
résout les collisions en vérifiant l'adresse exacte de chaque candidat. Une
case vide déclenche le repli par égalité des types. Ainsi, une collision ou une
adresse RTTI inconnue ne peut pas sélectionner arbitrairement une spécialisation.
Le coût attendu est constant pour les adresses connues bien réparties ; les
collisions et le repli gardent un pire cas linéaire. Les tableaux n'allouent pas.
Le [banc d'indexation](performances.md#choix-du-seuil-dindexation) documente
le choix du seuil ; celui-ci n'est pas un optimum universel.

`methode::preparer<Position>(objet)` valide le type et construit une
`reference_preparee<Base, Liste>` contenant un pointeur et un indice privé.
L'appel préparé calcule directement la case de dispatch. Les ajustements
d'héritage restent ceux de l'appel ordinaire, notamment le `dynamic_cast` vers
une base virtuelle. La préparation n'appelle aucune spécialisation.

Le constructeur est privé et les temporaires sont refusés. L'identité de la
racine et l'ordre de la liste empêchent de transmettre un indice à une table
incompatible. Une référence ne dépend pas des captures ni de l'adresse de la
méthode : elle reste utilisable après déplacement de celle-ci et peut servir
à une autre méthode compatible. L'objet référencé doit rester vivant à la même
adresse et conserver son type dynamique ; le remplacer exige de préparer une
nouvelle référence. Il n'y a ni possession ni vérification de durée de vie.

## Pourquoi les signatures explicites ?

Le trait lit `decltype(&Fonction::operator())`. Une lambda non générique expose
une signature unique. Une lambda générique expose un modèle de fonction et
un foncteur surchargé en expose plusieurs : aucun choix canonique n'est disponible.

L'adaptateur conseillé est une lambda explicitement typée qui appelle ensuite
le code générique. Les retours doivent être exactement identiques, ce qui évite
les conversions inattendues et les références pendantes causées par une conversion
de retour automatique. Cela ne peut pas empêcher une lambda utilisateur de
renvoyer elle-même une référence déjà invalide.

## Frontière de l'ouverture

De nouveaux traitements peuvent être écrits dans des composants séparés, puis
assemblés au point de composition. Les classes du domaine n'ont pas besoin de
connaître ces traitements. En revanche, cette composition doit être recompilée
pour intégrer une nouvelle spécialisation ou un nouveau type.

Un registre ouvert serait un second moteur, avec des garanties différentes :
enregistrement explicite, résolution au chargement, erreurs d'ambiguïté tardives,
synchronisation, et règles de durée de vie pour le déchargement des greffons.
Il ne serait pas honnête de lui promettre la même vérification exhaustive
à la compilation lorsque les contributions futures sont inconnues.

## Stratégie de validation

Les tests d'exécution utilisent doctest, avec un point d'entrée commun dans
`lanceur_tests.cpp`. Chaque `TEST_CASE` porte un nom français et est découvert
automatiquement par CTest sous le préfixe `unitaire/`. Les assertions `CHECK`
et les contrôles d'exceptions remplacent les vérificateurs et compteurs maison ;
les coordonnées des matrices sont jointes aux échecs pour les situer précisément.
Les unités qui ne contiennent que des `static_assert` sont compilées dans le
même exécutable. Le programme de détection et le consommateur de l'installation
restent autonomes.

Les tests des assistants incluent directement les nouveaux en-têtes, sans
`methode.hpp`, et vérifient les signatures analysées et les contraintes des
domaines. Des inclusions répétées contrôlent aussi leurs gardes d'inclusion.
Le remplacement par Boost est couvert notamment par un membre sans argument
(son objet implicite doit rester exclu), les fonctions `const noexcept`, les
signatures variadiques ou qualifiées refusées et l'unicité de types avec leurs
qualifications exactes.

Les tests d'exécution couvrent le choix dynamique, les replis, l'indépendance
de l'ordre, toute la matrice de dispatch, les types inconnus dans chaque position,
les racines différentes, les héritages multiples et virtuels, les bases
intermédiaires privées ou ambiguës, les captures mobiles/mutables, les retours
void et référence, et les exceptions. Le rejet des types inconnus est vérifié
avant toute conversion vers une spécialisation.

Les mêmes règles sont contrôlées avec des références préparées, y compris les
retours, exceptions et captures après déplacement. Des assertions vérifient
l'impossibilité de fabriquer un indice ou de mélanger des domaines incompatibles.
`test_arguments.cpp` vérifie les positions ordinaires avant, entre et après
les objets polymorphes, une matrice 3 × 2, les références et ressources mobiles,
le nombre de copies/déplacements, les exceptions et la réutilisation des
références préparées entre signatures différentes. Un objet polymorphe ordinaire
absent des listes confirme que ces paramètres ne sont pas indexés.
Les descripteurs invalides, l'absence ou l'excès de positions polymorphes,
les types ordinaires incompatibles, les signatures incomplètes et la préparation
d'une position ordinaire ont chacun un refus de compilation dédié.
`test_indexation.cpp` couvre les tailles 1, 2, 4, 5, 8, 9 et 32 et force toutes les
adresses dans la même case pour tester collisions, retour à zéro et rejet des
inconnus. Les temporaires et positions de préparation invalides ont des tests
de compilation refusée avec leur diagnostic attendu.

`test_resolution.cpp` compare les indices à des tables attendues : replis,
matrice rectangulaire, permutation des fonctions, intersections, doublons,
absence de candidat et héritages inaccessibles. En mode réflexion, il compare
aussi directement les deux générateurs par `static_assert`.
En mode réflexion, `test_grand_domaine.cpp` construit 256 spécialisations pour
16 × 16 types, avec un argument ordinaire intercalé, et vérifie toutes les cases
avec des objets bruts et des références préparées,
ainsi que le rejet d'un descendant inconnu. Il compile sans augmentation des
limites `constexpr`. Le script GCC l'exécute avec `ON` et `AUTO` ; le contrôle
du mode attendu garantit que la sélection automatique exerce bien la réflexion.
`test_detection.cpp` inclut les en-têtes sans hériter de la définition de mode
de la cible CMake, pour contrôler leur propre sélection automatique.

Les tests de compilation refusée contrôlent un fragment précis du diagnostic.
Ils ne peuvent pas réussir simplement parce que l'en-tête est absent ou qu'une
erreur de syntaxe empêche toute compilation. Les cas d'ambiguïté n'appellent pas
la méthode, afin de vérifier que la construction suffit à valider le domaine.

La CI vérifie le repli `AUTO` avec GCC 13, Clang 18 et MSVC. Le conteneur GCC 16.2
exécute la même suite dans les modes `OFF`, `ON` et `AUTO`, avec contrôle du
choix attendu. Les refus des fonctions variadiques et des opérateurs qualifiés
par référence sont conservés. Le script contrôle aussi qu'un choix explicite 0
reste respecté lorsque le compilateur possède la réflexion activée.
Les vérifications fonctionnelles doctest restent actives avec `NDEBUG`.
Les [mesures de performance](performances.md) sont
indicatives et n'imposent aucun seuil de temps aux tests.

Lorsque `MINI_OPENMETHOD_COMPARAISON_BOOST=ON`, le test `equivalence_boost`
vérifie sept scénarios sur 4 096 entrées chacun, avec des résultats attendus
calculés indépendamment du dispatch. Le mode `--verifier` ne chronomètre pas.
Boost.OpenMethod est une dépendance du seul exécutable de comparaison, fournie
par la fonctionnalité facultative `comparaison-boost` du manifeste vcpkg.
Boost.CallableTraits et Boost.Mp11 sont des dépendances d'en-têtes obligatoires
de la bibliothèque et de sa cible exportée. `find_dependency` les retrouve lors
de la consommation d'une installation. La sonde de réflexion et les tests de
compilation refusée reçoivent les mêmes dépendances que les cibles ordinaires.
La CI et le conteneur les installent via le registre vcpkg fixé dans le manifeste.

doctest est réservé à la fonctionnalité facultative `tests`, activée avant
`project()` lorsque `MINI_OPENMETHOD_TESTS=ON`. Il est lié uniquement à
l'exécutable de tests et n'apparaît pas dans les dépendances de la cible exportée.
Les exemples, mesures et consommateurs de la bibliothèque n'en dépendent pas.
La migration du lanceur modifie le travail de compilation des tests : les anciens
temps de compilation du projet complet ne sont donc pas directement comparables.
