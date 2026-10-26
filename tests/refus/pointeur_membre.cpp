#include "animaux.hpp"

/** Le pointeur seul ne fournit pas l'objet receveur du traitement. */
struct Traitement {
    int executer(const Chien&) const { return 1; }
};

int main() {
    // La construction doit suffire au refus, sans appeler la methode.
    auto operation = creer_methode<int(const Animal&)>(
        domaines<liste_types<Chien>>{}, &Traitement::executer);
    (void)operation;
}
