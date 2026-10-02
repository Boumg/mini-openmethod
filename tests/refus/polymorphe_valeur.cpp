#include "animaux.hpp"
auto operation = creer_methode<void(Animal)>(
    domaines<liste_types<Chien>>{}, [](Chien) {});
int main() {}
