#include <mini_openmethod/methode.hpp>
#include "animaux.hpp"
#include <iostream>
#include <string_view>
using namespace mini_openmethod;

int main() {
    // Mammifere peut servir de specialisation sans etre un type dynamique du domaine.
    auto decrire = creer_methode<std::string_view(const Animal&)>(
        domaines<liste_types<Chien, Chat, Oiseau>>{},
        [](const Animal&) -> std::string_view { return "animal"; },
        [](const Mammifere&) -> std::string_view { return "mammifere"; },
        [](const Chien&) -> std::string_view { return "chien"; });
    Chien chien;
    Chat chat;
    Oiseau oiseau;
    std::cout << decrire(chien) << '\n' << decrire(chat) << '\n' << decrire(oiseau) << '\n';
    return decrire(chien) == "chien" && decrire(chat) == "mammifere"
        && decrire(oiseau) == "animal" ? 0 : 1;
}
