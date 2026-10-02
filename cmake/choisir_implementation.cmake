# Une vraie construction de table verifie ensemble le langage et la bibliotheque.
function(sonder_reflexion entetes options resultat)
    include(CheckCXXSourceCompiles)
    include(CMakePushCheckState)
    cmake_push_check_state(RESET)
    set(CMAKE_CXX_STANDARD 26)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_REQUIRED_INCLUDES "${entetes}")
    set(CMAKE_REQUIRED_LIBRARIES Boost::callable_traits Boost::mp11)
    set(CMAKE_REQUIRED_DEFINITIONS -DMINI_OPENMETHOD_REFLEXION=1)
    set(CMAKE_REQUIRED_FLAGS "${options}")
    unset(REFLEXION_VERIFIEE CACHE)
    check_cxx_source_compiles([=[
        #include <mini_openmethod/methode.hpp>
        struct Animal { virtual ~Animal() = default; };
        struct Chien : Animal {};
        struct Chat : Animal {};
        int main() {
            using namespace mini_openmethod;
            static_assert(reflexion_active);
            auto operation = creer_methode<int(const Animal&, const Animal&)>(
                domaines<liste_types<Chien, Chat>, liste_types<Chien, Chat>>{},
                [](const Animal&, const Animal&) { return 0; },
                [](const Chien&, const Chat&) noexcept { return 1; });
            return operation(Chien{}, Chat{}) != 1;
        }
    ]=] REFLEXION_VERIFIEE)
    cmake_pop_check_state()
    set(${resultat} "${REFLEXION_VERIFIEE}" PARENT_SCOPE)
endfunction()

function(configurer_implementation cible entetes)
    set(MINI_OPENMETHOD_REFLEXION AUTO CACHE STRING "Implementation : AUTO, ON (C++26) ou OFF (C++23)")
    set_property(CACHE MINI_OPENMETHOD_REFLEXION PROPERTY STRINGS AUTO ON OFF)
    string(TOUPPER "${MINI_OPENMETHOD_REFLEXION}" mode)
    if(NOT mode MATCHES "^(AUTO|ON|OFF)$")
        message(FATAL_ERROR "MINI_OPENMETHOD_REFLEXION doit valoir AUTO, ON ou OFF")
    endif()
    set(active 0)
    set(options "")
    set(standard 23)
    if(NOT mode STREQUAL "OFF" AND "cxx_std_26" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        sonder_reflexion("${entetes}" "" disponible)
        if(NOT disponible AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            set(options -freflection)
            sonder_reflexion("${entetes}" "${options}" disponible)
        endif()
        if(disponible)
            set(active 1)
            set(standard 26)
        endif()
    endif()
    if(NOT active)
        set(options "")
        if(mode STREQUAL "ON")
            message(FATAL_ERROR "Reflexion C++26 indisponible : compilateur et bibliotheque <meta> compatibles requis. Utiliser AUTO ou OFF pour conserver C++23.")
        endif()
    endif()
    # L'installation exporte uniquement le socle C++23 ; le consommateur refait la sonde.
    get_target_property(importee ${cible} IMPORTED)
    if(importee)
        target_compile_features(${cible} INTERFACE cxx_std_${standard})
        target_compile_definitions(${cible} INTERFACE MINI_OPENMETHOD_REFLEXION=${active})
        target_compile_options(${cible} INTERFACE ${options})
    else()
        target_compile_features(${cible} INTERFACE $<BUILD_INTERFACE:cxx_std_${standard}>)
        target_compile_definitions(${cible} INTERFACE $<BUILD_INTERFACE:MINI_OPENMETHOD_REFLEXION=${active}>)
        target_compile_options(${cible} INTERFACE "$<BUILD_INTERFACE:${options}>")
    endif()
    set(MINI_OPENMETHOD_REFLEXION_ACTIVE ${active} PARENT_SCOPE)
    set(MINI_OPENMETHOD_STANDARD ${standard} PARENT_SCOPE)
    set(MINI_OPENMETHOD_OPTIONS_REFLEXION "${options}" PARENT_SCOPE)
    message(STATUS "mini_openmethod : C++${standard}, reflexion=${active} (selection ${mode})")
endfunction()
