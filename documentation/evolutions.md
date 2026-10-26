# Évolutions prévues

[Sommaire](README.md) · [API actuelle](reference.md)

Cette feuille de route présente les directions souhaitées pour les prochaines
évolutions. **Ces fonctionnalités ne sont pas encore disponibles.** L'ordre
indique une priorité, sans fixer de version ni de date de livraison.

L'objectif reste une bibliothèque simple, d'en-têtes C++23, sans macro de
déclaration, avec des erreurs détectées aussi tôt que possible.

## Les évolutions prioritaires

| Ordre | Évolution | Utilité |
| --- | --- | --- |
| 1 | [Préparer les références depuis le domaine](#1-préparer-les-références-depuis-le-domaine) | Partager une préparation entre plusieurs méthodes compatibles. |
| 2 | [Hériter d'un domaine et de ses méthodes](#2-hériter-dun-domaine-et-de-ses-méthodes) | Enrichir un ensemble existant sans le recopier. |
| 3 | [Accepter directement les fonctions membres](#3-accepter-directement-les-fonctions-membres) | Appeler un membre sous la forme `methode(objet, arguments…)`. |
| 4 | [Préparation constante et indice dans les classes](#4-préparation-constante-et-indice-dans-les-classes) | Déplacer la préparation, et les appels admissibles, vers la compilation. |

Les exemples ci-dessous illustrent l'usage recherché. **Leur syntaxe est une
proposition ; ils ne compilent pas avec l'API actuelle.** Aujourd'hui,
`domaines<...>` décrit les domaines des arguments d'une méthode. Le domaine
envisagé ici pourrait aussi regrouper les méthodes et leur résolution.

## 1. Préparer les références depuis le domaine

**Origine : cette proposition vient de jll63, auteur de Boost.OpenMethod**,
dans [son commentaire de l'issue #6](https://github.com/Boumg/mini-openmethod/issues/6#issuecomment-6301317986).
Elle est reprise dans l'[issue #7](https://github.com/Boumg/mini-openmethod/issues/7).

### Usage recherché

Aujourd'hui, `nourrir.preparer(chien)` produit une référence qui garde l'adresse
de **ce chien** et son indice de type. Cette référence est déjà réutilisable par
une autre méthode compatible, comme `taquiner`. La proposition rendrait cette
indépendance visible : on préparerait l'objet depuis le domaine, sans avoir
besoin d'une méthode.

```cpp
// Proposition de syntaxe, pas encore disponible.
using Animaux = domaine<Chien, Chat>;
auto chien_prepare = Animaux::preparer(chien);
nourrir(chien_prepare);
taquiner(chien_prepare);
```

Chaque méthode utiliserait l'indice préparé pour consulter sa propre table.
En double dispatch, deux objets seraient préparés indépendamment puis combinés
dans les appels. La racine et les droits d'accès restent à exprimer dans l'API
finale ; une liste de types ne suffit pas à les choisir sans ambiguïté.

### Ce qui serait mémorisé

La référence conserverait l'objet et **son indice de type dans le domaine**.
Elle ne mémoriserait ni méthode, ni fonction sélectionnée, ni résultat d'appel.
L'indice d'un type et le numéro d'une spécialisation sont deux informations
différentes : `nourrir` et `taquiner` ont chacune leur propre résolution.

Les [tables de dispatch](tables_dispatch.md) sont **déjà partagées** entre les
instances d'un même type de méthode, et l'index RTTI entre les utilisations d'une
même liste. L'évolution concerne l'entrée de l'API de préparation.

### Garanties à conserver

- Identifier le type réel exact et refuser les types absents du domaine, même
  s'ils dérivent d'un type connu. Une référence `Chien&` peut désigner un `Loup`.
- Réutiliser la référence seulement avec la même racine, la même liste ordonnée
  de types et des qualifications compatibles. Préserver `const` et empêcher la
  fabrication d'un couple objet/indice arbitraire.
- Garder une référence non propriétaire : l'objet doit rester vivant, à la même
  adresse et avec le même type dynamique. La durée de vie d'une méthode ne doit
  pas conditionner celle de la référence ; les temporaires restent refusés.
- Conserver `methode::preparer<Position>` pour les usages existants, en lui
  faisant déléguer la préparation au mécanisme commun.

Cette réorganisation ne promet pas un appel plus rapide : les appels préparés
évitent déjà une nouvelle identification, et certaines conversions d'héritage
peuvent encore nécessiter RTTI.

## 2. Hériter d'un domaine et de ses méthodes

### Usage recherché

Créer un domaine enfant en reprenant les types, les méthodes et les
spécialisations d'un parent, puis ajouter seulement les différences :

| Domaine | Types | Méthodes |
| --- | --- | --- |
| `animaux` | `Chien`, `Chat` | `parler`, `nourrir` |
| `refuge`, dérivé de `animaux` | Les précédents + `Loup` | Les précédentes + `soigner`, avec une spécialisation de `parler` pour `Loup` |

Il s'agit d'un héritage **entre domaines**, distinct de l'héritage C++ entre
`Animal` et `Chien`, déjà utilisé pour choisir les spécialisations.

### Règles proposées

Le domaine parent conserverait son comportement. Le domaine enfant serait une
nouvelle composition fermée, validée à la compilation : ajouter `Loup` impose
de vérifier les nouvelles combinaisons de chaque méthode. Pour une méthode à
deux positions animales, cela inclut `Chien/Loup`, `Loup/Chat`, `Loup/Loup`, etc.
Les spécialisations générales héritées pourraient couvrir ces cas ; sinon,
la compilation signalerait les combinaisons manquantes ou ambiguës.

L'ajout d'une spécialisation plus précise et le remplacement d'une spécialisation
de même signature devraient être distingués explicitement. L'ordre des
déclarations ne devrait pas masquer une ambiguïté.

Pour commencer simplement : un seul parent, des méthodes identifiées sans
ambiguïté, et une règle explicite pour la copie ou le partage de leur état.
Les indices et références préparées du parent ne seraient pas transférés implicitement
à l'enfant, car la fonction retenue peut changer. L'héritage multiple de domaines
resterait une extension ultérieure si un besoin concret apparaît.

Cette composition resterait effectuée à la compilation. Elle n'introduirait
pas d'enregistrement de nouveaux types pendant l'exécution.

## 3. Accepter directement les fonctions membres

### Usage recherché

Éviter d'écrire une lambda d'adaptation pour chaque fonction membre. L'objet
receveur serait fourni à chaque appel :

```cpp
// Proposition de syntaxe, pas encore disponible.
auto afficher = creer_methode_membre(
    animaux, &Chien::afficher, &Chat::afficher);

afficher(chien);  // appelle chien.afficher()
afficher(chat);   // appelle chat.afficher()
afficher(animal); // choisit selon le type réel de l'Animal référencé
```

La syntaxe `operation(objet)` existe déjà pour les opérations externes.
La nouveauté serait l'acceptation directe de **pointeurs de fonctions membres**,
actuellement refusés par `creer_methode` sans adaptation.

### Règles proposées

Le receveur participerait au choix de la fonction comme les autres objets de
dispatch. Des arguments ordinaires pourraient suivre, par exemple
`afficher(animal, flux)`. L'adaptateur respecterait `const`, les qualifications
de référence et les ajustements d'adresse de l'héritage multiple ou virtuel.
Les surcharges de membres devraient être désignées sans ambiguïté.

Une première version pourrait se limiter aux objets persistants passés par
référence. Lier durablement un membre à un objet, ou utiliser un service séparé
comme `imprimante.afficher(animal)`, demande un contrat distinct pour identifier
le receveur, les arguments de dispatch et leurs durées de vie.

## 4. Préparation constante et indice dans les classes

### Usage recherché

Calculer la préparation à la compilation lorsque c'est possible, puis permettre
à une classe de porter directement son indice dans un domaine. L'appel de la
méthode préparée pourrait lui aussi être évalué à la compilation lorsque les
objets, les conversions et la spécialisation choisie le permettent.

La préparation constante est déjà envisagée dans l'[issue #7](https://github.com/Boumg/mini-openmethod/issues/7).
L'intégration de l'indice dans les classes et l'évaluation constante du traitement
prolongent cette piste. Il faut distinguer trois niveaux :

| Niveau | Calcul effectué à la compilation | Ce qui peut rester à l'exécution |
| --- | --- | --- |
| Indice constant | Position du type exact dans le domaine. | Création de l'objet, association à son adresse et appel. |
| Référence préparée constante | Construction du descripteur, avec une référence admissible en expression constante. | Appel de la méthode. |
| Appel constant | Préparation et exécution de la spécialisation sur des données admissibles. | Utilisation du résultat déjà calculé. |

Les tables actuelles sont déjà construites à la compilation. Cette évolution
étendrait l'évaluation constante à la préparation et, lorsque possible, à l'appel :

```cpp
// Proposition de syntaxe ; suppose un objet et une méthode compatibles constexpr.
constexpr auto chien_prepare = Animaux::preparer(chien_constant);
constexpr auto resultat = compter_pattes(chien_prepare);
static_assert(resultat == 4);
```

### Porter l'indice dans la classe

Une intégration volontaire pourrait ajouter un membre constant ou une base
auxiliaire à la classe. Exemple d'intention :

```cpp
// Proposition de syntaxe, pas encore disponible.
struct Chien final : Animal {
    static constexpr auto indice_animaux = Animaux::indice<Chien>();
};
```

L'indice serait calculé depuis le domaine, sans nombre écrit à la main. Un
membre `static constexpr` serait partagé par les objets de la classe, sans
ajouter un champ à chaque objet. En C++23, cette intégration demanderait une
déclaration volontaire dans la classe ou une base auxiliaire ; elle ne modifierait
pas automatiquement une classe existante. Le fonctionnement sur les classes
non modifiées resterait disponible.

### Règles à préciser

- **Identifier le domaine.** Une classe peut avoir des indices différents dans
  plusieurs domaines. L'indice porterait cette association ; un numéro de
  fonction resterait, lui, propre à une méthode et à sa composition.
- **Garantir le type exact.** Un membre statique n'identifie pas à lui seul un
  objet reçu comme `Animal&`. Définir un accès fiable à l'indice du type réel et
  empêcher un descendant non déclaré d'utiliser silencieusement celui de son
  parent. Un type `final` aide quand l'objet est reçu sous son type concret.
- **Conserver le chemin dynamique.** `constexpr` autorise l'évaluation constante
  sans l'imposer à chaque appel. Une entrée `consteval` éventuelle serait séparée
  des usages dynamiques. Un indice constant n'exige pas un objet constant.
- **Valider l'appel complet.** La préparation constante ne rend pas automatiquement
  le traitement constant. Vérifier l'état des fonctions, les durées de vie et les
  conversions par des `static_assert`, ainsi que les refus attendus à la compilation.

Les règles C++ des [fonctions constantes](https://eel.is/c++draft/dcl.constexpr),
des [expressions constantes](https://eel.is/c++draft/expr.const) et des
[membres statiques](https://eel.is/c++draft/class.static) encadrent ces possibilités.
Le coût du chemin dynamique et les éventuelles données ajoutées aux classes
devront être évalués avant de retenir une représentation.

## Autres évolutions proposées

Ces pistes s'appuient sur la documentation de **Boost.OpenMethod 1.92.0**.
Elles ne visent pas à reproduire toute son architecture.

### À privilégier ensuite

- **Trois positions de dispatch et davantage.** Permettre par exemple
  `interagir(animal, lieu, objet)` avec trois types réels participant au choix.
  Aujourd'hui, seules une ou deux positions sont acceptées. Conserver les
  contrôles de couverture et d'ambiguïté, en rendant visible le coût des tables :
  trois domaines de 10 types représentent 1 000 combinaisons.
  [Dispatch multiple dans Boost](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/multiple_dispatch.html).
- **Appeler la spécialisation moins précise suivante.** Par exemple, effectuer
  le traitement général d'un `Animal`, puis ajouter le comportement propre au
  `Chien`. Définir les cas sans suivant et les ambiguïtés ; détecter à la
  compilation les usages invalides lorsque possible.
  [Fonction `next` de Boost](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/reference/boost/openmethod/method-0db/next.html).
- **Améliorer les diagnostics.** Indiquer les types et la position en cause,
  puis les spécialisations candidates en cas d'ambiguïté. Garder le parcours
  courant simple avant d'ajouter une politique complète de gestion des erreurs.
  [Gestion des erreurs dans Boost](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/error_handling.html).

### À étudier selon les besoins

- **Préparation avec propriété de l'objet.** Proposer des adaptateurs facultatifs
  pour `unique_ptr` et `shared_ptr`, distincts des références empruntées actuelles.
  Définir les valeurs nulles, le déplacement et la durée de vie. Un appel ordinaire
  sur `*pointeur` reste déjà possible si l'objet est valide.
  [Pointeurs intelligents dans Boost](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/smart_pointers.html).
- **Identification des types personnalisable.** Accepter éventuellement des
  identifiants applicatifs en conservant des conversions sûres, notamment avec
  l'héritage virtuel. Garder RTTI comme fonctionnement par défaut.
  [RTTI personnalisé dans Boost](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/custom_rtti.html).
- **Registre extensible et bibliothèques partagées.** Traiter ce sujet comme un
  moteur séparé : identité des types entre modules, compatibilité binaire,
  reconstruction des tables, concurrence et invalidation lors du déchargement.
  Aucun support de greffons ne devrait être annoncé avant ce contrat. Boost 1.92
  documente notamment une limite Windows pour les registres partagés entre
  modules chargés dynamiquement.
  [Bibliothèques partagées dans Boost](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/shared_libraries.html).

L'héritage des domaines proposé plus haut n'est pas simplement une reprise
des registres Boost : leurs mécanismes `with` et `without` composent des
**politiques**. Ils ne décrivent pas l'héritage d'une collection de types et de
méthodes ; une classe utilisée dans plusieurs registres doit être enregistrée
dans chacun. [Registres et politiques de Boost](https://www.boost.org/doc/libs/1_92_0/libs/openmethod/doc/html/openmethod/registries_and_policies.html).

## Comment avancer sans alourdir la bibliothèque

Pour chaque étape, commencer par un petit exemple d'usage et fixer les règles
de validité avant d'ajouter l'API. Vérifier les cas réussis, les erreurs attendues
et la compatibilité avec les usages existants, puis mettre à jour le guide et
la référence. Les exemples ci-dessus deviendront exécutables seulement lorsque
leur fonctionnalité sera implémentée.

Mesurer les changements touchant le dispatch ou les tables sur des scénarios
comparables, en suivant le [protocole existant](comparaison_appels_cpp.md).
La compilation plus longue, la taille des tables et le coût de l'appel comptent
autant que le nombre de lignes économisées.
