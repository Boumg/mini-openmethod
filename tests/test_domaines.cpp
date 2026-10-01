#include <mini_openmethod/domaines.hpp>
#include <mini_openmethod/domaines.hpp>
#include <type_traits>

/** @file Verifie l'inclusion autonome des declarations publiques de domaine. */
namespace {
using Animaux = mini_openmethod::liste_types<struct Chien, struct Chat>;
using Domaines = mini_openmethod::domaines<Animaux, Animaux>;
static_assert(std::is_empty_v<Animaux>);
static_assert(std::is_empty_v<Domaines>);
static_assert(std::is_base_of_v<std::runtime_error, mini_openmethod::type_inconnu>);
} // namespace
