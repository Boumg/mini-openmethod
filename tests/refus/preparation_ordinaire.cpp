#include <mini_openmethod/methode.hpp>

/** @file Refus attendu : Une position ordinaire ne peut pas etre preparee. */
using namespace mini_openmethod;
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};
using Domaine = liste_types<Chien>;
int main() {
    auto operation = creer_methode<int(int, const Animal&)>(domaines<argument_ordinaire, Domaine>{},
        [](int valeur, const Animal&) { return valeur; });
    int valeur = 1;
    operation.preparer<0>(valeur);
    (void)operation;
}
