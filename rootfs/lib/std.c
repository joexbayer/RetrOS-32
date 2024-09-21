enum {
    SYS_EXIT = 1,
    SYS_PUT = 2
};


void put(char c){
    __interrupt(0x30, SYS_PUT, c, 0, 0, 0);
}

int strlen(char* str){
    int a;
    a = 0;

    while(str[a] != 0){
	a = a + 1;
    }

    return a;
}

int print(char* str){
    int a;
    a = 0;

    while(str[a] != 0){
    	put(str[a]);
	a = a + 1;
    }

    return a;
}
