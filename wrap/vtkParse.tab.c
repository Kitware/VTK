#ifndef lint
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 44 "vtkParse.y"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1

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
  int sigAllocatedLength;
  int isEmpty;
  
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
      tmp = strdup(currentFunction->Signature);
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
  void delSig(void)
    {
    if (currentFunction->Signature)
      {
      free(currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
    }
  void emptyFile(void)
    {
    isEmpty = 1;
    }
#line 134 "vtkParse.y"
typedef union{
  char *str;
  int   integer;
  } YYSTYPE;
#line 106 "vtkParse.tab.c"
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
#define TypeMacro 303
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    4,    2,    5,    5,    6,    6,    6,    6,
    6,    6,    9,    9,    9,    9,    9,    9,   15,   17,
   11,   11,   11,   14,   14,   21,   13,   22,   23,   19,
   19,   16,   16,   16,   16,   20,   20,   24,   26,   24,
   25,   29,   25,   25,   28,   28,    8,    8,   27,   31,
   32,   31,   31,   12,   12,   12,   12,   33,   33,   35,
   35,   35,   35,   37,   34,   34,   36,   36,   36,   36,
   36,   36,   36,   36,   36,    3,    3,   38,   39,   38,
    7,    7,    7,   30,   30,   40,   40,   40,   41,   10,
   42,   43,   10,   44,   10,   45,   10,   46,   47,   10,
   48,   10,   49,   10,   50,   51,   10,   52,   10,   53,
   10,   54,   10,   55,   10,   56,   10,   57,   10,   58,
   10,   59,   10,   60,   10,   61,   10,   62,   10,   10,
   10,   10,    1,    1,   18,   18,   63,   63,   64,   64,
   64,   64,   64,   64,   64,   64,   64,   64,   64,   64,
   64,   64,   64,   64,   64,   64,   64,   64,   65,   66,
   67,
};
short yylen[] = {                                         2,
    3,    1,    0,    7,    1,    2,    2,    1,    1,    2,
    2,    1,    2,    3,    1,    2,    3,    2,    0,    0,
    5,    3,    4,    0,    1,    0,    5,    1,    1,    1,
    1,    1,    4,    3,    3,    0,    1,    1,    0,    4,
    1,    0,    4,    1,    0,    2,    3,    2,    2,    0,
    0,    3,    4,    2,    1,    2,    3,    1,    2,    1,
    1,    2,    2,    0,    3,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    0,    2,    2,    0,    5,
    1,    1,    1,    2,    1,    1,    3,    1,    0,    7,
    0,    0,    8,    0,    5,    0,    5,    0,    0,   10,
    0,    7,    0,    7,    0,    0,    8,    0,    7,    0,
    7,    0,    7,    0,    7,    0,    7,    0,    7,    0,
    7,    0,    7,    0,    7,    0,    9,    0,    9,    4,
    4,    6,    0,    2,    0,    2,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    3,    3,
    3,
};
short yydefred[] = {                                      0,
  147,  149,   74,   70,   67,   71,   72,   73,   68,   69,
  150,  139,  153,  154,   64,   75,  157,  158,    0,  144,
  137,  156,  143,    0,  145,    0,  151,  142,  155,  146,
    0,    0,  148,   66,    0,  138,  140,  141,  152,    0,
    0,    0,    0,    0,    0,  134,   65,  159,  160,  161,
    3,    1,    0,    0,    0,   81,   82,   83,    0,   77,
    0,    0,    0,    0,   28,    0,    0,    0,   29,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    8,    9,    0,   15,
    0,    0,    0,    0,    0,   55,    0,    0,    0,   18,
    0,    0,    0,   10,    0,   48,    0,   91,   94,   96,
    0,    0,    0,  105,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   31,   30,
   13,    4,    6,    7,   11,   16,    0,    0,    0,    0,
   26,   54,    0,   56,    0,    0,   59,    0,   14,   17,
   22,  136,    0,    0,    0,    0,    0,    0,    0,    0,
  108,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   51,    0,   49,   47,    0,   25,
   20,    0,   57,   62,   63,   80,   89,    0,    0,    0,
   98,  101,  103,    0,    0,  110,  114,  118,  122,  112,
  116,  120,  124,  126,  128,  130,  131,    0,    0,    0,
   23,    0,   44,    0,    0,   37,    0,    0,   92,   95,
   97,    0,    0,    0,  106,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   52,    0,    0,
    0,   32,   21,    0,   42,   27,    0,    0,    0,   99,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  132,   53,    0,    0,    0,    0,
   90,    0,    0,  102,  104,    0,  109,  111,  115,  119,
  123,  113,  117,  121,  125,    0,    0,    0,   35,    0,
   43,   40,   93,    0,  107,    0,    0,   88,    0,   85,
    0,   33,   46,    0,    0,   84,  127,  129,  100,   87,
};
short yydgoto[] = {                                      31,
   32,   45,   55,   53,   94,   95,   59,   97,   98,   99,
  100,  101,  102,  191,  150,  253,  222,  112,  103,  225,
  192,  104,  105,  226,  227,  257,  148,  301,  279,  309,
  187,  219,  106,   33,  157,   34,   40,   60,  108,  310,
  228,  164,  259,  165,  166,  232,  283,  233,  234,  170,
  263,  205,  237,  241,  238,  242,  239,  243,  240,  244,
  245,  246,   35,   36,   37,   38,   39,
};
short yysindex[] = {                                    -35,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -35,    0,
    0,    0,    0,  -35,    0,  -35,    0,    0,    0,    0,
    0, -232,    0,    0,  -35,    0,    0,    0,    0,  145,
  -98,   23,  -53, -207,  -35,    0,    0,    0,    0,    0,
    0,    0,   29, -183,  -34,    0,    0,    0, -178,    0,
  305,    0,  -58,    0,    0,   -1,  -97,    0,    0,   42,
   72,   84,   87,   88,   89,   90,   91,   93,   94,   95,
   96,   97,  107,  108,  110,  112,  116,  117,  118,  121,
  123,  125, -169,   50,  305,  122,    0,    0,  124,    0,
 -169,  115,  144,  130,   40,    0,    9,  141, -169,    0,
 -169,  127,   -1,    0, -169,    0, -247,    0,    0,    0,
 -247, -247, -247,    0, -247, -247, -247, -247, -247, -247,
 -247, -247, -247, -247, -247, -247, -247, -247,    0,    0,
    0,    0,    0,    0,    0,    0,  -36,  128,  -75,  -84,
    0,    0,  130,    0,    9,    9,    0, -183,    0,    0,
    0,    0,  148, -247, -247, -247,  149,  151,  154, -247,
    0,  155,  156,  157,  158,  161,  170,  171,  175,  178,
  179,  183,  184,  182,    0,   -1,    0,    0,  190,    0,
    0, -125,    0,    0,    0,    0,    0,  207,  213,  214,
    0,    0,    0,  216,  235,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -247,  -85,  164,
    0,  -44,    0, -247,  241,    0,    0,  130,    0,    0,
    0,  130,  130,  130,    0,  130,  130,  130,  130,  130,
  130,  130,  130,  130,  130,  130,  242,    0,  -85,  -35,
   -1,    0,    0,  -85,    0,    0,  240,  260,  130,    0,
  261,  271,  130,  272,  274,  276,  278,  279,  280,  281,
  282,  283,  284,  285,    0,    0,  200,  267,  269, -125,
    0,  293,  302,    0,    0,  308,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   25,   25,  291,    0,   25,
    0,    0,    0,   -1,    0,  143, -160,    0,  310,    0,
  311,    0,    0,  312,   92,    0,    0,    0,    0,    0,
};
short yyrindex[] = {                                      2,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  229,    0,
    0,    0,    0,  316,    0,  266,    0,    0,    0,    0,
    0,  360,    0,    0,    1,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  361,    0,    0,    0,    0,    0,
    0,    0,  243,    0,    0,    0,    0,    0,    0,    0,
    0,  -31,    0,  -22,    0,  304,    0,   12,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  239,    0,    0,    0,  168,    0,
    0,  -26,    0,    0,    0,    0,   17,    0,    0,    0,
    0,    0,  -13,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  306,    0,    0,  -37,
    0,    0,    0,    0,   21,   22,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  275,    0,    0,    0,    0,
    0,  326,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   37,    0,
    0,    0,    0,   64,    0,    0,   69,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   37,  229,
  304,    0,    0,   41,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   70,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  222,    0,    0,
    0,    0,    0,  328,    0,   75,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
   48,    0,    0,    0,  307,    0,  -42,    0,  303,    0,
    6,  -33,    0,    0,    0,    0,    0, -113,  255,    0,
    0,  268,    0,  137,    0,    0,  147,    0,    0, -177,
 -211,    0,  -69,   99, -107,  334,    0,  217,    0,   98,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -54,    0,    0,    0,
};
#define YYTABLESIZE 608
short yytable[] = {                                     162,
  133,  133,   27,  151,   24,  186,   28,  248,   25,   29,
   30,  113,   79,  251,  252,   74,  139,   31,   96,   74,
   24,   24,   20,   21,   44,   23,   48,  135,   93,  111,
  140,   19,   19,  115,  152,  154,   27,  276,   24,   50,
   28,  133,   25,   29,   30,  135,  155,  194,  195,   75,
  156,   30,   96,   75,  186,   26,   20,   58,  113,   23,
   58,   60,   61,   49,   60,   61,   41,  109,  110,  307,
   51,   42,  220,   43,   56,   57,   58,   50,  250,  135,
   50,   50,   46,  193,   50,   24,   54,   19,   61,   26,
   22,   78,   52,  133,  139,   50,   19,   50,  141,   62,
  116,   50,  306,  139,   41,   66,  146,   41,  140,   38,
   45,  117,   39,   45,  159,   86,  160,  140,   86,  311,
  146,   19,  313,  118,   22,  133,  119,  120,  121,  122,
  123,  113,  124,  125,  126,  127,  128,  278,    3,    4,
    5,    6,    7,    8,    9,   10,  129,  130,   65,  131,
   15,  132,   16,   69,  223,  133,  134,  135,  224,  107,
  136,  107,  137,   63,  138,  107,   64,    4,    5,    6,
    7,    8,    9,   10,  142,  149,   65,   66,   15,  144,
   68,   69,  145,  151,  158,  161,  188,  189,  315,  190,
  314,  197,  201,  107,  202,  185,  113,  203,  206,  207,
  208,  209,  107,  107,  210,   64,    4,    5,    6,    7,
    8,    9,   10,  211,  212,   65,   66,   15,  213,   68,
   69,  214,  215,  216,  217,  218,    1,    2,    3,    4,
    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
   15,   74,   16,   17,  185,   18,  224,   19,  221,  113,
  229,  107,   74,  230,  231,   74,  249,  133,  133,  235,
    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,
   11,   12,   13,   14,   15,   75,   16,   17,  236,   18,
   58,  256,  275,  280,   60,   61,   75,  306,  139,   75,
  107,   58,   12,   12,   58,   60,   61,  277,   60,   61,
  281,  284,  140,    3,    4,    5,    6,    7,    8,    9,
   10,  285,  287,   65,  288,   15,  289,   16,  290,  291,
  292,  293,  294,  295,  298,  299,  258,  296,  297,  300,
  260,  261,  262,  303,  264,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  304,   34,   34,  305,  312,
  317,  318,  319,  133,  320,  147,  133,  282,  133,    2,
  133,  286,  135,    5,   50,   76,   36,  135,  135,  114,
  255,  163,  153,   47,  196,  167,  168,  169,  107,  171,
  172,  173,  174,  175,  176,  177,  178,  179,  180,  181,
  182,  183,  184,    3,    4,    5,    6,    7,    8,    9,
   10,  143,    0,    0,  316,   15,    0,   16,    3,    4,
    5,    6,    7,    8,    9,   10,  302,    0,  198,  199,
  200,    0,   16,    0,  204,   12,   12,   12,   12,    0,
   93,   12,   12,   12,   12,   12,   12,   12,   12,    0,
    0,   12,   12,   12,   12,   12,   12,   12,    0,   12,
   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
   12,    0,  247,    0,    0,    0,    0,    0,  254,   34,
   34,   34,   34,    0,    0,   34,   34,   34,   34,   34,
   34,   34,   34,    0,    0,   34,   34,   34,   34,   34,
   34,   34,    0,   34,   34,   34,   34,   34,   34,   34,
   34,   34,   34,   34,   34,   34,   34,   34,   34,   34,
   34,   34,   34,   34,   34,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  308,  308,    0,    0,  308,    0,    0,    0,    0,    0,
    0,  308,   56,   57,   58,   63,    0,    0,   64,    4,
    5,    6,    7,    8,    9,   10,    0,    0,   65,   66,
   15,   67,   68,   69,   70,    0,   71,   72,   73,   74,
   75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
   85,   86,   87,   88,   89,   90,   91,   92,
};
short yycheck[] = {                                     113,
    0,    0,   38,   40,   40,   91,   42,  219,   44,   45,
   46,   66,   44,   58,   59,   38,  264,   40,   61,   42,
   58,   59,   58,   59,  257,   61,  125,   41,  126,   63,
  278,   58,   59,   67,  104,  105,   38,  249,   40,   93,
   42,   41,   44,   45,   46,   59,   38,  155,  156,   38,
   42,   40,   95,   42,   91,   91,   58,   41,  113,   61,
   44,   41,   41,   41,   44,   44,   19,  126,   63,   45,
  278,   24,  186,   26,  258,  259,  260,   41,  123,   93,
   44,   41,   35,  153,   44,  123,   58,  123,  123,   91,
  126,  123,   45,   93,  264,   59,  123,   61,   93,  278,
   59,   61,  263,  264,   41,  275,  101,   44,  278,   41,
   41,   40,   44,   44,  109,   41,  111,  278,   44,  297,
  115,  123,  300,   40,  126,  125,   40,   40,   40,   40,
   40,  186,   40,   40,   40,   40,   40,  251,  264,  265,
  266,  267,  268,  269,  270,  271,   40,   40,  274,   40,
  276,   40,  278,  279,  280,   40,   40,   40,  192,   61,
   40,   63,   40,  261,   40,   67,  264,  265,  266,  267,
  268,  269,  270,  271,  125,   61,  274,  275,  276,   58,
  278,  279,   59,   40,   44,   59,   59,  263,   46,  274,
  304,   44,   44,   95,   44,  281,  251,   44,   44,   44,
   44,   44,  104,  105,   44,  264,  265,  266,  267,  268,
  269,  270,  271,   44,   44,  274,  275,  276,   44,  278,
  279,   44,   44,   41,   41,   44,  262,  263,  264,  265,
  266,  267,  268,  269,  270,  271,  272,  273,  274,  275,
  276,  264,  278,  279,  281,  281,  280,  274,   59,  304,
   44,  153,  275,   41,   41,  278,   93,  257,  257,   44,
  262,  263,  264,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,  276,  264,  278,  279,   44,  281,
  264,   41,   41,   44,  264,  264,  275,  263,  264,  278,
  192,  275,  125,  126,  278,  275,  275,  250,  278,  278,
   41,   41,  278,  264,  265,  266,  267,  268,  269,  270,
  271,   41,   41,  274,   41,  276,   41,  278,   41,   41,
   41,   41,   41,   41,  125,   59,  228,   44,   44,   61,
  232,  233,  234,   41,  236,  237,  238,  239,  240,  241,
  242,  243,  244,  245,  246,   44,  125,  126,   41,   59,
   41,   41,   41,  125,  263,  101,   41,  259,   93,    0,
    0,  263,   59,  125,   59,  123,   41,   93,   41,   67,
  224,  117,  105,   40,  158,  121,  122,  123,  280,  125,
  126,  127,  128,  129,  130,  131,  132,  133,  134,  135,
  136,  137,  138,  264,  265,  266,  267,  268,  269,  270,
  271,   95,   -1,   -1,  307,  276,   -1,  278,  264,  265,
  266,  267,  268,  269,  270,  271,  280,   -1,  164,  165,
  166,   -1,  278,   -1,  170,  258,  259,  260,  261,   -1,
  126,  264,  265,  266,  267,  268,  269,  270,  271,   -1,
   -1,  274,  275,  276,  277,  278,  279,  280,   -1,  282,
  283,  284,  285,  286,  287,  288,  289,  290,  291,  292,
  293,  294,  295,  296,  297,  298,  299,  300,  301,  302,
  303,   -1,  218,   -1,   -1,   -1,   -1,   -1,  224,  258,
  259,  260,  261,   -1,   -1,  264,  265,  266,  267,  268,
  269,  270,  271,   -1,   -1,  274,  275,  276,  277,  278,
  279,  280,   -1,  282,  283,  284,  285,  286,  287,  288,
  289,  290,  291,  292,  293,  294,  295,  296,  297,  298,
  299,  300,  301,  302,  303,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  296,  297,   -1,   -1,  300,   -1,   -1,   -1,   -1,   -1,
   -1,  307,  258,  259,  260,  261,   -1,   -1,  264,  265,
  266,  267,  268,  269,  270,  271,   -1,   -1,  274,  275,
  276,  277,  278,  279,  280,   -1,  282,  283,  284,  285,
  286,  287,  288,  289,  290,  291,  292,  293,  294,  295,
  296,  297,  298,  299,  300,  301,  302,  303,
};
#define YYFINAL 31
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 303
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
"VIRTUAL","STRING","NUM","ID","INT","FLOAT","SHORT","LONG","DOUBLE","VOID",
"CHAR","CLASS_REF","OTHER","CONST","OPERATOR","UNSIGNED","FRIEND","VTK_ID",
"STATIC","VAR_FUNCTION","ARRAY_NUM","SetMacro","GetMacro","SetStringMacro",
"GetStringMacro","SetClampMacro","SetObjectMacro",
"SetReferenceCountedObjectMacro","GetObjectMacro","BooleanMacro",
"SetVector2Macro","SetVector3Macro","SetVector4Macro","SetVector6Macro",
"GetVector2Macro","GetVector3Macro","GetVector4Macro","GetVector6Macro",
"SetVectorMacro","GetVectorMacro","ViewportCoordinateMacro",
"WorldCoordinateMacro","TypeMacro",
};
char *yyrule[] = {
"$accept : strt",
"strt : maybe_other class_def maybe_other",
"strt : maybe_other",
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
"$$3 :",
"func : func_beg $$2 maybe_const $$3 func_end",
"func : OPERATOR maybe_other_no_semi ';'",
"func : func_beg '=' NUM ';'",
"maybe_const :",
"maybe_const : CONST",
"$$4 :",
"func_beg : any_id '(' $$4 args_list ')'",
"const_mod : CONST",
"static_mod : STATIC",
"any_id : VTK_ID",
"any_id : ID",
"func_end : ';'",
"func_end : '{' maybe_other '}' ';'",
"func_end : '{' maybe_other '}'",
"func_end : ':' maybe_other_no_semi ';'",
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
"type_indirection : '&'",
"type_indirection : '*'",
"type_indirection : '&' type_indirection",
"type_indirection : '*' type_indirection",
"$$8 :",
"type_red2 : UNSIGNED $$8 type_primitive",
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
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#line 867 "vtkParse.y"
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
  currentFunction->IsProtected = in_protected;
  
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
      case 304: case 305: case 306: case 313:
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
  int ret;
  
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
  
  isEmpty = 0;
  yyin = fin;
  yyout = stdout;
  ret = yyparse();
  if (ret)
    {
    fprintf(stdout,
            "*** SYNTAX ERROR found in parsing the header file %s ***\n", 
            argv[1]);
    return ret;
    }
  if (isEmpty)
    {
    fprintf(stderr,"No class found in file\n");
    fprintf(stdout,"/* EMTPY FILE -- NO CLASS FOUND */\n");
    return 0;
    }
  vtkParseOutput(stdout,&data);
  return 0;
}
 


#line 816 "vtkParse.tab.c"
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

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

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
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
        if (yyssp >= yyss + yystacksize - 1)
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
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
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
#line 193 "vtkParse.y"
{ emptyFile(); }
break;
case 3:
#line 196 "vtkParse.y"
{
      data.ClassName = strdup(yyvsp[0].str);
      }
break;
case 13:
#line 206 "vtkParse.y"
{ preSig("~"); output_function(); }
break;
case 14:
#line 207 "vtkParse.y"
{ preSig("virtual ~"); output_function(); }
break;
case 15:
#line 209 "vtkParse.y"
{
         output_function();
	 }
break;
case 16:
#line 213 "vtkParse.y"
{
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
	 }
break;
case 17:
#line 218 "vtkParse.y"
{
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
	 }
break;
case 18:
#line 224 "vtkParse.y"
{
         preSig("virtual ");
         output_function();
	 }
break;
case 19:
#line 229 "vtkParse.y"
{ postSig(")"); }
break;
case 20:
#line 229 "vtkParse.y"
{ postSig(";"); openSig = 0; }
break;
case 21:
#line 231 "vtkParse.y"
{
      openSig = 1;
      currentFunction->Name = yyvsp[-4].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-4].str); 
    }
break;
case 22:
#line 237 "vtkParse.y"
{ 
      currentFunction->IsOperator = 1; 
      fprintf(stderr,"   Converted operator\n"); 
    }
break;
case 23:
#line 242 "vtkParse.y"
{ 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-3].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-3].str); 
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    }
break;
case 25:
#line 250 "vtkParse.y"
{postSig(" const");}
break;
case 26:
#line 252 "vtkParse.y"
{postSig(" ("); }
break;
case 28:
#line 254 "vtkParse.y"
{postSig("const ");}
break;
case 29:
#line 256 "vtkParse.y"
{postSig("static ");}
break;
case 30:
#line 258 "vtkParse.y"
{postSig(yyvsp[0].str);}
break;
case 31:
#line 258 "vtkParse.y"
{postSig(yyvsp[0].str);}
break;
case 38:
#line 267 "vtkParse.y"
{ currentFunction->NumberOfArguments++;}
break;
case 39:
#line 268 "vtkParse.y"
{ currentFunction->NumberOfArguments++; postSig(", ");}
break;
case 41:
#line 271 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yyvsp[0].integer;}
break;
case 42:
#line 276 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
	yyvsp[0].integer / 10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yyvsp[-1].integer + yyvsp[0].integer % 10000;
      /* fail if array is not const */
      if (((yyvsp[0].integer % 10000)/100) % 10 != 0 
	  && (yyvsp[-1].integer / 1000) != 1 ) {
	currentFunction->ArrayFailure = 1;
      }
    }
break;
case 44:
#line 288 "vtkParse.y"
{ 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 5000;
    }
break;
case 47:
#line 296 "vtkParse.y"
{delSig();}
break;
case 48:
#line 296 "vtkParse.y"
{delSig();}
break;
case 49:
#line 298 "vtkParse.y"
{ yyval.integer = yyvsp[0].integer; }
break;
case 50:
#line 300 "vtkParse.y"
{ yyval.integer = 0; }
break;
case 51:
#line 301 "vtkParse.y"
{ char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); 
                   postSig(temp); }
break;
case 52:
#line 303 "vtkParse.y"
{ yyval.integer = 300 + 10000 * yyvsp[-2].integer; }
break;
case 53:
#line 305 "vtkParse.y"
{ postSig("[]"); yyval.integer = 300; }
break;
case 54:
#line 308 "vtkParse.y"
{yyval.integer = 1000 + yyvsp[0].integer;}
break;
case 55:
#line 309 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 56:
#line 310 "vtkParse.y"
{yyval.integer = 2000 + yyvsp[0].integer;}
break;
case 57:
#line 311 "vtkParse.y"
{yyval.integer = 3000 + yyvsp[0].integer;}
break;
case 58:
#line 313 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 59:
#line 315 "vtkParse.y"
{yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;}
break;
case 60:
#line 324 "vtkParse.y"
{ postSig("&"); yyval.integer = 100;}
break;
case 61:
#line 325 "vtkParse.y"
{ postSig("*"); yyval.integer = 300;}
break;
case 62:
#line 326 "vtkParse.y"
{ yyval.integer = 100 + yyvsp[0].integer;}
break;
case 63:
#line 327 "vtkParse.y"
{ yyval.integer = 400 + yyvsp[0].integer;}
break;
case 64:
#line 329 "vtkParse.y"
{postSig("unsigned ");}
break;
case 65:
#line 330 "vtkParse.y"
{ yyval.integer = 10 + yyvsp[0].integer;}
break;
case 66:
#line 331 "vtkParse.y"
{ yyval.integer = yyvsp[0].integer;}
break;
case 67:
#line 334 "vtkParse.y"
{ postSig("float "); yyval.integer = 1;}
break;
case 68:
#line 335 "vtkParse.y"
{ postSig("void "); yyval.integer = 2;}
break;
case 69:
#line 336 "vtkParse.y"
{ postSig("char "); yyval.integer = 3;}
break;
case 70:
#line 337 "vtkParse.y"
{ postSig("int "); yyval.integer = 4;}
break;
case 71:
#line 338 "vtkParse.y"
{ postSig("short "); yyval.integer = 5;}
break;
case 72:
#line 339 "vtkParse.y"
{ postSig("long "); yyval.integer = 6;}
break;
case 73:
#line 340 "vtkParse.y"
{ postSig("double "); yyval.integer = 7;}
break;
case 74:
#line 341 "vtkParse.y"
{       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 8;}
break;
case 75:
#line 347 "vtkParse.y"
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
case 78:
#line 367 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
break;
case 79:
#line 372 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
break;
case 81:
#line 377 "vtkParse.y"
{in_public = 1; in_protected = 0;}
break;
case 82:
#line 378 "vtkParse.y"
{in_public = 0; in_protected = 0;}
break;
case 83:
#line 379 "vtkParse.y"
{in_public = 0; in_protected = 1;}
break;
case 86:
#line 383 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 87:
#line 384 "vtkParse.y"
{yyval.integer = -1;}
break;
case 88:
#line 384 "vtkParse.y"
{yyval.integer = -1;}
break;
case 89:
#line 388 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 90:
#line 389 "vtkParse.y"
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
case 91:
#line 399 "vtkParse.y"
{postSig("Get");}
break;
case 92:
#line 399 "vtkParse.y"
{postSig(" ();"); invertSig = 1;}
break;
case 93:
#line 401 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   }
break;
case 94:
#line 408 "vtkParse.y"
{preSig("void Set");}
break;
case 95:
#line 409 "vtkParse.y"
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
case 96:
#line 419 "vtkParse.y"
{preSig("char *Get");}
break;
case 97:
#line 420 "vtkParse.y"
{ 
   postSig(" ();");
   sprintf(temps,"Get%s",yyvsp[-1].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 303;
   output_function();
   }
break;
case 98:
#line 429 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 99:
#line 430 "vtkParse.y"
{postSig(");"); openSig = 0;}
break;
case 100:
#line 431 "vtkParse.y"
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
case 101:
#line 441 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 102:
#line 442 "vtkParse.y"
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
case 103:
#line 453 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 104:
#line 454 "vtkParse.y"
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
case 105:
#line 464 "vtkParse.y"
{postSig("*Get");}
break;
case 106:
#line 465 "vtkParse.y"
{postSig(" ();"); invertSig = 1;}
break;
case 107:
#line 466 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   output_function();
   }
break;
case 108:
#line 474 "vtkParse.y"
{preSig("void "); postSig("On ();"); openSig = 0; }
break;
case 109:
#line 476 "vtkParse.y"
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
case 110:
#line 491 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 111:
#line 496 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s);",yyvsp[-4].str,
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
case 112:
#line 521 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 113:
#line 526 "vtkParse.y"
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
case 114:
#line 538 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 115:
#line 543 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s);",
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
case 116:
#line 570 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 117:
#line 575 "vtkParse.y"
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
case 118:
#line 587 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 119:
#line 592 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s);",
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
case 120:
#line 621 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 121:
#line 626 "vtkParse.y"
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
case 122:
#line 638 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 123:
#line 643 "vtkParse.y"
{ 
   char *local = strdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s, %s, %s, %s, %s, %s);",
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
case 124:
#line 676 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 125:
#line 681 "vtkParse.y"
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
case 126:
#line 693 "vtkParse.y"
{
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
break;
case 127:
#line 698 "vtkParse.y"
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
case 128:
#line 711 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 129:
#line 716 "vtkParse.y"
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
case 130:
#line 728 "vtkParse.y"
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
     sprintf(currentFunction->Signature,"void Set%s (float, float);",
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
case 131:
#line 776 "vtkParse.y"
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
     sprintf(currentFunction->Signature,"void Set%s (float, float, float);",
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
case 132:
#line 826 "vtkParse.y"
{ 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName ();");
   sprintf(temps,"GetClassName"); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 1303;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA (const char *name);");
   sprintf(temps,"IsA"); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 1303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 4;
   output_function();
   }
break;
#line 1835 "vtkParse.tab.c"
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
    if (yyssp >= yyss + yystacksize - 1)
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
