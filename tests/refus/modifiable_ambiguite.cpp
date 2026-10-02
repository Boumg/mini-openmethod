#include "animaux.hpp"
auto operation = creer_methode<void(Animal&, const Animal&)>(
    domaines<liste_types<Chien>, liste_types<Chat>>{},
    [](Chien&, const Animal&) {}, [](Animal&, const Chat&) {});
int main() {}
