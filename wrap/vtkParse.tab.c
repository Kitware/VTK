#if defined(__STDC__) || defined(__cplusplus)
#define YYCONST const
#define YYPARAMS(x) x
#define YYDEFUN(name, arglist, args) name(args)
#define YYAND ,
#define YYPTR void *
#else
#define YYCONST
#define YYPARAMS(x) ()
#define YYDEFUN(name, arglist, args) name arglist args;
#define YYAND ;
#define YYPTR char *
#endif
#ifndef lint
YYCONST static char yysccsid[] = "@(#)yaccpar	1.8 (Berkeley +Cygnus.28) 01/20/91";
#endif
#define YYBYACC 1
#ifndef YYDONT_INCLUDE_STDIO
#include <stdio.h>
#endif
#ifdef __cplusplus
#include <stdlib.h> /* for malloc/realloc/free */
#endif
#line 43 "vtkParse.y"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1

#include "vtkParse.h"
    
  FileInfo data;
  static FunctionInfo *currentFunction;

  FILE *fhint;
  char temps[2048];
  int  in_public;
  int  HaveComment;
  char CommentText[50000];
  int CommentState;
  int openSig;
  int invertSig;
  int sigAllocatedLength;
  
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
      currentFunction->Signature = malloc(2048);
      sigAllocatedLength = 2048; 
      sprintf(currentFunction->Signature,"%s",arg);
      }    
    else if (openSig)
      {
      char *tmp;
      checkSigSize(arg);
      tmp = strdup(currentFunction->Signature);
      sprintf(currentFunction->Signature,"%s%s",arg,tmp);
      free(tmp);
      }
    }
  void postSig(char *arg)
    {
    if (!currentFunction->Signature)
      {
      currentFunction->Signature = malloc(2048);
      sigAllocatedLength = 2048; 
      sprintf(currentFunction->Signature,"%s",arg);
      }    
    else if (openSig)
      {
      char *tmp;
      checkSigSize(arg);
      tmp = strdup(currentFunction->Signature);
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
#line 119 "vtkParse.y"
typedef union{
  char *str;
  int   integer;
  } YYSTYPE;
#line 105 "vtkParse.tab.c"
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
#define DOUBLE 269
#define VOID 270
#define CHAR 271
#define CLASS_REF 272
#define OTHER 273
#define CONST 274
#define OPERATOR 275
#define UNSIGNED 276
#define FRIEND 277
#define VTK_ID 278
#define STATIC 279
#define VAR_FUNCTION 280
#define ARRAY_NUM 281
#define SetMacro 282
#define GetMacro 283
#define SetStringMacro 284
#define GetStringMacro 285
#define SetClampMacro 286
#define SetObjectMacro 287
#define SetReferenceCountedObjectMacro 288
#define GetObjectMacro 289
#define BooleanMacro 290
#define SetVector2Macro 291
#define SetVector3Macro 292
#define SetVector4Macro 293
#define SetVector6Macro 294
#define GetVector2Macro 295
#define GetVector3Macro 296
#define GetVector4Macro 297
#define GetVector6Macro 298
#define SetVectorMacro 299
#define GetVectorMacro 300
#define ViewportCoordinateMacro 301
#define WorldCoordinateMacro 302
#define YYERRCODE 256
static YYCONST short yylhs[] = {                                        -1,
    0,    4,    2,    5,    5,    6,    6,    6,    6,    6,
    6,    9,    9,    9,    9,    9,    9,   15,   11,   11,
   11,   19,   13,   17,   17,   14,   14,   14,   14,   14,
   14,   14,   18,   18,   20,   22,   20,   21,   25,   21,
   21,   24,   24,    8,    8,   23,   27,   28,   27,   27,
   12,   12,   12,   12,   29,   29,   31,   31,   31,   31,
   33,   30,   30,   32,   32,   32,   32,   32,   32,   32,
   32,   32,    3,    3,   34,   35,   34,    7,    7,    7,
   26,   26,   36,   36,   36,   37,   10,   38,   39,   10,
   40,   10,   41,   10,   42,   43,   10,   44,   10,   45,
   10,   46,   47,   10,   48,   10,   49,   10,   50,   10,
   51,   10,   52,   10,   53,   10,   54,   10,   55,   10,
   56,   10,   57,   10,   58,   10,   10,   10,    1,    1,
   16,   16,   59,   59,   60,   60,   60,   60,   60,   60,
   60,   60,   60,   60,   60,   60,   60,   60,   60,   60,
   60,   60,   60,   60,   61,   62,   63,
};
static YYCONST short yylen[] = {                                         2,
    3,    0,    7,    1,    2,    2,    1,    1,    2,    2,
    1,    2,    3,    1,    2,    3,    2,    0,    3,    3,
    4,    0,    5,    1,    1,    1,    2,    5,    4,    4,
    3,    3,    0,    1,    1,    0,    4,    1,    0,    4,
    1,    0,    2,    3,    2,    2,    0,    0,    3,    4,
    2,    1,    2,    3,    1,    2,    1,    1,    2,    2,
    0,    3,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    0,    2,    2,    0,    5,    1,    1,    1,
    2,    1,    1,    3,    1,    0,    7,    0,    0,    8,
    0,    5,    0,    5,    0,    0,   10,    0,    7,    0,
    7,    0,    0,    8,    0,    7,    0,    7,    0,    7,
    0,    7,    0,    7,    0,    7,    0,    7,    0,    7,
    0,    7,    0,    9,    0,    9,    4,    4,    0,    2,
    0,    2,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    3,    3,    3,
};
static YYCONST short yydefred[] = {                                      0,
  143,  145,   71,   67,   64,   68,   69,   70,   65,   66,
  146,  135,  149,  150,   61,   72,  153,  154,    0,  140,
  133,  152,  139,    0,  141,    0,  147,  138,  151,  142,
    0,    0,  144,   63,    0,  134,  136,  137,  148,    0,
    0,    0,    0,    0,    0,  130,   62,  155,  156,  157,
    2,    1,    0,    0,    0,   78,   79,   80,    0,   74,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    7,    8,    0,   14,    0,
    0,    0,   52,    0,    0,    0,   17,    0,   51,    0,
    0,    9,    0,    0,   53,   45,    0,   88,   91,   93,
    0,    0,    0,  102,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   25,   24,   12,
    3,    5,    6,   10,   15,    0,    0,    0,    0,   22,
    0,    0,   56,    0,   13,   16,   20,  132,   54,    0,
    0,    0,    0,    0,    0,    0,    0,  105,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   48,    0,   46,   44,    0,    0,    0,    0,   26,   19,
    0,   59,   60,   77,   86,    0,    0,    0,   95,   98,
  100,    0,    0,  107,  111,  115,  119,  109,  113,  117,
  121,  123,  125,  127,  128,    0,    0,   21,    0,   27,
    0,    0,   41,    0,    0,   34,    0,    0,   89,   92,
   94,    0,    0,    0,  103,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   49,    0,    0,    0,
   32,    0,   39,   23,    0,    0,    0,   96,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   50,    0,   29,    0,    0,   87,    0,    0,
   99,  101,    0,  106,  108,  112,  116,  120,  110,  114,
  118,  122,    0,    0,   28,    0,   40,   37,   90,    0,
  104,    0,    0,   85,    0,   82,    0,   43,    0,    0,
   81,  124,  126,   97,   84,
};
static YYCONST short yydgoto[] = {                                      31,
   32,   45,   55,   53,   93,   94,   59,   96,   97,   98,
   99,  100,  101,  190,  149,  110,  102,  225,  191,  226,
  227,  255,  147,  297,  276,  305,  183,  216,  103,   33,
  153,   34,   40,   60,  105,  306,  228,  161,  257,  162,
  163,  232,  280,  233,  234,  167,  261,  203,  237,  241,
  238,  242,  239,  243,  240,  244,  245,  246,   35,   36,
   37,   38,   39,
};
static YYCONST short yysindex[] = {                                    -38,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -38,    0,
    0,    0,    0,  -38,    0,  -38,    0,    0,    0,    0,
    0, -245,    0,    0,  -38,    0,    0,    0,    0,  372,
  -85,    9,   -7, -222,  -38,    0,    0,    0,    0,    0,
    0,    0,   34, -146,  -18,    0,    0,    0, -170,    0,
  333,    0,  -80,    0,  249,   -1, -102,    0,  -56,   50,
   75,   81,   83,   87,   89,   90,   91,   92,   94,   95,
   96,   97,   98,   99,  100,  101,  103,  104,  105,  106,
  107, -202,    3,  333,  102,    0,    0,   93,    0, -202,
  109,  110,    0,   10,  112, -202,    0, -202,    0,  116,
   -1,    0, -202,  249,    0,    0, -229,    0,    0,    0,
 -229, -229, -229,    0, -229, -229, -229, -229, -229, -229,
 -229, -229, -229, -229, -229, -229, -229,    0,    0,    0,
    0,    0,    0,    0,    0,  -37,  119, -109,  -28,    0,
   10,   10,    0, -146,    0,    0,    0,    0,    0,  114,
 -229, -229, -229,  117,  127,  135, -229,    0,  136,  137,
  139,  148,  149,  153,  157,  159,  160,  162,  166,  175,
    0,   -1,    0,    0,  158,  -48,  -38,   -1,    0,    0,
  115,    0,    0,    0,    0,  177,  182,  201,    0,    0,
    0,  203,  205,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  -81,  161,    0,  -38,    0,
  126,  196,    0, -229,  216,    0,    0,  249,    0,    0,
    0,  249,  249,  249,    0,  249,  249,  249,  249,  249,
  249,  249,  249,  249,  249,  249,    0,  -81,  151,  224,
    0,  -81,    0,    0,  242,  247,  249,    0,  251,  252,
  249,  253,  254,  256,  261,  263,  265,  266,  267,  268,
  269,  270,    0,  258,    0,  250,  115,    0,  271,  274,
    0,    0,  278,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   27,   27,    0,   27,    0,    0,    0,   -1,
    0,  264, -162,    0,  282,    0,  296,    0,  297,   52,
    0,    0,    0,    0,    0,
};
static YYCONST short yyrindex[] = {                                     82,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  215,    0,
    0,    0,    0,  301,    0,  255,    0,    0,    0,    0,
    0,    0,    0,    0,    1,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  343,    0,    0,    0,    0,    0,
    0,    0,  221,    0,    0,    0,    0,    0,    0,    0,
    0,  -17,    0,  -25,    0,  287,    0,   21,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  222,    0,    0,    0,  156,    0,    0,
  -26,    0,    0,  -19,    0,    0,    0,    0,    0,    0,
  -23,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  291,    0,    0,    0,    0,
   23,   25,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  259,    0,    0,    0,    0,  215,  287,    0,    0,
  310,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   37,    0,    0,  215,    0,
    0,    0,    0,   14,    0,    0,   24,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   37,    0,  210,
    0,   38,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  281,    0,   36,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  312,
    0,   63,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,
};
static YYCONST short yygindex[] = {                                      0,
   65,    0,    0,    0,  260,    0,  -47,    0,  288,    0,
   11,  -58,    0,    0,    0,  -95,  241,    0,    0,   79,
    0,    0,  133,    0,    0, -176, -197,    0,  -31,   88,
 -123,  319,    0,  206,    0,   58,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  -40,
    0,    0,    0,
};
#define YYTABLESIZE 650
static YYCONST short yytable[] = {                                      27,
  129,   24,  150,   28,  108,   25,   29,   30,  113,  182,
  220,   44,   71,   95,   25,  158,   71,  131,  247,   20,
   21,   55,   23,   92,   55,  111,   76,  192,  193,  188,
  189,   18,   18,  109,  138,  131,   27,  115,   24,   48,
   28,  129,   25,   29,   30,  106,   95,  151,  139,   49,
  273,  152,   26,  182,   38,   51,   20,   38,   72,   23,
   24,  138,   72,   57,   35,   58,   57,   36,   58,  131,
  111,  303,   66,  107,  219,  139,   42,   47,   47,   42,
   47,   47,  159,   41,   19,   50,  217,   22,   42,   26,
   43,   54,  222,  129,  187,   47,   18,   47,   47,   46,
  302,  138,  140,   83,   61,   75,   83,   62,  116,   52,
  145,   56,   57,   58,  117,  139,  155,  307,  156,  308,
  118,   19,  119,  145,   22,  129,  120,  141,  121,  122,
  123,  124,  224,  125,  126,  127,  128,  129,  130,  131,
  132,  111,  133,  134,  135,  136,  137,  111,  104,  150,
  104,  144,  104,  185,  104,  154,  104,  195,   63,  143,
  199,   64,    4,    5,    6,    7,    8,    9,   10,  148,
  200,   65,   66,   15,  157,   68,   69,  184,  201,  204,
  205,  104,  206,   64,    4,    5,    6,    7,    8,    9,
   10,  207,  208,   65,   66,   15,  209,   68,   69,  181,
  210,  104,  211,  212,  309,  213,  214,    3,    4,    5,
    6,    7,    8,    9,   10,  215,  218,  114,  224,   15,
  229,   16,  230,    1,    2,    3,    4,    5,    6,    7,
    8,    9,   10,   11,   12,   13,   14,   15,   71,   16,
   17,  231,   18,  181,   55,  186,  235,   18,  236,   71,
  250,  221,   71,  248,  251,   55,  254,  129,   55,  111,
    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,
   11,   12,   13,   14,   15,  274,   16,   17,  104,   18,
   11,   11,  275,  249,   72,  277,   57,  278,   58,  302,
  138,  281,  282,  284,  285,   72,  286,   57,   72,   58,
   57,  287,   58,  288,  139,  289,  290,  291,  292,  310,
  296,  299,  293,  294,  315,  256,  295,  300,  301,  258,
  259,  260,  312,  262,  263,  264,  265,  266,  267,  268,
  269,  270,  271,  272,   31,   31,  313,  314,  129,  129,
  146,  129,  129,   73,  279,  131,    4,  129,  283,   47,
   33,  131,  131,  142,  112,  298,  253,  160,   47,  194,
  311,  164,  165,  166,  104,  168,  169,  170,  171,  172,
  173,  174,  175,  176,  177,  178,  179,  180,    3,    4,
    5,    6,    7,    8,    9,   10,    0,    0,   65,    0,
   15,    0,   16,   69,  223,    0,    0,    0,    0,    0,
    0,  196,  197,  198,    0,   30,   30,  202,    0,    0,
    0,    0,    0,   11,   11,   11,   11,    0,    0,   11,
   11,   11,   11,   11,   11,   11,   11,    0,    0,   11,
   11,   11,   11,   11,   11,   11,    0,   11,   11,   11,
   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   11,   11,   11,   11,   11,   92,    0,
    0,    0,    0,    0,  252,    0,    0,   31,   31,   31,
   31,    0,    0,   31,   31,   31,   31,   31,   31,   31,
   31,    0,    0,   31,   31,   31,   31,   31,   31,   31,
    0,   31,   31,   31,   31,   31,   31,   31,   31,   31,
   31,   31,   31,   31,   31,   31,   31,   31,   31,   31,
   31,   31,    3,    4,    5,    6,    7,    8,    9,   10,
    0,    0,    0,    0,   15,    0,   16,    0,    0,    0,
    0,    0,    0,  304,  304,    0,  304,    0,   30,   30,
   30,   30,    0,  304,   30,   30,   30,   30,   30,   30,
   30,   30,    0,    0,   30,   30,   30,   30,   30,   30,
   30,    0,   30,   30,   30,   30,   30,   30,   30,   30,
   30,   30,   30,   30,   30,   30,   30,   30,   30,   30,
   30,   30,   30,    0,    0,    0,    0,    0,    0,    0,
   56,   57,   58,   63,    0,    0,   64,    4,    5,    6,
    7,    8,    9,   10,    0,    0,   65,   66,   15,   67,
   68,   69,   70,    0,   71,   72,   73,   74,   75,   76,
   77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
   87,   88,   89,   90,   91,    3,    4,    5,    6,    7,
    8,    9,   10,    0,    0,    0,    0,    0,    0,   16,
};
static YYCONST short yycheck[] = {                                      38,
    0,   40,   40,   42,   63,   44,   45,   46,   67,   91,
   59,  257,   38,   61,   40,  111,   42,   41,  216,   58,
   59,   41,   61,  126,   44,   66,   44,  151,  152,   58,
   59,   58,   59,   65,  264,   59,   38,   69,   40,  125,
   42,   41,   44,   45,   46,  126,   94,   38,  278,   41,
  248,   42,   91,   91,   41,  278,   58,   44,   38,   61,
   40,  264,   42,   41,   41,   41,   44,   44,   44,   93,
  111,   45,  275,   63,  123,  278,   41,   41,   41,   44,
   44,   44,  114,   19,  123,   93,  182,  126,   24,   91,
   26,   58,  188,   93,  123,   59,  123,   61,   61,   35,
  263,  264,   92,   41,  123,  123,   44,  278,   59,   45,
  100,  258,  259,  260,   40,  278,  106,  294,  108,  296,
   40,  123,   40,  113,  126,  125,   40,  125,   40,   40,
   40,   40,  191,   40,   40,   40,   40,   40,   40,   40,
   40,  182,   40,   40,   40,   40,   40,  188,   61,   40,
   63,   59,   65,  263,   67,   44,   69,   44,  261,   58,
   44,  264,  265,  266,  267,  268,  269,  270,  271,   61,
   44,  274,  275,  276,   59,  278,  279,   59,   44,   44,
   44,   94,   44,  264,  265,  266,  267,  268,  269,  270,
  271,   44,   44,  274,  275,  276,   44,  278,  279,  281,
   44,  114,   44,   44,  300,   44,   41,  264,  265,  266,
  267,  268,  269,  270,  271,   41,   59,  274,  277,  276,
   44,  278,   41,  262,  263,  264,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,  276,  264,  278,
  279,   41,  281,  281,  264,  274,   44,  274,   44,  275,
  125,  187,  278,   93,   59,  275,   41,  257,  278,  300,
  262,  263,  264,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,  276,  125,  278,  279,  191,  281,
  125,  126,   59,  219,  264,   44,  264,   41,  264,  263,
  264,   41,   41,   41,   41,  275,   41,  275,  278,  275,
  278,   41,  278,   41,  278,   41,   41,   41,   41,   46,
   61,   41,   44,   44,  263,  228,   59,   44,   41,  232,
  233,  234,   41,  236,  237,  238,  239,  240,  241,  242,
  243,  244,  245,  246,  125,  126,   41,   41,  257,  125,
  100,   41,    0,  123,  257,   59,  125,   93,  261,   59,
   41,   93,   41,   94,   67,  277,  224,  117,   40,  154,
  303,  121,  122,  123,  277,  125,  126,  127,  128,  129,
  130,  131,  132,  133,  134,  135,  136,  137,  264,  265,
  266,  267,  268,  269,  270,  271,   -1,   -1,  274,   -1,
  276,   -1,  278,  279,  280,   -1,   -1,   -1,   -1,   -1,
   -1,  161,  162,  163,   -1,  125,  126,  167,   -1,   -1,
   -1,   -1,   -1,  258,  259,  260,  261,   -1,   -1,  264,
  265,  266,  267,  268,  269,  270,  271,   -1,   -1,  274,
  275,  276,  277,  278,  279,  280,   -1,  282,  283,  284,
  285,  286,  287,  288,  289,  290,  291,  292,  293,  294,
  295,  296,  297,  298,  299,  300,  301,  302,  126,   -1,
   -1,   -1,   -1,   -1,  224,   -1,   -1,  258,  259,  260,
  261,   -1,   -1,  264,  265,  266,  267,  268,  269,  270,
  271,   -1,   -1,  274,  275,  276,  277,  278,  279,  280,
   -1,  282,  283,  284,  285,  286,  287,  288,  289,  290,
  291,  292,  293,  294,  295,  296,  297,  298,  299,  300,
  301,  302,  264,  265,  266,  267,  268,  269,  270,  271,
   -1,   -1,   -1,   -1,  276,   -1,  278,   -1,   -1,   -1,
   -1,   -1,   -1,  293,  294,   -1,  296,   -1,  258,  259,
  260,  261,   -1,  303,  264,  265,  266,  267,  268,  269,
  270,  271,   -1,   -1,  274,  275,  276,  277,  278,  279,
  280,   -1,  282,  283,  284,  285,  286,  287,  288,  289,
  290,  291,  292,  293,  294,  295,  296,  297,  298,  299,
  300,  301,  302,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  258,  259,  260,  261,   -1,   -1,  264,  265,  266,  267,
  268,  269,  270,  271,   -1,   -1,  274,  275,  276,  277,
  278,  279,  280,   -1,  282,  283,  284,  285,  286,  287,
  288,  289,  290,  291,  292,  293,  294,  295,  296,  297,
  298,  299,  300,  301,  302,  264,  265,  266,  267,  268,
  269,  270,  271,   -1,   -1,   -1,   -1,   -1,   -1,  278,
};
#define YYFINAL 31
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 302
#if YYDEBUG
static YYCONST char *YYCONST yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,"'&'",0,"'('","')'","'*'",0,"','","'-'","'.'",0,0,0,0,0,0,0,0,0,0,0,
"':'","';'",0,"'='",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"'['",0,"']'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,
"'}'","'~'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"CLASS","PUBLIC","PRIVATE","PROTECTED",
"VIRTUAL","STRING","NUM","ID","INT","FLOAT","SHORT","LONG","DOUBLE","VOID",
"CHAR","CLASS_REF","OTHER","CONST","OPERATOR","UNSIGNED","FRIEND","VTK_ID",
"STATIC","VAR_FUNCTION","ARRAY_NUM","SetMacro","GetMacro","SetStringMacro",
"GetStringMacro","SetClampMacro","SetObjectMacro",
"SetReferenceCountedObjectMacro","GetObjectMacro","BooleanMacro",
"SetVector2Macro","SetVector3Macro","SetVector4Macro","SetVector6Macro",
"GetVector2Macro","GetVector3Macro","GetVector4Macro","GetVector6Macro",
"SetVectorMacro","GetVectorMacro","ViewportCoordinateMacro",
"WorldCoordinateMacro",
};
static YYCONST char *YYCONST yyrule[] = {
"$accept : strt",
"strt : maybe_other class_def maybe_other",
"$$1 :",
"class_def : CLASS VTK_ID $$1 optional_scope '{' class_def_body '}'",
"class_def_body : class_def_item",
"class_def_body : class_def_item class_def_body",
"class_def_item : scope_type ':'",
"class_def_item : var",
"class_def_item : function",
"class_def_item : FRIEND function",
"class_def_item : macro ';'",
"class_def_item : macro",
"function : '~' func",
"function : VIRTUAL '~' func",
"function : func",
"function : type func",
"function : VIRTUAL type func",
"function : VIRTUAL func",
"$$2 :",
"func : func_beg $$2 func_end",
"func : OPERATOR maybe_other_no_semi ';'",
"func : func_beg '=' NUM ';'",
"$$3 :",
"func_beg : any_id '(' $$3 args_list ')'",
"any_id : VTK_ID",
"any_id : ID",
"func_end : ';'",
"func_end : CONST ';'",
"func_end : CONST '{' maybe_other '}' ';'",
"func_end : '{' maybe_other '}' ';'",
"func_end : CONST '{' maybe_other '}'",
"func_end : '{' maybe_other '}'",
"func_end : ':' maybe_other_no_semi ';'",
"args_list :",
"args_list : more_args",
"more_args : arg",
"$$4 :",
"more_args : arg $$4 ',' more_args",
"arg : type",
"$$5 :",
"arg : type var_id $$5 opt_var_assign",
"arg : VAR_FUNCTION",
"opt_var_assign :",
"opt_var_assign : '=' float_num",
"var : type var_id ';'",
"var : VAR_FUNCTION ';'",
"var_id : any_id var_array",
"var_array :",
"$$6 :",
"var_array : ARRAY_NUM $$6 var_array",
"var_array : '[' maybe_other_no_semi ']' var_array",
"type : CONST type_red1",
"type : type_red1",
"type : STATIC type_red1",
"type : STATIC CONST type_red1",
"type_red1 : type_red2",
"type_red1 : type_red2 type_indirection",
"type_indirection : '&'",
"type_indirection : '*'",
"type_indirection : '&' type_indirection",
"type_indirection : '*' type_indirection",
"$$7 :",
"type_red2 : UNSIGNED $$7 type_primitive",
"type_red2 : type_primitive",
"type_primitive : FLOAT",
"type_primitive : VOID",
"type_primitive : CHAR",
"type_primitive : INT",
"type_primitive : SHORT",
"type_primitive : LONG",
"type_primitive : DOUBLE",
"type_primitive : ID",
"type_primitive : VTK_ID",
"optional_scope :",
"optional_scope : ':' scope_list",
"scope_list : scope_type VTK_ID",
"$$8 :",
"scope_list : scope_type VTK_ID $$8 ',' scope_list",
"scope_type : PUBLIC",
"scope_type : PRIVATE",
"scope_type : PROTECTED",
"float_num : '-' float_prim",
"float_num : float_prim",
"float_prim : NUM",
"float_prim : NUM '.' NUM",
"float_prim : any_id",
"$$9 :",
"macro : SetMacro '(' any_id ',' $$9 type_red2 ')'",
"$$10 :",
"$$11 :",
"macro : GetMacro '(' $$10 any_id ',' $$11 type_red2 ')'",
"$$12 :",
"macro : SetStringMacro '(' $$12 any_id ')'",
"$$13 :",
"macro : GetStringMacro '(' $$13 any_id ')'",
"$$14 :",
"$$15 :",
"macro : SetClampMacro '(' any_id ',' $$14 type_red2 $$15 ',' maybe_other_no_semi ')'",
"$$16 :",
"macro : SetObjectMacro '(' any_id ',' $$16 type_red2 ')'",
"$$17 :",
"macro : SetReferenceCountedObjectMacro '(' any_id ',' $$17 type_red2 ')'",
"$$18 :",
"$$19 :",
"macro : GetObjectMacro '(' $$18 any_id ',' $$19 type_red2 ')'",
"$$20 :",
"macro : BooleanMacro '(' any_id $$20 ',' type_red2 ')'",
"$$21 :",
"macro : SetVector2Macro '(' any_id ',' $$21 type_red2 ')'",
"$$22 :",
"macro : GetVector2Macro '(' any_id ',' $$22 type_red2 ')'",
"$$23 :",
"macro : SetVector3Macro '(' any_id ',' $$23 type_red2 ')'",
"$$24 :",
"macro : GetVector3Macro '(' any_id ',' $$24 type_red2 ')'",
"$$25 :",
"macro : SetVector4Macro '(' any_id ',' $$25 type_red2 ')'",
"$$26 :",
"macro : GetVector4Macro '(' any_id ',' $$26 type_red2 ')'",
"$$27 :",
"macro : SetVector6Macro '(' any_id ',' $$27 type_red2 ')'",
"$$28 :",
"macro : GetVector6Macro '(' any_id ',' $$28 type_red2 ')'",
"$$29 :",
"macro : SetVectorMacro '(' any_id ',' $$29 type_red2 ',' float_num ')'",
"$$30 :",
"macro : GetVectorMacro '(' any_id ',' $$30 type_red2 ',' float_num ')'",
"macro : ViewportCoordinateMacro '(' any_id ')'",
"macro : WorldCoordinateMacro '(' any_id ')'",
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
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif
#ifdef YYSTACKSIZE
#ifndef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#endif
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
#ifndef YYMAXSTACKSIZE
#define YYMAXSTACKSIZE 10000
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
YYSTYPE yyval;
YYSTYPE yylval;
static short *yyss;
static YYSTYPE *yyvs;
static int yystacksize;
#define yyfree(x) free(x)
extern int yylex();

static YYPTR
YYDEFUN (yymalloc, (bytes), unsigned bytes)
{
    YYPTR ptr = (YYPTR) malloc (bytes);
    if (ptr != 0) return (ptr);
    yyerror ("yyparse: memory exhausted");
    return (0);
}

static YYPTR
YYDEFUN (yyrealloc, (old, bytes), YYPTR old YYAND unsigned bytes)
{
    YYPTR ptr = (YYPTR) realloc (old, bytes);
    if (ptr != 0) return (ptr);
    yyerror ("yyparse: memory exhausted");
    return (0);
}

static int
#ifdef __GNUC__
__inline__
#endif
yygrow ()
{
    int old_stacksize = yystacksize;
    short *new_yyss;
    YYSTYPE *new_yyvs;

    if (yystacksize == YYMAXSTACKSIZE)
        return (1);
    yystacksize += (yystacksize + 1 ) / 2;
    if (yystacksize > YYMAXSTACKSIZE)
        yystacksize = YYMAXSTACKSIZE;
#if YYDEBUG
    if (yydebug)
        printf("yydebug: growing stack size from %d to %d\n",
               old_stacksize, yystacksize);
#endif
    new_yyss = (short *) yyrealloc ((char *)yyss, yystacksize * sizeof (short));
    if (new_yyss == 0)
        return (1);
    new_yyvs = (YYSTYPE *) yyrealloc ((char *)yyvs, yystacksize * sizeof (YYSTYPE));
    if (new_yyvs == 0)
    {
        yyfree (new_yyss);
        return (1);
    }
    yyss = new_yyss;
    yyvs = new_yyvs;
    return (0);
}
#line 812 "vtkParse.y"
#include <string.h>
#include "lex.yy.c"

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
  func->ReturnType = 2;
  func->ReturnClass = NULL;
  func->Comment = NULL;
  func->Signature = NULL;
  sigAllocatedLength = 0;
  openSig = 1;
  invertSig = 0;
}

/* when the cpp file doesn't have enough info use the hint file */
void look_for_hint()
{
  char h_cls[80];
  char h_func[80];
  int  h_type;
  int  h_value;

  /* reset the position */
  rewind(fhint);

  /* first find a hint */
  while (fscanf(fhint,"%s %s %i %i",h_cls,h_func,&h_type,&h_value) != EOF)
    {
    if ((!strcmp(h_cls,data.ClassName))&&
	currentFunction->Name &&
	(!strcmp(h_func,currentFunction->Name))&&
	(h_type == currentFunction->ReturnType))
      {
      currentFunction->HaveHint = 1;
      currentFunction->HintSize = h_value;
      }
    }
}

/* a simple routine that updates a few variables */
void output_function()
{
  /* a void argument is the same as no arguements */
  if (currentFunction->ArgTypes[0]%1000 == 2) 
    {
    currentFunction->NumberOfArguments = 0;
    }

  currentFunction->IsPublic = in_public;
  
  /* look for VAR FUNCTIONS */
  if (currentFunction->NumberOfArguments
      && (currentFunction->ArgTypes[0] == 5000))
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
    switch (currentFunction->ReturnType%1000)
      {
      case 301: case 302: case 307:
      case 304: case 305: case 306:
        look_for_hint();
	break;
      }
    }

  if (HaveComment)
    {
    currentFunction->Comment = strdup(CommentText);
    }
  
  data.NumberOfFunctions++;
  currentFunction = data.Functions + data.NumberOfFunctions;
  InitFunction(currentFunction);
}

extern void vtkParseOutput(FILE *,FileInfo *);

int main(int argc,char *argv[])
{
  FILE *fin;

  if (argc != 4)
    {
    fprintf(stderr,"Usage: %s input_file hint_file is_concrete\n",argv[0]);
    exit(1);
    }
  
  if (!(fin = fopen(argv[1],"r")))
    {
    fprintf(stderr,"Error opening input file %s\n",argv[1]);
    exit(1);
    }

  if (!(fhint = fopen(argv[2],"r")))
    {
    fprintf(stderr,"Error opening hint file %s\n",argv[2]);
    exit(1);
    }

  data.FileName = argv[1];
  data.NameComment = NULL;
  data.Description = NULL;
  data.Caveats = NULL;
  data.SeeAlso = NULL;
  CommentState = 0;
  data.IsConcrete = atoi(argv[3]);

  currentFunction = data.Functions;
  InitFunction(currentFunction);
  
  yyin = fin;
  yyout = stdout;
  yyparse();
  vtkParseOutput(stdout,&data);
  return 0;
}
 


#line 863 "vtkParse.tab.c"
#define YYABORT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab

#if YYDEBUG
#ifdef __cplusplus
extern "C" char *getenv();
#else
extern char *getenv();
#endif
#endif

int
yyparse()
{
    register int yym, yyn, yystate;
    register YYSTYPE *yyvsp;
    register short *yyssp;
    short *yysse;
#if YYDEBUG
    register YYCONST char *yys;

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == 0)
    {
        yyss = (short *) yymalloc (YYSTACKSIZE * sizeof (short));
        if (yyss == 0)
          goto yyabort;
        yyvs = (YYSTYPE *) yymalloc (YYSTACKSIZE * sizeof (YYSTYPE));
        if (yyvs == 0)
        {
            yyfree (yyss);
            goto yyabort;
        }
        yystacksize = YYSTACKSIZE;
    }
    yysse = yyss + yystacksize - 1;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;
    goto yyloop;

yypush_lex:
    yyval = yylval;
    yystate = yytable[yyn];
yypush:
    if (yyssp >= yysse)
    {
        int depth = yyssp - yyss;
        if (yygrow() != 0)
             goto yyoverflow;
        yysse = yyss + yystacksize -1;
        yyssp = depth + yyss;
        yyvsp = depth + yyvs;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    yyn = yysindex[yystate];
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("yydebug: state %d, reading %d (%s)\n", yystate,
                    yychar, yys);
        }
#endif
    }
    if (yyn != 0
        && ((yyn += yychar), ((unsigned)yyn <= (unsigned)YYTABLESIZE))
        && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("yydebug: state %d, shifting to state %d\n",
                    yystate, yytable[yyn]);
#endif
        if (yyerrflag > 0)  --yyerrflag;
        yychar = (-1);
        goto yypush_lex;
    }
    yyn = yyrindex[yystate];
    if (yyn != 0
        && ((yyn += yychar), ((unsigned)yyn <= (unsigned)YYTABLESIZE))
        && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            yyn = yysindex[*yyssp];
            if (yyn != 0
                && ((yyn += YYERRCODE), ((unsigned)yyn <= (unsigned)YYTABLESIZE))
                && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("yydebug: state %d, error recovery shifting\
 to state %d\n", *yyssp, yytable[yyn]);
#endif
                goto yypush_lex;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("yydebug: error recovery discarding state %d\n",
                            *yyssp);
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
            printf("yydebug: state %d, error recovery discards token %d (%s)\n",
                    yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("yydebug: state %d, reducing by rule %d (%s)\n",
                yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 2:
#line 180 "vtkParse.y"
{
      data.ClassName = strdup(yyvsp[0].str);
      }
break;
case 12:
#line 190 "vtkParse.y"
{ preSig("~"); output_function(); }
break;
case 13:
#line 191 "vtkParse.y"
{ preSig("virtual ~"); output_function(); }
break;
case 14:
#line 193 "vtkParse.y"
{
         output_function();
	 }
break;
case 15:
#line 197 "vtkParse.y"
{
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
	 }
break;
case 16:
#line 202 "vtkParse.y"
{
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
	 }
break;
case 17:
#line 208 "vtkParse.y"
{
         preSig("virtual ");
         output_function();
	 }
break;
case 18:
#line 213 "vtkParse.y"
{ postSig(");"); openSig = 0; }
break;
case 19:
#line 214 "vtkParse.y"
{
      openSig = 1;
      currentFunction->Name = yyvsp[-2].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-2].str); 
    }
break;
case 20:
#line 220 "vtkParse.y"
{ 
      currentFunction->IsOperator = 1; 
      fprintf(stderr,"   Converted operator\n"); 
    }
break;
case 21:
#line 225 "vtkParse.y"
{ 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-3].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-3].str); 
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    }
break;
case 22:
#line 233 "vtkParse.y"
{postSig(" ("); }
break;
case 24:
#line 236 "vtkParse.y"
{postSig(yyvsp[0].str);}
break;
case 25:
#line 236 "vtkParse.y"
{postSig(yyvsp[0].str);}
break;
case 35:
#line 248 "vtkParse.y"
{ currentFunction->NumberOfArguments++;}
break;
case 36:
#line 249 "vtkParse.y"
{ currentFunction->NumberOfArguments++; postSig(", ");}
break;
case 38:
#line 252 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yyvsp[0].integer;}
break;
case 39:
#line 257 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yyvsp[-1].integer;
    }
break;
case 41:
#line 263 "vtkParse.y"
{ 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 5000;
    }
break;
case 48:
#line 277 "vtkParse.y"
{char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); postSig(temp);}
break;
case 49:
#line 278 "vtkParse.y"
{ currentFunction->ArrayFailure = 1; }
break;
case 50:
#line 280 "vtkParse.y"
{ postSig("[]"); currentFunction->ArrayFailure = 1; }
break;
case 51:
#line 283 "vtkParse.y"
{yyval.integer = 1000 + yyvsp[0].integer;}
break;
case 52:
#line 284 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 53:
#line 285 "vtkParse.y"
{yyval.integer = 2000 + yyvsp[0].integer;}
break;
case 54:
#line 286 "vtkParse.y"
{yyval.integer = 3000 + yyvsp[0].integer;}
break;
case 55:
#line 288 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 56:
#line 290 "vtkParse.y"
{yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;}
break;
case 57:
#line 299 "vtkParse.y"
{ postSig("&"); yyval.integer = 100;}
break;
case 58:
#line 300 "vtkParse.y"
{ postSig("*"); yyval.integer = 300;}
break;
case 59:
#line 301 "vtkParse.y"
{ yyval.integer = 100 + yyvsp[0].integer;}
break;
case 60:
#line 302 "vtkParse.y"
{ yyval.integer = 400 + yyvsp[0].integer;}
break;
case 61:
#line 304 "vtkParse.y"
{postSig("unsigned ");}
break;
case 62:
#line 305 "vtkParse.y"
{ yyval.integer = 10 + yyvsp[0].integer;}
break;
case 63:
#line 306 "vtkParse.y"
{ yyval.integer = yyvsp[0].integer;}
break;
case 64:
#line 309 "vtkParse.y"
{ postSig("float "); yyval.integer = 1;}
break;
case 65:
#line 310 "vtkParse.y"
{ postSig("void "); yyval.integer = 2;}
break;
case 66:
#line 311 "vtkParse.y"
{ postSig("char "); yyval.integer = 3;}
break;
case 67:
#line 312 "vtkParse.y"
{ postSig("int "); yyval.integer = 4;}
break;
case 68:
#line 313 "vtkParse.y"
{ postSig("short "); yyval.integer = 5;}
break;
case 69:
#line 314 "vtkParse.y"
{ postSig("long "); yyval.integer = 6;}
break;
case 70:
#line 315 "vtkParse.y"
{ postSig("double "); yyval.integer = 7;}
break;
case 71:
#line 316 "vtkParse.y"
{ char ctmpid[2048]; sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid); yyval.integer = 8;}
break;
case 72:
#line 318 "vtkParse.y"
{ 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 9; 
      currentFunction->ArgClasses[currentFunction->NumberOfArguments] =
        strdup(yyvsp[0].str); 
      /* store the string into the return value just in case we need it */
      /* this is a parsing hack because the first "type" parser will */
      /* possibly be ht ereturn type of the first argument */
      if ((!currentFunction->ReturnClass) && 
          (!currentFunction->NumberOfArguments)) 
        { 
        currentFunction->ReturnClass = strdup(yyvsp[0].str); 
        }
    }
break;
case 75:
#line 336 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
break;
case 76:
#line 341 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
break;
case 78:
#line 346 "vtkParse.y"
{in_public = 1;}
break;
case 79:
#line 346 "vtkParse.y"
{in_public = 0;}
break;
case 80:
#line 347 "vtkParse.y"
{in_public = 0;}
break;
case 83:
#line 351 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 84:
#line 352 "vtkParse.y"
{yyval.integer = -1;}
break;
case 85:
#line 352 "vtkParse.y"
{yyval.integer = -1;}
break;
case 86:
#line 356 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 87:
#line 357 "vtkParse.y"
{
   postSig(");");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
break;
case 88:
#line 367 "vtkParse.y"
{postSig("Get");}
break;
case 89:
#line 367 "vtkParse.y"
{postSig(" ();"); invertSig = 1;}
break;
case 90:
#line 369 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   }
break;
case 91:
#line 376 "vtkParse.y"
{preSig("void Set");}
break;
case 92:
#line 377 "vtkParse.y"
{
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",yyvsp[-1].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
break;
case 93:
#line 387 "vtkParse.y"
{preSig("char *Get");}
break;
case 94:
#line 388 "vtkParse.y"
{ 
   postSig(" ();");
   sprintf(temps,"Get%s",yyvsp[-1].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 303;
   output_function();
   }
break;
case 95:
#line 397 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 96:
#line 398 "vtkParse.y"
{postSig(");"); openSig = 0;}
break;
case 97:
#line 399 "vtkParse.y"
{ 
   sprintf(temps,"Set%s",yyvsp[-7].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-4].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
break;
case 98:
#line 409 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 99:
#line 410 "vtkParse.y"
{ 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   }
break;
case 100:
#line 421 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 101:
#line 422 "vtkParse.y"
{ 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   }
break;
case 102:
#line 432 "vtkParse.y"
{postSig("*Get");}
break;
case 103:
#line 433 "vtkParse.y"
{postSig(" ();"); invertSig = 1;}
break;
case 104:
#line 434 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   output_function();
   }
break;
case 105:
#line 442 "vtkParse.y"
{preSig("void "); postSig("On ();"); openSig = 0; }
break;
case 106:
#line 444 "vtkParse.y"
{ 
   sprintf(temps,"%sOn",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 2;
   output_function();
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void %sOff ();",yyvsp[-4].str); 
   sprintf(temps,"%sOff",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   output_function();
   }
break;
case 107:
#line 459 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 108:
#line 464 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s , %s);",yyvsp[-4].str,
     local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 2;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yyvsp[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[2]);",yyvsp[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 2;
   output_function();
   }
break;
case 109:
#line 489 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 110:
#line 494 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   }
break;
case 111:
#line 506 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 112:
#line 511 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s , %s, %s);",
     yyvsp[-4].str, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 3;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yyvsp[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = yyvsp[-1].integer;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[3]);",yyvsp[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 3;
   output_function();
   }
break;
case 113:
#line 538 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 114:
#line 543 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   }
break;
case 115:
#line 555 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 116:
#line 560 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s , %s, %s, %s);",
     yyvsp[-4].str, local, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 4;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ArgTypes[1] = yyvsp[-1].integer;
   currentFunction->ArgCounts[1] = 0;
   currentFunction->ArgTypes[2] = yyvsp[-1].integer;
   currentFunction->ArgCounts[2] = 0;
   currentFunction->ArgTypes[3] = yyvsp[-1].integer;
   currentFunction->ArgCounts[3] = 0;
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[4]);",yyvsp[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 4;
   output_function();
   }
break;
case 117:
#line 589 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 118:
#line 594 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   }
break;
case 119:
#line 606 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 120:
#line 611 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s , %s, %s, %s, %s, %s);",
     yyvsp[-4].str, local, local, local, local, local, local);
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
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
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[6]);",yyvsp[-4].str,
     local);
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 6;
   output_function();
   }
break;
case 121:
#line 644 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 122:
#line 649 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   }
break;
case 123:
#line 661 "vtkParse.y"
{
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
break;
case 124:
#line 666 "vtkParse.y"
{
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",yyvsp[-6].str,
      local, yyvsp[-1].integer);
     sprintf(temps,"Set%s",yyvsp[-6].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->ReturnType = 2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 300 + yyvsp[-3].integer;
     currentFunction->ArgCounts[0] = yyvsp[-1].integer;
     output_function();
   }
break;
case 125:
#line 679 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 126:
#line 684 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-6].str);
   sprintf(temps,"Get%s",yyvsp[-6].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-3].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = yyvsp[-1].integer;
   output_function();
   }
break;
case 127:
#line 696 "vtkParse.y"
{ 
     char *local = strdup(currentFunction->Signature);
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float , float);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = 1;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 1;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ReturnType = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float a[2]);",
       yyvsp[-1].str);
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 301;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"float *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 301;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
break;
case 128:
#line 744 "vtkParse.y"
{ 
     char *local = strdup(currentFunction->Signature);
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = strdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float , float, float);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 3;
     currentFunction->ArgTypes[0] = 1;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 1;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ArgTypes[2] = 1;
     currentFunction->ArgCounts[2] = 0;
     currentFunction->ReturnType = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (float a[3]);",
       yyvsp[-1].str);
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 301;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"float *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = strdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 301;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   }
break;
#line 1843 "vtkParse.tab.c"
    }
    yyssp -= yym;
    yyvsp -= yym;
    yym = yylhs[yyn];
    yystate = *yyssp;
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("yydebug: after reduction, shifting from state 0 to\
 state %d\n", YYFINAL);
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
                printf("yydebug: state %d, reading %d (%s)\n",
                        YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    yyn = yygindex[yym];
	 if (yyn != 0
        && ((yyn += yystate), ((unsigned)yyn <= (unsigned)YYTABLESIZE))
        && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("yydebug: after reduction, shifting from state %d \
to state %d\n", *yyssp, yystate);
#endif
    goto yypush;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
