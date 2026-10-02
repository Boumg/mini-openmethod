# Fonctions non virtuelles, virtuelles et mini-openmethod

Cette page conserve une **référence mesurée le 2 octobre 2026**. Elle se consulte
sans reconstruire le projet ni refaire les chronométrages. Les nombres décrivent
ce banc, cette machine et ces compilateurs ; ils ne sont pas des garanties de
temps pour toutes les applications.

## Ce que chaque solution apporte

| Solution | Choix du traitement | Conséquence pratique |
| --- | --- | --- |
| Fonction non virtuelle, appel direct | Selon les types statiques et la résolution de surcharge | À privilégier quand le traitement est déjà connu ; le compilateur peut intégrer son corps |
| Fonction virtuelle | Selon le type dynamique de l'objet receveur | Solution native pour un dispatch simple ; l'opération doit être prévue dans la hiérarchie |
| mini-openmethod | Selon un ou deux types dynamiques, sur un domaine explicite | Opération extérieure aux classes, spécialisations par lambdas, validation des combinaisons à la compilation |

Une fonction non virtuelle appelée via une référence de base ne sélectionne
pas automatiquement la version d'une classe dérivée. Le choix virtuel dépend
du type dynamique de l'objet. Ces règles viennent du
[modèle objet C++](https://eel.is/c++draft/class.virtual).
Une seule fonction virtuelle ne réalise pas le double dispatch sur deux objets :
un visiteur, deux appels virtuels ou un autre mécanisme seraient nécessaires.
Ce banc compare donc uniquement le **dispatch simple**.

Le chemin machine typique d'un appel virtuel non dévirtualisé charge une cible
dans la table virtuelle, puis l'appelle indirectement. Un appel mini-openmethod
brut identifie en plus le type exact par RTTI et cherche l'entrée de sa table.
Une référence préparée réutilise l'indice, avec une validation faite auparavant.
Le standard n'impose pas une représentation particulière des tables virtuelles.

## Pourquoi séparer les cas

Le banc possède une séquence homogène à un type et deux séquences hétérogènes,
à deux puis huit types. Chaque variante d'une ligne traite **les mêmes objets,
dans le même ordre et avec le même résultat attendu**.

Dans le cas homogène, une vue typée `Variante<0>*` est disponible avant la mesure :
elle permet l'appel non virtuel direct. Les autres variantes utilisent une vue
`Objet*` ou une référence préparée. Pour les types mélangés, le choix de la
spécialisation reste à faire : aucun temps « non virtuel direct » n'est affiché.
Ajouter un `switch` ou préchoisir une fonction en dehors de la boucle serait un
autre mécanisme de dispatch, avec un coût à expliciter.

Deux présentations du même calcul sont distinguées :

- **Corps visible** : `calculer_direct()` est défini dans l'en-tête. Le compilateur
  peut l'intégrer, spécialiser les appels et éventuellement vectoriser la boucle.
- **Corps séparé** : `calculer_separe()` est défini dans un autre fichier.
  Sans LTO, son corps est inconnu du compilateur de la boucle. Les lambdas
  mini-openmethod appellent alors cette fonction concrète. La fonction virtuelle
  est définie dans ce même fichier séparé et réalise le même calcul.

La colonne virtuelle n'a pas un corps visible dans l'unité du chronométrage.
Il faut donc regarder en priorité les colonnes **séparées** pour apprécier le
surcoût du mécanisme d'appel. Les colonnes visibles illustrent les optimisations
supplémentaires possibles avec les lambdas ; elles ne prouvent pas que le
dispatch de mini-openmethod soit intrinsèquement plus rapide qu'un appel virtuel.
Une virtuelle que le compilateur réussit à dévirtualiser et intégrer peut elle
aussi se rapprocher du coût de l'appel direct.

## Résultats conservés

Machine : AMD Ryzen 9 9950X. Unité : **nanoseconde par appel**, coût moyen d'une
boucle, et non latence isolée d'une instruction. Tous les chiffres ci-dessous
sont des médianes de trois exécutions ; chacune retient la médiane de sept
passages de cinq millions d'appels.

### Un type connu : comparaison avec l'appel direct

| Compilateur | Direct visible | Direct séparé | Virtuel | Mini séparé | Mini séparé préparé |
| --- | ---: | ---: | ---: | ---: | ---: |
| MSVC 19.51 | 0,342 | 0,914 | 0,915 | 2,021 | 0,930 |
| Clang 23.1.2 | 0,212 | 0,727 | 1,091 | 2,008 | 0,913 |
| GCC 16.2 | 0,267 | 0,926 | 1,075 | 0,926 | 0,743 |

Sous MSVC, la virtuelle et l'appel direct séparé sont ici presque au même
niveau : la cible virtuelle est très prévisible. L'appel mini brut coûte
environ 2,2 fois la virtuelle dans ce cas ; la référence préparée est proche.
L'appel direct visible reste le moins coûteux. Il évite de conserver une
frontière d'appel et peut permettre d'optimiser la boucle entière.

### Types mélangés : comparaison des dispatchs dynamiques

| Compilateur | Types | Virtuel | Mini séparé | Mini séparé préparé |
| --- | ---: | ---: | ---: | ---: |
| MSVC 19.51 | 2 | 4,030 | 4,927 | 4,276 |
| MSVC 19.51 | 8 | 5,643 | 3,636 | 6,164 |
| Clang 23.1.2 | 2 | 4,144 | 2,446 | 0,960 |
| Clang 23.1.2 | 8 | 5,693 | 3,997 | 7,576 |
| GCC 16.2 | 2 | 4,445 | 1,017 | 4,623 |
| GCC 16.2 | 8 | 5,775 | 1,671 | 6,180 |

Le mélange des cibles rend l'appel virtuel plus coûteux dans ce banc. Les
résultats des méthodes externes varient avec la taille du domaine et les
optimisations du compilateur. **Une référence préparée n'est pas toujours
plus rapide** : elle change aussi les accès mémoire et le code produit.
Ni « même coût qu'une virtuelle », ni « toujours plus rapide » ne sont justifiés.

### Effet d'un traitement visible par le compilateur

Les mêmes lignes, mais avec les lambdas appelant le corps visible :

| Compilateur | Types | Mini visible | Mini visible préparé |
| --- | ---: | ---: | ---: |
| MSVC 19.51 | 1 | 2,012 | 1,098 |
| MSVC 19.51 | 2 | 6,666 | 3,744 |
| MSVC 19.51 | 8 | 3,426 | 5,749 |
| Clang 23.1.2 | 1 | 1,285 | 0,234 |
| Clang 23.1.2 | 2 | 1,341 | 0,299 |
| Clang 23.1.2 | 8 | 2,465 | 0,368 |
| GCC 16.2 | 1 | 0,366 | 0,292 |
| GCC 16.2 | 2 | 1,197 | 4,534 |
| GCC 16.2 | 8 | 1,628 | 5,808 |

Les valeurs inférieures à une nanoseconde expriment le débit d'un traitement
arithmétique très petit, optimisé dans une boucle. Elles ne doivent pas devenir
un coût universel d'une méthode externe. La visibilité du corps, la disposition
du code et la prédiction des branches peuvent modifier le classement.

Sorties complètes des trois exécutions :
[MSVC](resultats/appels_cpp_msvc.txt),
[Clang](resultats/appels_cpp_llvm.txt),
[GCC](resultats/appels_cpp_gcc.txt).
Ces objets et cette hiérarchie sont propres à ce banc : ne pas juxtaposer
ses nombres à ceux du [banc Boost](comparaison_boost.md) comme s'ils mesuraient
exactement le même programme.

## Protocole et limites

- Windows x64 : MSVC `/O2 /Ob2 /MD`, Clang `-O3` avec la bibliothèque standard
  Microsoft ; affinité sur le processeur logique 4. GCC `-O3` sous Linux x86-64
  dans Docker Desktop. C++23 pour tous les chronométrages, LTO désactivée.
- 4 096 entrées, graine 42, quatre objets par type ; types équilibrés et mélangés.
  La fabrique et les fonctions séparées sont dans une autre unité de compilation.
  Chaque variante reçoit exactement les mêmes données au sein d'un exécutable.
  Le mélange peut différer entre bibliothèques standard, même à graine identique :
  ces résultats ne classent donc pas les compilateurs entre eux.
- Création des objets et préparation des références exclues du temps mesuré.
  L'ordre des variantes tourne entre les passages ; pas de compilation simultanée.
- Chaque entrée puis chaque somme finale sont contrôlées. Cela empêche de mesurer
  un résultat incorrect, sans interdire les optimisations légales du compilateur.
  Le code Clang inspecté conserve des appels indirects via la table virtuelle
  dans les boucles et des appels directs aux fonctions séparées.
- Domaine fermé et entièrement valide pour mini-openmethod. Une virtuelle native
  ne fournit pas son contrôle de domaine exact ni sa validation d'exhaustivité.
- Petits traitements, caches chauds, préparation amortie ; pas de mesure de
  création, d'allocation, de double dispatch ni de charge applicative réelle.
  La réflexion C++26 est vérifiée fonctionnellement, pas chronométrée dans cette référence.

## Consulter, vérifier ou actualiser

### Contrôle après ajout des objets modifiables

Le 2 octobre 2026, les exécutables Release du banc à sources inchangées ont été
comparés avant et après l'ajout de `Classe&` (issue #4). La section machine
`.text` est identique octet pour octet sous MSVC 19.51 et Clang 23.1.2 :
27 136 et 34 304 octets respectivement. Les
[empreintes SHA-256 conservées](resultats/objets_modifiables_code.json)
comparent les exécutables de la version intégrée par #3 avec ceux de cette évolution.

Ce contrôle porte sur les appels constants existants, bruts et préparés, du
banc `comparer_appels`. Il n'est pas une mesure du coût de mutation d'un objet,
de conversion d'une référence préparée modifiable vers constante, ni une garantie
sur tous les compilateurs. Les résultats chronométriques ci-dessus sont conservés ;
aucun nouveau chronométrage n'a été nécessaire pour ce contrôle du code généré.

### Quand relancer les mesures

La référence ci-dessus reste figée. **Ne pas refaire les chronométrages pour
chaque changement de documentation, ajout de test ou question de comparaison.**
Une nouvelle mesure est utile sur demande, pour évaluer une évolution du chemin
de dispatch ou lors d'un changement significatif de compilateur ou de machine.
La date, l'environnement et le protocole doivent alors accompagner les nouvelles
valeurs ; conserver les anciennes comme historique.

Le banc indépendant se construit avec `MINI_OPENMETHOD_MESURES=ON` ou
`MINI_OPENMETHOD_COMPARAISON_BOOST=ON`. Il utilise les dépendances d'en-têtes déjà
présentes ; Boost.OpenMethod n'est pas requis avec la première option.

```powershell
cmake -S . -B build-appels -DCMAKE_TOOLCHAIN_FILE=C:/chemin/vcpkg/scripts/buildsystems/vcpkg.cmake -DMINI_OPENMETHOD_MESURES=ON -DMINI_OPENMETHOD_REFLEXION=OFF
cmake --build build-appels --config Release --target comparer_appels
# Verification fonctionnelle uniquement, y compris sans argument :
./build-appels/Release/comparer_appels.exe --verifier
# Chronometrage seulement sur demande :
./build-appels/Release/comparer_appels.exe 5000000 42
```

Avec Ninja ou Make, ajouter `-DCMAKE_BUILD_TYPE=Release` à la configuration et
utiliser `build-appels/comparer_appels` (suffixe `.exe` sous Windows).
CTest et la CI appellent uniquement `--verifier` pour ce banc, sans seuil de
performance. Les tests ont été vérifiés sous MSVC Debug/Release, Clang C++23
et GCC C++23/C++26.
