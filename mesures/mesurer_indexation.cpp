#include "jeux_comparaison.hpp"
#include <mini_openmethod/detail/index_types.hpp>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <utility>

/** @file Compare les recherches seules pour choisir le seuil, sans Boost ni LTO. */
namespace {
using namespace comparaison;
constexpr std::size_t iterations = 5'000'000;
constexpr std::size_t repetitions = 7;

template<class Appel>
double chronometrer(Appel appel, std::size_t attendu) {
    std::size_t somme = 0;
    const auto debut = std::chrono::steady_clock::now();
    for (std::size_t indice = 0; indice < iterations; ++indice) somme += appel(indice % taille_sequence);
    const auto fin = std::chrono::steady_clock::now();
    if (somme != attendu) throw std::runtime_error("Somme d'indices incorrecte");
    return std::chrono::duration<double, std::nano>(fin - debut).count() / iterations;
}

template<std::size_t... Rangs>
void comparer(std::index_sequence<Rangs...>) {
    const auto jeu = creer_jeu_animaux(sizeof...(Rangs), 42);
    const auto lineaire = [&](std::size_t position) {
        const auto* objet = jeu.gauche[position];
        return mini_openmethod::detail::indice_lineaire<Espece<Rangs>...>(typeid(*objet));
    };
    const auto hache = [&](std::size_t position) {
        // Meme initialisation locale et meme garde que dans le moteur.
        static const mini_openmethod::detail::index_types<sizeof...(Rangs)> index(
            std::array{&typeid(Espece<Rangs>)...});
        const auto* objet = jeu.gauche[position];
        return index.indice(typeid(*objet));
    };
    for (std::size_t position = 0; position < taille_sequence; ++position)
        if (lineaire(position) != hache(position)) throw std::runtime_error("Indices differents");
    std::size_t attendu = 0;
    for (std::size_t position = 0; position < iterations; ++position) attendu += lineaire(position % taille_sequence);
    std::array<double, repetitions> temps_lineaire{}, temps_hache{};
    for (std::size_t passage = 0; passage < repetitions; ++passage) {
        if (passage % 2 == 0) {
            temps_lineaire[passage] = chronometrer(lineaire, attendu);
            temps_hache[passage] = chronometrer(hache, attendu);
        } else {
            temps_hache[passage] = chronometrer(hache, attendu);
            temps_lineaire[passage] = chronometrer(lineaire, attendu);
        }
    }
    std::sort(temps_lineaire.begin(), temps_lineaire.end());
    std::sort(temps_hache.begin(), temps_hache.end());
    std::cout << sizeof...(Rangs) << ';' << temps_lineaire[repetitions / 2] << ';'
              << temps_hache[repetitions / 2] << ';' << attendu << '\n';
}
} // namespace

int main() {
    try {
        std::cout << std::unitbuf << std::fixed << std::setprecision(3);
#if defined(__clang__)
        std::cout << "Compilateur=Clang " << __clang_version__ << '\n';
#elif defined(_MSC_FULL_VER)
        std::cout << "Compilateur=MSVC " << _MSC_FULL_VER << '\n';
#else
        std::cout << "Compilateur=" << __VERSION__ << '\n';
#endif
        std::cout << "iterations=" << iterations << ";graine=42;passages=" << repetitions
                  << ";LTO=desactive;initialisation=hors_chronometrage\n";
        std::cout << "Types;Lineaire ns/recherche;Hachage ns/recherche;Somme\n";
        comparer(std::make_index_sequence<2>{});
        comparer(std::make_index_sequence<4>{});
        comparer(std::make_index_sequence<8>{});
        comparer(std::make_index_sequence<16>{});
        comparer(std::make_index_sequence<32>{});
    } catch (const std::exception& erreur) {
        std::cerr << erreur.what() << '\n';
        return 1;
    }
}
