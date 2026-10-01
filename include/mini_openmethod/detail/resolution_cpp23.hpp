#pragma once
#include <mini_openmethod/detail/traits.hpp>
#include <mini_openmethod/detail/selection.hpp>
#include <utility>

/** @file Construction C++23 des tables par developpement des parametres de modeles. */
namespace mini_openmethod::detail {
template<class Domaines, class Fonctions> struct resolution_cpp23;

/** Produit les indices gagnants ; K signifie absence et K + 1 ambiguite. */
template<class... Listes, class... Fonctions>
struct resolution_cpp23<domaines<Listes...>, std::tuple<Fonctions...>> {
private:
    static constexpr std::size_t arite = sizeof...(Listes);
    static constexpr std::size_t nombre_fonctions = sizeof...(Fonctions);
    static constexpr std::array<std::size_t, arite> dimensions{traits_liste<Listes>::taille...};
    static constexpr std::size_t nombre_cases = (traits_liste<Listes>::taille * ...);
    using positions = std::make_index_sequence<arite>;
    template<std::size_t Position>
    using liste = traits_liste<std::tuple_element_t<Position, std::tuple<Listes...>>>;
    template<std::size_t Fonction, std::size_t Position>
    using argument = std::tuple_element_t<Position,
        typename traits_fonction<std::tuple_element_t<Fonction, std::tuple<Fonctions...>>>::arguments>;

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
            std::remove_cvref_t<argument<Fonction, Positions>>> && ...);
    }
    template<std::size_t Gauche, std::size_t Droite, std::size_t... Positions>
    static consteval bool domine(std::index_sequence<Positions...>) {
        return (std::derived_from<std::remove_cvref_t<argument<Gauche, Positions>>,
                                 std::remove_cvref_t<argument<Droite, Positions>>> && ...)
            && (!std::same_as<argument<Gauche, Positions>, argument<Droite, Positions>> || ...);
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
        return choisir_specialisation(candidats, [](std::size_t gauche, std::size_t droite) {
            return dominance[gauche][droite];
        });
    }
    template<std::size_t... Cases>
    static consteval auto construire(std::index_sequence<Cases...>) {
        return std::array<std::size_t, nombre_cases>{resoudre<Cases>(std::make_index_sequence<nombre_fonctions>{})...};
    }
public:
    static constexpr auto indices = construire(std::make_index_sequence<nombre_cases>{});
};
} // namespace mini_openmethod::detail
