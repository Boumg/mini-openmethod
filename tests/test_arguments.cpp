#include <mini_openmethod/methode.hpp>
#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

/** @file Transmission des arguments exclus de la selection polymorphe. */
namespace {
using namespace mini_openmethod;
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};
struct Oiseau : Animal {};
struct Inconnu : Animal {};
struct Support { virtual ~Support() = default; };
struct Papier : Support {};
struct Ecran : Support {};
using Animaux = liste_types<Chien, Chat, Oiseau>;
using Supports = liste_types<Papier, Ecran>;
struct Contexte { int appels = 0; };

using Description = detail::description_arguments<
    domaines<argument_ordinaire, Animaux, argument_ordinaire, Supports, argument_ordinaire>>;
static_assert(std::same_as<Description::positions, std::index_sequence<1, 3>>);
static_assert(std::same_as<Description::domaines_polymorphes, domaines<Animaux, Supports>>);
static_assert(std::same_as<Description::signature<int(Contexte&, const Chien&, double,
                                                     const Papier&, std::ostream&)>,
                           void(const Chien&, const Papier&)>);
} // namespace

TEST_CASE("Arguments ordinaires : montant et flux apres le dispatch simple") {
    auto facturer = creer_methode<void(const Animal&, double, std::ostream&)>(
        domaines<Animaux, argument_ordinaire, argument_ordinaire>{},
        [](const Animal&, double montant, std::ostream& sortie) { sortie << "Animal " << montant; },
        [](const Chien&, double montant, std::ostream& sortie) { sortie << "Chien " << montant; });
    Chien chien;
    Chat chat;
    Inconnu inconnu;
    std::ostringstream sortie;
    facturer(chien, 12.5, sortie);
    facturer(facturer.preparer(chien), 18.0, sortie);
    facturer(chat, 2.0, sortie);
    CHECK(sortie.str() == "Chien 12.5Chien 18Animal 2");
    CHECK_THROWS_AS(facturer(inconnu, 0.0, sortie), type_inconnu);
}

TEST_CASE("Arguments ordinaires : matrice trois par deux et positions intercalees") {
    auto operation = creer_methode<int(Contexte&, const Animal&, int, const Support&, int&)>(
        domaines<argument_ordinaire, Animaux, argument_ordinaire, Supports, argument_ordinaire>{},
        [](Contexte& contexte, const Animal&, int montant, const Support&, int& trace) {
            ++contexte.appels; trace = 1; return montant + 1;
        },
        [](Contexte& contexte, const Chien&, int montant, const Support&, int& trace) {
            ++contexte.appels; trace = 2; return montant + 2;
        },
        [](Contexte& contexte, const Animal&, int montant, const Ecran&, int& trace) {
            ++contexte.appels; trace = 3; return montant + 3;
        },
        [](Contexte& contexte, const Chien&, int montant, const Ecran&, int& trace) {
            ++contexte.appels; trace = 4; return montant + 4;
        });
    Chien chien; Chat chat; Oiseau oiseau; Papier papier; Ecran ecran;
    const Animal* animaux[]{&chien, &chat, &oiseau};
    const Support* supports[]{&papier, &ecran};
    const int attendus[3][2]{{2, 4}, {1, 3}, {1, 3}};
    Contexte contexte;
    int trace = 0;
    for (std::size_t gauche = 0; gauche < 3; ++gauche) {
        for (std::size_t droite = 0; droite < 2; ++droite) {
            CHECK(operation(contexte, *animaux[gauche], 10, *supports[droite], trace)
                  == 10 + attendus[gauche][droite]);
            CHECK(trace == attendus[gauche][droite]);
            CHECK(operation(contexte, operation.preparer<1>(*animaux[gauche]), 20,
                            operation.preparer<3>(*supports[droite]), trace)
                  == 20 + attendus[gauche][droite]);
        }
    }
    CHECK(contexte.appels == 12);
    using Prepare = decltype(operation.preparer<1>(chien));
    // Une position polymorphe brute ne se melange pas a une autre preparee.
    static_assert(!std::is_invocable_v<decltype(operation)&,
        Contexte&, Prepare, int, const Support&, int&>);
}

TEST_CASE("Arguments ordinaires : references exactes et objet polymorphe non indexe") {
    Inconnu contexte;
    std::string texte = "valeur";
    Chien chien;
    auto observer = creer_methode<const std::string&(const Animal&, const Animal&,
                                                    const std::string&, std::string&&)>(
        domaines<argument_ordinaire, Animaux, argument_ordinaire, argument_ordinaire>{},
        [&](const Animal& objet, const Chien&, const std::string& valeur, std::string&& deplacable)
            -> const std::string& {
            CHECK(&objet == &contexte);
            CHECK(&valeur == &texte);
            CHECK(&deplacable == &texte);
            return valeur;
        },
        [](const Animal&, const Animal&, const std::string& valeur, std::string&&)
            -> const std::string& { return valeur; });
    CHECK(&observer(contexte, chien, texte, std::move(texte)) == &texte);
    CHECK(&observer(contexte, observer.preparer<1>(chien), texte, std::move(texte)) == &texte);
    CHECK(texte == "valeur");
}

TEST_CASE("Arguments ordinaires : possession unique et deplacement de la methode") {
    auto consommer = creer_methode<int(std::unique_ptr<int>, const Animal&)>(
        domaines<argument_ordinaire, Animaux>{},
        [bonus = std::make_unique<int>(3)](std::unique_ptr<int> valeur, const Animal&) mutable {
            return *valeur + (*bonus)++;
        });
    Chien chien;
    const auto reference = consommer.preparer<1>(chien);
    auto deplacee = std::move(consommer);
    auto valeur = std::make_unique<int>(7);
    CHECK(deplacee(std::move(valeur), chien) == 10);
    CHECK_FALSE(valeur);
    CHECK(deplacee(std::make_unique<int>(8), reference) == 12);
    // Une reference reste reutilisable avec un autre placement des arguments ordinaires.
    auto autre = creer_methode<int(const Animal&, int)>(
        domaines<Animaux, argument_ordinaire>{},
        [](const Animal&, int montant) { return montant; });
    CHECK(autre(reference, 15) == 15);
}

namespace {
/** Compte les transferts, sans dependre de l'elision des temporaires. */
struct Compteur {
    int* copies;
    int* deplacements;
    Compteur(int& copies, int& deplacements) : copies(&copies), deplacements(&deplacements) {}
    Compteur(const Compteur& autre) : copies(autre.copies), deplacements(autre.deplacements) { ++*copies; }
    // Meme sans noexcept, un deplacement disponible doit rester prefere a la copie.
    Compteur(Compteur&& autre) : copies(autre.copies), deplacements(autre.deplacements) {
        ++*deplacements;
    }
};
/** Valeur qui autorise la copie mais interdit explicitement le deplacement. */
struct Copiable {
    int* copies;
    int valeur = 42;
    explicit Copiable(int& copies) : copies(&copies) {}
    Copiable(const Copiable& autre) : copies(autre.copies), valeur(autre.valeur) { ++*copies; }
    Copiable(Copiable&&) = delete;
};
}

TEST_CASE("Arguments ordinaires : aucune copie dans les relais internes") {
    auto operation = creer_methode<void(const Animal&, Compteur)>(
        domaines<Animaux, argument_ordinaire>{}, [](const Animal&, Compteur) {});
    Chien chien;
    int copies = 0, deplacements = 0;
    Compteur valeur(copies, deplacements);
    operation(chien, valeur);
    CHECK(copies == 1);       // Entree du parametre public par valeur.
    CHECK(deplacements == 1); // Entree du parametre de la specialisation.
    copies = deplacements = 0;
    operation(operation.preparer(chien), valeur);
    CHECK(copies == 1);
    CHECK(deplacements == 1);
}

TEST_CASE("Arguments ordinaires : valeur copiable sans deplacement") {
    auto operation = creer_methode<int(const Animal&, Copiable)>(
        domaines<Animaux, argument_ordinaire>{}, [](const Animal&, Copiable valeur) {
            return ++valeur.valeur;
        });
    Chien chien;
    int copies = 0;
    const Copiable valeur(copies);
    auto verifier = [&](const auto& objet) {
        copies = 0;
        CHECK(operation(objet, valeur) == 43);
        CHECK(valeur.valeur == 42);
        CHECK(copies == 2); // Parametre public puis parametre de la specialisation.
        copies = 0;
        CHECK(operation(objet, Copiable(copies)) == 43);
        CHECK(copies == 1); // Construction directe du parametre public depuis le temporaire.
    };
    verifier(chien);
    verifier(operation.preparer(chien));
}

TEST_CASE("Arguments ordinaires : references sans copie malgre un deplacement interdit") {
    int copies = 0;
    Copiable valeur(copies);
    auto operation = creer_methode<void(Animal&, Copiable&, const Copiable&, Copiable&&)>(
        domaines<Animaux, argument_ordinaire, argument_ordinaire, argument_ordinaire>{},
        [&](Animal&, Copiable& modifiable, const Copiable& constante, Copiable&& temporaire) {
            CHECK(&modifiable == &valeur);
            CHECK(&constante == &valeur);
            CHECK(&temporaire == &valeur);
            ++modifiable.valeur;
        });
    Chien chien;
    operation(chien, valeur, valeur, std::move(valeur));
    operation(operation.preparer(chien), valeur, valeur, std::move(valeur));
    CHECK(valeur.valeur == 44);
    CHECK(copies == 0);
}

TEST_CASE("Arguments ordinaires : heritage virtuel multiple et exceptions") {
    struct Racine { virtual ~Racine() = default; };
    struct Gauche : virtual Racine {};
    struct Droite : virtual Racine {};
    struct Diamant : Gauche, Droite { int valeur = 7; };
    auto operation = creer_methode<int(int, const Racine&, bool)>(
        domaines<argument_ordinaire, liste_types<Diamant>, argument_ordinaire>{},
        [](int montant, const Diamant& objet, bool echouer) {
            if (echouer) throw std::runtime_error("traitement refuse");
            return montant + objet.valeur;
        });
    Diamant objet;
    const Racine& base = objet;
    CHECK(operation(2, base, false) == 9);
    CHECK(operation(3, operation.preparer<1>(base), false) == 10);
    CHECK_THROWS_WITH_AS(operation(0, base, true), "traitement refuse", std::runtime_error);
    CHECK_THROWS_WITH_AS(operation(0, operation.preparer<1>(base), true),
                         "traitement refuse", std::runtime_error);
}
