#include "animaux.hpp"
int main() {
    auto modifier = creer_methode<void(Animal&)>(
        domaines<liste_types<Chien>>{}, [](Chien&) {});
    auto lire = creer_methode<void(const Animal&)>(
        domaines<liste_types<Chien>>{}, [](const Chien&) {});
    Chien chien;
    modifier(lire.preparer(chien));
}
