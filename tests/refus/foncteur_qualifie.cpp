#include <mini_openmethod/methode.hpp>
#include "animaux.hpp"

/** Un operateur qualifie par reference reste hors du contrat dans les deux modes. */
struct Traitement {
    int operator()(const Animal&) & { return 1; }
};

int main() {
    auto operation = mini_openmethod::creer_methode<int(const Animal&)>(
        mini_openmethod::domaines<mini_openmethod::liste_types<Chien>>{}, Traitement{});
    (void)operation;
}
