#include <mini_openmethod/methode.hpp>
#include <doctest/doctest.h>
#include <array>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

/** @file Mutations des objets dispatches et preservation des droits d'acces. */
namespace {
using namespace mini_openmethod;
struct Animal {
    virtual ~Animal() = default;
    Animal() = default;
    Animal(const Animal&) = delete; // Une copie accidentelle doit empecher la compilation.
    int energie = 0;
};
struct Mammifere : Animal {};
struct Chien : Mammifere {};
struct Chat : Mammifere {};
struct Oiseau : Animal {};
struct Chiot : Chien {};
using Animaux = liste_types<Chien, Chat, Oiseau>;
using Prepare = reference_preparee<Animal, Animaux>;
using Prepare_modifiable = reference_preparee<Animal, Animaux, true>;
static_assert(std::is_convertible_v<Prepare_modifiable, Prepare>);
static_assert(!std::is_convertible_v<Prepare, Prepare_modifiable>);
static_assert(!std::is_constructible_v<Prepare_modifiable, Prepare>);
static_assert(!std::is_default_constructible_v<Prepare_modifiable>);
static_assert(!std::is_constructible_v<Prepare_modifiable, Animal&, std::size_t>);
static_assert(std::is_trivially_copyable_v<Prepare_modifiable>);
static_assert(sizeof(Prepare) == sizeof(Prepare_modifiable));
static_assert(std::same_as<decltype(std::declval<const Prepare&>().objet()), const Animal&>);
static_assert(std::same_as<decltype(std::declval<const Prepare_modifiable&>().objet()), Animal&>);

TEST_CASE("Objets modifiables : identite repli et type inconnu") {
    auto nourrir = creer_methode<Animal&(Animal&)>(domaines<Animaux>{},
        [](Animal& animal) -> Animal& { ++animal.energie; return animal; },
        [](Mammifere& animal) -> Animal& { animal.energie += 2; return animal; },
        [](Chien& chien) -> Animal& { chien.energie += 3; return chien; });
    Chien chien; Chat chat; Oiseau oiseau;
    const std::array<Animal*, 3> animaux{&chien, &chat, &oiseau};
    const std::array<int, 3> attendus{3, 2, 1};
    for (std::size_t indice = 0; indice < animaux.size(); ++indice) {
        auto& animal = *animaux[indice];
        CHECK(&nourrir(animal) == &animal);
        CHECK(animal.energie == attendus[indice]);
        const auto prepare = nourrir.preparer(animal);
        CHECK(&nourrir(prepare) == &animal);
        CHECK(animal.energie == 2 * attendus[indice]);
    }
    static_assert(std::is_invocable_v<decltype(nourrir)&, Chien&>);
    static_assert(!std::is_invocable_v<decltype(nourrir)&, const Chien&>);
    static_assert(!std::is_invocable_v<decltype(nourrir)&, Chien&&>);
    static_assert(!std::is_invocable_v<decltype(nourrir)&, Prepare>);
    Chiot inconnu;
    CHECK_THROWS_AS(nourrir(inconnu), type_inconnu);
    CHECK_THROWS_AS((void)nourrir.preparer(inconnu), type_inconnu);
    CHECK(inconnu.energie == 0);
}

template<bool Constante, class Type>
using reference = std::conditional_t<Constante, const Type&, Type&>;

/** Chaque combinaison doit choisir les memes specialisations avec les droits annonces. */
template<bool Gauche_constante, bool Droite_constante>
void verifier_double() {
    using Gauche = reference<Gauche_constante, Animal>;
    using Droite = reference<Droite_constante, Animal>;
    const auto modifier = []([[maybe_unused]] Gauche gauche, [[maybe_unused]] Droite droite, int valeur) {
        if constexpr (!Gauche_constante) gauche.energie += valeur;
        if constexpr (!Droite_constante) droite.energie += 10 * valeur;
        return valeur;
    };
    auto operation = creer_methode<int(Gauche, Droite)>(domaines<Animaux, Animaux>{},
        [modifier](Gauche gauche, Droite droite) { return modifier(gauche, droite, 1); },
        [modifier](reference<Gauche_constante, Chien> gauche, Droite droite) {
            return modifier(gauche, droite, 2);
        },
        [modifier](Gauche gauche, reference<Droite_constante, Chat> droite) {
            return modifier(gauche, droite, 3);
        },
        [modifier](reference<Gauche_constante, Chien> gauche, reference<Droite_constante, Chat> droite) {
            return modifier(gauche, droite, 4);
        });
    Chien chien_gauche, chien_droite;
    Chat chat_gauche, chat_droite;
    const std::array<Animal*, 2> gauches{&chien_gauche, &chat_gauche};
    const std::array<Animal*, 2> droites{&chien_droite, &chat_droite};
    const int attendus[2][2]{{2, 4}, {1, 3}};
    for (std::size_t gauche = 0; gauche < 2; ++gauche) {
        for (std::size_t droite = 0; droite < 2; ++droite) {
            CAPTURE(Gauche_constante);
            CAPTURE(Droite_constante);
            CAPTURE(gauche);
            CAPTURE(droite);
            gauches[gauche]->energie = droites[droite]->energie = 0;
            const int attendu = attendus[gauche][droite];
            CHECK(operation(*gauches[gauche], *droites[droite]) == attendu);
            CHECK(operation(operation.template preparer<0>(*gauches[gauche]),
                            operation.template preparer<1>(*droites[droite])) == attendu);
            CHECK(gauches[gauche]->energie == (Gauche_constante ? 0 : 2 * attendu));
            CHECK(droites[droite]->energie == (Droite_constante ? 0 : 20 * attendu));
        }
    }
}

TEST_CASE("Objets modifiables : qualifications independantes du double dispatch") {
    verifier_double<false, false>();
    verifier_double<false, true>();
    verifier_double<true, false>();
    verifier_double<true, true>();
}

TEST_CASE("Objets modifiables : preparation conversion constante et reutilisation") {
    auto modifier = creer_methode<int(Animal&)>(domaines<Animaux>{},
        [](Animal& animal) { return ++animal.energie; });
    auto lire = creer_methode<int(const Animal&)>(domaines<Animaux>{},
        [](const Animal& animal) { return animal.energie; });
    Chien chien;
    const auto prepare = std::as_const(modifier).preparer(chien);
    static_assert(std::same_as<std::remove_cv_t<decltype(prepare)>, Prepare_modifiable>);
    static_assert(std::same_as<decltype(lire.preparer(chien)), Prepare>);
    CHECK(&prepare.objet() == &chien);
    auto copie = prepare;
    const Prepare lecture = prepare;
    CHECK(&lecture.objet() == &chien);
    CHECK(modifier(copie) == 1);
    CHECK(lire(lecture) == 1);
    CHECK(lire(prepare) == 1); // Conversion implicite, sans nouvelle preparation.
    auto deplacee = std::move(modifier);
    CHECK(deplacee(prepare) == 2);
    prepare.objet().energie = 8; // Le const du descripteur ne rend pas l'objet constant.
    CHECK(lire(lecture) == 8);
    auto autre = creer_methode<int(Animal&)>(domaines<Animaux>{},
        [](Animal& animal) { return animal.energie += 2; });
    CHECK(autre(prepare) == 10);
    static_assert(!std::is_convertible_v<Prepare_modifiable,
        reference_preparee<Animal, liste_types<Chat, Chien, Oiseau>>>);
    static_assert(!std::is_convertible_v<Prepare_modifiable,
        reference_preparee<Mammifere, Animaux>>);
    const Chat chat;
    CHECK(lire(lire.preparer(chat)) == 0);
}

TEST_CASE("Objets modifiables : arguments ordinaires et positions de preparation") {
    struct Support { virtual ~Support() = default; int coefficient = 3; };
    struct Papier : Support {};
    struct Contexte { int appels = 0; };
    auto operation = creer_methode<void(Contexte&, Animal&, std::unique_ptr<int>, const Support&, int&)>(
        domaines<argument_ordinaire, Animaux, argument_ordinaire,
                 liste_types<Papier>, argument_ordinaire>{},
        [](Contexte& contexte, Animal& animal, std::unique_ptr<int> ajout,
           const Support& support, int& resultat) {
            ++contexte.appels;
            animal.energie += *ajout * support.coefficient;
            resultat = animal.energie;
        });
    Chien chien;
    const Papier papier;
    Contexte contexte;
    int resultat = 0;
    operation(contexte, chien, std::make_unique<int>(2), papier, resultat);
    CHECK(resultat == 6);
    const auto gauche = operation.preparer<1>(chien);
    const auto droite = operation.preparer<3>(papier);
    operation(contexte, gauche, std::make_unique<int>(4), droite, resultat);
    CHECK(chien.energie == 18);
    CHECK(resultat == 18);
    CHECK(contexte.appels == 2);
    CHECK(papier.coefficient == 3);
}

TEST_CASE("Objets modifiables : ajustements en heritage multiple et virtuel") {
    struct Etiquette { virtual ~Etiquette() = default; int valeur = 17; };
    struct Etiquete : Etiquette, Chien {};
    auto multiple = creer_methode<void(Animal&)>(domaines<liste_types<Etiquete>>{},
        [](Etiquete& objet) { ++objet.valeur; objet.energie += 3; });
    Etiquete etiquete;
    Animal& base = etiquete;
    multiple(base);
    multiple(multiple.preparer(base));
    CHECK(etiquete.valeur == 19);
    CHECK(etiquete.energie == 6);
    struct Gauche : virtual Animal { int gauche = 0; };
    struct Droite : virtual Animal { int droite = 0; };
    struct Diamant : Gauche, Droite {};
    auto intersection = creer_methode<void(Animal&)>(domaines<liste_types<Diamant>>{},
        [](Gauche& objet) { ++objet.gauche; },
        [](Droite& objet) { ++objet.droite; },
        [](Diamant& objet) { objet.gauche += 2; objet.droite += 3; ++objet.energie; });
    auto repli = creer_methode<void(Animal&)>(domaines<liste_types<Diamant>>{},
        [](Droite& objet) { objet.droite += 7; });
    Diamant diamant;
    Animal& racine = diamant;
    intersection(racine);
    const auto prepare = intersection.preparer(racine);
    intersection(prepare);
    repli(prepare);
    CHECK(diamant.gauche == 4);
    CHECK(diamant.droite == 13);
    CHECK(diamant.energie == 2);
}

TEST_CASE("Objets modifiables : exceptions et mutation visible avant propagation") {
    auto echec = creer_methode<void(Animal&)>(domaines<Animaux>{},
        [](Animal& animal) { ++animal.energie; throw std::logic_error("Mutation effectuee"); });
    Chien chien;
    CHECK_THROWS_WITH_AS(echec(chien), "Mutation effectuee", std::logic_error);
    CHECK(chien.energie == 1);
    CHECK_THROWS_WITH_AS(echec(echec.preparer(chien)), "Mutation effectuee", std::logic_error);
    CHECK(chien.energie == 2);
}
} // namespace
