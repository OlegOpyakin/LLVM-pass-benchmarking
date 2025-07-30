#include <stdio.h>

int original_cubic(int a) {
    int b = 2;
    return a * a * a + 3 * a * a * b + 3 * a * b * b + b * b * b;
}

int main() {
    int result = original_cubic(3);
    int expected = 125;
    printf("Original Test: result=%d, expected=%d -> %s\n", 
           result, expected, result == expected ? "PASS" : "FAIL");
    return result == expected ? 0 : 1;
}
