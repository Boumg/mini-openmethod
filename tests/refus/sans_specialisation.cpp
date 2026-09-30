#include "animaux.hpp"
int main() {
    auto operation = creer_methode<int(const Animal&)>(
        domaines<liste_types<Chien, Chat>>{},
        [](const Chien&) { return 1; });
    (void)operation;
}
