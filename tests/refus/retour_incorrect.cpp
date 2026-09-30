#include "animaux.hpp"
int main() {
    auto operation = creer_methode<int(const Animal&)>(
        domaines<liste_types<Chien>>{}, [](const Chien&) { return 1.0; });
    (void)operation;
}
