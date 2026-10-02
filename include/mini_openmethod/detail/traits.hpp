#pragma once
#include <mini_openmethod/detail/index_types.hpp>
#include <mini_openmethod/detail/traits_fonction.hpp>
#include <mini_openmethod/domaines.hpp>
#include <boost/mp11/set.hpp>
#include <array>
#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <typeinfo>

/** @file Assistants internes d'analyse des signatures et des listes de types. */
namespace mini_openmethod::detail {
/** Reference lvalue non volatile vers une classe, constante ou modifiable. */
template<class Type>
concept reference_classe = std::is_lvalue_reference_v<Type>
    && !std::is_volatile_v<std::remove_reference_t<Type>>
    && std::is_class_v<std::remove_cvref_t<Type>>;
/** Indique si les traits peuvent extraire une signature unique. */
template<class Fonction>
concept fonction_analysee = requires {
    typename traits_fonction<Fonction>::retour;
    typename traits_fonction<Fonction>::arguments;
};
/** Verifie l'absence de doublon dans une suite de types. */
template<class... Types>
using types_uniques = boost::mp11::mp_is_set<boost::mp11::mp_list<Types...>>;
/** Valide un domaine et retrouve l'indice d'un type dynamique exact. */
template<class Liste> struct traits_liste;
template<class... Types>
struct traits_liste<liste_types<Types...>> {
    using tuple = std::tuple<Types...>;
    static constexpr std::size_t taille = sizeof...(Types);
    template<class Base>
    static constexpr bool valide = sizeof...(Types) > 0
        && types_uniques<Types...>::value
        && (std::same_as<Types, std::remove_cvref_t<Types>> && ...)
        && (std::derived_from<Types, Base> && ...);
    /** @throws type_inconnu Si le type exact est absent du domaine. */
    static std::size_t indice(const std::type_info& type) {
        if constexpr (taille <= 8) {
            return indice_lineaire<Types...>(type);
        } else {
            // Initialisation locale synchronisee par C++, puis lectures immuables.
            static const index_types<taille> index(std::array{&typeid(Types)...});
            return index.indice(type);
        }
    }
};
} // namespace mini_openmethod::detail
