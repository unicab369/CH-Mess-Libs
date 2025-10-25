#define BUF_MAKE_U16(buff) ((buff[0] << 8) | buff[1])

// return 1 = Success, 0 = Timeout
#define UTIL_WAIT_FOR(condition, timeout_counter) ({ \
	u32 timeout = timeout_counter; \
	u8 result = 0; \
	while (!(condition)) { \
		if (--timeout == 0) { result = 0; break; } \
	} \
	if (timeout > 0) result = 1; \
	result; \
})


#define UTIL_PRINT_BITS(val, len, separator) do { \
    for (int i = len-1; i >= 0; i--) { \
        printf("%s", (((val) >> i) & 1) ? "1" : "0"); \
        if (i == len/2) printf("%s", separator); \
        else if (i > 0) printf(" "); \
    } \
} while(0)


#define UTIL_PRINT_BITS_8(val) UTIL_PRINT_BITS(val, 8, " | ")
#define UTIL_PRINT_BITS_16(val) UTIL_PRINT_BITS(val, 16, " | ")
#define UTIL_PRINT_BITS_32(val) UTIL_PRINT_BITS(val, 32, " | ")

#define UTIL_PRINT_REG8(reg, label) do { \
    printf("%s: 0x%02X \t", label, reg); \
    UTIL_PRINT_BITS_8(reg); \
    printf("\n"); \
} while(0)

#define UTIL_PRINT_REG16(reg, label) do { \
    printf("%s: 0x%04X \t", label, reg); \
    UTIL_PRINT_BITS_16(reg); \
    printf("\n"); \
} while(0)

#define UTIL_PRINT_REG32(reg, label) do { \
    printf("%s: 0x%08lX \t", label, reg); \
    UTIL_PRINT_BITS_32(reg); \
    printf("\n"); \
} while(0)