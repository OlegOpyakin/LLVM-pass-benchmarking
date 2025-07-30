#include <stdio.h>
#include <time.h>
#include <sys/time.h>

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000.0 + tv.tv_usec;
}

int cubic_function(int a, int b) {
    return a * a * a + 3 * a * a * b + 3 * a * b * b + b * b * b;
}

int main() {
    const int warmup_iterations = 10000;
    const int iterations = 1010000;
    int result = 0;
    
    for (int i = 0; i < warmup_iterations; i++) {
        int a = (i % 10) + 1;
        int b = (i % 7) + 2;
        result += cubic_function(a, b);
    }
    
    double start_time = get_time();
    
    for (int i = warmup_iterations; i < iterations; i++) {
        int a = (i % 10) + 1;
        int b = (i % 7) + 2;
        result += cubic_function(a, b);
    }
    
    double end_time = get_time();
    double elapsed = end_time - start_time;
    
    printf("Iterations: %d\n", iterations - warmup_iterations);
    printf("Time: %.2f microseconds\n", elapsed);
    printf("Result checksum: %d\n", result);
    
    return 0;
}
