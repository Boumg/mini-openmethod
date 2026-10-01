#pragma once
#include <stdexcept>

/** @file Description des domaines fermes et signalement des types inconnus. */
namespace mini_openmethod {
/** Types dynamiques exacts autorises pour une position. */
template<class... Types> struct liste_types {};
/** Un domaine par argument, dans l'ordre de la signature. */
template<class... Listes> struct domaines {};
/** Un type dynamique absent du domaine est toujours refuse. */
class type_inconnu : public std::runtime_error {
public:
    type_inconnu() : std::runtime_error("Type dynamique absent du domaine ferme") {}
};
} // namespace mini_openmethod
