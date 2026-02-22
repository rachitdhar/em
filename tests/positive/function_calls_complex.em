int func1() {
    return 1;
}
int func2(int x) {
    return x + 1;
}
int main() {
    int x = func2(func1());
    return 0;
}