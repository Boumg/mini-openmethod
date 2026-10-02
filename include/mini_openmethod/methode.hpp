#pragma once
#include <mini_openmethod/detail/arguments.hpp>
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
/** Diagnostique la forme des descripteurs avant d'associer les paquets. */
template<class Retour, class... Arguments, class... Descripteurs, class... Fonctions>
class methode<Retour(Arguments...), domaines<Descripteurs...>, Fonctions...> {
    static_assert(sizeof...(Arguments) == sizeof...(Descripteurs),
                  "Un descripteur par argument est requis");
    static_assert((detail::descripteur_valide<Descripteurs> && ...),
                  "Descripteur invalide : liste_types ou argument_ordinaire requis");
};
/**
 * Methode externe validee pour toutes les combinaisons du domaine.
 * @tparam Arguments Signature complete, dont une ou deux positions polymorphes T& ou const T&.
 * Les fonctions sont conservees par valeur, sans registre global.
 */
template<class Retour, class... Arguments, class... Listes, class... Fonctions>
    requires (sizeof...(Arguments) == sizeof...(Listes)
              && (detail::descripteur_valide<Listes> && ...))
class methode<Retour(Arguments...), domaines<Listes...>, Fonctions...> {
    using description = detail::description_arguments<domaines<Listes...>>;
    using positions_polymorphes = typename description::positions;
    static constexpr std::size_t arite = positions_polymorphes::size();
    static constexpr std::size_t arite_totale = sizeof...(Arguments);
    static constexpr std::size_t nombre_fonctions = sizeof...(Fonctions);
    static_assert(arite == 1 || arite == 2, "Une ou deux positions polymorphes sont requises");
    static_assert(nombre_fonctions > 0, "Au moins une specialisation est requise");
    static_assert((detail::fonction_analysee<Fonctions> && ...),
                  "Une signature explicite de lambda ou de fonction est requise");
    using tuple_arguments = std::tuple<Arguments...>;
    using tuple_listes = std::tuple<Listes...>;
    using tuple_fonctions = std::tuple<Fonctions...>;
    using positions = std::make_index_sequence<arite_totale>;
    template<std::size_t Position>
    using liste = detail::traits_liste<std::tuple_element_t<Position, tuple_listes>>;
    template<std::size_t Fonction, std::size_t Position>
    using argument_fonction = std::tuple_element_t<Position,
        typename detail::traits_fonction<std::tuple_element_t<Fonction, tuple_fonctions>>::arguments>;

    template<class Fonction, std::size_t... Positions>
    static consteval bool fonction_valide(std::index_sequence<Positions...>) {
        if constexpr (!detail::fonction_analysee<Fonction>) return false;
        else if constexpr (detail::traits_fonction<Fonction>::arite != arite_totale) return false;
        else {
            using traits = detail::traits_fonction<Fonction>;
            using parametres = typename traits::arguments;
            return std::same_as<Retour, typename traits::retour>
                && (detail::parametre<Arguments, Listes>::template accepte<
                    std::tuple_element_t<Positions, parametres>> && ...);
        }
    }
    static_assert((fonction_valide<Fonctions>(positions{}) && ...),
                  "Specialisation invalide : arite, retour exact, types ordinaires identiques et references derivees de meme qualification const requis");
    template<std::size_t... Positions>
    static consteval auto dimensions_domaines(std::index_sequence<Positions...>) {
        return std::array<std::size_t, arite>{liste<Positions>::taille...};
    }
    static constexpr auto dimensions = dimensions_domaines(positions_polymorphes{});
    static consteval auto resoudre() {
        if constexpr (arite >= 1 && arite <= 2 && nombre_fonctions > 0
                      && (fonction_valide<Fonctions>(positions{}) && ...))
            return detail::resolution<typename description::domaines_polymorphes,
                std::tuple<typename description::template signature<Fonctions>...>>::indices;
        else return std::array<std::size_t, 0>{};
    }
    static constexpr auto resolution = resoudre();

    /** Le type exact a deja ete valide par l'indexation RTTI avant cet ajustement. */
    template<class Cible, class Descripteur, class Argument>
    static decltype(auto) ajuster_argument(Argument&& argument) {
        if constexpr (std::same_as<Descripteur, argument_ordinaire>)
            return std::forward<Argument>(argument);
        else if constexpr (requires { static_cast<Cible>(argument); })
            return static_cast<Cible>(argument);
        else
            return dynamic_cast<Cible>(argument);
    }
    template<std::size_t Fonction, std::size_t... Positions>
    Retour invoquer(std::index_sequence<Positions...>, Arguments&&... arguments) {
        // Conversion statique lorsque possible, RTTI pour les bases virtuelles.
        return std::invoke(std::get<Fonction>(fonctions_),
            ajuster_argument<argument_fonction<Fonction, Positions>, Listes>(
                std::forward<Arguments>(arguments))...);
    }
    template<std::size_t Fonction>
    static Retour relais(methode& operation, Arguments&&... arguments) {
        return operation.template invoquer<Fonction>(positions{}, std::forward<Arguments>(arguments)...);
    }
    using pointeur_relais = Retour (*)(methode&, Arguments&&...);
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
        return std::array<pointeur_relais, sizeof...(Cases)>{choisir_relais<Cases>()...};
    }
    static constexpr auto table = creer_table(std::make_index_sequence<resolution.size()>{});
    tuple_fonctions fonctions_;

    Retour appeler_indices(const std::array<std::size_t, arite>& indices, Arguments&&... arguments) {
        std::size_t case_table = 0;
        for (std::size_t position = 0; position < arite; ++position)
            case_table = case_table * dimensions[position] + indices[position];
        return table[case_table](*this, std::forward<Arguments>(arguments)...);
    }
    template<bool Prepare, class Tuple, std::size_t... Positions>
    static auto identifier(Tuple& arguments, std::index_sequence<Positions...>) {
        if constexpr (Prepare)
            return std::array<std::size_t, arite>{std::get<Positions>(arguments).indice_...};
        else
            return std::array<std::size_t, arite>{
                liste<Positions>::indice(typeid(std::get<Positions>(arguments)))...};
    }
    template<class Argument, class Descripteur>
    static decltype(auto) extraire(typename detail::parametre<Argument, Descripteur>::prepare&& argument) {
        if constexpr (std::same_as<Descripteur, argument_ordinaire>)
            return std::forward<Argument>(argument);
        else return *argument.objet_;
    }
public:
    /** Force la validation, meme si la methode n'est jamais appelee. */
    explicit constexpr methode(Fonctions... fonctions) : fonctions_(std::move(fonctions)...) {
        static_assert(!table.empty());
    }
    /**
     * Appelle la specialisation correspondant aux types dynamiques exacts.
     * @throws type_inconnu Si un objet est absent du domaine.
     * Propage sans modification les exceptions des specialisations.
     */
    Retour operator()(Arguments... arguments) {
        if constexpr (arite == arite_totale) {
            // Conserve la forme directe des petits dispatchs existants pour l'optimiseur.
            const std::array<std::size_t, arite> indices{detail::traits_liste<Listes>::indice(typeid(arguments))...};
            return appeler_indices(indices, std::forward<Arguments>(arguments)...);
        } else {
            auto references = std::forward_as_tuple(arguments...);
            const auto indices = identifier<false>(references, positions_polymorphes{});
            return appeler_indices(indices, std::forward<Arguments>(arguments)...);
        }
    }
    /**
     * Valide le type une fois et memorise son indice pour les appels repetes.
     * @tparam Position Position dans la signature, a partir de zero (zero par defaut).
     * @throws type_inconnu Si le type exact est absent du domaine de cette position.
     * Les temporaires sont refuses pour eviter une reference immediatement pendante.
     */
    template<std::size_t Position = 0, class Objet>
    [[nodiscard]] auto preparer(Objet&& objet) const {
        static_assert(Position < arite_totale, "Position de preparation hors de la signature");
        static_assert(std::is_lvalue_reference_v<Objet>, "La preparation exige un objet persistant, pas un temporaire");
        if constexpr (Position < arite_totale && std::is_lvalue_reference_v<Objet>) {
            using Domaine = std::tuple_element_t<Position, tuple_listes>;
            static_assert(!std::same_as<Domaine, argument_ordinaire>,
                          "Une position ordinaire ne peut pas etre preparee");
            if constexpr (!std::same_as<Domaine, argument_ordinaire>) {
                using Parametre = detail::parametre<std::tuple_element_t<Position, tuple_arguments>, Domaine>;
                constexpr bool compatible = std::is_convertible_v<
                    std::remove_reference_t<Objet>*, typename Parametre::objet*>;
                static_assert(compatible, "Objet incompatible avec la racine ou la qualification const de cette position");
                if constexpr (compatible) {
                    return typename Parametre::reference(objet, liste<Position>::indice(typeid(objet)));
                }
            }
        }
    }
    /** Appel sans nouvelle identification RTTI ; chaque reference doit rester valide. */
    Retour operator()(typename detail::parametre<Arguments, Listes>::prepare... arguments) {
        if constexpr (arite == arite_totale) {
            return appeler_indices({arguments.indice_...}, *arguments.objet_...);
        } else {
            auto references = std::forward_as_tuple(arguments...);
            const auto indices = identifier<true>(references, positions_polymorphes{});
            return appeler_indices(indices, extraire<Arguments, Listes>(
                std::forward<typename detail::parametre<Arguments, Listes>::prepare>(arguments))...);
        }
    }
};
/** Deduit les signatures explicites des lambdas et conserve celles-ci par valeur. */
template<class Signature, class... Listes, class... Fonctions>
[[nodiscard]] constexpr auto creer_methode(domaines<Listes...>, Fonctions&&... fonctions) {
    return methode<Signature, domaines<Listes...>, std::decay_t<Fonctions>...>(
        std::forward<Fonctions>(fonctions)...);
}
} // namespace mini_openmethod
