#include <sys/cdefs.h>
#ifndef lint
#if 0
static char yysccsid[] = "@(#)yaccpar 1.9 (Berkeley) 02/21/93";
#else
__IDSTRING(yyrcsid, "$NetBSD: skeleton.c,v 1.14 1997/10/20 03:41:16 lukem Exp $");
#endif
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 16 "vtkParse.y"

/*

This file must be translated to C and modified to build everywhere.

Run yacc like this:

  yacc -b vtkParse vtkParse.y

Modify vtkParse.tab.c:
  - remove TABs
  - remove yyerrorlab stuff in range ["goto yyerrlab1;", "yyerrstatus = 3;")

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1

/* Make sure yacc-generated code knows we have included stdlib.h.  */
#ifndef _STDLIB_H
# define _STDLIB_H
#endif
#define YYINCLUDED_STDLIB_H

/* Map from the type enumeration in vtkType.h to the VTK wrapping type
   system number for the type.  Note that the wrapping type system
   does not enumerate its type values by name.  Look in the
   type_primitive production rule in the grammar for the "official"
   enumeration. */
static int vtkParseTypeMap[] =
  {
   0x2,  /* VTK_VOID                0 */
   0,    /* VTK_BIT                 1 */
   0x3,  /* VTK_CHAR                2 */
   0x13, /* VTK_UNSIGNED_CHAR       3 */
   0x5,  /* VTK_SHORT               4 */
   0x15, /* VTK_UNSIGNED_SHORT      5 */
   0x4,  /* VTK_INT                 6 */
   0x14, /* VTK_UNSIGNED_INT        7 */
   0x6,  /* VTK_LONG                8 */
   0x16, /* VTK_UNSIGNED_LONG       9 */
   0x1,  /* VTK_FLOAT              10 */
   0x7,  /* VTK_DOUBLE             11 */
   0xA,  /* VTK_ID_TYPE            12 */
   0,    /* VTK_STRING             13 */
   0,    /* VTK_OPAQUE             14 */
   0xD,  /* VTK_SIGNED_CHAR        15 */
   0xB,  /* VTK_LONG_LONG          16 */
   0x1B, /* VTK_UNSIGNED_LONG_LONG 17 */
   0xC,  /* VTK___INT64            18 */
   0x1C  /* VTK_UNSIGNED___INT64   19 */
  };

/* Define some constants to simplify references to the table lookup in
   the type_primitive production rule code.  */
#include "vtkType.h"
#define VTK_PARSE_INT8 vtkParseTypeMap[VTK_TYPE_INT8]
#define VTK_PARSE_UINT8 vtkParseTypeMap[VTK_TYPE_UINT8]
#define VTK_PARSE_INT16 vtkParseTypeMap[VTK_TYPE_INT16]
#define VTK_PARSE_UINT16 vtkParseTypeMap[VTK_TYPE_UINT16]
#define VTK_PARSE_INT32 vtkParseTypeMap[VTK_TYPE_INT32]
#define VTK_PARSE_UINT32 vtkParseTypeMap[VTK_TYPE_UINT32]
#define VTK_PARSE_INT64 vtkParseTypeMap[VTK_TYPE_INT64]
#define VTK_PARSE_UINT64 vtkParseTypeMap[VTK_TYPE_UINT64]
#define VTK_PARSE_FLOAT32 vtkParseTypeMap[VTK_TYPE_FLOAT32]
#define VTK_PARSE_FLOAT64 vtkParseTypeMap[VTK_TYPE_FLOAT64]

static void vtkParseDebug(const char* s1, const char* s2);

/* Borland and MSVC do not define __STDC__ properly. */
#if !defined(__STDC__)
# if (defined(_MSC_VER) && _MSC_VER >= 1200) || defined(__BORLANDC__)
#  define __STDC__ 1
# endif
#endif

/* Disable warnings in generated code. */
#if defined(_MSC_VER)
# pragma warning (disable: 4127) /* conditional expression is constant */
# pragma warning (disable: 4244) /* conversion to smaller integer type */
#endif
#if defined(__BORLANDC__)
# pragma warn -8004 /* assigned a value that is never used */
# pragma warn -8008 /* conditional is always true */
# pragma warn -8066 /* unreachable code */
#endif

int yylex(void);
void output_function(void);

/* vtkstrdup is not part of POSIX so we create our own */
char *vtkstrdup(const char *in)
{
  char *res = malloc(strlen(in)+1);
  strcpy(res,in);
  return res;
}

#include "vtkParse.h"
    
  FileInfo data;
  FunctionInfo *currentFunction;

  FILE *fhint;
  char temps[2048];
  int  in_public;
  int  in_protected;
  int  HaveComment;
  char CommentText[50000];
  int CommentState;
  int openSig;
  int invertSig;
  unsigned int sigAllocatedLength;
  
#define YYMAXDEPTH 1000

  void checkSigSize(char *arg)
    {
    if (strlen(currentFunction->Signature) + strlen(arg) + 3 > 
        sigAllocatedLength)
      {
      currentFunction->Signature = (char *)
        realloc(currentFunction->Signature, sigAllocatedLength*2);
      sigAllocatedLength = sigAllocatedLength*2;
      }
    } 
  void preSig(char *arg)
    {
    if (!currentFunction->Signature)
      {
      currentFunction->Signature = (char*)malloc(2048);
      sigAllocatedLength = 2048; 
      sprintf(currentFunction->Signature,"%s",arg);
      }    
    else if (openSig)
      {
      char *tmp;
      checkSigSize(arg);
      tmp = vtkstrdup(currentFunction->Signature);
      sprintf(currentFunction->Signature,"%s%s",arg,tmp);
      free(tmp);
      }
    }
  void postSig(char *arg)
    {
    if (!currentFunction->Signature)
      {
      currentFunction->Signature = (char*)malloc(2048);
      sigAllocatedLength = 2048; 
      sprintf(currentFunction->Signature,"%s",arg);
      }    
    else if (openSig)
      {
      char *tmp;
      checkSigSize(arg);
      tmp = vtkstrdup(currentFunction->Signature);
      if (invertSig)
        {
        sprintf(currentFunction->Signature,"%s%s",arg,tmp);
        }
      else
        {
        sprintf(currentFunction->Signature,"%s%s",tmp,arg);
        }
      free(tmp);
      }
    }
  void delSig(void)
    {
    if (currentFunction->Signature)
      {
      free(currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
    }
  void legacySig(void)
    {
    currentFunction->IsLegacy = 1;
    }
#line 200 "vtkParse.y"
typedef union{
  char *str;
  int   integer;
  } YYSTYPE;
#line 208 "vtkParse.tab.c"
#define CLASS 257
#define PUBLIC 258
#define PRIVATE 259
#define PROTECTED 260
#define VIRTUAL 261
#define STRING 262
#define NUM 263
#define ID 264
#define INT 265
#define FLOAT 266
#define SHORT 267
#define LONG 268
#define LONG_LONG 269
#define INT64__ 270
#define DOUBLE 271
#define VOID 272
#define CHAR 273
#define SIGNED_CHAR 274
#define BOOL 275
#define CLASS_REF 276
#define OTHER 277
#define CONST 278
#define OPERATOR 279
#define UNSIGNED 280
#define FRIEND 281
#define VTK_ID 282
#define STATIC 283
#define VAR_FUNCTION 284
#define ARRAY_NUM 285
#define VTK_LEGACY 286
#define TypeInt8 287
#define TypeUInt8 288
#define TypeInt16 289
#define TypeUInt16 290
#define TypeInt32 291
#define TypeUInt32 292
#define TypeInt64 293
#define TypeUInt64 294
#define TypeFloat32 295
#define TypeFloat64 296
#define IdType 297
#define StdString 298
#define SetMacro 299
#define GetMacro 300
#define SetStringMacro 301
#define GetStringMacro 302
#define SetClampMacro 303
#define SetObjectMacro 304
#define SetReferenceCountedObjectMacro 305
#define GetObjectMacro 306
#define BooleanMacro 307
#define SetVector2Macro 308
#define SetVector3Macro 309
#define SetVector4Macro 310
#define SetVector6Macro 311
#define GetVector2Macro 312
#define GetVector3Macro 313
#define GetVector4Macro 314
#define GetVector6Macro 315
#define SetVectorMacro 316
#define GetVectorMacro 317
#define ViewportCoordinateMacro 318
#define WorldCoordinateMacro 319
#define TypeMacro 320
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    4,    2,    5,    5,    6,    6,    6,    6,    6,
    6,    6,    6,    6,   12,   10,   10,   10,   10,   10,
   10,   10,   10,    9,    9,    9,    9,    9,    9,   16,
   20,   21,   14,   14,   19,   19,   24,   18,   25,   26,
   22,   22,   11,   11,   11,   11,   23,   23,   27,   29,
   27,   28,   32,   28,   28,   31,   31,    8,    8,   30,
   34,   35,   34,   34,   15,   15,   15,   15,   36,   36,
   36,   39,   39,   39,   40,   38,   38,   38,   38,   42,
   37,   37,   41,   41,   41,   41,   41,   41,   41,   41,
   41,   41,   41,   41,   41,   41,   41,   41,   41,   41,
   41,   41,   41,   41,   41,   41,    3,    3,   43,   44,
   43,    7,    7,    7,   33,   33,   45,   45,   45,   46,
   13,   47,   48,   13,   49,   13,   50,   13,   51,   52,
   13,   53,   13,   54,   13,   55,   56,   13,   57,   13,
   58,   13,   59,   13,   60,   13,   61,   13,   62,   13,
   63,   13,   64,   13,   65,   13,   66,   13,   67,   13,
   13,   13,   13,   13,    1,    1,   17,   17,   68,   68,
   69,   69,   69,   69,   69,   69,   69,   69,   69,   69,
   69,   69,   69,   69,   69,   69,   69,   69,   69,   69,
   69,   70,   71,   72,
};
short yylen[] = {                                         2,
    3,    0,    7,    1,    2,    2,    1,    1,    2,    2,
    3,    2,    2,    1,    4,    2,    3,    1,    2,    3,
    4,    3,    2,    1,    2,    3,    4,    3,    2,    3,
    0,    0,    4,    3,    0,    1,    0,    5,    1,    1,
    1,    1,    1,    4,    3,    3,    0,    1,    1,    0,
    4,    1,    0,    4,    1,    0,    2,    3,    2,    2,
    0,    0,    3,    4,    2,    1,    2,    3,    1,    2,
    1,    1,    2,    2,    1,    1,    1,    2,    2,    0,
    3,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    0,    2,    2,    0,
    5,    1,    1,    1,    2,    1,    1,    3,    1,    0,
    7,    0,    0,    8,    0,    5,    0,    5,    0,    0,
   10,    0,    7,    0,    7,    0,    0,    8,    0,    7,
    0,    7,    0,    7,    0,    7,    0,    7,    0,    7,
    0,    7,    0,    7,    0,    7,    0,    9,    0,    9,
    4,    4,    6,    7,    0,    2,    0,    2,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    3,    3,    3,
};
short yydefred[] = {                                      0,
  179,  182,  100,   96,   93,   97,   98,  103,  104,   99,
   94,   95,  105,  106,  183,  171,  186,  187,   80,  101,
  190,  191,   83,   84,   85,   86,   87,   88,   89,   90,
   91,   92,  102,   75,    0,  176,  169,    0,  189,  175,
  177,    0,  184,  174,  188,  178,    0,    0,  180,  181,
   82,    0,  170,  172,  173,  185,    0,    0,    0,    0,
    0,    0,  166,   81,  192,  193,  194,    2,    1,    0,
    0,    0,  112,  113,  114,    0,  108,    0,    0,    0,
    0,   39,    0,    0,    0,   40,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    7,    8,    0,    0,    0,   18,
    0,   24,    0,    0,    0,    0,   66,    0,   71,    0,
    0,    0,   23,    0,   29,    0,    0,    9,    0,    0,
   59,    0,    0,  122,  125,  127,    0,    0,    0,  136,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   42,   41,   16,    3,    5,    6,
    0,    0,   43,   10,   12,   13,    0,   19,   25,    0,
    0,    0,    0,   37,   65,    0,   67,    0,    0,   70,
   73,   74,    0,   17,    0,   22,   28,   30,  168,   11,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  139,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   20,   26,   62,
    0,   60,   58,   34,   36,   32,    0,   68,   78,   79,
  111,   21,   27,    0,   15,    0,  120,    0,    0,    0,
  129,  132,  134,    0,    0,  141,  145,  149,  153,  143,
  147,  151,  155,  157,  159,  161,  162,    0,    0,   46,
    0,    0,   33,   55,    0,    0,   48,    0,    0,    0,
  123,  126,  128,    0,    0,    0,  137,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   44,
   63,    0,    0,   53,   38,    0,    0,    0,  130,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  163,    0,   64,    0,    0,  121,    0,
    0,  133,  135,    0,  140,  142,  146,  150,  154,  144,
  148,  152,  156,    0,    0,  164,    0,   54,   51,  124,
    0,  138,    0,    0,  119,    0,  116,    0,   57,    0,
    0,  115,  158,  160,  131,  118,
};
short yydgoto[] = {                                      47,
   48,   62,   72,   70,  112,  113,   76,  115,  116,  117,
  174,  118,  119,  120,  121,  122,  136,  123,  236,  183,
  273,  124,  276,  237,  125,  126,  277,  278,  306,  181,
  348,  327,  356,  232,  271,  127,   49,  190,  129,   50,
   51,   57,   77,  131,  357,  280,  205,  308,  206,  207,
  284,  331,  285,  286,  211,  312,  255,  289,  293,  290,
  294,  291,  295,  292,  296,  297,  298,   52,   53,   54,
   55,   56,
};
short yysindex[] = {                                    -36,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  -36,    0,    0,  -36,    0,    0,
    0,  -36,    0,    0,    0,    0,    0, -233,    0,    0,
    0,  -36,    0,    0,    0,    0,  816,  -91,    9,  -18,
 -198,  -36,    0,    0,    0,    0,    0,    0,    0,   45,
 -100,  -10,    0,    0,    0, -164,    0,  447,    0,  583,
    0,    0,    7,  520,    0,    0,   79,  107,  111,  115,
  121,  122,  123,  124,  125,  126,  127,  128,  129,  130,
  131,  134,  136,  137,  150,  151,  158,  159,  160,  173,
 -125,   -6,  447,  156,    0,    0,  -41,  -41,  157,    0,
 -220,    0,  154,  178,  747,  712,    0,   -3,    0,   -1,
  175, -125,    0, -129,    0,  161,    7,    0,  -41, -220,
    0,  -86, -125,    0,    0,    0, -125, -125, -125,    0,
 -125, -125, -125, -125, -125, -125, -125, -125, -125, -125,
 -125, -125, -125, -125,    0,    0,    0,    0,    0,    0,
  -36,    7,    0,    0,    0,    0, -137,    0,    0,  -35,
  162,  -40,  -56,    0,    0,  747,    0,   -3,   -3,    0,
    0,    0, -100,    0, -137,    0,    0,    0,    0,    0,
  642,  183, -162,  181, -125, -125, -125,  201,  204,  221,
 -125,    0,  222,  223,  247,  249,  265,  266,  267,  270,
  271,  274,  282,  285,  283,  203,  277,    0,    0,    0,
    7,    0,    0,    0,    0,    0,  677,    0,    0,    0,
    0,    0,    0, -138,    0, -125,    0,  302,  311,  314,
    0,    0,    0,  312,  313,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -125,  299,    0,
  -90,  268,    0,    0, -125,  324,    0,    0, -125,  782,
    0,    0,    0,  782,  782,  782,    0,  782,  782,  782,
  782,  782,  782,  782,  782,  782,  782,  782,   35,    0,
    0,  -90,  -90,    0,    0,  318,  325,  782,    0,  326,
  328,  782,  330,  331,  333,  334,  335,  339,  355,  356,
  357,  358,  359,    0,  360,    0,  338,  677,    0,  363,
  362,    0,    0,  366,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   86,   86,    0,   86,    0,    0,    0,
    7,    0,  354, -190,    0,  367,    0,  368,    0,  369,
  148,    0,    0,    0,    0,    0,
};
short yyrindex[] = {                                    155,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  288,    0,    0,  374,    0,    0,
    0,  327,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   31,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  417,    0,    0,    0,    0,    0,    0,    0,  296,
    0,    0,    0,    0,    0,    0,    0,    0,  -23,    0,
   43,    0,  364,    0,   53,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  297,    0,    0,    0,    0,    0,  321,    0,
    0,    0,   70,    0,    0,    0,    0,   55,    0,   60,
    0,    0,    0,    0,    0,    0,  -27,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  288,  364,    0,    0,    0,    0,    0,    0,    0,  371,
    0,    0,  -29,    0,    0,    0,    0,   65,   81,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  332,    0,    0,    0,    0,    0,  380,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  384,    0,
   73,    0,    0,    0,   64,    0,    0,   66,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   73,    2,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   71,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  385,    0,  102,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
  278,    0,    0,    0,  315,    0,  -71,    0,  343,  -73,
  -82,    0,    0,  284,  -64,  -54, -134,    0,    0,    0,
    0,  230,    0,    0,  305,    0,  104,    0,    0,  163,
    0,    0, -193, -238,    0,  -98,  228, -118,    0,  -65,
  376,    0,  241,    0,   85,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -83,    0,
    0,    0,
};
#define YYTABLESIZE 1113
short yytable[] = {                                     137,
  231,   43,  199,   38,  184,   44,  114,   41,   45,   46,
  139,   35,  130,  167,  130,  134,  172,  173,  130,  140,
  110,   36,   37,   61,   40,  135,  185,  187,   35,   35,
  165,  167,  301,   65,  188,  175,  191,  227,  189,  111,
  192,  114,   61,  165,   43,   61,   38,  130,   44,   66,
   41,   45,   46,  137,   42,  231,  200,  177,   83,  130,
  130,  166,   61,  326,   36,  167,  179,   40,  202,  239,
  240,  165,  353,  165,   67,  324,  130,  203,  325,  197,
  100,  171,   42,   68,  100,  179,   35,  238,  137,   39,
  101,  166,   41,   35,  101,   69,  272,   42,   69,  109,
   72,  165,   71,   72,   52,   76,   49,   52,   76,   50,
   31,   56,   78,   61,   56,  246,   61,   79,  168,  166,
  130,   77,  229,  165,   77,  165,  165,   31,   31,   35,
  354,   61,   39,   61,  165,  130,  244,  141,  165,  279,
  243,   83,  117,  166,  166,  117,  142,  137,  195,   83,
  143,  358,  166,  359,  144,  165,  166,   73,   74,   75,
  145,  146,  147,  148,  149,  150,  151,  152,  153,  154,
  155,  130,  275,  156,  201,  157,  158,   81,    4,    5,
    6,    7,    8,    9,   10,   11,   12,   13,   14,  159,
  160,   82,   31,   19,  230,   85,   86,  161,  162,  163,
   23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
   33,   34,  164,  170,  182,  176,  360,  184,  193,  198,
  233,  235,  234,  245,  247,    1,    2,    3,    4,    5,
    6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
   16,   17,   18,   19,  251,   20,   21,  252,   22,  230,
   23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
   33,   34,  130,  275,  253,  256,  257,  137,    1,    2,
    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,
   13,   14,   15,   16,   17,   18,   19,  165,   20,   21,
  258,   22,  259,   23,   24,   25,   26,   27,   28,   29,
   30,   31,   32,   33,   34,  128,  100,  128,  260,  261,
  262,  128,   58,  263,  264,   59,  101,  265,   69,   60,
  100,  100,  266,   72,  100,  267,  268,  269,   76,   63,
  101,  101,   69,   69,  101,  270,   69,   72,   72,   69,
  128,   72,   76,   76,   77,  281,   76,   31,  353,  165,
  180,  282,  128,  128,  283,  287,  288,  300,   77,   77,
  302,  328,   77,  133,  305,  329,  332,  166,  333,  128,
  335,  336,  204,  337,  338,  339,  208,  209,  210,  340,
  212,  213,  214,  215,  216,  217,  218,  219,  220,  221,
  222,  223,  224,  225,  167,  341,  342,  343,  347,  361,
  346,  344,  345,  350,  178,  351,  352,  363,  364,  365,
  366,  165,  165,  128,  165,  194,  165,  196,  107,  165,
   47,    4,  167,  178,  167,  167,  138,  169,  128,   61,
  186,  349,   64,  241,  248,  249,  250,  304,  362,    0,
  254,    0,    0,    0,    0,   14,   14,    0,  226,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  228,    0,    0,    0,  128,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  242,    0,
    0,    0,    0,    0,  133,    0,  178,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  299,    0,    0,
    0,    0,    0,    0,  303,    0,    0,  307,   45,   45,
    0,  309,  310,  311,    0,  313,  314,  315,  316,  317,
  318,  319,  320,  321,  322,  323,    0,  196,    0,  228,
    0,    0,    0,    0,    0,  330,    0,    0,    0,  334,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  128,    0,    0,    0,    0,
    0,    0,  242,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  111,  355,  355,    0,  355,    0,   14,   14,
   14,   14,    0,  355,   14,   14,   14,   14,   14,   14,
   14,   14,   14,   14,   14,   14,    0,    0,   14,   14,
   14,   14,   14,   14,   14,    0,   14,   14,   14,   14,
   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
   14,   45,   45,   45,   45,  111,    0,   45,   45,   45,
   45,   45,   45,   45,   45,   45,   45,   45,   45,    0,
    0,   45,   45,   45,   45,   45,   45,   45,    0,   45,
   45,   45,   45,   45,   45,   45,   45,   45,   45,   45,
   45,   45,   45,   45,   45,   45,   45,   45,   45,   45,
   45,   45,   45,   45,   45,   45,   45,   45,   45,   45,
   45,   45,   45,   45,   73,   74,   75,   80,  132,    0,
   81,    4,    5,    6,    7,    8,    9,   10,   11,   12,
   13,   14,    0,    0,   82,   83,   19,   84,   85,   86,
   87,    0,   88,   23,   24,   25,   26,   27,   28,   29,
   30,   31,   32,   33,   34,   89,   90,   91,   92,   93,
   94,   95,   96,   97,   98,   99,  100,  101,  102,  103,
  104,  105,  106,  107,  108,  109,  110,  132,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   80,    0,    0,   81,    4,    5,    6,    7,    8,    9,
   10,   11,   12,   13,   14,    0,    0,   82,   83,   19,
    0,   85,   86,    0,    0,    0,   23,   24,   25,   26,
   27,   28,   29,   30,   31,   32,   33,   34,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   81,    4,    5,    6,
    7,    8,    9,   10,   11,   12,   13,   14,    0,    0,
   82,   83,   19,    0,   85,   86,    0,    0,    0,   23,
   24,   25,   26,   27,   28,   29,   30,   31,   32,   33,
   34,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   81,    4,    5,    6,    7,
    8,    9,   10,   11,   12,   13,   14,    0,    0,   82,
    0,   19,    0,   85,   86,    0,    0,    0,   23,   24,
   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,
    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,
   13,   14,    0,    0,   82,    0,   19,    0,   20,   86,
  274,    0,    0,   23,   24,   25,   26,   27,   28,   29,
   30,   31,   32,   33,   34,    3,    4,    5,    6,    7,
    8,    9,   10,   11,   12,   13,   14,    0,    0,   82,
    0,   19,    0,   20,    0,    0,    0,    0,   23,   24,
   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,
    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,
   13,   14,    0,    0,    0,    0,   19,    0,   20,    0,
    0,    0,    0,   23,   24,   25,   26,   27,   28,   29,
   30,   31,   32,   33,   34,    3,    4,    5,    6,    7,
    8,    9,   10,   11,   12,   13,   14,    0,    0,    0,
    0,   19,    0,   20,    0,    0,    0,    0,   23,   24,
   25,   26,   27,   28,   29,   30,   31,   32,   33,    3,
    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,
   14,    0,    0,    0,    0,    0,    0,   20,    0,    0,
    0,    0,   23,   24,   25,   26,   27,   28,   29,   30,
   31,   32,   33,
};
short yycheck[] = {                                      83,
   91,   38,  137,   40,   40,   42,   78,   44,   45,   46,
   84,   41,   78,   41,   80,   80,   58,   59,   84,   84,
   44,   58,   59,  257,   61,   80,  125,  126,   58,   59,
    0,   59,  271,  125,   38,  118,   38,  172,   42,  126,
   42,  113,   41,  264,   38,   44,   40,  113,   42,   41,
   44,   45,   46,  137,   91,   91,  139,  278,  279,  125,
  126,  282,   61,  302,   58,   93,  121,   61,  142,  188,
  189,   41,  263,  264,   93,   41,  142,  142,   44,  134,
   38,  123,   40,  282,   42,  140,  123,  186,  172,  126,
   38,  282,   40,  123,   42,   41,  231,   91,   44,  123,
   41,  264,   58,   44,   41,   41,   41,   44,   44,   44,
   41,   41,  123,   41,   44,  278,   44,  282,  125,  282,
  186,   41,  177,   93,   44,  264,  264,   58,   59,  123,
   45,   59,  126,   61,  264,  201,  201,   59,  264,  278,
  195,  279,   41,  282,  282,   44,   40,  231,  278,  279,
   40,  345,  282,  347,   40,  125,  282,  258,  259,  260,
   40,   40,   40,   40,   40,   40,   40,   40,   40,   40,
   40,  237,  237,   40,  261,   40,   40,  264,  265,  266,
  267,  268,  269,  270,  271,  272,  273,  274,  275,   40,
   40,  278,  123,  280,  285,  282,  283,   40,   40,   40,
  287,  288,  289,  290,  291,  292,  293,  294,  295,  296,
  297,  298,   40,   58,   61,   59,  351,   40,   44,   59,
   59,  278,  263,   41,   44,  262,  263,  264,  265,  266,
  267,  268,  269,  270,  271,  272,  273,  274,  275,  276,
  277,  278,  279,  280,   44,  282,  283,   44,  285,  285,
  287,  288,  289,  290,  291,  292,  293,  294,  295,  296,
  297,  298,  328,  328,   44,   44,   44,  351,  262,  263,
  264,  265,  266,  267,  268,  269,  270,  271,  272,  273,
  274,  275,  276,  277,  278,  279,  280,  257,  282,  283,
   44,  285,   44,  287,  288,  289,  290,  291,  292,  293,
  294,  295,  296,  297,  298,   78,  264,   80,   44,   44,
   44,   84,   35,   44,   44,   38,  264,   44,  264,   42,
  278,  279,   41,  264,  282,   41,   44,  125,  264,   52,
  278,  279,  278,  279,  282,   59,  282,  278,  279,   62,
  113,  282,  278,  279,  264,   44,  282,  278,  263,  264,
  121,   41,  125,  126,   41,   44,   44,   59,  278,  279,
   93,   44,  282,   80,   41,   41,   41,  282,   41,  142,
   41,   41,  143,   41,   41,   41,  147,  148,  149,   41,
  151,  152,  153,  154,  155,  156,  157,  158,  159,  160,
  161,  162,  163,  164,  111,   41,   41,   41,   61,   46,
   41,   44,   44,   41,  121,   44,   41,   41,   41,   41,
  263,  257,  125,  186,   41,  132,    0,  134,  123,   93,
   41,  125,   59,  140,   93,   41,   84,  113,  201,   59,
  126,  328,   57,  193,  205,  206,  207,  275,  354,   -1,
  211,   -1,   -1,   -1,   -1,  125,  126,   -1,  171,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  177,   -1,   -1,   -1,  237,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  195,   -1,
   -1,   -1,   -1,   -1,  201,   -1,  203,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  268,   -1,   -1,
   -1,   -1,   -1,   -1,  275,   -1,   -1,  280,  125,  126,
   -1,  284,  285,  286,   -1,  288,  289,  290,  291,  292,
  293,  294,  295,  296,  297,  298,   -1,  244,   -1,  246,
   -1,   -1,   -1,   -1,   -1,  308,   -1,   -1,   -1,  312,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  328,   -1,   -1,   -1,   -1,
   -1,   -1,  279,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  126,  344,  345,   -1,  347,   -1,  258,  259,
  260,  261,   -1,  354,  264,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  275,   -1,   -1,  278,  279,
  280,  281,  282,  283,  284,   -1,  286,  287,  288,  289,
  290,  291,  292,  293,  294,  295,  296,  297,  298,  299,
  300,  301,  302,  303,  304,  305,  306,  307,  308,  309,
  310,  311,  312,  313,  314,  315,  316,  317,  318,  319,
  320,  258,  259,  260,  261,  126,   -1,  264,  265,  266,
  267,  268,  269,  270,  271,  272,  273,  274,  275,   -1,
   -1,  278,  279,  280,  281,  282,  283,  284,   -1,  286,
  287,  288,  289,  290,  291,  292,  293,  294,  295,  296,
  297,  298,  299,  300,  301,  302,  303,  304,  305,  306,
  307,  308,  309,  310,  311,  312,  313,  314,  315,  316,
  317,  318,  319,  320,  258,  259,  260,  261,  126,   -1,
  264,  265,  266,  267,  268,  269,  270,  271,  272,  273,
  274,  275,   -1,   -1,  278,  279,  280,  281,  282,  283,
  284,   -1,  286,  287,  288,  289,  290,  291,  292,  293,
  294,  295,  296,  297,  298,  299,  300,  301,  302,  303,
  304,  305,  306,  307,  308,  309,  310,  311,  312,  313,
  314,  315,  316,  317,  318,  319,  320,  126,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  261,   -1,   -1,  264,  265,  266,  267,  268,  269,  270,
  271,  272,  273,  274,  275,   -1,   -1,  278,  279,  280,
   -1,  282,  283,   -1,   -1,   -1,  287,  288,  289,  290,
  291,  292,  293,  294,  295,  296,  297,  298,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  264,  265,  266,  267,
  268,  269,  270,  271,  272,  273,  274,  275,   -1,   -1,
  278,  279,  280,   -1,  282,  283,   -1,   -1,   -1,  287,
  288,  289,  290,  291,  292,  293,  294,  295,  296,  297,
  298,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  264,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,   -1,   -1,  278,
   -1,  280,   -1,  282,  283,   -1,   -1,   -1,  287,  288,
  289,  290,  291,  292,  293,  294,  295,  296,  297,  298,
  264,  265,  266,  267,  268,  269,  270,  271,  272,  273,
  274,  275,   -1,   -1,  278,   -1,  280,   -1,  282,  283,
  284,   -1,   -1,  287,  288,  289,  290,  291,  292,  293,
  294,  295,  296,  297,  298,  264,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,   -1,   -1,  278,
   -1,  280,   -1,  282,   -1,   -1,   -1,   -1,  287,  288,
  289,  290,  291,  292,  293,  294,  295,  296,  297,  298,
  264,  265,  266,  267,  268,  269,  270,  271,  272,  273,
  274,  275,   -1,   -1,   -1,   -1,  280,   -1,  282,   -1,
   -1,   -1,   -1,  287,  288,  289,  290,  291,  292,  293,
  294,  295,  296,  297,  298,  264,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,   -1,   -1,   -1,
   -1,  280,   -1,  282,   -1,   -1,   -1,   -1,  287,  288,
  289,  290,  291,  292,  293,  294,  295,  296,  297,  264,
  265,  266,  267,  268,  269,  270,  271,  272,  273,  274,
  275,   -1,   -1,   -1,   -1,   -1,   -1,  282,   -1,   -1,
   -1,   -1,  287,  288,  289,  290,  291,  292,  293,  294,
  295,  296,  297,
};
#define YYFINAL 47
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 320
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,"'&'",0,"'('","')'","'*'",0,"','","'-'","'.'",0,0,0,0,0,0,0,0,0,0,0,
"':'","';'",0,"'='",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"'['",0,"']'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,
"'}'","'~'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"CLASS","PUBLIC","PRIVATE","PROTECTED",
"VIRTUAL","STRING","NUM","ID","INT","FLOAT","SHORT","LONG","LONG_LONG",
"INT64__","DOUBLE","VOID","CHAR","SIGNED_CHAR","BOOL","CLASS_REF","OTHER",
"CONST","OPERATOR","UNSIGNED","FRIEND","VTK_ID","STATIC","VAR_FUNCTION",
"ARRAY_NUM","VTK_LEGACY","TypeInt8","TypeUInt8","TypeInt16","TypeUInt16",
"TypeInt32","TypeUInt32","TypeInt64","TypeUInt64","TypeFloat32","TypeFloat64",
"IdType","StdString","SetMacro","GetMacro","SetStringMacro","GetStringMacro",
"SetClampMacro","SetObjectMacro","SetReferenceCountedObjectMacro",
"GetObjectMacro","BooleanMacro","SetVector2Macro","SetVector3Macro",
"SetVector4Macro","SetVector6Macro","GetVector2Macro","GetVector3Macro",
"GetVector4Macro","GetVector6Macro","SetVectorMacro","GetVectorMacro",
"ViewportCoordinateMacro","WorldCoordinateMacro","TypeMacro",
};
char *yyrule[] = {
"$accept : strt",
"strt : maybe_other class_def maybe_other",
"$$1 :",
"class_def : CLASS VTK_ID $$1 optional_scope '{' class_def_body '}'",
"class_def_body : class_def_item",
"class_def_body : class_def_item class_def_body",
"class_def_item : scope_type ':'",
"class_def_item : var",
"class_def_item : operator",
"class_def_item : FRIEND operator",
"class_def_item : function func_body",
"class_def_item : FRIEND function func_body",
"class_def_item : legacy_function func_body",
"class_def_item : macro ';'",
"class_def_item : macro",
"legacy_function : VTK_LEGACY '(' function ')'",
"function : '~' func",
"function : VIRTUAL '~' func",
"function : func",
"function : type func",
"function : type CONST func",
"function : VIRTUAL type CONST func",
"function : VIRTUAL type func",
"function : VIRTUAL func",
"operator : operator_sig",
"operator : type operator_sig",
"operator : type CONST operator_sig",
"operator : VIRTUAL type CONST operator_sig",
"operator : VIRTUAL type operator_sig",
"operator : VIRTUAL operator_sig",
"operator_sig : OPERATOR maybe_other_no_semi ';'",
"$$2 :",
"$$3 :",
"func : func_sig $$2 maybe_const $$3",
"func : func_sig '=' NUM",
"maybe_const :",
"maybe_const : CONST",
"$$4 :",
"func_sig : any_id '(' $$4 args_list ')'",
"const_mod : CONST",
"static_mod : STATIC",
"any_id : VTK_ID",
"any_id : ID",
"func_body : ';'",
"func_body : '{' maybe_other '}' ';'",
"func_body : '{' maybe_other '}'",
"func_body : ':' maybe_other_no_semi ';'",
"args_list :",
"args_list : more_args",
"more_args : arg",
"$$5 :",
"more_args : arg $$5 ',' more_args",
"arg : type",
"$$6 :",
"arg : type var_id $$6 opt_var_assign",
"arg : VAR_FUNCTION",
"opt_var_assign :",
"opt_var_assign : '=' float_num",
"var : type var_id ';'",
"var : VAR_FUNCTION ';'",
"var_id : any_id var_array",
"var_array :",
"$$7 :",
"var_array : ARRAY_NUM $$7 var_array",
"var_array : '[' maybe_other_no_semi ']' var_array",
"type : const_mod type_red1",
"type : type_red1",
"type : static_mod type_red1",
"type : static_mod const_mod type_red1",
"type_red1 : type_red2",
"type_red1 : type_red2 type_indirection",
"type_red1 : type_string1",
"type_string1 : type_string2",
"type_string1 : type_string2 '&'",
"type_string1 : type_string2 '*'",
"type_string2 : StdString",
"type_indirection : '&'",
"type_indirection : '*'",
"type_indirection : '&' type_indirection",
"type_indirection : '*' type_indirection",
"$$8 :",
"type_red2 : UNSIGNED $$8 type_primitive",
"type_red2 : type_primitive",
"type_primitive : TypeInt8",
"type_primitive : TypeUInt8",
"type_primitive : TypeInt16",
"type_primitive : TypeUInt16",
"type_primitive : TypeInt32",
"type_primitive : TypeUInt32",
"type_primitive : TypeInt64",
"type_primitive : TypeUInt64",
"type_primitive : TypeFloat32",
"type_primitive : TypeFloat64",
"type_primitive : FLOAT",
"type_primitive : VOID",
"type_primitive : CHAR",
"type_primitive : INT",
"type_primitive : SHORT",
"type_primitive : LONG",
"type_primitive : DOUBLE",
"type_primitive : ID",
"type_primitive : VTK_ID",
"type_primitive : IdType",
"type_primitive : LONG_LONG",
"type_primitive : INT64__",
"type_primitive : SIGNED_CHAR",
"type_primitive : BOOL",
"optional_scope :",
"optional_scope : ':' scope_list",
"scope_list : scope_type VTK_ID",
"$$9 :",
"scope_list : scope_type VTK_ID $$9 ',' scope_list",
"scope_type : PUBLIC",
"scope_type : PRIVATE",
"scope_type : PROTECTED",
"float_num : '-' float_prim",
"float_num : float_prim",
"float_prim : NUM",
"float_prim : NUM '.' NUM",
"float_prim : any_id",
"$$10 :",
"macro : SetMacro '(' any_id ',' $$10 type_red2 ')'",
"$$11 :",
"$$12 :",
"macro : GetMacro '(' $$11 any_id ',' $$12 type_red2 ')'",
"$$13 :",
"macro : SetStringMacro '(' $$13 any_id ')'",
"$$14 :",
"macro : GetStringMacro '(' $$14 any_id ')'",
"$$15 :",
"$$16 :",
"macro : SetClampMacro '(' any_id ',' $$15 type_red2 $$16 ',' maybe_other_no_semi ')'",
"$$17 :",
"macro : SetObjectMacro '(' any_id ',' $$17 type_red2 ')'",
"$$18 :",
"macro : SetReferenceCountedObjectMacro '(' any_id ',' $$18 type_red2 ')'",
"$$19 :",
"$$20 :",
"macro : GetObjectMacro '(' $$19 any_id ',' $$20 type_red2 ')'",
"$$21 :",
"macro : BooleanMacro '(' any_id $$21 ',' type_red2 ')'",
"$$22 :",
"macro : SetVector2Macro '(' any_id ',' $$22 type_red2 ')'",
"$$23 :",
"macro : GetVector2Macro '(' any_id ',' $$23 type_red2 ')'",
"$$24 :",
"macro : SetVector3Macro '(' any_id ',' $$24 type_red2 ')'",
"$$25 :",
"macro : GetVector3Macro '(' any_id ',' $$25 type_red2 ')'",
"$$26 :",
"macro : SetVector4Macro '(' any_id ',' $$26 type_red2 ')'",
"$$27 :",
"macro : GetVector4Macro '(' any_id ',' $$27 type_red2 ')'",
"$$28 :",
"macro : SetVector6Macro '(' any_id ',' $$28 type_red2 ')'",
"$$29 :",
"macro : GetVector6Macro '(' any_id ',' $$29 type_red2 ')'",
"$$30 :",
"macro : SetVectorMacro '(' any_id ',' $$30 type_red2 ',' float_num ')'",
"$$31 :",
"macro : GetVectorMacro '(' any_id ',' $$31 type_red2 ',' float_num ')'",
"macro : ViewportCoordinateMacro '(' any_id ')'",
"macro : WorldCoordinateMacro '(' any_id ')'",
"macro : TypeMacro '(' any_id ',' any_id ')'",
"macro : TypeMacro '(' any_id ',' any_id ',' ')'",
"maybe_other :",
"maybe_other : other_stuff maybe_other",
"maybe_other_no_semi :",
"maybe_other_no_semi : other_stuff_no_semi maybe_other_no_semi",
"other_stuff : ';'",
"other_stuff : other_stuff_no_semi",
"other_stuff_no_semi : OTHER",
"other_stuff_no_semi : braces",
"other_stuff_no_semi : parens",
"other_stuff_no_semi : '*'",
"other_stuff_no_semi : '='",
"other_stuff_no_semi : ':'",
"other_stuff_no_semi : ','",
"other_stuff_no_semi : '.'",
"other_stuff_no_semi : STRING",
"other_stuff_no_semi : type_red2",
"other_stuff_no_semi : type_string2",
"other_stuff_no_semi : NUM",
"other_stuff_no_semi : CLASS_REF",
"other_stuff_no_semi : '&'",
"other_stuff_no_semi : brackets",
"other_stuff_no_semi : CONST",
"other_stuff_no_semi : OPERATOR",
"other_stuff_no_semi : '-'",
"other_stuff_no_semi : '~'",
"other_stuff_no_semi : STATIC",
"other_stuff_no_semi : ARRAY_NUM",
"braces : '{' maybe_other '}'",
"parens : '(' maybe_other ')'",
"brackets : '[' maybe_other ']'",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
int yystacksize;
#line 1114 "vtkParse.y"
#include <string.h>
#include "lex.yy.c"

static void vtkParseDebug(const char* s1, const char* s2)
{
  if ( getenv("DEBUG") )
    {
    fprintf(stderr, "   %s", s1);
    if ( s2 )
      {
      fprintf(stderr, " %s", s2);
      }
    fprintf(stderr, "\n");
    }
}

/* initialize the structure */
void InitFunction(FunctionInfo *func)
{
  func->Name = NULL;
  func->NumberOfArguments = 0;
  func->ArrayFailure = 0;
  func->IsPureVirtual = 0;
  func->IsPublic = 0;
  func->IsOperator = 0;
  func->HaveHint = 0;
  func->HintSize = 0;
  func->ReturnType = 0x2;
  func->ReturnClass = NULL;
  func->Comment = NULL;
  func->Signature = NULL;
  func->IsLegacy = 0;
  sigAllocatedLength = 0;
  openSig = 1;
  invertSig = 0;
}

/* when the cpp file doesn't have enough info use the hint file */
void look_for_hint(void)
{
  char h_cls[80];
  char h_func[80];
  unsigned int  h_type;
  int  h_value;

  /* reset the position */
  if (!fhint) 
    {
    return;
    }
  rewind(fhint);

  /* first find a hint */
  while (fscanf(fhint,"%s %s %x %i",h_cls,h_func,&h_type,&h_value) != EOF)
    {
    if ((!strcmp(h_cls,data.ClassName))&&
        currentFunction->Name &&
        (!strcmp(h_func,currentFunction->Name))&&
        ((int)h_type == currentFunction->ReturnType))
      {
      currentFunction->HaveHint = 1;
      currentFunction->HintSize = h_value;
      }
    }
}

/* a simple routine that updates a few variables */
void output_function()
{
  int i;

  /* a void argument is the same as no arguements */
  if (currentFunction->ArgTypes[0] % 0x1000 == 0x2) 
    {
    currentFunction->NumberOfArguments = 0;
    }

  currentFunction->IsPublic = in_public;
  currentFunction->IsProtected = in_protected;
  
  /* look for VAR FUNCTIONS */
  if (currentFunction->NumberOfArguments
      && (currentFunction->ArgTypes[0] == 0x5000))
    {
    if (currentFunction->NumberOfArguments == 2)
      {
      currentFunction->NumberOfArguments = 1;
      }
    else
      {
      currentFunction->ArrayFailure = 1;
      }
    }
  
  /* is it a delete function */
  if (currentFunction->Name && !strcmp("Delete",currentFunction->Name))
    {
    data.HasDelete = 1;
    }


  /* if we need a return type hint and dont currently have one */
  /* then try to find one */
  if (!currentFunction->HaveHint)
    {
    switch (currentFunction->ReturnType % 0x1000)
      {
      case 0x301: case 0x302: case 0x307: case 0x30A: case 0x30B: case 0x30C:
      case 0x304: case 0x305: case 0x306: case 0x313:
        look_for_hint();
        break;
      }
    }

  /* reject multi-dimensional arrays from wrappers */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if ((currentFunction->ArgTypes[i] % 0x1000)/0x100 == 0x6 ||
        (currentFunction->ArgTypes[i] % 0x1000)/0x100 == 0x9)
      {
      currentFunction->ArrayFailure = 1;
      }
    }

  if (HaveComment)
    {
    currentFunction->Comment = vtkstrdup(CommentText);
    }
  
  data.NumberOfFunctions++;
  currentFunction = data.Functions + data.NumberOfFunctions;
  InitFunction(currentFunction);
}

extern void vtkParseOutput(FILE *,FileInfo *);

int main(int argc,char *argv[])
{
  FILE *fin;
  int ret;
  FILE *fout;
  
  if (argc < 4 || argc > 5)
    {
    fprintf(stderr,
            "Usage: %s input_file <hint_file> is_concrete output_file\n",argv[0]);
    exit(1);
    }
  
  if (!(fin = fopen(argv[1],"r")))
    {
    fprintf(stderr,"Error opening input file %s\n",argv[1]);
    exit(1);
    }

  fhint = 0;
  data.FileName = argv[1];
  data.NameComment = NULL;
  data.Description = NULL;
  data.Caveats = NULL;
  data.SeeAlso = NULL;
  CommentState = 0;

  if (argc == 5)
    {
    if (!(fhint = fopen(argv[2],"r")))
      {
      fprintf(stderr,"Error opening hint file %s\n",argv[2]);
      exit(1);
      }
    data.IsConcrete = atoi(argv[3]);
    }
  else
    {
    data.IsConcrete = atoi(argv[2]);
    }
  
  currentFunction = data.Functions;
  InitFunction(currentFunction);
  
  yyin = fin;
  yyout = stdout;
  ret = yyparse();
  if (ret)
    {
    fprintf(stdout,
            "*** SYNTAX ERROR found in parsing the header file %s before line %d ***\n", 
            argv[1], yylineno);
    return ret;
    }

  if (argc == 5)
    {
    fout = fopen(argv[4],"w");
    data.OutputFileName = argv[4];
    }
  else
    {
    fout = fopen(argv[3],"w");
    data.OutputFileName = argv[3];
    }
  
  if (!fout)
    {
    fprintf(stderr,"Error opening output file %s\n",argv[3]);
    exit(1);
    }
  vtkParseOutput(fout,&data);
  fclose (fout);

  return 0;
}


#line 1148 "vtkParse.tab.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
int yyparse __P((void));
static int yygrowstack __P((void));
static int yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = yyssp - yyss;
    if ((newss = (short *)realloc(yyss, newsize * sizeof *newss)) == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    if ((newvs = (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs)) == NULL)
        return -1;
    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    int yym, yyn, yystate;
#if YYDEBUG
    char *yys;

    if ((yys = getenv("YYDEBUG")) != NULL)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
    goto yynewerror;
yynewerror:
    yyerror("syntax error");
    goto yyerrlab;
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 2:
#line 279 "vtkParse.y"
{
      data.ClassName = vtkstrdup(yyvsp[0].str);
      }
break;
case 10:
#line 289 "vtkParse.y"
{ output_function(); }
break;
case 11:
#line 290 "vtkParse.y"
{ output_function(); }
break;
case 12:
#line 291 "vtkParse.y"
{ legacySig(); output_function(); }
break;
case 16:
#line 297 "vtkParse.y"
{ preSig("~"); }
break;
case 17:
#line 298 "vtkParse.y"
{ preSig("virtual ~"); }
break;
case 19:
#line 301 "vtkParse.y"
{
         currentFunction->ReturnType = yyvsp[-1].integer;
         }
break;
case 20:
#line 305 "vtkParse.y"
{
         currentFunction->ReturnType = yyvsp[-2].integer;
         }
break;
case 21:
#line 309 "vtkParse.y"
{
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
         }
break;
case 22:
#line 314 "vtkParse.y"
{
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         }
break;
case 23:
#line 319 "vtkParse.y"
{
         preSig("virtual ");
         }
break;
case 24:
#line 325 "vtkParse.y"
{
         output_function();
         }
break;
case 25:
#line 329 "vtkParse.y"
{
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
break;
case 26:
#line 334 "vtkParse.y"
{
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
         }
break;
case 27:
#line 339 "vtkParse.y"
{
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-2].integer;
         output_function();
         }
break;
case 28:
#line 345 "vtkParse.y"
{
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
break;
case 29:
#line 351 "vtkParse.y"
{
         preSig("virtual ");
         output_function();
         }
break;
case 30:
#line 357 "vtkParse.y"
{
      currentFunction->IsOperator = 1;
      vtkParseDebug("Converted operator", 0);
    }
break;
case 31:
#line 362 "vtkParse.y"
{ postSig(")"); }
break;
case 32:
#line 362 "vtkParse.y"
{ postSig(";"); openSig = 0; }
break;
case 33:
#line 363 "vtkParse.y"
{
      openSig = 1;
      currentFunction->Name = yyvsp[-3].str; 
      vtkParseDebug("Parsed func", yyvsp[-3].str);
    }
break;
case 34:
#line 369 "vtkParse.y"
{ 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-2].str; 
      vtkParseDebug("Parsed func", yyvsp[-2].str);
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    }
break;
case 36:
#line 377 "vtkParse.y"
{postSig(" const");}
break;
case 37:
#line 379 "vtkParse.y"
{postSig(" ("); }
break;
case 39:
#line 381 "vtkParse.y"
{postSig("const ");}
break;
case 40:
#line 383 "vtkParse.y"
{postSig("static ");}
break;
case 41:
#line 385 "vtkParse.y"
{postSig(yyvsp[0].str);}
break;
case 42:
#line 385 "vtkParse.y"
{postSig(yyvsp[0].str);}
break;
case 49:
#line 394 "vtkParse.y"
{ currentFunction->NumberOfArguments++;}
break;
case 50:
#line 395 "vtkParse.y"
{ currentFunction->NumberOfArguments++; postSig(", ");}
break;
case 52:
#line 398 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer;}
break;
case 53:
#line 403 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer / 0x10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[-1].integer + yyvsp[0].integer % 0x10000;
    }
break;
case 55:
#line 410 "vtkParse.y"
{ 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 0x5000;
    }
break;
case 58:
#line 418 "vtkParse.y"
{delSig();}
break;
case 59:
#line 418 "vtkParse.y"
{delSig();}
break;
case 60:
#line 420 "vtkParse.y"
{ yyval.integer = yyvsp[0].integer; }
break;
case 61:
#line 428 "vtkParse.y"
{ yyval.integer = 0; }
break;
case 62:
#line 429 "vtkParse.y"
{ char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); 
                   postSig(temp); }
break;
case 63:
#line 431 "vtkParse.y"
{ yyval.integer = 0x300 + 0x10000 * yyvsp[-2].integer + yyvsp[0].integer % 0x1000; }
break;
case 64:
#line 433 "vtkParse.y"
{ postSig("[]"); yyval.integer = 0x300 + yyvsp[0].integer % 0x1000; }
break;
case 65:
#line 435 "vtkParse.y"
{yyval.integer = 0x1000 + yyvsp[0].integer;}
break;
case 66:
#line 436 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 67:
#line 437 "vtkParse.y"
{yyval.integer = 0x2000 + yyvsp[0].integer;}
break;
case 68:
#line 438 "vtkParse.y"
{yyval.integer = 0x3000 + yyvsp[0].integer;}
break;
case 69:
#line 440 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 70:
#line 442 "vtkParse.y"
{yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;}
break;
case 71:
#line 443 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 72:
#line 445 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 73:
#line 446 "vtkParse.y"
{ postSig("&"); yyval.integer = yyvsp[-1].integer;}
break;
case 74:
#line 447 "vtkParse.y"
{ postSig("*"); yyval.integer = 0x400 + yyvsp[-1].integer;}
break;
case 75:
#line 449 "vtkParse.y"
{ postSig("vtkStdString "); yyval.integer = 0x1303; }
break;
case 76:
#line 459 "vtkParse.y"
{ postSig("&"); yyval.integer = 0x100;}
break;
case 77:
#line 460 "vtkParse.y"
{ postSig("*"); yyval.integer = 0x300;}
break;
case 78:
#line 461 "vtkParse.y"
{ yyval.integer = 0x100 + yyvsp[0].integer;}
break;
case 79:
#line 462 "vtkParse.y"
{ yyval.integer = 0x400 + yyvsp[0].integer;}
break;
case 80:
#line 464 "vtkParse.y"
{postSig("unsigned ");}
break;
case 81:
#line 465 "vtkParse.y"
{ yyval.integer = 0x10 + yyvsp[0].integer;}
break;
case 82:
#line 466 "vtkParse.y"
{ yyval.integer = yyvsp[0].integer;}
break;
case 83:
#line 469 "vtkParse.y"
{ postSig("vtkTypeInt8 "); yyval.integer = VTK_PARSE_INT8; }
break;
case 84:
#line 470 "vtkParse.y"
{ postSig("vtkTypeUInt8 "); yyval.integer = VTK_PARSE_UINT8; }
break;
case 85:
#line 471 "vtkParse.y"
{ postSig("vtkTypeInt16 "); yyval.integer = VTK_PARSE_INT16; }
break;
case 86:
#line 472 "vtkParse.y"
{ postSig("vtkTypeUInt16 "); yyval.integer = VTK_PARSE_UINT16; }
break;
case 87:
#line 473 "vtkParse.y"
{ postSig("vtkTypeInt32 "); yyval.integer = VTK_PARSE_INT32; }
break;
case 88:
#line 474 "vtkParse.y"
{ postSig("vtkTypeUInt32 "); yyval.integer = VTK_PARSE_UINT32; }
break;
case 89:
#line 475 "vtkParse.y"
{ postSig("vtkTypeInt64 "); yyval.integer = VTK_PARSE_INT64; }
break;
case 90:
#line 476 "vtkParse.y"
{ postSig("vtkTypeUInt64 "); yyval.integer = VTK_PARSE_UINT64; }
break;
case 91:
#line 477 "vtkParse.y"
{ postSig("vtkTypeFloat32 "); yyval.integer = VTK_PARSE_FLOAT32; }
break;
case 92:
#line 478 "vtkParse.y"
{ postSig("vtkTypeFloat64 "); yyval.integer = VTK_PARSE_FLOAT64; }
break;
case 93:
#line 479 "vtkParse.y"
{ postSig("float "); yyval.integer = 0x1;}
break;
case 94:
#line 480 "vtkParse.y"
{ postSig("void "); yyval.integer = 0x2;}
break;
case 95:
#line 481 "vtkParse.y"
{ postSig("char "); yyval.integer = 0x3;}
break;
case 96:
#line 482 "vtkParse.y"
{ postSig("int "); yyval.integer = 0x4;}
break;
case 97:
#line 483 "vtkParse.y"
{ postSig("short "); yyval.integer = 0x5;}
break;
case 98:
#line 484 "vtkParse.y"
{ postSig("long "); yyval.integer = 0x6;}
break;
case 99:
#line 485 "vtkParse.y"
{ postSig("double "); yyval.integer = 0x7;}
break;
case 100:
#line 486 "vtkParse.y"
{       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 0x8;}
break;
case 101:
#line 492 "vtkParse.y"
{ 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 0x9; 
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        vtkstrdup(yyvsp[0].str); 
      /* store the string into the return value just in case we need it */
      /* this is a parsing hack because the first "type" parser will */
      /* possibly be ht ereturn type of the first argument */
      if ((!currentFunction->ReturnClass) && 
          (!currentFunction->NumberOfArguments)) 
        { 
        currentFunction->ReturnClass = vtkstrdup(yyvsp[0].str); 
        }
    }
break;
case 102:
#line 508 "vtkParse.y"
{ postSig("vtkIdType "); yyval.integer = 0xA;}
break;
case 103:
#line 509 "vtkParse.y"
{ postSig("long long "); yyval.integer = 0xB;}
break;
case 104:
#line 510 "vtkParse.y"
{ postSig("__int64 "); yyval.integer = 0xC;}
break;
case 105:
#line 511 "vtkParse.y"
{ postSig("signed char "); yyval.integer = 0xD;}
break;
case 106:
#line 512 "vtkParse.y"
{ postSig("bool "); yyval.integer = 0xE;}
break;
case 109:
#line 517 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
break;
case 110:
#line 522 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
break;
case 112:
#line 527 "vtkParse.y"
{in_public = 1; in_protected = 0;}
break;
case 113:
#line 528 "vtkParse.y"
{in_public = 0; in_protected = 0;}
break;
case 114:
#line 529 "vtkParse.y"
{in_public = 0; in_protected = 1;}
break;
case 117:
#line 533 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 118:
#line 534 "vtkParse.y"
{yyval.integer = -1;}
break;
case 119:
#line 534 "vtkParse.y"
{yyval.integer = -1;}
break;
case 120:
#line 538 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 121:
#line 539 "vtkParse.y"
{
   postSig(");");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
break;
case 122:
#line 549 "vtkParse.y"
{postSig("Get");}
break;
case 123:
#line 549 "vtkParse.y"
{postSig(" ();"); invertSig = 1;}
break;
case 124:
#line 551 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   }
break;
case 125:
#line 558 "vtkParse.y"
{preSig("void Set");}
break;
case 126:
#line 559 "vtkParse.y"
{
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
break;
case 127:
#line 569 "vtkParse.y"
{preSig("char *Get");}
break;
case 128:
#line 570 "vtkParse.y"
{ 
   postSig(" ();");
   sprintf(temps,"Get%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x303;
   output_function();
   }
break;
case 129:
#line 579 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 130:
#line 580 "vtkParse.y"
{postSig(");"); openSig = 0;}
break;
case 131:
#line 581 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sscanf (currentFunction->Signature, "%*s %*s (%s);", local);
   sprintf(temps,"Set%s",yyvsp[-7].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-4].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMinValue ();",local,yyvsp[-7].str);
   sprintf(temps,"Get%sMinValue",yyvsp[-7].str);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-4].integer;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"%s Get%sMaxValue ();",local,yyvsp[-7].str);
   sprintf(temps,"Get%sMaxValue",yyvsp[-7].str);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-4].integer;
   output_function();
   }
break;
case 132:
#line 611 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 133:
#line 612 "vtkParse.y"
{ 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
break;
case 134:
#line 623 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 135:
#line 624 "vtkParse.y"
{ 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 0x2;
   output_function();
   }
break;
case 136:
#line 634 "vtkParse.y"
{postSig("*Get");}
break;
case 137:
#line 635 "vtkParse.y"
{postSig(" ();"); invertSig = 1;}
break;
case 138:
#line 636 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   output_function();
   }
break;
case 139:
#line 644 "vtkParse.y"
{preSig("void "); postSig("On ();"); openSig = 0; }
break;
case 140:
#line 646 "vtkParse.y"
{ 
   sprintf(temps,"%sOn",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x2;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff ();",yyvsp[-4].str); 
   sprintf(temps,"%sOff",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   }
break;
case 141:
#line 661 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 142:
#line 666 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s);",yyvsp[-4].str,
     local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 2;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yyvsp[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[2]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0x2;
   output_function();
   }
break;
case 143:
#line 691 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 144:
#line 696 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   }
break;
case 145:
#line 708 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 146:
#line 713 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s);",
     yyvsp[-4].str, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 3;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yyvsp[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = yyvsp[-1].integer;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[3]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 3;
   output_function();
   }
break;
case 147:
#line 740 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 148:
#line 745 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   }
break;
case 149:
#line 757 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 150:
#line 762 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s);",
     yyvsp[-4].str, local, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 4;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yyvsp[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = yyvsp[-1].integer;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = yyvsp[-1].integer;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[4]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 4;
   output_function();
   }
break;
case 151:
#line 791 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 152:
#line 796 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   }
break;
case 153:
#line 808 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 154:
#line 813 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s, %s, %s);",
     yyvsp[-4].str, local, local, local, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 6;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yyvsp[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = yyvsp[-1].integer;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = yyvsp[-1].integer;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ArgTypes[4] = yyvsp[-1].integer;
   currentFunction->ArgCounts[4] = 0;
   currentFunction->ArgTypes[5] = yyvsp[-1].integer;
   currentFunction->ArgCounts[5] = 0;
   currentFunction->ReturnType = 0x2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[6]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 6;
   output_function();
   }
break;
case 155:
#line 846 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 156:
#line 851 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   }
break;
case 157:
#line 863 "vtkParse.y"
{
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
break;
case 158:
#line 868 "vtkParse.y"
{
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",yyvsp[-6].str,
      local, yyvsp[-1].integer);
     sprintf(temps,"Set%s",yyvsp[-6].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->ReturnType = 0x2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x300 + yyvsp[-3].integer;
     currentFunction->ArgCounts[0] = yyvsp[-1].integer;
     output_function();
   }
break;
case 159:
#line 881 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 160:
#line 886 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-6].str);
   sprintf(temps,"Get%s",yyvsp[-6].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x300 + yyvsp[-3].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = yyvsp[-1].integer;
   output_function();
   }
break;
case 161:
#line 898 "vtkParse.y"
{ 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = 0x7;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 0x7;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ReturnType = 0x2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[2]);",
       yyvsp[-1].str);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x307;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
break;
case 162:
#line 945 "vtkParse.y"
{ 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double, double);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 3;
     currentFunction->ArgTypes[0] = 0x7;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 0x7;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ArgTypes[2] = 0x7;
     currentFunction->ArgCounts[2] = 0;
     currentFunction->ReturnType = 0x2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[3]);",
       yyvsp[-1].str);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x307;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 0x307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   }
break;
case 163:
#line 994 "vtkParse.y"
{ 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName ();");
   sprintf(temps,"GetClassName"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x1303;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA (const char *name);");
   sprintf(temps,"IsA"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x1303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x4;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance ();",
           yyvsp[-3].str);
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   currentFunction->ReturnClass = vtkstrdup(yyvsp[-3].str);
   output_function();

   if ( data.IsConcrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast (vtkObject* o);",
             yyvsp[-3].str);
     sprintf(temps,"SafeDownCast"); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x309;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = 0x2309;
     currentFunction->ReturnClass = vtkstrdup(yyvsp[-3].str);
     output_function();
     }
   }
break;
case 164:
#line 1045 "vtkParse.y"
{ 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName ();");
   sprintf(temps,"GetClassName"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x1303;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA (const char *name);");
   sprintf(temps,"IsA"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 0x1303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 0x4;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance ();",
           yyvsp[-4].str);
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 0x309;
   currentFunction->ReturnClass = vtkstrdup(yyvsp[-4].str);
   output_function();

   if ( data.IsConcrete )
     {
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature, "%s *SafeDownCast (vtkObject* o);",
             yyvsp[-4].str);
     sprintf(temps,"SafeDownCast"); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 0x309;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = 0x2309;
     currentFunction->ReturnClass = vtkstrdup(yyvsp[-4].str);
     output_function();
     }
   }
break;
#line 2421 "vtkParse.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
