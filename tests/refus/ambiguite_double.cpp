#include "animaux.hpp"
// Ambiguite sur Chien x Chien, meme sans aucun appel de la methode.
int main() {
    auto operation = creer_methode<int(const Animal&, const Animal&)>(
        domaines<liste_types<Chien>, liste_types<Chien>>{},
        [](const Chien&, const Animal&) { return 1; },
        [](const Animal&, const Chien&) { return 2; });
    (void)operation;
}
