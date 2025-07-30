# LLVM Pass for Cubic Expression Optimization

## Related projects

- [Control flow graph analysis](https://github.com/OlegOpyakin/Dominators.git)
- [Reaching definitions analysis (also AE & LV)](https://github.com/OlegOpyakin/Graphs.git)
- [Cycles (VDG) Analysis](https://github.com/OlegOpyakin/Cycles.git)

## Overview

This project demonstrates a custom LLVM pass that optimizes cubic polynomial expressions. The pass identifies the expanded form of `(a+b)³` which is `a³ + 3a²b + 3ab² + b³` and transforms it into the more efficient `(a+b)³` computation. This optimization reduces both instruction count and execution time significantly.

The pass handles various IR patterns including code with temporary variables and load/store operations, making it robust for real-world unoptimized code.

## Features

- **Pattern Recognition**: Identifies cubic expansion patterns in LLVM IR.
- **IR Transformation**: Replaces expanded cubic expressions with optimized `(a+b)³` form.
- **Load-Through Analysis**: Handles unoptimized code with temporary variables and memory operations.
- **Performance Benchmarking**: Comprehensive testing suite with performance measurements.
- **Assembly Analysis**: Generates and compares assembly output to verify optimizations.

## Project Structure

```
LLVM-pass-benchmarking/
├── src/
│   ├── pass.cc                 # Main LLVM pass implementation
│   ├── a.c                     # Original test case
│   ├── simple_cubic.c          # Direct cubic expansion test
│   ├── explicit_cubic.c        # Test with temporary variables
│   ├── test_*.c                # Generated test files
│   └── benchmark.c             # Performance benchmark (generated)
├── build/
│   ├── src/libPass.so          # Compiled LLVM pass
│   ├── tests/                  # Test executables
│   ├── asm_out/                # Assembly output files
│   └── perf/                   # Performance benchmark executables
├── run_tests.sh                # Automated testing script
├── CMakeLists.txt              # Build configuration
└── README.md
```

## Building and Running

### Prerequisites

- C++17 compatible compiler (GCC 9+, Clang 10+)
- CMake 3.16+
- LLVM development libraries (tested with LLVM 20+)
- bc calculator (for performance calculations)

### Build Instructions

```bash
# Run all tests automatically
./run_tests.sh
```

The test script will:
1. Build the LLVM pass
2. Run functional correctness tests
3. Generate assembly comparisons
4. Execute performance benchmarks

All outputs are saved in the `build/` directory by default.

## Performance Results

The pass demonstrates significant improvements:

### Assembly Reduction
- **With pass**: 8 instructions
- **Without pass**: 27 instructions  
- **Reduction**: 19 instructions (70% fewer)

### Runtime Performance
- **Speedup**: 4.83x faster execution
- **Improvement**: 70% reduction in execution time
- **Test**: 1,000,000 iterations with warmup

## Examples

### Input Code
```c
int cubic_function(int a, int b) {
    return a * a * a + 3 * a * a * b + 3 * a * b * b + b * b * b;
}
```

### LLVM IR Transformation

**Before optimization (27 instructions):**
```llvm
%5 = load i32, ptr %3, align 4
%6 = load i32, ptr %3, align 4
%7 = mul nsw i32 %5, %6
%8 = load i32, ptr %3, align 4
%9 = mul nsw i32 %7, %8
; ... many more instructions
```

**After optimization (3 instructions):**
```llvm
%29 = add i32 %0, %1          ; a + b
%30 = mul i32 %29, %29        ; (a + b)²
%31 = mul i32 %30, %29        ; (a + b)³
```

### Assembly Output

**Optimized version:**
```assembly
add  w9, w0, w1     ; a + b
mul  w8, w9, w9     ; (a + b)²  
mul  w0, w8, w9     ; (a + b)³
```

This demonstrates the dramatic reduction from complex multiplication and addition sequences to just three simple operations.
