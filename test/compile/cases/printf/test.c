extern int printf(const char *fmt, ...);

int main(void) {
    printf("%s\n", "test 1");
    printf("test %d\n", 2);
    printf("test %lld\n", (long long) 0xFFFFFFFFFFFFFF);
    return 0;
}
