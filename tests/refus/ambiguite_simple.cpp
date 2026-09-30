#include "animaux.hpp"
struct Gauche : virtual Animal {};
struct Droite : virtual Animal {};
struct Diamant : Gauche, Droite {};
int main() {
    auto operation = creer_methode<int(const Animal&)>(
        domaines<liste_types<Diamant>>{},
        [](const Gauche&) { return 1; },
        [](const Droite&) { return 2; });
    (void)operation;
}
