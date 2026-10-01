#include <mini_openmethod/reference_preparee.hpp>
#include <mini_openmethod/reference_preparee.hpp>
#include <mini_openmethod/detail/index_types.hpp>
#include <mini_openmethod/detail/index_types.hpp>
#include <mini_openmethod/methode.hpp>
#include <array>
#include <doctest/doctest.h>
#include <limits>
#include <tuple>
#include <utility>

/** @file Controle les grands domaines, les collisions et les rejets, meme en Release. */
namespace {
struct Animal { virtual ~Animal() = default; };
template<std::size_t Rang> struct Espece : Animal {};
struct Descendant_inconnu : Espece<0> {};

/** Force tous les types dans la derniere case, y compris le passage par zero. */
struct Hachage_collision {
    std::size_t operator()(const std::type_info*) const noexcept {
        return std::numeric_limits<std::size_t>::max();
    }
};

template<std::size_t... Rangs>
void verifier_domaine(std::index_sequence<Rangs...>) {
    using namespace mini_openmethod;
    using Domaine = liste_types<Espece<Rangs>...>;
    using Traits = detail::traits_liste<Domaine>;
    const std::array types{&typeid(Espece<Rangs>)...};
    const detail::index_types<sizeof...(Rangs), Hachage_collision> collisions(types);
    std::tuple<Espece<Rangs>...> objets;
    const std::array<const Animal*, sizeof...(Rangs)> bases{&std::get<Rangs>(objets)...};
    auto operation = creer_methode<std::size_t(const Animal&)>(domaines<Domaine>{},
        [](const Espece<Rangs>&) { return Rangs; }...);
    for (std::size_t rang = 0; rang < bases.size(); ++rang) {
        CAPTURE(rang);
        CHECK(Traits::indice(typeid(*bases[rang])) == rang);
        CHECK(collisions.indice(typeid(*bases[rang])) == rang);
        CHECK(operation(*bases[rang]) == rang);
        CHECK(operation(operation.preparer(*bases[rang])) == rang);
    }
    Descendant_inconnu inconnu;
    CHECK_THROWS_AS(Traits::indice(typeid(inconnu)), type_inconnu);
    CHECK_THROWS_AS(collisions.indice(typeid(inconnu)), type_inconnu);
    CHECK_THROWS_AS(operation(inconnu), type_inconnu);
    CHECK_THROWS_AS((void)operation.preparer(inconnu), type_inconnu);
    CHECK_THROWS_AS(collisions.indice(typeid(Animal)), type_inconnu);
}
} // namespace

TEST_CASE("Indexation avec 1 type") { verifier_domaine(std::make_index_sequence<1>{}); }
TEST_CASE("Indexation avec 2 types") { verifier_domaine(std::make_index_sequence<2>{}); }
TEST_CASE("Indexation avec 4 types") { verifier_domaine(std::make_index_sequence<4>{}); }
TEST_CASE("Indexation avec 5 types") { verifier_domaine(std::make_index_sequence<5>{}); }
TEST_CASE("Indexation avec 8 types") { verifier_domaine(std::make_index_sequence<8>{}); }
TEST_CASE("Indexation avec 9 types") { verifier_domaine(std::make_index_sequence<9>{}); }
TEST_CASE("Indexation avec 32 types") { verifier_domaine(std::make_index_sequence<32>{}); }
