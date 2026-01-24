int main() {
    int x = 3;
    int y = 3;
    int sum = 0;
    while (x > 0) {
        while (y > 0) {
            sum = sum + 1;
            y = y - 1;
        }
        x = x - 1;
        y = 3;
    }
    return sum;
}