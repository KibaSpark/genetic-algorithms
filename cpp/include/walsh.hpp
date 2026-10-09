// walsh.hpp — Espectro de Walsh-Hadamard, no linealidad (NL) y función de costo WHS.
//
// ESTE ES EL NÚCLEO MATEMÁTICO. Las tres funciones marcadas con TODO las implementas tú
// en src/walsh.cpp. Las pruebas en tests/test_core.cpp te dicen si quedaron bien.
#pragma once

#include <array>
#include <cstdint>

#include "sbox.hpp"

namespace sbox {

// W[b][a] = sum_{x=0}^{255} (-1)^( b·S(x)  XOR  a·x )
//   b = máscara de SALIDA (qué combinación de bits de S(x) miras); b = 0 no se usa.
//   a = máscara de ENTRADA (con qué función lineal de x la comparas).
// "·" es el producto punto en GF(2): paridad de los bits de (b & S(x)).
// Cada coeficiente es un entero par entre -256 y 256.
using WalshTable = std::array<std::array<std::int32_t, 256>, 256>;

struct Metrics {
    int    nl  = 0;    // no linealidad (más alto = mejor; meta 104, AES = 112)
    double whs = 0.0;  // costo WHS (más bajo = mejor)
};

// TODO(1): llena W[b][a] para b = 1..255 y a = 0..255 (la fila b = 0 puede quedar en ceros).
void walsh_spectrum(const SBox& s, WalshTable& W);

// TODO(2): NL(S) = 128 - (1/2) * max_{b != 0, a} |W[b][a]|   (sec. 3.1 del paper, n = 8)
int nonlinearity(const WalshTable& W);

// TODO(3): WHS = sum_{b=1}^{255} sum_{a=0}^{255} | |W[b][a]| - X |^R   (sec. 4.3, R = 12, X = 0)
double whs_cost(const WalshTable& W, double R, double X);

// Ya implementada: calcula el espectro una vez y saca NL y WHS de ahí.
Metrics evaluate(const SBox& s, double R, double X);

}  // namespace sbox
