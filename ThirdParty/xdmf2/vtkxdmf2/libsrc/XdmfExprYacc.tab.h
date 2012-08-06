/* A Bison parser, made by GNU Bison 1.875a.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     lFLOAT = 258,
     tokINTEGER = 259,
     tokARRAY = 260,
     NAME = 261,
     SIN = 262,
     COS = 263,
     TAN = 264,
     ACOS = 265,
     ASIN = 266,
     ATAN = 267,
     LOG = 268,
     EXP = 269,
     ABS_TOKEN = 270,
     SQRT = 271,
     WHERE = 272,
     INDEX = 273,
     EQEQ = 274,
     LT = 275,
     LE = 276,
     GT = 277,
     GE = 278,
     NE = 279,
     LTLT = 280,
     GTGT = 281,
     JOIN = 282
   };
#endif
#define lFLOAT 258
#define tokINTEGER 259
#define tokARRAY 260
#define NAME 261
#define SIN 262
#define COS 263
#define TAN 264
#define ACOS 265
#define ASIN 266
#define ATAN 267
#define LOG 268
#define EXP 269
#define ABS_TOKEN 270
#define SQRT 271
#define WHERE 272
#define INDEX 273
#define EQEQ 274
#define LT 275
#define LE 276
#define GT 277
#define GE 278
#define NE 279
#define LTLT 280
#define GTGT 281
#define JOIN 282




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef union YYSTYPE {
        double          DoubleValue;
        long            IntegerValue;
        void            *ArrayPointer;
        XdmfExprSymbol  *Symbol;
} YYSTYPE;
/* Line 1240 of yacc.c.  */
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE dice_yylval;



