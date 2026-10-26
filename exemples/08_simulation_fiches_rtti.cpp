#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

/**
 * @file Simulation autonome : aucune fausse adresse n'est convertie en pointeur.
 * Le nom joue le role de l'identite du type, uniquement pour cette demonstration.
 * Le vrai moteur utilise std::type_info::operator==, jamais cette comparaison de noms.
 */
namespace {
struct Fiche_simulee {
    std::uint64_t adresse;
    std::string_view type;
};
struct Resultat {
    std::size_t indice;
    bool repli_par_type;
};
struct Type_inconnu_simule : std::runtime_error {
    Type_inconnu_simule() : std::runtime_error("Type absent du domaine simule") {}
};

constexpr std::uint64_t melanger(std::uint64_t adresse) {
    return (adresse >> 3) * UINT64_C(0x9e3779b97f4a7c15);
}
constexpr std::size_t choisir_case(std::uint64_t adresse, std::size_t capacite) {
    // Precondition locale : capacite est une puissance de deux au moins egale a 2.
    return static_cast<std::size_t>(melanger(adresse) >> (64 - std::countr_zero(capacite)));
}
static_assert(std::bit_ceil(2u * 9u) == 32u);
static_assert(std::bit_ceil(2u * 16u) == 32u);
static_assert(std::bit_ceil(2u * 17u) == 64u);
static_assert(melanger(0x1000) == UINT64_C(0x6ef372fe94f82a00));
static_assert(choisir_case(0x1000, 32) == 13);
static_assert(choisir_case(0x3000, 32) == 9);

template<std::size_t Nombre>
class Index_simule {
    static_assert(Nombre > 0);
    static constexpr std::size_t capacite = std::bit_ceil(2 * Nombre);
    // Nombre est une sentinelle : les indices valides vont de 0 a Nombre - 1.
    std::array<Fiche_simulee, Nombre> fiches_;
    std::array<std::size_t, capacite> cases_;
    bool collisions_;

    std::size_t depart(std::uint64_t adresse) const {
        return collisions_ ? capacite - 1 : choisir_case(adresse, capacite);
    }
public:
    explicit Index_simule(std::array<Fiche_simulee, Nombre> fiches, bool collisions = false)
        : fiches_(fiches), collisions_(collisions) {
        cases_.fill(Nombre);
        for (std::size_t indice = 0; indice < Nombre; ++indice) {
            auto position = depart(fiches_[indice].adresse);
            while (cases_[position] != Nombre)
                position = (position + 1) & (capacite - 1);
            cases_[position] = indice;
        }
    }

    Resultat chercher(Fiche_simulee recue, bool tracer = false) const {
        // Controle propre a la simulation : une adresse ne peut pas designer
        // simultanement deux fiches de types differents. Le vrai moteur n'a
        // pas besoin de ce controle, puisqu'il recoit de vrais objets type_info.
        for (const auto& fiche : fiches_)
            if (fiche.adresse == recue.adresse && fiche.type != recue.type)
                throw std::invalid_argument("Simulation incoherente : meme adresse, types differents");

        auto position = depart(recue.adresse);
        while (cases_[position] != Nombre) {
            const auto candidat = cases_[position];
            if (tracer) std::cout << "  case " << position << " -> " << fiches_[candidat].type << '\n';
            if (fiches_[candidat].adresse == recue.adresse) return {candidat, false};
            position = (position + 1) & (capacite - 1);
        }
        if (tracer) std::cout << "  case " << position << " vide -> comparaison de tous les types\n";
        for (std::size_t candidat = 0; candidat < Nombre; ++candidat)
            if (recue.type == fiches_[candidat].type) return {candidat, true};
        throw Type_inconnu_simule{};
    }
};

void exiger(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class Erreur, class Appel>
void attendre_erreur(std::string_view nom, Appel appel) {
    try {
        appel();
    } catch (const Erreur& erreur) {
        std::cout << nom << " : erreur attendue : " << erreur.what() << '\n';
        return;
    }
    throw std::runtime_error("L'erreur attendue n'a pas ete levee");
}

void montrer_simulation() {
    const std::array<Fiche_simulee, 9> fiches{{
        {0x1008, "Chien"}, {0x1000, "Chat"}, {0x1010, "Oiseau"},
        {0x1018, "Poisson"}, {0x1020, "Cheval"}, {0x1028, "Vache"},
        {0x1030, "Mouton"}, {0x1038, "Lapin"}, {0x1040, "Tortue"}}};
    const Index_simule<9> normal(fiches);
    const Index_simule<9> collisions(fiches, true);

    std::cout << "Simulation 64 bits, 9 types, 32 cases\n"
              << "Chat : adresse 0x1000 -> case " << choisir_case(0x1000, 32) << '\n'
              << "Autre fiche Chat : 0x3000 -> case " << choisir_case(0x3000, 32) << '\n';

    // Toutes les fiches connues, puis une autre adresse pour chacun des memes types.
    // On exerce chaque cas avec le hachage normal et avec toutes les collisions.
    for (const auto* index : std::array{&normal, &collisions}) {
        std::cout << (index == &normal ? "Hachage normal\n" : "Collisions forcees\n");
        for (std::size_t indice = 0; indice < fiches.size(); ++indice) {
            const auto direct = index->chercher(fiches[indice]);
            exiger(direct.indice == indice && !direct.repli_par_type, "Recherche par adresse incorrecte");
            const Fiche_simulee autre{fiches[indice].adresse + 0x2000, fiches[indice].type};
            const auto repli = index->chercher(autre);
            exiger(repli.indice == indice && repli.repli_par_type, "Repli par type incorrect");
        }
        attendre_erreur<Type_inconnu_simule>("Adresse nouvelle, type inconnu", [&] {
            (void)index->chercher({0x5000, "Loup"}, true);
        });
    }
    std::cout << "36 recherches connues verifiees : 9 types x 2 adresses x 2 hachages\n";

    std::cout << "Autre fiche pour Chat : recherche par type\n";
    const auto chat = normal.chercher({0x3000, "Chat"}, true);
    exiger(chat.indice == 1 && chat.repli_par_type, "La seconde fiche Chat doit donner 1");
    std::cout << "  resultat : indice " << chat.indice << '\n';

    std::cout << "Collisions imposees : Tortue apres retour de 31 vers 0\n";
    const auto tortue = collisions.chercher(fiches[8], true);
    exiger(tortue.indice == 8 && !tortue.repli_par_type, "Retour au debut incorrect");

    attendre_erreur<std::invalid_argument>("Meme adresse, type different : scenario impossible", [&] {
        (void)normal.chercher({0x1000, "Loup"});
    });
}
} // namespace

int main() {
    try {
        montrer_simulation();
        return 0;
    } catch (const std::exception& erreur) {
        std::cerr << "Echec de l'exemple : " << erreur.what() << '\n';
        return 1;
    }
}
