int main() {
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        if (i < 3) {
            continue;
        }
        if (i > 7) {
            continue;
        }
        sum = sum + i;
    }
    return sum;
}