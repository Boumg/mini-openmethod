#pragma once
#include <mini_openmethod/domaines.hpp>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <typeinfo>

/** @file Index ferme des adresses RTTI, avec repli sur l'egalite des types. */
namespace mini_openmethod::detail {
/** Arret au premier succes, d'abord par adresse, puis par egalite portable. */
template<class... Types>
#if defined(__GNUC__) && !defined(__clang__)
// GCC doit voir ces comparaisons dans l'appelant pour optimiser le petit dispatch.
[[gnu::always_inline]] inline
#endif
std::size_t indice_lineaire(const std::type_info& type) {
    std::size_t resultat = 0;
    if (((&type == &typeid(Types) || (++resultat, false)) || ...)) return resultat;
    resultat = 0;
    if (((type == typeid(Types) || (++resultat, false)) || ...)) return resultat;
    throw type_inconnu{};
}

/** Melange multiplicatif ; les bits hauts servent a choisir la case initiale. */
struct hachage_adresse_type {
    std::size_t operator()(const std::type_info* type) const noexcept {
        constexpr auto coefficient = sizeof(std::size_t) == 8
            ? static_cast<std::size_t>(0x9e3779b97f4a7c15ULL)
            : static_cast<std::size_t>(0x9e3779b9UL);
        return (static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(type)) >> 3) * coefficient;
    }
};

/**
 * Table sans allocation, remplie une fois puis seulement consultee.
 * Les collisions sont resolues par sondage lineaire. Une adresse RTTI absente
 * declenche une recherche par egalite : des adresses distinctes peuvent designer
 * le meme type, notamment entre bibliotheques partagees.
 */
template<std::size_t Taille, class Hachage = hachage_adresse_type>
class index_types {
    static_assert(Taille > 0);
    static constexpr auto capacite = std::bit_ceil(2 * Taille);
    static constexpr auto decalage = std::numeric_limits<std::size_t>::digits - std::countr_zero(capacite);
    std::array<const std::type_info*, Taille> types_;
    std::array<std::size_t, capacite> cases_;

    static std::size_t case_initiale(const std::type_info& type) noexcept {
        return Hachage{}(&type) >> decalage;
    }
public:
    explicit index_types(std::array<const std::type_info*, Taille> types) noexcept : types_(types) {
        cases_.fill(Taille);
        for (std::size_t indice = 0; indice < Taille; ++indice) {
            auto position = case_initiale(*types_[indice]);
            while (cases_[position] != Taille) position = (position + 1) & (capacite - 1);
            cases_[position] = indice;
        }
    }

    /** @throws type_inconnu Si aucune adresse ni aucun type ne correspond. */
    std::size_t indice(const std::type_info& type) const {
        auto position = case_initiale(type);
        while (cases_[position] != Taille) {
            const auto candidat = cases_[position];
            if (types_[candidat] == &type) return candidat;
            position = (position + 1) & (capacite - 1);
        }
        for (std::size_t candidat = 0; candidat < Taille; ++candidat)
            if (type == *types_[candidat]) return candidat;
        throw type_inconnu{};
    }
};
} // namespace mini_openmethod::detail
