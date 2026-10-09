#include "ga.hpp"

#include <algorithm>
#include <stdexcept>

namespace sbox {

bool better(const Metrics& a, const Metrics& b) {
    if (a.nl != b.nl) return a.nl > b.nl;
    return a.whs < b.whs;
}

void elite_selection(std::vector<Individual>& pop, int k_pop) {
    if (static_cast<int>(pop.size()) <= k_pop) {
        std::sort(pop.begin(), pop.end(), [](const Individual& x, const Individual& y) { return better(x.m, y.m); });
        return;
    }
    std::partial_sort(pop.begin(), pop.begin() + k_pop, pop.end(),
                      [](const Individual& x, const Individual& y) { return better(x.m, y.m); });
    pop.resize(k_pop);
}

// ============================================================================
// TODO(4) — Mutación por intercambio (sec. 4.2)
// ----------------------------------------------------------------------------
//  - Elige i y j uniformes en 0..255 con i != j (el pseudocódigo del paper no lo
//    fuerza, el texto sí: nosotros lo forzamos).
//  - Un truco limpio para garantizar i != j sin ciclos: elige j en 0..254 y,
//    si j >= i, súmale 1.
//  - Pregunta para ti: ¿por qué esta mutación nunca rompe la biyectividad?
// ============================================================================
void swap_mutation(SBox& s, Rng& rng) {
    (void)s;
    (void)rng;
    throw std::runtime_error("TODO(4): implementa swap_mutation en cpp/src/ga.cpp");
}

// ============================================================================
// TODO(5) — Ciclo principal (sec. 4.1)
// ----------------------------------------------------------------------------
// Supuestos de implementación (documentados en el README):
//  A1. La población inicial son k_pop S-boxes aleatorias (random_sbox).
//  A2. TODA llamada a evaluate() suma 1 a k_sbox, incluida la población inicial
//      y la evaluación que encuentra la S-box ganadora.
//  A3. Padres e hijos compiten juntos en elite_selection (esquema mu + lambda).
//  A4. Cada hijo sale de UNA copia del padre con UN swap (S' <- S dentro del ciclo k).
//  A5. Si al terminar k_iter iteraciones no llegó a target_nl, success = false
//      (corrida censurada) y se reporta la mejor que encontró.
//
// Esqueleto sugerido (traduce el pseudocódigo, línea por línea):
//
//   RunResult r;
//   std::vector<Individual> pop;
//   // A1: crea k_pop individuos, evalúalos, cuenta k_sbox, revisa si alguno ya cumple
//   for (long t = 0; t < p.k_iter; ++t) {
//       elite_selection(pop, p.k_pop);            // línea 2
//       // líneas 3-15: para cada padre (los primeros k_pop) y para k en 0..k_mut-1:
//       //   copia -> swap_mutation -> evaluate -> ++k_sbox
//       //   si nl >= target_nl: llena r y regresa (success = true)
//       //   si no: agrégalo a pop
//       // ojo: si haces push_back a pop mientras iteras sobre pop por referencia,
//       //      el vector puede reacomodarse en memoria. Copia al padre primero.
//   }
//   // A5: aquí llegas si no encontró nada; guarda la mejor en r.best
//   return r;
//
// Lleva r.iterations para saber en qué iteración terminó.
// ============================================================================
RunResult run_ga(const GAParams& p, Rng& rng) {
    (void)p;
    (void)rng;
    throw std::runtime_error("TODO(5): implementa run_ga en cpp/src/ga.cpp");
}

}  // namespace sbox
