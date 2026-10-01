#include <mini_openmethod/methode.hpp>
#include <array>
#include <doctest/doctest.h>
#include <tuple>
#include <utility>

/** @file Regression : 16 x 16 types et 256 specialisations, sans relever les limites constexpr. */
namespace {
static_assert(mini_openmethod::reflexion_active, "Ce test doit exercer la reflexion selectionnee par ON ou AUTO");
struct Animal { virtual ~Animal() = default; };
template<std::size_t Rang> struct Espece : Animal {};
struct Inconnu : Espece<0> {};
constexpr std::size_t nombre_types = 16;

template<std::size_t... Rangs, std::size_t... Cases>
void verifier_grand_domaine(std::index_sequence<Rangs...>, std::index_sequence<Cases...>) {
    using Domaine = mini_openmethod::liste_types<Espece<Rangs>...>;
    auto operation = mini_openmethod::creer_methode<std::size_t(const Animal&, const Animal&)>(
        mini_openmethod::domaines<Domaine, Domaine>{},
        [](const Espece<Cases / nombre_types>&, const Espece<Cases % nombre_types>&) { return Cases; }...);
    std::tuple<Espece<Rangs>...> objets;
    const std::array<const Animal*, nombre_types> bases{&std::get<Rangs>(objets)...};
    for (std::size_t gauche = 0; gauche < nombre_types; ++gauche) {
        for (std::size_t droite = 0; droite < nombre_types; ++droite) {
            CAPTURE(gauche);
            CAPTURE(droite);
            const auto attendu = gauche * nombre_types + droite;
            CHECK(operation(*bases[gauche], *bases[droite]) == attendu);
            CHECK(operation(operation.template preparer<0>(*bases[gauche]),
                            operation.template preparer<1>(*bases[droite])) == attendu);
        }
    }
    Inconnu inconnu;
    CHECK_THROWS_AS(operation(inconnu, *bases[0]), mini_openmethod::type_inconnu);
}
} // namespace

TEST_CASE("Reflexion sur 16 x 16 types et 256 specialisations") {
    verifier_grand_domaine(std::make_index_sequence<nombre_types>{},
                          std::make_index_sequence<nombre_types * nombre_types>{});
}
