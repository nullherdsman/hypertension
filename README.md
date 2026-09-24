# HyperTension

A fast, minimal search utility written in modern C++.

HyperTension provides a small command-line interface for searching values with minimal overhead and straightforward output.

## Build

Requires a C++23-compatible compiler.

```bash
g++ -std=c++23 -O3 main.cpp -o hypertension
```

## Usage

```bash
./hypertension search 42
```

Example:

```text
Found 42 at index 3.
Latency: 184 ns
```

Additional commands:

```bash
./hypertension --help
./hypertension --version
```

## Verified Search

HyperTension also provides an optional extended verification mode:

```bash
./hypertension search 42 --verified
```

Extended verification may require additional runtime components.

## Development

The standard search path is intentionally small, dependency-free, and performance-oriented.

C++23 · O(n) search · O(1) auxiliary space

## License

See `LICENSE`.
