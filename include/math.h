#ifndef E9ADA698_B583_4B4F_BBB9_3513A8E02614
#define E9ADA698_B583_4B4F_BBB9_3513A8E02614

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) ( (x) < (lo) ? (lo) : ( (x) > (hi) ? (hi) : (x) ) )
#define SIGN(x) ( (x) < 0 ? -1 : ( (x) > 0 ? 1 : 0 ) )


#define DISTANCE(x1, y1, x2, y2) sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2))
/* int startValue, int endValue, float t */
#define EASE(startValue, endValue, t) (startValue + (endValue - startValue) * ((t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t))

#define PI 3.14159265358979323846
#define DEG_TO_RAD(a) (float)(a*PI/180.0)

#endif /* E9ADA698_B583_4B4F_BBB9_3513A8E02614 */
