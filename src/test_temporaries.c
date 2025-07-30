#include <stdio.h>

int cubic_with_temps(int a, int b) {
    int a3 = a * a * a;
    int b3 = b * b * b;
    int three_a2_b = 3 * a * a * b;
    int three_ab2 = 3 * a * b * b;
    return a3 + three_a2_b + three_ab2 + b3;
}

int main() {
    int result = cubic_with_temps(2, 3);
    int expected = 125;
    printf("Temporaries Test: result=%d, expected=%d -> %s\n", 
           result, expected, result == expected ? "PASS" : "FAIL");
    return result == expected ? 0 : 1;
}
