#include "jeux_comparaison.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>

namespace comparaison {
namespace {
template<std::size_t Rang, bool Virtuel>
std::unique_ptr<Animal<Virtuel>> creer_objet(int valeur) {
    auto objet = std::make_unique<Espece<Rang, Virtuel>>();
    objet->valeur = valeur;
    return objet;
}

template<bool Virtuel, std::size_t... Rangs>
Jeu<Virtuel> creer_jeu(std::size_t nombre_types, std::uint32_t graine, std::index_sequence<Rangs...>) {
    if (nombre_types == 0 || nombre_types > sizeof...(Rangs))
        throw std::invalid_argument("Nombre de types hors du domaine de mesure");
    const std::array fabriques{&creer_objet<Rangs, Virtuel>...};
    std::mt19937 generateur(graine);
    Jeu<Virtuel> jeu;
    std::vector<int> valeurs;
    // Quatre objets par type, avec des valeurs determinees seulement a l'execution.
    for (std::size_t indice = 0; indice < 4 * nombre_types; ++indice) {
        const int valeur = 1 + static_cast<int>(generateur() % 100);
        jeu.objets.push_back(fabriques[indice % nombre_types](valeur));
        valeurs.push_back(valeur);
    }
    std::array<std::size_t, taille_sequence> ordre{};
    std::iota(ordre.begin(), ordre.end(), 0);
    std::shuffle(ordre.begin(), ordre.end(), generateur);
    for (std::size_t indice = 0; indice < taille_sequence; ++indice) {
        const auto rang_gauche = ordre[indice] % nombre_types;
        const auto rang_droite = (ordre[indice] / nombre_types) % nombre_types;
        const auto gauche = rang_gauche + nombre_types * (generateur() % 4);
        const auto droite = rang_droite + nombre_types * (generateur() % 4);
        jeu.gauche[indice] = jeu.objets[gauche].get();
        jeu.droite[indice] = jeu.objets[droite].get();
        jeu.attendu_simple[indice] = valeurs[gauche] + 1 + static_cast<int>(rang_gauche);
        jeu.attendu_double[indice] = valeurs[gauche] + valeurs[droite] + 1
            + 3 * static_cast<int>(rang_gauche) + 7 * static_cast<int>(rang_droite);
    }
    return jeu;
}
} // namespace

Jeu<false> creer_jeu_animaux(std::size_t nombre_types, std::uint32_t graine) {
    return creer_jeu<false>(nombre_types, graine, std::make_index_sequence<32>{});
}
Jeu<true> creer_jeu_virtuel(std::uint32_t graine) {
    return creer_jeu<true>(2, graine, std::make_index_sequence<2>{});
}
} // namespace comparaison
