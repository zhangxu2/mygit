#ifndef __PUB_MATH_H__
#define __PUB_MATH_H__

#define pub_math_abs(value)       (((value) >= 0) ? (value) : - (value))
#define pub_math_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define pub_math_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))

#endif

