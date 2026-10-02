#include <mini_openmethod/detail/traits_fonction.hpp>
#include <mini_openmethod/detail/traits_fonction.hpp>
#include <mini_openmethod/detail/traits.hpp>
#include <mini_openmethod/detail/traits.hpp>
#include <doctest/doctest.h>

/** @file Verifie les assistants sans inclure le moteur de dispatch. */
namespace {
using namespace mini_openmethod;

struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};
struct Chiot : Chien {};
struct Prive : private Animal {};

/** Controle le retour, l'ordre des arguments et l'arite sans conversion implicite. */
template<class Fonction, class Retour, class... Arguments>
consteval bool signature_conforme() {
    using Traits = detail::traits_fonction<Fonction>;
    return detail::fonction_analysee<Fonction>
        && std::is_same_v<typename Traits::retour, Retour>
        && std::is_same_v<typename Traits::arguments, std::tuple<Arguments...>>
        && Traits::arite == sizeof...(Arguments);
}

using Signature = int(const Animal&, const Chien&);
static_assert(signature_conforme<Signature, int, const Animal&, const Chien&>());
static_assert(signature_conforme<Signature*, int, const Animal&, const Chien&>());
static_assert(signature_conforme<int (*)(const Chien&) noexcept, int, const Chien&>());
static_assert(signature_conforme<int(const Chien&) noexcept, int, const Chien&>());
static_assert(signature_conforme<void(), void>());
static_assert(signature_conforme<int& (*)(const Chien&), int&, const Chien&>());

// Les lambdas couvrent les operateurs constants, mutables et noexcept.
static_assert(signature_conforme<decltype([](const Chien&) { return 1; }), int, const Chien&>());
static_assert(signature_conforme<decltype([compte = 0](const Chien&) mutable { return ++compte; }),
                                  int, const Chien&>());
static_assert(signature_conforme<decltype([](const Chien&) noexcept { return 1; }), int, const Chien&>());
static_assert(signature_conforme<decltype([compte = 0](const Chien&) mutable noexcept { return ++compte; }),
                                  int, const Chien&>());

struct Surcharge {
    int operator()(const Chien&) const;
    int operator()(const Chat&) const;
};
struct Qualifie_reference { int operator()(const Chien&) &; };
struct Qualifie_reference_droite { int operator()(const Chien&) &&; };
struct Qualifie_volatile { int operator()(const Chien&) const volatile; };
struct Membres {
    int valeur;
    int traiter(const Chien&);
    int traiter_constante(const Chien&) const;
    int traiter_sans_exception(const Chien&) noexcept;
    int traiter_constante_sans_exception(const Chien&) const noexcept;
    void sans_argument() const noexcept;
};
static_assert(signature_conforme<decltype(&Membres::traiter), int, const Chien&>());
static_assert(signature_conforme<decltype(&Membres::traiter_constante), int, const Chien&>());
static_assert(signature_conforme<decltype(&Membres::traiter_sans_exception), int, const Chien&>());
static_assert(signature_conforme<decltype(&Membres::traiter_constante_sans_exception), int, const Chien&>());
// Boost inclut normalement l'objet dans args_t d'un membre : notre contrat l'exclut.
static_assert(signature_conforme<decltype(&Membres::sans_argument), void>());
static_assert(signature_conforme<int(const Chien&) const noexcept, int, const Chien&>());
static_assert(!detail::fonction_analysee<decltype([](const auto&) { return 1; })>);
static_assert(!detail::fonction_analysee<Surcharge>);
static_assert(!detail::fonction_analysee<Qualifie_reference>);
static_assert(!detail::fonction_analysee<Qualifie_reference_droite>);
static_assert(!detail::fonction_analysee<Qualifie_volatile>);
static_assert(!detail::fonction_analysee<decltype(&Membres::valeur)>);
static_assert(!detail::fonction_analysee<int (*)(const Chien&, ...)>);
static_assert(!detail::fonction_analysee<decltype([](const Chien&) { return 1; })*>);
static_assert(!detail::fonction_analysee<int>);
static_assert(!detail::fonction_analysee<int(const Chien&, ...) noexcept>);
static_assert(!detail::fonction_analysee<int(const Chien&) volatile>);
static_assert(!detail::fonction_analysee<int(const Chien&) const & noexcept>);
static_assert(!detail::fonction_analysee<int(const Chien&) &&>);
static_assert(!detail::fonction_analysee<Signature&>);
static_assert(!detail::fonction_analysee<Signature* const>);

static_assert(detail::reference_classe<const Animal&>);
static_assert(detail::reference_classe<Animal&>);
static_assert(!detail::reference_classe<const volatile Animal&>);
static_assert(!detail::reference_classe<volatile Animal&>);
static_assert(!detail::reference_classe<const Animal&&>);
static_assert(!detail::reference_classe<Animal*>);
static_assert(!detail::reference_classe<Animal>);
static_assert(!detail::reference_classe<const int&>);
static_assert(detail::types_uniques<>::value);
static_assert(detail::types_uniques<Chien>::value);
static_assert(detail::types_uniques<Chien, const Chien, Chien&>::value);
static_assert(!detail::types_uniques<Chien, Chien>::value);
static_assert(detail::types_uniques<Chien, Chat>::value);
static_assert(!detail::types_uniques<Chien, Chat, Chien>::value);

using Traits_animaux = detail::traits_liste<liste_types<Chien, Chat>>;
static_assert(Traits_animaux::taille == 2);
static_assert(std::is_same_v<Traits_animaux::tuple, std::tuple<Chien, Chat>>);
static_assert(Traits_animaux::valide<Animal>);
static_assert(!detail::traits_liste<liste_types<>>::valide<Animal>);
static_assert(!detail::traits_liste<liste_types<Chien, Chien>>::valide<Animal>);
static_assert(!detail::traits_liste<liste_types<const Chien>>::valide<Animal>);
static_assert(!detail::traits_liste<liste_types<Prive>>::valide<Animal>);
} // namespace

/** Controle l'ordre des indices et le rejet d'un descendant non declare. */
TEST_CASE("Indices des types declares et rejet d'un descendant inconnu") {
    CHECK(Traits_animaux::indice(typeid(Chien)) == 0);
    CHECK(Traits_animaux::indice(typeid(Chat)) == 1);
    CHECK_THROWS_AS(Traits_animaux::indice(typeid(Chiot)), mini_openmethod::type_inconnu);
}
