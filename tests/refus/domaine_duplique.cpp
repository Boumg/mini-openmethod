#include "animaux.hpp"
int main() {
    auto operation = creer_methode<int(const Animal&)>(
        domaines<liste_types<Chien, Chien>>{}, [](const Animal&) { return 1; });
    (void)operation;
}
