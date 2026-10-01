#pragma once
#include <mini_openmethod/detail/resolution.hpp>
#include <mini_openmethod/detail/traits.hpp>
#include <mini_openmethod/domaines.hpp>
#include <mini_openmethod/reference_preparee.hpp>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>

/** @file Dispatch dynamique externe avec validation d'un domaine ferme. */
namespace mini_openmethod {
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
    static constexpr auto resolution = detail::resolution<domaines<Listes...>, tuple_fonctions>::indices;

    /** Le type exact a deja ete valide par l'indexation RTTI avant cet ajustement. */
    template<class Cible, class Base>
    static Cible ajuster_argument(const Base& argument) {
        if constexpr (requires { static_cast<Cible>(argument); })
            return static_cast<Cible>(argument);
        else
            return dynamic_cast<Cible>(argument);
    }
    template<std::size_t Fonction, std::size_t... Positions>
    Retour invoquer(std::index_sequence<Positions...>, Arguments... arguments) {
        // Conversion statique lorsque possible, RTTI pour les bases virtuelles.
        return std::invoke(std::get<Fonction>(fonctions_),
            ajuster_argument<argument_fonction<Fonction, Positions>>(arguments)...);
    }
    template<std::size_t Fonction>
    static Retour relais(methode& operation, Arguments... arguments) {
        return operation.template invoquer<Fonction>(positions{}, arguments...);
    }
    using pointeur_relais = Retour (*)(methode&, Arguments...);
    template<std::size_t Case>
    static consteval pointeur_relais choisir_relais() {
        constexpr auto gagnant = resolution[Case];
        static_assert(gagnant != nombre_fonctions, "Combinaison sans specialisation applicable");
        static_assert(gagnant != nombre_fonctions + 1, "Ambiguite : plusieurs specialisations maximales");
        if constexpr (gagnant < nombre_fonctions) return &relais<gagnant>;
        else return nullptr;
    }
    template<std::size_t... Cases>
    static consteval auto creer_table(std::index_sequence<Cases...>) {
        return std::array<pointeur_relais, nombre_cases>{choisir_relais<Cases>()...};
    }
    static constexpr auto table = creer_table(std::make_index_sequence<nombre_cases>{});
    tuple_fonctions fonctions_;

    Retour appeler_indices(const std::array<std::size_t, arite>& indices, Arguments... arguments) {
        std::size_t case_table = 0;
        for (std::size_t position = 0; position < arite; ++position)
            case_table = case_table * dimensions[position] + indices[position];
        return table[case_table](*this, arguments...);
    }
public:
    /** Force la validation, meme si la methode n'est jamais appelee. */
    explicit constexpr methode(Fonctions... fonctions) : fonctions_(std::move(fonctions)...) {
        static_assert(table.size() == nombre_cases);
    }
    /**
     * Appelle la specialisation correspondant aux types dynamiques exacts.
     * @throws type_inconnu Si un objet est absent du domaine.
     * Propage sans modification les exceptions des specialisations.
     */
    Retour operator()(Arguments... arguments) {
        const std::array<std::size_t, arite> indices{detail::traits_liste<Listes>::indice(typeid(arguments))...};
        return appeler_indices(indices, arguments...);
    }
    /**
     * Valide le type une fois et memorise son indice pour les appels repetes.
     * @tparam Position Position dans la signature, a partir de zero (zero par defaut).
     * @throws type_inconnu Si le type exact est absent du domaine de cette position.
     * Les temporaires sont refuses pour eviter une reference immediatement pendante.
     */
    template<std::size_t Position = 0, class Objet>
    [[nodiscard]] auto preparer(Objet&& objet) const {
        static_assert(Position < arite, "Position de preparation hors de la signature");
        static_assert(std::is_lvalue_reference_v<Objet>, "La preparation exige un objet persistant, pas un temporaire");
        if constexpr (Position < arite && std::is_lvalue_reference_v<Objet>) {
            using Base = std::remove_cvref_t<std::tuple_element_t<Position, tuple_arguments>>;
            constexpr bool compatible = std::is_convertible_v<std::remove_reference_t<Objet>*, const Base*>;
            static_assert(compatible, "Objet incompatible avec la racine de cette position");
            if constexpr (compatible) {
                using Domaine = std::tuple_element_t<Position, tuple_listes>;
                return reference_preparee<Base, Domaine>(objet, liste<Position>::indice(typeid(objet)));
            }
        }
    }
    /** Appel sans nouvelle identification RTTI ; chaque reference doit rester valide. */
    Retour operator()(const reference_preparee<std::remove_cvref_t<Arguments>, Listes>&... arguments) {
        return appeler_indices({arguments.indice_...}, *arguments.objet_...);
    }
};
/** Deduit les signatures explicites des lambdas et conserve celles-ci par valeur. */
template<class Signature, class... Listes, class... Fonctions>
[[nodiscard]] constexpr auto creer_methode(domaines<Listes...>, Fonctions&&... fonctions) {
    return methode<Signature, domaines<Listes...>, std::decay_t<Fonctions>...>(
        std::forward<Fonctions>(fonctions)...);
}
} // namespace mini_openmethod
