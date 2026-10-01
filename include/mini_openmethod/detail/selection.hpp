#pragma once
#include <array>
#include <cstddef>

/** @file Selection commune aux generateurs, independante de la representation des types. */
namespace mini_openmethod::detail {
/** Renvoie le candidat maximal unique, Taille si absent, Taille + 1 si ambigu. */
template<std::size_t Taille, class Dominance>
constexpr std::size_t choisir_specialisation(const std::array<bool, Taille>& candidats, Dominance domine) {
    std::array<std::size_t, Taille> applicables{};
    std::size_t nombre = 0;
    for (std::size_t candidat = 0; candidat < Taille; ++candidat)
        if (candidats[candidat]) applicables[nombre++] = candidat;
    if (nombre == 0) return Taille;
    if (nombre == 1) return applicables[0];

    std::size_t gagnant = Taille;
    for (std::size_t position = 0; position < nombre; ++position) {
        const auto candidat = applicables[position];
        bool domine_par_autre = false;
        for (std::size_t autre = 0; autre < nombre; ++autre) {
            if (domine(applicables[autre], candidat)) { domine_par_autre = true; break; }
        }
        if (domine_par_autre) continue;
        if (gagnant != Taille) return Taille + 1;
        gagnant = candidat;
    }
    return gagnant;
}
} // namespace mini_openmethod::detail
