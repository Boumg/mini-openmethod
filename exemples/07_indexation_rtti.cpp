#include <mini_openmethod/detail/index_types.hpp>
#include <mini_openmethod/methode.hpp>
#include "animaux.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

/** @file Cas reels : recherches, collisions, preparation et erreurs attendues. */
namespace {
using namespace mini_openmethod;
struct Poisson : Animal {};
struct Cheval : Animal {};
struct Vache : Animal {};
struct Mouton : Animal {};
struct Lapin : Animal {};
struct Tortue : Animal {};
struct Loup : Chien {}; // Descendant absent des listes, donc refuse.

using Petit_domaine = liste_types<Chien, Chat, Oiseau>;
using Grand_domaine = liste_types<Chien, Chat, Oiseau, Poisson, Cheval,
                                  Vache, Mouton, Lapin, Tortue>;

void exiger(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class Appel>
void attendre_type_inconnu(std::string_view nom, Appel appel) {
    try {
        appel();
    } catch (const type_inconnu& erreur) {
        std::cout << "  " << nom << " : erreur attendue : " << erreur.what() << '\n';
        return;
    }
    throw std::runtime_error("L'erreur type_inconnu attendue n'a pas ete levee");
}

/** Les bits hauts a 1 imposent la derniere case comme depart pour tous les types. */
struct Hachage_collision {
    std::size_t operator()(const std::type_info*) const noexcept {
        return std::numeric_limits<std::size_t>::max();
    }
};

void montrer_indexation() {
    Chien chien; Chat chat; Oiseau oiseau; Poisson poisson; Cheval cheval;
    Vache vache; Mouton mouton; Lapin lapin; Tortue tortue;
    const std::array<const Animal*, 9> objets{
        &chien, &chat, &oiseau, &poisson, &cheval, &vache, &mouton, &lapin, &tortue};
    const std::array noms{"Chien", "Chat", "Oiseau", "Poisson", "Cheval",
                          "Vache", "Mouton", "Lapin", "Tortue"};
    const std::array types{&typeid(Chien), &typeid(Chat), &typeid(Oiseau),
                          &typeid(Poisson), &typeid(Cheval), &typeid(Vache),
                          &typeid(Mouton), &typeid(Lapin), &typeid(Tortue)};

    // Utilisation didactique d'un detail interne : 9 types, 32 cases.
    // Les insertions forcees occupent 31, 0, 1, 2, 3, 4, 5, 6 et 7.
    const detail::index_types<9, Hachage_collision> collisions(types);
    auto identifier = creer_methode<std::size_t(const Animal&)>(domaines<Grand_domaine>{},
        [](const Chien&) -> std::size_t { return 0; },
        [](const Chat&) -> std::size_t { return 1; },
        [](const Oiseau&) -> std::size_t { return 2; },
        [](const Poisson&) -> std::size_t { return 3; },
        [](const Cheval&) -> std::size_t { return 4; },
        [](const Vache&) -> std::size_t { return 5; },
        [](const Mouton&) -> std::size_t { return 6; },
        [](const Lapin&) -> std::size_t { return 7; },
        [](const Tortue&) -> std::size_t { return 8; });

    std::cout << "Recherche des neuf types connus\n";
    for (std::size_t indice = 0; indice < objets.size(); ++indice) {
        const Animal& objet = *objets[indice];
        if (indice < 3)
            exiger(detail::traits_liste<Petit_domaine>::indice(typeid(objet)) == indice,
                   "Indice lineaire incorrect");
        exiger(detail::traits_liste<Grand_domaine>::indice(typeid(objet)) == indice,
               "Indice hache incorrect");
        exiger(collisions.indice(typeid(objet)) == indice, "Collision mal resolue");
        exiger(identifier(objet) == indice, "Dispatch incorrect");
        exiger(identifier(identifier.preparer(objet)) == indice, "Appel prepare incorrect");
        std::cout << "  " << noms[indice] << " -> " << indice
                  << " ; collisions et appel prepare verifies\n";
    }

    // Le meme type pour deux objets distincts ; aucune exigence sur les adresses RTTI.
    Chat autre_chat;
    exiger(typeid(chat) == typeid(autre_chat), "Deux chats doivent avoir le meme type");
    exiger(identifier(autre_chat) == 1, "Le second chat doit garder l'indice 1");

    std::cout << "Types inconnus\n";
    Animal animal;
    Loup loup;
    const std::array<const Animal*, 2> inconnus{&animal, &loup};
    const std::array noms_inconnus{"Animal", "Loup"};
    for (std::size_t indice = 0; indice < inconnus.size(); ++indice) {
        const Animal* objet = inconnus[indice];
        std::cout << "  Objet de type exact " << noms_inconnus[indice] << '\n';
        attendre_type_inconnu("recherche lineaire", [&] {
            (void)detail::traits_liste<Petit_domaine>::indice(typeid(*objet));
        });
        attendre_type_inconnu("recherche hachee", [&] {
            (void)detail::traits_liste<Grand_domaine>::indice(typeid(*objet));
        });
        attendre_type_inconnu("collisions imposees", [&] { (void)collisions.indice(typeid(*objet)); });
        attendre_type_inconnu("dispatch", [&] { (void)identifier(*objet); });
        attendre_type_inconnu("preparation", [&] { (void)identifier.preparer(*objet); });
    }
    // Un type connu d'une liste peut etre inconnu d'une autre.
    attendre_type_inconnu("Tortue dans le petit domaine", [&] {
        (void)detail::traits_liste<Petit_domaine>::indice(typeid(tortue));
    });
}

void montrer_toutes_les_paires() {
    using Animaux = liste_types<Chien, Chat>;
    auto rencontrer = creer_methode<std::string_view(const Animal&, const Animal&)>(
        domaines<Animaux, Animaux>{},
        [](const Animal&, const Animal&) -> std::string_view { return "observation"; },
        [](const Chien&, const Chat&) -> std::string_view { return "poursuite"; },
        [](const Chat&, const Chien&) -> std::string_view { return "prudence"; });
    Chien chien; Chat chat; Loup loup;
    const std::array<const Animal*, 3> objets{&chien, &chat, &loup};
    const std::array noms{"Chien", "Chat", "Loup"};
    const std::array<std::string_view, 4> attendus{
        "observation", "poursuite", "prudence", "observation"};

    std::cout << "Les neuf couples Chien / Chat / Loup\n";
    for (std::size_t gauche = 0; gauche < objets.size(); ++gauche) {
        for (std::size_t droite = 0; droite < objets.size(); ++droite) {
            std::cout << "  " << noms[gauche] << " / " << noms[droite] << " -> ";
            if (gauche == 2 || droite == 2) {
                attendre_type_inconnu("appel", [&] {
                    (void)rencontrer(*objets[gauche], *objets[droite]);
                });
            } else {
                const auto resultat = rencontrer(*objets[gauche], *objets[droite]);
                exiger(resultat == attendus[gauche * 2 + droite], "Mauvaise case de dispatch");
                const auto premier = rencontrer.preparer<0>(*objets[gauche]);
                const auto second = rencontrer.preparer<1>(*objets[droite]);
                exiger(rencontrer(premier, second) == resultat, "Paire preparee incorrecte");
                std::cout << resultat << '\n';
            }
        }
    }
}
} // namespace

int main() {
    try {
        montrer_indexation();
        montrer_toutes_les_paires();
        return 0;
    } catch (const std::exception& erreur) {
        std::cerr << "Echec de l'exemple : " << erreur.what() << '\n';
        return 1;
    }
}
