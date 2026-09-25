#include "prolog.hpp"
#include "../util/timing.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

namespace hypertension::prolog {

namespace {

std::filesystem::path find_rules() {
    if (const char* env = std::getenv("HYPERTENSION_PROLOG_SRC"))
        return env;

    if (std::filesystem::exists("engine/admissibility.pl"))
        return "engine/admissibility.pl";

    // Relative to the executable (/proc/self/exe → sibling of build/).
    char buf[4096]{};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        auto sibling = std::filesystem::path(buf)
                           .parent_path().parent_path()
                       / "engine/admissibility.pl";
        if (std::filesystem::exists(sibling))
            return sibling;
    }

    return {};
}

struct SubprocResult {
    std::string output;
    int exit_code = -1;
};

SubprocResult invoke_swipl(const std::string& goal, const std::string& rules_path) {
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
        execlp("swipl", "swipl", "-q",
               "-g", goal.c_str(),
               "-t", "halt(1)",
               rules_path.c_str(),
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
    // Check for swipl.
    FILE* f = popen("command -v swipl >/dev/null 2>&1 && echo y", "r");
    if (!f) return false;
    char c = '\0';
    bool found = fread(&c, 1, 1, f) > 0 && c == 'y';
    pclose(f);
    return found;
}

auto parse_prolog_output(std::string_view raw, std::string& error) -> std::optional<bool> {
    // Trim whitespace.
    auto start = raw.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        error = "empty Prolog output";
        return std::nullopt;
    }
    auto end = raw.find_last_not_of(" \t\r\n");
    std::string trimmed(raw.substr(start, end - start + 1));

    if (trimmed == "admissible")   return true;
    if (trimmed == "inadmissible") return false;

    // Fail closed.
    error = "unexpected Prolog output: " + trimmed;
    return std::nullopt;
}

auto logical_admissibility(const AdmissibilityEvidence& ev, std::string& error)
    -> std::optional<AdmissibilityResult>
{
    if (!runtime_available()) {
        error = "swipl not found in PATH";
        return std::nullopt;
    }

    auto rules = find_rules();
    if (rules.empty()) {
        error = "engine/admissibility.pl not found";
        return std::nullopt;
    }

    // The evidence is passed as a conjunction of assertz/1 goals prepended
    // to main/0. SWI-Prolog executes -g after loading the rules file, so the
    // dynamic predicates are declared before any fact is asserted.
    std::string cpp_state   = ev.cpp_found   ? "present" : "absent";
    std::string algol_state = ev.algol_found ? "present" : "absent";
    std::string bf_state    = ev.bf_positive ? "positive" : "negative";
    char conf_buf[64];
    std::snprintf(conf_buf, sizeof(conf_buf), "%.4f", ev.confidence);
    std::string conf_str = conf_buf;

    std::string goal =
        "assertz(cpp_result("   + cpp_state   + "," + std::to_string(ev.cpp_index)   + ")),"
        "assertz(algol_result(" + algol_state + "," + std::to_string(ev.algol_index) + ")),"
        "assertz(bf_membership(" + bf_state + ")),"
        "assertz(confidence_value(" + conf_str + ")),"
        "main,halt(0)";

    // Ask Prolog.
    Timer timer;
    auto sub = invoke_swipl(goal, rules.string());
    long long ns = timer.elapsed_ns();

    if (sub.output.empty()) {
        error = "swipl exited " + std::to_string(sub.exit_code) + " with no output";
        return std::nullopt;
    }

    auto admissible = parse_prolog_output(sub.output, error);
    if (!admissible.has_value()) return std::nullopt;

    return AdmissibilityResult{*admissible, ns};
}

} // namespace hypertension::prolog
