extern int printf(const char *fmt, ...);

int main(void) {
    printf("%s\n", "test 1");
    printf("test %d\n", 2);
    printf("test %lld\n", (long long) 0xFFFFFFFFFFFFFF);
    printf("test %f\n", 1.0);
    printf("test %f\n", 2.3456e89);
    return 0;
}
