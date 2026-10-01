#include <mini_openmethod/configuration.hpp>
#include <mini_openmethod/configuration.hpp>
#include <mini_openmethod/methode.hpp>

/** @file Verifie la selection sans definition de mode imposee par la cible CMake. */
static_assert(mini_openmethod::reflexion_active == MINI_OPENMETHOD_REFLEXION_ATTENDUE,
              "La detection doit choisir l'implementation attendue");
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
int main() {
    auto operation = mini_openmethod::creer_methode<int(const Animal&)>(
        mini_openmethod::domaines<mini_openmethod::liste_types<Chien>>{},
        [](const Chien&) { return 7; });
    return operation(Chien{}) == 7 ? 0 : 1;
}
