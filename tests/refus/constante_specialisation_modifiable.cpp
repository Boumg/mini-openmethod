#include "animaux.hpp"
auto operation = creer_methode<void(const Animal&)>(
    domaines<liste_types<Chien>>{}, [](Chien&) {});
int main() {}
