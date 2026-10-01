#pragma once
#include <boost/callable_traits/args.hpp>
#include <boost/callable_traits/return_type.hpp>
#include <boost/callable_traits/has_varargs.hpp>
#include <boost/callable_traits/is_reference_member.hpp>
#include <boost/callable_traits/is_volatile_member.hpp>
#include <cstddef>
#include <tuple>
#include <type_traits>

/** @file Analyse commune des signatures C++23 et C++26 avec Boost.CallableTraits. */
namespace mini_openmethod::detail {
namespace traits_boost = boost::callable_traits;

/** Extrait le retour, les arguments et l'arite ; une signature non admise reste vide. */
template<class Fonction, class = void> struct traits_fonction {};

template<class Fonction>
    requires (std::is_function_v<Fonction>
              && !traits_boost::is_volatile_member_v<Fonction>
              && !traits_boost::is_reference_member_v<Fonction>
              && !traits_boost::has_varargs_v<Fonction>)
struct traits_fonction<Fonction, void> {
    using retour = traits_boost::return_type_t<Fonction>;
    using arguments = traits_boost::args_t<Fonction>;
    static constexpr std::size_t arite = std::tuple_size_v<arguments>;
};

// Normalise les appelables ; le parametre objet d'un membre ne fait pas partie des arguments.
template<class Fonction>
    requires std::is_function_v<Fonction>
struct traits_fonction<Fonction*, void> : traits_fonction<Fonction> {};

template<class Fonction, class Classe>
    requires std::is_function_v<Fonction>
struct traits_fonction<Fonction Classe::*, void> : traits_fonction<Fonction> {};

template<class Fonction>
struct traits_fonction<Fonction, std::void_t<decltype(&Fonction::operator())>>
    : traits_fonction<decltype(&Fonction::operator())> {};
} // namespace mini_openmethod::detail
