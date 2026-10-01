#include <mini_openmethod/methode.hpp>
#include "animaux.hpp"

/** Les arguments variables C ne doivent pas etre oublies par l'analyse des parametres. */
int traiter(const Animal&, ...) { return 1; }

int main() {
    auto operation = mini_openmethod::creer_methode<int(const Animal&)>(
        mini_openmethod::domaines<mini_openmethod::liste_types<Chien>>{}, &traiter);
    (void)operation;
}
