#pragma once
#include <mini_openmethod/methode.hpp>
struct Animal { virtual ~Animal() = default; };
struct Chien : Animal {};
struct Chat : Animal {};
using namespace mini_openmethod;
