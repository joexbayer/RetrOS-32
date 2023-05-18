#ifndef CB6305A9_E364_46B4_A6D9_005ADCCD8710
#define CB6305A9_E364_46B4_A6D9_005ADCCD8710

#define DISPLAY_MAX_NAME 25

struct display_info {
    int width;
    int height;
    int bpp;
    int color;
    char name[DISPLAY_MAX_NAME];
};

#endif /* CB6305A9_E364_46B4_A6D9_005ADCCD8710 */
