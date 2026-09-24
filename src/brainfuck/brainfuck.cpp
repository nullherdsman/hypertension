#include "brainfuck.hpp"

#include <array>
#include <stack>
#include <string>
#include <vector>

namespace hypertension::bf {

namespace {

constexpr std::size_t TAPE_SIZE  = 30000;
constexpr std::size_t MAX_STEPS  = 10'000'000;

} // namespace

auto run(std::string_view program, std::span<const uint8_t> input) -> RunResult {
    // Validate bracket matching and build jump table in one pass.
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

    while (ip < program.size()) {
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
                tape[dp] = input_pos < input.size() ? input[input_pos++] : 0;
                break;
            case '[': if (tape[dp] == 0) ip = jump[ip]; break;
            case ']': if (tape[dp] != 0) ip = jump[ip]; break;
            default: break;
        }
        ++ip;
    }

    return {output, {}};
}

} // namespace hypertension::bf
