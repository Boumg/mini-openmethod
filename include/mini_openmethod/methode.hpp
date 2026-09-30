#pragma once
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>

/** @file Dispatch dynamique externe avec validation d'un domaine ferme. */
namespace mini_openmethod {
/** Types dynamiques exacts autorises pour une position. */
template<class... Types> struct liste_types {};
/** Un domaine par argument, dans l'ordre de la signature. */
template<class... Listes> struct domaines {};
/** Un type dynamique absent du domaine est toujours refuse. */
class type_inconnu : public std::runtime_error {
public:
    type_inconnu() : std::runtime_error("Type dynamique absent du domaine ferme") {}
};

namespace detail {
template<class Fonction, class = void> struct traits_fonction {};
template<class Retour, class... Arguments>
struct traits_fonction<Retour(Arguments...), void> {
    using retour = Retour;
    using arguments = std::tuple<Arguments...>;
    static constexpr std::size_t arite = sizeof...(Arguments);
};
template<class Retour, class... Arguments>
struct traits_fonction<Retour (*)(Arguments...), void>
    : traits_fonction<Retour(Arguments...)> {};
template<class Retour, class... Arguments>
struct traits_fonction<Retour (*)(Arguments...) noexcept, void>
    : traits_fonction<Retour(Arguments...)> {};
template<class Classe, class Retour, class... Arguments>
struct traits_fonction<Retour (Classe::*)(Arguments...), void>
    : traits_fonction<Retour(Arguments...)> {};
template<class Classe, class Retour, class... Arguments>
struct traits_fonction<Retour (Classe::*)(Arguments...) const, void>
    : traits_fonction<Retour(Arguments...)> {};
template<class Classe, class Retour, class... Arguments>
struct traits_fonction<Retour (Classe::*)(Arguments...) noexcept, void>
    : traits_fonction<Retour(Arguments...)> {};
template<class Classe, class Retour, class... Arguments>
struct traits_fonction<Retour (Classe::*)(Arguments...) const noexcept, void>
    : traits_fonction<Retour(Arguments...)> {};
template<class Fonction>
struct traits_fonction<Fonction, std::void_t<decltype(&Fonction::operator())>>
    : traits_fonction<decltype(&Fonction::operator())> {};
template<class Type>
concept reference_constante = std::is_lvalue_reference_v<Type>
    && std::is_const_v<std::remove_reference_t<Type>>
    && !std::is_volatile_v<std::remove_reference_t<Type>>
    && std::is_class_v<std::remove_cvref_t<Type>>;
template<class Fonction>
concept fonction_analysee = requires {
    typename traits_fonction<Fonction>::retour;
    typename traits_fonction<Fonction>::arguments;
};
template<class... Types> struct types_uniques : std::true_type {};
template<class Premier, class... Suite>
struct types_uniques<Premier, Suite...>
    : std::bool_constant<(!std::same_as<Premier, Suite> && ...)
                         && types_uniques<Suite...>::value> {};
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
    static std::size_t indice(const std::type_info& type) {
        const std::array<bool, sizeof...(Types)> correspondances{(type == typeid(Types))...};
        for (std::size_t indice = 0; indice < correspondances.size(); ++indice)
            if (correspondances[indice]) return indice;
        throw type_inconnu{};
    }
};
} // namespace detail

template<class Signature, class Domaines, class... Fonctions> class methode;
/**
 * Methode externe validee pour toutes les combinaisons du domaine.
 * @tparam Arguments Une ou deux references constantes vers des bases polymorphes.
 * Les fonctions sont conservees par valeur, sans registre global.
 */
template<class Retour, class... Arguments, class... Listes, class... Fonctions>
class methode<Retour(Arguments...), domaines<Listes...>, Fonctions...> {
    static constexpr std::size_t arite = sizeof...(Arguments);
    static constexpr std::size_t nombre_fonctions = sizeof...(Fonctions);
    static_assert(arite == 1 || arite == 2, "Une ou deux positions polymorphes sont requises");
    static_assert(sizeof...(Listes) == arite, "Un domaine par argument est requis");
    static_assert((detail::reference_constante<Arguments> && ...),
                  "La signature exige des references constantes vers des classes");
    static_assert((std::is_polymorphic_v<std::remove_cvref_t<Arguments>> && ...),
                  "Les bases doivent etre polymorphes");
    static_assert(nombre_fonctions > 0, "Au moins une specialisation est requise");
    static_assert((detail::fonction_analysee<Fonctions> && ...),
                  "Une signature explicite de lambda ou de fonction est requise");
    using tuple_arguments = std::tuple<Arguments...>;
    using tuple_listes = std::tuple<Listes...>;
    using tuple_fonctions = std::tuple<Fonctions...>;
    using positions = std::make_index_sequence<arite>;
    template<std::size_t Position>
    using liste = detail::traits_liste<std::tuple_element_t<Position, tuple_listes>>;
    template<std::size_t Fonction, std::size_t Position>
    using argument_fonction = std::tuple_element_t<Position,
        typename detail::traits_fonction<std::tuple_element_t<Fonction, tuple_fonctions>>::arguments>;

    template<std::size_t... Positions>
    static consteval bool domaines_valides(std::index_sequence<Positions...>) {
        return (liste<Positions>::template valide<
            std::remove_cvref_t<std::tuple_element_t<Positions, tuple_arguments>>> && ...);
    }
    static_assert(domaines_valides(positions{}),
                  "Domaine invalide : types uniques, non qualifies, publics et non ambigus requis");
    template<class Fonction, std::size_t... Positions>
    static consteval bool fonction_valide(std::index_sequence<Positions...>) {
        if constexpr (!detail::fonction_analysee<Fonction>) return false;
        else if constexpr (detail::traits_fonction<Fonction>::arite != arite) return false;
        else {
            using traits = detail::traits_fonction<Fonction>;
            using parametres = typename traits::arguments;
            return std::same_as<Retour, typename traits::retour>
                && (detail::reference_constante<std::tuple_element_t<Positions, parametres>> && ...)
                && (std::derived_from<std::remove_cvref_t<std::tuple_element_t<Positions, parametres>>,
                    std::remove_cvref_t<std::tuple_element_t<Positions, tuple_arguments>>> && ...);
        }
    }
    static_assert((fonction_valide<Fonctions>(positions{}) && ...),
                  "Specialisation invalide : arite, retour exact et references constantes derives requis");
    static constexpr std::array<std::size_t, arite> dimensions{detail::traits_liste<Listes>::taille...};
    static constexpr std::size_t nombre_cases = (detail::traits_liste<Listes>::taille * ...);
    // En double dispatch : ligne * nombre de colonnes + colonne.
    template<std::size_t Case, std::size_t Position>
    static consteval std::size_t coordonnee() {
        std::size_t diviseur = 1;
        for (std::size_t suivante = Position + 1; suivante < arite; ++suivante)
            diviseur *= dimensions[suivante];
        return (Case / diviseur) % dimensions[Position];
    }
    template<std::size_t Case, std::size_t Fonction, std::size_t... Positions>
    static consteval bool applicable(std::index_sequence<Positions...>) {
        return (std::derived_from<
            std::tuple_element_t<coordonnee<Case, Positions>(), typename liste<Positions>::tuple>,
            std::remove_cvref_t<argument_fonction<Fonction, Positions>>> && ...);
    }
    // Ordre produit : au moins aussi specialise partout, strictement quelque part.
    template<std::size_t Gauche, std::size_t Droite, std::size_t... Positions>
    static consteval bool domine(std::index_sequence<Positions...>) {
        return (std::derived_from<std::remove_cvref_t<argument_fonction<Gauche, Positions>>,
                                 std::remove_cvref_t<argument_fonction<Droite, Positions>>> && ...)
            && (!std::same_as<argument_fonction<Gauche, Positions>,
                             argument_fonction<Droite, Positions>> || ...);
    }
    template<std::size_t Gauche, std::size_t... Droites>
    static consteval auto ligne_dominance(std::index_sequence<Droites...>) {
        return std::array<bool, nombre_fonctions>{domine<Gauche, Droites>(positions{})...};
    }
    template<std::size_t... Gauches>
    static consteval auto creer_dominance(std::index_sequence<Gauches...> indices) {
        return std::array<std::array<bool, nombre_fonctions>, nombre_fonctions>{ligne_dominance<Gauches>(indices)...};
    }
    static constexpr auto dominance = creer_dominance(std::make_index_sequence<nombre_fonctions>{});
    template<std::size_t Case, std::size_t... Indices>
    static consteval std::size_t resoudre(std::index_sequence<Indices...>) {
        constexpr std::array<bool, nombre_fonctions> candidats{applicable<Case, Indices>(positions{})...};
        std::size_t gagnant = nombre_fonctions;
        std::size_t maximaux = 0;
        for (std::size_t candidat = 0; candidat < nombre_fonctions; ++candidat) {
            if (!candidats[candidat]) continue;
            bool domine_par_autre = false;
            for (std::size_t autre = 0; autre < nombre_fonctions; ++autre)
                domine_par_autre = domine_par_autre || (candidats[autre] && dominance[autre][candidat]);
            if (!domine_par_autre) { gagnant = candidat; ++maximaux; }
        }
        return maximaux > 1 ? nombre_fonctions + 1 : gagnant;
    }
    template<std::size_t Case>
    static consteval std::size_t resoudre_verifie() {
        constexpr auto resultat = resoudre<Case>(std::make_index_sequence<nombre_fonctions>{});
        static_assert(resultat != nombre_fonctions, "Combinaison sans specialisation applicable");
        static_assert(resultat != nombre_fonctions + 1, "Ambiguite : plusieurs specialisations maximales");
        return resultat;
    }
    template<std::size_t... Cases>
    static consteval auto creer_resolution(std::index_sequence<Cases...>) {
        return std::array<std::size_t, nombre_cases>{resoudre_verifie<Cases>()...};
    }
    static constexpr auto resolution = creer_resolution(std::make_index_sequence<nombre_cases>{});
    template<std::size_t Fonction, std::size_t... Positions>
    Retour invoquer(std::index_sequence<Positions...>, Arguments... arguments) {
        // Ajuste aussi les pointeurs dans les heritages multiples et virtuels.
        return std::invoke(std::get<Fonction>(fonctions_),
            dynamic_cast<argument_fonction<Fonction, Positions>>(arguments)...);
    }
    template<std::size_t Fonction>
    static Retour relais(methode& operation, Arguments... arguments) {
        return operation.template invoquer<Fonction>(positions{}, arguments...);
    }
    using pointeur_relais = Retour (*)(methode&, Arguments...);
    template<std::size_t... Cases>
    static consteval auto creer_table(std::index_sequence<Cases...>) {
        return std::array<pointeur_relais, nombre_cases>{&relais<resolution[Cases]>...};
    }
    static constexpr auto table = creer_table(std::make_index_sequence<nombre_cases>{});
    tuple_fonctions fonctions_;
public:
    /** Force la validation, meme si la methode n'est jamais appelee. */
    explicit constexpr methode(Fonctions... fonctions) : fonctions_(std::move(fonctions)...) {
        static_assert(resolution.size() == nombre_cases);
    }
    /**
     * Appelle la specialisation correspondant aux types dynamiques exacts.
     * @throws type_inconnu Si un objet est absent du domaine.
     * Propage sans modification les exceptions des specialisations.
     */
    Retour operator()(Arguments... arguments) {
        const std::array<std::size_t, arite> indices{detail::traits_liste<Listes>::indice(typeid(arguments))...};
        std::size_t case_table = 0;
        for (std::size_t position = 0; position < arite; ++position)
            case_table = case_table * dimensions[position] + indices[position];
        return table[case_table](*this, arguments...);
    }
};
/** Deduit les signatures explicites des lambdas et conserve celles-ci par valeur. */
template<class Signature, class... Listes, class... Fonctions>
[[nodiscard]] constexpr auto creer_methode(domaines<Listes...>, Fonctions&&... fonctions) {
    return methode<Signature, domaines<Listes...>, std::decay_t<Fonctions>...>(
        std::forward<Fonctions>(fonctions)...);
}
} // namespace mini_openmethod
