// test_core.cpp — Pruebas del núcleo. Córrelas cada vez que cambies walsh.cpp o ga.cpp.
//
//   ./test_core
//
// Van en el orden en que conviene implementar: primero Walsh/NL/WHS, luego mutación,
// al final el ciclo del algoritmo. Mientras haya TODOs, verás FAIL con el mensaje del TODO.
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "ga.hpp"
#include "sbox.hpp"
#include "walsh.hpp"

using namespace sbox;

namespace {

int passed = 0, failed = 0;

void check(const std::string& name, const std::function<bool(std::string&)>& fn) {
    std::string detail;
    bool ok = false;
    try {
        ok = fn(detail);
    } catch (const std::exception& e) {
        detail = e.what();
    }
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << name;
    if (!ok && !detail.empty()) std::cout << "  ->  " << detail;
    std::cout << '\n';
    (ok ? passed : failed)++;
}

}  // namespace

int main() {
    std::cout << "== Plomería (ya implementada) ==\n";

    check("la S-box de AES es biyectiva", [](std::string&) { return is_bijective(AES); });

    check("Fisher-Yates genera permutaciones válidas (1000 casos)", [](std::string& d) {
        Rng rng(1);
        for (int i = 0; i < 1000; ++i)
            if (!is_bijective(random_sbox(rng))) { d = "falló en la iteración " + std::to_string(i); return false; }
        return true;
    });

    check("to_hex / from_hex ida y vuelta", [](std::string&) { return from_hex(to_hex(AES)) == AES; });

    std::cout << "\n== TODO(1-3): Walsh, NL y WHS ==\n";

    check("identidad: W[b][b] = 256 para todo b", [](std::string& d) {
        WalshTable W{};
        walsh_spectrum(identity(), W);
        for (int b = 1; b < 256; ++b)
            if (W[b][b] != 256) { d = "W[" + std::to_string(b) + "][" + std::to_string(b) + "] = " + std::to_string(W[b][b]); return false; }
        return true;
    });

    check("todos los coeficientes son pares y |W| <= 256 (S-box aleatoria)", [](std::string& d) {
        Rng rng(7);
        WalshTable W{};
        walsh_spectrum(random_sbox(rng), W);
        for (int b = 1; b < 256; ++b)
            for (int a = 0; a < 256; ++a)
                if (W[b][a] % 2 != 0 || std::abs(W[b][a]) > 256) { d = "coeficiente raro en b=" + std::to_string(b) + ", a=" + std::to_string(a); return false; }
        return true;
    });

    check("Parseval: sum_a W[b][a]^2 = 65536 para cada b", [](std::string& d) {
        Rng rng(11);
        WalshTable W{};
        walsh_spectrum(random_sbox(rng), W);
        for (int b = 1; b < 256; ++b) {
            long long sum = 0;
            for (int a = 0; a < 256; ++a) sum += 1LL * W[b][a] * W[b][a];
            if (sum != 65536) { d = "b=" + std::to_string(b) + " suma " + std::to_string(sum); return false; }
        }
        return true;
    });

    check("NL(identidad) = 0", [](std::string& d) {
        int nl = evaluate(identity(), 12, 0).nl;
        d = "obtuve " + std::to_string(nl);
        return nl == 0;
    });

    check("NL(AES) = 112   <- la prueba de oro", [](std::string& d) {
        int nl = evaluate(AES, 12, 0).nl;
        d = "obtuve " + std::to_string(nl);
        return nl == 112;
    });

    check("NL de S-boxes aleatorias entre 80 y 100 (típico ~94)", [](std::string& d) {
        Rng rng(3);
        for (int i = 0; i < 20; ++i) {
            int nl = evaluate(random_sbox(rng), 12, 0).nl;
            if (nl < 80 || nl > 100) { d = "salió " + std::to_string(nl); return false; }
        }
        return true;
    });

    check("WHS finito, positivo y menor en AES que en una aleatoria", [](std::string& d) {
        Rng rng(5);
        double w_aes = evaluate(AES, 12, 0).whs;
        double w_rnd = evaluate(random_sbox(rng), 12, 0).whs;
        d = "AES=" + std::to_string(w_aes) + " aleatoria=" + std::to_string(w_rnd);
        return std::isfinite(w_aes) && std::isfinite(w_rnd) && w_aes > 0 && w_aes < w_rnd;
    });

    check("WHS(AES) con R=12, X=0 coincide con el valor exacto", [](std::string& d) {
        // Valor exacto: suma de |W|^12 sobre b = 1..255, a = 0..255, calculada aparte con
        // enteros de precisión arbitraria en Python. Exacto = 2780281712723519078400.
        const double expected = 2.780281712723519e+21;
        double got = evaluate(AES, 12, 0).whs;
        d = "esperado " + std::to_string(expected) + ", obtuve " + std::to_string(got);
        return std::abs(got - expected) / expected < 1e-12;
    });

    std::cout << "\n== TODO(4): mutación ==\n";

    check("swap_mutation cambia exactamente 2 posiciones y sigue siendo biyectiva", [](std::string& d) {
        Rng rng(9);
        for (int t = 0; t < 2000; ++t) {
            SBox s = random_sbox(rng), m = s;
            swap_mutation(m, rng);
            int diff = 0;
            for (int x = 0; x < 256; ++x) diff += (s[x] != m[x]);
            if (diff != 2 || !is_bijective(m)) { d = "cambió " + std::to_string(diff) + " posiciones"; return false; }
        }
        return true;
    });

    std::cout << "\n== TODO(5): el algoritmo ==\n";

    check("run_ga llega a NL >= 100 rápido (k_pop=1, k_mut=7)", [](std::string& d) {
        Rng rng(42);
        GAParams p;
        p.k_pop = 1; p.k_mut = 7; p.k_iter = 5000; p.target_nl = 100;
        RunResult r = run_ga(p, rng);
        d = "success=" + std::to_string(r.success) + " nl=" + std::to_string(r.best.m.nl) + " k_sbox=" + std::to_string(r.k_sbox);
        return r.success && r.best.m.nl >= 100 && is_bijective(r.best.s) && r.k_sbox > 0;
    });

    check("corrida censurada: con k_iter=3 no llega a 104 y lo reporta", [](std::string& d) {
        Rng rng(1);
        GAParams p;
        p.k_pop = 2; p.k_mut = 3; p.k_iter = 3; p.target_nl = 104;
        RunResult r = run_ga(p, rng);
        // A1 + A2: 2 iniciales + 3 iteraciones * 2 padres * 3 hijos = 20 evaluaciones.
        d = "success=" + std::to_string(r.success) + " k_sbox=" + std::to_string(r.k_sbox);
        return !r.success && r.k_sbox == 20 && r.iterations == 3;
    });

    check("misma semilla -> mismo resultado (reproducibilidad)", [](std::string& d) {
        GAParams p;
        p.k_pop = 3; p.k_mut = 4; p.k_iter = 2000; p.target_nl = 100;
        Rng a(123), b(123);
        RunResult ra = run_ga(p, a), rb = run_ga(p, b);
        d = std::to_string(ra.k_sbox) + " vs " + std::to_string(rb.k_sbox);
        return ra.k_sbox == rb.k_sbox && ra.best.s == rb.best.s;
    });

    std::cout << "\n" << passed << " pasaron, " << failed << " fallaron\n";
    return failed == 0 ? 0 : 1;
}
