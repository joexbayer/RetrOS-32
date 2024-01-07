// Triangle

void draw_triangle(int height) {
    int j;
    int i;

    while(i < height) {
        j = 0;
        while(j <= i) {
            printf("* ");
            j++;
        }
        printf("\n");
        i++;
    }
}

int main() {
    int height = 5;
    printf("Triangle of height %d:\n", height);
    draw_triangle(height);

    return 0;
}