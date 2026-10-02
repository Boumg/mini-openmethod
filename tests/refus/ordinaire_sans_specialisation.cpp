#include <mini_openmethod/methode.hpp>

/** @file Refus attendu : Combinaison sans specialisation applicable. */
using namespace mini_openmethod;
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};
using Domaine = liste_types<Chien>;
int main() {
    auto operation = creer_methode<int(int, const Animal&)>(
        domaines<argument_ordinaire, liste_types<Chien, Chat>>{},
        [](int valeur, const Chien&) { return valeur; });
    (void)operation;
}
