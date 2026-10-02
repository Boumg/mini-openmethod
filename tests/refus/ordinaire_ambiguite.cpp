#include <mini_openmethod/methode.hpp>

/** @file Refus attendu : Ambiguite : plusieurs specialisations maximales. */
using namespace mini_openmethod;
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};
using Domaine = liste_types<Chien>;
int main() {
    auto operation = creer_methode<int(const Animal&, int, const Animal&)>(
        domaines<Domaine, argument_ordinaire, Domaine>{},
        [](const Chien&, int valeur, const Animal&) { return valeur; },
        [](const Animal&, int valeur, const Chien&) { return valeur; });
    (void)operation;
}
