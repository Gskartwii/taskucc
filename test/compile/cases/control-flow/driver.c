extern int controlflow_if(int control);

int main(void) {
    _Bool ok = 1;

    ok &= controlflow_if(1) == 1;
    ok &= controlflow_if(0) == 1;
    ok &= controlflow_if(-1) == 0;
    ok &= controlflow_if(30) == 1;
    ok &= controlflow_if(-30) == 0;

    return !ok;
}
