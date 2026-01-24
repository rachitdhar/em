int main() {
    int x = 0;
    int sum = 0;
    while (x < 10) {
        x = x + 1;
        if (x % 2 == 0) {
            continue;
        }
        sum = sum + x;
    }
    return sum;
}