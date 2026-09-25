#include "forth.hpp"
#include "../util/timing.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

namespace hypertension::forth {

namespace {

std::filesystem::path find_source() {
    if (const char* env = std::getenv("HYPERTENSION_FORTH_SRC"))
        return env;

    if (std::filesystem::exists("engine/finalize.fs"))
        return "engine/finalize.fs";

    // Relative to the executable (/proc/self/exe → sibling of build/).
    char buf[4096]{};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        auto sibling = std::filesystem::path(buf)
                           .parent_path().parent_path()
                       / "engine/finalize.fs";
        if (std::filesystem::exists(sibling))
            return sibling;
    }

    return {};
}

struct SubprocResult {
    std::string output;
    int exit_code = -1;
};

SubprocResult invoke_gforth(const std::string& source_path, const std::string& goal) {
    int out_pipe[2];
    if (pipe(out_pipe))
        return {"", -1};

    pid_t pid = fork();
    if (pid < 0) {
        close(out_pipe[0]); close(out_pipe[1]);
        return {"", -1};
    }

    if (pid == 0) {
        dup2(out_pipe[1], STDOUT_FILENO);
        close(out_pipe[0]); close(out_pipe[1]);
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        // Ask Forth.
        execlp("gforth", "gforth",
               source_path.c_str(),
               "-e", goal.c_str(),
               nullptr);
        _exit(127);
    }

    close(out_pipe[1]);

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

bool runtime_available() {
    FILE* f = popen("command -v gforth >/dev/null 2>&1 && echo y", "r");
    if (!f) return false;
    char c = '\0';
    bool found = fread(&c, 1, 1, f) > 0 && c == 'y';
    pclose(f);
    return found;
}

auto parse_seal_output(std::string_view raw, std::string& error) -> std::optional<std::string> {
    // Trim whitespace.
    auto start = raw.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        error = "empty Forth output";
        return std::nullopt;
    }
    auto end = raw.find_last_not_of(" \t\r\n");
    std::string trimmed(raw.substr(start, end - start + 1));

    // Must be exactly 8 hex characters.
    if (trimmed.size() != 8) {
        error = "unexpected Forth output: " + trimmed;
        return std::nullopt;
    }

    for (char c : trimmed) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            error = "unexpected Forth output: " + trimmed;
            return std::nullopt;
        }
    }

    // Store the seal.
    for (auto& c : trimmed)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    return trimmed;
}

auto record_finalization(const SealEvidence& ev, std::string& error)
    -> std::optional<SealResult>
{
    if (!runtime_available()) {
        error = "gforth not found in PATH";
        return std::nullopt;
    }

    auto source = find_source();
    if (source.empty()) {
        error = "engine/finalize.fs not found";
        return std::nullopt;
    }

    // Build the invocation goal: push the eight canonical fields onto the
    // stack, then call FINALIZE. Field order matches finalize.fs documentation.
    std::string goal =
        std::to_string(ev.target)                          + " " +
        std::to_string(ev.dataset_size)                    + " " +
        std::to_string(ev.present     ? 1 : 0)            + " " +
        std::to_string(ev.index)                           + " " +
        std::to_string(ev.membership  ? 1 : 0)            + " " +
        std::to_string(ev.agreement   ? 1 : 0)            + " " +
        std::to_string(ev.confidence_bp)                   + " " +
        std::to_string(ev.admissible  ? 1 : 0)            + " " +
        "FINALIZE";

    Timer timer;
    auto sub = invoke_gforth(source.string(), goal);
    long long ns = timer.elapsed_ns();

    if (sub.exit_code != 0) {
        error = "gforth exited " + std::to_string(sub.exit_code) + " with output: " + sub.output;
        return std::nullopt;
    }

    // Check if empty.
    if (sub.output.empty()) {
        error = "gforth produced no output";
        return std::nullopt;
    }

    auto seal = parse_seal_output(sub.output, error);
    if (!seal.has_value()) return std::nullopt;

    return SealResult{*seal, ns};
}

} // namespace hypertension::forth
