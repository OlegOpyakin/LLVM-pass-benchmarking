#!/bin/bash

set -e

PASS_NAME="Pass"
PASS_SO="build/src/lib${PASS_NAME}.so"

run() {
    echo "--- Running: $@ ---
    "
    $@
}

build_pass() {
    if [ ! -d "build" ]; then
        run mkdir build
        (cd build && cmake ..)
    fi
    run make -C build
}

run_test() {
    local test_name="$1"
    local test_file="src/${test_name}.c"
    local test_out_dir="build/tests"
    local test_out="${test_out_dir}/${test_name}.out"

    echo "--- Testing: ${test_name} ---
    "

    mkdir -p "${test_out_dir}"

    run clang -fpass-plugin="${PASS_SO}" -Xclang -disable-O0-optnone -O0 "${test_file}" -o "${test_out}"
    run "${test_out}"
}

test_assembly() {
    echo "
    --- ASM Comparison Test ---"
    
    mkdir -p build/asm_out
    
    run clang -fpass-plugin="${PASS_SO}" -Xclang -disable-O0-optnone -O0 -S src/test_simple.c -o "build/asm_out/with_pass.s" # asm with pass
    
    run clang -Xclang -disable-O0-optnone -O0 -S src/test_simple.c -o "build/asm_out/without_pass.s" # asm without pass
    
    echo "ASM generated:"
    echo "  With pass:    build/asm_out/with_pass.s"
    echo "  Without pass: build/asm_out/without_pass.s"
    
    echo "
    Instruction count comparison:"
    with_count=$(grep -A 50 "_simple_cubic:" "build/asm_out/with_pass.s" | grep -E "^\t(add|mul|ldr)" | wc -l | xargs)
    without_count=$(grep -A 50 "_simple_cubic:" "build/asm_out/without_pass.s" | grep -E "^\t(add|mul|ldr)" | wc -l | xargs)
    
    echo "  With pass:    ${with_count} instructions"
    echo "  Without pass: ${without_count} instructions"
    echo "  Reduction:    $((without_count - with_count)) instructions"
}

test_performance() {
    mkdir -p build/perf
    
    echo "Compiling optimized version (with pass)..."
    run clang -fpass-plugin="${PASS_SO}" -Xclang -disable-O0-optnone -O0 src/benchmark.c -o "build/perf/benchmark_optimized.out"
    
    echo "Compiling unoptimized version (without pass)..."
    run clang -Xclang -disable-O0-optnone -O0 src/benchmark.c -o "build/perf/benchmark_unoptimized.out"
    
    echo "Running benchmarks..."
    
    echo "\n=== WITH PASS ==="
    optimized_output=$(build/perf/benchmark_optimized.out)
    echo "$optimized_output"
    optimized_time=$(echo "$optimized_output" | grep "Time:" | awk '{print $2}')
    
    echo "\n=== WITHOUT PASS ==="
    unoptimized_output=$(build/perf/benchmark_unoptimized.out)
    echo "$unoptimized_output"
    unoptimized_time=$(echo "$unoptimized_output" | grep "Time:" | awk '{print $2}')
    
    if [ -n "$optimized_time" ] && [ -n "$unoptimized_time" ]; then
        echo "\n=== PERFORMANCE COMPARISON ==="
        echo "Optimized time:   ${optimized_time} μs"
        echo "Unoptimized time: ${unoptimized_time} μs"
        
        speedup=$(echo "scale=2; $unoptimized_time / $optimized_time" | bc)
        improvement=$(echo "scale=2; ($unoptimized_time - $optimized_time) / $unoptimized_time * 100" | bc)
        
        echo "Speedup: ${speedup}x"
        echo "Performance improvement: ${improvement}%"
    else
        echo "Could not extract time"
    fi
}

# --- Main Script ---

echo "=== LLVM Pass Testing Suite ===
"

build_pass

run_test "test_simple"
run_test "test_temporaries"
run_test "test_original"

test_assembly
test_performance

echo "=== All tests passed! ==="
