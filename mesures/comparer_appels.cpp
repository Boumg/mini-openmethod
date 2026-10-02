#include "objets_appels.hpp"
#include <mini_openmethod/methode.hpp>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

/** @file Reference ponctuelle : appels directs, virtuels et methodes externes. */
namespace comparaison_appels {
constexpr std::size_t repetitions = 7;

template<class Appel>
double chronometrer(std::size_t iterations, std::uint64_t attendu, Appel& appel) {
    std::uint64_t somme = 0;
    const auto debut = std::chrono::steady_clock::now();
    for (std::size_t indice = 0; indice < iterations; ++indice)
        somme += static_cast<std::uint64_t>(appel(indice % taille_sequence));
    const auto fin = std::chrono::steady_clock::now();
    if (somme != attendu) throw std::runtime_error("Somme incorrecte pendant la mesure");
    return std::chrono::duration<double, std::nano>(fin - debut).count()
        / static_cast<double>(iterations);
}

template<class... Appels>
void comparer(std::size_t nombre_types, std::size_t iterations,
              const std::array<int, taille_sequence>& attendus, Appels... appels) {
    for (std::size_t indice = 0; indice < taille_sequence; ++indice)
        if (!((appels(indice) == attendus[indice]) && ...))
            throw std::runtime_error("Desaccord avec le resultat attendu");
    if (iterations == 0) {
        std::cout << nombre_types << " types : " << sizeof...(Appels)
                  << " variantes, 4096 entrees verifiees\n";
        return;
    }
    std::uint64_t somme_attendue = 0;
    for (std::size_t indice = 0; indice < iterations; ++indice)
        somme_attendue += static_cast<std::uint64_t>(attendus[indice % taille_sequence]);
    std::array<std::array<double, repetitions>, sizeof...(Appels)> durees{};
    for (std::size_t passage = 0; passage < repetitions; ++passage) {
        for (std::size_t position = 0; position < sizeof...(Appels); ++position) {
            const auto version = (passage + position) % sizeof...(Appels);
            std::size_t rang = 0;
            // Appels de types distincts : aucune couche std::function dans la boucle.
            ((rang++ == version
                ? static_cast<void>(durees[version][passage] =
                      chronometrer(iterations, somme_attendue, appels))
                : static_cast<void>(0)), ...);
        }
    }
    std::cout << nombre_types;
    // Un appel direct ne peut pas choisir une specialisation parmi des types inconnus.
    if constexpr (sizeof...(Appels) == 5) std::cout << ";sans objet;sans objet";
    for (auto& version : durees) {
        std::sort(version.begin(), version.end());
        std::cout << ';' << version[repetitions / 2];
    }
    std::cout << ';' << somme_attendue << '\n';
}

template<std::size_t... Rangs>
void mesurer(std::size_t iterations, std::uint32_t graine, std::index_sequence<Rangs...>) {
    const auto jeu = creer_jeu(sizeof...(Rangs), graine);
    auto methode = mini_openmethod::creer_methode<int(const Objet&)>(
        mini_openmethod::domaines<mini_openmethod::liste_types<Variante<Rangs>...>>{},
        [](const Variante<Rangs>& objet) { return objet.calculer_direct(); }...);
    auto methode_separee = mini_openmethod::creer_methode<int(const Objet&)>(
        mini_openmethod::domaines<mini_openmethod::liste_types<Variante<Rangs>...>>{},
        [](const Variante<Rangs>& objet) { return objet.calculer_separe(); }...);
    std::vector<decltype(methode.preparer(*jeu.sequence[0]))> references;
    references.reserve(taille_sequence);
    for (const auto* objet : jeu.sequence) references.push_back(methode.preparer(*objet));
    auto virtuel = [&](std::size_t indice) { return jeu.sequence[indice]->calculer(); };
    auto externe = [&](std::size_t indice) { return methode(*jeu.sequence[indice]); };
    auto prepare = [&](std::size_t indice) { return methode(references[indice]); };
    auto externe_separe = [&](std::size_t indice) { return methode_separee(*jeu.sequence[indice]); };
    auto prepare_separe = [&](std::size_t indice) { return methode_separee(references[indice]); };
    if constexpr (sizeof...(Rangs) == 1) {
        comparer(1, iterations, jeu.attendus,
            [&](std::size_t indice) { return jeu.concrets[indice]->calculer_direct(); },
            [&](std::size_t indice) { return jeu.concrets[indice]->calculer_separe(); },
            virtuel, externe, prepare, externe_separe, prepare_separe);
    } else {
        comparer(sizeof...(Rangs), iterations, jeu.attendus,
                 virtuel, externe, prepare, externe_separe, prepare_separe);
    }
}

std::uint64_t lire_entier(std::string_view texte) {
    std::uint64_t valeur = 0;
    const auto [fin, erreur] = std::from_chars(texte.data(), texte.data() + texte.size(), valeur);
    if (erreur != std::errc{} || fin != texte.data() + texte.size())
        throw std::invalid_argument("Un entier positif est attendu");
    return valeur;
}
} // namespace comparaison_appels

int main(int nombre_arguments, char** arguments) {
    try {
        using namespace comparaison_appels;
        // Une execution sans argument verifie seulement : les mesures sont explicites.
        const bool verifier = nombre_arguments == 1
            || (nombre_arguments == 2 && std::string_view(arguments[1]) == "--verifier");
        const auto iterations = verifier ? 0 : lire_entier(arguments[1]);
        const auto graine = nombre_arguments > 2 ? lire_entier(arguments[2]) : 42;
        if (nombre_arguments > 3 || (!verifier && iterations == 0)
            || iterations > std::numeric_limits<std::size_t>::max()
            || graine > std::numeric_limits<std::uint32_t>::max())
            throw std::invalid_argument("Usage : comparer_appels [iterations>0] [graine32] ou --verifier");
        std::cout << std::unitbuf << std::fixed << std::setprecision(3);
#if defined(__clang__)
        std::cout << "Compilateur=Clang " << __clang_version__ << '\n';
#elif defined(_MSC_FULL_VER)
        std::cout << "Compilateur=MSVC " << _MSC_FULL_VER << '\n';
#else
        std::cout << "Compilateur=" << __VERSION__ << '\n';
#endif
        std::cout << "Reflexion=" << MINI_OPENMETHOD_REFLEXION << ";iterations=" << iterations
                  << ";graine=" << graine << ";passages=" << repetitions << '\n';
#if defined(NDEBUG)
        std::cout << "NDEBUG=1;LTO=desactivee;preparation=hors_chronometrage\n";
#else
        std::cout << "NDEBUG=0;utiliser Release pour les mesures\n";
#endif
        std::cout << "Types;Direct visible ns/appel;Direct separe ns/appel;Virtuel ns/appel;"
                     "Mini visible ns/appel;Mini visible prepare ns/appel;"
                     "Mini separe ns/appel;Mini separe prepare ns/appel;Somme\n";
        const auto nombre = static_cast<std::size_t>(iterations);
        const auto graine32 = static_cast<std::uint32_t>(graine);
        mesurer(nombre, graine32, std::make_index_sequence<1>{});
        mesurer(nombre, graine32, std::make_index_sequence<2>{});
        mesurer(nombre, graine32, std::make_index_sequence<8>{});
    } catch (const std::exception& erreur) {
        std::cerr << erreur.what() << '\n';
        return 1;
    }
}
