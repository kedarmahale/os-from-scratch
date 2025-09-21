unsigned long long __udivdi3(unsigned long long numerator, unsigned long long denominator) {
    unsigned long long quotient = 0, remainder = 0;
    int i;
    for (i = 63; i >= 0; i--) {
        remainder <<= 1;
        remainder |= (numerator >> i) & 1;
        if (remainder >= denominator) {
            remainder -= denominator;
            quotient |= (1ULL << i);
        }
    }
    return quotient;
}

