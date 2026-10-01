#pragma once

/** @file Selection automatique selon les capacites actives du compilateur et de la bibliotheque. */
// Une valeur explicite 0 ou 1 permet de comparer les deux implementations.
#if !defined(MINI_OPENMETHOD_REFLEXION) || MINI_OPENMETHOD_REFLEXION
#if defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202506L && __has_include(<meta>)
#include <meta>
#endif
#endif

#ifndef MINI_OPENMETHOD_REFLEXION
#if defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202506L \
    && defined(__cpp_lib_reflection) && __cpp_lib_reflection >= 202506L
#define MINI_OPENMETHOD_REFLEXION 1
#else
#define MINI_OPENMETHOD_REFLEXION 0
#endif
#endif

#if MINI_OPENMETHOD_REFLEXION != 0 && MINI_OPENMETHOD_REFLEXION != 1
#error "MINI_OPENMETHOD_REFLEXION doit valoir 0 ou 1 dans le code C++"
#endif
#if MINI_OPENMETHOD_REFLEXION
#if !defined(__cpp_impl_reflection) || __cpp_impl_reflection < 202506L \
    || !defined(__cpp_lib_reflection) || __cpp_lib_reflection < 202506L
#error "Reflexion C++26 indisponible : compilateur et bibliotheque <meta> compatibles requis"
#endif
#endif

namespace mini_openmethod {
/** Vrai si la construction des tables utilise la reflexion C++26. */
inline constexpr bool reflexion_active = MINI_OPENMETHOD_REFLEXION != 0;
} // namespace mini_openmethod
