#pragma once

#ifndef MY_STDINT_H
#define MY_STDINT_H

/*
 * Portable stdint.h for a hypothetical standalone OS.
 * This is a simplified example; a real version needs
 * to be more comprehensive and handle different platform variations.
 */

// Basic integer types based on platform assumptions
#if defined(__x86_64__) || defined(_WIN64)
  typedef char int8_t;
  typedef unsigned char uint8_t;
  typedef short int16_t;
  typedef unsigned short uint16_t;
  typedef int int32_t;
  typedef unsigned int uint32_t;
  typedef long long int64_t;
  typedef unsigned long long uint64_t;
#elif defined(__i386__) || defined(_WIN32)
  typedef char int8_t;
  typedef unsigned char uint8_t;
  typedef short int16_t;
  typedef unsigned short uint16_t;
  typedef int int32_t;
  typedef unsigned int uint32_t;
  typedef long long int64_t; // May depend on the platform's long long size
  typedef unsigned long long uint64_t;
#else
  #error "Unsupported architecture for custom stdint.h"
#endif

// Add integer types for minimum/maximum sizes and pointer-related types
// ... (e.g., INT8_MIN, INT8_MAX, UINTPTR_MAX, etc.)

#endif // MY_STDINT_H


//typedef signed char int8_t;
//typedef unsigned char uint8_t;
//typedef signed short int16_t;
//typedef unsigned short uint16_t;
//typedef signed long int int32_t;
//typedef unsigned long int uint32_t;
//typedef signed long long int int64_t;
//typedef unsigned long long int uint64_t;

// typedef uint8_t bool; // replaced by custom stdbool.h

//#define false       0
//#define true        1

//#define NULL        ((void*)0)
//#define min(a,b)    ((a) < (b) ? (a) : (b))
//#define max(a,b)    ((a) > (b) ? (a) : (b))

// may have unwanted and unintended consequences cause of a and b being evaluated multiple times; plz fix later me. THX!!!

// also probs move to a more general header file later

// byeee  19:22 10/9/2025

// TEMPORARY KERNEL RESLOVE