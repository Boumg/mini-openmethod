#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

/** @file Objets du banc virtuel/non virtuel, independant des anciennes mesures. */
namespace comparaison_appels {
inline constexpr std::size_t taille_sequence = 4096;
struct Objet {
    int valeur = 0;
    virtual ~Objet() = default;
    virtual int calculer() const = 0;
};

template<std::size_t Rang> struct Variante final : Objet {
    /** Corps visible : le compilateur peut integrer cet appel non virtuel. */
    int calculer_direct() const { return valeur + 1 + static_cast<int>(Rang); }
    /** Meme calcul dans une autre unite, pour comparer a visibilite egale. */
    int calculer_separe() const;
    /** Definition separee : le banc appelle cette operation via Objet. */
    int calculer() const override;
};

struct Jeu {
    std::vector<std::unique_ptr<Objet>> objets;
    std::array<const Objet*, taille_sequence> sequence{};
    // Disponible uniquement dans le cas homogene, sans conversion chronometree.
    std::array<const Variante<0>*, taille_sequence> concrets{};
    std::array<int, taille_sequence> attendus{};
};

/** Fabrique opaque, types equilibres puis melanges ; accepte de 1 a 8 types. */
Jeu creer_jeu(std::size_t nombre_types, std::uint32_t graine);
} // namespace comparaison_appels
