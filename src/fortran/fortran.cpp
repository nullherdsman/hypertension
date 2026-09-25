#include "fortran.hpp"
#include "../util/timing.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

namespace hypertension::fortran {

namespace {

std::filesystem::path find_binary() {
    if (const char* env = std::getenv("HYPERTENSION_FORTRAN_BIN"))
        return env;

    // Relative to CWD (project root).
    if (std::filesystem::exists("build/hypertension-confidence"))
        return "build/hypertension-confidence";

    // Same directory as the running executable.
    char buf[4096]{};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        auto sibling = std::filesystem::path(buf).parent_path() / "hypertension-confidence";
        if (std::filesystem::exists(sibling))
            return sibling;
    }

    return {};
}

struct SubprocResult {
    std::string output;
    int exit_code = -1;
};

SubprocResult invoke_binary(const std::string& path, const std::string& stdin_data) {
    int in_pipe[2], out_pipe[2];
    if (pipe(in_pipe) || pipe(out_pipe))
        return {"", -1};

    pid_t pid = fork();
    if (pid < 0) {
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        return {"", -1};
    }

    if (pid == 0) {
        dup2(in_pipe[0],  STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) dup2(devnull, STDERR_FILENO);
        execl(path.c_str(), path.c_str(), nullptr);
        _exit(127);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);

    write(in_pipe[1], stdin_data.data(), stdin_data.size());
    close(in_pipe[1]);

    std::string output;
    char chunk[256];
    ssize_t n;
    while ((n = read(out_pipe[0], chunk, sizeof(chunk))) > 0)
        output.append(chunk, static_cast<std::size_t>(n));
    close(out_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    return {output, code};
}

} // namespace

bool fortran_available() {
    return !find_binary().empty();
}

auto parse_confidence_output(std::string_view raw, std::string& error) -> std::optional<double> {
    auto start = raw.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        error = "empty FORTRAN output";
        return std::nullopt;
    }
    auto end = raw.find_last_not_of(" \t\r\n");
    std::string trimmed(raw.substr(start, end - start + 1));

    // Reject NaN / Inf before attempting parse.
    std::string lower = trimmed;
    for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lower.find("nan") != std::string::npos || lower.find("inf") != std::string::npos) {
        error = "invalid FORTRAN output: " + trimmed;
        return std::nullopt;
    }

    double val = 0.0;
    try {
        std::size_t pos = 0;
        val = std::stod(trimmed, &pos);
        if (pos != trimmed.size()) {
            error = "malformed FORTRAN output: " + trimmed;
            return std::nullopt;
        }
    } catch (...) {
        error = "malformed FORTRAN output: " + trimmed;
        return std::nullopt;
    }

    if (std::isnan(val) || std::isinf(val)) {
        error = "invalid FORTRAN output: " + trimmed;
        return std::nullopt;
    }
    if (val < 0.0 || val > 100.0) {
        error = "confidence out of range: " + trimmed;
        return std::nullopt;
    }

    return val;
}

auto statistical_confidence(const ConfidenceEvidence& ev, std::string& error)
    -> std::optional<ConfidenceResult>
{
    auto binary = find_binary();
    if (binary.empty()) {
        error = "build/hypertension-confidence not found (run: make fortran)";
        return std::nullopt;
    }

    // Protocol: "N CFND BFPOS AGREE CIDX AIDX\n"
    std::string input =
        std::to_string(ev.n)                       + " " +
        std::to_string(ev.cpp_found   ? 1 : 0)    + " " +
        std::to_string(ev.bf_positive ? 1 : 0)    + " " +
        std::to_string(ev.agree       ? 1 : 0)    + " " +
        std::to_string(ev.cpp_index)               + " " +
        std::to_string(ev.algol_index)             + "\n";

    Timer timer;
    auto sub = invoke_binary(binary.string(), input);
    long long ns = timer.elapsed_ns();

    if (sub.exit_code != 0) {
        error = "confidence binary exited " + std::to_string(sub.exit_code);
        return std::nullopt;
    }

    auto conf = parse_confidence_output(sub.output, error);
    if (!conf) return std::nullopt;

    return ConfidenceResult{*conf, ns};
}

} // namespace hypertension::fortran
