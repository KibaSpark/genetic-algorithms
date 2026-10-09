// main.cpp — Corre el barrido de parámetros (K_pop x K_mut x corridas) y guarda CSVs.
//
// Ya implementado: lectura de argumentos, semillas reproducibles, hilos, reanudar
// corridas interrumpidas y escritura de resultados. No necesitas tocarlo para empezar.
//
// Ejemplos:
//   ./sbox_ga --bench 2000                                  # cuántas evaluaciones/s da tu máquina
//   ./sbox_ga --kpop 1 --kmut 7 --runs 5 --tag prueba       # unas cuantas corridas
//   ./sbox_ga --kpop 1:21:2 --kmut 1:31:3 --runs 100 --tag paper   # grid completo del paper
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "ga.hpp"
#include "sbox.hpp"
#include "walsh.hpp"

namespace fs = std::filesystem;
using namespace sbox;

namespace {

struct Config {
    std::vector<int> kpops{1};
    std::vector<int> kmuts{7};
    int         runs      = 1;
    long        k_iter    = 150000;
    int         target_nl = 104;
    double      R         = 12.0;
    double      X         = 0.0;
    unsigned    threads   = std::max(1u, std::thread::hardware_concurrency());
    std::uint64_t seed    = 20240806;  // fecha de CQPC 2024; cámbiala si quieres
    std::string out_dir   = "results";
    std::string tag       = "run";
    long        bench     = 0;         // > 0: solo mide velocidad de evaluate()
};

// "1,3,5" o "1:21:2" (inicio:fin:paso, fin incluido).
std::vector<int> parse_list(const std::string& s) {
    std::vector<int> out;
    if (s.find(':') != std::string::npos) {
        int a = 0, b = 0, step = 1;
        char c1 = 0, c2 = 0;
        std::istringstream in(s);
        in >> a >> c1 >> b;
        if (in >> c2) in >> step;
        if (step <= 0) throw std::invalid_argument("paso debe ser > 0 en " + s);
        for (int v = a; v <= b; v += step) out.push_back(v);
    } else {
        std::istringstream in(s);
        std::string tok;
        while (std::getline(in, tok, ',')) if (!tok.empty()) out.push_back(std::stoi(tok));
    }
    if (out.empty()) throw std::invalid_argument("lista vacía: " + s);
    return out;
}

void usage() {
    std::cout <<
        "Uso: sbox_ga [opciones]\n"
        "  --kpop LISTA     valores de K_pop, p. ej. 1,3,5 o 1:21:2   (default 1)\n"
        "  --kmut LISTA     valores de K_mut, p. ej. 7 o 1:31:3        (default 7)\n"
        "  --runs N         corridas por combinación                   (default 1)\n"
        "  --kiter N        máximo de iteraciones por corrida          (default 150000)\n"
        "  --target NL      no linealidad objetivo                     (default 104)\n"
        "  --R r --X x      parámetros de WHS                          (default 12, 0)\n"
        "  --threads N      hilos en paralelo (uno por corrida)        (default: núcleos)\n"
        "  --seed S         semilla base                               (default 20240806)\n"
        "  --out DIR        carpeta de salida                          (default results)\n"
        "  --tag NOMBRE     prefijo de los archivos de salida          (default run)\n"
        "  --bench N        solo mide N evaluaciones y sale\n";
}

Config parse_args(int argc, char** argv) {
    Config c;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument("falta valor para " + k);
            return argv[++i];
        };
        if (k == "--kpop") c.kpops = parse_list(next());
        else if (k == "--kmut") c.kmuts = parse_list(next());
        else if (k == "--runs") c.runs = std::stoi(next());
        else if (k == "--kiter") c.k_iter = std::stol(next());
        else if (k == "--target") c.target_nl = std::stoi(next());
        else if (k == "--R") c.R = std::stod(next());
        else if (k == "--X") c.X = std::stod(next());
        else if (k == "--threads") c.threads = static_cast<unsigned>(std::max(1, std::stoi(next())));
        else if (k == "--seed") c.seed = std::stoull(next());
        else if (k == "--out") c.out_dir = next();
        else if (k == "--tag") c.tag = next();
        else if (k == "--bench") c.bench = std::stol(next());
        else if (k == "-h" || k == "--help") { usage(); std::exit(0); }
        else throw std::invalid_argument("opción desconocida: " + k);
    }
    return c;
}

// SplitMix64: mezcla la semilla base con (k_pop, k_mut, corrida) para que cada
// corrida tenga su propia semilla, reproducible sin importar el orden de los hilos.
std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

std::uint64_t run_seed(std::uint64_t base, int kpop, int kmut, int run) {
    std::uint64_t s = splitmix64(base);
    s = splitmix64(s ^ static_cast<std::uint64_t>(kpop));
    s = splitmix64(s ^ static_cast<std::uint64_t>(kmut));
    return splitmix64(s ^ static_cast<std::uint64_t>(run));
}

using Key = std::tuple<int, int, int>;

// Lee un CSV de corridas previas para no repetirlas (reanudar tras cerrar la laptop).
std::set<Key> completed_runs(const fs::path& path) {
    std::set<Key> done;
    std::ifstream in(path);
    if (!in) return done;
    std::string line;
    std::getline(in, line);  // encabezado
    while (std::getline(in, line)) {
        std::istringstream row(line);
        std::string tag, kp, km, run;
        if (std::getline(row, tag, ',') && std::getline(row, kp, ',') && std::getline(row, km, ',') &&
            std::getline(row, run, ','))
            done.emplace(std::stoi(kp), std::stoi(km), std::stoi(run));
    }
    return done;
}

int do_bench(const Config& c) {
    Rng rng(c.seed);
    std::vector<SBox> boxes;
    boxes.reserve(256);
    for (int i = 0; i < 256; ++i) boxes.push_back(random_sbox(rng));
    double sink = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (long i = 0; i < c.bench; ++i) sink += evaluate(boxes[i % boxes.size()], c.R, c.X).whs;
    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    double per_sec = c.bench / secs;
    std::cout << std::fixed << std::setprecision(1)
              << "evaluaciones: " << c.bench << " en " << secs << " s  ->  " << per_sec << " eval/s por hilo\n"
              << "estimado para una corrida de ~50,000 evaluaciones: " << 50000.0 / per_sec << " s\n"
              << "(ignorar) " << (sink > 0 ? "" : " ") << "\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    Config c;
    try {
        c = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n\n";
        usage();
        return 2;
    }

    try {
        if (c.bench > 0) return do_bench(c);

        fs::create_directories(c.out_dir);
        const fs::path runs_path  = fs::path(c.out_dir) / (c.tag + "_runs.csv");
        const fs::path sbox_path  = fs::path(c.out_dir) / (c.tag + "_sboxes.csv");
        const fs::path meta_path  = fs::path(c.out_dir) / (c.tag + "_config.txt");

        const auto done = completed_runs(runs_path);
        std::vector<Key> jobs;
        for (int kp : c.kpops)
            for (int km : c.kmuts)
                for (int r = 0; r < c.runs; ++r)
                    if (!done.count({kp, km, r})) jobs.emplace_back(kp, km, r);

        // Encabezados solo si los archivos son nuevos.
        const bool new_runs = !fs::exists(runs_path);
        const bool new_sbox = !fs::exists(sbox_path);
        std::ofstream runs_csv(runs_path, std::ios::app);
        std::ofstream sbox_csv(sbox_path, std::ios::app);
        if (new_runs)
            runs_csv << "tag,k_pop,k_mut,run,seed,success,k_sbox,iterations,nl,whs,seconds\n";
        if (new_sbox) sbox_csv << "tag,k_pop,k_mut,run,nl,whs,sbox_hex\n";

        {
            std::ofstream meta(meta_path);
            meta << "seed=" << c.seed << "\nk_iter=" << c.k_iter << "\ntarget_nl=" << c.target_nl
                 << "\nR=" << c.R << "\nX=" << c.X << "\nruns=" << c.runs << "\nthreads=" << c.threads
                 << "\nkpops=";
            for (int v : c.kpops) meta << v << ' ';
            meta << "\nkmuts=";
            for (int v : c.kmuts) meta << v << ' ';
            meta << '\n';
        }

        std::cerr << "Corridas pendientes: " << jobs.size() << " (ya hechas: " << done.size() << "), hilos: "
                  << c.threads << "\n";

        std::atomic<std::size_t> next{0};
        std::atomic<std::size_t> finished{0};
        std::atomic<bool> failed{false};
        std::mutex io_mutex;
        std::string first_error;

        auto worker = [&]() {
            while (!failed) {
                const std::size_t idx = next.fetch_add(1);
                if (idx >= jobs.size()) return;
                const auto [kp, km, r] = jobs[idx];
                const std::uint64_t seed = run_seed(c.seed, kp, km, r);
                try {
                    Rng rng(seed);
                    GAParams p;
                    p.k_pop = kp;
                    p.k_mut = km;
                    p.k_iter = c.k_iter;
                    p.target_nl = c.target_nl;
                    p.R = c.R;
                    p.X = c.X;
                    const auto t0 = std::chrono::steady_clock::now();
                    const RunResult res = run_ga(p, rng);
                    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

                    std::lock_guard<std::mutex> lock(io_mutex);
                    runs_csv << c.tag << ',' << kp << ',' << km << ',' << r << ',' << seed << ','
                             << (res.success ? 1 : 0) << ',' << res.k_sbox << ',' << res.iterations << ','
                             << res.best.m.nl << ',' << std::setprecision(17) << res.best.m.whs << ','
                             << std::setprecision(6) << secs << '\n';
                    runs_csv.flush();  // si se corta la luz, no pierdes lo ya corrido
                    sbox_csv << c.tag << ',' << kp << ',' << km << ',' << r << ',' << res.best.m.nl << ','
                             << std::setprecision(17) << res.best.m.whs << ',' << to_hex(res.best.s) << '\n';
                    sbox_csv.flush();
                    const std::size_t n = ++finished;
                    std::cerr << "[" << n << "/" << jobs.size() << "] k_pop=" << kp << " k_mut=" << km
                              << " run=" << r << " -> " << (res.success ? "OK" : "CENSURADA")
                              << " k_sbox=" << res.k_sbox << " (" << std::setprecision(3) << secs << " s)\n";
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(io_mutex);
                    if (!failed.exchange(true)) first_error = e.what();
                    return;
                }
            }
        };

        std::vector<std::thread> pool;
        const unsigned n_threads = std::min<unsigned>(c.threads, std::max<std::size_t>(1, jobs.size()));
        for (unsigned t = 0; t < n_threads; ++t) pool.emplace_back(worker);
        for (auto& th : pool) th.join();

        if (failed) {
            std::cerr << "\nSe detuvo: " << first_error << "\n";
            return 1;
        }
        std::cerr << "Listo. Resultados en " << runs_path << " y " << sbox_path << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
