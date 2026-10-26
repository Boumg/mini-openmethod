# Documentation de mini-openmethod

[Présentation du projet](../README.md)

mini-openmethod choisit une fonction selon le type réel d'un ou de deux objets.
Pour une première lecture, il suffit de connaître les références C++, l'héritage
et les lambdas. Les détails de compilation et de réflexion peuvent attendre.

## Découvrir

1. Lire le [premier exemple](../README.md#premier-exemple) : un `Chien` ou un
   `Chat` est transmis par une référence `Animal`.
2. Suivre le [guide d'utilisation](utilisation.md) : héritage, double dispatch,
   puis arguments ordinaires, modification et préparation selon les besoins.
3. [Construire et lancer les exemples](construction.md) pour essayer le code.

Pour comprendre l'intérêt des opérations externes, lire la
[comparaison avec Visitor](comparaison_visitor.md) : ajouter des traitements
sans modifier les classes, avec un exemple de chaque approche.

## Comprendre ce qui se passe

4. [Comprendre les tables](tables_dispatch.md) : leur construction et leur
   utilisation, avec les schémas et une matrice `Chien / Chat`.
5. [Retrouver un type avec l'index RTTI](exemples_indexation_rtti.md) : recherche,
   hachage, collisions, fiches multiples et erreurs, avec du code exécutable.

## Approfondir au besoin

- [Référence API](reference.md) : syntaxe, préconditions, retours et limites de
  chaque élément public. Pour résoudre un problème, aller directement à
  [Comprendre une erreur](reference.md#comprendre-une-erreur).
- [Architecture et choix de conception](architecture.md) : organisation du code,
  règles de résolution, variantes C++23/C++26 et validation.
- [Comparaison avec les appels C++](comparaison_appels_cpp.md) : appels directs,
  virtuels et mini-openmethod ; commencer ici pour les performances.
- [Comparaison avec Boost.OpenMethod](comparaison_boost.md) et
  [détails des mesures](performances.md) : protocoles, optimisations et résultats datés.

Les pages de mesures renvoient à leurs résultats bruts. Les consulter ne demande
pas de relancer les chronométrages.

## Voir la suite du projet

La [feuille de route](evolutions.md) présente les évolutions prévues, avec des
exemples : références préparées depuis un domaine, héritage de domaines,
fonctions membres, préparation et appels constants avec un indice intégré aux
classes. Elle précise l'origine des propositions et distingue ces priorités des
pistes complémentaires et de l'API disponible aujourd'hui.
