extern int controlflow_if(int control);
extern int controlflow_while(int control);
extern int controlflow_do_while(int control);
extern int controlflow_for(int control);

int main(void) {
    _Bool ok = 1;

    ok &= controlflow_if(1) == 1;
    ok &= controlflow_if(0) == 1;
    ok &= controlflow_if(-1) == 0;
    ok &= controlflow_if(30) == 1;
    ok &= controlflow_if(-30) == 0;

    ok &= controlflow_while(1) == 4;
    ok &= controlflow_while(0) == 3;
    ok &= controlflow_while(5) == 8;

    ok &= controlflow_do_while(1) == 5;
    ok &= controlflow_do_while(0) == 5;
    ok &= controlflow_do_while(5) == 9;

    ok &= controlflow_for(1) == 6;
    ok &= controlflow_for(0) == 5;
    ok &= controlflow_for(5) == 10;

    return !ok;
}
