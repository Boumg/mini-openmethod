#include <mini_openmethod/methode.hpp>

/** @file Refus attendu : Specialisation invalide. */
using namespace mini_openmethod;
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};
using Domaine = liste_types<Chien>;
int main() {
    auto operation = creer_methode<int(const Animal&, int)>(domaines<Domaine, argument_ordinaire>{},
        [](const Animal&, double) { return 0; });
    (void)operation;
}
