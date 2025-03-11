#include <stdio.h>

int main() {
    int num = 5;
    printf("!!num = %d\n", !!num);
    if (!!num) {
        return 1;
    }
    return 0;
}