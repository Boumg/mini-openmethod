#pragma once
#include <mini_openmethod/detail/traits.hpp>
#include <mini_openmethod/detail/selection.hpp>
#include <meta>
#include <vector>

/** @file Construction C++26 des tables par parcours des types reflechis. */
namespace mini_openmethod::detail {
/** Equivalent de derived_from, y compris pour les bases privees et ambigues. */
consteval bool derive_publiquement(std::meta::info derive, std::meta::info base) {
    return std::meta::is_base_of_type(base, derive)
        && std::meta::is_convertible_type(std::meta::add_pointer(derive), std::meta::add_pointer(base));
}

template<class Domaines, class Fonctions> struct resolution_cpp26;

/** Produit les indices gagnants ; les objets meta restent limites a la compilation. */
template<class... Listes, class... Fonctions>
struct resolution_cpp26<domaines<Listes...>, std::tuple<Fonctions...>> {
private:
    static consteval auto construire() {
        constexpr std::size_t arite = sizeof...(Listes);
        constexpr std::size_t nombre_fonctions = sizeof...(Fonctions);
        constexpr std::size_t nombre_cases = (traits_liste<Listes>::taille * ...);
        const std::array<std::vector<std::meta::info>, arite> types_domaines{
            std::meta::template_arguments_of(std::meta::dealias(^^Listes))...};
        const std::array<std::vector<std::meta::info>, nombre_fonctions> types_signatures{
            std::meta::template_arguments_of(std::meta::dealias(^^typename traits_fonction<Fonctions>::arguments))...};

        // Les memes parametres reviennent dans de nombreuses signatures du double dispatch.
        std::vector<std::meta::info> types;
        const auto identifier = [&](std::meta::info type) {
            type = std::meta::dealias(std::meta::remove_cvref(type));
            for (std::size_t indice = 0; indice < types.size(); ++indice)
                if (std::meta::is_same_type(types[indice], type)) return indice;
            types.push_back(type);
            return types.size() - 1;
        };
        std::array<std::vector<std::size_t>, arite> domaines_indices;
        for (std::size_t position = 0; position < arite; ++position)
            for (const auto type : types_domaines[position])
                domaines_indices[position].push_back(identifier(type));
        std::array<std::array<std::size_t, arite>, nombre_fonctions> signatures{};
        for (std::size_t fonction = 0; fonction < nombre_fonctions; ++fonction)
            for (std::size_t position = 0; position < arite; ++position)
                signatures[fonction][position] = identifier(types_signatures[fonction][position]);

        // Une seule requete d'heritage par paire de types distincts, jamais par case.
        const auto nombre_types = types.size();
        std::vector<unsigned char> relations(nombre_types * nombre_types);
        for (std::size_t derive = 0; derive < nombre_types; ++derive)
            for (std::size_t base = 0; base < nombre_types; ++base)
                relations[derive * nombre_types + base] = derive_publiquement(types[derive], types[base]);
        const auto domine = [&](std::size_t gauche, std::size_t droite) {
            bool strictement_plus_precise = false;
            for (std::size_t position = 0; position < arite; ++position) {
                const auto a = signatures[gauche][position];
                const auto b = signatures[droite][position];
                if (!relations[a * nombre_types + b]) return false;
                strictement_plus_precise |= a != b;
            }
            return strictement_plus_precise;
        };

        // La compatibilite d'une position ne depend pas du type choisi aux autres positions.
        std::array<std::vector<unsigned char>, arite> applicabilites;
        for (std::size_t position = 0; position < arite; ++position) {
            applicabilites[position].reserve(domaines_indices[position].size() * nombre_fonctions);
            for (const auto type : domaines_indices[position])
                for (std::size_t fonction = 0; fonction < nombre_fonctions; ++fonction)
                    applicabilites[position].push_back(relations[type * nombre_types + signatures[fonction][position]]);
        }
        std::array<std::size_t, nombre_cases> resultat{};
        for (std::size_t case_table = 0; case_table < nombre_cases; ++case_table) {
            std::array<bool, nombre_fonctions> candidats{};
            candidats.fill(true);
            std::size_t reste = case_table;
            for (std::size_t position = arite; position-- > 0;) {
                const auto coordonnee = reste % domaines_indices[position].size();
                reste /= domaines_indices[position].size();
                for (std::size_t fonction = 0; fonction < nombre_fonctions; ++fonction)
                    if (candidats[fonction])
                        candidats[fonction] = applicabilites[position][coordonnee * nombre_fonctions + fonction];
            }
            resultat[case_table] = choisir_specialisation(candidats, domine);
        }
        return resultat;
    }
public:
    static constexpr auto indices = construire();
};
} // namespace mini_openmethod::detail
