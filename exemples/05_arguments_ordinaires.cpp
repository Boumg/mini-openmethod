#include <mini_openmethod/methode.hpp>
#include <iostream>
#include <sstream>

/** @file Une facture choisie par l'animal, avec montant et flux transmis directement. */
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};

int main() {
    using namespace mini_openmethod;
    auto facturer = creer_methode<void(const Animal&, double, std::ostream&)>(
        domaines<liste_types<Chien, Chat>, argument_ordinaire, argument_ordinaire>{},
        [](const Chien&, double montant, std::ostream& sortie) { sortie << "Chien : " << montant << '\n'; },
        [](const Chat&, double montant, std::ostream& sortie) { sortie << "Chat : " << montant << '\n'; });
    Chien chien;
    Chat chat;
    std::ostringstream sortie;
    facturer(chien, 12.5, sortie);
    facturer(facturer.preparer(chat), 18.0, sortie);
    std::cout << sortie.str();
    return sortie.str() == "Chien : 12.5\nChat : 18\n" ? 0 : 1;
}
