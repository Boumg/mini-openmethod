#pragma once
#include <stdexcept>

/** @file Description des domaines fermes et signalement des types inconnus. */
namespace mini_openmethod {
/** Types dynamiques exacts autorises pour une position. */
template<class... Types> struct liste_types {};
/** Argument transmis a la specialisation sans participer au dispatch. */
struct argument_ordinaire {};
/** Un descripteur par argument : liste_types ou argument_ordinaire, dans l'ordre. */
template<class... Listes> struct domaines {};
/** Un type dynamique absent du domaine est toujours refuse. */
class type_inconnu : public std::runtime_error {
public:
    type_inconnu() : std::runtime_error("Type dynamique absent du domaine ferme") {}
};
} // namespace mini_openmethod
