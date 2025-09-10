#pragma once

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed long int int32_t;
typedef unsigned long int uint32_t;
typedef signed long long int int64_t;
typedef unsigned long long int uint64_t;

typedef uint8_t bool;

#define false       0
#define true        1

#define NULL        ((void*)0)
#define min(a,b)    ((a) < (b) ? (a) : (b))
#define max(a,b)    ((a) > (b) ? (a) : (b))

// may have unwanted and unintended consequences cause of a and b being evaluated multiple times; plz fix later me. THX!!!

// also probs move to a more general header file later

// byeee  19:22 10/9/2025