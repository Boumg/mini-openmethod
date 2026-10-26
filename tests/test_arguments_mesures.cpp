#include "../mesures/arguments_mesures.hpp"
#include <doctest/doctest.h>
#include <array>
#include <limits>
#include <string>

/** @file Verifie les entrees des bancs sans lancer leurs boucles chronometrees. */
namespace {
using mesures::Mode_verification;

template<std::size_t Taille>
mesures::Arguments lire(const std::array<const char*, Taille>& arguments,
                       Mode_verification mode = Mode_verification::indisponible) {
    return mesures::lire_arguments(static_cast<int>(Taille), arguments.data(), mode);
}
} // namespace

TEST_CASE("Arguments des mesures : valeurs par defaut et verification explicite") {
    const std::array sans_arguments{"banc"};
    const std::array verification{"banc", "--verifier"};
    for (const auto mode : {Mode_verification::indisponible, Mode_verification::explicite,
                           Mode_verification::par_defaut}) {
        const auto defaut = lire(sans_arguments, mode);
        CHECK(defaut.iterations == (mode == Mode_verification::par_defaut ? 0u : 5'000'000u));
        CHECK(defaut.graine == 42);
    }
    for (const auto mode : {Mode_verification::explicite, Mode_verification::par_defaut}) {
        const auto options = lire(verification, mode);
        CHECK(options.iterations == 0);
        CHECK(options.graine == 42);
    }
    CHECK_THROWS_WITH_AS(lire(verification),
        "L'option --verifier n'est pas proposee par ce programme", std::invalid_argument);
}

TEST_CASE("Arguments des mesures : nombres valides et bornes incluses") {
    const std::array iterations_seules{"banc", "1"};
    const auto options = lire(iterations_seules);
    CHECK(options.iterations == 1);
    CHECK(options.graine == 42);

    const auto maximum = std::to_string(std::numeric_limits<std::size_t>::max());
    const std::array bornes{"banc", maximum.c_str(), "4294967295"};
    const auto maximales = lire(bornes);
    CHECK(maximales.iterations == std::numeric_limits<std::size_t>::max());
    CHECK(maximales.graine == std::numeric_limits<std::uint32_t>::max());

    const std::array graine_nulle{"banc", "123", "0"};
    for (const auto mode : {Mode_verification::indisponible, Mode_verification::explicite,
                           Mode_verification::par_defaut}) {
        const auto explicites = lire(graine_nulle, mode);
        CHECK(explicites.iterations == 123);
        CHECK(explicites.graine == 0);
    }
}

TEST_CASE("Arguments des mesures : refus des nombres mal formes") {
    for (const auto* texte : {"-1", "+1", "10suite", "", " 10", "10 ", "1.5", "0x10",
                             "18446744073709551616"}) {
        CAPTURE(texte);
        const std::array iterations{"banc", texte};
        const std::array graine{"banc", "1", texte};
        CHECK_THROWS_WITH_AS(lire(iterations),
            "Un entier decimal non signe est attendu", std::invalid_argument);
        CHECK_THROWS_WITH_AS(lire(graine),
            "Un entier decimal non signe est attendu", std::invalid_argument);
    }
}

TEST_CASE("Arguments des mesures : refus de zero iterations et des depassements") {
    const std::array zero{"banc", "0"};
    CHECK_THROWS_WITH_AS(lire(zero),
        "Le nombre d'iterations doit etre strictement positif", std::invalid_argument);
    const std::array graine_trop_grande{"banc", "1", "4294967296"};
    CHECK_THROWS_WITH_AS(lire(graine_trop_grande),
        "La graine doit tenir sur 32 bits", std::invalid_argument);

    if constexpr (std::numeric_limits<std::size_t>::max()
                  < std::numeric_limits<std::uint64_t>::max()) {
        const auto trop_grand = std::to_string(
            static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) + 1);
        const std::array arguments{"banc", trop_grand.c_str()};
        CHECK_THROWS_WITH_AS(lire(arguments),
            "Le nombre d'iterations depasse la capacite de size_t", std::invalid_argument);
    }
}

TEST_CASE("Arguments des mesures : refus des arguments supplementaires") {
    const std::array trop_arguments{"banc", "1", "42", "surplus"};
    CHECK_THROWS_WITH_AS(lire(trop_arguments),
        "Deux arguments au plus sont acceptes : iterations puis graine", std::invalid_argument);
    const std::array verification_avec_graine{"banc", "--verifier", "42"};
    for (const auto mode : {Mode_verification::explicite, Mode_verification::par_defaut}) {
        CHECK_THROWS_WITH_AS(lire(verification_avec_graine, mode),
            "L'option --verifier s'utilise seule", std::invalid_argument);
    }
}
