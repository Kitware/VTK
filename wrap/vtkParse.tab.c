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
#line 120 "vtkParse.y"
typedef union{
  char *str;
  int   integer;
  } YYSTYPE;
#line 92 "y.tab.c"
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
   56,   10,   57,   10,   58,   10,   10,   10,   10,    1,
    1,   16,   16,   59,   59,   60,   60,   60,   60,   60,
   60,   60,   60,   60,   60,   60,   60,   60,   60,   60,
   60,   60,   60,   60,   60,   61,   62,   63,
};
short yylen[] = {                                         2,
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
    0,    7,    0,    9,    0,    9,    4,    4,    6,    0,
    2,    0,    2,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    3,    3,    3,
};
short yydefred[] = {                                      0,
  144,  146,   71,   67,   64,   68,   69,   70,   65,   66,
  147,  136,  150,  151,   61,   72,  154,  155,    0,  141,
  134,  153,  140,    0,  142,    0,  148,  139,  152,  143,
    0,    0,  145,   63,    0,  135,  137,  138,  149,    0,
    0,    0,    0,    0,    0,  131,   62,  156,  157,  158,
    2,    1,    0,    0,    0,   78,   79,   80,    0,   74,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    7,    8,    0,   14,
    0,    0,    0,   52,    0,    0,    0,   17,    0,   51,
    0,    0,    9,    0,    0,   53,   45,    0,   88,   91,
   93,    0,    0,    0,  102,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   25,
   24,   12,    3,    5,    6,   10,   15,    0,    0,    0,
    0,   22,    0,    0,   56,    0,   13,   16,   20,  133,
   54,    0,    0,    0,    0,    0,    0,    0,    0,  105,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   48,    0,   46,   44,    0,    0,    0,
    0,   26,   19,    0,   59,   60,   77,   86,    0,    0,
    0,   95,   98,  100,    0,    0,  107,  111,  115,  119,
  109,  113,  117,  121,  123,  125,  127,  128,    0,    0,
    0,   21,    0,   27,    0,    0,   41,    0,    0,   34,
    0,    0,   89,   92,   94,    0,    0,    0,  103,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   49,    0,    0,    0,   32,    0,   39,   23,    0,
    0,    0,   96,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  129,   50,    0,
   29,    0,    0,   87,    0,    0,   99,  101,    0,  106,
  108,  112,  116,  120,  110,  114,  118,  122,    0,    0,
   28,    0,   40,   37,   90,    0,  104,    0,    0,   85,
    0,   82,    0,   43,    0,    0,   81,  124,  126,   97,
   84,
};
short yydgoto[] = {                                      31,
   32,   45,   55,   53,   94,   95,   59,   97,   98,   99,
  100,  101,  102,  193,  151,  111,  103,  229,  194,  230,
  231,  260,  149,  303,  282,  311,  186,  220,  104,   33,
  155,   34,   40,   60,  106,  312,  232,  163,  262,  164,
  165,  236,  286,  237,  238,  169,  266,  206,  241,  245,
  242,  246,  243,  247,  244,  248,  249,  250,   35,   36,
   37,   38,   39,
};
short yysindex[] = {                                    -38,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -38,    0,
    0,    0,    0,  -38,    0,  -38,    0,    0,    0,    0,
    0, -230,    0,    0,  -38,    0,    0,    0,    0,  116,
  -91,   15,  -53, -213,  -38,    0,    0,    0,    0,    0,
    0,    0,   20, -139,  -39,    0,    0,    0, -196,    0,
  334,    0,  -76,    0,   69,   -1, -107,    0, -132,   40,
   72,   73,   75,   77,   87,   88,   89,   91,  108,  110,
  112,  115,  125,  126,  130,  133,  134,  135,  136,  138,
  139,  141, -171,   31,  334,  124,    0,    0,  127,    0,
 -171,  122,  144,    0,   39,  143, -171,    0, -171,    0,
  137,   -1,    0, -171,   69,    0,    0, -240,    0,    0,
    0, -240, -240, -240,    0, -240, -240, -240, -240, -240,
 -240, -240, -240, -240, -240, -240, -240, -240, -240,    0,
    0,    0,    0,    0,    0,    0,    0,  -37,  155,  -78,
  -28,    0,   39,   39,    0, -139,    0,    0,    0,    0,
    0,  172, -240, -240, -240,  174,  178,  179, -240,    0,
  203,  205,  207,  208,  210,  211,  212,  213,  215,  216,
  238,  241,  237,    0,   -1,    0,    0,  224,  -31,  -38,
   -1,    0,    0,  -59,    0,    0,    0,    0,  242,  243,
  247,    0,    0,    0,  246,  250,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -240,  -80,
  202,    0,  -38,    0,  181,  245,    0, -240,  256,    0,
    0,   69,    0,    0,    0,   69,   69,   69,    0,   69,
   69,   69,   69,   69,   69,   69,   69,   69,   69,   69,
  267,    0,  -80,  184,  251,    0,  -80,    0,    0,  271,
  270,   69,    0,  275,  276,   69,  280,  301,  302,  305,
  308,  309,  310,  311,  312,  313,  317,    0,    0,  295,
    0,  294,  -59,    0,  315,  344,    0,    0,  348,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   29,   29,
    0,   29,    0,    0,    0,   -1,    0,  345, -177,    0,
  349,    0,  351,    0,  352,  132,    0,    0,    0,    0,
    0,
};
short yyrindex[] = {                                    140,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  273,    0,
    0,    0,    0,  355,    0,  306,    0,    0,    0,    0,
    0,    0,    0,    0,    1,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  400,    0,    0,    0,    0,    0,
    0,    0,  278,    0,    0,    0,    0,    0,    0,    0,
    0,  -32,    0,  -25,    0,  343,    0,   21,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  281,    0,    0,    0,  187,    0,
    0,  -26,    0,    0,   23,    0,    0,    0,    0,    0,
    0,  -23,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  354,    0,    0,
    0,    0,   25,   27,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  314,    0,    0,    0,    0,  273,
  343,    0,    0,  367,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   14,
    0,    0,  273,    0,    0,    0,    0,   59,    0,    0,
   64,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   14,    0,  234,    0,   35,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  286,
    0,   65,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  369,    0,   70,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,
};
short yygindex[] = {                                      0,
  -10,    0,    0,    0,  319,    0,  -15,    0,  350,    0,
    9,  -41,    0,    0,    0, -102,  240,    0,    0,  142,
    0,    0,  188,    0,    0, -253, -191,    0,  -17,   82,
  -13,  375,    0,  262,    0,  111,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  -61,
    0,    0,    0,
};
#define YYTABLESIZE 637
short yytable[] = {                                      27,
  130,   24,  152,   28,  112,   25,   29,   30,   41,  160,
  185,   76,   71,   42,   25,   43,   71,  132,   93,   20,
   21,  109,   23,  140,   46,  114,   44,  224,  252,  191,
  192,   18,   18,   48,   52,  132,   27,  141,   24,   50,
   28,  130,   25,   29,   30,   96,  313,  110,  314,  107,
  112,  116,   26,  185,   47,   49,   20,   47,   72,   23,
   24,  279,   72,   55,   51,   57,   55,   58,   57,  132,
   58,  108,   47,  309,   47,   47,  153,   54,   47,   96,
  154,   62,  221,   61,   19,  308,  140,   22,  226,   26,
   75,  223,  140,  130,  190,   47,   18,  161,  117,   38,
  141,  142,   38,   66,   35,   42,  141,   36,   42,  147,
   83,  118,  119,   83,  120,  157,  121,  158,   56,   57,
   58,   19,  147,  112,   22,  130,  122,  123,  124,  112,
  125,    3,    4,    5,    6,    7,    8,    9,   10,  195,
  196,  115,  105,   15,  105,   16,  105,  126,  105,  127,
  105,  128,  228,   63,  129,  143,   64,    4,    5,    6,
    7,    8,    9,   10,  130,  131,   65,   66,   15,  132,
   68,   69,  133,  134,  135,  136,  105,  137,  138,  225,
  139,  145,  150,  152,  188,  146,  156,   64,    4,    5,
    6,    7,    8,    9,   10,  159,  105,   65,   66,   15,
  184,   68,   69,  315,    3,    4,    5,    6,    7,    8,
    9,   10,  254,  187,   65,  198,   15,  202,   16,   69,
  227,  203,  204,    1,    2,    3,    4,    5,    6,    7,
    8,    9,   10,   11,   12,   13,   14,   15,   71,   16,
   17,  228,   18,  184,  112,  189,  207,   18,  208,   71,
  209,  210,   71,  211,  212,  213,  214,  130,  215,  216,
    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,
   11,   12,   13,   14,   15,  105,   16,   17,  217,   18,
  219,  218,  222,  234,   72,  233,   55,  235,   57,  239,
   58,  308,  140,  240,  253,   72,  259,   55,   72,   57,
   55,   58,   57,  256,   58,  255,  141,  278,  280,  281,
  284,   11,   11,  261,  283,  287,  288,  263,  264,  265,
  290,  267,  268,  269,  270,  271,  272,  273,  274,  275,
  276,  277,    3,    4,    5,    6,    7,    8,    9,   10,
  148,  291,  292,  285,   15,  293,   16,  289,  294,  295,
  296,  297,  298,  301,  302,  305,  299,  162,   31,   31,
  300,  166,  167,  168,  105,  170,  171,  172,  173,  174,
  175,  176,  177,  178,  179,  180,  181,  182,  183,    3,
    4,    5,    6,    7,    8,    9,   10,  306,  307,  318,
  316,  319,  320,   16,  321,  130,  130,  130,  130,  130,
   73,  132,  199,  200,  201,    4,  132,   33,  205,  132,
   30,   30,   47,  144,   47,  258,  113,  197,    0,  317,
    0,    0,    0,    0,  304,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   11,   11,   11,   11,    0,    0,
   11,   11,   11,   11,   11,   11,   11,   11,  251,   93,
   11,   11,   11,   11,   11,   11,   11,  257,   11,   11,
   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,
    0,   31,   31,   31,   31,    0,    0,   31,   31,   31,
   31,   31,   31,   31,   31,    0,    0,   31,   31,   31,
   31,   31,   31,   31,    0,   31,   31,   31,   31,   31,
   31,   31,   31,   31,   31,   31,   31,   31,   31,   31,
   31,   31,   31,   31,   31,   31,   31,    0,  310,  310,
    0,  310,    0,   30,   30,   30,   30,    0,  310,   30,
   30,   30,   30,   30,   30,   30,   30,    0,    0,   30,
   30,   30,   30,   30,   30,   30,    0,   30,   30,   30,
   30,   30,   30,   30,   30,   30,   30,   30,   30,   30,
   30,   30,   30,   30,   30,   30,   30,   30,   30,    0,
    0,   56,   57,   58,   63,    0,    0,   64,    4,    5,
    6,    7,    8,    9,   10,    0,    0,   65,   66,   15,
   67,   68,   69,   70,    0,   71,   72,   73,   74,   75,
   76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
   86,   87,   88,   89,   90,   91,   92,
};
short yycheck[] = {                                      38,
    0,   40,   40,   42,   66,   44,   45,   46,   19,  112,
   91,   44,   38,   24,   40,   26,   42,   41,  126,   58,
   59,   63,   61,  264,   35,   67,  257,   59,  220,   58,
   59,   58,   59,  125,   45,   59,   38,  278,   40,   93,
   42,   41,   44,   45,   46,   61,  300,   65,  302,  126,
  112,   69,   91,   91,   41,   41,   58,   44,   38,   61,
   40,  253,   42,   41,  278,   41,   44,   41,   44,   93,
   44,   63,   59,   45,   61,   41,   38,   58,   44,   95,
   42,  278,  185,  123,  123,  263,  264,  126,  191,   91,
  123,  123,  264,   93,  123,   61,  123,  115,   59,   41,
  278,   93,   44,  275,   41,   41,  278,   44,   44,  101,
   41,   40,   40,   44,   40,  107,   40,  109,  258,  259,
  260,  123,  114,  185,  126,  125,   40,   40,   40,  191,
   40,  264,  265,  266,  267,  268,  269,  270,  271,  153,
  154,  274,   61,  276,   63,  278,   65,   40,   67,   40,
   69,   40,  194,  261,   40,  125,  264,  265,  266,  267,
  268,  269,  270,  271,   40,   40,  274,  275,  276,   40,
  278,  279,   40,   40,   40,   40,   95,   40,   40,  190,
   40,   58,   61,   40,  263,   59,   44,  264,  265,  266,
  267,  268,  269,  270,  271,   59,  115,  274,  275,  276,
  281,  278,  279,  306,  264,  265,  266,  267,  268,  269,
  270,  271,  223,   59,  274,   44,  276,   44,  278,  279,
  280,   44,   44,  262,  263,  264,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,  276,  264,  278,
  279,  283,  281,  281,  306,  274,   44,  274,   44,  275,
   44,   44,  278,   44,   44,   44,   44,  257,   44,   44,
  262,  263,  264,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,  276,  194,  278,  279,   41,  281,
   44,   41,   59,   41,  264,   44,  264,   41,  264,   44,
  264,  263,  264,   44,   93,  275,   41,  275,  278,  275,
  278,  275,  278,   59,  278,  125,  278,   41,  125,   59,
   41,  125,  126,  232,   44,   41,   41,  236,  237,  238,
   41,  240,  241,  242,  243,  244,  245,  246,  247,  248,
  249,  250,  264,  265,  266,  267,  268,  269,  270,  271,
  101,   41,   41,  262,  276,   41,  278,  266,   41,   41,
   41,   41,   41,   59,   61,   41,   44,  118,  125,  126,
   44,  122,  123,  124,  283,  126,  127,  128,  129,  130,
  131,  132,  133,  134,  135,  136,  137,  138,  139,  264,
  265,  266,  267,  268,  269,  270,  271,   44,   41,   41,
   46,   41,   41,  278,  263,   41,  257,  125,   93,    0,
  123,   59,  163,  164,  165,  125,   93,   41,  169,   41,
  125,  126,   59,   95,   40,  228,   67,  156,   -1,  309,
   -1,   -1,   -1,   -1,  283,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  258,  259,  260,  261,   -1,   -1,
  264,  265,  266,  267,  268,  269,  270,  271,  219,  126,
  274,  275,  276,  277,  278,  279,  280,  228,  282,  283,
  284,  285,  286,  287,  288,  289,  290,  291,  292,  293,
  294,  295,  296,  297,  298,  299,  300,  301,  302,  303,
   -1,  258,  259,  260,  261,   -1,   -1,  264,  265,  266,
  267,  268,  269,  270,  271,   -1,   -1,  274,  275,  276,
  277,  278,  279,  280,   -1,  282,  283,  284,  285,  286,
  287,  288,  289,  290,  291,  292,  293,  294,  295,  296,
  297,  298,  299,  300,  301,  302,  303,   -1,  299,  300,
   -1,  302,   -1,  258,  259,  260,  261,   -1,  309,  264,
  265,  266,  267,  268,  269,  270,  271,   -1,   -1,  274,
  275,  276,  277,  278,  279,  280,   -1,  282,  283,  284,
  285,  286,  287,  288,  289,  290,  291,  292,  293,  294,
  295,  296,  297,  298,  299,  300,  301,  302,  303,   -1,
   -1,  258,  259,  260,  261,   -1,   -1,  264,  265,  266,
  267,  268,  269,  270,  271,   -1,   -1,  274,  275,  276,
  277,  278,  279,  280,   -1,  282,  283,  284,  285,  286,
  287,  288,  289,  290,  291,  292,  293,  294,  295,  296,
  297,  298,  299,  300,  301,  302,  303,
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
#line 847 "vtkParse.y"
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
  vtkParseOutput(stdout,&data);
  return 0;
}
 


#line 798 "y.tab.c"
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
#line 182 "vtkParse.y"
{
      data.ClassName = strdup(yyvsp[0].str);
      }
break;
case 12:
#line 192 "vtkParse.y"
{ preSig("~"); output_function(); }
break;
case 13:
#line 193 "vtkParse.y"
{ preSig("virtual ~"); output_function(); }
break;
case 14:
#line 195 "vtkParse.y"
{
         output_function();
	 }
break;
case 15:
#line 199 "vtkParse.y"
{
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
	 }
break;
case 16:
#line 204 "vtkParse.y"
{
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
	 }
break;
case 17:
#line 210 "vtkParse.y"
{
         preSig("virtual ");
         output_function();
	 }
break;
case 18:
#line 215 "vtkParse.y"
{ postSig(");"); openSig = 0; }
break;
case 19:
#line 216 "vtkParse.y"
{
      openSig = 1;
      currentFunction->Name = yyvsp[-2].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-2].str); 
    }
break;
case 20:
#line 222 "vtkParse.y"
{ 
      currentFunction->IsOperator = 1; 
      fprintf(stderr,"   Converted operator\n"); 
    }
break;
case 21:
#line 227 "vtkParse.y"
{ 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-3].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-3].str); 
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    }
break;
case 22:
#line 235 "vtkParse.y"
{postSig(" ("); }
break;
case 24:
#line 238 "vtkParse.y"
{postSig(yyvsp[0].str);}
break;
case 25:
#line 238 "vtkParse.y"
{postSig(yyvsp[0].str);}
break;
case 35:
#line 250 "vtkParse.y"
{ currentFunction->NumberOfArguments++;}
break;
case 36:
#line 251 "vtkParse.y"
{ currentFunction->NumberOfArguments++; postSig(", ");}
break;
case 38:
#line 254 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yyvsp[0].integer;}
break;
case 39:
#line 259 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
	yyvsp[0].integer / 10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
	yyvsp[-1].integer + yyvsp[0].integer % 10000;
      /* fail if array is not const */
      currentFunction->ArrayFailure = ( ((yyvsp[0].integer % 10000)/100) % 10 != 0 
					&& ((yyvsp[-1].integer / 1000) & 1) == 0 );
     }
break;
case 41:
#line 269 "vtkParse.y"
{ 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 5000;
    }
break;
case 46:
#line 279 "vtkParse.y"
{ yyval.integer = yyvsp[0].integer; }
break;
case 47:
#line 281 "vtkParse.y"
{ yyval.integer = 0; }
break;
case 48:
#line 282 "vtkParse.y"
{ char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); 
                   postSig(temp); }
break;
case 49:
#line 284 "vtkParse.y"
{ yyval.integer = 300 + 10000 * yyvsp[-2].integer; }
break;
case 50:
#line 286 "vtkParse.y"
{ postSig("[]"); yyval.integer = 300; }
break;
case 51:
#line 289 "vtkParse.y"
{yyval.integer = 1000 + yyvsp[0].integer;}
break;
case 52:
#line 290 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 53:
#line 291 "vtkParse.y"
{yyval.integer = 2000 + yyvsp[0].integer;}
break;
case 54:
#line 292 "vtkParse.y"
{yyval.integer = 3000 + yyvsp[0].integer;}
break;
case 55:
#line 294 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 56:
#line 296 "vtkParse.y"
{yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;}
break;
case 57:
#line 305 "vtkParse.y"
{ postSig("&"); yyval.integer = 100;}
break;
case 58:
#line 306 "vtkParse.y"
{ postSig("*"); yyval.integer = 300;}
break;
case 59:
#line 307 "vtkParse.y"
{ yyval.integer = 100 + yyvsp[0].integer;}
break;
case 60:
#line 308 "vtkParse.y"
{ yyval.integer = 400 + yyvsp[0].integer;}
break;
case 61:
#line 310 "vtkParse.y"
{postSig("unsigned ");}
break;
case 62:
#line 311 "vtkParse.y"
{ yyval.integer = 10 + yyvsp[0].integer;}
break;
case 63:
#line 312 "vtkParse.y"
{ yyval.integer = yyvsp[0].integer;}
break;
case 64:
#line 315 "vtkParse.y"
{ postSig("float "); yyval.integer = 1;}
break;
case 65:
#line 316 "vtkParse.y"
{ postSig("void "); yyval.integer = 2;}
break;
case 66:
#line 317 "vtkParse.y"
{ postSig("char "); yyval.integer = 3;}
break;
case 67:
#line 318 "vtkParse.y"
{ postSig("int "); yyval.integer = 4;}
break;
case 68:
#line 319 "vtkParse.y"
{ postSig("short "); yyval.integer = 5;}
break;
case 69:
#line 320 "vtkParse.y"
{ postSig("long "); yyval.integer = 6;}
break;
case 70:
#line 321 "vtkParse.y"
{ postSig("double "); yyval.integer = 7;}
break;
case 71:
#line 322 "vtkParse.y"
{       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 8;}
break;
case 72:
#line 328 "vtkParse.y"
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
#line 348 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
break;
case 76:
#line 353 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = strdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
break;
case 78:
#line 358 "vtkParse.y"
{in_public = 1;}
break;
case 79:
#line 358 "vtkParse.y"
{in_public = 0;}
break;
case 80:
#line 359 "vtkParse.y"
{in_public = 0;}
break;
case 83:
#line 363 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 84:
#line 364 "vtkParse.y"
{yyval.integer = -1;}
break;
case 85:
#line 364 "vtkParse.y"
{yyval.integer = -1;}
break;
case 86:
#line 368 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 87:
#line 369 "vtkParse.y"
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
#line 379 "vtkParse.y"
{postSig("Get");}
break;
case 89:
#line 379 "vtkParse.y"
{postSig(" ();"); invertSig = 1;}
break;
case 90:
#line 381 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   }
break;
case 91:
#line 388 "vtkParse.y"
{preSig("void Set");}
break;
case 92:
#line 389 "vtkParse.y"
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
#line 399 "vtkParse.y"
{preSig("char *Get");}
break;
case 94:
#line 400 "vtkParse.y"
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
#line 409 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 96:
#line 410 "vtkParse.y"
{postSig(");"); openSig = 0;}
break;
case 97:
#line 411 "vtkParse.y"
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
#line 421 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 99:
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
case 100:
#line 433 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 101:
#line 434 "vtkParse.y"
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
#line 444 "vtkParse.y"
{postSig("*Get");}
break;
case 103:
#line 445 "vtkParse.y"
{postSig(" ();"); invertSig = 1;}
break;
case 104:
#line 446 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = strdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   output_function();
   }
break;
case 105:
#line 454 "vtkParse.y"
{preSig("void "); postSig("On ();"); openSig = 0; }
break;
case 106:
#line 456 "vtkParse.y"
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
#line 471 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 108:
#line 476 "vtkParse.y"
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
#line 501 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 110:
#line 506 "vtkParse.y"
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
#line 518 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 112:
#line 523 "vtkParse.y"
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
#line 550 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 114:
#line 555 "vtkParse.y"
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
#line 567 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 116:
#line 572 "vtkParse.y"
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
#line 601 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 118:
#line 606 "vtkParse.y"
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
#line 618 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 120:
#line 623 "vtkParse.y"
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
#line 656 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 122:
#line 661 "vtkParse.y"
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
#line 673 "vtkParse.y"
{
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
break;
case 124:
#line 678 "vtkParse.y"
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
#line 691 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 126:
#line 696 "vtkParse.y"
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
#line 708 "vtkParse.y"
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
#line 756 "vtkParse.y"
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
case 129:
#line 806 "vtkParse.y"
{ 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName();");
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
#line 1787 "y.tab.c"
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
