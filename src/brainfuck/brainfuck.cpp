#include "brainfuck.hpp"

#include <array>
#include <stack>
#include <string>
#include <vector>

namespace hypertension::bf {

namespace {

// The tape is exactly 30000 cells, as originally specified by Urban Müller
// in 1993. Programs that require more cells are not supported.
constexpr std::size_t TAPE_SIZE  = 30000;
constexpr std::size_t MAX_STEPS  = 10'000'000;

} // namespace

auto run(std::string_view program, std::span<const uint8_t> input) -> RunResult {
    // Validate bracket matching and build the jump table in a single forward pass.
    // Each '[' pushes its position onto open_stack; each matching ']' pops and
    // records a bidirectional mapping in jump[]: jump['['] = ']' and jump[']'] = '['.
    // Entries at positions that are not '[' or ']' remain zero and are never read.
    std::stack<std::size_t> open_stack;
    std::vector<std::size_t> jump(program.size(), 0);

    for (std::size_t i = 0; i < program.size(); ++i) {
        if (program[i] == '[') {
            open_stack.push(i);
        } else if (program[i] == ']') {
            if (open_stack.empty())
                return {{}, "unmatched ']' at position " + std::to_string(i)};
            std::size_t j = open_stack.top();
            open_stack.pop();
            jump[i] = j;
            jump[j] = i;
        }
    }
    if (!open_stack.empty())
        return {{}, "unmatched '[' at position " + std::to_string(open_stack.top())};

    std::array<uint8_t, TAPE_SIZE> tape{};
    std::size_t dp = 0;
    std::size_t ip = 0;
    std::size_t input_pos = 0;
    std::string output;
    std::size_t steps = 0;

    // Execute.
    while (ip < program.size()) {
        // Step limit.
        if (++steps > MAX_STEPS)
            return {{}, "execution limit exceeded"};

        switch (program[ip]) {
            case '>':
                if (dp + 1 >= TAPE_SIZE) return {{}, "tape pointer out of bounds"};
                ++dp; break;
            case '<':
                if (dp == 0) return {{}, "tape pointer out of bounds"};
                --dp; break;
            case '+': ++tape[dp]; break;
            case '-': --tape[dp]; break;
            case '.': output += static_cast<char>(tape[dp]); break;
            case ',':
                // If the input stream has been exhausted, the cell receives zero.
                // This is the conventional behavior for Brainfuck interpreters
                // and is required for programs that read fewer bytes than available.
                tape[dp] = input_pos < input.size() ? input[input_pos++] : 0;
                break;
            case '[': if (tape[dp] == 0) ip = jump[ip]; break;
            case ']': if (tape[dp] != 0) ip = jump[ip]; break;
            default: break;
        }
        // Increment ip.
        ++ip;
    }

    return {output, {}};
}

} // namespace hypertension::bf
