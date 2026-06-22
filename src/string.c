#include "string.h"

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) { s1++; s2++; }
    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, unsigned int n) {
    while (n && *s1 && *s2 && *s1 == *s2) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return *s1 - *s2;
}