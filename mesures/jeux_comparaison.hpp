#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

/** @file Objets communs aux deux bibliotheques ; leur fabrique est compilee separement. */
namespace comparaison {
inline constexpr std::size_t taille_sequence = 4096;

template<bool Virtuel> struct Animal { virtual ~Animal() = default; };
template<std::size_t Rang, bool Virtuel = false> struct Espece : Animal<false> { int valeur = 0; };
template<std::size_t Rang> struct Espece<Rang, true> : virtual Animal<true> { int valeur = 0; };

template<bool Virtuel> struct Jeu {
    std::vector<std::unique_ptr<Animal<Virtuel>>> objets;
    std::array<const Animal<Virtuel>*, taille_sequence> gauche{}, droite{};
    std::array<int, taille_sequence> attendu_simple{}, attendu_double{};
};

/** Donnees equilibrees puis melangees ; les attendus ne font appel a aucun moteur. */
Jeu<false> creer_jeu_animaux(std::size_t nombre_types, std::uint32_t graine);
Jeu<true> creer_jeu_virtuel(std::uint32_t graine);
} // namespace comparaison
