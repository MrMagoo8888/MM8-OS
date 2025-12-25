#pragma once

#ifndef _STDBOOL_H
#define _STDBOOL_H

#define __bool_true_false_are_defined 1

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define bool _Bool
#else
    typedef unsigned char bool;
#endif

#define true 1
#define false 0
#endif