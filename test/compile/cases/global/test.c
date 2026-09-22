int global_val = 4;

int get_global(void) { return global_val; }
int set_global(int new) {
    global_val = new;
    return 0;
}
