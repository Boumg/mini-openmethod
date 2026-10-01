#include <mini_openmethod/methode.hpp>
#include <mini_openmethod/methode.hpp>
#include <array>
#include <doctest/doctest.h>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

using namespace mini_openmethod;
namespace {
struct Animal { virtual ~Animal() = default; };
struct Mammifere : Animal {};
struct Chien : Mammifere {};
struct Chat : Mammifere {};
struct Oiseau : Animal {};
struct Chiot : Chien {};
using Animaux = liste_types<Chien, Chat, Oiseau>;
int decrire_chien(const Chien&) noexcept { return 7; }

/** Le type statique de la reference ne doit pas dicter la specialisation. */
TEST_CASE("Dispatch simple et ordre des specialisations") {
    auto operation = creer_methode<int(const Animal&)>(
        domaines<Animaux>{},
        [](const Animal&) { return 0; },
        [](const Mammifere&) { return 1; },
        [](const Chien&) { return 2; });
    Chien chien;
    Chat chat;
    Oiseau oiseau;
    const Animal& reference = chien;
    CHECK_MESSAGE(operation(reference) == 2, "Type dynamique a travers une reference de base");
    CHECK_MESSAGE(operation(chat) == 1, "Repli sur la classe intermediaire");
    CHECK_MESSAGE(operation(oiseau) == 0, "Repli sur la racine");
    auto inverse = creer_methode<int(const Animal&)>(
        domaines<Animaux>{},
        [](const Chien&) { return 2; },
        [](const Mammifere&) { return 1; },
        [](const Animal&) { return 0; });
    for (const Animal* animal : std::array<const Animal*, 3>{&chien, &chat, &oiseau})
        CHECK_MESSAGE(operation(*animal) == inverse(*animal), "Independance de l'ordre des specialisations");
    Chiot inconnu;
    CHECK_THROWS_AS_MESSAGE(operation(inconnu), type_inconnu, "Un descendant non declare doit etre refuse");
}

/** Verifie toute la matrice, notamment les intersections et l'asymetrie. */
TEST_CASE("Double dispatch et references preparees") {
    auto operation = creer_methode<int(const Animal&, const Animal&)>(
        domaines<Animaux, Animaux>{},
        [](const Animal&, const Animal&) { return 0; },
        [](const Chien&, const Animal&) { return 1; },
        [](const Animal&, const Chat&) { return 2; },
        [](const Chien&, const Chat&) { return 3; });
    Chien chien;
    Chat chat;
    Oiseau oiseau;
    const std::array<const Animal*, 3> animaux{&chien, &chat, &oiseau};
    constexpr std::array<std::array<int, 3>, 3> attendus{{
        {1, 3, 1}, {0, 2, 0}, {0, 2, 0}}};
    for (std::size_t ligne = 0; ligne < animaux.size(); ++ligne) {
        for (std::size_t colonne = 0; colonne < animaux.size(); ++colonne) {
            CAPTURE(ligne);
            CAPTURE(colonne);
            CHECK_MESSAGE(operation(*animaux[ligne], *animaux[colonne]) == attendus[ligne][colonne],
                     "Matrice complete du double dispatch");
            CHECK_MESSAGE(operation(operation.preparer<0>(*animaux[ligne]), operation.preparer<1>(*animaux[colonne]))
                == attendus[ligne][colonne], "Matrice complete avec references preparees");
        }
    }
    Chiot inconnu;
    CHECK_THROWS_AS_MESSAGE(operation(inconnu, chat), type_inconnu, "Inconnu a gauche");
    CHECK_THROWS_AS_MESSAGE(operation(chien, inconnu), type_inconnu, "Inconnu a droite");
    CHECK_THROWS_AS_MESSAGE((void)operation.preparer<0>(inconnu), type_inconnu, "Preparation inconnue a gauche");
    CHECK_THROWS_AS_MESSAGE((void)operation.preparer<1>(inconnu), type_inconnu, "Preparation inconnue a droite");
}

struct Gauche : virtual Animal { int gauche = 4; };
struct Droite : virtual Animal { int droite = 9; };
struct Diamant : Gauche, Droite {};
struct Etiquette { virtual ~Etiquette() = default; int valeur = 10; };
struct Etiquete : Etiquette, Chien { int valeur = 12; };
struct Support { virtual ~Support() = default; };
struct Papier : Support {};
struct Ecran : Support {};

/** Les ajustements de pointeurs ne doivent pas supposer un heritage simple. */
TEST_CASE("Heritages multiples et domaines distincts") {
    auto operation = creer_methode<int(const Animal&)>(
        domaines<liste_types<Diamant>>{},
        [](const Gauche& objet) { return objet.gauche; },
        [](const Droite& objet) { return objet.droite; },
        [](const Diamant& objet) { return objet.gauche + objet.droite; });
    Diamant diamant;
    const Animal& reference = diamant;
    CHECK_MESSAGE(operation(reference) == 13, "Diamant virtuel et intersection explicite");
    CHECK_MESSAGE(operation(operation.preparer(reference)) == 13, "Preparation du diamant virtuel");
    auto repli = creer_methode<int(const Animal&)>(
        domaines<liste_types<Diamant>>{}, [](const Droite& objet) { return objet.droite; });
    CHECK_MESSAGE(repli(reference) == 9, "Conversion vers une base intermediaire virtuelle");
    CHECK_MESSAGE(repli(operation.preparer(reference)) == 9, "Reference reutilisee par une autre methode compatible");
    auto multiple = creer_methode<int(const Animal&)>(
        domaines<liste_types<Etiquete>>{}, [](const Etiquete& objet) { return objet.valeur; });
    Etiquete etiquete;
    const Animal& base_decalee = etiquete;
    CHECK_MESSAGE(multiple(base_decalee) == 12, "Ajustement en heritage multiple non virtuel");
    CHECK_MESSAGE(multiple(multiple.preparer(base_decalee)) == 12, "Preparation d'une base decalee");
    auto afficher = creer_methode<int(const Animal&, const Support&)>(
        domaines<liste_types<Chien, Chat, Oiseau>, liste_types<Papier, Ecran>>{},
        [](const Animal&, const Support&) { return 0; },
        [](const Chat&, const Ecran&) { return 8; });
    Chien chien;
    Chat chat;
    Oiseau oiseau;
    Papier papier;
    Ecran ecran;
    const std::array<const Animal*, 3> animaux{&chien, &chat, &oiseau};
    const std::array<const Support*, 2> supports{&papier, &ecran};
    for (std::size_t ligne = 0; ligne < animaux.size(); ++ligne) {
        for (std::size_t colonne = 0; colonne < supports.size(); ++colonne) {
            CAPTURE(ligne);
            CAPTURE(colonne);
            CHECK_MESSAGE(afficher(*animaux[ligne], *supports[colonne])
                == (ligne == 1 && colonne == 1 ? 8 : 0), "Domaines rectangulaires de racines distinctes");
        }
    }
    CHECK_MESSAGE(afficher(afficher.preparer(chat), afficher.preparer<1>(ecran)) == 8,
             "References preparees de racines et domaines distincts");
    using Animal_prepare = decltype(afficher.preparer(chat));
    using Support_prepare = decltype(afficher.preparer<1>(ecran));
    static_assert(!std::is_invocable_v<decltype(afficher)&, Support_prepare, Animal_prepare>);
}

/** Captures, deplacement, void, references et propagation des exceptions. */
TEST_CASE("Captures retours et exceptions des appelables") {
    Chien chien;
    auto pointeur = creer_methode<int(const Animal&)>(
        domaines<liste_types<Chien>>{}, &decrire_chien);
    CHECK_MESSAGE(pointeur(chien) == 7, "Pointeur de fonction noexcept");
    auto compteur = creer_methode<int(const Animal&)>(
        domaines<liste_types<Chien>>{},
        [compte = std::make_unique<int>(0)](const Chien&) mutable noexcept { return ++*compte; });
    CHECK_MESSAGE(compteur(chien) == 1, "Premier appel de la capture mobile et mutable");
    CHECK_MESSAGE(compteur(chien) == 2, "Second appel de la capture mobile et mutable");
    const auto prepare = compteur.preparer(chien);
    auto deplace = std::move(compteur);
    CHECK_MESSAGE(deplace(prepare) == 3, "Preparation preservee apres deplacement de la methode");
    int appels = 0;
    auto action = creer_methode<void(const Animal&)>(
        domaines<liste_types<Chien>>{}, [&appels](const Animal&) { ++appels; });
    action(chien);
    action(prepare);
    CHECK_MESSAGE(appels == 2, "Retour void et capture par reference, dont appel prepare");
    int valeur = 42;
    auto reference = creer_methode<int&(const Animal&)>(
        domaines<liste_types<Chien>>{}, [&valeur](const Chien&) -> int& { return valeur; });
    CHECK_MESSAGE(&reference(chien) == &valeur, "Retour par reference preserve");
    CHECK_MESSAGE(&reference(prepare) == &valeur, "Retour par reference preserve avec preparation");
    auto echec = creer_methode<void(const Animal&)>(
        domaines<liste_types<Chien>>{},
        [](const Chien&) { throw std::logic_error("Erreur de la specialisation"); });
    CHECK_THROWS_AS_MESSAGE(echec(chien), std::logic_error, "Exception utilisateur propagee");
    CHECK_THROWS_AS_MESSAGE(echec(prepare), std::logic_error, "Exception utilisateur propagee avec preparation");
}

/** Une preparation valide le type, sans figer l'etat ni les captures. */
TEST_CASE("Validite et reutilisation des references preparees") {
    struct Mesure : Animal { int valeur = 5; };
    struct Autre : Animal {};
    using Domaine = liste_types<Mesure, Autre>;
    auto operation = creer_methode<int(const Animal&)>(domaines<Domaine>{},
        [](const Animal&) { return 0; }, [](const Mesure& mesure) { return mesure.valeur; });
    Mesure mesure;
    const Animal& base = mesure;
    const auto prepare = std::as_const(operation).preparer(base);
    auto copie = prepare;
    CHECK_MESSAGE(&prepare.objet() == &base, "Acces a l'objet d'origine");
    CHECK_MESSAGE(operation(copie) == 5, "Preparation depuis une reference constante de base");
    mesure.valeur = 9;
    CHECK_MESSAGE(operation(prepare) == 9, "Les modifications de l'objet restent visibles");
    auto autre_operation = creer_methode<int(const Animal&)>(domaines<Domaine>{},
        [valeur = 17](const Animal&) { return valeur; });
    CHECK_MESSAGE(autre_operation(prepare) == 17, "Les captures de la methode appelee sont utilisees");
    using Prepare = decltype(copie);
    static_assert(std::is_copy_constructible_v<Prepare> && std::is_copy_assignable_v<Prepare>);
    static_assert(!std::is_default_constructible_v<Prepare>);
    static_assert(!std::is_constructible_v<Prepare, const Animal&, std::size_t>);
    static_assert(!std::is_invocable_v<decltype(operation)&,
        reference_preparee<Animal, liste_types<Autre, Mesure>>>);
    static_assert(!std::is_invocable_v<decltype(operation)&,
        reference_preparee<Animal, liste_types<Mesure>>>);
    Chiot inconnu;
    CHECK_THROWS_AS_MESSAGE((void)operation.preparer(inconnu), type_inconnu, "Un type inconnu ne peut pas produire de reference preparee");
}

/** Une base intermediaire privee ou ambigue ne constitue pas une specialisation applicable. */
TEST_CASE("Repli sur les bases inaccessibles") {
    struct Racine { virtual ~Racine() = default; };
    struct Specialisation : virtual Racine {};
    struct Cache : private Specialisation, virtual Racine {};
    struct Gauche : Specialisation {};
    struct Droite : Specialisation {};
    struct Ambigu : Gauche, Droite {};
    auto operation = creer_methode<int(const Racine&)>(
        domaines<liste_types<Cache, Ambigu, Specialisation>>{},
        [](const Racine&) { return 0; }, [](const Specialisation&) { return 1; });
    Cache cache;
    Ambigu ambigu;
    Specialisation directe;
    CHECK_MESSAGE(operation(cache) == 0, "Repli quand la base intermediaire est privee");
    CHECK_MESSAGE(operation(ambigu) == 0, "Repli quand la base intermediaire est ambigue");
    CHECK_MESSAGE(operation(directe) == 1, "Conversion a travers une racine virtuelle");
}
} // namespace
