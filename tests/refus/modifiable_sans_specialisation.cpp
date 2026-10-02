#include "animaux.hpp"
auto operation = creer_methode<void(Animal&)>(
    domaines<liste_types<Chien, Chat>>{}, [](Chien&) {});
int main() {}
