#include "animaux.hpp"
int main() {
    auto operation = creer_methode<void(Animal&)>(
        domaines<liste_types<Chien>>{}, [](Chien&) {});
    (void)operation.preparer(Chien{});
}
