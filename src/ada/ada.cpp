#include "ada.hpp"
#include "../util/timing.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

namespace hypertension::ada {

namespace {

std::filesystem::path find_binary() {
    if (const char* env = std::getenv("HYPERTENSION_ADA_BIN"))
        return env;

    if (std::filesystem::exists("build/hypertension-integrity"))
        return "build/hypertension-integrity";

    // Relative to the executable (same directory as the main binary).
    char buf[4096]{};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        auto sibling = std::filesystem::path(buf)
                           .parent_path()
                       / "hypertension-integrity";
        if (std::filesystem::exists(sibling))
            return sibling;
    }

    return {};
}

struct SubprocResult {
    std::string output;
    int exit_code = -1;
};

SubprocResult invoke_integrity(const std::string& binary_path,
                                const std::string& stdin_data) {
    int in_pipe[2], out_pipe[2];
    if (pipe(in_pipe) || pipe(out_pipe))
        return {"", -1};

    pid_t pid = fork();
    if (pid < 0) {
        close(in_pipe[0]);  close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        return {"", -1};
    }

    if (pid == 0) {
        dup2(in_pipe[0],   STDIN_FILENO);
        dup2(out_pipe[1],  STDOUT_FILENO);
        close(in_pipe[0]);  close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execlp(binary_path.c_str(), binary_path.c_str(), nullptr);
        _exit(127);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);

    // Write stdin.
    const char* ptr = stdin_data.data();
    std::size_t left = stdin_data.size();
    while (left > 0) {
        ssize_t w = write(in_pipe[1], ptr, left);
        if (w <= 0) break;
        ptr += w;
        left -= static_cast<std::size_t>(w);
    }
    close(in_pipe[1]);

    // Read stdout.
    std::string output;
    char chunk[256];
    ssize_t n;
    while ((n = read(out_pipe[0], chunk, sizeof(chunk))) > 0)
        output.append(chunk, static_cast<std::size_t>(n));
    close(out_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    return {output, WIFEXITED(status) ? WEXITSTATUS(status) : -1};
}

} // namespace

bool runtime_available() {
    auto bin = find_binary();
    return !bin.empty() && std::filesystem::exists(bin);
}

auto parse_integrity_output(std::string_view raw, std::string& error)
    -> std::optional<IntegrityResult>
{
    auto start = raw.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        error = "empty Ada integrity output";
        return std::nullopt;
    }
    auto end = raw.find_last_not_of(" \t\r\n");
    std::string line(raw.substr(start, end - start + 1));

    long long total = 0, valid = 0, corrupted = 0;
    char verdict[16] = {};
    if (sscanf(line.c_str(), "%lld %lld %lld %15s",
               &total, &valid, &corrupted, verdict) != 4) {
        error = "malformed Ada integrity output: " + line;
        return std::nullopt;
    }

    std::string_view v(verdict);
    if (v != "PASS" && v != "FAIL") {
        error = "unexpected verdict in Ada integrity output: " + std::string(v);
        return std::nullopt;
    }

    return IntegrityResult{total, valid, corrupted, v == "PASS", 0LL};
}

auto exhaustive_integrity(const IntegrityEvidence& ev, std::string& error)
    -> std::optional<IntegrityResult>
{
    auto bin = find_binary();
    if (bin.empty() || !std::filesystem::exists(bin)) {
        error = "Ada integrity binary not found (run: make ada)";
        return std::nullopt;
    }

    std::string stdin_data =
        std::to_string(ev.target)                       + " " +
        std::to_string(ev.dataset_size)                 + " " +
        std::to_string(ev.present     ? 1 : 0)          + " " +
        std::to_string(ev.index)                        + " " +
        std::to_string(ev.membership  ? 1 : 0)          + " " +
        std::to_string(ev.agreement   ? 1 : 0)          + " " +
        std::to_string(ev.confidence_bp)                 + " " +
        std::to_string(ev.admissible  ? 1 : 0)          + " " +
        ev.seal                                          + " " +
        std::to_string(ev.test_mode   ? 1 : 0)          + "\n";

    const bool is_tty = isatty(STDOUT_FILENO);
    std::atomic<bool> done{false};

    std::thread spinner_thread([&done, is_tty]() {
        if (!is_tty) return;
        static const char* frames[] = {
            "\xe2\xa0\x8b", "\xe2\xa0\x99", "\xe2\xa0\xb9", "\xe2\xa0\xb8",
            "\xe2\xa0\xbc", "\xe2\xa0\xb4", "\xe2\xa0\xa6", "\xe2\xa0\xa7",
            "\xe2\xa0\x87", "\xe2\xa0\x8f"
        };
        int f = 0;
        while (!done.load(std::memory_order_relaxed)) {
            std::cout << "\r" << frames[f % 10]
                      << " Exhaustive integrity              Ada  \r"
                      << std::flush;
            f = (f + 1) % 10;
            usleep(100'000);
        }
    });

    Timer timer;
    auto sub = invoke_integrity(bin.string(), stdin_data);
    long long ns = timer.elapsed_ns();

    done.store(true, std::memory_order_relaxed);
    spinner_thread.join();

    if (is_tty)
        std::cout << "\r" << std::string(45, ' ') << "\r" << std::flush;

    if (sub.exit_code != 0) {
        error = "Ada integrity binary exited " + std::to_string(sub.exit_code) +
                " with output: " + sub.output;
        return std::nullopt;
    }

    if (sub.output.empty()) {
        error = "Ada integrity binary produced no output";
        return std::nullopt;
    }

    auto result = parse_integrity_output(sub.output, error);
    if (!result) return std::nullopt;

    result->elapsed_ns = ns;
    return result;
}

} // namespace hypertension::ada
