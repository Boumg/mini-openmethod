#include <mini_openmethod/methode.hpp>
#include "animaux.hpp"
#include <iostream>
#include <string_view>
using namespace mini_openmethod;

int main() {
    using Animaux = liste_types<Chien, Chat>;
    auto rencontrer = creer_methode<std::string_view(const Animal&, const Animal&)>(
        domaines<Animaux, Animaux>{},
        [](const Animal&, const Animal&) -> std::string_view { return "observation"; },
        [](const Chien&, const Chat&) -> std::string_view { return "poursuite"; },
        [](const Chat&, const Chien&) -> std::string_view { return "prudence"; });
    Chien chien;
    Chat chat;
    std::cout << rencontrer(chien, chat) << '\n' << rencontrer(chat, chien) << '\n';
    return rencontrer(chien, chat) == "poursuite"
        && rencontrer(chat, chien) == "prudence" ? 0 : 1;
}
