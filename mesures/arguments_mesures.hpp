#pragma once
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>

/** @file Lecture commune des arguments des bancs, sans executer de mesure. */
namespace mesures {
enum class Mode_verification { indisponible, explicite, par_defaut };

struct Arguments {
    std::size_t iterations;
    std::uint32_t graine;
};

inline std::uint64_t lire_entier(std::string_view texte) {
    if (texte.empty()) throw std::invalid_argument("Un entier decimal non signe est attendu");
    std::uint64_t valeur = 0;
    const auto [fin, erreur] = std::from_chars(texte.data(), texte.data() + texte.size(), valeur);
    if (erreur != std::errc{} || fin != texte.data() + texte.size())
        throw std::invalid_argument("Un entier decimal non signe est attendu");
    return valeur;
}

/** Zero iterations represente uniquement une verification, jamais une mesure. */
inline Arguments lire_arguments(int nombre, const char* const* arguments, Mode_verification mode) {
    if (nombre < 1 || nombre > 3)
        throw std::invalid_argument("Deux arguments au plus sont acceptes : iterations puis graine");
    if (nombre > 1 && std::string_view(arguments[1]) == "--verifier") {
        if (mode == Mode_verification::indisponible)
            throw std::invalid_argument("L'option --verifier n'est pas proposee par ce programme");
        if (nombre != 2)
            throw std::invalid_argument("L'option --verifier s'utilise seule");
        return {0, 42};
    }
    if (nombre == 1)
        return {mode == Mode_verification::par_defaut ? 0u : 5'000'000u, 42};

    const auto iterations = lire_entier(arguments[1]);
    const auto graine = nombre == 3 ? lire_entier(arguments[2]) : 42;
    if (iterations == 0)
        throw std::invalid_argument("Le nombre d'iterations doit etre strictement positif");
    if (iterations > std::numeric_limits<std::size_t>::max())
        throw std::invalid_argument("Le nombre d'iterations depasse la capacite de size_t");
    if (graine > std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument("La graine doit tenir sur 32 bits");
    return {static_cast<std::size_t>(iterations), static_cast<std::uint32_t>(graine)};
}
} // namespace mesures
