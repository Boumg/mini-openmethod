#pragma once
/** Hierarchie minimale : les operations sont definies a l'exterieur. */
struct Animal { virtual ~Animal() = default; };
struct Mammifere : Animal {};
struct Chien : Mammifere {};
struct Chat : Mammifere {};
struct Oiseau : Animal {};
