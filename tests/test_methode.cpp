#include <mini_openmethod/methode.hpp>
#include <mini_openmethod/methode.hpp>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

using namespace mini_openmethod;
namespace {
struct Animal { virtual ~Animal() = default; };
struct Mammifere : Animal {};
struct Chien : Mammifere {};
struct Chat : Mammifere {};
struct Oiseau : Animal {};
struct Chiot : Chien {};
using Animaux = liste_types<Chien, Chat, Oiseau>;
int nombre_verifications = 0;

/** Reste actif en Release, contrairement a assert. */
void verifier(bool condition, std::string_view message) {
    ++nombre_verifications;
    if (!condition) throw std::runtime_error(std::string(message));
}
template<class Exception, class Fonction>
void verifier_exception(Fonction&& fonction, std::string_view message) {
    try { std::forward<Fonction>(fonction)(); }
    catch (const Exception&) { verifier(true, message); return; }
    verifier(false, message);
}
int decrire_chien(const Chien&) noexcept { return 7; }

/** Le type statique de la reference ne doit pas dicter la specialisation. */
void verifier_dispatch_simple() {
    auto operation = creer_methode<int(const Animal&)>(
        domaines<Animaux>{},
        [](const Animal&) { return 0; },
        [](const Mammifere&) { return 1; },
        [](const Chien&) { return 2; });
    Chien chien;
    Chat chat;
    Oiseau oiseau;
    const Animal& reference = chien;
    verifier(operation(reference) == 2, "Type dynamique a travers une reference de base");
    verifier(operation(chat) == 1, "Repli sur la classe intermediaire");
    verifier(operation(oiseau) == 0, "Repli sur la racine");
    auto inverse = creer_methode<int(const Animal&)>(
        domaines<Animaux>{},
        [](const Chien&) { return 2; },
        [](const Mammifere&) { return 1; },
        [](const Animal&) { return 0; });
    for (const Animal* animal : std::array<const Animal*, 3>{&chien, &chat, &oiseau})
        verifier(operation(*animal) == inverse(*animal), "Independance de l'ordre des specialisations");
    Chiot inconnu;
    verifier_exception<type_inconnu>([&] { (void)operation(inconnu); },
                                    "Un descendant non declare doit etre refuse");
}

/** Verifie toute la matrice, notamment les intersections et l'asymetrie. */
void verifier_dispatch_double() {
    auto operation = creer_methode<int(const Animal&, const Animal&)>(
        domaines<Animaux, Animaux>{},
        [](const Animal&, const Animal&) { return 0; },
        [](const Chien&, const Animal&) { return 1; },
        [](const Animal&, const Chat&) { return 2; },
        [](const Chien&, const Chat&) { return 3; });
    Chien chien;
    Chat chat;
    Oiseau oiseau;
    const std::array<const Animal*, 3> animaux{&chien, &chat, &oiseau};
    constexpr std::array<std::array<int, 3>, 3> attendus{{
        {1, 3, 1}, {0, 2, 0}, {0, 2, 0}}};
    for (std::size_t ligne = 0; ligne < animaux.size(); ++ligne)
        for (std::size_t colonne = 0; colonne < animaux.size(); ++colonne)
            verifier(operation(*animaux[ligne], *animaux[colonne]) == attendus[ligne][colonne],
                     "Matrice complete du double dispatch");
    Chiot inconnu;
    verifier_exception<type_inconnu>([&] { (void)operation(inconnu, chat); }, "Inconnu a gauche");
    verifier_exception<type_inconnu>([&] { (void)operation(chien, inconnu); }, "Inconnu a droite");
}

struct Gauche : virtual Animal { int gauche = 4; };
struct Droite : virtual Animal { int droite = 9; };
struct Diamant : Gauche, Droite {};
struct Etiquette { virtual ~Etiquette() = default; int valeur = 10; };
struct Etiquete : Etiquette, Chien { int valeur = 12; };
struct Support { virtual ~Support() = default; };
struct Papier : Support {};
struct Ecran : Support {};

/** Les ajustements de pointeurs ne doivent pas supposer un heritage simple. */
void verifier_heritages_et_domaines_distincts() {
    auto operation = creer_methode<int(const Animal&)>(
        domaines<liste_types<Diamant>>{},
        [](const Gauche& objet) { return objet.gauche; },
        [](const Droite& objet) { return objet.droite; },
        [](const Diamant& objet) { return objet.gauche + objet.droite; });
    Diamant diamant;
    const Animal& reference = diamant;
    verifier(operation(reference) == 13, "Diamant virtuel et intersection explicite");
    auto repli = creer_methode<int(const Animal&)>(
        domaines<liste_types<Diamant>>{}, [](const Droite& objet) { return objet.droite; });
    verifier(repli(reference) == 9, "Conversion vers une base intermediaire virtuelle");
    auto multiple = creer_methode<int(const Animal&)>(
        domaines<liste_types<Etiquete>>{}, [](const Etiquete& objet) { return objet.valeur; });
    Etiquete etiquete;
    const Animal& base_decalee = etiquete;
    verifier(multiple(base_decalee) == 12, "Ajustement en heritage multiple non virtuel");
    auto afficher = creer_methode<int(const Animal&, const Support&)>(
        domaines<liste_types<Chien, Chat, Oiseau>, liste_types<Papier, Ecran>>{},
        [](const Animal&, const Support&) { return 0; },
        [](const Chat&, const Ecran&) { return 8; });
    Chien chien;
    Chat chat;
    Oiseau oiseau;
    Papier papier;
    Ecran ecran;
    const std::array<const Animal*, 3> animaux{&chien, &chat, &oiseau};
    const std::array<const Support*, 2> supports{&papier, &ecran};
    for (std::size_t ligne = 0; ligne < animaux.size(); ++ligne)
        for (std::size_t colonne = 0; colonne < supports.size(); ++colonne)
            verifier(afficher(*animaux[ligne], *supports[colonne])
                == (ligne == 1 && colonne == 1 ? 8 : 0), "Domaines rectangulaires de racines distinctes");
}

/** Captures, deplacement, void, references et propagation des exceptions. */
void verifier_appelables() {
    Chien chien;
    auto pointeur = creer_methode<int(const Animal&)>(
        domaines<liste_types<Chien>>{}, &decrire_chien);
    verifier(pointeur(chien) == 7, "Pointeur de fonction noexcept");
    auto compteur = creer_methode<int(const Animal&)>(
        domaines<liste_types<Chien>>{},
        [compte = std::make_unique<int>(0)](const Chien&) mutable noexcept { return ++*compte; });
    verifier(compteur(chien) == 1 && compteur(chien) == 2, "Capture mobile et mutable");
    auto deplace = std::move(compteur);
    verifier(deplace(chien) == 3, "Deplacement de la methode");
    int appels = 0;
    auto action = creer_methode<void(const Animal&)>(
        domaines<liste_types<Chien>>{}, [&appels](const Animal&) { ++appels; });
    action(chien);
    verifier(appels == 1, "Retour void et capture par reference");
    int valeur = 42;
    auto reference = creer_methode<int&(const Animal&)>(
        domaines<liste_types<Chien>>{}, [&valeur](const Chien&) -> int& { return valeur; });
    verifier(&reference(chien) == &valeur, "Retour par reference preserve");
    auto echec = creer_methode<void(const Animal&)>(
        domaines<liste_types<Chien>>{},
        [](const Chien&) { throw std::logic_error("Erreur de la specialisation"); });
    verifier_exception<std::logic_error>([&] { echec(chien); }, "Exception utilisateur propagee");
}
} // namespace

int main() {
    try {
        verifier_dispatch_simple();
        verifier_dispatch_double();
        verifier_heritages_et_domaines_distincts();
        verifier_appelables();
        std::cout << nombre_verifications << " verifications reussies\n";
        return 0;
    } catch (const std::exception& erreur) {
        std::cerr << "Echec : " << erreur.what() << '\n';
        return 1;
    }
}
