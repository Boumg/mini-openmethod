#include <mini_openmethod/methode.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>

/** @file Mesure indicative : entrees melangees, resultats controles et mediane de cinq passages. */
namespace {
struct Animal { virtual ~Animal() = default; int valeur = 0; };
struct Chien : Animal {};
struct Chat : Animal {};
struct Racine { virtual ~Racine() = default; };
struct Virtuel : virtual Racine { int valeur = 13; };
struct Autre_virtuel : virtual Racine { int valeur = 17; };
constexpr std::size_t taille_sequence = 4096;

template<class Appel, class Attendu>
void mesurer(std::string_view nom, std::size_t iterations, Appel appel, Attendu attendu) {
    std::uint64_t somme_attendue = 0;
    for (std::size_t i = 0; i < iterations; ++i) somme_attendue += attendu(i % taille_sequence);
    for (std::size_t i = 0; i < taille_sequence; ++i)
        if (appel(i) != attendu(i)) throw std::runtime_error("Resultat incorrect pendant l'echauffement");
    std::array<double, 5> durees{};
    for (auto& duree : durees) {
        std::uint64_t somme = 0;
        const auto debut = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < iterations; ++i) somme += appel(i % taille_sequence);
        const auto fin = std::chrono::steady_clock::now();
        if (somme != somme_attendue) throw std::runtime_error("Somme incorrecte pendant la mesure");
        duree = std::chrono::duration<double, std::nano>(fin - debut).count() / static_cast<double>(iterations);
    }
    std::sort(durees.begin(), durees.end());
    std::cout << nom << " : " << durees[2] << " ns/appel ; somme=" << somme_attendue << '\n';
}
} // namespace

int main(int nombre_arguments, char** arguments) {
    try {
        const auto iterations = nombre_arguments > 1 ? std::stoull(arguments[1]) : 5'000'000ULL;
        const auto graine = nombre_arguments > 2 ? std::stoul(arguments[2]) : 42UL;
        if (iterations == 0) throw std::runtime_error("Le nombre d'iterations doit etre positif");
        std::mt19937 generateur(static_cast<std::uint32_t>(graine));
        std::array<unsigned, taille_sequence> choix{};
        for (auto& valeur : choix) valeur = static_cast<unsigned>(generateur() % 4);
        Chien chien;
        Chat chat;
        chien.valeur = 3;
        chat.valeur = 7;
        const std::array<const Animal*, 2> animaux{&chien, &chat};
        std::array<const Animal*, taille_sequence> premiers{}, seconds{};
        for (std::size_t i = 0; i < taille_sequence; ++i) {
            premiers[i] = animaux[choix[i] / 2];
            seconds[i] = animaux[choix[i] % 2];
        }
        using namespace mini_openmethod;
        using Animaux = liste_types<Chien, Chat>;
        auto simple = creer_methode<int(const Animal&)>(domaines<Animaux>{},
            [](const Chien& a) { return a.valeur + 1; }, [](const Chat& a) { return a.valeur + 2; });
        auto double_dispatch = creer_methode<int(const Animal&, const Animal&)>(domaines<Animaux, Animaux>{},
            [](const Chien& a, const Chien& b) { return a.valeur + b.valeur + 1; },
            [](const Chien& a, const Chat& b) { return a.valeur + b.valeur + 2; },
            [](const Chat& a, const Chien& b) { return a.valeur + b.valeur + 3; },
            [](const Chat& a, const Chat& b) { return a.valeur + b.valeur + 4; });
        std::cout << "Reflexion=" << MINI_OPENMETHOD_REFLEXION << " ; iterations=" << iterations
                  << " ; graine=" << graine << '\n';
        mesurer("Simple", iterations, [&](std::size_t i) { return simple(*premiers[i]); },
            [&](std::size_t i) { return premiers[i]->valeur + 1 + static_cast<int>(choix[i] / 2); });
        mesurer("Double", iterations, [&](std::size_t i) { return double_dispatch(*premiers[i], *seconds[i]); },
            [&](std::size_t i) { return premiers[i]->valeur + seconds[i]->valeur + 1 + static_cast<int>(choix[i]); });
        Virtuel objet;
        Autre_virtuel autre;
        const std::array<const Racine*, 2> racines{&objet, &autre};
        std::array<const Racine*, taille_sequence> sequence_virtuelle{};
        for (std::size_t i = 0; i < taille_sequence; ++i) sequence_virtuelle[i] = racines[choix[i] % 2];
        auto virtuel = creer_methode<int(const Racine&)>(domaines<liste_types<Virtuel, Autre_virtuel>>{},
            [](const Virtuel& a) { return a.valeur; }, [](const Autre_virtuel& a) { return a.valeur; });
        mesurer("Heritage virtuel", iterations, [&](std::size_t i) { return virtuel(*sequence_virtuelle[i]); },
            [&](std::size_t i) { return choix[i] % 2 == 0 ? objet.valeur : autre.valeur; });
    } catch (const std::exception& erreur) {
        std::cerr << erreur.what() << '\n';
        return 1;
    }
}
