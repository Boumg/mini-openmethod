#include "objets_appels.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>

namespace comparaison_appels {
template<std::size_t Rang>
int Variante<Rang>::calculer() const { return calculer_separe(); }
template<std::size_t Rang>
int Variante<Rang>::calculer_separe() const { return calculer_direct(); }

// Emet aussi les fonctions non virtuelles appelees depuis l'autre unite.
template struct Variante<0>;
template struct Variante<1>;
template struct Variante<2>;
template struct Variante<3>;
template struct Variante<4>;
template struct Variante<5>;
template struct Variante<6>;
template struct Variante<7>;

namespace {
template<std::size_t Rang>
std::unique_ptr<Objet> creer_objet(int valeur) {
    auto objet = std::make_unique<Variante<Rang>>();
    objet->valeur = valeur;
    return objet;
}

template<std::size_t... Rangs>
Jeu creer_jeu(std::size_t nombre_types, std::uint32_t graine, std::index_sequence<Rangs...>) {
    if (nombre_types == 0 || nombre_types > sizeof...(Rangs))
        throw std::invalid_argument("Nombre de types hors du domaine de mesure");
    const std::array fabriques{&creer_objet<Rangs>...};
    std::mt19937 generateur(graine);
    Jeu jeu;
    std::vector<int> valeurs;
    for (std::size_t indice = 0; indice < 4 * nombre_types; ++indice) {
        const int valeur = 1 + static_cast<int>(generateur() % 100);
        jeu.objets.push_back(fabriques[indice % nombre_types](valeur));
        valeurs.push_back(valeur);
    }
    std::array<std::size_t, taille_sequence> ordre{};
    std::iota(ordre.begin(), ordre.end(), 0);
    std::shuffle(ordre.begin(), ordre.end(), generateur);
    for (std::size_t indice = 0; indice < taille_sequence; ++indice) {
        const auto rang = ordre[indice] % nombre_types;
        const auto objet = rang + nombre_types * (generateur() % 4);
        jeu.sequence[indice] = jeu.objets[objet].get();
        jeu.attendus[indice] = valeurs[objet] + 1 + static_cast<int>(rang);
        if (nombre_types == 1)
            jeu.concrets[indice] = static_cast<const Variante<0>*>(jeu.sequence[indice]);
    }
    return jeu;
}
} // namespace

Jeu creer_jeu(std::size_t nombre_types, std::uint32_t graine) {
    return creer_jeu(nombre_types, graine, std::make_index_sequence<8>{});
}
} // namespace comparaison_appels
