#include "animaux.hpp"
auto operation = creer_methode<void(volatile Animal&)>(
    domaines<liste_types<Chien>>{}, [](volatile Chien&) {});
int main() {}
