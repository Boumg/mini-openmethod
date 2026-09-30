#include <mini_openmethod/methode.hpp>
#include "animaux.hpp"
#include <iostream>
using namespace mini_openmethod;

int main() {
    using Animaux = liste_types<Chien, Chat>;
    auto evaluer = creer_methode<int(const Animal&, const Animal&)>(
        domaines<Animaux, Animaux>{},
        [](const Animal&, const Animal&) { return 0; },
        [](const Chien&, const Animal&) { return 1; },
        [](const Animal&, const Chien&) { return 2; },
        // Cette intersection domine les deux candidats incomparables.
        // La retirer provoque l'erreur testee dans tests/refus/ambiguite_double.cpp.
        [](const Chien&, const Chien&) { return 3; });
    Chien chien;
    std::cout << evaluer(chien, chien) << '\n';
    return evaluer(chien, chien) == 3 ? 0 : 1;
}
