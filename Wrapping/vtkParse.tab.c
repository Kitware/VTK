#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING() (yyerrflag!=0)
static int yygrowstack();
#define YYPREFIX "yy"
#line 44 "vtkParse.y"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) fprintf(stderr,"%s\n",a)
#define yywrap() 1

void output_function();
int yyparse();

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
  int sigAllocatedLength;
  
#define YYMAXDEPTH 1000

  void checkSigSize(char *arg)
    {
    if (strlen(currentFunction->Signature) + strlen(arg) + 3 > 
        (unsigned int)sigAllocatedLength)
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
#line 139 "vtkParse.y"
typedef union{
  char *str;
  int   integer;
  } YYSTYPE;
#line 116 "vtkParse.tab.c"
#define YYERRCODE 256
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
const short yylhs[] = {                                        -1,
    0,    4,    2,    5,    5,    6,    6,    6,    6,    6,
    6,    9,    9,    9,    9,    9,    9,   15,   17,   11,
   11,   11,   14,   14,   21,   13,   22,   23,   19,   19,
   16,   16,   16,   16,   20,   20,   24,   26,   24,   25,
   29,   25,   25,   28,   28,    8,    8,   27,   31,   32,
   31,   31,   12,   12,   12,   12,   33,   33,   35,   35,
   35,   35,   37,   34,   34,   36,   36,   36,   36,   36,
   36,   36,   36,   36,    3,    3,   38,   39,   38,    7,
    7,    7,   30,   30,   40,   40,   40,   41,   10,   42,
   43,   10,   44,   10,   45,   10,   46,   47,   10,   48,
   10,   49,   10,   50,   51,   10,   52,   10,   53,   10,
   54,   10,   55,   10,   56,   10,   57,   10,   58,   10,
   59,   10,   60,   10,   61,   10,   62,   10,   10,   10,
   10,    1,    1,   18,   18,   63,   63,   64,   64,   64,
   64,   64,   64,   64,   64,   64,   64,   64,   64,   64,
   64,   64,   64,   64,   64,   64,   64,   65,   66,   67,
};
const short yylen[] = {                                         2,
    3,    0,    7,    1,    2,    2,    1,    1,    2,    2,
    1,    2,    3,    1,    2,    3,    2,    0,    0,    5,
    3,    4,    0,    1,    0,    5,    1,    1,    1,    1,
    1,    4,    3,    3,    0,    1,    1,    0,    4,    1,
    0,    4,    1,    0,    2,    3,    2,    2,    0,    0,
    3,    4,    2,    1,    2,    3,    1,    2,    1,    1,
    2,    2,    0,    3,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    0,    2,    2,    0,    5,    1,
    1,    1,    2,    1,    1,    3,    1,    0,    7,    0,
    0,    8,    0,    5,    0,    5,    0,    0,   10,    0,
    7,    0,    7,    0,    0,    8,    0,    7,    0,    7,
    0,    7,    0,    7,    0,    7,    0,    7,    0,    7,
    0,    7,    0,    7,    0,    9,    0,    9,    4,    4,
    6,    0,    2,    0,    2,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    3,    3,    3,
};
const short yydefred[] = {                                      0,
  146,  148,   73,   69,   66,   70,   71,   72,   67,   68,
  149,  138,  152,  153,   63,   74,  156,  157,    0,  143,
  136,  155,  142,    0,  144,    0,  150,  141,  154,  145,
    0,    0,  147,   65,    0,  137,  139,  140,  151,    0,
    0,    0,    0,    0,    0,  133,   64,  158,  159,  160,
    2,    1,    0,    0,    0,   80,   81,   82,    0,   76,
    0,    0,    0,    0,   27,    0,    0,    0,   28,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    7,    8,    0,   14,
    0,    0,    0,    0,    0,   54,    0,    0,    0,   17,
    0,    0,    0,    9,    0,   47,    0,   90,   93,   95,
    0,    0,    0,  104,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   30,   29,
   12,    3,    5,    6,   10,   15,    0,    0,    0,    0,
   25,   53,    0,   55,    0,    0,   58,    0,   13,   16,
   21,  135,    0,    0,    0,    0,    0,    0,    0,    0,
  107,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   50,    0,   48,   46,    0,   24,
   19,    0,   56,   61,   62,   79,   88,    0,    0,    0,
   97,  100,  102,    0,    0,  109,  113,  117,  121,  111,
  115,  119,  123,  125,  127,  129,  130,    0,    0,    0,
   22,    0,   43,    0,    0,   36,    0,    0,   91,   94,
   96,    0,    0,    0,  105,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   51,    0,    0,
    0,   31,   20,    0,   41,   26,    0,    0,    0,   98,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  131,   52,    0,    0,    0,    0,
   89,    0,    0,  101,  103,    0,  108,  110,  114,  118,
  122,  112,  116,  120,  124,    0,    0,    0,   34,    0,
   42,   39,   92,    0,  106,    0,    0,   87,    0,   84,
    0,   32,   45,    0,    0,   83,  126,  128,   99,   86,
};
const short yydgoto[] = {                                      31,
   32,   45,   55,   53,   94,   95,   59,   97,   98,   99,
  100,  101,  102,  191,  150,  253,  222,  112,  103,  225,
  192,  104,  105,  226,  227,  257,  148,  301,  279,  309,
  187,  219,  106,   33,  157,   34,   40,   60,  108,  310,
  228,  164,  259,  165,  166,  232,  283,  233,  234,  170,
  263,  205,  237,  241,  238,  242,  239,  243,  240,  244,
  245,  246,   35,   36,   37,   38,   39,
};
const short yysindex[] = {                                    -38,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -38,    0,
    0,    0,    0,  -38,    0,  -38,    0,    0,    0,    0,
    0, -246,    0,    0,  -38,    0,    0,    0,    0,  127,
 -109,   -7,  -35, -216,  -38,    0,    0,    0,    0,    0,
    0,    0,   34, -139,  -53,    0,    0,    0, -178,    0,
  348,    0,  -58,    0,    0,   -1,  -96,    0,    0,   49,
   70,   83,   84,   87,   88,   98,   99,  101,  107,  116,
  121,  124,  126,  136,  137,  141,  144,  145,  146,  147,
  148,  149, -226,   67,  348,  109,    0,    0,  131,    0,
 -226,  132,  155,   42, -116,    0,   13,  153, -226,    0,
 -226,  139,   -1,    0, -226,    0, -166,    0,    0,    0,
 -166, -166, -166,    0, -166, -166, -166, -166, -166, -166,
 -166, -166, -166, -166, -166, -166, -166, -166,    0,    0,
    0,    0,    0,    0,    0,    0,  -37,  140,  -60,  -59,
    0,    0,   42,    0,   13,   13,    0, -139,    0,    0,
    0,    0,  161, -166, -166, -166,  170,  175,  178, -166,
    0,  179,  202,  204,  207,  208,  210,  213,  216,  232,
  235,  240,  241,  239,    0,   -1,    0,    0,  225,    0,
    0, -134,    0,    0,    0,    0,    0,  242,  253,  254,
    0,    0,    0,  249,  258,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -166,  -79,  211,
    0,  -40,    0, -166,  256,    0,    0,   42,    0,    0,
    0,   42,   42,   42,    0,   42,   42,   42,   42,   42,
   42,   42,   42,   42,   42,   42,  273,    0,  -79,  -38,
   -1,    0,    0,  -79,    0,    0,  271,  275,   42,    0,
  276,  278,   42,  280,  281,  282,  284,  285,  286,  290,
  305,  306,  299,  304,    0,    0,  224,  291,  293, -134,
    0,  310,  308,    0,    0,  315,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   27,   27,  298,    0,   27,
    0,    0,    0,   -1,    0,  312, -177,    0,  319,    0,
  320,    0,    0,  321,  100,    0,    0,    0,    0,    0,
};
const short yyrindex[] = {                                    108,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  243,    0,
    0,    0,    0,  323,    0,  274,    0,    0,    0,    0,
    0,    0,    0,    0,    1,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  366,    0,    0,    0,    0,    0,
    0,    0,  247,    0,    0,    0,    0,    0,    0,    0,
    0,  -34,    0,  -25,    0,  313,    0,   21,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  246,    0,    0,    0,  219,    0,
    0,  -32,    0,    0,    0,    0,  -19,    0,    0,    0,
    0,    0,  -13,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  340,    0,    0,  -26,
    0,    0,    0,    0,   23,   25,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  307,    0,    0,    0,    0,
    0,  360,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   55,    0,
    0,    0,    0,   30,    0,    0,   61,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   55,  243,
  313,    0,    0,   32,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   63,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  302,    0,    0,
    0,    0,    0,  361,    0,   65,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
const short yygindex[] = {                                      0,
    5,    0,    0,    0,  309,    0,  -47,    0,  336,    0,
    2,   12,    0,    0,    0,    0,    0, -108,  252,    0,
    0,  301,    0,  128,    0,    0,  183,    0,    0, -182,
 -172,    0,  -69,   96,  -74,  369,    0,  255,    0,  103,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -57,    0,    0,    0,
};
#define YYTABLESIZE 651
const short yytable[] = {                                      27,
  132,   24,  151,   28,  162,   25,   29,   30,  113,   78,
   44,  186,   73,   96,   30,   48,   73,  251,  252,   20,
   21,   57,   23,   41,   57,   18,   18,  134,   42,   93,
   43,   23,   23,   49,  152,  154,   27,  139,   24,   46,
   28,  132,   25,   29,   30,  134,  248,   96,   66,   52,
  155,  140,   26,  186,  156,  113,   20,   50,   74,   23,
   29,   51,   74,   59,  110,   60,   59,  109,   60,   61,
   40,  307,   49,   40,  111,   49,  276,  220,  115,  134,
  194,  195,  250,  193,   19,  306,  139,   22,   77,   26,
   18,   54,   49,  132,  141,   49,   23,  139,   49,   62,
  140,   37,  146,   44,   38,   85,   44,  116,   85,  117,
  159,  140,  160,   49,  311,   49,  146,  313,   56,   57,
   58,   19,  118,  119,   22,  132,  120,  121,  113,    3,
    4,    5,    6,    7,    8,    9,   10,  122,  123,   65,
  124,   15,  278,   16,   69,  223,  125,    3,    4,    5,
    6,    7,    8,    9,   10,  126,  107,   65,  107,   15,
  127,   16,  107,  128,   63,  129,  144,   64,    4,    5,
    6,    7,    8,    9,   10,  130,  131,   65,   66,   15,
  132,   68,   69,  133,  134,  135,  136,  137,  138,  145,
  107,  142,  149,  113,  151,  314,  158,  161,  188,  107,
  107,  185,  189,  224,  197,   64,    4,    5,    6,    7,
    8,    9,   10,  201,  190,   65,   66,   15,  202,   68,
   69,  203,  206,    1,    2,    3,    4,    5,    6,    7,
    8,    9,   10,   11,   12,   13,   14,   15,   73,   16,
   17,   18,   18,  185,   57,  207,  113,  208,  107,   73,
  209,  210,   73,  211,  277,   57,  212,  132,   57,  213,
    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,
   11,   12,   13,   14,   15,  214,   16,   17,  215,   18,
  216,  217,  218,  221,   74,  229,   59,  107,   60,  306,
  139,  224,  235,  230,  231,   74,  256,   59,   74,   60,
   59,  236,   60,  249,  140,    3,    4,    5,    6,    7,
    8,    9,   10,  275,  280,  281,  284,   15,  285,   16,
  287,  288,  289,  258,  290,  291,  292,  260,  261,  262,
  293,  264,  265,  266,  267,  268,  269,  270,  271,  272,
  273,  274,  296,   11,   11,  294,  295,  297,  298,  299,
  303,  304,  147,  300,  282,  305,  312,  315,  286,  317,
  318,  319,  320,  132,  132,  132,  132,  132,  163,   75,
    4,  134,  167,  168,  169,  107,  171,  172,  173,  174,
  175,  176,  177,  178,  179,  180,  181,  182,  183,  184,
    3,    4,    5,    6,    7,    8,    9,   10,   49,  134,
   35,  134,  114,  143,   16,  153,  255,  302,   47,  316,
    0,    0,  196,    0,    0,  198,  199,  200,    0,    0,
    0,  204,    0,    0,    0,    0,   33,   33,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  247,
    0,    0,    0,   93,    0,  254,   11,   11,   11,   11,
    0,    0,   11,   11,   11,   11,   11,   11,   11,   11,
    0,    0,   11,   11,   11,   11,   11,   11,   11,    0,
   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,   11,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  308,  308,    0,
    0,  308,    0,    0,    0,    0,    0,    0,  308,   33,
   33,   33,   33,    0,    0,   33,   33,   33,   33,   33,
   33,   33,   33,    0,    0,   33,   33,   33,   33,   33,
   33,   33,    0,   33,   33,   33,   33,   33,   33,   33,
   33,   33,   33,   33,   33,   33,   33,   33,   33,   33,
   33,   33,   33,   33,   33,   56,   57,   58,   63,    0,
    0,   64,    4,    5,    6,    7,    8,    9,   10,    0,
    0,   65,   66,   15,   67,   68,   69,   70,    0,   71,
   72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
   82,   83,   84,   85,   86,   87,   88,   89,   90,   91,
   92,
};
const short yycheck[] = {                                      38,
    0,   40,   40,   42,  113,   44,   45,   46,   66,   44,
  257,   91,   38,   61,   40,  125,   42,   58,   59,   58,
   59,   41,   61,   19,   44,   58,   59,   41,   24,  126,
   26,   58,   59,   41,  104,  105,   38,  264,   40,   35,
   42,   41,   44,   45,   46,   59,  219,   95,  275,   45,
   38,  278,   91,   91,   42,  113,   58,   93,   38,   61,
   40,  278,   42,   41,   63,   41,   44,  126,   44,  123,
   41,   45,   41,   44,   63,   44,  249,  186,   67,   93,
  155,  156,  123,  153,  123,  263,  264,  126,  123,   91,
  123,   58,   61,   93,   93,   41,  123,  264,   44,  278,
  278,   41,  101,   41,   44,   41,   44,   59,   44,   40,
  109,  278,  111,   59,  297,   61,  115,  300,  258,  259,
  260,  123,   40,   40,  126,  125,   40,   40,  186,  264,
  265,  266,  267,  268,  269,  270,  271,   40,   40,  274,
   40,  276,  251,  278,  279,  280,   40,  264,  265,  266,
  267,  268,  269,  270,  271,   40,   61,  274,   63,  276,
   40,  278,   67,   40,  261,   40,   58,  264,  265,  266,
  267,  268,  269,  270,  271,   40,   40,  274,  275,  276,
   40,  278,  279,   40,   40,   40,   40,   40,   40,   59,
   95,  125,   61,  251,   40,  304,   44,   59,   59,  104,
  105,  281,  263,  192,   44,  264,  265,  266,  267,  268,
  269,  270,  271,   44,  274,  274,  275,  276,   44,  278,
  279,   44,   44,  262,  263,  264,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,  276,  264,  278,
  279,  274,  281,  281,  264,   44,  304,   44,  153,  275,
   44,   44,  278,   44,  250,  275,   44,  257,  278,   44,
  262,  263,  264,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,  276,   44,  278,  279,   44,  281,
   41,   41,   44,   59,  264,   44,  264,  192,  264,  263,
  264,  280,   44,   41,   41,  275,   41,  275,  278,  275,
  278,   44,  278,   93,  278,  264,  265,  266,  267,  268,
  269,  270,  271,   41,   44,   41,   41,  276,   41,  278,
   41,   41,   41,  228,   41,   41,   41,  232,  233,  234,
   41,  236,  237,  238,  239,  240,  241,  242,  243,  244,
  245,  246,   44,  125,  126,   41,   41,   44,  125,   59,
   41,   44,  101,   61,  259,   41,   59,   46,  263,   41,
   41,   41,  263,   41,  257,    0,   93,  125,  117,  123,
  125,   59,  121,  122,  123,  280,  125,  126,  127,  128,
  129,  130,  131,  132,  133,  134,  135,  136,  137,  138,
  264,  265,  266,  267,  268,  269,  270,  271,   59,   93,
   41,   41,   67,   95,  278,  105,  224,  280,   40,  307,
   -1,   -1,  158,   -1,   -1,  164,  165,  166,   -1,   -1,
   -1,  170,   -1,   -1,   -1,   -1,  125,  126,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  218,
   -1,   -1,   -1,  126,   -1,  224,  258,  259,  260,  261,
   -1,   -1,  264,  265,  266,  267,  268,  269,  270,  271,
   -1,   -1,  274,  275,  276,  277,  278,  279,  280,   -1,
  282,  283,  284,  285,  286,  287,  288,  289,  290,  291,
  292,  293,  294,  295,  296,  297,  298,  299,  300,  301,
  302,  303,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  296,  297,   -1,
   -1,  300,   -1,   -1,   -1,   -1,   -1,   -1,  307,  258,
  259,  260,  261,   -1,   -1,  264,  265,  266,  267,  268,
  269,  270,  271,   -1,   -1,  274,  275,  276,  277,  278,
  279,  280,   -1,  282,  283,  284,  285,  286,  287,  288,
  289,  290,  291,  292,  293,  294,  295,  296,  297,  298,
  299,  300,  301,  302,  303,  258,  259,  260,  261,   -1,
   -1,  264,  265,  266,  267,  268,  269,  270,  271,   -1,
   -1,  274,  275,  276,  277,  278,  279,  280,   -1,  282,
  283,  284,  285,  286,  287,  288,  289,  290,  291,  292,
  293,  294,  295,  296,  297,  298,  299,  300,  301,  302,
  303,
};
#define YYFINAL 31
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 303
#if YYDEBUG
const char * const yyname[] = {
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
const char * const yyrule[] = {
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
#if YYDEBUG
#include <stdio.h>
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
#line 890 "vtkParse.y"
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
  if (!fhint)
    {
    return;
    }
  
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
  int i;

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

  /* reject multi-dimensional arrays from wrappers */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if ((currentFunction->ArgTypes[i]%1000)/100 == 6 ||
        (currentFunction->ArgTypes[i]%1000)/100 == 9)
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
    /* The following prevents warning */
    char *str = 0;
    str = (char *)yy_flex_realloc(0, strlen("SYNTAX ERROR")+1);
    sprintf(str, "SYNTAX ERROR");
    
    fprintf(stdout,
            "*** %s found in parsing the header file %s before line %d***\n", 
            str, argv[1], yylineno);
    free(str);
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
 


#line 858 "vtkParse.tab.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
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
    newss = yyss ? (short *)realloc(yyss, newsize * sizeof *newss) :
      (short *)malloc(newsize * sizeof *newss);
    if (newss == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    newvs = yyvs ? (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs) :
      (YYSTYPE *)malloc(newsize * sizeof *newvs);
    if (newvs == NULL)
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

#ifndef YYPARSE_PARAM
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG void
#define YYPARSE_PARAM_DECL
#else        /* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif        /* ANSI-C/C++ */
#else        /* YYPARSE_PARAM */
#ifndef YYPARSE_PARAM_TYPE
#define YYPARSE_PARAM_TYPE void *
#endif
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG YYPARSE_PARAM_TYPE YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else        /* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL YYPARSE_PARAM_TYPE YYPARSE_PARAM;
#endif        /* ANSI-C/C++ */
#endif        /* ! YYPARSE_PARAM */

int
yyparse (YYPARSE_PARAM_ARG)
    YYPARSE_PARAM_DECL
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register const char *yys;

    if ((yys = getenv("YYDEBUG")))
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
    if ((yyn = yydefred[yystate])) goto yyreduce;
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
    yyerror("syntax error");
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
#line 201 "vtkParse.y"
{
      data.ClassName = vtkstrdup(yyvsp[0].str);
      }
break;
case 12:
#line 211 "vtkParse.y"
{ preSig("~"); output_function(); }
break;
case 13:
#line 212 "vtkParse.y"
{ preSig("virtual ~"); output_function(); }
break;
case 14:
#line 214 "vtkParse.y"
{
         output_function();
         }
break;
case 15:
#line 218 "vtkParse.y"
{
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
break;
case 16:
#line 223 "vtkParse.y"
{
         preSig("virtual ");
         currentFunction->ReturnType = yyvsp[-1].integer;
         output_function();
         }
break;
case 17:
#line 229 "vtkParse.y"
{
         preSig("virtual ");
         output_function();
         }
break;
case 18:
#line 234 "vtkParse.y"
{ postSig(")"); }
break;
case 19:
#line 234 "vtkParse.y"
{ postSig(";"); openSig = 0; }
break;
case 20:
#line 236 "vtkParse.y"
{
      openSig = 1;
      currentFunction->Name = yyvsp[-4].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-4].str); 
    }
break;
case 21:
#line 242 "vtkParse.y"
{ 
      currentFunction->IsOperator = 1; 
      fprintf(stderr,"   Converted operator\n"); 
    }
break;
case 22:
#line 247 "vtkParse.y"
{ 
      postSig(") = 0;"); 
      currentFunction->Name = yyvsp[-3].str; 
      fprintf(stderr,"   Parsed func %s\n",yyvsp[-3].str); 
      currentFunction->IsPureVirtual = 1; 
      data.IsAbstract = 1;
    }
break;
case 24:
#line 255 "vtkParse.y"
{postSig(" const");}
break;
case 25:
#line 257 "vtkParse.y"
{postSig(" ("); }
break;
case 27:
#line 259 "vtkParse.y"
{postSig("const ");}
break;
case 28:
#line 261 "vtkParse.y"
{postSig("static ");}
break;
case 29:
#line 263 "vtkParse.y"
{postSig(yyvsp[0].str);}
break;
case 30:
#line 263 "vtkParse.y"
{postSig(yyvsp[0].str);}
break;
case 37:
#line 272 "vtkParse.y"
{ currentFunction->NumberOfArguments++;}
break;
case 38:
#line 273 "vtkParse.y"
{ currentFunction->NumberOfArguments++; postSig(", ");}
break;
case 40:
#line 276 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer;}
break;
case 41:
#line 281 "vtkParse.y"
{
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 
        yyvsp[0].integer / 10000; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 
        yyvsp[-1].integer + yyvsp[0].integer % 10000;
    }
break;
case 43:
#line 288 "vtkParse.y"
{ 
      postSig("void (*func)(void *) ");
      currentFunction->ArgCounts[currentFunction->NumberOfArguments] = 0; 
      currentFunction->ArgTypes[currentFunction->NumberOfArguments] = 5000;
    }
break;
case 46:
#line 296 "vtkParse.y"
{delSig();}
break;
case 47:
#line 296 "vtkParse.y"
{delSig();}
break;
case 48:
#line 298 "vtkParse.y"
{ yyval.integer = yyvsp[0].integer; }
break;
case 49:
#line 306 "vtkParse.y"
{ yyval.integer = 0; }
break;
case 50:
#line 307 "vtkParse.y"
{ char temp[100]; sprintf(temp,"[%i]",yyvsp[0].integer); 
                   postSig(temp); }
break;
case 51:
#line 309 "vtkParse.y"
{ yyval.integer = 300 + 10000 * yyvsp[-2].integer + yyvsp[0].integer % 1000; }
break;
case 52:
#line 311 "vtkParse.y"
{ postSig("[]"); yyval.integer = 300 + yyvsp[0].integer % 1000; }
break;
case 53:
#line 313 "vtkParse.y"
{yyval.integer = 1000 + yyvsp[0].integer;}
break;
case 54:
#line 314 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 55:
#line 315 "vtkParse.y"
{yyval.integer = 2000 + yyvsp[0].integer;}
break;
case 56:
#line 316 "vtkParse.y"
{yyval.integer = 3000 + yyvsp[0].integer;}
break;
case 57:
#line 318 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 58:
#line 320 "vtkParse.y"
{yyval.integer = yyvsp[-1].integer + yyvsp[0].integer;}
break;
case 59:
#line 329 "vtkParse.y"
{ postSig("&"); yyval.integer = 100;}
break;
case 60:
#line 330 "vtkParse.y"
{ postSig("*"); yyval.integer = 300;}
break;
case 61:
#line 331 "vtkParse.y"
{ yyval.integer = 100 + yyvsp[0].integer;}
break;
case 62:
#line 332 "vtkParse.y"
{ yyval.integer = 400 + yyvsp[0].integer;}
break;
case 63:
#line 334 "vtkParse.y"
{postSig("unsigned ");}
break;
case 64:
#line 335 "vtkParse.y"
{ yyval.integer = 10 + yyvsp[0].integer;}
break;
case 65:
#line 336 "vtkParse.y"
{ yyval.integer = yyvsp[0].integer;}
break;
case 66:
#line 339 "vtkParse.y"
{ postSig("float "); yyval.integer = 1;}
break;
case 67:
#line 340 "vtkParse.y"
{ postSig("void "); yyval.integer = 2;}
break;
case 68:
#line 341 "vtkParse.y"
{ postSig("char "); yyval.integer = 3;}
break;
case 69:
#line 342 "vtkParse.y"
{ postSig("int "); yyval.integer = 4;}
break;
case 70:
#line 343 "vtkParse.y"
{ postSig("short "); yyval.integer = 5;}
break;
case 71:
#line 344 "vtkParse.y"
{ postSig("long "); yyval.integer = 6;}
break;
case 72:
#line 345 "vtkParse.y"
{ postSig("double "); yyval.integer = 7;}
break;
case 73:
#line 346 "vtkParse.y"
{       
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 8;}
break;
case 74:
#line 352 "vtkParse.y"
{ 
      char ctmpid[2048];
      sprintf(ctmpid,"%s ",yyvsp[0].str);
      postSig(ctmpid);
      yyval.integer = 9; 
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
case 77:
#line 372 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
break;
case 78:
#line 377 "vtkParse.y"
{ 
      data.SuperClasses[data.NumberOfSuperClasses] = vtkstrdup(yyvsp[0].str); 
      data.NumberOfSuperClasses++; 
    }
break;
case 80:
#line 382 "vtkParse.y"
{in_public = 1; in_protected = 0;}
break;
case 81:
#line 383 "vtkParse.y"
{in_public = 0; in_protected = 0;}
break;
case 82:
#line 384 "vtkParse.y"
{in_public = 0; in_protected = 1;}
break;
case 85:
#line 388 "vtkParse.y"
{yyval.integer = yyvsp[0].integer;}
break;
case 86:
#line 389 "vtkParse.y"
{yyval.integer = -1;}
break;
case 87:
#line 389 "vtkParse.y"
{yyval.integer = -1;}
break;
case 88:
#line 393 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 89:
#line 394 "vtkParse.y"
{
   postSig(");");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
break;
case 90:
#line 404 "vtkParse.y"
{postSig("Get");}
break;
case 91:
#line 404 "vtkParse.y"
{postSig(" ();"); invertSig = 1;}
break;
case 92:
#line 406 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = yyvsp[-1].integer;
   output_function();
   }
break;
case 93:
#line 413 "vtkParse.y"
{preSig("void Set");}
break;
case 94:
#line 414 "vtkParse.y"
{
   postSig(" (char *);"); 
   sprintf(temps,"Set%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
   output_function();
   }
break;
case 95:
#line 424 "vtkParse.y"
{preSig("char *Get");}
break;
case 96:
#line 425 "vtkParse.y"
{ 
   postSig(" ();");
   sprintf(temps,"Get%s",yyvsp[-1].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 303;
   output_function();
   }
break;
case 97:
#line 434 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 98:
#line 435 "vtkParse.y"
{postSig(");"); openSig = 0;}
break;
case 99:
#line 436 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sscanf (currentFunction->Signature, "%*s %*s (%s);", local);
   sprintf(temps,"Set%s",yyvsp[-7].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = yyvsp[-4].integer;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 2;
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
case 100:
#line 466 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 101:
#line 467 "vtkParse.y"
{ 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   }
break;
case 102:
#line 478 "vtkParse.y"
{preSig("void Set"); postSig(" ("); }
break;
case 103:
#line 479 "vtkParse.y"
{ 
   postSig("*);");
   sprintf(temps,"Set%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 309;
   currentFunction->ArgCounts[0] = 1;
   currentFunction->ReturnType = 2;
   output_function();
   }
break;
case 104:
#line 489 "vtkParse.y"
{postSig("*Get");}
break;
case 105:
#line 490 "vtkParse.y"
{postSig(" ();"); invertSig = 1;}
break;
case 106:
#line 491 "vtkParse.y"
{ 
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
   output_function();
   }
break;
case 107:
#line 499 "vtkParse.y"
{preSig("void "); postSig("On ();"); openSig = 0; }
break;
case 108:
#line 501 "vtkParse.y"
{ 
   sprintf(temps,"%sOn",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 2;
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
case 109:
#line 516 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 110:
#line 521 "vtkParse.y"
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
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[2]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 2;
   output_function();
   }
break;
case 111:
#line 546 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 112:
#line 551 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 2;
   output_function();
   }
break;
case 113:
#line 563 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 114:
#line 568 "vtkParse.y"
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
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[3]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 3;
   output_function();
   }
break;
case 115:
#line 595 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 116:
#line 600 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 3;
   output_function();
   }
break;
case 117:
#line 612 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 118:
#line 617 "vtkParse.y"
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
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[4]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 4;
   output_function();
   }
break;
case 119:
#line 646 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 120:
#line 651 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 4;
   output_function();
   }
break;
case 121:
#line 663 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 122:
#line 668 "vtkParse.y"
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
   currentFunction->ReturnType = 2;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,"void Set%s (%s a[6]);",yyvsp[-4].str,
     local);
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 300 + yyvsp[-1].integer;
   currentFunction->ArgCounts[0] = 6;
   output_function();
   }
break;
case 123:
#line 701 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 124:
#line 706 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-4].str);
   sprintf(temps,"Get%s",yyvsp[-4].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-1].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = 6;
   output_function();
   }
break;
case 125:
#line 718 "vtkParse.y"
{
      free (currentFunction->Signature);
      currentFunction->Signature = NULL;
      }
break;
case 126:
#line 723 "vtkParse.y"
{
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"void Set%s (%s [%i]);",yyvsp[-6].str,
      local, yyvsp[-1].integer);
     sprintf(temps,"Set%s",yyvsp[-6].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->ReturnType = 2;
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 300 + yyvsp[-3].integer;
     currentFunction->ArgCounts[0] = yyvsp[-1].integer;
     output_function();
   }
break;
case 127:
#line 736 "vtkParse.y"
{
     free (currentFunction->Signature);
     currentFunction->Signature = NULL;
     }
break;
case 128:
#line 741 "vtkParse.y"
{ 
   char *local = vtkstrdup(currentFunction->Signature);
   sprintf(currentFunction->Signature,"%s *Get%s ();",local, yyvsp[-6].str);
   sprintf(temps,"Get%s",yyvsp[-6].str); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 300 + yyvsp[-3].integer;
   currentFunction->HaveHint = 1;
   currentFunction->HintSize = yyvsp[-1].integer;
   output_function();
   }
break;
case 129:
#line 753 "vtkParse.y"
{ 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 2;
     currentFunction->ArgTypes[0] = 7;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 7;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ReturnType = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[2]);",
       yyvsp[-1].str);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 307;
     currentFunction->ArgCounts[0] = 2;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 2;
     output_function();
   }
break;
case 130:
#line 800 "vtkParse.y"
{ 
     sprintf(currentFunction->Signature,"vtkCoordinate *Get%sCoordinate ();",
       yyvsp[-1].str);

     sprintf(temps,"Get%sCoordinate",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 309;
     currentFunction->ReturnClass = vtkstrdup("vtkCoordinate");
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double, double, double);",
       yyvsp[-1].str);
     sprintf(temps,"Set%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 3;
     currentFunction->ArgTypes[0] = 7;
     currentFunction->ArgCounts[0] = 0;
     currentFunction->ArgTypes[1] = 7;
     currentFunction->ArgCounts[1] = 0;
     currentFunction->ArgTypes[2] = 7;
     currentFunction->ArgCounts[2] = 0;
     currentFunction->ReturnType = 2;
     output_function();

     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"void Set%s (double a[3]);",
       yyvsp[-1].str);
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 1;
     currentFunction->ArgTypes[0] = 307;
     currentFunction->ArgCounts[0] = 3;
     output_function();
     
     currentFunction->Signature = (char *)malloc(2048);
     sigAllocatedLength = 2048;
     sprintf(currentFunction->Signature,"double *Get%s ();", yyvsp[-1].str);
     sprintf(temps,"Get%s",yyvsp[-1].str); 
     currentFunction->Name = vtkstrdup(temps);
     currentFunction->NumberOfArguments = 0;
     currentFunction->ReturnType = 307;
     currentFunction->HaveHint = 1;
     currentFunction->HintSize = 3;
     output_function();
   }
break;
case 131:
#line 849 "vtkParse.y"
{ 
   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "const char *GetClassName ();");
   sprintf(temps,"GetClassName"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 1303;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature,
           "int IsA (const char *name);");
   sprintf(temps,"IsA"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 1;
   currentFunction->ArgTypes[0] = 1303;
   currentFunction->ArgCounts[0] = 0;
   currentFunction->ReturnType = 4;
   output_function();

   currentFunction->Signature = (char *)malloc(2048);
   sigAllocatedLength = 2048;
   sprintf(currentFunction->Signature, "%s *NewInstance ();",
           yyvsp[-3].str);
   sprintf(temps,"NewInstance"); 
   currentFunction->Name = vtkstrdup(temps);
   currentFunction->NumberOfArguments = 0;
   currentFunction->ReturnType = 309;
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
     currentFunction->ArgTypes[0] = 309;
     currentFunction->ArgCounts[0] = 1;
     currentFunction->ArgClasses[0] = vtkstrdup("vtkObject");
     currentFunction->ReturnType = 2309;
     currentFunction->ReturnClass = vtkstrdup(yyvsp[-3].str);
     output_function();
     }
   }
break;
#line 1940 "vtkParse.tab.c"
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
