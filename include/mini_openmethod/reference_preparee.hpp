#pragma once
#include <cstddef>
#include <memory>

/** @file Reference non proprietaire dont le type dynamique a deja ete valide. */
namespace mini_openmethod {
template<class Signature, class Domaines, class... Fonctions> class methode;

/**
 * Conserve un objet et son indice dans une liste ordonnee de types.
 * Seule methode::preparer peut construire cette reference ; elle est copiable.
 * L'objet doit rester vivant a la meme adresse et conserver son type dynamique.
 * La destruction, le remplacement ou le deplacement physique de l'objet impose
 * une nouvelle preparation. Aucun lien n'est conserve vers l'objet methode.
 */
template<class Base, class Liste>
class reference_preparee {
    template<class Signature, class Domaines, class... Fonctions> friend class methode;
    const Base* objet_;
    std::size_t indice_;

    reference_preparee(const Base& objet, std::size_t indice) noexcept
        : objet_(std::addressof(objet)), indice_(indice) {}
public:
    /** Acces a l'objet d'origine ; sa duree de vie reste a la charge de l'appelant. */
    [[nodiscard]] const Base& objet() const noexcept { return *objet_; }
};
} // namespace mini_openmethod
