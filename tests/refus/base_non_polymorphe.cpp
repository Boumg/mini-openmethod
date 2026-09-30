#include <mini_openmethod/methode.hpp>
struct Base {};
int main() {
    auto operation = mini_openmethod::creer_methode<int(const Base&)>(
        mini_openmethod::domaines<mini_openmethod::liste_types<Base>>{},
        [](const Base&) { return 1; });
    (void)operation;
}
