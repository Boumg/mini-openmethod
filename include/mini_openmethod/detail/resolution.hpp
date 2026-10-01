#pragma once
#include <mini_openmethod/configuration.hpp>
#if MINI_OPENMETHOD_REFLEXION
#include <mini_openmethod/detail/resolution_cpp26.hpp>
#else
#include <mini_openmethod/detail/resolution_cpp23.hpp>
#endif

/** @file Point de selection de la construction des tables. */
namespace mini_openmethod::detail {
template<class Domaines, class Fonctions>
using resolution =
#if MINI_OPENMETHOD_REFLEXION
    resolution_cpp26<Domaines, Fonctions>;
#else
    resolution_cpp23<Domaines, Fonctions>;
#endif
} // namespace mini_openmethod::detail
