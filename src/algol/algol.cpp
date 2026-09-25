#include "algol.hpp"
#include "../util/timing.hpp"

#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

namespace hypertension::algol {

namespace {

std::filesystem::path find_source() {
    if (const char* env = std::getenv("HYPERTENSION_ALGOL_SRC"))
        return env;

    // Relative to CWD (project root when invoked from there).
    if (std::filesystem::exists("engine/consensus.a68"))
        return "engine/consensus.a68";

    // Relative to the executable (/proc/self/exe → sibling of build/).
    char buf[4096]{};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        auto sibling = std::filesystem::path(buf).parent_path().parent_path() / "engine/consensus.a68";
        if (std::filesystem::exists(sibling))
            return sibling;
    }

    return {};
}

struct SubprocResult {
    std::string output;
    int exit_code = -1;
    std::string error;
};

SubprocResult invoke_a68g(const std::string& source, const std::string& stdin_data) {
    int in_pipe[2], out_pipe[2];
    if (pipe(in_pipe) || pipe(out_pipe))
        return {"", -1, "pipe() failed"};

    pid_t pid = fork();
    if (pid < 0) {
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        return {"", -1, "fork() failed"};
    }

    if (pid == 0) {
        dup2(in_pipe[0],  STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) dup2(devnull, STDERR_FILENO);
        execlp("a68g", "a68g", "--quiet", source.c_str(), nullptr);
        _exit(127);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);

    // Write is small (< 100 bytes); won't block.
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

    return {output, code, ""};
}

} // namespace

bool runtime_available() {
    // Check for a68g.
    FILE* f = popen("command -v a68g >/dev/null 2>&1 && echo y", "r");
    if (!f) return false;
    char c = '\0';
    bool found = fread(&c, 1, 1, f) > 0 && c == 'y';
    pclose(f);
    return found;
}

auto parse_algol_output(std::string_view raw, std::string& error) -> std::optional<int> {
    // Trim leading/trailing whitespace.
    auto start = raw.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        error = "empty ALGOL output";
        return std::nullopt;
    }
    auto end = raw.find_last_not_of(" \t\r\n");
    auto trimmed = raw.substr(start, end - start + 1);

    int value = 0;
    auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);
    if (ec != std::errc{} || ptr != trimmed.data() + trimmed.size()) {
        error = "malformed ALGOL output: \"" + std::string(raw) + "\"";
        return std::nullopt;
    }
    return value;
}

bool consensus_agrees(
    std::optional<std::size_t> cpp_result,
    const ConsensusResult& algol_result
) {
    bool cpp_found   = cpp_result.has_value();
    bool algol_found = algol_result.index.has_value();
    if (cpp_found != algol_found) return false;
    if (!cpp_found) return true;
    return static_cast<int>(*cpp_result) == *algol_result.index;
}

auto consensus_search(
    std::span<const int> dataset,
    int target,
    std::string& error
) -> std::optional<ConsensusResult>
{
    if (!runtime_available()) {
        error = "a68g not found in PATH";
        return std::nullopt;
    }

    auto source = find_source();
    if (source.empty()) {
        error = "engine/consensus.a68 not found";
        return std::nullopt;
    }

    // Protocol: "n target v0 v1 ... v_{n-1}\n"
    std::string input = std::to_string(dataset.size()) + " " + std::to_string(target);
    for (int v : dataset)
        input += " " + std::to_string(v);
    input += "\n";

    Timer timer;
    auto sub = invoke_a68g(source.string(), input);
    long long ns = timer.elapsed_ns();

    if (sub.exit_code != 0) {
        error = "a68g exited " + std::to_string(sub.exit_code);
        if (!sub.output.empty()) error += ": " + sub.output;
        return std::nullopt;
    }

    auto idx = parse_algol_output(sub.output, error);
    if (!idx.has_value()) return std::nullopt;

    return ConsensusResult{*idx == -1 ? std::nullopt : std::optional<int>{*idx}, ns};
}

} // namespace hypertension::algol
