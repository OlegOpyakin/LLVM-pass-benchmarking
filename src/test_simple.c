#include <stdio.h>

int simple_cubic(int a, int b) {
    return a * a * a + 3 * a * a * b + 3 * a * b * b + b * b * b;
}

int main() {
    int result = simple_cubic(2, 3);
    int expected = 125;
    printf("Simple Test: result=%d, expected=%d -> %s\n", 
           result, expected, result == expected ? "PASS" : "FAIL");
    return result == expected ? 0 : 1;
}
