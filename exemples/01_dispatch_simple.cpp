#include <mini_openmethod/methode.hpp>
#include "animaux.hpp"
#include <iostream>
#include <string_view>
using namespace mini_openmethod;

int main() {
    auto parler = creer_methode<std::string_view(const Animal&)>(
        domaines<liste_types<Chien, Chat>>{},
        [](const Chien&) -> std::string_view { return "ouaf"; },
        [](const Chat&) -> std::string_view { return "miaou"; });
    Chien chien;
    Chat chat;
    const Animal& premier = chien;
    const Animal& second = chat;
    std::cout << parler(premier) << '\n' << parler(second) << '\n';
    return parler(premier) == "ouaf" && parler(second) == "miaou" ? 0 : 1;
}
