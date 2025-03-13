C++ Mathematical Expression Toolkit Library Documentation

  Section 00 - Introduction
  Section 01 - Capabilities
  Section 02 - Example Expressions
  Section 03 - Copyright Notice
  Section 04 - Downloads & Updates
  Section 05 - Installation
  Section 06 - Compilation
  Section 07 - Compiler Compatibility
  Section 08 - Built-In Operations & Functions
  Section 09 - Fundamental Types
  Section 10 - Components
  Section 11 - Compilation Options
  Section 12 - Expression Structures
  Section 13 - Variable, Vector & String Definition
  Section 14 - Vector Processing
  Section 15 - User Defined Functions
  Section 16 - Expression Dependents
  Section 17 - Hierarchies Of Symbol Tables
  Section 18 - Unknown Unknowns
  Section 19 - Enabling & Disabling Features
  Section 20 - Expression Return Values
  Section 21 - Compilation Errors
  Section 22 - Runtime Library Packages
  Section 23 - Helpers & Utils
  Section 24 - Runtime Checks
  Section 25 - Benchmarking
  Section 26 - Exprtk Notes
  Section 27 - Simple Exprtk Example
  Section 28 - Build Options
  Section 29 - Files
  Section 30 - Language Structure


[SECTION 00 - INTRODUCTION]
The C++ Mathematical Expression  Toolkit Library (ExprTk) is  a simple
to  use,   easy  to   integrate  and   extremely  efficient   run-time
mathematical  expression parsing  and evaluation  engine. The  parsing
engine  supports numerous  forms  of  functional and  logic processing
semantics and is easily extensible.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 01 - CAPABILITIES]
The  ExprTk expression  evaluator supports  the following  fundamental
arithmetic operations, functions and processes:

 (00) Types:           Scalar, Vector, String

 (01) Basic operators: +, -, *, /, %, ^

 (02) Assignment:      :=, +=, -=, *=, /=, %=

 (03) Equalities &
      Inequalities:    =, ==, <>, !=, <, <=, >, >=

 (04) Logic operators: and, mand, mor, nand, nor, not, or, shl, shr,
                       xnor, xor, true, false

 (05) Functions:       abs, avg, ceil, clamp, equal, erf, erfc,  exp,
                       expm1, floor, frac,  log, log10, log1p,  log2,
                       logn, max,  min, mul,  ncdf,  not_equal, root,
                       round, roundn, sgn, sqrt, sum, swap, trunc

 (06) Trigonometry:    acos, acosh, asin, asinh, atan, atanh,  atan2,
                       cos,  cosh, cot,  csc, sec,  sin, sinc,  sinh,
                       tan, tanh, hypot, rad2deg, deg2grad,  deg2rad,
                       grad2deg

 (07) Control
      structures:      if-then-else, ternary conditional, switch-case,
                       return-statement

 (08) Loop statements: while, for, repeat-until, break, continue

 (09) String
      processing:      in, like, ilike, concatenation

 (10) Optimisations:   constant-folding, simple strength reduction and
                       dead code elimination

 (11) Runtime checks:  vector bounds, string bounds, loop iteration,
                       execution-time bounds and compilation process
                       checkpointing, assert statements

 (12) Calculus:        numerical integration and differentiation

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 02 - EXAMPLE EXPRESSIONS]
The following is  a short listing  of infix format  based mathematical
expressions that can be parsed and evaluated using the ExprTk library.

  (01) sqrt(1 - (3 / x^2))
  (02) clamp(-1, sin(2 * pi * x) + cos(y / 2 * pi), +1)
  (03) sin(2.34e-3 * x)
  (04) if(((x[2] + 2) == 3) and ((y + 5) <= 9),1 + w, 2 / z)
  (05) inrange(-2,m,+2) == if(({-2 <= m} and [m <= +2]),1,0)
  (06) ({1/1}*[1/2]+(1/3))-{1/4}^[1/5]+(1/6)-({1/7}+[1/8]*(1/9))
  (07) a * exp(2.2 / 3.3 * t) + c
  (08) z := x + sin(2.567 * pi / y)
  (09) u := 2.123 * {pi * z} / (w := x + cos(y / pi))
  (10) 2x + 3y + 4z + 5w == 2 * x + 3 * y + 4 * z + 5 * w
  (11) 3(x + y) / 2.9 + 1.234e+12 == 3 * (x + y) / 2.9 + 1.234e+12
  (12) (x + y)3.3 + 1 / 4.5 == [x + y] * 3.3 + 1 / 4.5
  (13) (x + y[i])z + 1.1 / 2.7 == (x + y[i]) * z + 1.1 / 2.7
  (14) (sin(x / pi) cos(2y) + 1) == (sin(x / pi) * cos(2 * y) + 1)
  (15) 75x^17 + 25.1x^5 - 35x^4 - 15.2x^3 + 40x^2 - 15.3x + 1
  (16) (avg(x,y) <= x + y ? x - y : x * y) + 2.345 * pi / x
  (17) while (x <= 100) { x -= 1; }
  (18) x <= 'abc123' and (y in 'AString') or ('1x2y3z' != z)
  (19) ((x + 'abc') like '*123*') or ('a123b' ilike y)
  (20) sgn(+1.2^3.4z / -5.6y) <= {-7.8^9 / -10.11x }

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 03 - COPYRIGHT NOTICE]
Free  use  of  the  C++  Mathematical  Expression  Toolkit  Library is
permitted under the guidelines and in accordance with the most current
version of the MIT License.

   (1) https://www.opensource.org/licenses/MIT
   (2) SPDX-License-Identifier: MIT
   (3) SPDX-FileCopyrightText : Copyright (C) 1999-2024 Arash Partow

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 04 - DOWNLOADS & UPDATES]
The most  recent version  of the C++ Mathematical  Expression  Toolkit
Library including all updates and tests can be found at the  following
locations:

  (1) Download: https://www.partow.net/programming/exprtk/index.html
  (2) Mirror Repository: https://github.com/ArashPartow/exprtk
                         https://github.com/ArashPartow/exprtk-extras

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 05 - INSTALLATION]
The header  file exprtk.hpp  should be  placed in a project or  system
include path (e.g: /usr/include/).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 06 - COMPILATION]
The  ExprTk  package  contains  the ExprTk  header,  a  set  of simple
examples and a benchmark and unit test  suite. The following is a list
of commands to build the various components:

  (a) For a complete build: make clean all
  (b) For a PGO build: make clean pgo
  (c) To strip executables: make strip_bin
  (d) Execute valgrind check: make valgrind_check

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 07 - COMPILER COMPATIBILITY]
ExprTk has been built error and warning free using the following set
of C++ compilers:

  (*) GNU Compiler Collection (3.5+)
  (*) Clang/LLVM (1.1+)
  (*) Microsoft Visual Studio C++ Compiler (7.1+)
  (*) Intel C++ Compiler (8.x+)
  (*) AMD Optimizing C++ Compiler (1.2+)
  (*) Nvidia C++ Compiler (19.x+)
  (*) PGI C++ (10.x+)
  (*) Circle C++ (circa: b81c37d2bb227c)
  (*) IBM XL C/C++ (9.x+)
  (*) C++ Builder (XE4+)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 08 - BUILT-IN OPERATIONS & FUNCTIONS]

(0) Arithmetic & Assignment Operators
+----------+---------------------------------------------------------+
| OPERATOR | DEFINITION                                              |
+----------+---------------------------------------------------------+
|  +       | Addition between x and y.  (eg: x + y)                  |
+----------+---------------------------------------------------------+
|  -       | Subtraction between x and y.  (eg: x - y)               |
+----------+---------------------------------------------------------+
|  *       | Multiplication between x and y.  (eg: x * y)            |
+----------+---------------------------------------------------------+
|  /       | Division between x and y.  (eg: x / y)                  |
+----------+---------------------------------------------------------+
|  %       | Modulus of x with respect to y.  (eg: x % y)            |
+----------+---------------------------------------------------------+
|  ^       | x to the power of y.  (eg: x ^ y)                       |
+----------+---------------------------------------------------------+
|  :=      | Assign the value of x to y. Where y is either a variable|
|          | or vector type.  (eg: y := x)                           |
+----------+---------------------------------------------------------+
|  +=      | Increment x by the value of the expression on the right |
|          | hand side. Where x is either a variable or vector type. |
|          | (eg: x += abs(y - z))                                   |
+----------+---------------------------------------------------------+
|  -=      | Decrement x by the value of the expression on the right |
|          | hand side. Where x is either a variable or vector type. |
|          | (eg: x[i] -= abs(y + z))                                |
+----------+---------------------------------------------------------+
|  *=      | Assign the multiplication of x by the value of the      |
|          | expression on the righthand side to x. Where x is either|
|          | a variable or vector type.                              |
|          | (eg: x *= abs(y / z))                                   |
+----------+---------------------------------------------------------+
|  /=      | Assign the division of x by the value of the expression |
|          | on the right-hand side to x. Where x is either a        |
|          | variable or vector type.  (eg: x[i + j] /= abs(y * z))  |
+----------+---------------------------------------------------------+
|  %=      | Assign x modulo the value of the expression on the right|
|          | hand side to x. Where x is either a variable or vector  |
|          | type.  (eg: x[2] %= y ^ 2)                              |
+----------+---------------------------------------------------------+

(1) Equalities & Inequalities
+----------+---------------------------------------------------------+
| OPERATOR | DEFINITION                                              |
+----------+---------------------------------------------------------+
| == or =  | True only if x is strictly equal to y. (eg: x == y)     |
+----------+---------------------------------------------------------+
| <> or != | True only if x does not equal y. (eg: x <> y or x != y) |
+----------+---------------------------------------------------------+
|  <       | True only if x is less than y. (eg: x < y)              |
+----------+---------------------------------------------------------+
|  <=      | True only if x is less than or equal to y. (eg: x <= y) |
+----------+---------------------------------------------------------+
|  >       | True only if x is greater than y. (eg: x > y)           |
+----------+---------------------------------------------------------+
|  >=      | True only if x greater than or equal to y. (eg: x >= y) |
+----------+---------------------------------------------------------+

(2) Boolean Operations
+----------+---------------------------------------------------------+
| OPERATOR | DEFINITION                                              |
+----------+---------------------------------------------------------+
| true     | True state or any value other than zero (typically 1).  |
+----------+---------------------------------------------------------+
| false    | False state, value of exactly zero.                     |
+----------+---------------------------------------------------------+
| and      | Logical AND, True only if x and y are both true.        |
|          | (eg: x and y)                                           |
+----------+---------------------------------------------------------+
| mand     | Multi-input logical AND, True only if all inputs are    |
|          | true. Left to right short-circuiting of expressions.    |
|          | (eg: mand(x > y, z < w, u or v, w and x))               |
+----------+---------------------------------------------------------+
| mor      | Multi-input logical OR, True if at least one of the     |
|          | inputs are true. Left to right short-circuiting of      |
|          | expressions.  (eg: mor(x > y, z < w, u or v, w and x))  |
+----------+---------------------------------------------------------+
| nand     | Logical NAND, True only if either x or y is false.      |
|          | (eg: x nand y)                                          |
+----------+---------------------------------------------------------+
| nor      | Logical NOR, True only if the result of x or y is false |
|          | (eg: x nor y)                                           |
+----------+---------------------------------------------------------+
| not      | Logical NOT, Negate the logical sense of the input.     |
|          | (eg: not(x and y) == x nand y)                          |
+----------+---------------------------------------------------------+
| or       | Logical OR, True if either x or y is true. (eg: x or y) |
+----------+---------------------------------------------------------+
| xor      | Logical XOR, True only if the logical states of x and y |
|          | differ.  (eg: x xor y)                                  |
+----------+---------------------------------------------------------+
| xnor     | Logical XNOR, True iff the biconditional of x and y is  |
|          | satisfied.  (eg: x xnor y)                              |
+----------+---------------------------------------------------------+
| &        | Similar to AND but with left to right expression short  |
|          | circuiting optimisation.  (eg: (x & y) == (y and x))    |
+----------+---------------------------------------------------------+
| |        | Similar to OR but with left to right expression short   |
|          | circuiting optimisation.  (eg: (x | y) == (y or x))     |
+----------+---------------------------------------------------------+

(3) General Purpose Functions
+----------+---------------------------------------------------------+
| FUNCTION | DEFINITION                                              |
+----------+---------------------------------------------------------+
| abs      | Absolute value of x.  (eg: abs(x))                      |
+----------+---------------------------------------------------------+
| avg      | Average of all the inputs.                              |
|          | (eg: avg(x,y,z,w,u,v) == (x + y + z + w + u + v) / 6)   |
+----------+---------------------------------------------------------+
| ceil     | Smallest integer that is greater than or equal to x.    |
+----------+---------------------------------------------------------+
| clamp    | Clamp x in range between r0 and r1, where r0 < r1.      |
|          | (eg: clamp(r0,x,r1))                                    |
+----------+---------------------------------------------------------+
| equal    | Equality test between x and y using normalised epsilon  |
+----------+---------------------------------------------------------+
| erf      | Error function of x.  (eg: erf(x))                      |
+----------+---------------------------------------------------------+
| erfc     | Complimentary error function of x.  (eg: erfc(x))       |
+----------+---------------------------------------------------------+
| exp      | e to the power of x.  (eg: exp(x))                      |
+----------+---------------------------------------------------------+
| expm1    | e to the power of x minus 1, where x is very small.     |
|          | (eg: expm1(x))                                          |
+----------+---------------------------------------------------------+
| floor    | Largest integer that is less than or equal to x.        |
|          | (eg: floor(x))                                          |
+----------+---------------------------------------------------------+
| frac     | Fractional portion of x.  (eg: frac(x))                 |
+----------+---------------------------------------------------------+
| hypot    | Hypotenuse of x and y (eg: hypot(x,y) = sqrt(x*x + y*y))|
+----------+---------------------------------------------------------+
| iclamp   | Inverse-clamp x outside of the range r0 and r1. Where   |
|          | r0 < r1. If x is within the range it will snap to the   |
|          | closest bound. (eg: iclamp(r0,x,r1)                     |
+----------+---------------------------------------------------------+
| inrange  | In-range returns 'true' when x is within the range r0   |
|          | and r1. Where r0 < r1.  (eg: inrange(r0,x,r1)           |
+----------+---------------------------------------------------------+
| log      | Natural logarithm of x.  (eg: log(x))                   |
+----------+---------------------------------------------------------+
| log10    | Base 10 logarithm of x.  (eg: log10(x))                 |
+----------+---------------------------------------------------------+
| log1p    | Natural logarithm of 1 + x, where x is very small.      |
|          | (eg: log1p(x))                                          |
+----------+---------------------------------------------------------+
| log2     | Base 2 logarithm of x.  (eg: log2(x))                   |
+----------+---------------------------------------------------------+
| logn     | Base N logarithm of x. where n is a positive integer.   |
|          | (eg: logn(x,8))                                         |
+----------+---------------------------------------------------------+
| max      | Largest value of all the inputs. (eg: max(x,y,z,w,u,v)) |
+----------+---------------------------------------------------------+
| min      | Smallest value of all the inputs. (eg: min(x,y,z,w,u))  |
+----------+---------------------------------------------------------+
| mul      | Product of all the inputs.                              |
|          | (eg: mul(x,y,z,w,u,v,t) == (x * y * z * w * u * v * t)) |
+----------+---------------------------------------------------------+
| ncdf     | Normal cumulative distribution function.  (eg: ncdf(x)) |
+----------+---------------------------------------------------------+
| not_equal| Not-equal test between x and y using normalised epsilon |
+----------+---------------------------------------------------------+
| pow      | x to the power of y.  (eg: pow(x,y) == x ^ y)           |
+----------+---------------------------------------------------------+
| root     | Nth-Root of x. where n is a positive integer.           |
|          | (eg: root(x,3) == x^(1/3))                              |
+----------+---------------------------------------------------------+
| round    | Round x to the nearest integer.  (eg: round(x))         |
+----------+---------------------------------------------------------+
| roundn   | Round x to n decimal places  (eg: roundn(x,3))          |
|          | where n > 0 and is an integer.                          |
|          | (eg: roundn(1.2345678,4) == 1.2346)                     |
+----------+---------------------------------------------------------+
| sgn      | Sign of x, -1 where x < 0, +1 where x > 0, else zero.   |
|          | (eg: sgn(x))                                            |
+----------+---------------------------------------------------------+
| sqrt     | Square root of x, where x >= 0.  (eg: sqrt(x))          |
+----------+---------------------------------------------------------+
| sum      | Sum of all the inputs.                                  |
|          | (eg: sum(x,y,z,w,u,v,t) == (x + y + z + w + u + v + t)) |
+----------+---------------------------------------------------------+
| swap     | Swap the values of the variables x and y and return the |
| <=>      | current value of y.  (eg: swap(x,y) or x <=> y)         |
+----------+---------------------------------------------------------+
| trunc    | Integer portion of x.  (eg: trunc(x))                   |
+----------+---------------------------------------------------------+

(4) Trigonometry Functions
+----------+---------------------------------------------------------+
| FUNCTION | DEFINITION                                              |
+----------+---------------------------------------------------------+
| acos     | Arc cosine of x expressed in radians. Interval [-1,+1]  |
|          | (eg: acos(x))                                           |
+----------+---------------------------------------------------------+
| acosh    | Inverse hyperbolic cosine of x expressed in radians.    |
|          | (eg: acosh(x))                                          |
+----------+---------------------------------------------------------+
| asin     | Arc sine of x expressed in radians. Interval [-1,+1]    |
|          | (eg: asin(x))                                           |
+----------+---------------------------------------------------------+
| asinh    | Inverse hyperbolic sine of x expressed in radians.      |
|          | (eg: asinh(x))                                          |
+----------+---------------------------------------------------------+
| atan     | Arc tangent of x expressed in radians. Interval [-1,+1] |
|          | (eg: atan(x))                                           |
+----------+---------------------------------------------------------+
| atan2    | Arc tangent of (x / y) expressed in radians. [-pi,+pi]  |
|          | eg: atan2(x,y)                                          |
+----------+---------------------------------------------------------+
| atanh    | Inverse hyperbolic tangent of x expressed in radians.   |
|          | (eg: atanh(x))                                          |
+----------+---------------------------------------------------------+
| cos      | Cosine of x.  (eg: cos(x))                              |
+----------+---------------------------------------------------------+
| cosh     | Hyperbolic cosine of x.  (eg: cosh(x))                  |
+----------+---------------------------------------------------------+
| cot      | Cotangent of x.  (eg: cot(x))                           |
+----------+---------------------------------------------------------+
| csc      | Cosecant of x.  (eg: csc(x))                            |
+----------+---------------------------------------------------------+
| sec      | Secant of x.  (eg: sec(x))                              |
+----------+---------------------------------------------------------+
| sin      | Sine of x.  (eg: sin(x))                                |
+----------+---------------------------------------------------------+
| sinc     | Sine cardinal of x.  (eg: sinc(x))                      |
+----------+---------------------------------------------------------+
| sinh     | Hyperbolic sine of x.  (eg: sinh(x))                    |
+----------+---------------------------------------------------------+
| tan      | Tangent of x.  (eg: tan(x))                             |
+----------+---------------------------------------------------------+
| tanh     | Hyperbolic tangent of x.  (eg: tanh(x))                 |
+----------+---------------------------------------------------------+
| deg2rad  | Convert x from degrees to radians.  (eg: deg2rad(x))    |
+----------+---------------------------------------------------------+
| deg2grad | Convert x from degrees to gradians.  (eg: deg2grad(x))  |
+----------+---------------------------------------------------------+
| rad2deg  | Convert x from radians to degrees.  (eg: rad2deg(x))    |
+----------+---------------------------------------------------------+
| grad2deg | Convert x from gradians to degrees.  (eg: grad2deg(x))  |
+----------+---------------------------------------------------------+

(5) String Processing
+----------+---------------------------------------------------------+
| FUNCTION | DEFINITION                                              |
+----------+---------------------------------------------------------+
|  = , ==  | All common equality/inequality operators are applicable |
|  !=, <>  | to strings and are applied in a case sensitive manner.  |
|  <=, >=  | In the following example x, y and z are of type string. |
|  < , >   | (eg: not((x <= 'AbC') and ('1x2y3z' <> y)) or (z == x)  |
+----------+---------------------------------------------------------+
| in       | True only if x is a substring of y.                     |
|          | (eg: x in y or 'abc' in 'abcdefgh')                     |
+----------+---------------------------------------------------------+
| like     | True only if the string x matches the pattern y.        |
|          | Available wildcard characters are '*' and '?' denoting  |
|          | zero or more and zero or one matches respectively.      |
|          | (eg: x like y or 'abcdefgh' like 'a?d*h')               |
+----------+---------------------------------------------------------+
| ilike    | True only if the string x matches the pattern y in a    |
|          | case insensitive manner. Available wildcard characters  |
|          | are '*' and '?' denoting zero or more and zero or one   |
|          | matches respectively.                                   |
|          | (eg: x ilike y or 'a1B2c3D4e5F6g7H' ilike 'a?d*h')      |
+----------+---------------------------------------------------------+
| [r0:r1]  | The closed interval[r0,r1] of the specified string.     |
|          | eg: Given a string x with a value of 'abcdefgh' then:   |
|          | 1. x[1:4] == 'bcde'                                     |
|          | 2. x[ :4] == x[:8 / 2] == 'abcde'                       |
|          | 3. x[2 + 1: ] == x[3:] =='defgh'                        |
|          | 4. x[ : ] == x[:] == 'abcdefgh'                         |
|          | 5. x[4/2:3+1] == x[2:4] == 'cde'                        |
|          |                                                         |
|          | Note: Both r0 and r1 are assumed to be integers, where  |
|          | r0 <= r1. They may also be the result of an expression, |
|          | in the event they have fractional components truncation |
|          | shall be performed. (eg: 1.67 --> 1)                    |
+----------+---------------------------------------------------------+
|  :=      | Assign the value of x to y. Where y is a mutable string |
|          | or string range and x is either a string or a string    |
|          | range. eg:                                              |
|          | 1. y := x                                               |
|          | 2. y := 'abc'                                           |
|          | 3. y := x[:i + j]                                       |
|          | 4. y := '0123456789'[2:7]                               |
|          | 5. y := '0123456789'[2i + 1:7]                          |
|          | 6. y := (x := '0123456789'[2:7])                        |
|          | 7. y[i:j] := x                                          |
|          | 8. y[i:j] := (x + 'abcdefg'[8 / 4:5])[m:n]              |
|          |                                                         |
|          | Note: For options 7 and 8 the shorter of the two ranges |
|          | will denote the number characters that are to be copied.|
+----------+---------------------------------------------------------+
|  +       | Concatenation of x and y. Where x and y are strings or  |
|          | string ranges. eg                                       |
|          | 1. x + y                                                |
|          | 2. x + 'abc'                                            |
|          | 3. x + y[:i + j]                                        |
|          | 4. x[i:j] + y[2:3] + '0123456789'[2:7]                  |
|          | 5. 'abc' + x + y                                        |
|          | 6. 'abc' + '1234567'                                    |
|          | 7. (x + 'a1B2c3D4' + y)[i:2j]                           |
+----------+---------------------------------------------------------+
|  +=      | Append to x the value of y. Where x is a mutable string |
|          | and y is either a string or a string range. eg:         |
|          | 1. x += y                                               |
|          | 2. x += 'abc'                                           |
|          | 3. x += y[:i + j] + 'abc'                               |
|          | 4. x += '0123456789'[2:7]                               |
+----------+---------------------------------------------------------+
| <=>      | Swap the values of x and y. Where x and y are mutable   |
|          | strings.  (eg: x <=> y)                                 |
+----------+---------------------------------------------------------+
| []       | The string size operator returns the size of the string |
|          | being actioned.                                         |
|          | eg:                                                     |
|          | 1. 'abc'[] == 3                                         |
|          | 2. var max_str_length := max(s0[], s1[], s2[], s3[])    |
|          | 3. ('abc' + 'd')[] == 6                                 |
|          | 4. (('abc' + 'xyz')[1:4])[] == 4                        |
+----------+---------------------------------------------------------+

(6) Control Structures
+----------+---------------------------------------------------------+
|STRUCTURE | DEFINITION                                              |
+----------+---------------------------------------------------------+
| if       | If x is true then return y else return z.               |
|          | eg:                                                     |
|          | 1. if (x, y, z)                                         |
|          | 2. if ((x + 1) > 2y, z + 1, w / v)                      |
|          | 3. if (x > y) z;                                        |
|          | 4. if (x <= 2*y) { z + w };                             |
+----------+---------------------------------------------------------+
| if-else  | The if-else/else-if statement. Subject to the condition |
|          | branch the statement will return either the value of the|
|          | consequent or the alternative branch.                   |
|          | eg:                                                     |
|          | 1. if (x > y) z; else w;                                |
|          | 2. if (x > y) z; else if (w != u) v;                    |
|          | 3. if (x < y) { z; w + 1; } else u;                     |
|          | 4. if ((x != y) and (z > w))                            |
|          |    {                                                    |
|          |      y := sin(x) / u;                                   |
|          |      z := w + 1;                                        |
|          |    }                                                    |
|          |    else if (x > (z + 1))                                |
|          |    {                                                    |
|          |      w := abs (x - y) + z;                              |
|          |      u := (x + 1) > 2y ? 2u : 3u;                       |
|          |    }                                                    |
+----------+---------------------------------------------------------+
| switch   | The first true case condition that is encountered will  |
|          | determine the result of the switch. If none of the case |
|          | conditions hold true, the default action is assumed as  |
|          | the final return value. This is sometimes also known as |
|          | a multi-way branch mechanism.                           |
|          | eg:                                                     |
|          | switch                                                  |
|          | {                                                       |
|          |    case x > (y + z) : 2 * x / abs(y - z);               |
|          |    case x < 3       : sin(x + y);                       |
|          |    default          : 1 + x;                            |
|          | }                                                       |
+----------+---------------------------------------------------------+
| while    | The structure will repeatedly evaluate the internal     |
|          | statement(s) 'while' the condition is true. The final   |
|          | statement in the final iteration shall be used as the   |
|          | return value of the loop.                               |
|          | eg:                                                     |
|          | while ((x -= 1) > 0)                                    |
|          | {                                                       |
|          |    y := x + z;                                          |
|          |    w := u + y;                                          |
|          | }                                                       |
+----------+---------------------------------------------------------+
| repeat/  | The structure will repeatedly evaluate the internal     |
| until    | statement(s) 'until' the condition is true. The final   |
|          | statement in the final iteration shall be used as the   |
|          | return value of the loop.                               |
|          | eg:                                                     |
|          | repeat                                                  |
|          |    y := x + z;                                          |
|          |    w := u + y;                                          |
|          | until ((x += 1) > 100)                                  |
+----------+---------------------------------------------------------+
| for      | The structure will repeatedly evaluate the internal     |
|          | statement(s) while the condition is true. On each loop  |
|          | iteration, an 'incrementing' expression is evaluated.   |
|          | The conditional is mandatory whereas the initialiser    |
|          | and incrementing expressions are optional.              |
|          | eg:                                                     |
|          | for (var x := 0; (x < n) and (x != y); x += 1)          |
|          | {                                                       |
|          |    y := y + x / 2 - z;                                  |
|          |    w := u + y;                                          |
|          | }                                                       |
+----------+---------------------------------------------------------+
| break    | Break terminates the execution of the nearest enclosed  |
| break[]  | loop, allowing for the execution to continue on external|
|          | to the loop. The default break statement will set the   |
|          | return value of the loop to NaN, where as the return    |
|          | based form will set the value to that of the break      |
|          | expression.                                             |
|          | eg:                                                     |
|          | while ((i += 1) < 10)                                   |
|          | {                                                       |
|          |    if (i < 5)                                           |
|          |       j -= i + 2;                                       |
|          |    else if (i % 2 == 0)                                 |
|          |       break;                                            |
|          |    else                                                 |
|          |       break[2i + 3];                                    |
|          | }                                                       |
+----------+---------------------------------------------------------+
| continue | Continue results in the remaining portion of the nearest|
|          | enclosing loop body to be skipped.                      |
|          | eg:                                                     |
|          | for (var i := 0; i < 10; i += 1)                        |
|          | {                                                       |
|          |    if (i < 5)                                           |
|          |       continue;                                         |
|          |    j -= i + 2;                                          |
|          | }                                                       |
+----------+---------------------------------------------------------+
| return   | Return immediately from within the current expression.  |
|          | With the option of passing back a variable number of    |
|          | values (scalar, vector or string). eg:                  |
|          | 1. return [1];                                          |
|          | 2. return [x, 'abx'];                                   |
|          | 3. return [x, x + y,'abx'];                             |
|          | 4. return [];                                           |
|          | 5. if (x < y)                                           |
|          |     return [x, x - y, 'result-set1', 123.456];          |
|          |    else                                                 |
|          |     return [y, x + y, 'result-set2'];                   |
+----------+---------------------------------------------------------+
| ?:       | Ternary conditional statement, similar to that of the   |
|          | above denoted if-statement.                             |
|          | eg:                                                     |
|          | 1. x ? y : z                                            |
|          | 2. x + 1 > 2y ? z + 1 : (w / v)                         |
|          | 3. min(x,y) > z ? (x < y + 1) ? x : y : (w * v)         |
+----------+---------------------------------------------------------+
| ~        | Evaluate each sub-expression, then return as the result |
|          | the value of the last sub-expression. This is sometimes |
|          | known as multiple sequence point evaluation.            |
|          | eg:                                                     |
|          | ~(i := x + 1, j := y / z, k := sin(w/u)) == (sin(w/u))) |
|          | ~{i := x + 1; j := y / z; k := sin(w/u)} == (sin(w/u))) |
+----------+---------------------------------------------------------+
| [*]      | Evaluate any consequent for which its case statement is |
|          | true. The return value will be either zero or the result|
|          | of the last consequent to have been evaluated.          |
|          | eg:                                                     |
|          | [*]                                                     |
|          | {                                                       |
|          |   case (x + 1) > (y - 2)    : x := z / 2 + sin(y / pi); |
|          |   case (x + 2) < abs(y + 3) : w / 4 + min(5y,9);        |
|          |   case (x + 3) == (y * 4)   : y := abs(z / 6) + 7y;     |
|          | }                                                       |
+----------+---------------------------------------------------------+
| []       | The vector size operator returns the size of the vector |
|          | being actioned.                                         |
|          | eg:                                                     |
|          | 1. v[]                                                  |
|          | 2. max_size := max(v0[],v1[],v2[],v3[])                 |
+----------+---------------------------------------------------------+

Note01: In  the  tables above, the symbols x, y, z, w, u  and v  where
appropriate may represent any of one the following:

   1. Literal numeric/string value
   2. A variable
   3. A vector element
   4. A vector
   5. A string
   6. An expression comprised of [1], [2] or [3] (eg: 2 + x / vec[3])

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 09 - FUNDAMENTAL TYPES]
ExprTk supports three fundamental types which can be used freely in
expressions. The types are as follows:

   (1) Scalar
   (2) Vector
   (3) String


(1) Scalar Type
The scalar type  is a singular  numeric value. The  underlying type is
that used  to specialise  the ExprTk  components (float,  double, long
double, MPFR et al).


(2) Vector Type
The vector type is a fixed size sequence of contiguous scalar  values.
A  vector  can be  indexed  resulting in  a  scalar value.  Operations
between a vector and scalar will result in a vector with a size  equal
to that  of the  original vector,  whereas operations  between vectors
will result in a  vector of size equal  to that of the  smaller of the
two. In both mentioned cases, the operations will occur element-wise.


(3) String Type
The string type is a variable length sequence of 8-bit chars.  Strings
can be  assigned and  concatenated to  one another,  they can  also be
manipulated via sub-ranges using the range definition syntax.  Strings
however can not interact with scalar or vector types.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 10 - COMPONENTS]
There are three primary components, that are specialised upon a  given
numeric type, which make up the core of ExprTk. The components are  as
follows:

   (1) Symbol Table  exprtk::symbol_table<NumericType>
   (2) Expression    exprtk::expression<NumericType>
   (3) Parser        exprtk::parser<NumericType>


(1) Symbol Table
A structure that is used  to store references to variables,  constants
and functions that are to  be used within expressions. Furthermore  in
the context  of composited  recursive functions  the symbol  table can
also be thought of as a simple representation of a stack specific  for
the expression(s) that reference it. The following is a list of the
types a symbol table can handle:

   (a) Numeric variables
   (b) Numeric constants
   (c) Numeric vector elements
   (d) String variables
   (e) String constants
   (f) Functions
   (g) Vararg functions


During the compilation  process if an  expression is found  to require
any  of  the  elements   noted  above,  the  expression's   associated
symbol_table  will  be  queried  for  the  element  and  if  present a
reference to the element will be embedded within the expression's AST.
This allows for the original  element to be modified independently  of
the  expression  instance  and  to also  allow  the  expression  to be
evaluated using the current value of the element.

The  example  below demonstrates  the  relationship between variables,
symbol_table and expression. Note  the variables are modified  as they
normally would in a program, and when the expression is  evaluated the
current values assigned to the variables shall be used.

   typedef exprtk::symbol_table<double> symbol_table_t;
   typedef exprtk::expression<double>   expression_t;
   typedef exprtk::parser<double>       parser_t;

   double x = 0;
   double y = 0;

   symbol_table_t symbol_table;
   expression_t   expression;
   parser_t       parser;

   std::string expression_string = "x * y + 3";

   symbol_table.add_variable("x",x);
   symbol_table.add_variable("y",y);

   expression.register_symbol_table(symbol_table);

   parser.compile(expression_string,expression);

   x = 1.0;
   y = 2.0;
   expression.value(); // 1 * 2 + 3

   x = 3.7;
   expression.value(); // 3.7 * 2 + 3

   y = -9.0;
   expression.value(); // 3.7 * -9 + 3

   // 'x * -9 + 3' for x in range of [0,100) in steps of 0.0001
   for (x = 0.0; x < 100.0; x += 0.0001)
   {
      expression.value(); // x * -9 + 3
   }


Note02: Any  variable  reference  provided  to  a  given  symbol_table
instance, must have a lifetime at least as long as the lifetime of the
symbol_table  instance.  In  the  event  the  variable  reference   is
invalidated  before  the  symbol_table  or  any  dependent  expression
instances  have  been  destructed,  then  any  associated   expression
evaluations or variable referencing via the symbol_table instance will
result in undefined behaviour.

The following bit of  code instantiates a symbol_table  and expression
instance,  then  proceeds  to   demonstrate  various  ways  in   which
references to  variables can  be added  to the  symbol_table, and  how
those  references are  subsequently invalidated  resulting in  various
forms of undefined behaviour.

   typedef exprtk::symbol_table<double> symbol_table_t;
   typedef exprtk::expression<double>   expression_t;

   symbol_table_t symbol_table;
   expression_t   expression;

   std::deque<double > y {1.1, 2.2, 3.3};
   std::vector<double> z {4.4, 5.5, 6.6};
   double* w = new double(123.456);

   {
      double x = 123.4567;
      symbol_table.add_variable("x", x);
   }             // Reference to variable x has been invalidated


   symbol_table.add_variable("y", y.back());

   y.pop_back(); // Reference to variable y has been invalidated


   symbol_table.add_variable("z", z.front());

   z.erase(z.begin());
                 // Reference to variable z has been invalidated

   symbol_table.add_variable("w", *w);

   delete w;     // Reference to variable w has been invalidated

   const std::string expression_string = "x + y / z * w";

                 // Compilation of expression will succeed
   parser.compile(expression_string,expression);

   expression.value();
                 // Evaluation will result in undefined behaviour
                 // due to 'x' and 'w' having been destroyed.

   symbol_table.get_variable("x")->ref() = 135.791;
                 // Assignment will result in undefined behaviour


A compiled expression that references variables from a symbol_table is
dependent on  that symbol_table  instance and  the variables  it holds
being valid.

   typedef exprtk::symbol_table<double> symbol_table_t;
   typedef exprtk::expression<double>   expression_t;

   symbol_table_t symbol_table;
   expression_t   expression;

   double x = 123.456;

   symbol_table.add_variable("x", x);

   const std::string expression_string = "(x + 1) / 2";

                 // Compilation of the expression will succeed
   parser.compile(expression_string,expression);

                 // Clear all variables from symbol_table
   symbol_table.clear();

   expression.value();
                 // Evaluation will result in undefined behaviour
                 // because the reference to 'x' having been destroyed
                 // during the clearing of the symbol_table


In  the  above  example, an  expression  is  compiled that  references
variable "x". As part of the compilation process the node holding  the
variable "x" is obtained from the symbol_table and embedded in the AST
of the  expression -  in short  the expression  is now referencing the
node that holds  the variable "x".  The following diagram  depicts the
dependencies between  the  variable   x,  the  symbol  table  and  the
expression:

      +--[Symbol Table]--+
      |                  |
      | +- ------+       |
      | | x-node |       |
      | +-----A--+       |    +--[Expression]--+
      +---|---|----------+    |  +---------+   |
          v   |               |  |  A.S.T  |   |
          |   +--------<--------[.]        |   |
    +-----+                   |  +---------+   |
    |                         +----------------+
  +-v-[variable]---+
  | x: 123.456     |
  +----------------+


When the  clear method  is called  on the  symbol table  the X-Node is
destroyed, so now the expression  is referencing a node that  has been
destroyed.  From  this point  onwards  any attempts  to  reference the
expression instance will result in undefined behaviour. Simply put the
above  example  violates  the requirement  that  the  lifetime of  any
objects referenced by  expressions should exceed  the lifetime of  the
expression instance.

   typedef exprtk::symbol_table<double> symbol_table_t;
   typedef exprtk::expression<double>   expression_t;

   symbol_table_t symbol_table;
   expression_t   expression;

   double x = 123.456;

   symbol_table.add_variable("x", x);

   const std::string expression_string = "(x + 1) / 2";

                 // Compilation of the expression will succeed
   parser.compile(expression_string,expression);

   expression.value();

                 // Release the expression and its dependents
   expression.release();

                 // Clear all variables from symbol_table
   symbol_table.clear();

   expression.value();
                 // Will return null_node value of NaN


In the above example the expression is released before the  associated
symbol_table is cleared of its variables, which resolves the undefined
behaviour issue noted in the previous example.

Note03: It  is possible  to  register  multiple  symbol_tables  with a
single  expression object.  In the  event an  expression has  multiple
symbol tables, and where  there exists conflicts between  symbols, the
compilation stage  will resolve  the conflicts  based on  the order of
registration  of  the  symbol_tables to  the  expression.  For a  more
expansive discussion please review section [17 - Hierarchies Of Symbol
Tables]

   typedef exprtk::symbol_table<double> symbol_table_t;
   typedef exprtk::expression<double>   expression_t;
   typedef exprtk::parser<double>       parser_t;

   symbol_table_t symbol_table0;
   symbol_table_t symbol_table1;

   expression_t   expression;
   parser_t       parser;

   double x0 = 123.0;
   double x1 = 678.0;

   std::string expression_string = "x + 1";

   symbol_table0.add_variable("x",x0);
   symbol_table1.add_variable("x",x1);

   expression.register_symbol_table(symbol_table0);
   expression.register_symbol_table(symbol_table1);

   parser.compile(expression_string,expression);

   expression.value(); // 123 + 1


The symbol table supports  adding references to external  instances of
types  that  can  be accessed  within  expressions  via the  following
methods:

  1. bool add_variable    (const std::string& name, scalar_t&        )
  2. bool add_constant    (const std::string& name, const scalar_t&  )
  3. bool add_stringvar   (const std::string& name, std::string&     )
  4. bool add_vector      (const std::string& name, vector_type&     )
  5. bool add_function    (const std::string& name, function_t&      )
  6. bool create_stringvar(const std::string& name,const std::string&)
  7. bool create_variable (const std::string& name, const T&         )


Note04: The 'vector' type must be comprised from a contiguous array of
scalars with a size that is  larger than zero. The vector type  itself
can be any one of the following:

   1. std::vector<scalar_t>
   2. scalar_t(&v)[N]
   3. scalar_t* and array size
   4. exprtk::vector_view<scalar_t>


When  registering  a variable,  vector,  string or  function  with  an
instance of a symbol_table, the call to 'add_...' may fail and  return
a false result due to one or more of the following reasons:

  1. Variable name contains invalid characters or is ill-formed
  2. Variable name conflicts with a reserved word (eg: 'while')
  3. Variable name conflicts with a previously registered variable
  4. A vector of size (length) zero is being registered
  5. A free function exceeding fifteen parameters is being registered
  6. The symbol_table instance is in an invalid state


Note05: The symbol_table has a method called clear, which when invoked
will clear  all variables,  vectors, strings  and functions registered
with the symbol_table instance. If  this method is to be  called, then
one  must  make  sure  that  all  compiled  expression  instances that
reference  variables  belonging  to  that  symbol_table  instance  are
released (aka call  release method on  expression) before calling  the
clear  method  on  the  symbol_table  instance,  otherwise   undefined
behaviours will occur.

A further property of symbol tables is that they can be classified  at
instantiation as either being mutable (by default) or immutable.  This
property determines if variables,  vectors or strings registered  with
the symbol  table can  undergo modifications  within expressions  that
reference  them.  The   following  demonstrates  construction   of  an
immutable symbol table instance:

  symbol_table_t immutable_symbol_table
     (symbol_table_t::symtab_mutability_type::e_immutable);


When a symbol table, that has been constructed as  being immutable, is
registered with an expression, any statements in the expression string
that modify  the variables  that are  managed by  the immutable symbol
table will result in a compilation error. The operations that  trigger
the mutability constraint are the following assignment operators:

  1. Assignment:       :=
  2. Assign operation: +=, -=, *=, /= , %=

   const std::string expression_str = "x += x + 123.456";

   symbol_table_t immutable_symbol_table
      (symbol_table_t::symtab_mutability_type::e_immutable);

   T x = 0.0;

   immutable_symbol_table.add_variable("x" , x);

   expression_t expression;
   expression.register_symbol_table(immutable_symbol_table);

   parser_t parser;

   parser.compile(expression_str, expression);
      // Compile error because of assignment to variable x


In the above example, variable x is registered to an immutable  symbol
table,  making it  an immutable  variable within  the context  of any
expressions that  reference it.  The expression  string being compiled
uses the addition assignment operator  which will modify the value  of
variable x.  The compilation  process detects  this semantic violation
and proceeds to halt compilation and return the appropriate error.

One of the main reasons for  this functionality is that, one may  want
the immutability  properties that  come with  constness of  a variable
such  as  scalars,  vectors  and  strings,  but  not  necessarily  the
accompanying  compile  time  const-folding  optimisations,  that would
result in  the value  of the  variables being  retrieved only  once at
compile time, causing external updates to the variables to not be part
of the expression evaluation.

   symbol_table_t immutable_symbol_table
      (symbol_table_t::symtab_mutability_type::e_immutable);

   T x = 0.0;

   const std::string expression_str = "x + (y + y)";

   immutable_symbol_table.add_variable("x" , x    );
   immutable_symbol_table.add_constant("y" , 123.0);

   expression_t expression;
   expression.register_symbol_table(immutable_symbol_table);

   parser_t parser;
   parser.compile(expression_str, expression);

   for (; x < 10.0; ++x)
   {
      const auto expected_value = x + (123.0 + 123.0);
      const auto result_value   = expression.value();
      assert(expression.value() !=  expected_value);
   }


In the above example,  there are two variables  X and Y. Where  Y is a
constant and X is a normal variable. Both are registered with a symbol
table that is immutable. The  expression when compiled will result  in
the "(y + y)" part being  const-folded at compile time to the  literal
value of 246. Whereas  the current value of  X, being updated via  the
for-loop, externally to  the expression and  the symbol table shall be
observable to the expression upon each evaluation.


(2) Expression
A structure that holds an Abstract Syntax Tree or AST for a  specified
expression and is used to evaluate said expression. Evaluation of  the
expression is accomplished by performing a post-order traversal of the
AST.  If  a  compiled  Expression  uses  variables  or  user   defined
functions, it will have an associated Symbol Table, which will contain
references  to said  variables, functions  or strings. An example  AST
structure for the denoted expression is as follows:

Expression:  z := (x + y^-2.345) * sin(pi / min(w - 7.3,v))

                  [Root]
                    |
               [Assignment]
        ________/        \_____
       /                       \
 Variable(z)            [Multiplication]
                ____________/      \___________
               /                               \
              /                      [Unary-Function(sin)]
         [Addition]                            |
      ____/      \____                    [Division]
     /                \                 ___/      \___
 Variable(x)   [Exponentiation]        /              \
              ______/   \______  Constant(pi) [Binary-Function(min)]
             /                 \                ____/    \___
        Variable(y)        [Negation]          /             \
                               |              /          Variable(v)
                        Constant(2.345)      /
                                            /
                                     [Subtraction]
                                   ____/      \____
                                  /                \
                             Variable(w)      Constant(7.3)


The above denoted AST shall be evaluated in the following order:

   (01) Load Variable  (z)        (10) Load Constant  (7.3)
   (02) Load Variable  (x)        (11) Subtraction    (09 & 10)
   (03) Load Variable  (y)        (12) Load Variable  (v)
   (04) Load Constant  (2.345)    (13) Min            (11 & 12)
   (05) Negation       (04)       (14) Division       (08 & 13)
   (06) Exponentiation (03 & 05)  (15) Sin            (14)
   (07) Addition       (02 & 06)  (16) Multiplication (07 & 15)
   (08) Load Constant  (pi)       (17) Assignment     (01 & 16)
   (09) Load Variable  (w)


Generally an expression in ExprTk can be thought of as a free function
similar to those  found in imperative  languages. This form  of pseudo
function will have a name, it may have a set of one or more inputs and
will return at least one value as its result. Furthermore the function
when invoked, may  cause a side-effect  that changes the  state of the
host program.

As an  example the  following is  a pseudo-code  definition of  a free
function that performs a computation taking four inputs, modifying one
of them and returning a value based on some arbitrary calculation:

   ResultType foo(InputType x, InputType y, InputType z, InputType w)
   {
      w = 2 * x^y + z;        // Side-Effect
      return abs(x - y) / z;  // Return Result
   }


Given the above definition the following is a functionally  equivalent
version using ExprTk:

   const std::string foo_str =
      " w := 2 * x^y + z; "
      " abs(x - y) / z;   ";

   T x, y, z, w;

   symbol_table_t symbol_table;
   symbol_table.add_variable("x",x);
   symbol_table.add_variable("y",y);
   symbol_table.add_variable("z",z);
   symbol_table.add_variable("w",w);

   expression_t foo;
   foo.register_symbol_table(symbol_table);

   parser_t parser;
   if (!parser.compile(foo_str,foo))
   {
      // Error in expression...
      return;
   }

   T result = foo.value();


(3) Parser
A  component  which  takes  as input  a  string  representation  of an
expression and attempts to compile said input with the result being an
instance  of  Expression.  If  an  error  is  encountered  during  the
compilation  process, the  parser will  stop compiling  and return  an
error status code,  with a more  detailed description of  the error(s)
and  its  location  within  the  input  provided  by  the  'get_error'
interface.

Note06: The exprtk::expression and exprtk::symbol_table components are
reference counted entities. Copy constructing or assigning to or  from
either component will result in  a shallow copy and a  reference count
increment,  rather  than  a  complete  replication.  Furthermore   the
expression  and symbol_table  components being  Default-Constructible,
Copy-Constructible  and  Copy-Assignable  make  them  compatible  with
various  C++  standard  library   containers  and  adaptors  such   as
std::vector, std::map, std::stack etc.

The following is  an example of  two unique expressions,  after having
been instantiated and  compiled,  one expression  is  assigned to  the
other. The diagrams depict  their initial and post  assignment states,
including  which  control  block each  expression references and their
associated reference counts.


    exprtk::expression e0; // constructed expression, eg: x + 1
    exprtk::expression e1; // constructed expression, eg: 2z + y

  +-----[ e0 cntrl block]----+     +-----[ e1 cntrl block]-----+
  | 1. Expression Node 'x+1' |     | 1. Expression Node '2z+y' |
  | 2. Ref Count: 1          |<-+  | 2. Ref Count: 1           |<-+
  +--------------------------+  |  +---------------------------+  |
                                |                                 |
    +--[ e0 expression]--+      |    +--[ e1 expression]--+       |
    | 1. Reference to    ]------+    | 1. Reference to    ]-------+
    | e0 Control Block   |           | e1 Control Block   |
    +--------------------+           +--------------------+


   e0 = e1; // e0 and e1 are now 2z+y

               +-----[ e1 cntrl block]-----+
               | 1. Expression Node '2z+y' |
  +----------->| 2. Ref Count: 2           |<----------+
  |            +---------------------------+           |
  |                                                    |
  |   +--[ e0 expression]--+  +--[ e1 expression]--+   |
  +---[ 1. Reference to    |  | 1. Reference to    ]---+
      | e1 Control Block   |  | e1 Control Block   |
      +--------------------+  +--------------------+

The reason for  the above complexity  and restrictions of  deep copies
for the expression and symbol_table components is because  expressions
may include user defined variables or functions. These are embedded as
references into the expression's AST. When copying an expression, said
references  need to  also  be  copied. If  the references  are blindly
copied,  it  will then  result  in two  or more  identical expressions
utilising the exact same  references for variables. This  obviously is
not the  default assumed  scenario and  will give  rise to non-obvious
behaviours  when  using the  expressions in  various contexts such  as
multi-threading et al.

The prescribed method for cloning an expression is to compile it  from
its string  form. Doing so will allow the 'user' to  properly consider
the exact source of user defined variables and functions.

Note07: The exprtk::parser  is  a  non-copyable  and  non-thread  safe
component, and should only be shared via either a reference, a  shared
pointer  or  a  std::ref  mechanism,  and  considerations  relating to
synchronisation  taken  into  account  where  appropriate.  The parser
represents an object factory,  specifically a factory of  expressions,
and generally should  not be instantiated  solely on a  per expression
compilation basis.

The  following  diagram  and  example depicts  the  flow  of  data and
operations  for  compiling  multiple expressions  via  the  parser and
inserting  the  newly  minted  exprtk::expression  instances  into   a
std::vector.

                      +----[exprtk::parser]---+
                      |   Expression Factory  |
                      | parser_t::compile(...)|
                    +--> ~.~.~.~.~.~.~.~.~.~ ->--+
                    | +-----------------------+  |
 Expressions in     |                            |  Expressions as
 string form        ^                            V  exprtk::expression
                    |                            |  instances
 [s0:'x+1']--->--+  |                            |  +-[e0: x+1]
                 |  |                            |  |
 [s1:'2z+y']-->--+--+                            +->+-[e1: 2z+y]
                 |                                  |
 [s2:'sin(k+w)']-+                                  +-[e2: sin(k+w)]


   const std::string expression_str[3] =
      {
         "x + 1",
         "2x + y",
         "sin(k + w)"
      };

   std::vector<expression_t> expression_list;

   parser_t       parser;
   expression_t   expression;
   symbol_table_t symbol_table;

   expression.register_symbol_table(symbol_table);

   for (std::size_t i = 0; i < 3; ++i)
   {
      if (parser.compile(expression_str[i],expression))
      {
         expression_list.push_back(expression);
      }
      else
        std::cout << "Error in " << expression_str[i] << "\n";
   }

   for (auto& e : expression_list)
   {
      e.value();
   }

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 11 - COMPILATION OPTIONS]
The exprtk::parser  when being  instantiated takes  as input  a set of
options  to be  used during  the compilation  process of  expressions.
An  example instantiation  of exprtk::parser  where only  the  joiner,
commutative and strength reduction options are enabled is as  follows:

   typedef exprtk::parser<NumericType>::settings_t settings_t;

   const std::size_t compile_options =
                        settings_t::e_joiner            +
                        settings_t::e_commutative_check +
                        settings_t::e_strength_reduction;

   parser_t parser(compile_options);


Currently  eight  types of  compile  time options  are  supported, and
enabled by default. The options and their explanations are as follows:

   (1) Replacer
   (2) Joiner
   (3) Numeric Check
   (4) Bracket Check
   (5) Sequence Check
   (6) Commutative Check
   (7) Strength Reduction Check
   (8) Stack And Node Depth Check
   (9) Vector Size Check


(1) Replacer (e_replacer)
Enable replacement of specific  tokens with other tokens.  For example
the token  "true" of  type symbol  shall be  replaced with the numeric
token of value one.

   (a) (x < y) == true   --->  (x < y) == 1
   (b) false == (x > y)  --->  0 == (x > y)


(2) Joiner (e_joiner)
Enable  joining  of  multi-character  operators  that  may  have  been
incorrectly  disjoint in the  string  representation  of the specified
expression. For example the consecutive tokens of ">" "=" will  become
">=" representing  the "greater  than or  equal to"  operator. If  not
properly resolved the  original form will  cause a compilation  error.
The  following  is  a listing  of the  scenarios that  the joiner  can
handle:

   (a) '>' '='  --->  '>='  (gte)
   (b) '<' '='  --->  '<='  (lte)
   (c) '=' '='  --->  '=='  (equal)
   (d) '!' '='  --->  '!='  (not-equal)
   (e) '<' '>'  --->  '<>'  (not-equal)
   (f) ':' '='  --->  ':='  (assignment)
   (g) '+' '='  --->  '+='  (addition assignment)
   (h) '-' '='  --->  '-='  (subtraction assignment)
   (i) '*' '='  --->  '*='  (multiplication assignment)
   (j) '/' '='  --->  '/='  (division assignment)
   (k) '%' '='  --->  '%='  (modulo assignment)
   (l) '+' '-'  --->  '-'   (subtraction)
   (m) '-' '+'  --->  '-'   (subtraction)
   (n) '-' '-'  --->  '+'   (addition)
   (o) '<=' '>' --->  '<=>' (swap)


An example of the transformation that takes place is as follows:

   (a) (x > = y) and (z ! = w)  --->  (x >= y) and (z != w)


(3) Numeric Check (e_numeric_check)
Enable validation of tokens representing numeric types so as to  catch
any errors prior  to the costly  process of the  main compilation step
commencing.


(4) Bracket Check (e_bracket_check)
Enable  the  check for  validating  the ordering  of  brackets in  the
specified expression.


(5) Sequence Check (e_sequence_check)
Enable the  check for  validating that  sequences of  either pairs  or
triplets of tokens make sense.  For example the following sequence  of
tokens when encountered will raise an error:

   (a) (x + * 3)  --->  sequence error


(6) Commutative Check (e_commutative_check)
Enable the check that will transform sequences of pairs of tokens that
imply a multiplication operation.  The following are some  examples of
such transformations:

   (a) 2x             --->  2 * x
   (b) 25x^3          --->  25 * x^3
   (c) 3(x + 1)       --->  3 * (x + 1)
   (d) (x + 1)4       --->  (x + 1) * 4
   (e) 5foo(x,y)      --->  5 * foo(x,y)
   (f) foo(x,y)6 + 1  --->  foo(x,y) * 6 + 1
   (g) (4((2x)3))     --->  4 * ((2 * x) * 3)
   (h) w / (x - y)z   --->  w / (x - y) * z


(7) Strength Reduction Check (e_strength_reduction)
Enable  the  use  of  strength  reduction  optimisations  during   the
compilation  process.  In  ExprTk  strength  reduction   optimisations
predominantly involve  transforming sub-expressions  into other  forms
that  are algebraically  equivalent yet  less costly  to compute.  The
following are examples of the various transformations that can occur:

   (a) (x / y) / z               --->  x / (y * z)
   (b) (x / y) / (z / w)         --->  (x * w) / (y * z)
   (c) (2 * x) - (2 * y)         --->  2 * (x - y)
   (d) (2 / x) / (3 / y)         --->  (2 / 3) / (x * y)
   (e) (2 * x) * (3 * y)         --->  6 * (x * y)
   (f) (2 * x) * (2 - 4 / 2)     --->  0
   (g) (3 - 6 / 2) / (2 * x)     --->  0
   (h) avg(x,y,z) * (2 - 4 / 2)  --->  0


Note08: When using strength reduction in conjunction with  expressions
whose inputs or sub-expressions may result in values nearing either of
the bounds of the underlying  numeric type (eg: double), there  may be
the  possibility of a decrease in the precision of results.

In  the following  example the  given expression  which represents  an
attempt at computing the average  between x and y will  be transformed
as follows:

   (0.5 * x) + (y * 0.5) ---> 0.5 * (x + y)

There  may be  situations where  the above  transformation will  cause
numerical overflows and  that the original  form of the  expression is
desired over the strength reduced form. In these situations it is best
to turn off strength reduction optimisations  or to use a type with  a
larger numerical bound.


(8) Stack And Node Depth Check
ExprTk  incorporates   a  recursive   descent  parser.   When  parsing
expressions comprising inner sub-expressions, the recursive nature  of
the parsing process causes the stack to grow. If the expression causes
the stack to grow  beyond the stack size  limit, this would lead  to a
stackoverflow  and  its  associated  stack  corruption  and   security
vulnerability issues.

Similarly to parsing, evaluating an expression may cause the stack  to
grow. Such things like user defined functions, composite functions and
the general nature of the AST  being evaluated can cause the stack  to
grow,  and may  result in  potential stackoverflow  issues as  denoted
above.

ExprTk provides a set of checks that prevent both of the above denoted
problems at  compile time.  These checks rely on  two specific  limits
being set on the parser settings instance, these limits are:

   1. max_stack_depth (default: 400  )
   2. max_node_depth  (default: 10000)


The following demonstrates how these two parser parameters can be set:

   parser_t parser;

   parser.settings().set_max_stack_depth(100);
   parser.settings().set_max_node_depth(200);


In  the  above  code, during  parsing if  the stack  depth reaches  or
exceeds  100 levels,  the parsing  process will  immediately halt  and
return with a failure.  Similarly, during synthesizing the  AST nodes,
if the  compilation process  detects an  AST tree  depth exceeding 200
levels the parsing process will halt and return a parsing failure.


(9) Vector Size Check
When defining an  expression local vector,  ExprTk uses a  default max
vector size of  two billion elements.  One may want  to limit the  max
vector size to be either smaller or larger than the specified  default
value. The max size value can be changed via the parser settings.

   parser_t parser;

   parser.settings().set_max_local_vector_size(1000000);

   std::string expression1 = "var v[1e6] := [123]";
   std::string expression2 = "var v[1e9] := [123]";

   expression_t expression;

   parser.compile(expression1, expression); // compilation success
   parser.compile(expression2, expression); // compilation error


In the above  code, the max  local vector size  is set to  one million
elements. During  compilation  of an expression  if there is  a vector
definition  where  the  vector  size exceeds  the  max  vector  size a
compilation error shall be emitted.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 12 - EXPRESSION STRUCTURES]
Exprtk supports mathematical expressions in numerous forms based on  a
simple  imperative  programming  model. This  section  will  cover the
following  topics  related  to general  structure  and  programming of
expressions using ExprTk:

   (1) Multi-Statement Expressions
   (2) Statements And Side-Effects
   (3) Conditional Statements
   (4) Special Functions


(1) Multi-Statement Expressions
Expressions in ExprTk can be comprised of one or more statements, which
may  sometimes  be  called  sub-expressions.  The  following  are  two
examples of expressions stored  in std::string variables, the  first a
single statement and the second a multi-statement expression:

   std::string single_statement = " z := x + y ";

   std::string multi_statement  = " var temp := x; "
                                  " x := y + z;    "
                                  " y := temp;     ";


In a  multi-statement expression,  the final  statement will determine
the overall result of the expression. In the following multi-statement
expression, the result of the expression when evaluated will be '2.3',
which will also be the value stored in the 'y' variable.

   z := x + y;
   y := 2.3;


As  demonstrated  in  the  expression  above,  statements  within   an
expression are  separated using  the semi-colon  ';' operator.  In the
event  two  statements are  not  separated by  a  semi-colon, and  the
implied multiplication  feature is  active (enabled  by default),  the
compiler  will  assume  a  multiplication  operation  between  the two
statements.

In the following example we have a multi-statement expression composed
of two variable definitions and initialisations for variables x and  y
and two seemingly separate mathematical operations.

   var x:= 2;
   var y:= 3;
   x + 1
   y * 2


However the result of  the expression will not  be 6 as may  have been
assumed based on  the calculation of  'y * 2',  but rather the  result
will be 8. This  is because the compiler  will have conjoined the  two
mathematical statements into one  via a multiplication operation.  The
expression when compiled will actually evaluate as the following:

   var x:= 2;
   var y:= 3;
   x + 1 * y * 2;   // 2 + 1 * 3 * 2 == 8


In ExprTk any valid statement  will itself return a value.  This value
can  further  be  used  in  conjunction  with  other  statements. This
includes language structures such as if-statements, loops (for, while)
and the switch statement. Typically the last statement executed in the
given construct  (conditional, loop  etc), will  be the  value that is
returned.

In the following example, the  return value of the expression  will be
11, which is the sum of the variable 'x' and the final value  computed
within the loop body upon its last iteration:

   var x := 1;
   x + for (var i := x; i < 10; i += 1)
       {
          i / 2;
          i + 1;
       }


(2) Statements And Side-Effects
Statements themselves may have side effects, which in-turn affect  the
proceeding statements in multi-statement expressions.

A statement is said  to have a side-effect  if it causes the  state of
the  expression to  change in  some way  -  this  includes but  is not
limited to the  modification of the  state of external  variables used
within the expression. Currently  the following actions being  present
in a statement will cause it to have a side-effect:

   (a) Assignment operation (explicit or potentially)
   (b) Invoking a user-defined function that has side-effects

The following are examples of expressions where the side-effect status
of the  statements (sub-expressions) within the expressions  have been
noted:

  +-+----------------------+------------------------------+
  |#|      Expression      |      Side Effect Status      |
  +-+----------------------+------------------------------+
  |0| x + y                | False                        |
  +-+----------------------+------------------------------+
  |1| z := x + y           | True - Due to assignment     |
  +-+----------------------+------------------------------+
  |2| abs(x - y)           | False                        |
  +-+----------------------+------------------------------+
  |3| abs(x - y);          | False                        |
  | | z := (x += y);       | True - Due to assignments    |
  +-+----------------------+------------------------------+
  |4| abs(x - y);          | False                        |
  | | z := (x += y);       | True - Due to assignments    |
  +-+----------------------+------------------------------+
  |5| var t := abs(x - y); | True - Due to initialisation |
  | | t + x;               | False                        |
  | | z := (x += y);       | True - Due to assignments    |
  +-+----------------------+------------------------------+
  |6| foo(x - y)           | True - user defined function |
  +-+----------------------+------------------------------+


Note09: In example  6 from  the above  set, it  is  assumed  the  user
defined function foo has been  registered as having a side-effect.  By
default all user defined  functions are assumed to  have side-effects,
unless they  are configured  in their  constructors to  not have side-
effects using the  'disable_has_side_effects' free function.  For more
information review Section 15  - User Defined Functions  sub-section 7
Function Side-Effects.

At this point we  can see that there  will be expressions composed  of
certain kinds  of statements  that when  executed will  not affect the
nature  of the  expression's  result.  These statements  are typically
called 'dead code'.  These statements though  not affecting the  final
result will still be executed and as such they will consume processing
time that could otherwise be saved. As such ExprTk attempts to  detect
and remove such statements from expressions.

The  'Dead  Code  Elimination' (DCE)  optimisation  process,  which is
enabled by default, will remove any statements that are determined  to
not have a side-effect in a multi-statement expression, excluding  the
final or last statement.

By default the final statement in an expression will always be present
regardless of  its side-effect  status, as  it is  the statement whose
value shall be used as the result of the expression.

In order to further explain the actions taken during the DCE  process,
lets review the following expression:

   var x := 2;      // Statement 1
   var y := x + 2;  // Statement 2
   x + y;           // Statement 3
   y := x + 3y;     // Statement 4
   x - y;           // Statement 5


The above expression has five statements.  Three of them (1, 2 and  4)
actively have side-effects. The first two are variable declaration and
initialisations, where as the third is due to an assignment operation.
There  are  two  statements (3  and 5),  that do  not explicitly  have
side-effects, however the latter, statement 5, is the final  statement
in the expression and hence will be assumed to have a side-effect.

During compilation when the DCE  optimisation is applied to the  above
expression, statement 3 will be removed from the expression, as it has
no  bearing  on  the  final result  of  expression,  the  rest of  the
statements will all remain. The optimised form of the expression is as
follows:

   var x := 2;      // Statement 1
   var y := x + 2;  // Statement 2
   y := x + 3y;     // Statement 3
   x - y;           // Statement 4


(3) Conditional Statements (If-Then-Else)
ExprTk supports two forms  of conditional branching or otherwise known
as  if-statements.  The  first  form,  is  a  simple  function   based
conditional  statement, that  takes exactly  three input  expressions:
condition, consequent  and alternative.  The following  is an  example
expression that utilises the function based if-statement.

   x := if (y < z, y + 1, 2 * z)


In the  example above,  if the  condition 'y  < z'  is true,  then the
consequent 'y + 1' will be evaluated, its value shall be  returned and
subsequently assigned to the  variable 'x'. Otherwise the  alternative
'2 *  z' will  be evaluated  and its  value will  be returned. This is
essentially the  simplest form of  an if-then-else statement. A simple
variation of  the expression  where the  value of  the if-statement is
used within another statement is as follows:

   x := 3 * if (y < z, y + 1, 2 * z) / 2


The second form of if-statement resembles the standard syntax found in
most imperative languages. There are two variations of the statement:

   (a) If-Statement
   (b) If-Then-Else Statement


(a) If-Statement
This version  of the  conditional statement  returns the  value of the
consequent expression when the  condition expression is true,  else it
will return a quiet NaN value as its result.

   Example 1:
   x := if (y < z) y + 3;

   Example 2:
   x := if (y < z)
        {
           y + 3
        };

The two  example expressions  above are  equivalent. If  the condition
'y < z' is true, the 'x' variable shall be  assigned the value of  the
consequent 'y + 3', otherwise it  will be assigned the value of  quiet
NaN.  As  previously  discussed,  if-statements  are  value  returning
constructs, and if  not properly terminated  using a semi-colon,  will
end-up  combining  with  the  next  statement  via  a   multiplication
operation. The following example will NOT result in the expected value
of 'w + x' being returned:

   x := if (y < z) y + 3  // missing semi-colon ';'
   w + x


When the  above supposed  multi-statement expression  is compiled, the
expression  will  have  a  multiplication  inserted  between  the  two
'intended' statements resulting in the unanticipated expression:

   x := (if (y < z) y + 3) * w + x


The  solution  to  the  above situation  is  to  simply  terminate the
conditional statement with a semi-colon as follows:

   x := if (y < z) y + 3;
   w + x


(b) If-Then-Else Statement
The second variation of  the if-statement is to  allow for the use  of
Else and Else-If cascading statements. Examples of such statements are
as follows:

 Example 1:             Example 2:         Example 3:
 if (x < y)             if (x < y)         if (x > y + 1)
    z := x + 3;         {                     y := abs(x - z);
 else                      y := z + x;     else
    y := x - z;            z := x + 3;     {
                        }                     y := z + x;
                        else                  z := x + 3;
                           y := x - z;     };


 Example 4:             Example 5:         Example 6:
 if (2 * x < max(y,3))  if (x < y)         if (x < y or (x + z) > y)
 {                         z := x + 3;     {
    y := z + x;         else if (2y != z)     z := x + 3;
    z := x + 3;         {                     y := x - z;
 }                         z := x + 3;     }
 else if (2y - z)          y := x - z;     else if (abs(2y - z) >= 3)
    y := x - z;         }                     y := x - z;
                        else               else
                           x * x;          {
                                              z := abs(x * x);
                                              x * y * z;
                                           };


In  the case  where  there  is no  final else  statement and  the flow
through the conditional  arrives at this  final point, the  same rules
apply to this form of if-statement as to the previous. That is a quiet
NaN shall be returned as the  result of the  if-statement. Furthermore
the same requirements of  terminating the statement with  a semi-colon
apply.

(4) Special Functions
The purpose  of special  functions in  ExprTk is  to provide  compiler
generated equivalents of common mathematical expressions which can  be
invoked by  using the  'special function'  syntax (eg:  $f12(x,y,z) or
$f82(x,y,z,w)).

Special functions dramatically decrease  the total evaluation time  of
expressions which would otherwise  have been written using  the common
form by reducing the total number  of nodes in the evaluation tree  of
an  expression  and  by  also  leveraging  the  compiler's  ability to
correctly optimise such expressions for a given architecture.

          3-Parameter                       4-Parameter
 +-------------+-------------+    +--------------+------------------+
 |  Prototype  |  Operation  |    |  Prototype   |    Operation     |
 +-------------+-------------+    +--------------+------------------+
   $f00(x,y,z) | (x + y) / z       $f48(x,y,z,w) | x + ((y + z) / w)
   $f01(x,y,z) | (x + y) * z       $f49(x,y,z,w) | x + ((y + z) * w)
   $f02(x,y,z) | (x + y) - z       $f50(x,y,z,w) | x + ((y - z) / w)
   $f03(x,y,z) | (x + y) + z       $f51(x,y,z,w) | x + ((y - z) * w)
   $f04(x,y,z) | (x - y) + z       $f52(x,y,z,w) | x + ((y * z) / w)
   $f05(x,y,z) | (x - y) / z       $f53(x,y,z,w) | x + ((y * z) * w)
   $f06(x,y,z) | (x - y) * z       $f54(x,y,z,w) | x + ((y / z) + w)
   $f07(x,y,z) | (x * y) + z       $f55(x,y,z,w) | x + ((y / z) / w)
   $f08(x,y,z) | (x * y) - z       $f56(x,y,z,w) | x + ((y / z) * w)
   $f09(x,y,z) | (x * y) / z       $f57(x,y,z,w) | x - ((y + z) / w)
   $f10(x,y,z) | (x * y) * z       $f58(x,y,z,w) | x - ((y + z) * w)
   $f11(x,y,z) | (x / y) + z       $f59(x,y,z,w) | x - ((y - z) / w)
   $f12(x,y,z) | (x / y) - z       $f60(x,y,z,w) | x - ((y - z) * w)
   $f13(x,y,z) | (x / y) / z       $f61(x,y,z,w) | x - ((y * z) / w)
   $f14(x,y,z) | (x / y) * z       $f62(x,y,z,w) | x - ((y * z) * w)
   $f15(x,y,z) | x / (y + z)       $f63(x,y,z,w) | x - ((y / z) / w)
   $f16(x,y,z) | x / (y - z)       $f64(x,y,z,w) | x - ((y / z) * w)
   $f17(x,y,z) | x / (y * z)       $f65(x,y,z,w) | ((x + y) * z) - w
   $f18(x,y,z) | x / (y / z)       $f66(x,y,z,w) | ((x - y) * z) - w
   $f19(x,y,z) | x * (y + z)       $f67(x,y,z,w) | ((x * y) * z) - w
   $f20(x,y,z) | x * (y - z)       $f68(x,y,z,w) | ((x / y) * z) - w
   $f21(x,y,z) | x * (y * z)       $f69(x,y,z,w) | ((x + y) / z) - w
   $f22(x,y,z) | x * (y / z)       $f70(x,y,z,w) | ((x - y) / z) - w
   $f23(x,y,z) | x - (y + z)       $f71(x,y,z,w) | ((x * y) / z) - w
   $f24(x,y,z) | x - (y - z)       $f72(x,y,z,w) | ((x / y) / z) - w
   $f25(x,y,z) | x - (y / z)       $f73(x,y,z,w) | (x * y) + (z * w)
   $f26(x,y,z) | x - (y * z)       $f74(x,y,z,w) | (x * y) - (z * w)
   $f27(x,y,z) | x + (y * z)       $f75(x,y,z,w) | (x * y) + (z / w)
   $f28(x,y,z) | x + (y / z)       $f76(x,y,z,w) | (x * y) - (z / w)
   $f29(x,y,z) | x + (y + z)       $f77(x,y,z,w) | (x / y) + (z / w)
   $f30(x,y,z) | x + (y - z)       $f78(x,y,z,w) | (x / y) - (z / w)
   $f31(x,y,z) | x * y^2 + z       $f79(x,y,z,w) | (x / y) - (z * w)
   $f32(x,y,z) | x * y^3 + z       $f80(x,y,z,w) | x / (y + (z * w))
   $f33(x,y,z) | x * y^4 + z       $f81(x,y,z,w) | x / (y - (z * w))
   $f34(x,y,z) | x * y^5 + z       $f82(x,y,z,w) | x * (y + (z * w))
   $f35(x,y,z) | x * y^6 + z       $f83(x,y,z,w) | x * (y - (z * w))
   $f36(x,y,z) | x * y^7 + z       $f84(x,y,z,w) | x*y^2 + z*w^2
   $f37(x,y,z) | x * y^8 + z       $f85(x,y,z,w) | x*y^3 + z*w^3
   $f38(x,y,z) | x * y^9 + z       $f86(x,y,z,w) | x*y^4 + z*w^4
   $f39(x,y,z) | x * log(y)+z      $f87(x,y,z,w) | x*y^5 + z*w^5
   $f40(x,y,z) | x * log(y)-z      $f88(x,y,z,w) | x*y^6 + z*w^6
   $f41(x,y,z) | x * log10(y)+z    $f89(x,y,z,w) | x*y^7 + z*w^7
   $f42(x,y,z) | x * log10(y)-z    $f90(x,y,z,w) | x*y^8 + z*w^8
   $f43(x,y,z) | x * sin(y)+z      $f91(x,y,z,w) | x*y^9 + z*w^9
   $f44(x,y,z) | x * sin(y)-z      $f92(x,y,z,w) | (x and y) ? z : w
   $f45(x,y,z) | x * cos(y)+z      $f93(x,y,z,w) | (x or  y) ? z : w
   $f46(x,y,z) | x * cos(y)-z      $f94(x,y,z,w) | (x <   y) ? z : w
   $f47(x,y,z) | x ? y : z         $f95(x,y,z,w) | (x <=  y) ? z : w
                                   $f96(x,y,z,w) | (x >   y) ? z : w
                                   $f97(x,y,z,w) | (x >=  y) ? z : w
                                   $f98(x,y,z,w) | (x ==  y) ? z : w
                                   $f99(x,y,z,w) | x*sin(y)+z*cos(w)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 13 - VARIABLE, VECTOR & STRING DEFINITION]
ExprTk supports the definition of expression local variables,  vectors
and  strings.  The definitions  must  be unique  as  shadowing is  not
allowed and object lifetimes are based  on scope. Definitions use  the
following general form:

   var <name> := <initialiser>;

(1) Variable Definition
Variables are  of numeric  type denoting  a single  value. They can be
explicitly initialised to a value, otherwise they will be defaulted to
zero. The following are examples of variable definitions:

   (a) Initialise x to zero
       var x;

   (b) Initialise y to three
       var y := 3;

   (c) Initialise z to the expression
       var z := if (max(1, x + y) > 2, w, v);

   (d) Initialise const literal n
       var n := 12 / 3;


(2) Vector Definition
Vectors are arrays of a common numeric type. The elements in a  vector
can be explicitly initialised, otherwise they will all be defaulted to
zero. The following are examples of vector definitions:

   (a) Initialise all values to zero
       var x[3];

   (b) Initialise all values to zero
       var x[3] := {};

   (c) Initialise all values to given value or expression
       var x[3]   := [ 42 ];
       var y[x[]] := [ 123 + 3y + sin(w / z) ];

   (d) Initialise all values iota style
       var v[4] := [ 0 : +1];  //  0,  1,  2,  3
       var v[5] := [-3 : -2];  // -3, -5, -7, -9, -11

   (e) Initialise the first two values, all other elements to zero
       var x[3] := { (1 + x[2]) / x[], (sin(y[0] / x[]) + 3) / x[] };

   (f) Initialise the first three (all) values
       const var size := 3;
       var x[size] := { 1, 2, 3 };

   (g) Initialise vector from a vector
       var x[4] := { 1, 2, 3, 4 };
       var y[3] := x;
       var w[5] := { 1, 2 }; // 1, 2, 0, 0, 0

   (h) Initialise vector from a smaller vector
       var x[3] := { 1, 2, 3 };
       var y[5] := x;   // 1, 2, 3, ??, ??

   (i) Non-initialised vector
       var x[3] := null; // ?? ?? ??

   (j) Error as there are too many initialisers
       var x[3] := { 1, 2, 3, 4 };

   (k) Error as a vector of size zero is not allowed.
       var x[0];


(3) String Definition
Strings are sequences comprised of 8-bit characters. They can only be
defined  with an explicit  initialisation  value. The  following  are
examples of string variable definitions:

   (a) Initialise to a string
       var x := 'abc';

   (b) Initialise to an empty string
       var x := '';

   (c) Initialise to a string expression
       var x := 'abc' + '123';

   (d) Initialise to a string range
       var x := 'abc123'[2:4];

   (e) Initialise to another string variable
       var x := 'abc';
       var y := x;

   (f) Initialise to another string variable range
       var x := 'abc123';
       var y := x[2:4];

   (g) Initialise to a string expression
       var x := 'abc';
       var y := x + '123';

   (h) Initialise to a string expression range
       var x := 'abc';
       var y := (x + '123')[1:3];


(4) Return Value
Variable and vector  definitions have a  return value. In  the case of
variable definitions, the value  to which the variable  is initialised
will be returned. Where as for vectors, the value of the first element
(eg: v[0]) shall be returned.

   8 == ((var x := 7;) + 1)
   4 == (var y[3] := {4, 5, 6};)


(5) Variable/Vector Assignment
The value of a variable can be assigned to a vector and a vector or  a
vector expression can be assigned to a variable.

  (a) Variable To Vector:
      Every element of the vector is assigned the value of the variable
      or expression.
      var x    := 3;
      var y[3] := { 1, 2, 3 };
      y := x + 1;

  (b) Vector To Variable:
      The variable is assigned the value of the first element of the
      vector (aka vec[0])
      var x    := 3;
      var y[3] := { 1, 2, 3 };
      x := y + 1;


Note10: During the expression compilation phase, tokens are classified
based on the following priorities:

   (a) Reserved keywords or operators (+, -, and, or, etc)
   (b) Base functions (abs, sin, cos, min, max etc)
   (c) Symbol table variables
   (d) Expression local defined variables
   (e) Symbol table functions
   (f) Unknown symbol resolver based variables

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 14 - VECTOR PROCESSING]
ExprTk  provides  support  for   various  forms  of  vector   oriented
arithmetic, inequalities and  processing. The various  supported pairs
are as follows:

   (a) vector and vector (eg: v0 + v1)
   (b) vector and scalar (eg: v  + 33)
   (c) scalar and vector (eg: 22 *  v)

The following is a list of operations that can be used in  conjunction
with vectors:

   (a) Arithmetic:       +, -, *, /, %
   (b) Exponentiation:   vector ^ scalar
   (c) Assignment:       :=, +=, -=, *=, /=, %=, <=>
   (d) Inequalities:     <, <=, >, >=, ==, =, equal
   (e) Boolean logic:    and, nand, nor, or, xnor, xor
   (f) Unary operations:
       abs, acos, acosh, asin, asinh, atan, atanh, ceil, cos,  cosh,
       cot, csc,  deg2grad, deg2rad,  erf, erfc,  exp, expm1, floor,
       frac, grad2deg, log, log10, log1p, log2, rad2deg, round, sec,
       sgn, sin, sinc, sinh, sqrt, swap, tan, tanh, trunc,
       thresholding
   (g) Aggregate and Reduce operations:
       avg, max, min, mul,  dot, dotk, sum, sumk,  count, all_true,
       all_false, any_true, any_false
   (h) Transformation operations:
       copy, diff, reverse, rotate-left/right, shift-left/right, sort,
       nth_element
   (i) BLAS-L1:
       axpy, axpby, axpyz, axpbyz, axpbz

Note11: When one of the above  described operations is being performed
between two  vectors, the  operation will  only span  the size  of the
smallest vector.  The elements  of the  larger vector  outside of  the
range will  not be included. The  operation  itself will  be processed
element-wise over values of the smaller of the two ranges.

The  following  simple  example  demonstrates  the  vector  processing
capabilities by computing the dot-product of the vectors v0 and v1 and
then assigning it to the variable v0dotv1:

   var v0[3] := { 1, 2, 3 };
   var v1[3] := { 4, 5, 6 };
   var v0dotv1 := sum(v0 * v1);


The following is a for-loop based implementation that is equivalent to
the previously mentioned dot-product computation expression:

   var v0[3] := { 1, 2, 3 };
   var v1[3] := { 4, 5, 6 };
   var v0dotv1;

   for (var i := 0; i < min(v0[],v1[]); i += 1)
   {
      v0dotv1 += (v0[i] * v1[i]);
   }


Note12: When the aggregate or reduction  operations denoted above  are
used  in conjunction with a  vector or  vector  expression, the return
value is not a vector but rather a single value.

   var x[3] := { 1, 2, 3 };

   sum(x)      ==  6
   sum(1 + 2x) == 15
   avg(3x + 1) ==  7
   min(1 / x)  == (1 / 3)
   max(x / 2)  == (3 / 2)
   sum(x > 0 and x < 5) == x[]


When utilising external user defined  vectors via the symbol table  as
opposed to expression local defined vectors, the typical  'add_vector'
method from the symbol table will register the entirety of the  vector
that is passed. The following example attempts to evaluate the sum  of
elements of  the external  user defined  vector within  a typical  yet
trivial expression:

   const std::string reduce_program = " sum(2 * v + 1) ";

   std::vector<T> v0 { T(1.1), T(2.2), ..... , T(99.99) };

   symbol_table_t symbol_table;
   symbol_table.add_vector("v",v);

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   parser_t parser;
   parser.compile(reduce_program,expression);

   T sum = expression.value();


For the most part, this is  a very common use-case. However there  may
be situations where one may want to evaluate the same vector  oriented
expression many times over, but using different vectors or sub  ranges
of the same vector of the same size to that of the original upon every
evaluation.

The usual solution is to  either recompile the expression for  the new
vector instance, or to  copy the contents from  the new vector to  the
symbol table registered vector  and then perform the  evaluation. When
the  vectors are  large or  the re-evaluation  attempts are  numerous,
these  solutions  can  become  rather  time  consuming  and  generally
inefficient.

   std::vector<T> v1 { T(2.2), T(2.2), ..... , T(2.2) };
   std::vector<T> v2 { T(3.3), T(3.3), ..... , T(3.3) };
   std::vector<T> v3 { T(4.4), T(4.4), ..... , T(4.4) };

   std::vector<std::vector<T>> vv { v1, v2, v3 };
   ...
   T sum = T(0);

   for (auto& new_vec : vv)
   {
      v = new_vec; // update vector
      sum += expression.value();
   }


A  solution  to  the  above  'efficiency'  problem,  is  to  use   the
exprtk::vector_view  object. The  vector_view is  instantiated with  a
size and backing based upon a vector. Upon evaluations if the  backing
needs  to  be  'updated' to  either another  vector or  sub-range, the
vector_view instance  can be  efficiently rebased,  and the expression
evaluated as normal.

   exprtk::vector_view<T> view = exprtk::make_vector_view(v,v.size());

   symbol_table_t symbol_table;
   symbol_table.add_vector("v",view);

   ...

   T sum = T(0);

   for (auto& new_vec : vv)
   {
      view.rebase(new_vec.data()); // update vector
      sum += expression.value();
   }


Another useful feature of exprtk::vector_view is that all such vectors
can have  their sizes  modified (or  "resized"). The  resizing of  the
associated vectors can happen either between or during evaluations.

   std::vector<T> v = { 1, 2, 3, 4, 5, 6, 7, 8 };
   exprtk::vector_view<T> view = exprtk::make_vector_view(v,v.size());

   symbol_table_t symbol_table;
   symbol_table.add_vector("v",view);

   const std::string expression_string = "v[]";

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   parser_t parser;
   parser.compile(expression_string, expression);

   for (std::size_t i = 1; i <= v.size(); ++i)
   {
      vv.set_size(i);
      expression.value();
   }


In the example above, a vector_view is instantiated with a std::vector
instance with eight elements and registered to the given symbol_table.
An expression is then compiled,  which in this case simply returns the
size of the vector at that point in time. The expression is  evaluated
eight times (size of vector times), where upon each iteration the size
of the vector is changed with values ranging from one to eight.

Note13: When modifying the size of  a vector, the  new size must be at
least one  or larger  and must  not exceed  the original  size of  the
vector_view when it was instantiated.

Note14: The  lifetime  of  any  parser,  symbol_table   or  expression
instance must  not exceed  that of  any vector_view  instance that has
been registered  with it.  Furthermore the  lifetime of  a vector_view
must not exceed that of the underlying vector instance it is
associated with.

Note15: In a multi-threaded context the  rebase function should not be
called during associated expression  evaluation, as this will  lead to
undefined behaviour (eg: torn reads and writes).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 15 - USER DEFINED FUNCTIONS]
ExprTk provides a means  whereby custom functions can  be defined  and
utilised within  expressions.  The   concept  requires  the  user   to
provide a reference  to the function  coupled with an  associated name
that  will be invoked within  expressions. Functions may take numerous
inputs but will always return a single value of the underlying numeric
type.

During  expression  compilation  when required  the  reference  to the
function  shall be obtained from  the associated  symbol_table and  be
embedded into the expression.

There are five types of function interface:

  +---+----------------------+--------------+----------------------+
  | # |         Name         | Return Type  | Input Types          |
  +---+----------------------+--------------+----------------------+
  | 1 | ifunction            | Scalar       | Scalar               |
  | 2 | ivararg_function     | Scalar       | Scalar               |
  | 3 | igeneric_function    | Scalar       | Scalar,Vector,String |
  | 4 | igeneric_function II | String       | Scalar,Vector,String |
  | 5 | igeneric_function III| String/Scalar| Scalar,Vector,String |
  | 6 | function_compositor  | Scalar       | Scalar               |
  +---+----------------------+--------------+----------------------+

(1) ifunction
This interface supports zero to 20 input parameters of only the scalar
type (numbers). The  usage requires a custom function be  derived from
ifunction and to override one of the 21 function operators. As part of
the constructor the custom function will define how many parameters it
expects  to  handle.  The  following  example  defines  a  3 parameter
function called 'foo':

   template <typename T>
   struct foo final : public exprtk::ifunction<T>
   {
      foo() : exprtk::ifunction<T>(3)
      {}

      T operator()(const T& v1, const T& v2, const T& v3) override
      {
         return T(1) + (v1 * v2) / T(v3);
      }
   };


(2) ivararg_function
This interface supports a variable number of scalar arguments as input
into the function. The function operator interface uses a  std::vector
specialised upon type T to facilitate parameter passing. The following
example defines a vararg function called 'boo':

   template <typename T>
   struct boo final : public exprtk::ivararg_function<T>
   {
      inline T operator()(const std::vector<T>& arglist) override
      {
         T result = T(0);

         for (std::size_t i = 0; i < arglist.size(); ++i)
         {
            result += arglist[i] / arglist[i > 0 ? (i - 1) : 0];
         }

         return result;
      }
   };


(3) igeneric_function
This interface supports  a variable number  of arguments and  types as
input  into  the  function. The  function  operator  interface uses  a
std::vector  specialised  upon  the  type_store  type  to   facilitate
parameter passing.

    Scalar <-- function(i_0, i_1, i_2....., i_N)


The  fundamental  types  that  can  be  passed  into  the  function as
parameters and their views are as follows:

   (1) Scalar - scalar_view
   (2) Vector - vector_view
   (3) String - string_view


The above denoted type  views provide non-const reference-like  access
to each parameter, as such modifications made to the input  parameters
will  persist after  the function  call has  completed. The  following
example defines a generic function called 'too':

   template <typename T>
   struct too final : public exprtk::igeneric_function<T>
   {
      typedef typename exprtk::igeneric_function<T>::parameter_list_t
                                                     parameter_list_t;

      too()
      {}

      inline T operator()(parameter_list_t parameters) override
      {
         for (std::size_t i = 0; i < parameters.size(); ++i)
         {
            ...
         }

         return T(0);
      }
   };


In the example above, the input 'parameters' to the function operator,
parameter_list_t,  is  a  type  of  std::vector  of  type_store.  Each
type_store  instance  has  a  member  called  'type'  which  holds the
enumeration pertaining to the underlying type of the type_store. There
are three type enumerations:

   (1) e_scalar - literals, variables, vector elements, expressions
       eg: 123.456, x, vec[3x + 1], 2x + 3

   (2) e_vector - vectors, vector expressions
       eg: vec1, 2 * vec1 + vec2 / 3

   (3) e_string - strings, string literals and range variants of both
       eg: 'AString', s0, 'AString'[x:y], s1[1 + x:] + 'AString'


Each of the  parameters can be  accessed using its  designated view.
A typical loop for processing the parameters is as follows:

   inline T operator()(parameter_list_t parameters)
   {
      typedef typename exprtk::igeneric_function<T>::generic_type
                                                     generic_type;

      typedef typename generic_type::scalar_view scalar_t;
      typedef typename generic_type::vector_view vector_t;
      typedef typename generic_type::string_view string_t;

      for (std::size_t i = 0; i < parameters.size(); ++i)
      {
         generic_type& gt = parameters[i];

         if (generic_type::e_scalar == gt.type)
         {
            scalar_t x(gt);
            ...
         }
         else if (generic_type::e_vector == gt.type)
         {
            vector_t vector(gt);
            ...
         }
         else if (generic_type::e_string == gt.type)
         {
            string_t string(gt);
            ...
         }
      }

      return T(0);
   }


Most often than not a custom generic function will require a  specific
sequence of parameters, rather than some arbitrary sequence of  types.
In those situations, ExprTk can perform compile-time type checking  to
validate that function invocations  are carried out using  the correct
sequence of parameters. Furthermore  performing the checks at  compile
-time rather than at run-time (aka every time the function is invoked)
will result in expression evaluation performance gains.

Compile-time type  checking of  input parameters  can be  requested by
passing  a string  to the  constructor of  the igeneric_function  that
represents the required sequence of parameter types. When no parameter
sequence is provided, it is implied the function can accept a variable
number of parameters comprised of any of the fundamental types.

Each fundamental type has an  associated character. The following is a
listing of said characters and their meanings:

   (1) T - Scalar
   (2) V - Vector
   (3) S - String
   (4) Z - Zero or no parameters
   (5) ? - Any type (Scalar, Vector or String)
   (6) * - Wildcard operator
   (7) | - Parameter sequence delimiter


No other characters other than the seven denoted above may be included
in the parameter sequence  definition. If any such  invalid characters
do exist, registration of the associated generic function to a  symbol
table ('add_function' method) will fail. If the parameter sequence  is
modified resulting in it becoming  invalid after having been added  to
the symbol table but before the compilation step, a compilation  error
will be incurred.

The  following   example  demonstrates   a  simple   generic  function
implementation with a user specified parameter sequence:

   template <typename T>
   struct moo final : public exprtk::igeneric_function<T>
   {
      typedef typename exprtk::igeneric_function<T>::parameter_list_t
                                                     parameter_list_t;

      moo()
      : exprtk::igeneric_function<T>("SVTT")
      {}

      inline T operator()(parameter_list_t parameters) override
      {
         ...
      }
   };


In the example above the  generic function 'moo' expects exactly  four
parameters in the following sequence:

   (1) String
   (2) Vector
   (3) Scalar
   (4) Scalar

Note16: The 'Z' or no parameter option may not be used in  conjunction
with any other type option in a parameter sequence. When  incorporated
in the parameter  sequence list, the  'No Parameter' option  indicates
that the function may be invoked without any parameters being  passed.
For more information refer to the section: 'Zero Parameter Functions'


(4) igeneric_function II
This interface is identical to  the igeneric_function, in that in  can
consume an  arbitrary number  of parameters  of varying  type, but the
difference being  that the  function returns  a string  and as such is
treated as a string when  invoked within expressions. As a  result the
function call can  alias a string  and interact with  other strings in
situations such as concatenation and equality operations.

    String <-- function(i_0, i_1, i_2....., i_N)


The following example defines a generic function  named 'toupper' with
the string return type function operator being explicitly overridden:

   template <typename T>
   struct toupper final : public exprtk::igeneric_function<T>
   {
      typedef exprtk::igeneric_function<T> igenfunct_t;
      typedef typename igenfunct_t::generic_type generic_t;
      typedef typename igenfunct_t::parameter_list_t parameter_list_t;
      typedef typename generic_t::string_view string_t;

      toupper()
      : exprtk::igeneric_function<T>("S",igenfunct_t::e_rtrn_string)
      {}

      inline T operator()(std::string& result,
                          parameter_list_t parameters) override
      {
         result.clear();

         string_t string(parameters[0]);

         for (std::size_t i = 0; i < string.size(); ++i)
         {
            result += std::toupper(string[i]);
         }

         return T(0);
      }
   };


In the example above the  generic function 'toupper' expects only  one
input parameter  of type  string, as  noted by  the parameter sequence
string passed during the  constructor. Furthermore a second  parameter
is passed to the constructor indicating that it should be treated as a
string returning function -  by default it is  assumed to be a  scalar
returning function.

When executed,  the function  will return  as a  result a  copy of the
input string converted to uppercase form. An example expression  using
the toupper function registered as the symbol 'toupper' is as follows:

   "'ABCDEF' == toupper('aBc') + toupper('DeF')"


Note17: When  adding a  string type  returning generic  function to  a
symbol  table  the  'add_function'  is  invoked.  The  example   below
demonstrates how this can be done:

   toupper<T> tu;

   exprtk::symbol_table<T> symbol_table;

   symbol_table.add_function("toupper",tu);


Note18: Two further refinements to the  type checking facility are the
possibilities  of  a variable  number  of common  types  which can  be
accomplished by using a wildcard '*' and a special 'any type' which is
done using  the '?'  character. It  should be  noted that the wildcard
operator is  associated with  the previous  type in  the sequence  and
implies one or more of that type.

   template <typename T>
   struct zoo final : public exprtk::igeneric_function<T>
   {
      typedef typename exprtk::igeneric_function<T>::parameter_list_t
                                                     parameter_list_t;

      zoo()
      : exprtk::igeneric_function<T>("SVT*V?")
      {}

      inline T operator()(parameter_list_t parameters) override
      {
         ...
      }
   };


In the example above the generic function 'zoo' expects at least  five
parameters in the following sequence:

   (1) String
   (2) Vector
   (3) One or more Scalars
   (4) Vector
   (5) Any type (one type of either a scalar, vector or string)


A final  piece of  type checking  functionality is  available for  the
scenarios where  a single  function name  is intended  to be  used for
multiple distinct parameter sequences,  another name for this  feature
is function  overloading. The  parameter sequences  are passed  to the
constructor as a  single string delimited  by the pipe  '|' character.
Two specific overrides of the  function operator are provided one  for
standard generic functions and one for string returning functions. The
overrides are as follows:

      // Scalar <-- function(psi,i_0,i_1,....,i_N)
      inline T operator()(const std::size_t& ps_index,
                          parameter_list_t parameters)
      {
         ...
      }

      // String <-- function(psi,i_0,i_1,....,i_N)
      inline T operator()(const std::size_t& ps_index,
                          std::string& result,
                          parameter_list_t parameters)
      {
         ...
      }


When the function  operator is invoked  the 'ps_index' parameter  will
have as its value the index of the parameter sequence that matches the
specific invocation. This way complex and time consuming type checking
conditions need not  be executed in  the function itself  but rather a
simple and efficient  dispatch to a  specific implementation for  that
particular parameter sequence can be performed.

   template <typename T>
   struct roo final : public exprtk::igeneric_function<T>
   {
      typedef typename exprtk::igeneric_function<T>::parameter_list_t
                                                     parameter_list_t;

      moo()
      : exprtk::igeneric_function<T>("SVTT|SS|TTV|S?V*S")
      {}

      inline T operator()(const std::size_t& ps_index,
                          parameter_list_t parameters) override
      {
         ...
      }
   };


In the example above there are four distinct parameter sequences  that
can be processed  by the generic  function 'roo'. Any  other parameter
sequences will cause a compilation error. The four valid sequences are
as follows:

    Sequence-0    Sequence-1    Sequence-2    Sequence-3
      'SVTT'         'SS'          'TTV'       'S?V*S'
   (1) String    (1) String    (1) Scalar    (1) String
   (2) Vector    (2) String    (2) Scalar    (2) Any Type
   (3) Scalar                  (3) Vector    (3) One or more Vectors
   (4) Scalar                                (4) String


(5) igeneric_function III
In this section we will discuss an extension of the  igeneric_function
interface that will allow for the overloading of a user defined custom
function, where by it can return either a scalar or string value  type
depending on the input parameter  sequence with which the function  is
invoked.

   template <typename T>
   struct foo final : public exprtk::igeneric_function<T>
   {
      typedef typename exprtk::igeneric_function<T>::parameter_list_t
                                                     parameter_list_t;

      foo()
      : exprtk::igeneric_function<T>
        (
          "T:T|S:TS",
          igfun_t::e_rtrn_overload
        )
      {}

      // Scalar value returning invocations
      inline T operator()(const std::size_t& ps_index,
                          parameter_list_t parameters) override
      {
         ...
      }

      // String value returning invocations
      inline T operator()(const std::size_t& ps_index,
                          std::string& result,
                          parameter_list_t& parameters) override
      {
         ...
      }
   };


In the  example above  the custom  user defined  function "foo" can be
invoked by using  either one of  two input parameter  sequences, which
are defined as follows:

   Sequence-0    Sequence-1
   'T' -> T      'TS' -> S
   (1) Scalar    (1) Scalar
                 (2) String


The parameter  sequence definitions  are identical  to the  previously
defined igeneric_function, with the exception of the inclusion  of the
return type - which can only be either a scalar T or a string S.


(6) function_compositor
The function  compositor is  a factory  that allows  one to define and
construct a function using ExprTk syntax. The functions are limited to
returning a single scalar value and consuming up to six parameters  as
input.

All composited functions are registered with a symbol table,  allowing
them  to  call  other  functions  and  use  variables  that  have been
registered with the symbol  table instance. Furthermore the  functions
can be  recursive in  nature due  to the  inherent function  prototype
forwarding  that  occurs during  construction.  The following  example
defines,  by using  two different  methods, composited  functions and
implicitly registering the functions with the denoted symbol table.

   typedef exprtk::symbol_table<T>         symbol_table_t;
   typedef exprtk::function_compositor<T>  compositor_t;
   typedef typename compositor_t::function function_t;

   T avogadro = T(6.022e23);

   symbol_table_t symbol_table;

   symbol_table.add_constant("avogadro", avogadro);

   compositor_t compositor(symbol_table);

   // Define function koo0(v1, v2) { ... }
   compositor.add(
      function_t("koo0"),
      .vars("v1", "v2")
      .expression
      (
         " 1 + cos(v1 * v2) / avogadro; "
      ));

   // Define function koo1(x, y, z) { ... }
   compositor.add(
      function_t()
      .name("koo1")
      .var("x").var("y").var("z")
      .expression
      (
         "1 + koo0(x * y, 3) / z;"
      ));


A function compositor can also be instantiated without a symbol_table.
When this is the case an internal symbol_table is used for holding the
references to the composited functions.

   compositor_t compositor;

   // Define function koo2(v1, v2) { ... }
   compositor.add(
      function_t("koo2"),
      .vars("v1", "v2", "v3")
      .expression
      ( " abs(v1 * v2) / v3; " ));


When wanting to  reference functions from  the compositor above  in an
expression, the compositor's symbol_table  will need to be  registered
with the expression  prior to compilation,  as is demonstrated  in the
following code:

   expression_t expression;
   .
   .
   expression.register_symbol_table(compositor.symbol_table());


In the situation where  more than one symbol table's contents will  be
required by the functions  being composited, then those  symbol tables
can be registered as auxiliary symbol tables with the compositor:

   symbol_table_t global_symbol_table;
   symbol_table_t local_symbol_table;
   .
   .
   .
   compositor_t compositor;

   compositor.add_auxiliary_symtab(global_symbol_table);
   compositor.add_auxiliary_symtab(local_symbol_table );

Note19: In the event, that two or more symbol tables contain similarly
named  variables,  vectors,  strings   or  functions,  the  order   of
registration with the compositor shall determine the symbol table from
which the target symbol will be referenced.


(7) Using Functions In Expressions
For the above denoted custom and composited functions to be used in an
expression, an instance of each function needs to be registered with a
symbol_table that  has been  associated with  the expression instance.
The following demonstrates how all the pieces are put together:

   typedef exprtk::symbol_table<double>        symbol_table_t;
   typedef exprtk::expression<double>          expression_t;
   typedef exprtk::parser<double>              parser_t;
   typedef exprtk::function_compositor<double> compositor_t;
   typedef typename compositor_t::function     function_t;

   foo<double> f;
   boo<double> b;
   too<double> t;
   toupper<double> tu;

   symbol_table_t symbol_table;
   compositor_t   compositor(symbol_table);

   symbol_table.add_function("foo",f);
   symbol_table.add_function("boo",b);
   symbol_table.add_function("too",t);

   symbol_table
      .add_function("toupper", tu, symbol_table_t::e_ft_strfunc);

   compositor.add(
      function_t("koo")
      .var("v1")
      .var("v2")
      .expression
      (
         "1 + cos(v1 * v2) / 3;"
      ));

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   const std::string expression_str =
      " if (foo(1,2,3) + boo(1) > boo(1/2, 2/3, 3/4, 4/5)) "
      "    koo(3,4);                                       "
      " else                                               "
      "    too(2 * v1 + v2 / 3, 'abcdef'[2:4], 3.3);       "
      "                                                    ";

   parser_t parser;
   parser.compile(expression_str,expression);

   expression.value();


(8) Function Side-Effects
All function calls are assumed  to have side-effects by default.  This
assumption implicitly disables constant folding optimisations when all
parameters being passed to the function are deduced as being constants
at compile time.

If it is certain that the function being registered does not have  any
side-effects and can  be correctly constant folded  where appropriate,
then during the construction of the function the side-effect trait  of
the function can be disabled.

   template <typename T>
   struct foo final : public exprtk::ifunction<T>
   {
      foo() : exprtk::ifunction<T>(3)
      {
         exprtk::disable_has_side_effects(*this);
      }

      T operator()(const T& v1, const T& v2, const T& v3) override
      { ... }
   };


(9) Zero Parameter Functions
When  either  an  ifunction,  ivararg_function  or   igeneric_function
derived type is defined with zero number of parameters, there are  two
calling  conventions  within  expressions  that  are  allowed.  For  a
function named 'foo' with zero input parameters the calling styles are
as follows:

   (1)  x + sin(foo()- 2) / y
   (2)  x + sin(foo  - 2) / y


By default the  zero parameter trait  is disabled. In  order to enable
it, a process similar to that of  enabling of the side-effect trait is
carried out:

   template <typename T>
   struct foo final : public exprtk::ivararg_function<T>
   {
      foo()
      {
         exprtk::enable_zero_parameters(*this);
      }

      inline T operator()(const std::vector<T>& arglist) override
      { ... }
   };


Note20: For the igeneric_function  type, there  also needs to be a 'Z'
parameter sequence  defined in order for the  zero parameter  trait to
properly take effect otherwise a compilation error will occur.


(10) Free Functions
The ExprTk symbol  table supports the  registration of free  functions
and lambdas  (anonymous functors)  for use  in expressions.  The basic
requirements  are similar  to those  found in  ifunction derived  user
defined  functions. This  includes  support  for free  functions using
anywhere from zero up to fifteen input parameters of scalar type, with
a return type that is also scalar. Furthermore such functions will  by
default be assumed to have side-effects and hence will not participate
in constant folding optimisations.

In the following  example, a one  input parameter free  function named
'compute1',  a  two  input  parameter  template  free  function  named
'compute2' and a three input parameter lambda named 'compute3' will be
registered with the given symbol_table instance:

   double compute1(double v0)
   {
      return 2.0 * std::abs(v0);
   }

   template <typename T>
   T compute2(T v0, T v1)
   {
      return 2.0 * v0 + v1 / 3.0;
   }
   .
   .
   .

   typedef exprtk::symbol_table<double> symbol_table_t;

   symbol_table_t symbol_table;

   symbol_table.add_function("compute1", compute1);
   symbol_table.add_function("compute2", compute2<double>);

   symbol_table.add_function(
      "compute3",
      [](double v0, double v1, double v2) -> double
      { return v0 / v1 + v2; });


Note21: Similar to  variables registered with  symbol_table instances,
for any of the following function providers:

   1. ifunction
   2. ivararg_function
   3. igeneric_function
   4. function_compositor
   5. Free function
   7. Lambda


Their instance lifetimes must exceed the symbol_tables and expressions
they are  registered with.  In the  event that  is not  the case,  the
expected result shall be undefined behaviour.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 16 - EXPRESSION DEPENDENTS]
Any  expression  that  is  not  a  literal  (aka  constant)  will have
dependencies. The types of  'dependencies' an expression can  have are
as follows:

   (a) Variables
   (b) Vectors
   (c) Strings
   (d) Functions
   (e) Assignments


In  the  following  example the  denoted  expression  has its  various
dependencies listed:

   z := abs(x + sin(2 * pi / y))

   (a) Variables:   x, y, z and pi
   (b) Functions:   abs, sin
   (c) Assignments: z


ExprTk allows for  the derivation of  expression dependencies via  the
'dependent_entity_collector'  (DEC).  When  activated  either  through
'compile_options' at the construction  of the parser or  through calls
to enabler methods just prior to compilation, the DEC will proceed  to
collect any  of the  relevant types  that are  encountered during  the
parsing  phase.   Once  the   compilation  process   has  successfully
completed, the  caller can  then obtain  a list  of symbols  and their
associated types from the DEC.

The kinds of  questions one can  ask regarding the  dependent entities
within an expression are as follows:

  * What user defined variables, vectors or strings are used?
  * What functions or custom user functions are used?
  * Which variables, vectors or strings have values assigned to them?


The following example demonstrates usage of the DEC in determining the
dependents of the given expression:

   typedef typename parser_t::
      dependent_entity_collector::symbol_t symbol_t;

   const std::string expression_string =
      "z := abs(x + sin(2 * pi / y))";

   T x,y,z;

   parser_t parser;
   symbol_table_t symbol_table;

   symbol_table.add_variable("x",x);
   symbol_table.add_variable("y",y);
   symbol_table.add_variable("z",z);

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   // Collect only variable and function symbols
   parser.dec().collect_variables() = true;
   parser.dec().collect_functions() = true;

   if (!parser.compile(expression_string,expression))
   {
      // error....
   }

   std::deque<symbol_t> symbol_list;

   parser.dec().symbols(symbol_list);

   for (std::size_t i = 0; i < symbol_list.size(); ++i)
   {
      const symbol_t& symbol = symbol_list[i];

      switch (symbol.second)
      {
         case parser_t::e_st_variable : ... break;
         case parser_t::e_st_vector   : ... break;
         case parser_t::e_st_string   : ... break;
         case parser_t::e_st_function : ... break;
      }
   }


Note22: The 'symbol_t'  type is a  std::pair comprising of  the symbol
name (std::string) and the associated type of the symbol as denoted by
the cases in the switch statement.

Having  particular  symbols  (variable  or  function)  present  in  an
expression is one form of dependency. Another and just as  interesting
and important type of  dependency is that of  assignments. Assignments
are the set of dependent symbols that 'may' have their values modified
within an expression. The following are example expressions and  their
associated assignments:

       Assignments   Expression
   (1) x             x := y + z
   (2) x, y          x += y += z
   (3) x, y, z       x := y += sin(z := w + 2)
   (4) w, z          if (x > y, z := x + 2, w := 'A String')
   (5) None          x + y + z


Note23: In  expression 4,  both variables  'w' and  'z' are denoted as
being assignments even though only one of them can ever be modified at
the time of evaluation. Furthermore the determination of which of  the
two variables the modification will occur upon can only be known  with
certainty at evaluation time and not beforehand, hence both are listed
as being candidates for assignment.

The following builds upon the previous example demonstrating the usage
of the DEC in determining the 'assignments' of the given expression:

   // Collect assignments
   parser.dec().collect_assignments() = true;

   if (!parser.compile(expression_string,expression))
   {
      // error....
   }

   std::deque<symbol_t> symbol_list;

   parser.dec().assignment_symbols(symbol_list);

   for (std::size_t i = 0; i < symbol_list.size(); ++i)
   {
      symbol_t& symbol = symbol_list[i];

      switch (symbol.second)
      {
         case parser_t::e_st_variable : ... break;
         case parser_t::e_st_vector   : ... break;
         case parser_t::e_st_string   : ... break;
      }
   }


Note24: The  assignments will  only consist  of variable  types and as
such will not contain symbols denoting functions.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 17 - HIERARCHIES OF SYMBOL TABLES]
Most situations will only require a single symbol_table instance to be
associated with a given expression instance.

However as an expression can have more than one symbol table  instance
associated  with  itself,  when  building  more  complex  systems that
utilise many expressions  where each can  in turn utilise  one or more
variables  from  a  large set  of  potential  variables, functions  or
constants, it becomes evident  that grouping variables into  layers of
symbol_tables will simplify and streamline the overall process.

A recommended hierarchy of symbol tables is the following:

   (a) Global constant value symbol table
   (b) Global non side-effect functions symbol table
   (c) Global variable symbol table
   (d) Expression specific variable symbol table


(a) Global constant value symbol table
This symbol table will  contain constant variables denoting  immutable
values. These variables can be made available to all expressions,  and
in turn expressions  will assume the  values themselves will  never be
modified for the  duration of the  process run-time. Examples  of such
variables are:

   (1) pi or e
   (2) speed_of_light
   (3) avogadro_number
   (4) num_cpus


(b) Global non side-effect functions symbol table
This symbol table will contain  only user defined functions that  will
not  incur  any  side-effects  that  are  observable  to  any  of  the
expressions that invoke them. These functions shall be  thread-safe or
threading invariant and  will not maintain  any form of  state between
invocations. Examples of such functions are:

   (1) calc_volume_of_sphere(r)
   (2) distance(x0,y0,x1,y1)


(c) Global variable symbol table
This symbol table  will contain variables  that will be  accessible to
all associated expressions  and will not  be specific or  exclusive to
any one expression. This variant  differs from (a) in that  the values
of the  variables can  change (or  be updated)  between evaluations of
expressions  -   but  through   properly  scheduled   evaluations  are
guaranteed  to never  change during  the evaluation  of any  dependent
expressions. Furthermore it  is assumed that  these variables will  be
used in a  read-only context and  that no expressions  will attempt to
modify these variables via assignments or other means.

   (1) price_of_stock_xyz
   (2) outside_temperature or inside_temperature
   (3) fuel_in_tank
   (4) num_customers_in_store
   (5) num_items_on_shelf


(d) Expression specific variable symbol table
This  symbol_table  is  the most  common form,  and is  used to  store
variables that are specific and exclusive to a particular  expression.
That is to say references  to variables in this symbol_table  will not
be  part of  another expression.  Though it  may be  possible to  have
expressions that  contain the  variables with  the same  name, in that
case those variables will be distinctly different. Which would mean if
a particular  expression were  to be  compiled twice,  each expression
would have its  own unique symbol_table  which in turn  would have its
own instances of those variables. Examples of such variables could be:

   (1) x or y
   (2) customer_name


The following is a diagram depicting a possible variant of the denoted
symbol  table  hierarchies.  In  the  diagram  there  are  two  unique
expressions, each of  which have a  reference to the  Global constant,
functions and variables symbol tables and an exclusive reference to  a
local symbol table.

  +-------------------------+    +-------------------------+
  |     Global Constants    |    |     Global Functions    |
  |       Symbol Table      |    |       Symbol Table      |
  +----o--o-----------------+    +--------------------o----+
       |  |                                           |
       |  |                                           +-------+
       |  +------------------->----------------------------+  |
       |         +----------------------------+            |  |
       |         |      Global Variables      |            |  |
       |  +------o        Symbol Table        o-----+      |  V
       |  |      +----------------------------+     |      |  |
       |  |                                         |      |  |
       |  | +----------------+   +----------------+ |      |  |
       |  | | Symbol Table 0 |   | Symbol Table 1 | |      V  |
       |  | +--o-------------+   +--o-------------+ |      |  |
       |  |    |                    |               |      |  |
       |  |    |                    |               |      |  |
    +--V--V----V---------+        +-V---------------V--+   |  |
    |    Expression 0    |        |    Expression 1    |<--+--+
    |  '2 * sin(x) - y'  |        |  'k + abs(x - y)'  |
    +--------------------+        +--------------------+


Bringing  all of  the above  together, in  the following  example the
hierarchy  of  symbol  tables  are instantiated  and  initialised. An
expression that makes use of various elements of each symbol table is
then compiled and later on evaluated:

   typedef exprtk::symbol_table<double> symbol_table_t;
   typedef exprtk::expression<double>   expression_t;

   // Setup global constants symbol table
   symbol_table_t glbl_const_symbol_table;
   glbl_const_symbtab.add_constants(); // pi, epsilon and inf
   glbl_const_symbtab.add_constant("speed_of_light",299e6);
   glbl_const_symbtab.add_constant("avogadro_number",6e23);

   // Setup global function symbol table
   symbol_table_t glbl_funcs_symbol_table;
   glbl_func_symbtab.add_function('distance',distance);
   glbl_func_symbtab.add_function('calc_spherevol',calc_sphrvol);

   ......

   // Setup global variable symbol table
   symbol_table_t glbl_variable_symbol_table;
   glbl_variable_symbtab.add_variable('temp_outside',thermo.outside);
   glbl_variable_symbtab.add_variable('temp_inside' ,thermo.inside );
   glbl_variable_symbtab.add_variable('num_cstmrs',store.num_cstmrs);

   ......

   double x,y,z;

   // Setup expression specific symbol table
   symbol_table_t symbol_table;
   symbol_table.add_variable('x',x);
   symbol_table.add_variable('y',y);
   symbol_table.add_variable('z',z);

   expression_t expression;

   // Register the various symbol tables
   expression
      .register_symbol_table(symbol_table);

   expression
      .register_symbol_table(glbl_funcs_symbol_table);

   expression
      .register_symbol_table(glbl_const_symbol_table);

   expression
      .register_symbol_table(glbl_variable_symbol_table);

   const std::string expression_str =
      "abs(temp_inside - temp_outside) + 2 * speed_of_light / x";

   parser_t parser;
   parser.compile(expression_str,expression);

   ......

   while (keep_evaluating)
   {
     ....

     T result = expression.value();

     ....
   }

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 18 - UNKNOWN UNKNOWNS]
In this section  we will discuss  the process of  handling expressions
with a mix of known and unknown variables. Initially a discussion into
the types of expressions that exist will be provided, then a series of
possible solutions will be presented for each scenario.

When parsing an expression, there  may be situations where one  is not
fully  aware  of what  if  any variables  will  be used  prior  to the
expression being compiled.

This can become problematic, as in the default scenario it is  assumed
the symbol_table that is registered with the expression instance  will
already possess  the  externally  available  variables,  functions and
constants needed during the compilation of the expression.

In the event there are symbols in the expression that can't be  mapped
to   either   a  reserved   word,   or  located   in   the  associated
symbol_table(s), an "Undefined  symbol" error will  be raised and  the
compilation process will fail.

The numerous  scenarios that  can occur  when compiling  an expression
with ExprTk generally fall into one of the following three categories:

   (a) No external variables
   (b) Predetermined set of external variables
   (c) Unknown set of variables


(a) No external variables
These  are  expressions that  contain  no external  variables  but may
contain  local  variables.  As  local  variables  cannot  be  accessed
externally from the  expression, it is  assumed that such  expressions
will not have  a need for  a symbol_table and  furthermore expressions
which  don't  make  use of  functions that  have side-effects  will be
evaluated completely at  compile time resulting  in a constant  return
value. The following are examples of such expressions:

   (1) 1 + 2
   (2) var x := 3; 2 * x - 3
   (3) var x := 3; var y := abs(x - 8); x - y / 7


(b) Predetermined set of external variables
These  are  expressions  that are  comprised  of  externally available
variables  and functions  and will  only compile  successfully if  the
symbols that  correspond to  the variables  and functions  are already
defined in their associated symbol_table(s).  This is by far the  most
common scenario when using ExprTk.

As an example, one may have three external variables: x, y and z which
have been registered with  the associated symbol_table, and  will then
need to compile  and evaluate expressions  comprised of any  subset of
these  three  variables. The  following  are a  few  examples of  such
expressions:

   (1) 1 + x
   (2) x / y
   (3) 2 * x * y / z


In  this  scenario   one  can  use   the  'dependent_entity_collector'
component as described in [Section  16] to further determine which  of
the registered variables were  actually used in the  given expression.
As  an example  once the  set of  utilised  variables  are known,  any
further 'attention'  can be  restricted to  only those  variables when
evaluating the expression. This can be quite useful when dealing  with
expressions that can draw from a set of hundreds or even thousands  of
variables.


(c) Unknown set of variables
These are  expressions that  are comprised  of symbols  other than the
standard ExprTk reserved words or what has been registered with  their
associated symbol_table, and will normally fail compilation due to the
associated symbol_table not having a  reference to them. As such  this
scenario can be  seen as a  combination of scenario  B, where one  may
have a symbol_table with registered variables, but would also like  to
handle  the  situation  of  variables  that  aren't  present  in  said
symbol_table.

When dealing with expressions of category (c), one must perform all of
the following:

   (1) Determine the variables used in the expression
   (2) Populate a symbol_table(s) with the entities from (1)
   (3) Compile the expression
   (4) Provide a means by which the entities from (1) can be modified


Depending on the nature of processing,  steps (1) and (2) can be  done
either independently of each other or combined into one. The following
example  will  initially  look  at  solving  the  problem  of  unknown
variables with the  latter method using  the 'unknown_symbol_resolver'
component.

   typedef exprtk::symbol_table<T> symbol_table_t;
   typedef exprtk::expression<T>   expression_t;
   typedef exprtk::parser<T>       parser_t;

   T x = T(123.456);
   T y = T(789.123);

   symbol_table_t unknown_var_symbol_table;

   symbol_table_t symbol_table;
   symbol_table.add_variable("x",x);
   symbol_table.add_variable("y",y);

   expression_t expression;
   expression.register_symbol_table(unknown_var_symbol_table);
   expression.register_symbol_table(symbol_table);

   parser_t parser;
   parser.enable_unknown_symbol_resolver();

   const std::string expression_str = "x + abs(y / 3k) * z + 2";

   parser.compile(expression_str,expression);


In the  example above,  the symbols  'k' and  'z' will  be treated  as
unknown symbols. The  parser in the  example is set  to handle unknown
symbols using the built-in default unknown_symbol_resolver (USR).  The
default  USR  will  automatically resolve  any  unknown  symbols as  a
variable (scalar type). The new variables will be added to the primary
symbol_table,  which in  this case  is the  'unknown_var_symbol_table'
instance.  Once  the  compilation  has  completed  successfully,   the
variables that were resolved  during compilation can be  accessed from
the   primary   symbol_table   using   the   'get_variable_list'   and
'variable_ref' methods and then if needed can be modified  accordingly
after which the expression itself can be evaluated.

   std::vector<std::string> variable_list;

   unknown_var_symbol_table.get_variable_list(variable_list);

   for (const auto& var_name : variable_list)
   {
      T& v = unknown_var_symbol_table.variable_ref(var_name);

      v = ...;
   }

   ...

   expression.value();


Note25: As previously  mentioned the  default  USR  will automatically
assume any unknown symbol to be a valid scalar variable, and will then
proceed to add said symbol  as a variable to the  primary symbol_table
of the associated expression during the compilation process. However a
problem that may arise, is  that expressions that are parsed  with the
USR enabled,  but contain  'typos' or  otherwise syntactic  errors may
inadvertently compile successfully due to the simplistic nature of the
default USR. The following are some example expressions:

   (1) 1 + abz(x + 1)
   (2) sine(y / 2) - coz(3x)


The two  expressions above contain misspelt  symbols (abz,  sine, coz)
which if implied  multiplications and default  USR are enabled  during
compilation will result in them being assumed to be valid 'variables',
which obviously is  not the intended  outcome by the  user. A possible
solution to this  problem is for  one to implement  their own specific
USR that will perform a user defined business logic in determining  if
an encountered unknown symbol should be treated as a variable or if it
should raise a compilation error. The following example demonstrates a
simple user defined USR:

   typedef exprtk::symbol_table<T> symbol_table_t;
   typedef exprtk::expression<T>   expression_t;
   typedef exprtk::parser<T>       parser_t;

   template <typename T>
   struct my_usr final : public parser_t::unknown_symbol_resolver
   {
      typedef typename parser_t::unknown_symbol_resolver usr_t;

      bool process(const std::string& unknown_symbol,
                   typename usr_t::usr_symbol_type& st,
                   T& default_value,
                   std::string& error_message) override
      {
         if (0 != unknown_symbol.find("var_"))
         {
            error_message = "Invalid symbol: " + unknown_symbol;
            return false;
         }

         st = usr_t::e_usr_variable_type;
         default_value = T(123.123);

         return true;
      }
   };

   ...

   T x = T(123.456);
   T y = T(789.123);

   symbol_table_t unknown_var_symbol_table;

   symbol_table_t symbol_table;
   symbol_table.add_variable("x",x);
   symbol_table.add_variable("y",y);

   expression_t expression;
   expression.register_symbol_table(unknown_var_symbol_table);
   expression.register_symbol_table(symbol_table);

   my_usr<T> musr;

   parser_t parser;
   parser.enable_unknown_symbol_resolver(&musr);

   std::string expression_str = "var_x + abs(var_y - 3) * var_z";

   parser.compile(expression_str,expression);


In  the  example  above,  a user  specified  USR  is  defined, and  is
registered   with   the  parser   enabling   the  USR   functionality.
Subsequently during the compilation process when an unknown symbol  is
encountered, the USR's process method will be invoked. The USR in  the
example  will only  'accept' unknown  symbols  that  have a  prefix of
'var_' as being valid variables, all other unknown symbols will result
in a compilation error being raised.

In the example above  the callback of the  USR that is invoked  during
the unknown symbol resolution process only allows for scalar variables
to be defined and resolved -  as that is the simplest and  most common
form.

There is a further  extended  version  of  the  callback  that  can be
overridden that will allow for  more control and choice over  the type
of symbol being  resolved. The following  is an example  definition of
said extended callback:

   template <typename T>
   struct my_usr final : public parser_t::unknown_symbol_resolver
   {
     typedef typename parser_t::unknown_symbol_resolver usr_t;

     my_usr()
     : usr_t(usr_t::e_usrmode_extended)
     {}

     bool process(const std::string& unknown_symbol,
                  symbol_table_t&    symbol_table,
                  std::string&       error_message) override
     {
        bool result = false;

        if (0 == unknown_symbol.find("var_"))
        {
           // Default value of zero
           result = symbol_table.create_variable(unknown_symbol,0);

           if (!result)
           {
              error_message = "Failed to create variable...";
           }
        }
        else if (0 == unknown_symbol.find("str_"))
        {
           // Default value of empty string
           result = symbol_table.create_stringvar(unknown_symbol,"");

           if (!result)
           {
              error_message = "Failed to create string variable...";
           }
        }
        else
           error_message = "Indeterminable symbol type.";

        return result;
     }
   };


In the  example above,  the USR  callback when  invoked will  pass the
primary symbol table associated with the expression being parsed.  The
symbol  resolution  business  logic  can  then  determine  under  what
conditions  a  symbol will  be  resolved including  its  type (scalar,
string, vector etc) and default value. When the callback  successfully
returns  the  symbol  parsing and  resolution  process  will again  be
executed by the parser. The idea here is that given the primary symbol
table will now have the previously detected unknown symbol registered,
it will be correctly resolved  and the general parsing processing  can
then resume as per normal.

Note26: In order to have  the USR's extended mode callback  be invoked
it is necessary to pass  the e_usrmode_extended enum value during  the
constructor of the user defined USR.

Note27: The primary symbol table for an expression is the first symbol
table to be registered with that instance of the expression.

Note28: For a successful symbol resolution using the normal USR all of
the following are required:

   (1) Only if successful shall the process method return TRUE
   (2) The default_value parameter will have been set
   (3) The error_message parameter will be empty
   (4) usr_symbol_type input parameter field will be set to either:
         (*) e_usr_variable_type
         (*) e_usr_constant_type

Note29: For a successful symbol resolution using the extended USR  all
of the following are required:

   (1) Only if successful shall the process method return TRUE
   (2) symbol_table parameter will have had the newly resolved
       variable or string added to it
   (3) error_message parameter will be empty

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 19 - ENABLING & DISABLING FEATURES]
The parser can be configured via its settings instance to either allow
or  disallow certain  features that  are available  within the  ExprTk
grammar. The features fall  into one  of the following six categories:

   (1) Base Functions
   (2) Control Flow Structures
   (3) Logical Operators
   (4) Arithmetic Operators
   (5) Inequality Operators
   (6) Assignment Operators


(1) Base Functions
The list of available base functions is as follows:

   abs, acos, acosh, asin,  asinh, atan, atanh, atan2,  avg, ceil,
   clamp,  cos,  cosh, cot,  csc,  equal, erf,  erfc,  exp, expm1,
   floor,  frac,  hypot,  iclamp, like,  log,  log10,  log2, logn,
   log1p, mand, max, min, mod,  mor, mul, ncdf, pow, root,  round,
   roundn, sec, sgn, sin, sinc, sinh, sqrt, sum, swap, tan,  tanh,
   trunc, not_equal, inrange, deg2grad, deg2rad, rad2deg, grad2deg


The above mentioned base functions  can be either enabled or  disabled
'all' at once, as is demonstrated below:

   parser_t parser;
   expression_t expression;

   parser.settings().disable_all_base_functions();

   parser
      .compile("2 * abs(2 - 3)",expression); // compilation failure

   parser.settings().enable_all_base_functions();

   parser
      .compile("2 * abs(2 - 3)",expression); // compilation success


One can also enable or disable specific base functions. The  following
example  demonstrates  the disabling  of  the trigonometric  functions
'sin' and 'cos':

   parser_t parser;
   expression_t expression;

   parser.settings()
      .disable_base_function(settings_t::e_bf_sin)
      .disable_base_function(settings_t::e_bf_cos);

   parser
      .compile("(sin(x) / cos(x)) == tan(x)",expression); // failure

   parser.settings()
      .enable_base_function(settings_t::e_bf_sin)
      .enable_base_function(settings_t::e_bf_cos);

   parser
      .compile("(sin(x) / cos(x)) == tan(x)",expression); // success


(2) Control Flow Structures
The list of available control flow structures is as follows:

   (a) If or If-Else
   (b) Switch statement
   (c) For Loop
   (d) While Loop
   (e) Repeat Loop


The  above  mentioned  control flow structures  can be  either enabled
or disabled 'all' at once, as is demonstrated below:

   parser_t parser;
   expression_t expression;

   const std::string program =
      " var x := 0;                      "
      " for (var i := 0; i < 10; i += 1) "
      " {                                "
      "   x += i;                        "
      " }                                ";

   parser.settings().disable_all_control_structures();

   parser
      .compile(program,expression); // compilation failure

   parser.settings().enable_all_control_structures();

   parser
      .compile(program,expression); // compilation success


One can also enable or  disable specific control flow structures.  The
following example demonstrates the  disabling of the for-loop  control
flow structure:

   parser_t parser;
   expression_t expression;

   const std::string program =
      " var x := 0;                      "
      " for (var i := 0; i < 10; i += 1) "
      " {                                "
      "   x += i;                        "
      " }                                ";

   parser.settings()
      .disable_control_structure(settings_t::e_ctrl_for_loop);

   parser
      .compile(program,expression); // failure

   parser.settings()
      .enable_control_structure(settings_t::e_ctrl_for_loop);

   parser
      .compile(program,expression); // success


(3) Logical Operators
The list of available logical operators is as follows:

   and, nand, nor, not, or, xnor, xor, &, |


The  above  mentioned  logical  operators  can  be  either  enabled or
disabled 'all' at once, as is demonstrated below:

   parser_t parser;
   expression_t expression;

   parser.settings().disable_all_logic_ops();

   parser
      .compile("1 or not(0 and 1)",expression); // compilation failure

   parser.settings().enable_all_logic_ops();

   parser
      .compile("1 or not(0 and 1)",expression); // compilation success


One  can  also  enable  or  disable  specific  logical  operators. The
following  example  demonstrates  the disabling  of the  'and' logical
operator:

   parser_t parser;
   expression_t expression;

   parser.settings()
      .disable_logic_operation(settings_t::e_logic_and);

   parser
      .compile("1 or not(0 and 1)",expression); // failure

   parser.settings()
      .enable_logic_operation(settings_t::e_logic_and);

   parser
      .compile("1 or not(0 and 1)",expression); // success


(4) Arithmetic Operators
The list of available arithmetic operators is as follows:

   +, -, *, /, %, ^


The  above mentioned  arithmetic operators  can be  either enabled  or
disabled 'all' at once, as is demonstrated below:

   parser_t parser;
   expression_t expression;

   parser.settings().disable_all_arithmetic_ops();

   parser
      .compile("1 + 2 / 3",expression); // compilation failure

   parser.settings().enable_all_arithmetic_ops();

   parser
      .compile("1 + 2 / 3",expression); // compilation success


One  can also  enable or  disable specific  arithmetic operators.  The
following  example  demonstrates  the disabling  of  the  addition '+'
arithmetic operator:

   parser_t parser;
   expression_t expression;

   parser.settings()
      .disable_arithmetic_operation(settings_t::e_arith_add);

   parser
      .compile("1 + 2 / 3",expression); // failure

   parser.settings()
      .enable_arithmetic_operation(settings_t::e_arith_add);

   parser
      .compile("1 + 2 / 3",expression); // success


(5) Inequality Operators
The list of available inequality operators is as follows:

   <, <=, >, >=, ==, =, != <>


The  above mentioned  inequality operators  can be  either enabled  or
disabled 'all' at once, as is demonstrated below:

   parser_t parser;
   expression_t expression;

   parser.settings().disable_all_inequality_ops();

   parser
      .compile("1 < 3",expression); // compilation failure

   parser.settings().enable_all_inequality_ops();

   parser
      .compile("1 < 3",expression); // compilation success


One  can also  enable or  disable specific  inequality operators.  The
following  example demonstrates  the disabling  of  the  less-than '<'
inequality operator:

   parser_t parser;
   expression_t expression;

   parser.settings()
      .disable_inequality_operation(settings_t::e_ineq_lt);

   parser
      .compile("1 < 3",expression); // failure

   parser.settings()
      .enable_inequality_operation(settings_t::e_ineq_lt);

   parser
      .compile("1 < 3",expression); // success


(6) Assignment Operators
The list of available assignment operators is as follows:

   :=, +=, -=, *=, /=, %=


The  above mentioned  assignment operators  can be  either enabled  or
disabled 'all' at once, as is demonstrated below:

   T x = T(0);

   parser_t       parser;
   expression_t   expression;
   symbol_table_t symbol_table;

   symbol_table.add_variable("x",x);

   expression.register_symbol_table(symbol_table);

   parser.settings().disable_all_assignment_ops();

   parser
      .compile("x := 3",expression); // compilation failure

   parser.settings().enable_all_assignment_ops();

   parser
      .compile("x := 3",expression); // compilation success


One  can also  enable or  disable specific  assignment operators.  The
following  example demonstrates  the  disabling  of the  '+=' addition
assignment operator:

   T x = T(0);

   parser_t       parser;
   expression_t   expression;
   symbol_table_t symbol_table;

   symbol_table.add_variable("x",x);

   expression.register_symbol_table(symbol_table);

   parser.settings()
      .disable_assignment_operation(settings_t::e_assign_addass);

   parser
      .compile("x += 3",expression); // failure

   parser.settings()
      .enable_assignment_operation(settings_t::e_assign_addass);

   parser
      .compile("x += 3",expression); // success


Note30: In the  event of  a  base  function  being  disabled, one  can
redefine  the  base  function  using  the  standard  custom   function
definition process.  In the  following example  the 'sin'  function is
disabled then redefined as a function taking degree input.

   template <typename T>
   struct sine_deg final : public exprtk::ifunction<T>
   {
      sine_deg() : exprtk::ifunction<T>(1) {}

      inline T operator()(const T& v) override
      {
         const T pi = exprtk::details::numeric::constant::pi;
         return std::sin((v * T(pi)) / T(180));
      }
   };

    ...

   typedef exprtk::symbol_table<T> symbol_table_t;
   typedef exprtk::expression<T>   expression_t;
   typedef exprtk::parser<T>       parser_t;

   typedef typename parser_t::settings_store settings_t;

   sine_deg<T> sine;

   symbol_table.add_reserved_function("sin",sine);

   expression_t expression;

   expression.register_symbol_table(symbol_table);

   parser_t parser;

   parser.settings()
      .disable_base_function(settings_t::e_bf_sin);

   parser.compile("1 + sin(30)",expression);


In the example above, the custom 'sin' function is registered with the
symbol_table using the method 'add_reserved_function'. This is done so
as to bypass the checks for reserved words that are carried out on the
provided symbol names when calling the standard 'add_function' method.
Normally if  a user  specified symbol  name conflicts  with any of the
ExprTk reserved words, the add_function call will fail.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 20 - EXPRESSION RETURN VALUES]
ExprTk expressions can return immediately from any point by  utilising
the return call. Furthermore the  return call can be used  to transfer
out multiple return values from within the expression.

If an expression evaluation exits using a return point, the result  of
the call  to the 'value' method will be  NaN, and  it is expected that
the return values will be available from the results_context.

In  the  following example  there  are  three  return  points  in  the
expression.  If  neither  of  the  return  points  are  hit,  then the
expression will return normally.

   const std::string expression_string =
      " if (x < y)                                   "
      "    return [x + 1,'return-call 1'];           "
      " else if (x > y)                              "
      "    return [y / 2, y + 1, 'return-call 2'];   "
      " else if (equal(x,y))                         "
      "    x + y;                                    "
      " return [x, y, x + y, x - y, 'return-call 3'] ";

   typedef exprtk::symbol_table<double> symbol_table_t;
   typedef exprtk::expression<double>   expression_t;
   typedef exprtk::parser<double>       parser_t;

   symbol_table_t symbol_table;
   expression_t   expression;
   parser_t       parser;

   double x = 0;
   double y = 0;

   symbol_table.add_variable("x",x);
   symbol_table.add_variable("y",y);

   expression.register_symbol_table(symbol_table);

   parser.compile(expression_string,expression);

   T result = expression.value();

   if (expression.return_invoked())
   {
      typedef exprtk::results_context<T> results_context_t;
      typedef typename results_context_t::type_store_t type_t;
      typedef typename type_t::scalar_view scalar_t;
      typedef typename type_t::vector_view vector_t;
      typedef typename type_t::string_view string_t;

      const results_context_t& results = expression.results();

      for (std::size_t i = 0; i < results.count(); ++i)
      {
         type_t t = results[i];

         switch (t.type)
         {
            case type_t::e_scalar : ...
                                    break;

            case type_t::e_vector : ...
                                    break;

            case type_t::e_string : ...
                                    break;

            default               : continue;
         }
   }


In the above example, there are three possible "return" points and one
regular result. Only one of the four paths can ever be realised. Hence
it is necessary to capture  the result of the expression  value method
call. In the event,  the call to return_invoked  is not true then  the
non-return code  path was  executed and  the result  of the evaluation
will be the result of the expression's value method.

Note31: Processing of  the return  results is  similar to  that of the
generic function call parameters.

The results_context provides getter  methods for each of  the possible
return types (scalar, vector and string) and can be used as follows:

   typedef exprtk::symbol_table<T> symbol_table_t;
   typedef exprtk::expression<T>   expression_t;
   typedef exprtk::parser<T>       parser_t;

   const std::string expression_str =
      " if (x > y)                                  "
      "    return [1];                              "
      " else                                        "
      "    return [ x, x + y, 2 * v, s + 'world' ]; ";

   symbol_table_t symbol_table;
   expression_t   expression;
   parser_t       parser;

   symbol_table.add_variable ("x", x);
   symbol_table.add_variable ("y", y);
   symbol_table.add_variable ("z", z);
   symbol_table.add_vector   ("v", v);
   symbol_table.add_stringvar("s", s);

   parser.compile(expression_str, expression);

   expression.value();

   typedef exprtk::results_context<T> results_context_t;
   const results_context_t& results = expression.results();

   if (results.count() == 4)
   {
      T result_x0;
      T result_x1;
      std::string result_s;
      std::vector<T> result_v;

      results.get_scalar(0, result_x0);
      results.get_scalar(1, result_x1);
      results.get_string(3, result_s );
      results.get_vector(2, result_v );
   }


It is however recommended that if there is to be only a single flow of
execution  through  the  expression,  that  the  simpler  approach  of
registering external variables of appropriate type be used.

This method simply requires the variables that are to hold the various
results that are to be computed within the expression to be registered
with an associated symbol_table  instance. Then within the  expression
itself  to  have  the result  variables  be  assigned the  appropriate
values.

   typedef exprtk::symbol_table<double> symbol_table_t;
   typedef exprtk::expression<double>   expression_t;
   typedef exprtk::parser<double>       parser_t;

   const std::string expression_string =
      " var x := 123.456;     "
      " var s := 'ijk';       "
      " result0 := x + 78.90; "
      " result1 := s + '123'  ";

   double      result0;
   std::string result1;

   symbol_table_t symbol_table;
   symbol_table.add_variable ("result0",result0);
   symbol_table.add_stringvar("result1",result1);

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   parser_t parser;
   parser.compile(expression_string,expression);

   expression.value();

   printf("Result0: %15.5f\n", result0        );
   printf("Result1: %s\n"    , result1.c_str());


In the example above, the expression will compute two results. As such
two result variables are defined to hold the values named result0  and
result1 respectively. The first is of scalar type (double), the second
is of  string type.  Once the  expression has  been evaluated, the two
variables will have been updated  with the new result values,  and can
then be further utilised from within the calling host program.

There will be times when  an expression may have multiple  exit paths,
where not all the paths will be return-statement based. The  following
example builds upon the previous examples, but this time at least  one
path is not return based.

   typedef exprtk::symbol_table<double> symbol_table_t;
   typedef exprtk::expression<double>   expression_t;
   typedef exprtk::parser<double>       parser_t;

   double x = 100.0;
   double y = 200.0;

   symbol_table_t symbol_table;
   expression_t   expression;
   parser_t       parser;

   symbol_table.add_variable ("x", x);
   symbol_table.add_variable ("y", y);

   expression.register_symbol_table(symbol_table);

   const std::string expression_string =
      " for (var i := 0; i < 10; i += 1)       "
      " {                                      "
      "    if (i > x)                          "
      "    {                                   "
      "       return [x + y, 'return-call 1']; "
      "    }                                   "
      "    else if (i > y)                     "
      "    {                                   "
      "       return [x - y, 'return-call 2']; "
      "    }                                   "
      " };                                     "
      "                                        "
      " x / y                                  ";

   parser.compile(expression_str, expression);

   const auto result = expression.value();

   if (expression.return_invoked())
   {
      const auto results = expression.results();

      for (std::size_t i = 0; i <  results.count(); ++i)
      {
         const auto& rtrn_result = results[i];
         .
         .
         .
      }
   }
   else
   {
      printf("result: %f\n",result);
   }


After having called  the value method  on the expression,  calling the
return_invoked method will determine  if the expression completed  due
to a return statement being invoked or if it finished normally.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 21 - COMPILATION ERRORS]
When attempting to compile  a malformed or otherwise  erroneous ExprTk
expression, the  compilation process  will result  in an  error, as is
indicated  by  the  'compile'  method  returning  a  false  value.   A
diagnostic indicating the first error encountered and its cause can be
obtained by  invoking the  'error' method,  as is  demonstrated in the
following example:

   if (!parser.compile(expression_string,expression))
   {
      printf("Error: %s\n", parser.error().c_str());
      return false;
   }


Any error(s) resulting from a failed compilation will be stored in the
parser instance until the next time a compilation is performed. Before
then errors can be enumerated  in the order they occurred  by invoking
the 'get_error' method which itself will return a 'parser_error' type.
A parser_error object will contain an error diagnostic, an error  mode
(or class), and the character position of the error in the  expression
string. The following example demonstrates the enumeration of error(s)
in the event of a failed compilation.

   typedef exprtk::parser<T>          parser_t;
   typedef exprtk::parser_error::type error_t;

   if (!parser.compile(expression_string,expression))
   {
      for (std::size_t i = 0; i < parser.error_count(); ++i)
      {
         typedef exprtk::parser_error::type error_t;

         error_t error = parser.get_error(i);

         printf("Error[%02d] Position: %02d Type: [%14s] Msg: %s\n",
                i,
                error.token.position,
                exprtk::parser_error::to_str(error.mode).c_str(),
                error.diagnostic.c_str());
      }

      return false;
   }


Assuming the  following expression '2 + (3 / log(1 + x))' which uses a
variable named 'x'  that has not been registered  with the appropriate
symbol_table  instance and  is not  a locally  defined variable,  once
compiled the  above denoted post compilation error handling code shall
produce the following output:

  Error[00] Pos:17 Type:[Syntax] Msg: ERR184 - Undefined symbol: 'x'


For  expressions  comprised  of  multiple  lines,  the  error position
provided in the  parser_error object can  be converted into  a pair of
line and column numbers by invoking the 'update_error' function as  is
demonstrated by the following example:

   if (!parser.compile(program_str,expression))
   {
      for (std::size_t i = 0; i < parser.error_count(); ++i)
      {
         typedef exprtk::parser_error::type error_t;

         error_t error = parser.get_error(i);

         exprtk::parser_error::update_error(error,program_str);

         printf("Error[%0lu] at line: %lu column: %lu\n",
                i,
                error.line_no,
                error.column_no);
      }

      return false;
   }


Note32: There are five distinct error modes in ExprTk which denote the
class of an error. These classes are as follows:

   (a) Syntax
   (b) Token
   (c) Numeric
   (d) Symbol Table
   (e) Lexer


(a) Syntax Errors
These are errors  related to invalid  syntax found within  the denoted
expression. Examples are invalid sequences of operators and variables,
incorrect number  of parameters  to functions,  invalid conditional or
loop structures and invalid use of keywords.

   eg:  'for := sin(x,y,z) + 2 * equal > until[2 - x,3]'


(b) Token Errors
Errors in this class relate to  token level errors detected by one  or
more of the following checkers:

   (1) Bracket Checker
   (2) Numeric Checker
   (3) Sequence Checker


(c) Numeric Errors
This class of  error is related  to conversion of  numeric values from
their  string form  to the  underlying numerical  type (float,  double
etc).

(d) Symbol Table Errors
This is the class of errors related to failures when interacting  with
the registered symbol_table instance. Errors such as not being able to
find,  within  the  symbol_table,  symbols  representing  variables or
functions, to being unable to create new variables in the symbol_table
via the 'unknown symbol resolver' mechanism.

Note33: The function compositor  also supports  error message handling
similar to how it is  done via the parser. The  following demonstrates
how after a failed function  composition the associated errors can  be
enumerated.

   typedef exprtk::function_compositor<T>  compositor_t;
   typedef typename compositor_t::function function_t;

   compositor_t compositor;

   const bool compositor_result =
   compositor.add(
      function_t("foobar")
      .vars("x","y")
      .expression
      ( " x + y / z " ));

   if (!compositor_result)
   {
      printf("Error: %s\n", compositor.error().c_str());

      for (std::size_t i = 1; i < compositor.error_count(); ++i)
      {
         typedef exprtk::parser_error::type error_t;

         error_t error = compositor.get_error(i);

         printf("Err No.: %02d  Pos: %02d  Type: [%14s] Msg: %s\n",
                static_cast<unsigned int>(i),
                static_cast<unsigned int>(error.token.position),
                exprtk::parser_error::to_str(error.mode).c_str(),
                error.diagnostic.c_str());
      }
   }


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 22 - RUNTIME LIBRARY PACKAGES]
ExprTk includes  a range  of extensions,  that provide functionalities
beyond simple numerical calculations. Currently the available packages
are:

  +---+--------------------+-----------------------------------+
  | # |    Package Name    |          Namespace/Type           |
  +---+--------------------+-----------------------------------+
  | 1 | Basic I/O          | exprtk::rtl::io::package<T>       |
  | 2 | File I/O           | exprtk::rtl::io::file::package<T> |
  | 3 | Vector Operations  | exprtk::rtl::vecops::package<T>   |
  +---+--------------------+-----------------------------------+


In order to make the  features of a specific package  available within
an  expression,  an instance  of  the package  must  be added  to  the
expression's associated  symbol table.  In the  following example, the
file I/O package is made available for the given expression:

   typedef exprtk::symbol_table<T> symbol_table_t;
   typedef exprtk::expression<T>   expression_t;
   typedef exprtk::parser<T>       parser_t;

   exprtk::rtl::io::file::package<T> fileio_package;

   const std::string expression_string =
      " var file_name := 'file.txt';       "
      " var stream    := null;             "
      "                                    "
      " stream := open(file_name,'w');     "
      "                                    "
      " write(stream,'Hello world....\n'); "
      "                                    "
      " close(stream);                     "
      "                                    ";

   symbol_table_t symbol_table;
   symbol_table.add_package(fileio_package);

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   parser_t parser;
   parser.compile(expression_string,expression);

   expression.value();


(1) Basic I/O functions:

   (a) print
   (b) println

(2) File I/O functions:

   (a) open    (b) close
   (c) write   (d) read
   (e) getline (f) eof

(3) Vector Operations functions:

(a) all_true     (b) all_false
(c) any_true     (d) any_false
(e) assign       (f) count
(g) copy         (h) reverse
(i) rotate-left  (j) rotate-right
(k) shift-left   (l) shift-right
(m) sort         (n) nth_element
(o) iota         (p) sumk
(q) axpy         (r) axpby
(s) axpyz        (t) axpbyz
(u) axpbz        (v) dot
(w) dotk         (x) diff

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 23 - HELPERS & UTILS]
The  ExprTk library  provides a  series of  usage simplifications  via
helper routines that combine various processes into a single 'function
call'  making  certain  actions   easier  to  carry  out   though  not
necessarily in the most efficient way possible. A list of the routines
are as follows:

   (a) collect_variables
   (b) collect_functions
   (c) compute
   (d) integrate
   (e) derivative
   (f) second_derivative
   (g) third_derivative


(a) collect_variables
This function will collect all the variable symbols in a given  string
representation of an expression and  return them in an STL  compatible
sequence  data structure  (eg: std::vector,  dequeue etc)  specialised
upon a std::string type. If an error occurs during the parsing of  the
expression  then  the return  value  of the  function  will be  false,
otherwise it will be  true. An example use  of the given routine is as
follows:

   const std::string expression = "x + abs(y / z)";

   std::vector<std::string> variable_list;

   if (exprtk::collect_variables(expression, variable_list))
   {
      for (const auto& var : variable_list)
      {
         ...
      }
   }
   else
     printf("An error occurred.");


(b) collect_functions
This function will collect all the function symbols in a given  string
representation of an expression and  return them in an STL  compatible
sequence  data structure  (eg: std::vector,  dequeue etc)  specialised
upon a std::string type. If an error occurs during the parsing of  the
expression  then  the return  value  of the  function  will be  false,
otherwise it  will be true. An example  use of the given routine is as
follows:

   const std::string expression = "x + abs(y / cos(1 + z))";

   std::deque<std::string> function_list;

   if (exprtk::collect_functions(expression, function_list))
   {
      for (const auto& func : function_list)
      {
         ...
      }
   }
   else
     printf("An error occurred.");


Note34: When  either the  'collect_variables'  or  'collect_functions'
free functions return  true - that  does not necessarily  indicate the
expression itself is  valid. It is  still possible that  when compiled
the expression may have certain  'type' related errors - though  it is
highly likely  that no  semantic errors  will occur  if either  return
true.

Note35: The default interface provided for both the  collect_variables
and collect_functions  free_functions, assumes  that expressions  will
only be  utilising the  ExprTk reserved  functions (eg:  abs, cos, min
etc). When user defined functions are  to be used in an expression,  a
symbol_table  instance  containing  said functions  can  be  passed to
either routine, and  will be incorporated  during the compilation  and
Dependent Entity  Collection processes.  In the  following example,  a
user  defined  free  function   named  'foo'  is  registered   with  a
symbol_table.  Finally  the   symbol_table  instance  and   associated
expression string are passed to the exprtk::collect_functions routine.

   template <typename T>
   T foo(T v)
   {
      return std::abs(v + T(2)) / T(3);
   }

   ......

   exprtk::symbol_table<T> sym_tab;

   symbol_table.add_function("foo",foo);

   const std::string expression = "x + foo(y / cos(1 + z))";

   std::deque<std::string> function_list;

   if (exprtk::collect_functions(expression, sym_tab, function_list))
   {
      for (const auto& func : function_list)
      {
         ...
      }
   }
   else
     printf("An error occurred.");


(c) compute
This free function  will compute the  value of an  expression from its
string form.  If an  invalid expression  is passed,  the result of the
function will be false indicating an error, otherwise the return value
will  be  true  indicating success.  The  compute  function has  three
overloads, the definitions of which are:

   (1) No variables
   (2) One variable called x
   (3) Two variables called x and y
   (3) Three variables called x, y and z


Example uses of  each of the  three overloads for  the compute routine
are as follows:

   T result = T(0);

   // No variables overload
   const std::string no_vars = "abs(1 - (3 / pi)) * 5";

   if (!exprtk::compute(no_vars,result))
      printf("Failed to compute: %s",no_vars.c_str());
   else
      printf("Result: %15.5f\n",result);

   // One variable 'x' overload
   T x = T(123.456);

   const std::string one_var = "abs(x - (3 / pi)) * 5";

   if (!exprtk::compute(one_var, x, result))
      printf("Failed to compute: %s",one_var.c_str());
   else
      printf("Result: %15.5f\n",result);

   // Two variables 'x' and 'y' overload
   T y = T(789.012);

   const std::string two_var = "abs(x - (y / pi)) * 5";

   if (!exprtk::compute(two_var, x, y, result))
      printf("Failed to compute: %s",two_var.c_str());
   else
      printf("Result: %15.5f\n",result);

   // Three variables 'x', 'y' and 'z' overload
   T z = T(345.678);

   const std::string three_var = "abs(x - (y / pi)) * z";

   if (!exprtk::compute(three_var, x, y, z, result))
      printf("Failed to compute: %s",three_var.c_str());
   else
      printf("Result: %15.5f\n",result);


(d) integrate
This free function will attempt to perform a numerical integration  of
a single variable compiled expression over a specified range and  step
size. The numerical  integration is based  on the three  point form of
Simpson's rule. The  integrate function has  two overloads, where  the
variable of integration can  either be passed as  a reference or as  a
name in string form. Example usage of the function is as follows:

   typedef exprtk::symbol_table<T> symbol_table_t;
   typedef exprtk::expression<T>   expression_t;
   typedef exprtk::parser<T>       parser_t;

   const std::string expression_string = "sqrt(1 - (x^2))";

   T x = T(0);

   symbol_table_t symbol_table;
   symbol_table.add_variable("x",x);

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   parser_t parser;
   parser.compile(expression_string,expression);

   ....

   // Integrate in domain [-1,1] using a reference to x variable
   T area1 = exprtk::integrate(expression, x, T(-1), T(1));

   // Integrate in domain [-1,1] using name of x variable
   T area2 = exprtk::integrate(expression, "x", T(-1), T(1));


(e) derivative
This free function will attempt to perform a numerical differentiation
of a single variable compiled expression at a given point for a  given
epsilon, using a  variant of Newton's  difference quotient called  the
five-point stencil method. The derivative function has two  overloads,
where  the  variable of  differentiation  can either  be  passed as  a
reference or as a name in string form. Example usage of the derivative
function is as follows:

   typedef exprtk::symbol_table<T> symbol_table_t;
   typedef exprtk::expression<T>   expression_t;
   typedef exprtk::parser<T>       parser_t;

   const std::string expression_string = "sqrt(1 - (x^2))";

   T x = T(0);

   symbol_table_t symbol_table;
   symbol_table.add_variable("x",x);

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   parser_t parser;
   parser.compile(expression_string,expression);

   ....

   // Differentiate expression at value of x = 12.3 using a reference
   // to the x variable
   x = T(12.3);
   T derivative1 = exprtk::derivative(expression, x);

   // Differentiate expression where value x = 45.6 using name
   // of the x variable
   x = T(45.6);
   T derivative2 = exprtk::derivative(expression, "x");


(f) second_derivative
This  free  function  will  attempt  to  perform  a  numerical  second
derivative of a single variable  compiled expression at a given  point
for a given epsilon, using  a variant of Newton's difference  quotient
method. The second_derivative function  has  two overloads, where  the
variable of differentiation can either be passed as a reference or  as
a name in string form. Example usage of the second_derivative function
is as follows:

   typedef exprtk::symbol_table<T> symbol_table_t;
   typedef exprtk::expression<T>   expression_t;
   typedef exprtk::parser<T>       parser_t;

   const std::string expression_string = "sqrt(1 - (x^2))";

   T x = T(0);

   symbol_table_t symbol_table;
   symbol_table.add_variable("x",x);

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   parser_t parser;
   parser.compile(expression_string,expression);

   ....

   // Second derivative of expression where value of x = 12.3 using a
   // reference to x variable
   x = T(12.3);
   T derivative1 = exprtk::second_derivative(expression,x);

   // Second derivative of expression where value of x = 45.6 using
   // name of x variable
   x = T(45.6);
   T derivative2 = exprtk::second_derivative(expression, "x");


(g) third_derivative
This  free  function  will  attempt  to  perform  a  numerical   third
derivative of a single variable  compiled expression at a given  point
for a given epsilon, using  a variant of Newton's difference  quotient
method. The  third_derivative function  has two  overloads, where  the
variable of differentiation can either be passed as a reference or  as
a name in string form. Example  usage of the third_derivative function
is as follows:

   typedef exprtk::symbol_table<T> symbol_table_t;
   typedef exprtk::expression<T>   expression_t;
   typedef exprtk::parser<T>       parser_t;

   const std::string expression_string = "sqrt(1 - (x^2))";

   T x = T(0);

   symbol_table_t symbol_table;
   symbol_table.add_variable("x",x);

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   parser_t parser;
   parser.compile(expression_string,expression);

   ....

   // Third derivative of expression where value of x = 12.3 using a
   // reference to the x variable
   x = T(12.3);
   T derivative1 = exprtk::third_derivative(expression, x);

   // Third derivative of expression where value of x = 45.6 using
   // name of the x variable
   x = T(45.6);
   T derivative2 = exprtk::third_derivative(expression, "x");

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 24 - RUNTIME CHECKS]
The  ExprTk library   provides the ability  to perform runtime  checks
during expression evaluation so as to ensure memory access  violations
errors  are  caught and  handled  without causing further  issues. The
checks typically cover:

   1. Vector access and handling
   2. String access and handling
   3. Loop iteration checks
   4. Compilation checkpointing
   5. Assert statements


(1) Vector Access Runtime Checks
Expressions that contain vectors where elements of the vectors may  be
accessed using  indexes that  can only  be determined  at runtime  may
result  in  memory  access violations  when the  index is  out of  the
vector's  bound.  Some  examples  of  problematic  expressions  are as
follows:

   1. vec[i]
   2. vec[i + j]
   3. vec[i + 10]
   4. vec[i + vec[]] := x + y
   5. vec[i + j] <=> vec[i]
   6. vec[i + j] := (vec1 + vec2)[i + j]


In the above expressions,  it is assumed that  the values used in  the
index operator  may either  exceed the  vector bounds  or precede  the
vector's start, In  short, the indexes  may not necessarily  be within
the range [0,vec[]).

ExprTk provides the ability to inject a runtime check at the point  of
index evaluation and  handle situations where  the index violates  the
vector's  bounds.  This  capability is  done  by  registering a  user-
implemented Vector Access Runtime  Check (VARTC) to the  parser before
expression compilation. Initially a VARTC can be defined as follows:

   struct my_vector_access_rtc final :
      public exprtk::vector_access_runtime_check
   {
      bool handle_runtime_violation(violation_context& context)
      override
      {
         // Handling of the violation
         return ...;
      }
   };


Then an instance of the VARTC can be registered with a parser instance
as follows:

   my_vector_access_rtc vartc;

   exprtk::symbol_table<T> symbol_table;

   T i;
   T x;
   T y;
   std::vector<T> vec = { 0, 1, 2, 3, 4 };

   symbol_table.add_variable("i"  , i  );
   symbol_table.add_variable("x"  , x  );
   symbol_table.add_variable("y"  , y  );
   symbol_table.add_vector  ("vec", vec);

   exprtk::expression<T> expression;
   exprtk::parser<T> parser;

   parser.register_vector_access_runtime_check(vartc);

   std::string expression = "vec[i + vec[]] := x + y";

   parser.compile(expression_str, expression);

   try
   {
      expression.value();
   }
   catch (std::runtime_error& rte)
   {
      printf("Exception: %s\n", rte.what());
   }


Note36: The  lifetime of  any parser  or expression  instance must not
exceed that of any VARTC instance that has been registered with it.

When a vector access violation occurs, the registered VARTC instance's
handle_runtime_violation method  will be  invoked, coupled  with it  a
violation_context shall  be provided  that will  contain the following
members:

  1. base_ptr: Of type void*,  which points to the  first element
     of the vector.  The base_ptr can  also be used  as a key  to
     determine the  vector upon  which the  access violation  has
     occurred.

  2. end_ptr : Of type void*, which points to one position  after
     the last element of the vector

  3. access_ptr: Of  type void*,  points to  the memory  location
     which is the base_ptr offset by the derived index value.

  4. type_size: Size of the vector's element type in bytes. This
     value can be used to  determine  the number of  elements in
     the vector based on the base_ptr and end_ptr.


The implementation of the handle_runtime_violation method can at this
point perform various actions such as:

  1. Log the violation
  2. Throw an exception (eg: std::runtime_error)
  3. Remedy the access_ptr to allow for the evaluation to continue


Note37: When employing option [3],  handle_runtime_violation needs  to
return  true,  otherwise  the  caller  will assume an unhandled access
violation and default to using the base_ptr.

It is  recommended, at  the  very least,  to throw an  exception  when
handling vector access violations and to only consider option [3] when
the the ramifications of changing the access_ptr are well understood.

The following are simple examples of how the  handle_runtime_violation
can be implemented.

Example 1: Log the access violation to stdout and then throw a runtime
error exception:

   bool handle_runtime_violation(violation_context& context) override
   {
      printf("ERROR -  Runtime vector access violation. "
             "base: %p end: %p access: %p typesize: %lu\n",
             context.base_ptr  ,
             context.end_ptr   ,
             context.access_ptr,
             context.type_size);

      throw std::runtime_error("Runtime vector access violation.");
      return false;
   }


Example 2: Handle the access violation by resetting the access pointer
to the last value in the vector.

   bool handle_runtime_violation(violation_context& context) override
   {
      context.access_ptr =
         static_cast<char*>(context.end_ptr) - context.type_size;
      return true;
   }


Note38: The return value  of true  in the above handler method signals
the caller to continue the vector access using the updated access_ptr.


(2) String Access Runtime Checks
Expressions that contain strings  where elements or substrings  of the
strings may be accessed using  indexes that can only be  determined at
runtime may result in memory access violations when the index or range
is out of the string's bound. Examples of problematic expressions  are
as follows:

   1. s[i : j + k]
   2. s[i : j + k][-x : y]
   3. (s1 + s2)[i : j + k]
   4. '01234'[5 + i]
   5. s += s[i : j + k]
   6. s[i : j + k] := 'chappy days'[1 : ]


To enable string access runtime checks all  one needs to  do is simply
use the following  define before the  ExprTk header is  included or as
part of the compilation define parameters:

   exprtk_enable_range_runtime_checks


When  the above  define is used,  and a string  related runtime access
violation occurs  a std::runtime_error  exception will  be thrown. The
following  demonstrates  the  general  flow  of  handling  the  access
violation:

   parser.compile(expression_string, expression)
   .
   .
   try
   {
      expression.value();
   }
   catch (std::runtime_error& rte)
   {
      printf("Exception: %s\n", rte.what());
   }


(3) Loop Iteration Checks
Expressions that contain loop structures (eg: for/while/repeat et  al)
can be problematic from a usage point of view due to the difficulty in
determining the following:

   1. Will the loop ever complete (aka is this an infinite loop?)
   2. Maximum loop execution time


ExprTk provides  the ability  to inject  a runtime  check within  loop
conditionals, and to  have the result  of the check  either signal the
loop to continue or for the check to raise a loop violation error.

The process involves instantiating a user defined   loop_runtime_check
(LRTC), registering  the instance  with a  exprtk::parser instance and
specifying  which  loop types  the  check is  to  performed upon.  The
following code demonstrates a how custom LRTC can be instantiated  and
registered with the associated parser:

   typedef exprtk::parser<T> parser_t;
   typedef exprtk::loop_runtime_check loop_runtime_check_t;

   my_loop_rtc loop_rtc;
   loop_runtime_check.loop_set = loop_runtime_check_t::e_all_loops;
   loop_runtime_check.max_loop_iterations = 100000;

   parser_t parser;

   parser.register_loop_runtime_check(loop_rtc);


The following is an example of how one could derive from and implement
a custom loop_runtime_check:

   struct my_loop_rtc final : exprtk::loop_runtime_check
   {

      bool check() override
      {
         //
         return ...
      }

      void handle_runtime_violation
         (const exprtk::violation_context&) override
      {
         throw std::runtime_error("Loop runtime violation.");
      }
   };


In the above  code, if either  the check method  returns false or  the
loop  iteration  count  exceeds  the  max_loop_iterations  value,  the
handle_runtime_violation method  will be  invoked, coupled  with it  a
violation_context shall  be provided  that will  contain the following
members:

   1. loop: Of  type loop_types.  This value  denotes the  type of
      loop that triggered the violation (e_for_loop, e_while_loop,
      e_repeat_until_loop).

   2. violation:  Of type  type. This  value denotes  the type  of
      violation (e_iteration_count, e_timeout)

   3. iteration_count: Of type uint64_t. The number of  iterations
      that the triggering loop has executed since the start of the
      expression.


Note39: The  lifetime of  any parser  or expression  instance must not
exceed that of any LRTC instance that has been registered with it.

The following is an  example implementation of an  LRTC that
supports loop timeout violations:

   struct timeout_loop_rtc final : exprtk::loop_runtime_check
   {
      using time_point_t =
         std::chrono::time_point<std::chrono::steady_clock>;

      std::size_t iterations_ = 0;
      time_point_t timeout_tp_;

      bool check() override
      {
         if (std::chrono::steady_clock::now() >= timeout_tp_)
         {
            // handle_runtime_violation shall be invoked
            return false;
         }

         return true;
      }

      void handle_runtime_violation
         (const exprtk::violation_context&) override
      {
         throw std::runtime_error("Loop timed out");
      }

      void set_timeout_time(const time_point_t& timeout_tp)
      {
         timeout_tp_ = timeout_tp;
      }
   };


In the above code, the check method shall be invoked on each iteration
of the associated loop. Within the method the current time is compared
to the setup timeout time-point, in the event the current time exceeds
the timeout, the method returns false, triggering the violation, which
in turn will result in the handle_runtime_violation being invoked.

The following code demonstrates how the above defined LRTC can be used
to ensure that at the very least the loop portion(s) of an  expression
will never exceed a given amount of execution time.

   typedef exprtk::parser<T> parser_t;
   typedef exprtk::loop_runtime_check loop_runtime_check_t;

   my_loop_rtc loop_rtc;
   loop_rtc.loop_set = loop_runtime_check_t::e_all_loops;
   loop_rtc.max_loop_iterations = 100000;

   parser_t parser;

   parser.register_loop_runtime_check(loop_rtc);
   .
   .
   .
   .
   using std::chrono;
   const auto max_duration = seconds(25);

   try
   {
      loop_rtc.set_timeout_time(steady_clock::now() + max_duration);
      expression.value();

      loop_rtc.set_timeout_time(steady_clock::now() + max_duration);
      expression.value();

      loop_rtc.set_timeout_time(steady_clock::now() + max_duration);
      expression.value();

   }
   catch(std::runtime_error& exception)
   {
      printf("Exception: %s\n",exception.what());
   }


(4) Compilation Process Checkpointing
When compiling an expression, one may require the compilation  process
to periodically  checkpoint its  internal state,  subsequently at  the
checkpoint one can then make the decision to continue the  compilation
process or to immediately terminate and return.

The following are reasons one  may want to checkpoint the  compilation
process:

   1. Determine if the compilation process has run for far too long
   2. Determine if the current stack frame size exceeds a limit
   3. Enforce an external termination request


ExprTk  provides  the  ability   to  inject  a  checkpoint   into  the
compilation  process  that   will  be  evaluated   periodically.  This
capability is achieved  by registering a  user-implemented compilation
check (CCK) to the parser  before expression  compilation. Initially a
CCK can be defined as follows:

   struct compilation_timeout_check final :
      public exprtk::compilation_check
   {
      bool continue_compilation(compilation_context& context)
      override
      {
         // Determine if compilation should continue
         return ...;
      }
   };


An  example checkpoint  use-case could  be that  we do  not want  the
compilation process to take longer than a maximum defined period,  eg:
five seconds. The associated compilation check implementation could be
as follows:

   struct my_compilation_timeout_check final :
      public exprtk::compilation_check
   {

      bool continue_compilation(compilation_context& context)
      override
      {
         static constexpr std::size_t max_iters_per_check = 1000;

         if (++iterations_ >= max_iters_per_check)
         {
            if (std::chrono::steady_clock::now() >= timeout_tp_)
            {
               context.error_message = "Compilation has timed-out";
               return false;
            }

            iterations_ = 0;
         }

         return true;
      }

      using time_point_t = std::chrono::time_point<std::chrono::steady_clock>;

      void set_timeout_time(const time_point_t& timeout_tp)
      {
         timeout_tp_ = timeout_tp;
      }

      std::size_t iterations_ = 0;
      time_point_t timeout_tp_;
   };


Usage of the above defined compilation check will require  registering
the  check  with the  parser,  setting up  the  expiry time  and  then
proceeding  to  compile the  expression.  The following  is  a general
outline of what will be needed:

   typedef exprtk::expression<T> expression_t;
   typedef exprtk::parser<T>     parser_t;

   expression_t expression;

   my_compilation_timeout_check compilation_timeout_check;

   parser_t parser;
   parser.
      register_compilation_timeout_check(compilation_timeout_check);

   const auto max_duration = std::chrono::seconds(5);
   const auto timeout_tp   =
      std::chrono::steady_clock::now() + max_duration;

   compilation_timeout_check.set_timeout_time(timeout_tp);

   if (!parser.compile(large_expression_string, expression))
   {
      printf("Error: %s\t\n", parser.error().c_str());
      return;
   }


(5) Assert statements
ExprTk supports the  use of assert  statements to verify  pre and post
conditions during the evaluation of expressions. The assert statements
are only active when a user defined assert handler is registered  with
the parser before expression compilation, otherwise they are  compiled
out,  this is  similar to  how asserts  are included/excluded  in C++
coupled with  the definition  of NDEBUG.  The assert  syntax has three
variations as described below:

   assert(x + y > i);
   assert(x + y > i, 'assert statement 1');
   assert(x + y > i, 'assert statement 1', 'ASSERT01');


The three assert statement input parameters are as follows:

   1. assert condition (mandatory)
   2. assert message   (optional)
   3. assert id        (optional)


The  assert  condition  is essentially  a  boolean  statement that  is
expected to  be true  during evaluation.  The other  two parameters of
assert message and ID are  string values that are intended  to provide
feedback  to  the  handler  and  to  ensure the  uniqueness  of assert
statement respectively.  The three  parameters denoted  above and  the
offset of the  assert statement from  the beginning of  the expression
are  placed  inside  assert_context that  is provided  as part  of the
assert_check  handler.  A  user defined  assert_check  handler  can be
defined as follows:

   struct my_assert_handler final : public exprtk::assert_check
   {
      void handle_assert(const assert_context& ctxt) override
      {
         printf("condition: [%s] \n", ctxt.condition.c_str());
         printf("message:   [%s] \n", ctxt.message  .c_str());
         printf("id:        [%s] \n", ctxt.id       .c_str());
         printf("offset:    [%lu]\n", ctxt.offet            );
         // throw std::runtime_error(.....);
      }
   };


Once the  assert_check handler  has been  registered with  the parser,
expressions that  contain assert  statements will  have their  asserts
compiled in as part final evaluable expression instance:

   typedef exprtk::symbol_table<T> symbol_table_t;
   typedef exprtk::expression<T>   expression_t;
   typedef exprtk::parser<T>       parser_t;

   const std::string program =
      " var x := 4;                             "
      "                                         "
      " for (var i := 0; i < 10; i += 1)        "
      " {                                       "
      "    assert(i < x, 'assert statement 1'); "
      " }                                       ";

   my_assert_handler handler;

   expression_t expression;
   parser_t parser;

   parser.register_assert_check(handler);
   parser.compile(program, expression);


(6) Runtime Check Overheads
All of the above mentioned runtime checks will incur an execution time
overhead during the evaluation of expressions. This is an  unfortunate
but necessary  side-effect of  the process  when runtime  safety is of
concern.

A recommendation to consider, that is not demonstrated above, is  that
in the check method of the  LRTC, one should not evaluate the  timeout
condition  on  every call  to  check (aka  on  every loop  iteration).
Instead a counter should be maintained  and incremented on each   call
and  when  the  counter  exceeds  some  predefined  amount  (eg: 10000
iterations),  then  the  timeout based  check  can  be preformed.  The
reasoning here  is that  incrementing an  integer should  be far  less
expensive than computing the current "now" time-point.


(7) Runtime Check Limitations
The available  RTC mechanisms  in ExprTk  are limited  to implementing
said checks only within ExprTk based syntax sections of an expression.
The  RTCs  will  not  be  active  within  user  defined  functions, or
composited functions  that have  been compiled  with parser  instances
that don't have the same set of RTC configurations enabled.


(8) Runtime Handlers

When implementing stateful run-time check handlers one must be careful
to ensure the handler is setup correctly or reset between calls to the
expression::value or parser::compile methods.

The following example  code utilises the  compilation timeout RTC  and
expression loop duration  RTC examples from  above to demonstrate  the
need  to  reset the  internal  state of  the  various handlers  before
compilation and valuation processes are invoked, as not doing so  will
affect the ability for  the next expression in  the list to either  be
correctly  compiled or  evaluated due  to the  potential of  erroneous
timeouts occurring.

   typedef exprtk::expression<T> expression_t;
   typedef exprtk::parser<T>     parser_t;

   my_compilation_timeout_check compilation_timeout_check;

   my_loop_rtc loop_rtc;
   loop_rtc.loop_set = loop_runtime_check_t::e_all_loops;
   loop_rtc.max_loop_iterations = 100000;

   parser_t parser;
   parser.register_loop_runtime_check(loop_rtc);
   parser.
      register_compilation_timeout_check(compilation_timeout_check);

   const auto compile_timeout_tp = []()
   {
      const auto max_duration = std::chrono::seconds(5);
      return std::chrono::steady_clock::now() + max_duration;
   };

   const auto loop_timeout_tp = []()
   {
      const auto max_duration = std::chrono::seconds(10);
      return std::chrono::steady_clock::now() + max_duration;
   };

   const std::vector<std::string> expressions =
   {
      "x + y / 2",
      "sin(x) / cos(y) + 1",
      "clamp(-1, sin(2 * pi * x) + cos(y / 2 * pi), +1)"
   };

   for (const auto& expr_str : expressions)
   {
      // Reset the timeout for the compilation RTC
      compilation_timeout_check
         .set_timeout_time(compile_timeout_tp());

      expression_t expression;

      if (!parser.compile(large_expression_string, expression))
      {
         printf("Error: %s\t\n", parser.error().c_str());
         continue;
      }

      try
      {
         // Reset the timeout for the loop duration RTC
         loop_rtc.set_timeout_time(loop_timeout_tp());

         expression.value();
      }
      catch(std::runtime_error& exception)
      {
         printf("Exception: %s\n Expression: %s\n",
                exception.what(),
                expr_str.c_str());
      }
   }


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 25 - BENCHMARKING]
As part of the ExprTk package there is an expression benchmark utility
named 'exprtk_benchmark'. The utility attempts to determine expression
evaluation  speed (or  rate of  evaluations -  evals  per  second), by
evaluating each expression numerous times and mutating the  underlying
variables  of  the  expression between  each  evaluation.  The utility
assumes any  valid ExprTk  expression (containing  conditionals, loops
etc), however  it will  only make  use of  a predefined  set of scalar
variables, namely: a, b, c, x, y, z and w. That being said expressions
themselves  can  contain any  number  of local  variables,  vectors or
strings. There are two modes of operation:

   (1) Default
   (2) User Specified Expressions


(1) Default
The default mode is  enabled simply by executing  the exprtk_benchmark
binary with no command line parameters. In this mode a predefined  set
of expressions will be evaluated in three phases:

   (a) ExprTk evaluation
   (b) Native evaluation
   (c) ExprTk parse


In the first two  phases (a and b)  a list of predefined  (hard-coded)
expressions  will  be  evaluated using  both  ExprTk  and native  mode
implementations.  This  is  done so  as  to  compare evaluation  times
between ExprTk and native implementations. The set of expressions used
are as follows:

   (01) (y + x)
   (02) 2 * (y + x)
   (03) (2 * y + 2 * x)
   (04) ((1.23 * x^2) / y) - 123.123
   (05) (y + x / y) * (x - y / x)
   (06) x / ((x + y) + (x - y)) / y
   (07) 1 - ((x * y) + (y / x)) - 3
   (08) (5.5 + x) + (2 * x - 2 / 3 * y) * (x / 3 + y / 4) + (y + 7.7)
   (09) 1.1x^1 + 2.2y^2 - 3.3x^3 + 4.4y^15 - 5.5x^23 + 6.6y^55
   (10) sin(2 * x) + cos(pi / y)
   (11) 1 - sin(2 * x) + cos(pi / y)
   (12) sqrt(111.111 - sin(2 * x) + cos(pi / y) / 333.333)
   (13) (x^2 / sin(2 * pi / y)) - x / 2
   (14) x + (cos(y - sin(2 / x * pi)) - sin(x - cos(2 * y / pi))) - y
   (15) clamp(-1.0, sin(2 * pi * x) + cos(y / 2 * pi), +1.0)
   (16) max(3.33, min(sqrt(1 - sin(2 * x) + cos(pi / y) / 3), 1.11))
   (17) if((y + (x * 2.2)) <= (x + y + 1.1), x - y, x*y) + 2 * pi / x


The  third  and  final  phase  (c),  is  used  to  determine   average
compilation rates  (compiles per  second) for  expressions of  varying
complexity. Each expression is compiled 100K times and the average for
each expression is output.


(2) User Specified Expressions
In this mode two parameters are passed to the utility via the  command
line:

   (a) A name of a text file containing one expression per line
   (b) An integer representing the number of evaluations per expression


An  example execution  of the  benchmark utility  in this  mode is  as
follows:

   ./exprtk_benchmark my_expressions.txt 1000000


The  above  invocation  will  load  the  expressions  from  the   file
'my_expressions.txt' and will then proceed to evaluate each expression
one million  times, varying  the above  mentioned variables  (x, y,  z
etc.) between each evaluation, and at the end of each expression round
a print out of  running times, result of a single evaluation and total
sum of results is provided as demonstrated below:

   Expression 1 of 7 4.770 ns 47700 ns  ( 9370368.0) '((((x+y)+z)))'
   Expression 2 of 7 4.750 ns 47500 ns  ( 1123455.9) '((((x+y)-z)))'
   Expression 3 of 7 4.766 ns 47659 ns  (21635410.7) '((((x+y)*z)))'
   Expression 4 of 7 5.662 ns 56619 ns  ( 1272454.9) '((((x+y)/z)))'
   Expression 5 of 7 4.950 ns 49500 ns  ( 4123455.9) '((((x-y)+z)))'
   Expression 6 of 7 7.581 ns 75810 ns  (-4123455.9) '((((x-y)-z)))'
   Expression 7 of 7 4.801 ns 48010 ns  (       0.0) '((((x-y)*z)))'


The benchmark utility can be very useful when investigating evaluation
efficiency  issues with  ExprTk or  simply during  the prototyping  of
expressions. As an example, lets take the following expression:

   1 / sqrt(2x) * e^(3y)


Lets say we would like to determine which sub-part  of the  expression
takes the  most time  to evaluate  and perhaps  attempt to  rework the
expression based on the results. In order to do this we will create  a
text file  called 'test.txt'  and then  proceed to  make some educated
guesses  about  how  to  break   the  expression  up  into  its   more
'interesting' sub-parts which we will  then add as one expression  per
line to the file. An example breakdown may be as follows:

   1 / sqrt(2x) * e^(3y)
   1 / sqrt(2x)
   e^(3y)


The  benchmark with  the given  file, where  each expression  will be
evaluated 100K times can be executed as follows:

   ./exprtk_benchmark test.txt 100000
   Expr 1 of 3 90.340 ns 9034000 ns (296417859.3) '1/sqrt(2x)*e^(3y)'
   Expr 2 of 3 11.100 ns 1109999 ns (    44267.3) '1/sqrt(2x)'
   Expr 3 of 3 77.830 ns 7783000 ns (615985286.6) 'e^(3y)'
   [*] Number Of Evals:         300000
   [*] Total Time:              0.018sec
   [*] Total Single Eval Time:  0.000ms


From the results above we conclude that the third expression  (e^(3y))
consumes the largest amount of time. The variable 'e', as used in both
the  benchmark  and  in the  expression,  is  an approximation  of the
transcendental mathematical constant e (2.71828182845904...) hence the
sub-expression  should perhaps be  modified  to use the generally more
efficient built-in 'exp' function.

   ./exprtk_benchmark test.txt 1000000
   Expr 1 of 5 86.563 ns 8656300ns (296417859.6) '1/sqrt(2x)*e^(3y)'
   Expr 2 of 5 40.506 ns 4050600ns (296417859.6) '1/sqrt(2x)*exp(3y)'
   Expr 3 of 5 14.248 ns 1424799ns (    44267.2) '1/sqrt(2x)'
   Expr 4 of 5 88.840 ns 8884000ns (615985286.9) 'e^(3y)'
   Expr 5 of 5 29.267 ns 2926699ns (615985286.9) 'exp(3y)'
   [*] Number Of Evals:        5000000
   [*] Total Time:             0.260sec
   [*] Total Single Eval Time: 0.000ms


The above output demonstrates the  results from  making the previously
mentioned modification to the expression. As can be seen the new  form
of the expression using the 'exp' function reduces the evaluation time
by over 50%, in other words increases the evaluation rate by two fold.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 26 - EXPRTK NOTES]
The following is a list of facts and suggestions one may want to take
into account when using ExprTk:

 (00) Precision  and performance  of expression  evaluations are  the
      dominant principles of the ExprTk library.

 (01) ExprTk  uses a  rudimentary imperative  programming model  with
      syntax based  on languages  such as  Pascal and  C. Furthermore
      ExprTk  is  an LL(2)  type  grammar and  is  processed using  a
      recursive descent parsing algorithm.

 (02) Supported types  are float,  double, long  double and MPFR/GMP.
      Generally any user defined numerical type that supports all the
      basic floating  point arithmetic  operations: -,+,*,/,^,%  etc;
      unary and binary operations: sin,cos,min,max,equal etc and  any
      other ExprTk dependent operations can be used to specialise the
      various components: expression, parser and symbol_table.

 (03) Standard arithmetic operator precedence is applied (BEDMAS). In
      general C, Pascal or Rust equivalent unary, binary, logical and
      equality/inequality operator precedence rules apply.
      eg: a == b and c > d + 1  --->  (a == b) and (c > (d + 1))
          x - y <= z / 2        --->  (x - y) <= (z / 2)
          a - b / c * d^2^3     --->  a - ((b / c) * d^(2^3))

 (04) Results of expressions that are deemed as being 'valid' are  to
      exist within the set of Real numbers. All other results will be
      of  the  value:  Not-A-Number  (NaN).  However  this  may   not
      necessarily be a requirement for user defined numerical  types,
      eg: complex number type.

 (05) Supported   user  defined   types   are   numeric  and   string
      variables, numeric vectors and functions.

 (06) All  reserved words,  keywords, variable,  vector, string   and
      function names are case-insensitive.

 (07) Variable, vector, string variable and function names must begin
      with  a letter  (A-Z or  a-z), then  can  be  comprised of  any
      combination of letters, digits, underscores and dots, ending in
      either a letter (A-Z or a-z), digit or underscore. (eg: x,  y2,
      var1,  power_func99,  person.age,  item.size.0). The associated
      regex pattern is: [a-zA-Z][a-zA-Z0-9_.]*[a-zA-Z0-9_]+

 (08) Expression lengths and sub-expression lists are limited only by
      storage capacity.

 (09) The  life-time of  objects registered  with or  created from  a
      specific symbol-table must  span at least  the lifetime of  the
      symbol  table  instance  and  all  compiled  expressions  which
      utilise objects, such as variables, strings, vectors,  function
      compositor  functions  and  functions  of  that   symbol-table,
      otherwise the result will be undefined behaviour.

 (10) Equal and not_equal  are  normalised-epsilon equality routines,
      which use epsilons of 0.0000000001 and 0.000001 for double  and
      float types respectively.

 (11) All trigonometric functions  assume radian input  unless stated
      otherwise.

 (12) Expressions may contain  white-space characters such  as space,
      tabs, new-lines, control-feed et al.
      ('\n', '\r', '\t', '\b', '\v', '\f')

 (13) Strings may be comprised of any  combination of letters, digits
      special characters including (~!@#$%^&*()[]|=+ ,./?<>;:"`~_) or
      hexadecimal escaped sequences (eg: \0x30) and must be enclosed
      with single-quotes.
      eg: 'Frankly my dear, \0x49 do n0t give a damn!'

 (14) User defined  normal functions  can have  up to  20 parameters,
      where as  user defined  generic-functions and  vararg-functions
      can have an unlimited number of parameters.

 (15) The inbuilt polynomial functions can be at most of degree 12.

 (16) Where appropriate constant folding optimisations may be applied.
      (eg: The expression '2 + (3 - (x / y))' becomes '5 - (x / y)')

 (17) If the strength  reduction compilation option has  been enabled,
      then where applicable  strength reduction  optimisations  may be
      applied.

 (18) String  processing capabilities  are available  by default.  To
      turn them  off, the  following needs  to be  defined at compile
      time: exprtk_disable_string_capabilities

 (19) Composited functions can call themselves or any other functions
      that have been defined prior to their own definition.

 (20) Recursive calls made from within composited functions will have
      a stack size bound by the stack of the executing architecture.

 (21) User  defined functions  by default  are assumed  to have  side
      effects. As such an "all constant parameter" invocation of such
      functions wont result in constant folding. If the function  has
      no side-effects then that  can be noted during the  constructor
      of  the  ifunction  allowing it  to  be  constant folded  where
      appropriate.

 (22) The entity relationship between symbol_table and an  expression
      is many-to-many. However the intended 'typical' use-case  where
      possible, is to have a single symbol table manage the  variable
      and function requirements of multiple expressions.

 (23) The common use-case  for an expression  is to have  it compiled
      only  ONCE  and  then subsequently  have it  evaluated multiple
      times. An extremely  inefficient and suboptimal  approach would
      be to recompile an expression  from its string form every  time
      it requires evaluating.

 (24) It is  strongly recommended  that  the  return value  of method
      invocations from  the parser  and symbol_table  types be  taken
      into account. Specifically the  'compile' method of the  parser
      and the 'add_xxx'  set of methods  of the symbol_table  as they
      denote either the success or failure state of the invoked call.
      Continued processing from  a failed  state without having first
      rectified the underlying issue  will in turn result  in further
      failures and undefined behaviours.

 (25) The following are examples of compliant floating point value
      representations:

      (01) 12345         (06) -123.456
      (02) +123.456e+12  (07) 123.456E-12
      (03) +012.045e+07  (08) .1234
      (04) 1234.         (09) -56789.
      (05) 123.456f      (10) -321.654E+3L

 (26) Expressions may contain any of the following comment styles:

      (1) // .... \n
      (2) #  .... \n
      (3) /* .... */

 (27) The  'null'  value  type  is  a  special  non-zero  type   that
      incorporates specific semantics when undergoing operations with
      the standard numeric type. The following is a list of type  and
      boolean results associated with the use of 'null':

      (1) null  +,-,*,/,%  x    --> x
      (2) x     +,-,*,/,%  null --> x
      (3) null  +,-,*,/,%  null --> null
      (4) null     ==      null --> true
      (5) null     ==      x    --> true
      (6) x        ==      null --> true
      (7) x        !=      null --> false
      (8) null     !=      null --> false
      (9) null     !=      x    --> false

 (28) The following is a list  of reserved words and symbols  used by
      ExprTk. Attempting to  add a variable  or custom function  to a
      symbol table using any of  the reserved words will result  in a
      failure.

      abs, acos,  acosh, and, asin,  asinh,  assert, atan,  atan2,
      atanh, avg, break, case,  ceil, clamp, continue, cosh,  cos,
      cot,  csc, default,  deg2grad, deg2rad,  else, equal,  erfc,
      erf, exp, expm1, false,  floor, for, frac, grad2deg,  hypot,
      iclamp, if, ilike, in, inrange, in, like, log, log10, log1p,
      log2, logn, mand, max, min, mod, mor, mul, nand, ncdf,  nor,
      not, not_equal, not, null, or, pow, rad2deg, repeat, return,
      root, roundn, round,  sec, sgn, shl,  shr, sinc, sinh,  sin,
      sqrt, sum, swap, switch, tanh, tan, true, trunc, until, var,
      while, xnor, xor

 (29) Every valid ExprTk statement is a "value returning" expression.
      Unlike some languages that limit the types of expressions  that
      can be  performed in  certain situations,  in ExprTk  any valid
      expression can be used in any "value consuming" context. eg:

      var y := 3;
      for (var x := switch
                    {
                      case 1  :  7;
                      case 2  : -1 + ~{var x{};};
                      default :  y > 2 ? 3 : 4;
                    };
           x != while (y > 0) { y -= 1; };
           x -= {
                  if (min(x,y) < 2 * max(x,y))
                    x + 2;
                  else
                    x + y - 3;
                }
          )
      {
        (x + y) / (x - y);
      };

 (30) It is recommended when prototyping expressions that the  ExprTk
      REPL be utilised, as it supports all the features available  in
      the library,  including complete  error analysis,  benchmarking
      and   dependency   dumps    etc   which   allows    for   rapid
      coding/prototyping  and  debug  cycles  without  the  hassle of
      having to  recompile test  programs with  expressions that have
      been hard-coded. It is also a  good source of truth for how the
      library's various features can be applied.

 (31) For performance considerations,  one should assume  the actions
      of expression, symbol  table and parser  instance instantiation
      and destruction, and the expression compilation process  itself
      to be of high latency. Hence none of them should be part of any
      performance  critical  code  paths, and  should  instead  occur
      entirely either before or after such code paths.

 (32) Deep  copying  an  expression  instance  for  the  purposes  of
      persisting to disk or otherwise transmitting elsewhere with the
      intent to 'resurrect' the  expression instance later on  is not
      possible due  to the  reasons described  in the  final note  of
      Section 10. The recommendation is to instead simply persist the
      string form  of the  expression and  compile the  expression at
      run-time on the target.

 (33) The  correctness  and  robustness  of  the  ExprTk  library  is
      maintained by having  a comprehensive suite  of unit tests  and
      functional tests all of  which are run using  sanitizers (ASAN,
      UBSAN, LSAN, MSAN, TSAN). Additionally, continuous fuzz-testing
      provided by Google  OSS Fuzz, and static analysis via  Synopsis
      Coverity.

 (34) The library  name ExprTk  is pronounced  "Ex-Pee-Ar-Tee-Kay" or
      simply "Mathematical Expression Toolkit"


 (35) For general support, inquires or bug/issue reporting:
      https://www.partow.net/programming/exprtk/index.html#support

 (36) Before jumping in and using ExprTk, do take the time to  peruse
      the documentation and all of the examples, both in the main and
      the extras  distributions. Having  an informed  general view of
      what can and  can't be done,  and how something  should be done
      with ExprTk, will  likely result in  a far more  productive and
      enjoyable programming experience.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 27 - SIMPLE EXPRTK EXAMPLE]
The following is a  simple yet complete example  demonstrating typical
usage of the ExprTk Library.  The example instantiates a symbol  table
object, adding to it  three variables named x,  y and z, and  a custom
user defined function, that accepts only two parameters, named myfunc.
The  example then  proceeds to  instantiate an  expression object  and
register to it the symbol table instance.

A parser is  then instantiated, and  the string representation  of the
expression  and  the  expression object  are  passed  to the  parser's
compile  method   for  compilation.   If  an   error  occurred  during
compilation, the compile method will return false, leading to a series
of  error diagnostics  being printed  to stdout.  Otherwise the  newly
compiled expression is evaluated  by invoking  the expression object's
value method, and subsequently printing the result  of the computation
to stdout.


--- snip ---
#include <cstdio>
#include <string>

#include "exprtk.hpp"

template <typename T>
struct myfunc final : public exprtk::ifunction<T>
{
   myfunc() : exprtk::ifunction<T>(2) {}

   T operator()(const T& v1, const T& v2) override
   {
      return T(1) + (v1 * v2) / T(3);
   }
};

int main()
{
   typedef exprtk::symbol_table<double> symbol_table_t;
   typedef exprtk::expression<double>   expression_t;
   typedef exprtk::parser<double>       parser_t;
   typedef exprtk::parser_error::type   error_t;

   const std::string expression_string =
      "z := 2 myfunc([4 + sin(x / pi)^3],y ^ 2)";

   double x = 1.1;
   double y = 2.2;
   double z = 3.3;

   myfunc<double> mf;

   symbol_table_t symbol_table;
   symbol_table.add_constants();
   symbol_table.add_variable("x",x);
   symbol_table.add_variable("y",y);
   symbol_table.add_variable("z",z);
   symbol_table.add_function("myfunc",mf);

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   parser_t parser;

   if (!parser.compile(expression_string,expression))
   {
      // A compilation error has occurred. Attempt to
      // print all errors to stdout.

      printf("Error: %s\tExpression: %s\n",
             parser.error().c_str(),
             expression_string.c_str());

      for (std::size_t i = 0; i < parser.error_count(); ++i)
      {
         // Include the specific nature of each error
         // and its position in the expression string.

         error_t error = parser.get_error(i);

         printf("Error: %02d Position: %02d "
                "Type: [%s] "
                "Message: %s "
                "Expression: %s\n",
                static_cast<int>(i),
                static_cast<int>(error.token.position),
                exprtk::parser_error::to_str(error.mode).c_str(),
                error.diagnostic.c_str(),
                expression_string.c_str());
      }

      return 1;
   }

   // Evaluate the expression and obtain its result.

   double result = expression.value();

   printf("Result: %10.5f\n",result);

   return 0;
}
--- snip ---

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 28 - BUILD OPTIONS]
When building ExprTk there are a number of defines that will enable or
disable certain features and  capabilities. The defines can  either be
part of a compiler command line switch or scoped around the include to
the ExprTk header. The defines are as follows:

   (01) exprtk_enable_debugging
   (02) exprtk_disable_cardinal_pow_optimisation
   (03) exprtk_disable_comments
   (04) exprtk_disable_break_continue
   (05) exprtk_disable_sc_andor
   (06) exprtk_disable_return_statement
   (07) exprtk_disable_enhanced_features
   (08) exprtk_disable_string_capabilities
   (09) exprtk_disable_superscalar_unroll
   (10) exprtk_disable_rtl_io
   (11) exprtk_disable_rtl_io_file
   (12) exprtk_disable_rtl_vecops
   (13) exprtk_disable_caseinsensitivity
   (14) exprtk_enable_range_runtime_checks

(01) exprtk_enable_debugging
This define will enable printing of debug information to stdout during
the compilation process.

(02) exprtk_disable_cardinal_pow_optimisation
This  define   will  disable  the optimisation  invoked when  constant
integers are used as powers in exponentiation expressions (eg: x^7).

(03) exprtk_disable_comments
This define will disable the ability for expressions to have comments.
Expressions that have comments when parsed with a build that has  this
option, will result in a compilation failure.

(04) exprtk_disable_break_continue
This  define  will  disable  the  loop-wise  'break'  and   'continue'
capabilities. Any expression that contains those keywords will  result
in a compilation failure.

(05) exprtk_disable_sc_andor
This define  will disable  the short-circuit  '&' (and)  and '|'  (or)
operators

(06) exprtk_disable_return_statement
This define will disable use of return statements within expressions.

(07) exprtk_disable_enhanced_features
This  define  will  disable all  enhanced  features  such as  strength
reduction and special  function optimisations and  expression specific
type instantiations.  This feature  will reduce  compilation times and
binary sizes but will  also result in massive  performance degradation
of expression evaluations.

(08) exprtk_disable_string_capabilities
This  define  will  disable all  string  processing  capabilities. Any
expression that contains a string or string related syntax will result
in a compilation failure.

(09) exprtk_disable_superscalar_unroll
This define will set  the loop unroll batch  size to 4 operations  per
loop  instead of  the default  8 operations.  This define  is used  in
operations that  involve vectors  and aggregations  over vectors. When
targeting  non-superscalar  architectures, it  may  be recommended  to
build using this particular option if efficiency of evaluations is  of
concern.

(10) exprtk_disable_rtl_io
This define will  disable all of  basic IO RTL  package features. When
present, any attempt to register the basic IO RTL package with a given
symbol table will fail causing a compilation error.

(11) exprtk_disable_rtl_io_file
This  define will  disable  the  file I/O  RTL package  features. When
present, any  attempts to register  the file I/O package with  a given
symbol table will fail causing a compilation error.

(12) exprtk_disable_rtl_vecops
This define will  disable the extended  vector operations RTL  package
features. When present, any attempts to register the vector operations
package with  a given  symbol table  will fail  causing a  compilation
error.

(13) exprtk_disable_caseinsensitivity
This define  will disable  case-insensitivity when  matching variables
and  functions. Furthermore  all reserved  and keywords  will only  be
acknowledged when in all lower-case.

(14) exprtk_enable_range_runtime_checks
This define will enable run-time checks pertaining to vector  indexing
operations used  in any  of the  vector-to-vector and vector-to-scalar
operations.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 29 - FILES]
The source distribution of ExprTk is comprised of the following set of
files:

   (00) Makefile
   (01) readme.txt
   (02) exprtk.hpp
   (03) exprtk_test.cpp
   (04) exprtk_benchmark.cpp
   (05) exprtk_simple_example_01.cpp
   (06) exprtk_simple_example_02.cpp
   (07) exprtk_simple_example_03.cpp
   (08) exprtk_simple_example_04.cpp
   (09) exprtk_simple_example_05.cpp
   (10) exprtk_simple_example_06.cpp
   (11) exprtk_simple_example_07.cpp
   (12) exprtk_simple_example_08.cpp
   (13) exprtk_simple_example_09.cpp
   (14) exprtk_simple_example_10.cpp
   (15) exprtk_simple_example_11.cpp
   (16) exprtk_simple_example_12.cpp
   (17) exprtk_simple_example_13.cpp
   (18) exprtk_simple_example_14.cpp
   (19) exprtk_simple_example_15.cpp
   (20) exprtk_simple_example_16.cpp
   (21) exprtk_simple_example_17.cpp
   (22) exprtk_simple_example_18.cpp
   (23) exprtk_simple_example_19.cpp
   (24) exprtk_simple_example_20.cpp
   (25) exprtk_simple_example_21.cpp
   (26) exprtk_simple_example_22.cpp
   (27) exprtk_simple_example_23.cpp
   (28) exprtk_simple_example_24.cpp


Details for each of the above examples can be found here:

  https://www.partow.net/programming/exprtk/index.html#examples


Various extended and advanced examples using ExprTk are available
via the following:

   (00) exprtk_american_option_binomial_model.cpp
   (01) exprtk_binomial_coefficient.cpp
   (02) exprtk_bsm_benchmark.cpp
   (03) exprtk_calc.cpp
   (04) exprtk_collatz.cpp
   (05) exprtk_compilation_timeout.cpp
   (06) exprtk_exprgen.cpp
   (07) exprtk_extract_dependents.cpp
   (08) exprtk_e_10kdigits.cpp
   (09) exprtk_factorize_fermat.cpp
   (10) exprtk_factorize_pollard.cpp
   (11) exprtk_fizzbuzz.cpp
   (12) exprtk_funcall_benchmark.cpp
   (13) exprtk_game_of_life.cpp
   (14) exprtk_gcd.cpp
   (15) exprtk_gnuplot.cpp
   (16) exprtk_gnuplot_multi.cpp
   (17) exprtk_groups_examples.cpp
   (18) exprtk_immutable_symbol_table_example.cpp
   (19) exprtk_import_packages.cpp
   (20) exprtk_instruction_primer.cpp
   (21) exprtk_jump_diffusion_process.cpp
   (22) exprtk_loop_timeout_rtc.cpp
   (23) exprtk_magic_square.cpp
   (24) exprtk_mandelbrot.cpp
   (25) exprtk_max_subarray_sum.cpp
   (26) exprtk_maze_generator.cpp
   (27) exprtk_miller_rabin_primality_test.cpp
   (28) exprtk_montecarlo_e.cpp
   (29) exprtk_montecarlo_option_pricing_model.cpp
   (30) exprtk_montecarlo_pi.cpp
   (31) exprtk_naive_primes.cpp
   (32) exprtk_normal_random_marsaglia_method.cpp
   (33) exprtk_nqueens_problem.cpp
   (34) exprtk_nthroot_bisection.cpp
   (35) exprtk_ornstein_uhlenbeck_process.cpp
   (36) exprtk_pascals_triangle.cpp
   (37) exprtk_pi_10kdigits.cpp
   (38) exprtk_prime_sieve.cpp
   (39) exprtk_prime_sieve_vectorized.cpp
   (40) exprtk_pythagorean_triples.cpp
   (41) exprtk_recursive_fibonacci.cpp
   (42) exprtk_repl.cpp
   (43) exprtk_riddle.cpp
   (44) exprtk_rtc_overhead.cpp
   (45) exprtk_sudoku_solver.cpp
   (46) exprtk_sumofprimes.cpp
   (47) exprtk_symtab_functions.cpp
   (48) exprtk_testgen.cpp
   (49) exprtk_tower_of_hanoi.cpp
   (50) exprtk_truthtable_gen.cpp
   (51) exprtk_vectorized_binomial_model.cpp
   (52) exprtk_vectornorm.cpp
   (53) exprtk_vector_benchmark.cpp
   (54) exprtk_vector_benchmark_multithreaded.cpp
   (55) exprtk_vector_resize_example.cpp
   (56) exprtk_vector_resize_inline_example.cpp
   (57) exprtk_wiener_process_pi.cpp


Details for each of the above examples can be found here:

   https://partow.net/programming/exprtk/index.html#variousexamples

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[SECTION 30 - LANGUAGE STRUCTURE]
The following  are the  various language  structures available  within
ExprTk and their structural representations.

   (00) If Statement
   (01) Else Statement
   (02) Ternary Statement
   (03) While Loop
   (04) Repeat Until Loop
   (05) For Loop
   (06) Switch Statement
   (07) Multi Subexpression Statement
   (08) Multi Case-Consequent Statement
   (09) Variable Definition Statement
   (10) Vector Definition Statement
   (11) String Definition Statement
   (12) Range Statement
   (13) Return Statement


(00) - If Statement
+-------------------------------------------------------------+
|                                                             |
|   [if] ---> [(] ---> [condition] -+-> [,] -+                |
|                                   |        |                |
|   +---------------<---------------+        |                |
|   |                                        |                |
|   |  +------------------<------------------+                |
|   |  |                                                      |
|   |  +--> [consequent] ---> [,] ---> [alternative] ---> [)] |
|   |                                                         |
|   +--> [)] --+-> [{] ---> [expression*] ---> [}] --+        |
|              |                                     |        |
|              |                +---------<----------+        |
|   +----<-----+                |                             |
|   |                           v                             |
|   +--> [consequent] --> [;] -{*}-> [else-statement]         |
|                                                             |
+-------------------------------------------------------------+


(01) - Else Statement
+-------------------------------------------------------------+
|                                                             |
|   [else] -+-> [alternative] ---> [;]                        |
|           |                                                 |
|           +--> [{] ---> [expression*] ---> [}]              |
|           |                                                 |
|           +--> [if-statement]                               |
|                                                             |
+-------------------------------------------------------------+


(02) - Ternary Statement
+-------------------------------------------------------------+
|                                                             |
|   [condition] ---> [?] ---> [consequent] ---> [:] --+       |
|                                                     |       |
|   +------------------------<------------------------+       |
|   |                                                         |
|   +--> [alternative] --> [;]                                |
|                                                             |
+-------------------------------------------------------------+


(03) - While Loop
+-------------------------------------------------------------+
|                                                             |
|   [while] ---> [(] ---> [condition] ---> [)] ---+           |
|                                                 |           |
|   +----------------------<----------------------+           |
|   |                                                         |
|   +--> [{] ---> [expression*] ---> [}]                      |
|                                                             |
+-------------------------------------------------------------+


(04) - Repeat Until Loop
+-------------------------------------------------------------+
|                                                             |
|   [repeat] ---> [expression*] ---+                          |
|                                  |                          |
|   +--------------<---------------+                          |
|   |                                                         |
|   +--> [until] ---> [(] ---> [condition] --->[)]            |
|                                                             |
+-------------------------------------------------------------+


(05) - For Loop
+-------------------------------------------------------------+
|                                                             |
|   [for] ---> [(] -+-> [initialise expression] --+--+        |
|                   |                             |  |        |
|                   +------------->---------------+  v        |
|                                                    |        |
|   +-----------------------<------------------------+        |
|   |                                                         |
|   +--> [;] -+-> [condition] -+-> [;] ---+                   |
|             |                |          |                   |
|             +------->--------+          v                   |
|                                         |                   |
|   +------------------<---------+--------+                   |
|   |                            |                            |
|   +--> [increment expression] -+-> [)] --+                  |
|                                          |                  |
|   +------------------<-------------------+                  |
|   |                                                         |
|   +--> [{] ---> [expression*] ---> [}]                      |
|                                                             |
+-------------------------------------------------------------+


(06) - Switch Statement
+-------------------------------------------------------------+
|                                                             |
|   [switch] ---> [{] ---+                                    |
|                        |                                    |
|   +---------<----------+-----------<-----------+            |
|   |                                            |            |
|   +--> [case] ---> [condition] ---> [:] ---+   |            |
|                                            |   |            |
|   +-------------------<--------------------+   |            |
|   |                                            |            |
|   +--> [consequent] ---> [;] --------->--------+            |
|   |                                            |            |
|   |                                            |            |
|   +--> [default] ---> [consequent] ---> [;] ---+            |
|   |                                            |            |
|   +---------------------<----------------------+            |
|   |                                                         |
|   +--> [}]                                                  |
|                                                             |
+-------------------------------------------------------------+


(07) - Multi Subexpression Statement
+-------------------------------------------------------------+
|                                                             |
|                   +--------------<---------------+          |
|                   |                              |          |
|   [~] ---> [{\(] -+-> [expression] -+-> [;\,] ---+          |
|                                     |                       |
|   +----------------<----------------+                       |
|   |                                                         |
|   +--> [}\)]                                                |
|                                                             |
+-------------------------------------------------------------+


(08) - Multi Case-Consequent Statement
+-------------------------------------------------------------+
|                                                             |
|   [[*]] ---> [{] ---+                                       |
|                     |                                       |
|   +--------<--------+--------------<----------+             |
|   |                                           |             |
|   +--> [case] ---> [condition] ---> [:] ---+  |             |
|                                            |  |             |
|   +-------------------<--------------------+  |             |
|   |                                           |             |
|   +--> [consequent] ---> [;] ---+------>------+             |
|                                 |                           |
|                                 +--> [}]                    |
|                                                             |
+-------------------------------------------------------------+


(09) - Variable Definition Statement
+-------------------------------------------------------------+
|                                                             |
|   [var] ---> [symbol] -+-> [:=] -+-> [expression] -+-> [;]  |
|                        |         |                 |        |
|                        |         +-----> [{}] -->--+        |
|                        |                           |        |
|                        +------------->-------------+        |
|                                                             |
+-------------------------------------------------------------+


(10) - Vector Definition Statement
+-------------------------------------------------------------+
|                                                             |
|   [var] ---> [symbol] ---> [[] ---> [constant] ---> []] --+ |
|                                                           | |
|   +---------------------------<---------------------------+ |
|   |                                                         |
|   |                   +--------->---------+                 |
|   |                   |                   |                 |
|   +--> [:=] ---> [{] -+-+-> [expression] -+-> [}] ---> [;]  |
|                         |                 |                 |
|                         +--<--- [,] <-----+                 |
|                                                             |
+-------------------------------------------------------------+


(11) - String Definition Statement
+-------------------------------------------------------------+
|                                                             |
|   [var] --> [symbol] --> [:=] --> [str-expression] ---> [;] |
|                                                             |
+-------------------------------------------------------------+


(12) - Range Statement
+-------------------------------------------------------------+
|                                                             |
|      +-------->--------+                                    |
|      |                 |                                    |
| [[] -+-> [expression] -+-> [:] -+-> [expression] -+--> []]  |
|                                 |                 |         |
|                                 +-------->--------+         |
|                                                             |
+-------------------------------------------------------------+


(13) - Return Statement
+-------------------------------------------------------------+
|                                                             |
|   [return] ---> [[] -+-> [expression] -+-> []] ---> [;]     |
|                      |                 |                    |
|                      +--<--- [,] <-----+                    |
|                                                             |
+-------------------------------------------------------------+
