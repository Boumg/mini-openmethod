# Simplicité et performance des deux versions

La comparaison avec Boost.OpenMethod, installé par vcpkg, dispose de son
[protocole et de ses résultats](comparaison_boost.md). Le tableau ci-dessous
conserve la mesure antérieure de l'optimisation des conversions.

L'API ne change pas selon le compilateur. La version C++26 parcourt des types
réfléchis dans des boucles `consteval` pour construire la table ; la version
C++23 emploie des développements de paramètres de modèles. Le chemin d'appel
est commun : recherche du type exact, lecture de la table, ajustement des
références et appel de la spécialisation.

Les mesures conservées ci-dessous précèdent l'adoption de Boost.CallableTraits
et Boost.Mp11 pour les assistants de compilation. Ce remplacement simplifie
leur maintenance ; il ne constitue pas une nouvelle optimisation du dispatch.
Son effet propre sur le temps de compilation n'a pas été mesuré isolément.

## Corrections vérifiées après la revue

La revue a révélé deux limites que les mesures Windows ne montraient pas :
un dépassement de la limite d'évaluation constante en réflexion sur 16 × 16 types,
et une régression du dispatch à deux types sous GCC. Les corrections ci-dessous
conservent le contrat de sélection et le rejet des types inconnus.

### Compilation du domaine réfléchi 16 × 16

Le générateur mémorise désormais les relations entre types distincts et la
compatibilité de chaque position. La sélection du candidat maximal est commune
aux deux générateurs ; elle ne compare que les candidats applicables. La voie
réfléchie ne construit plus la matrice de toutes les paires de fonctions.

`tests/test_grand_domaine.cpp` vérifie 256 spécialisations et toutes leurs cases,
avec des références ordinaires et préparées. Le test fait partie des modes GCC
`ON` et `AUTO`, sans modifier les limites du compilateur.

Sur ce même fichier, GCC 16.2 refuse l'ancien générateur avec le diagnostic
`constexpr evaluation operation count exceeds limit of 33554432`. Le générateur
corrigé termine l'analyse (`-O0 -fsyntax-only`) en **3,58 s**, avec un pic de
**798 740 Kio, soit environ 0,76 Gio**. Il s'agit d'un relevé isolé de l'analyse
d'un fichier, sans génération de code ni édition de liens, pas d'une mesure du
temps de compilation de tout le projet. L'ancien temps avant échec n'est pas
une durée de compilation réussie comparable.

Commande d'analyse dans le conteneur, depuis `/sources` :

```sh
g++ -std=c++26 -freflection -DMINI_OPENMETHOD_REFLEXION=1 -Iinclude -isystem /resultats/vcpkg_installed/x64-linux/include -O0 -fsyntax-only tests/test_grand_domaine.cpp
```

Depuis l'adoption des traits Boost et de doctest, la reproduction requiert leurs
en-têtes dans le chemin d'inclusion ajouté à la commande ci-dessus. Les chiffres
de ce paragraphe précèdent ces changements et ne mesurent pas leur effet sur la compilation.

Le script habituel `outils/verifier-gcc.ps1` effectue aussi la compilation complète
et les appels de ce test. Le coût des grands domaines en C++23 reste un chantier
distinct : la matrice de dominance par modèles y est conservée.

### Recherche sur les petits domaines sous GCC

L'arrêt anticipé empêchait certaines optimisations de l'appel complet lorsque
GCC ne décidait pas d'intégrer la petite fonction de recherche. L'attribut
`[[gnu::always_inline]]`, accompagné de `inline`, est maintenant appliqué à
`indice_lineaire` sous GCC uniquement. L'algorithme et le seuil restent inchangés ;
Clang et MSVC ne reçoivent pas cet attribut.

Comparaison contrôlée GCC 16.2, Linux x86-64 dans Docker Desktop, `-O3 -DNDEBUG`,
C++23, graine 42 : seul cet attribut change entre les deux variantes. Chaque
exécution donne la médiane de cinq passages de vingt millions d'appels ; trois
exécutions sont alternées, puis leurs médianes sont prises ci-dessous.

| Cas | Sans indication d'inlining | Avec indication GCC |
| --- | ---: | ---: |
| Simple, deux types | 3,55 ns/appel | 1,01 ns/appel |
| Double, deux types par position | 3,19 ns/appel | 1,48 ns/appel |
| Héritage virtuel | 11,99 ns/appel | 12,12 ns/appel |

Les sommes sont identiques. L'héritage virtuel conserve son coût d'ajustement RTTI.
Trois variantes exploratoires ont également été essayées : tableau d'égalités
dans un assistant (2,49 / 3,28 ns en simple / double), masque de bits des adresses
(6,80 / 9,77 ns), et tableau directement dans `traits_liste` pour deux types
(1,03 / 1,67 ns). Ces variantes ont chacune une seule exécution de cinq passages :
elles ne constituent pas un classement universel des algorithmes. Elles montrent
surtout l'importance de mesurer l'appel complet, pas seulement la recherche isolée.

Résultats bruts : [indexation GCC](resultats/correction_indexation_gcc.txt).
Le programme `mesurer_dispatch 20000000 42` reproduit la mesure de la version
actuelle. Les variantes exploratoires et le témoin sans attribut sont des copies
locales de travail ; elles ne deviennent pas des implémentations entretenues.

Le banc Boost a aussi été relancé après correction, sans compilation simultanée :
[MSVC](resultats/boost_msvc_apres_revue.txt) et
[Clang](resultats/boost_clang_apres_revue.txt). Les résultats restent proches des
mesures précédentes ; ces données réutilisées et ces petits traitements ne
garantissent pas les mêmes gains dans une application réelle.

### Vérification répétée des appels préparés sous MSVC

Les premières mesures après revue montraient un ralentissement du double appel
préparé sous MSVC : de 5,240 à 5,713 ns avec deux types (+9 %), et de 6,432 à
6,773 ns avec huit types (+5 %). Une seule exécution de chaque version ne
permettait pas d'attribuer ces écarts aux corrections.

Un contrôle supplémentaire du 1er octobre 2026 compare dix paires d'exécutions
avec MSVC 19.51.36260, sur le même Ryzen 9 9950X, en Release `/O2 /Ob2 /GL-`.
Chaque exécution mesure sept passages de dix millions d'appels par scénario,
avec la graine 42. L'ordre des deux exécutables s'inverse à chaque paire ;
l'ordre des quatre implémentations tourne à chaque passage. Aucun autre banc
ni compilation n'est lancé simultanément. La préparation reste hors chronométrage.

Le témoin représente la version optimisée juste avant les corrections de revue.
Il est reconstitué à partir des anciens `resolution_cpp23.hpp`,
`resolution_cpp26.hpp` et `index_types.hpp` conservés localement. Les autres
sources, le programme de mesure, les options et les paquets vcpkg sont identiques.
Le mode `AUTO` sélectionne C++23 dans les deux constructions. Les 22 tests passent
pour chacune et toutes les sommes des mesures concordent.

Médianes des dix médianes, en ns/appel préparé ; écarts calculés avant arrondi :

| Cas | Types par position | Avant revue | Après correction | Écart |
| --- | ---: | ---: | ---: | ---: |
| Simple | 2 | 3,690 | 3,706 | +0,45 % |
| Simple | 8 | 6,028 | 6,018 | −0,17 % |
| Simple | 32 | 6,215 | 6,045 | −2,73 % |
| Double | 2 | 5,579 | 5,577 | −0,04 % |
| Double | 8 | 6,251 | 6,277 | +0,42 % |
| Héritage virtuel | 2 | 7,694 | 7,711 | +0,22 % |

Les ralentissements initiaux de 9 % et 5 % ne sont donc pas confirmés par cette
série. En double à deux types, un même exécutable donne des médianes allant de
5,267 à 5,610 ns avant correction, et de 5,201 à 5,608 ns après. Les plages à huit
types se recouvrent également : 6,170–6,306 ns et 6,187–6,332 ns. Les médianes
des écarts calculés paire par paire valent respectivement −0,23 % et +0,51 %.
Les contrôles Boost préparés varient de +0,24 % et +0,05 % entre les médianes
des deux constructions sur ces mêmes scénarios.

Le [contrôle du code généré](resultats/controle_msvc_code_genere.txt) retrouve
les mêmes séquences d'instructions après normalisation des adresses de données.
Les exécutables diffèrent toutefois par certaines de ces adresses : ils ne sont
pas identiques octet par octet. Une reconstruction successive dans le même
emplacement reproduit exactement la section de code de chaque variante et
confirme que cette différence ne vient pas du seul chemin de compilation.

Ce résultat ne démontre pas une égalité universelle : la fréquence du processeur,
le placement des données et les optimisations du compilateur restent visibles
à cette échelle. Le gain apparent à 32 types n'est pas attribué au changement
du générateur ; Boost préparé baisse aussi d'environ 2,4 % dans ce contrôle.
Aucun changement supplémentaire du moteur n'est justifié par ces mesures.

Les [vingt exécutions complètes](resultats/controle_msvc_prepare.txt) et leur
[synthèse avec plages et écarts appariés](resultats/controle_msvc_prepare_resume.csv)
sont conservées. Les copies de sources et les scripts de ce contrôle ponctuel
se trouvent localement dans `build-controle-msvc`.

## Optimisation commune à l'exécution

Le type dynamique est déjà vérifié avant l'appel du relais. Lorsque la
conversion de la racine vers le paramètre choisi permet un `static_cast`,
le moteur utilise cette conversion et évite un second contrôle RTTI. Les bases
virtuelles continuent à utiliser `dynamic_cast`. Cette optimisation profite
aux deux versions ; la réflexion seule n'accélère pas les appels.

L'identification utilise désormais une recherche courte jusqu'à 8 types et un
index haché au-delà. Les deux chemins comparent d'abord les adresses RTTI et
conservent un repli sur l'égalité des types. Une référence préparée évite les
recherches suivantes. Une table plus grande, un autre compilateur ou des
spécialisations coûteuses peuvent changer la part du temps consacrée au dispatch.

## Choix du seuil d'indexation

`mesurer_indexation` compare la recherche linéaire avec arrêt anticipé et le
hachage, avant l'appel d'une spécialisation. Les deux chemins vérifient le type
exact. La fabrique d'objets est compilée séparément, sans LTO. Les données
sont équilibrées et mélangées (4 096 entrées, graine 42), avec cinq millions
de recherches par passage et une médiane de sept passages en ordre alterné.
L'initialisation du hachage est exclue ; les indices et leurs sommes sont vérifiés.

Mesure du 1er octobre 2026 sur Ryzen 9 9950X, Windows x64, en Release ; ns/recherche :

| Types | MSVC linéaire | MSVC hachage | Clang linéaire | Clang hachage |
| ---: | ---: | ---: | ---: | ---: |
| 2 | 2,18 | 1,47 | 1,28 | 1,96 |
| 4 | 1,46 | 1,49 | 2,18 | 4,98 |
| 8 | 1,58 | 1,63 | 2,62 | 3,85 |
| 16 | 3,12 | 3,52 | 3,11 | 2,22 |
| 32 | 3,93 | 2,49 | 3,86 | 3,65 |

Le seuil retenu est **8 types**. Il conserve une voie simple sans initialisation
pour les petits domaines et favorise le hachage lorsque la liste grandit.
Ce compromis n'est pas optimal dans chaque case, notamment MSVC à 2 ou 16 types.
Les collisions dépendent de la disposition des adresses RTTI ; la distribution
des entrées et le compilateur peuvent déplacer le point d'équilibre.
Sources brutes : [MSVC](resultats/indexation_msvc.txt) et
[Clang](resultats/indexation_clang.txt). Le coût complet d'un appel est mesuré
séparément dans la [comparaison Boost](comparaison_boost.md).

Pour reproduire, configurer avec `MINI_OPENMETHOD_MESURES=ON`, compiler la cible
`mesurer_indexation` en Release et exécuter son binaire sans argument.
Ce banc ne requiert pas Boost.OpenMethod ; les traits Boost de la bibliothèque
restent nécessaires. Aucun seuil de durée ne sert de test.

## Mesure antérieure des conversions du 1er octobre 2026

Environnement : GCC 16.2.0 dans Docker Desktop Linux x86-64 sous Windows,
compilation Release (`-O3 -DNDEBUG`), sans LTO. Même programme de mesure, mêmes
entrées et même graine pour toutes les variantes, exécutées successivement.
Ce tableau précède l'optimisation de l'indexation et l'ajout des références
préparées. La référence emploie les en-têtes sauvegardés avant l'optimisation, avec
`dynamic_cast` systématique dans les relais.

| Cas | Avant, C++23 | Après, C++23 | Après, réflexion C++26 |
| --- | ---: | ---: | ---: |
| Dispatch simple, deux types | 3,99 ns/appel | 1,06 ns/appel | 1,04 ns/appel |
| Double dispatch, quatre combinaisons | 8,26 ns/appel | 1,72 ns/appel | 1,69 ns/appel |
| Héritage virtuel, deux types | 12,37 ns/appel | 12,40 ns/appel | 12,79 ns/appel |

Chaque nombre est la médiane de cinq passages de cinq millions d'appels.
Une séquence de 4 096 entrées est mélangée à l'exécution, avec la graine 42.
L'échauffement vérifie chaque résultat ; les passages chronométrés contrôlent
une somme attendue calculée indépendamment. Aucun seuil temporel ne détermine
la réussite des tests.

Ces petits domaines et traitements rendent les optimisations du compilateur
très visibles. Il s'agit d'un microbenchmark local, sans garantie de gain sur
une application réelle. Ces chiffres ne se comparent pas directement au nouveau
banc Boost, qui utilise une fabrique séparée et d'autres jeux de données. Les écarts
entre C++23 et C++26 sont trop faibles pour conclure à un avantage de la
réflexion à l'exécution ; l'héritage virtuel conserve son coût RTTI.

## Reproduire les mesures

Depuis PowerShell, avec Docker Desktop démarré :

```powershell
./outils/verifier-gcc.ps1
```

Le script compile proprement et teste `OFF`, `ON` et `AUTO`. Il vérifie aussi
un consommateur de chaque installation. Les résultats sont enregistrés dans
`build-gcc/<mode>/mesures.txt`. `configuration.log` indique le choix effectif,
`tests.log` et `installation.log` les validations, `compilation.txt` la durée
de compilation et la mémoire maximale d'un processus enfant.

La durée mesurée concerne tous les exemples, tests et le programme de mesure,
hors configuration et tests de compilation refusée. Elle dépend des caches
de fichiers, de la machine et du parallélisme fixé à deux. Elle ne permet pas
d'isoler le coût du générateur de tables ni de promettre une compilation C++26
plus rapide. Les deux algorithmes restent quadratiques dans le nombre de
spécialisations pour chaque combinaison du domaine au pire cas.

Sans Docker, le programme est facultatif :

```sh
cmake -S . -B build-mesures -DCMAKE_TOOLCHAIN_FILE=/chemin/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DMINI_OPENMETHOD_MESURES=ON -DMINI_OPENMETHOD_REFLEXION=AUTO
cmake --build build-mesures --config Release --target mesurer_dispatch
```

Exécuter ensuite `build-mesures/mesurer_dispatch 5000000 42`, ou
`build-mesures/Release/mesurer_dispatch.exe 5000000 42` avec Visual Studio.
Utiliser des dossiers distincts avec `OFF` et `ON` pour comparer les deux
générateurs sur une chaîne possédant la réflexion.

Le programme livré reproduit les versions actuelles. Les mesures « avant »
ci-dessus documentent la comparaison locale avec la sauvegarde des anciens
en-têtes ; elles ne sont pas une troisième implémentation entretenue.
