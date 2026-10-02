#include <mini_openmethod/methode.hpp>

/** @file Refus attendu : Une ou deux positions polymorphes sont requises. */
using namespace mini_openmethod;
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};
using Domaine = liste_types<Chien>;
int main() {
    auto operation = creer_methode<int(const Animal&, int, const Animal&, const Animal&)>(
        domaines<Domaine, argument_ordinaire, Domaine, Domaine>{},
        [](const Animal&, int valeur, const Animal&, const Animal&) { return valeur; });
    (void)operation;
}
