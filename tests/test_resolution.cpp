#include <mini_openmethod/detail/resolution.hpp>
#include <mini_openmethod/detail/resolution.hpp>
#if MINI_OPENMETHOD_REFLEXION
#include <mini_openmethod/detail/resolution_cpp23.hpp>
#endif

/** @file Verifie les cases produites et compare directement les deux generateurs en C++26. */
namespace {
using namespace mini_openmethod;
static_assert(reflexion_active == MINI_OPENMETHOD_REFLEXION_ATTENDUE);
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};
struct Oiseau : Animal {};
struct Support { virtual ~Support() = default; };
struct Papier : Support {};
struct Ecran : Support {};

template<class Domaines, class... Fonctions>
consteval auto calculer() {
    using Appelables = std::tuple<Fonctions...>;
    constexpr auto indices = detail::resolution<Domaines, Appelables>::indices;
#if MINI_OPENMETHOD_REFLEXION
    static_assert(indices == detail::resolution_cpp23<Domaines, Appelables>::indices,
                  "Les deux versions doivent produire les memes gagnants et les memes erreurs");
#endif
    return indices;
}

using Animaux = liste_types<Chien, Chat, Oiseau>;
using Paire = domaines<liste_types<Chien, Chat>, liste_types<Chien, Chat>>;
static_assert(calculer<domaines<Animaux>, int(const Animal&), int(const Chien&)>()
              == std::array<std::size_t, 3>{1, 0, 0});
static_assert(calculer<domaines<Animaux>, int(const Chien&), int(const Animal&)>()
              == std::array<std::size_t, 3>{0, 1, 1});
static_assert(calculer<domaines<Animaux, liste_types<Papier, Ecran>>,
                       int(const Animal&, const Support&), int(const Chat&, const Ecran&)>()
              == std::array<std::size_t, 6>{0, 0, 0, 1, 0, 0});
// Les sentinelles distinguent une ambiguite (K + 1) d'une absence (K).
static_assert(calculer<Paire, int(const Chien&, const Animal&), int(const Animal&, const Chien&)>()
              == std::array<std::size_t, 4>{3, 0, 1, 2});
static_assert(calculer<Paire, int(const Chien&, const Animal&), int(const Animal&, const Chien&),
                       int(const Chien&, const Chien&), int(const Animal&, const Animal&)>()
              == std::array<std::size_t, 4>{2, 0, 1, 3});
static_assert(calculer<domaines<liste_types<Chien>>, int(const Chien&), int(const Chien&)>()
              == std::array<std::size_t, 1>{3});
static_assert(calculer<domaines<liste_types<Chien>>, int(const Chat&)>()
              == std::array<std::size_t, 1>{1});

struct Racine { virtual ~Racine() = default; };
struct Specialisation : virtual Racine {};
struct Cache : private Specialisation, virtual Racine {};
struct Gauche : Specialisation {};
struct Droite : Specialisation {};
struct Ambigu : Gauche, Droite {};
static_assert(std::derived_from<Cache, Racine> && std::derived_from<Ambigu, Racine>);
static_assert(!std::derived_from<Cache, Specialisation> && !std::derived_from<Ambigu, Specialisation>);
static_assert(calculer<domaines<liste_types<Cache, Ambigu, Specialisation>>,
                       int(const Racine&), int(const Specialisation&)>()
              == std::array<std::size_t, 3>{0, 0, 1});
} // namespace
