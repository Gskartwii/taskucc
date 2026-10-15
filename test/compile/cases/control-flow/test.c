int controlflow_if(int control) {
    if (control < 0)
        return 0;
    else
        return 1;
}

int controlflow_while(int control) {
    int counter;
    int ret;

    counter = 0;
    ret = 3;

    while (counter < control) {
        ret = ret + 1;
        counter = counter + 1;
    }

    return ret;
}

int controlflow_do_while(int control) {
    int counter;
    int ret;

    counter = 0;
    ret = 4;

    do {
        ret = ret + 1;
        counter = counter + 1;
    } while (counter < control);

    return ret;
}

int controlflow_for(int control) {
    int counter;
    int ret;

    ret = 5;

    for (counter = 0; counter < control; counter = counter + 1) {
        ret = ret + 1;
    }

    return ret;
}
