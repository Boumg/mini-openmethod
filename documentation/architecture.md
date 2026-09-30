# Architecture et choix de conception

## Séparer les trois moments

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
est identique. Cette relation est évaluée avec `std::derived_from`.

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

## Pourquoi RTTI et dynamic_cast ?

Les traits ne découvrent pas le type réel derrière une référence de base.
Le prototype utilise `typeid` pour cette identification, puis exige une égalité
exacte avec un type déclaré. Accepter silencieusement un descendant non listé
invaliderait le caractère exhaustif de la vérification.

Une conversion inverse par `static_cast` ne couvre pas les bases virtuelles.
Le relais utilise `dynamic_cast`, qui ajuste les références dans les héritages
multiples et virtuels autorisés par le contrat. Les conversions sont vérifiées
au niveau des types pendant la résolution et réalisées sur les objets à l'appel.

L'absence de candidat est une erreur de compilation. Le type inconnu est une
erreur d'exécution distincte. Les exceptions des traitements utilisateur sont
propagées ; elles ne sont pas transformées en erreurs du moteur.

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

Les tests d'exécution couvrent le choix dynamique, les replis, l'indépendance
de l'ordre, toute la matrice de dispatch, les types inconnus dans chaque position,
les racines différentes, les diamants virtuels, les captures mobiles/mutables,
les retours void et référence, et les exceptions.

Les tests de compilation refusée contrôlent un fragment précis du diagnostic.
Ils ne peuvent pas réussir simplement parce que l'en-tête est absent ou qu'une
erreur de syntaxe empêche toute compilation. Les cas d'ambiguïté n'appellent pas
la méthode, afin de vérifier que la construction suffit à valider le domaine.

La CI fournit une matrice GCC, Clang et MSVC. Les vérifications fonctionnelles
restent actives avec NDEBUG ; elles n'utilisent pas assert.
