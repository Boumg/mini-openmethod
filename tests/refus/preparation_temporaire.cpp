#include <mini_openmethod/methode.hpp>
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
int main() {
    using namespace mini_openmethod;
    auto operation = creer_methode<int(const Animal&)>(domaines<liste_types<Chien>>{},
        [](const Chien&) { return 1; });
    (void)operation.preparer(Chien{});
}
