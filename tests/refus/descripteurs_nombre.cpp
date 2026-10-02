#include <mini_openmethod/methode.hpp>

/** @file Refus attendu : Un descripteur par argument est requis. */
using namespace mini_openmethod;
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};
using Domaine = liste_types<Chien>;
int main() {
    auto operation = creer_methode<int(const Animal&, int)>(domaines<Domaine>{},
        [](const Animal&, int valeur) { return valeur; });
    (void)operation;
}
