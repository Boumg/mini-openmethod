#pragma once
#include <mini_openmethod/detail/traits.hpp>
#include <mini_openmethod/reference_preparee.hpp>
#include <boost/mp11/algorithm.hpp>
#include <utility>

/** @file Description des arguments et projection des seules positions polymorphes. */
namespace mini_openmethod::detail {
template<class Descripteur>
inline constexpr bool descripteur_valide = false;
template<class... Types>
inline constexpr bool descripteur_valide<liste_types<Types...>> = true;
template<>
inline constexpr bool descripteur_valide<argument_ordinaire> = true;

/** Contrat d'une position polymorphe et type attendu par l'appel prepare. */
template<class Argument, class Descripteur>
struct parametre {
    static_assert(reference_constante<Argument>,
                  "La signature exige des references constantes vers des classes");
    using base = std::remove_cvref_t<Argument>;
    static_assert(std::is_polymorphic_v<base>, "Les bases doivent etre polymorphes");
    static_assert(traits_liste<Descripteur>::template valide<base>,
                  "Domaine invalide : types uniques, non qualifies, publics et non ambigus requis");
    using prepare = const reference_preparee<base, Descripteur>&;
    template<class Specialise>
    static constexpr bool accepte = reference_constante<Specialise>
        && std::derived_from<std::remove_cvref_t<Specialise>, base>;
};

/** Une position ordinaire garde exactement son type, y compris ses references. */
template<class Argument>
struct parametre<Argument, argument_ordinaire> {
    using prepare = Argument;
    template<class Specialise>
    static constexpr bool accepte = std::same_as<Argument, Specialise>;
};

template<class Tuple, class Positions> struct projection_signature;
template<class... Arguments, std::size_t... Positions>
struct projection_signature<std::tuple<Arguments...>, std::index_sequence<Positions...>> {
    using type = void(std::tuple_element_t<Positions, std::tuple<Arguments...>>...);
};

template<class... Indices>
using sequence_positions = std::index_sequence<Indices::value...>;

/** Filtre les descripteurs sans instancier traits_liste pour un argument ordinaire. */
template<class Domaines> struct description_arguments;
template<class... Descripteurs>
struct description_arguments<domaines<Descripteurs...>> {
    using descripteurs = boost::mp11::mp_list<Descripteurs...>;
    template<class Indice>
    using est_polymorphe = std::bool_constant<!std::same_as<
        boost::mp11::mp_at<descripteurs, Indice>, argument_ordinaire>>;
    using positions = boost::mp11::mp_rename<boost::mp11::mp_copy_if<
        boost::mp11::mp_iota_c<sizeof...(Descripteurs)>, est_polymorphe>, sequence_positions>;
    using domaines_polymorphes = boost::mp11::mp_rename<
        boost::mp11::mp_remove<descripteurs, argument_ordinaire>, domaines>;
    template<class Fonction>
    using signature = typename projection_signature<
        typename traits_fonction<Fonction>::arguments, positions>::type;
};
} // namespace mini_openmethod::detail
