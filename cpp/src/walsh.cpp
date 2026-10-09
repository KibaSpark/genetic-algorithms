#include "walsh.hpp"

#include <stdexcept>

namespace sbox {

// ============================================================================
// TODO(1) — Espectro de Walsh-Hadamard
// ----------------------------------------------------------------------------
// Pistas (de menos a más):
//  - Para cada b fijo, arma el vector f[x] = (-1)^( paridad(b & S[x]) ), x = 0..255.
//    __builtin_parity(unsigned) te da la paridad en gcc/clang.
//  - La transformada de ese vector, F[a] = sum_x f[x] * (-1)^(paridad(a & x)),
//    es justo la fila W[b][·].
//  - Hacerlo directo cuesta 256*256 por fila. La transformada RÁPIDA (FWHT) cuesta
//    256*8: en cada una de las 8 etapas (h = 1, 2, 4, ..., 128) combinas pares
//    (u, v) -> (u + v, u - v). Búscala como "fast Walsh-Hadamard transform butterfly".
//  - Chequeo mental: con la S-box identidad, W[b][b] = 256 para todo b.
// ============================================================================
void walsh_spectrum(const SBox& s, WalshTable& W) {
    (void)s;
    (void)W;
    throw std::runtime_error("TODO(1): implementa walsh_spectrum en cpp/src/walsh.cpp");
}

// ============================================================================
// TODO(2) — No linealidad
// ----------------------------------------------------------------------------
//  - Busca el máximo de |W[b][a]| con b de 1 a 255 y a de 0 a 255.
//  - Ojo: si incluyes b = 0, W[0][0] = 256 y siempre te da NL = 0.
//  - Prueba de oro: con la S-box de AES te tiene que dar 112.
// ============================================================================
int nonlinearity(const WalshTable& W) {
    (void)W;
    throw std::runtime_error("TODO(2): implementa nonlinearity en cpp/src/walsh.cpp");
}

// ============================================================================
// TODO(3) — Costo WHS
// ----------------------------------------------------------------------------
//  - Acumula en double, NUNCA en entero: 48^12 ya es ~1.5e20 y no cabe en 64 bits.
//  - std::pow funciona, pero es lento con 65,280 llamadas por evaluación. Si R es
//    entero (12), elevar por multiplicaciones repetidas es mucho más rápido.
//    (Primero que sea correcto; luego que sea rápido.)
// ============================================================================
double whs_cost(const WalshTable& W, double R, double X) {
    (void)W;
    (void)R;
    (void)X;
    throw std::runtime_error("TODO(3): implementa whs_cost en cpp/src/walsh.cpp");
}

// Ya implementada. El buffer es thread_local para que cada hilo tenga el suyo
// (256 KB) y no se reserve memoria en cada evaluación.
Metrics evaluate(const SBox& s, double R, double X) {
    thread_local WalshTable W{};
    walsh_spectrum(s, W);
    return Metrics{nonlinearity(W), whs_cost(W, R, X)};
}

}  // namespace sbox
