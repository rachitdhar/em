int main() {
    int x = 0;
    while (true) {
        x = x + 1;
        if (x == 3) {
            break;
        }
        if (x > 10) {
            break;
        }
    }
    return x;
}