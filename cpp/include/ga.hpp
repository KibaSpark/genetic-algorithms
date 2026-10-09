// ga.hpp — El algoritmo "genético modificado" del paper (sec. 4.1).
//
// En rigor es una estrategia evolutiva (mu + lambda): mu = k_pop padres,
// lambda = k_pop * k_mut hijos por iteración, cada hijo = UN swap de su padre.
#pragma once

#include <vector>

#include "sbox.hpp"
#include "walsh.hpp"

namespace sbox {

struct GAParams {
    int       k_pop     = 1;        // padres que sobreviven por iteración (K_pop)
    int       k_mut     = 7;        // hijos por padre por iteración (K_mut)
    long      k_iter    = 150000;   // máximo de iteraciones (K_iter)
    int       target_nl = 104;      // criterio de paro
    double    R         = 12.0;     // exponente de WHS
    double    X         = 0.0;      // desplazamiento de WHS
};

struct Individual {
    SBox    s{};
    Metrics m{};
};

struct RunResult {
    bool       success    = false;  // ¿llegó a target_nl? (false = corrida censurada)
    long long  k_sbox     = 0;      // S-boxes evaluadas (métrica principal del paper)
    long       iterations = 0;      // iteraciones completas o parciales usadas
    Individual best{};              // la mejor S-box encontrada
};

// Orden de la selección elitista (supuesto documentado):
// primero mayor NL; si empatan, menor WHS.
bool better(const Metrics& a, const Metrics& b);

// Deja en `pop` solo los k_pop mejores según better(). Ya implementada.
void elite_selection(std::vector<Individual>& pop, int k_pop);

// TODO(4): intercambia S[i] y S[j] con i != j elegidos al azar (sec. 4.2).
void swap_mutation(SBox& s, Rng& rng);

// TODO(5): el ciclo principal (pseudocódigo de la sec. 4.1).
RunResult run_ga(const GAParams& p, Rng& rng);

}  // namespace sbox
