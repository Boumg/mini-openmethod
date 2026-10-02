#include <mini_openmethod/methode.hpp>
#include <iostream>

/** @file Modifie l'objet d'origine puis reutilise sa reference preparee en lecture seule. */
struct Animal { virtual ~Animal() = default; int energie = 0; };
struct Chien : Animal {};
struct Chat : Animal {};

int main() {
    using namespace mini_openmethod;
    using Animaux = liste_types<Chien, Chat>;
    auto nourrir = creer_methode<void(Animal&, int)>(
        domaines<Animaux, argument_ordinaire>{},
        [](Chien& chien, int quantite) { chien.energie += 2 * quantite; },
        [](Chat& chat, int quantite) { chat.energie += quantite; });
    auto lire = creer_methode<int(const Animal&)>(
        domaines<Animaux>{}, [](const Animal& animal) { return animal.energie; });

    Chien chien;
    Chat chat;
    Animal& animal = chien;
    nourrir(animal, 3);
    const auto prepare = nourrir.preparer(animal);
    nourrir(prepare, 2);
    nourrir(chat, 4);
    const int energie = lire(prepare); // Conversion vers un acces constant.
    std::cout << "Energie du chien : " << energie << '\n';
    std::cout << "Energie du chat : " << lire(chat) << '\n';
    return energie == 10 && lire(chat) == 4 ? 0 : 1;
}
