#ifndef STDBOOL_H
#define STDBOOL_H

// Check if the C standard is C99 or later
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201710L

#define bool _Bool // _Bool is the built-in type for boolean values
#define true 1     // true expands to the integer constant 1
#define false 0    // false expands to the integer constant 0

#else // Fallback for C89 or older compilers

// Provide alternative definitions if _Bool is not available
typedef signed char bool;
#define true 1
#define false 0

#endif

// Define __bool_true_false_are_defined as per C standard
#define __bool_true_false_are_defined 1

#endif // TDBOOL_H