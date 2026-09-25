int global_val = 4;

int get_global(void) { return global_val; }
int set_global(int new) {
    global_val = new;
    return 0;
}

int indirect_access(void) { return get_global(); }
int indirect_set(int new) { return set_global(new); }
