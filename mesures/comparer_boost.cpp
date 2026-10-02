#include "jeux_comparaison.hpp"
#include <mini_openmethod/methode.hpp>
#include <boost/openmethod/core.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/version.hpp>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

/** @file Comparaison sur donnees communes ; aucune limite de temps dans les tests. */
namespace comparaison {
namespace om = boost::openmethod;
using Horloge = std::chrono::steady_clock;
constexpr std::size_t repetitions = 7;

/** Etiquette sans comportement : isole les donnees des registres de chaque scenario. */
template<class Cas> struct Identite_registre {
    using category = Identite_registre;
    template<class> struct fn {};
};
template<class Cas> struct Registre : om::default_registry::with<Identite_registre<Cas>> {};

template<std::size_t Rang, bool Virtuel>
int traiter_simple(const Espece<Rang, Virtuel>& objet) {
    return objet.valeur + 1 + static_cast<int>(Rang);
}
template<std::size_t Gauche, std::size_t Droite>
int traiter_double(const Espece<Gauche>& gauche, const Espece<Droite>& droite) {
    return gauche.valeur + droite.valeur + 1 + 3 * static_cast<int>(Gauche) + 7 * static_cast<int>(Droite);
}
template<class Contexte, std::size_t Rang, bool Virtuel>
int traiter_pointeur_simple(om::virtual_ptr<const Espece<Rang, Virtuel>, Contexte> objet) {
    return traiter_simple(*objet);
}
template<class Contexte, std::size_t Gauche, std::size_t Droite>
int traiter_pointeurs_doubles(om::virtual_ptr<const Espece<Gauche>, Contexte> gauche,
                              om::virtual_ptr<const Espece<Droite>, Contexte> droite) {
    return traiter_double(*gauche, *droite);
}

template<class Appel>
double chronometrer(std::size_t iterations, std::uint64_t attendu, Appel& appel) {
    std::uint64_t somme = 0;
    const auto debut = Horloge::now();
    for (std::size_t indice = 0; indice < iterations; ++indice)
        somme += static_cast<std::uint64_t>(appel(indice % taille_sequence));
    const auto fin = Horloge::now();
    if (somme != attendu) throw std::runtime_error("Somme incorrecte pendant la comparaison");
    return std::chrono::duration<double, std::nano>(fin - debut).count() / static_cast<double>(iterations);
}

template<class Mini, class Mini_prepare, class Reference, class Prepare>
void comparer(std::string_view nom, std::size_t nombre_types, std::size_t iterations,
              const std::array<int, taille_sequence>& attendus, Mini mini, Mini_prepare mini_prepare,
              Reference reference, Prepare prepare) {
    // Le controle par entree couvre toutes les combinaisons, avant toute mesure.
    for (std::size_t indice = 0; indice < taille_sequence; ++indice)
        if (mini(indice) != attendus[indice] || mini_prepare(indice) != attendus[indice]
            || reference(indice) != attendus[indice]
            || prepare(indice) != attendus[indice])
            throw std::runtime_error("Desaccord entre une implementation et le resultat attendu");
    if (iterations == 0) {
        std::cout << nom << ";" << nombre_types << ";4096 entrees verifiees\n";
        return;
    }
    std::uint64_t somme_attendue = 0;
    for (std::size_t indice = 0; indice < iterations; ++indice)
        somme_attendue += static_cast<std::uint64_t>(attendus[indice % taille_sequence]);
    std::array<std::array<double, repetitions>, 4> durees{};
    for (std::size_t passage = 0; passage < repetitions; ++passage) {
        // L'ordre tourne pour limiter le biais de frequence et d'echauffement.
        for (std::size_t position = 0; position < durees.size(); ++position) {
            const auto version = (passage + position) % durees.size();
            if (version == 0) durees[0][passage] = chronometrer(iterations, somme_attendue, mini);
            if (version == 1) durees[1][passage] = chronometrer(iterations, somme_attendue, mini_prepare);
            if (version == 2) durees[2][passage] = chronometrer(iterations, somme_attendue, reference);
            if (version == 3) durees[3][passage] = chronometrer(iterations, somme_attendue, prepare);
        }
    }
    std::cout << nom << ';' << nombre_types;
    for (auto& version : durees) {
        std::sort(version.begin(), version.end());
        std::cout << ';' << version[repetitions / 2];
    }
    std::cout << ';' << somme_attendue << '\n';
}

template<std::size_t Nombre, bool Virtuel> struct Cas_simple;
template<std::size_t Nombre> struct Cas_double;

template<std::size_t Nombre, bool Virtuel, std::size_t... Rangs>
void comparer_simple(std::size_t iterations, std::uint32_t graine, std::index_sequence<Rangs...>) {
    using Contexte = Registre<Cas_simple<Nombre, Virtuel>>;
    using Base = Animal<Virtuel>;
    using Pointeur = om::virtual_ptr<const Base, Contexte>;
    struct Identifiant_reference;
    struct Identifiant_prepare;
    using Reference = om::method<Identifiant_reference, int(om::virtual_<const Base&>), Contexte>;
    using Prepare = om::method<Identifiant_prepare, int(Pointeur), Contexte>;
    // Les enregistrements Boost vivent jusqu'a la fin du programme.
    static const om::use_classes<Base, Espece<Rangs, Virtuel>..., Contexte> classes;
    static const typename Reference::template override<traiter_simple<Rangs, Virtuel>...> references;
    static const typename Prepare::template override<traiter_pointeur_simple<Contexte, Rangs, Virtuel>...> prepares;
    om::initialize<Contexte>();
    const auto jeu = [&] {
        if constexpr (Virtuel) return creer_jeu_virtuel(graine);
        else return creer_jeu_animaux(Nombre, graine);
    }();
    std::array<Pointeur, taille_sequence> pointeurs;
    for (std::size_t indice = 0; indice < taille_sequence; ++indice)
        pointeurs[indice] = Pointeur(*jeu.gauche[indice]);
    auto mini = mini_openmethod::creer_methode<int(const Base&)>(
        mini_openmethod::domaines<mini_openmethod::liste_types<Espece<Rangs, Virtuel>...>>{},
        [](const Espece<Rangs, Virtuel>& objet) { return traiter_simple(objet); }...);
    std::vector<decltype(mini.preparer(*jeu.gauche[0]))> mini_prepares;
    mini_prepares.reserve(taille_sequence);
    for (const auto* objet : jeu.gauche) mini_prepares.push_back(mini.preparer(*objet));
    comparer(Virtuel ? "Heritage virtuel" : "Simple", Nombre, iterations, jeu.attendu_simple,
        [&](std::size_t indice) { return mini(*jeu.gauche[indice]); },
        [&](std::size_t indice) { return mini(mini_prepares[indice]); },
        [&](std::size_t indice) { return Reference::fn(*jeu.gauche[indice]); },
        [&](std::size_t indice) { return Prepare::fn(pointeurs[indice]); });
}

template<std::size_t Nombre, std::size_t... Rangs, std::size_t... Cases>
void comparer_double(std::size_t iterations, std::uint32_t graine,
                     std::index_sequence<Rangs...>, std::index_sequence<Cases...>) {
    using Contexte = Registre<Cas_double<Nombre>>;
    using Base = Animal<false>;
    using Pointeur = om::virtual_ptr<const Base, Contexte>;
    struct Identifiant_reference;
    struct Identifiant_prepare;
    using Reference = om::method<Identifiant_reference,
        int(om::virtual_<const Base&>, om::virtual_<const Base&>), Contexte>;
    using Prepare = om::method<Identifiant_prepare, int(Pointeur, Pointeur), Contexte>;
    static const om::use_classes<Base, Espece<Rangs>..., Contexte> classes;
    static const typename Reference::template override<traiter_double<Cases / Nombre, Cases % Nombre>...> references;
    static const typename Prepare::template override<traiter_pointeurs_doubles<Contexte, Cases / Nombre, Cases % Nombre>...> prepares;
    om::initialize<Contexte>();
    const auto jeu = creer_jeu_animaux(Nombre, graine);
    std::array<Pointeur, taille_sequence> gauche, droite;
    for (std::size_t indice = 0; indice < taille_sequence; ++indice) {
        gauche[indice] = Pointeur(*jeu.gauche[indice]);
        droite[indice] = Pointeur(*jeu.droite[indice]);
    }
    using Domaine = mini_openmethod::liste_types<Espece<Rangs>...>;
    auto mini = mini_openmethod::creer_methode<int(const Base&, const Base&)>(
        mini_openmethod::domaines<Domaine, Domaine>{},
        [](const Espece<Cases / Nombre>& premier, const Espece<Cases % Nombre>& second) {
            return traiter_double(premier, second);
        }...);
    std::vector<decltype(mini.preparer(*jeu.gauche[0]))> mini_gauche, mini_droite;
    mini_gauche.reserve(taille_sequence);
    mini_droite.reserve(taille_sequence);
    for (std::size_t indice = 0; indice < taille_sequence; ++indice) {
        mini_gauche.push_back(mini.template preparer<0>(*jeu.gauche[indice]));
        mini_droite.push_back(mini.template preparer<1>(*jeu.droite[indice]));
    }
    comparer("Double", Nombre, iterations, jeu.attendu_double,
        [&](std::size_t indice) { return mini(*jeu.gauche[indice], *jeu.droite[indice]); },
        [&](std::size_t indice) { return mini(mini_gauche[indice], mini_droite[indice]); },
        [&](std::size_t indice) { return Reference::fn(*jeu.gauche[indice], *jeu.droite[indice]); },
        [&](std::size_t indice) { return Prepare::fn(gauche[indice], droite[indice]); });
}

/** Deux valeurs ordinaires entourent la position polymorphe. */
template<std::size_t Rang>
int traiter_montant(const int& contexte, const Espece<Rang>& objet, int montant) {
    return traiter_simple(objet) + contexte + montant;
}
template<class Contexte, std::size_t Rang>
int traiter_pointeur_montant(const int& contexte,
                            om::virtual_ptr<const Espece<Rang>, Contexte> objet, int montant) {
    return traiter_montant(contexte, *objet, montant);
}
template<std::size_t Nombre> struct Cas_ordinaire;

template<std::size_t Nombre, std::size_t... Rangs>
void comparer_ordinaires(std::size_t iterations, std::uint32_t graine, std::index_sequence<Rangs...>) {
    using Contexte = Registre<Cas_ordinaire<Nombre>>;
    using Base = Animal<false>;
    using Pointeur = om::virtual_ptr<const Base, Contexte>;
    struct Identifiant_reference;
    struct Identifiant_prepare;
    using Reference = om::method<Identifiant_reference,
        int(const int&, om::virtual_<const Base&>, int), Contexte>;
    using Prepare = om::method<Identifiant_prepare, int(const int&, Pointeur, int), Contexte>;
    static const om::use_classes<Base, Espece<Rangs>..., Contexte> classes;
    static const typename Reference::template override<traiter_montant<Rangs>...> references;
    static const typename Prepare::template override<traiter_pointeur_montant<Contexte, Rangs>...> prepares;
    om::initialize<Contexte>();
    const auto jeu = creer_jeu_animaux(Nombre, graine);
    using namespace mini_openmethod;
    auto mini = creer_methode<int(const int&, const Base&, int)>(
        domaines<argument_ordinaire, liste_types<Espece<Rangs>...>, argument_ordinaire>{},
        [](const int& contexte, const Espece<Rangs>& objet, int montant) {
            return traiter_montant(contexte, objet, montant);
        }...);
    std::array<Pointeur, taille_sequence> pointeurs;
    std::vector<decltype(mini.template preparer<1>(*jeu.gauche[0]))> mini_prepares;
    mini_prepares.reserve(taille_sequence);
    std::array<int, taille_sequence> attendus;
    const int contexte = 7;
    for (std::size_t indice = 0; indice < taille_sequence; ++indice) {
        pointeurs[indice] = Pointeur(*jeu.gauche[indice]);
        mini_prepares.push_back(mini.template preparer<1>(*jeu.gauche[indice]));
        attendus[indice] = jeu.attendu_simple[indice] + contexte + static_cast<int>(indice);
    }
    comparer("Arguments ordinaires", Nombre, iterations, attendus,
        [&](std::size_t indice) { return mini(contexte, *jeu.gauche[indice], static_cast<int>(indice)); },
        [&](std::size_t indice) { return mini(contexte, mini_prepares[indice], static_cast<int>(indice)); },
        [&](std::size_t indice) { return Reference::fn(contexte, *jeu.gauche[indice], static_cast<int>(indice)); },
        [&](std::size_t indice) { return Prepare::fn(contexte, pointeurs[indice], static_cast<int>(indice)); });
}

std::uint64_t lire_entier(std::string_view texte) {
    std::uint64_t valeur = 0;
    const auto [fin, erreur] = std::from_chars(texte.data(), texte.data() + texte.size(), valeur);
    if (erreur != std::errc{} || fin != texte.data() + texte.size())
        throw std::invalid_argument("Un entier positif est attendu");
    return valeur;
}
} // namespace comparaison

int main(int nombre_arguments, char** arguments) {
    try {
        std::cout << std::unitbuf;
        using namespace comparaison;
        const bool verification = nombre_arguments == 2 && std::string_view(arguments[1]) == "--verifier";
        const auto nombre = verification ? 0 : nombre_arguments > 1 ? lire_entier(arguments[1]) : 5'000'000;
        const auto graine = nombre_arguments > 2 ? lire_entier(arguments[2]) : 42;
        if (nombre_arguments > 3 || (!verification && nombre == 0)
            || nombre > std::numeric_limits<std::size_t>::max()
            || graine > std::numeric_limits<std::uint32_t>::max())
            throw std::invalid_argument("Usage : comparer_boost [iterations>0] [graine32] ou --verifier");
        const auto iterations = static_cast<std::size_t>(nombre);
        const auto graine32 = static_cast<std::uint32_t>(graine);
        std::cout << "Boost=" << BOOST_LIB_VERSION << ";reflexion=" << MINI_OPENMETHOD_REFLEXION
                  << ";iterations=" << iterations << ";graine=" << graine << ";passages=" << repetitions << '\n';
#if defined(__clang__)
        std::cout << "Compilateur=Clang " << __clang_version__ << '\n';
#elif defined(_MSC_FULL_VER)
        std::cout << "Compilateur=MSVC " << _MSC_FULL_VER << '\n';
#else
        std::cout << "Compilateur=" << __VERSION__ << '\n';
#endif
#if defined(NDEBUG)
        std::cout << "NDEBUG=1;LTO=desactive;preparation=hors_chronometrage\n";
#else
        std::cout << "NDEBUG=0;ATTENTION : utiliser Release pour comparer les temps\n";
#endif
        std::cout << "Cas;Types par position;Mini ns/appel;Mini prepare ns/appel;Boost reference ns/appel;Boost prepare ns/appel;Somme\n"
                  << std::fixed << std::setprecision(3);
        comparer_simple<2, false>(iterations, graine32, std::make_index_sequence<2>{});
        comparer_simple<8, false>(iterations, graine32, std::make_index_sequence<8>{});
        comparer_simple<32, false>(iterations, graine32, std::make_index_sequence<32>{});
        comparer_double<2>(iterations, graine32, std::make_index_sequence<2>{}, std::make_index_sequence<4>{});
        comparer_double<8>(iterations, graine32, std::make_index_sequence<8>{}, std::make_index_sequence<64>{});
        comparer_simple<2, true>(iterations, graine32, std::make_index_sequence<2>{});
        comparer_ordinaires<2>(iterations, graine32, std::make_index_sequence<2>{});
    } catch (const std::exception& erreur) {
        std::cerr << erreur.what() << '\n';
        return 1;
    }
}
