// sbox.hpp — Representación de una S-box 8x8 y utilidades básicas.
//
// Una S-box es UN individuo del algoritmo: una permutación de 0..255.
// Todo lo de aquí ya está implementado (es "plomería", no el núcleo del paper).
#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <string>

namespace sbox {

using SBox = std::array<std::uint8_t, 256>;
using Rng  = std::mt19937_64;

// S-box identidad: S[x] = x. Es totalmente lineal → NL = 0. Útil para pruebas.
SBox identity();

// S-box aleatoria biyectiva con Fisher-Yates (sec. 4.1 del paper).
SBox random_sbox(Rng& rng);

// ¿Cada valor 0..255 aparece exactamente una vez?
bool is_bijective(const SBox& s);

// Serialización a 512 caracteres hex (para guardar en CSV y verificar en Python).
std::string to_hex(const SBox& s);
SBox from_hex(const std::string& hex);

// S-box de AES (FIPS-197). Su no linealidad es 112: es tu prueba de oro.
extern const SBox AES;

}  // namespace sbox
