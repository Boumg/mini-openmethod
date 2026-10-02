#pragma once
#include <cstddef>
#include <memory>
#include <type_traits>

/** @file Reference non proprietaire dont le type dynamique a deja ete valide. */
namespace mini_openmethod {
template<class Signature, class Domaines, class... Fonctions> class methode;

/**
 * Conserve un objet et son indice dans une liste ordonnee de types.
 * Seule methode::preparer peut construire cette reference ; elle est copiable.
 * L'objet doit rester vivant a la meme adresse et conserver son type dynamique.
 * La destruction, le remplacement ou le deplacement physique de l'objet impose
 * une nouvelle preparation. Aucun lien n'est conserve vers l'objet methode.
 * @tparam Modifiable Autorise la modification de l'objet ; faux preserve l'acces constant.
 */
template<class Base, class Liste, bool Modifiable = false>
class reference_preparee {
    template<class Signature, class Domaines, class... Fonctions> friend class methode;
    template<class, class, bool> friend class reference_preparee;
    using objet_type = std::conditional_t<Modifiable, Base, const Base>;
    objet_type* objet_;
    std::size_t indice_;

    reference_preparee(objet_type& objet, std::size_t indice) noexcept
        : objet_(std::addressof(objet)), indice_(indice) {}
public:
    /** Restreint les droits d'acces sans identifier a nouveau le type dynamique. */
    template<bool Autre>
        requires (!Modifiable && Autre)
    reference_preparee(const reference_preparee<Base, Liste, Autre>& autre) noexcept
        : objet_(autre.objet_), indice_(autre.indice_) {}

    /** Acces a l'objet d'origine ; sa duree de vie reste a la charge de l'appelant. */
    [[nodiscard]] objet_type& objet() const noexcept { return *objet_; }
};
} // namespace mini_openmethod
