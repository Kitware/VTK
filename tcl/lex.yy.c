#include <stdio.h>
# define U(x) x
# define NLSTATE yyprevious=YYNEWLINE
# define BEGIN yybgin = yysvec + 1 +
# define INITIAL 0
# define YYLERR yysvec
# define YYSTATE (yyestate-yysvec-1)
# define YYOPTIM 1
# define YYLMAX BUFSIZ
#ifndef __cplusplus
# define output(c) (void)putc(c,yyout)
#else
# define lex_output(c) (void)putc(c,yyout)
#endif

#if defined(__cplusplus) || defined(__STDC__)

#if defined(__cplusplus) && defined(__EXTERN_C__)
extern "C" {
#endif
	int yyback(int *, int);
	int yyinput(void);
	int yylook(void);
	void yyoutput(int);
	int yyracc(int);
	int yyreject(void);
	void yyunput(int);
	int yylex(void);
#ifdef YYLEX_E
	void yywoutput(wchar_t);
	wchar_t yywinput(void);
#endif
#ifndef yyless
	int yyless(int);
#endif
#ifndef yywrap
	int yywrap(void);
#endif
#ifdef LEXDEBUG
	void allprint(char);
	void sprint(char *);
#endif
#if defined(__cplusplus) && defined(__EXTERN_C__)
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
	void exit(int);
#ifdef __cplusplus
}
#endif

#endif
# define unput(c) {yytchar= (c);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;}
# define yymore() (yymorfg=1)
#ifndef __cplusplus
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
#else
# define lex_input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
#endif
#define ECHO fprintf(yyout, "%s",yytext)
# define REJECT { nstr = yyreject(); goto yyfussy;}
int yyleng;
char yytext[YYLMAX];
int yymorfg;
extern char *yysptr, yysbuf[];
int yytchar;
FILE *yyin = {stdin}, *yyout = {stdout};
extern int yylineno;
struct yysvf { 
	struct yywork *yystoff;
	struct yysvf *yyother;
	int *yystops;};
struct yysvf *yyestate;
extern struct yysvf yysvec[], *yybgin;
# define YYNEWLINE 10
yylex(){
int nstr; extern int yyprevious;
#ifdef __cplusplus
/* to avoid CC and lint complaining yyfussy not being used ...*/
static int __lex_hack = 0;
if (__lex_hack) goto yyfussy;
#endif
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:

# line 5 "../../vtk/tcl/concrete.l"
{ int c1 = 0, c2 = input();
       for (;;)
	 {
         if (c2 == EOF) break;
         if (c1 == '*' && c2 == '/') break;
         c1 = c2; c2 = input();
         }
     }
break;
case 2:

# line 14 "../../vtk/tcl/concrete.l"

     { int c1 = 0, c2 = 0, c3 = 0, c4 = 0, c5 = input();
       for (;;)
	 {
         if (c5 == EOF) break;
         if (c1 == '/' && c2 == '/' && c3 == 'E' && c4 == 'T' && c5 == 'X') break;
         c1 = c2; c2 = c3; c3 = c4; c4 = c5; c5 = input();
         }
       for (;;)
         {
         if (c5 == EOF) break;
         if (c5 == '\n') break;
         c5 = input();
         } 
     }
break;
case 3:

# line 31 "../../vtk/tcl/concrete.l"
          ;
break;
case 4:

# line 33 "../../vtk/tcl/concrete.l"
;
break;
case 5:

# line 35 "../../vtk/tcl/concrete.l"
 
  { sscanf(yytext+1,"%d",&yylval.integer); return(ARRAY_NUM);}
break;
case 6:

# line 39 "../../vtk/tcl/concrete.l"
 return(CLASS_REF);
break;
case 7:

# line 41 "../../vtk/tcl/concrete.l"
return(VAR_FUNCTION);
break;
case 8:

# line 43 "../../vtk/tcl/concrete.l"
return(SHORT);
break;
case 9:

# line 44 "../../vtk/tcl/concrete.l"
 return(LONG);
break;
case 10:

# line 45 "../../vtk/tcl/concrete.l"
return(SHORT);
break;
case 11:

# line 46 "../../vtk/tcl/concrete.l"
 return(LONG);
break;
case 12:

# line 48 "../../vtk/tcl/concrete.l"
	return(CLASS);
break;
case 13:

# line 49 "../../vtk/tcl/concrete.l"
 	return(PUBLIC);
break;
case 14:

# line 50 "../../vtk/tcl/concrete.l"
 	return(PRIVATE);
break;
case 15:

# line 51 "../../vtk/tcl/concrete.l"
	return(PROTECTED);
break;
case 16:

# line 52 "../../vtk/tcl/concrete.l"
	        return(INT);
break;
case 17:

# line 53 "../../vtk/tcl/concrete.l"
	return(FLOAT);
break;
case 18:

# line 54 "../../vtk/tcl/concrete.l"
	return(SHORT);
break;
case 19:

# line 55 "../../vtk/tcl/concrete.l"
 	return(LONG);
break;
case 20:

# line 56 "../../vtk/tcl/concrete.l"
	return(DOUBLE);
break;
case 21:

# line 57 "../../vtk/tcl/concrete.l"
	        return(VOID);
break;
case 22:

# line 58 "../../vtk/tcl/concrete.l"
	        return(CHAR);
break;
case 23:

# line 59 "../../vtk/tcl/concrete.l"
      return(VIRTUAL);
break;
case 24:

# line 60 "../../vtk/tcl/concrete.l"
        return(CONST);
break;
case 25:

# line 61 "../../vtk/tcl/concrete.l"
     return(OPERATOR);
break;
case 26:

# line 62 "../../vtk/tcl/concrete.l"
     return(UNSIGNED);
break;
case 27:

# line 63 "../../vtk/tcl/concrete.l"
       return(FRIEND);
break;
case 28:

# line 64 "../../vtk/tcl/concrete.l"
       return(STATIC);
break;
case 29:

# line 66 "../../vtk/tcl/concrete.l"
         return(SetMacro);
break;
case 30:

# line 67 "../../vtk/tcl/concrete.l"
         return(GetMacro);
break;
case 31:

# line 68 "../../vtk/tcl/concrete.l"
   return(SetStringMacro);
break;
case 32:

# line 69 "../../vtk/tcl/concrete.l"
   return(GetStringMacro);
break;
case 33:

# line 70 "../../vtk/tcl/concrete.l"
    return(SetClampMacro);
break;
case 34:

# line 71 "../../vtk/tcl/concrete.l"
   return(SetObjectMacro);
break;
case 35:

# line 72 "../../vtk/tcl/concrete.l"
return(SetRefCountedObjectMacro);
break;
case 36:

# line 73 "../../vtk/tcl/concrete.l"
   return(GetObjectMacro);
break;
case 37:

# line 74 "../../vtk/tcl/concrete.l"
     return(BooleanMacro);
break;
case 38:

# line 75 "../../vtk/tcl/concrete.l"
  return(SetVector2Macro);
break;
case 39:

# line 76 "../../vtk/tcl/concrete.l"
  return(SetVector3Macro);
break;
case 40:

# line 77 "../../vtk/tcl/concrete.l"
  return(SetVector4Macro);
break;
case 41:

# line 78 "../../vtk/tcl/concrete.l"
   return(SetVectorMacro);
break;
case 42:

# line 79 "../../vtk/tcl/concrete.l"
   return(GetVectorMacro);
break;
case 43:

# line 80 "../../vtk/tcl/concrete.l"
return(ImageRegionSetMacro);
break;
case 44:

# line 81 "../../vtk/tcl/concrete.l"
return(ImageRegionSetBoundsMacro);
break;
case 45:

# line 83 "../../vtk/tcl/concrete.l"
	{ sscanf(yytext,"%d",&yylval.integer); return(NUM);}
break;
case 46:

# line 85 "../../vtk/tcl/concrete.l"
{
		yylval.str =  strdup(yytext + 1);
		yylval.str[strlen(yytext)-2] = '\0';
		return(STRING);
		}
break;
case 47:

# line 91 "../../vtk/tcl/concrete.l"
    ;
break;
case 48:

# line 94 "../../vtk/tcl/concrete.l"
 { yylval.str = (char *)malloc(yyleng + 1);
                     memcpy(yylval.str,yytext,yyleng);
                     yylval.str[yyleng] = '\0';
                     return(VTK_ID);
                    }
break;
case 49:

# line 100 "../../vtk/tcl/concrete.l"
 { yylval.str = (char *)malloc(yyleng + 1);
                          memcpy(yylval.str,yytext,yyleng);
                          yylval.str[yyleng] = '\0';
                          return(ID);
                        }
break;
case 50:

# line 106 "../../vtk/tcl/concrete.l"
;
break;
case 51:

# line 108 "../../vtk/tcl/concrete.l"
return(yytext[0]);
break;
case 52:

# line 109 "../../vtk/tcl/concrete.l"
return(yytext[0]);
break;
case 53:

# line 112 "../../vtk/tcl/concrete.l"
return(yytext[0]);
break;
case 54:

# line 114 "../../vtk/tcl/concrete.l"
	return(OTHER);
break;
case -1:
break;
default:
(void)fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */
int yyvstop[] = {
0,

54,
0,

50,
54,
0,

50,
0,

54,
0,

53,
54,
0,

54,
0,

45,
54,
0,

49,
54,
0,

51,
54,
0,

52,
54,
0,

49,
54,
0,

49,
54,
0,

49,
54,
0,

49,
54,
0,

49,
54,
0,

49,
54,
0,

49,
54,
0,

49,
54,
0,

49,
54,
0,

49,
54,
0,

54,
-47,
0,

1,
0,

-3,
0,

45,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

-47,
0,

47,
0,

46,
0,

-3,
0,

3,
0,

-3,
0,

5,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

16,
49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

48,
49,
0,

-3,
0,

22,
49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

19,
49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

21,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

-2,
-3,
0,

12,
49,
0,

24,
49,
0,

49,
0,

17,
49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

18,
49,
0,

49,
0,

49,
0,

49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

2,
3,
0,

20,
49,
0,

27,
49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

49,
0,

13,
49,
0,

49,
0,

28,
49,
0,

49,
0,

49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

11,
49,
0,

49,
0,

9,
49,
0,

49,
0,

14,
49,
0,

49,
0,

49,
0,

49,
0,

23,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

11,
0,

10,
49,
0,

9,
0,

25,
49,
0,

49,
0,

8,
49,
0,

26,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

10,
0,

15,
49,
0,

8,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

6,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

30,
48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

29,
48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

4,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

7,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

37,
48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

33,
48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

36,
48,
49,
0,

32,
48,
49,
0,

42,
48,
49,
0,

48,
49,
0,

34,
48,
49,
0,

48,
49,
0,

31,
48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

41,
48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

38,
48,
49,
0,

39,
48,
49,
0,

40,
48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

43,
48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

48,
49,
0,

35,
48,
49,
0,

44,
48,
49,
0,
0};
# define YYTYPE int
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	1,3,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	1,4,	1,5,	
0,0,	4,5,	4,5,	0,0,	
0,0,	0,0,	0,0,	0,0,	
6,24,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
6,24,	6,24,	0,0,	0,0,	
0,0,	1,4,	0,0,	1,6,	
4,5,	24,50,	0,0,	1,7,	
0,0,	0,0,	1,7,	0,0,	
0,0,	0,0,	0,0,	8,25,	
1,8,	1,9,	1,9,	6,24,	
8,26,	6,0,	0,0,	0,0,	
0,0,	6,24,	0,0,	2,23,	
6,24,	0,0,	93,120,	93,120,	
0,0,	0,0,	1,10,	6,24,	
6,24,	0,0,	0,0,	2,8,	
9,27,	9,27,	9,27,	9,27,	
9,27,	9,27,	9,27,	9,27,	
9,27,	9,27,	101,128,	0,0,	
6,24,	93,120,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
1,11,	93,121,	1,12,	0,0,	
0,0,	53,74,	0,0,	0,0,	
1,13,	1,14,	74,100,	1,15,	
0,0,	0,0,	1,16,	13,31,	
0,0,	1,17,	31,56,	13,32,	
1,18,	1,19,	13,33,	2,11,	
1,20,	2,12,	1,21,	1,22,	
14,34,	15,35,	16,37,	2,13,	
2,14,	17,38,	2,15,	15,36,	
18,39,	2,16,	19,40,	21,44,	
2,17,	19,41,	32,57,	2,18,	
2,19,	33,58,	34,59,	2,20,	
35,60,	2,21,	2,22,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	36,61,	37,62,	38,63,	
39,64,	41,67,	42,68,	43,69,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	44,70,	45,71,	
46,72,	47,73,	10,28,	56,75,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	10,28,	10,28,	
10,28,	10,28,	11,29,	11,29,	
20,42,	40,65,	22,45,	54,54,	
54,54,	57,76,	58,77,	40,66,	
22,46,	59,78,	60,79,	61,80,	
20,43,	22,47,	63,84,	64,85,	
23,48,	65,86,	66,87,	67,88,	
68,89,	11,29,	69,90,	70,91,	
23,48,	23,49,	54,54,	71,92,	
26,51,	62,81,	62,81,	30,54,	
30,54,	72,93,	76,101,	77,102,	
26,51,	26,52,	11,30,	11,30,	
11,30,	11,30,	11,30,	11,30,	
11,30,	11,30,	11,30,	23,48,	
78,103,	23,48,	79,104,	80,105,	
62,81,	23,48,	30,54,	82,108,	
23,48,	83,109,	51,51,	26,51,	
85,112,	26,51,	86,113,	23,48,	
23,48,	26,51,	51,51,	51,52,	
26,51,	87,114,	30,30,	88,115,	
89,116,	90,117,	91,118,	26,51,	
26,51,	92,119,	103,129,	105,130,	
23,48,	106,131,	107,132,	108,133,	
84,110,	84,110,	109,134,	54,55,	
111,136,	51,51,	112,137,	51,51,	
26,51,	26,53,	113,138,	51,51,	
94,94,	114,139,	51,51,	115,140,	
117,143,	94,94,	118,144,	94,94,	
95,94,	51,51,	51,51,	84,110,	
94,94,	95,94,	119,145,	95,94,	
131,153,	94,94,	132,154,	30,55,	
95,94,	133,155,	134,156,	135,157,	
136,158,	95,94,	51,51,	81,81,	
81,81,	128,128,	128,128,	137,159,	
62,82,	138,160,	139,161,	142,163,	
144,164,	145,165,	152,181,	62,83,	
73,94,	73,94,	73,94,	73,94,	
73,94,	73,94,	73,94,	73,94,	
73,94,	73,94,	81,81,	153,182,	
128,128,	95,122,	154,183,	156,184,	
157,185,	73,94,	73,95,	73,94,	
73,94,	73,94,	73,94,	73,96,	
73,94,	73,97,	73,94,	73,94,	
73,94,	73,94,	73,98,	73,94,	
73,94,	73,94,	73,94,	73,99,	
73,94,	73,94,	73,94,	73,94,	
73,94,	73,94,	73,94,	159,186,	
84,111,	161,187,	162,188,	73,94,	
163,189,	73,94,	73,94,	73,94,	
73,94,	73,94,	73,94,	73,94,	
73,94,	73,94,	73,94,	73,94,	
73,94,	73,94,	73,94,	73,94,	
73,94,	73,94,	73,94,	73,94,	
73,94,	73,94,	73,94,	73,94,	
73,94,	73,94,	73,94,	96,94,	
110,110,	110,110,	97,94,	164,190,	
96,94,	181,205,	96,94,	97,94,	
98,94,	97,94,	81,106,	96,94,	
183,206,	98,94,	97,94,	98,94,	
96,94,	81,107,	187,207,	97,94,	
98,94,	99,94,	128,152,	110,110,	
188,208,	98,94,	99,94,	100,100,	
99,94,	116,141,	116,141,	120,120,	
120,120,	99,94,	96,123,	100,100,	
100,127,	209,225,	99,94,	225,239,	
121,121,	121,121,	239,254,	141,141,	
141,141,	97,124,	0,0,	246,246,	
0,0,	0,0,	0,0,	0,0,	
116,141,	98,125,	120,120,	246,247,	
99,126,	0,0,	100,100,	0,0,	
100,100,	0,0,	120,121,	121,121,	
100,100,	122,94,	141,141,	100,100,	
0,0,	0,0,	122,94,	123,94,	
122,94,	121,146,	100,100,	100,100,	
123,94,	122,94,	123,94,	0,0,	
124,94,	0,0,	122,94,	123,94,	
0,0,	124,94,	0,0,	124,94,	
123,94,	0,0,	0,0,	100,100,	
124,94,	0,0,	0,0,	125,94,	
110,135,	124,94,	0,0,	0,0,	
125,94,	126,94,	125,94,	0,0,	
0,0,	0,0,	126,94,	125,94,	
126,94,	0,0,	122,147,	124,149,	
125,94,	126,94,	146,166,	146,166,	
0,0,	150,94,	126,94,	167,167,	
167,167,	123,148,	150,94,	147,94,	
150,94,	116,142,	149,94,	0,0,	
147,94,	150,94,	147,94,	149,94,	
0,0,	149,94,	150,94,	147,94,	
150,174,	146,166,	149,94,	141,162,	
147,94,	0,0,	167,167,	149,94,	
0,0,	125,150,	146,167,	0,0,	
0,0,	0,0,	167,191,	126,151,	
0,0,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	149,173,	
0,0,	147,168,	0,0,	0,0,	
0,0,	0,0,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
0,0,	0,0,	0,0,	0,0,	
146,146,	0,0,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
146,146,	146,146,	146,146,	146,146,	
148,94,	166,166,	166,166,	0,0,	
0,0,	148,94,	0,0,	148,94,	
0,0,	0,0,	0,0,	148,169,	
148,94,	148,170,	151,94,	151,175,	
0,0,	148,171,	0,0,	151,94,	
148,172,	151,94,	191,191,	191,191,	
166,166,	151,176,	151,94,	151,177,	
0,0,	0,0,	151,178,	151,179,	
168,94,	166,167,	151,180,	169,94,	
170,94,	168,94,	0,0,	168,94,	
169,94,	170,94,	169,94,	170,94,	
168,94,	191,191,	0,0,	169,94,	
170,94,	168,94,	223,223,	223,223,	
169,94,	170,94,	0,0,	0,0,	
0,0,	171,94,	172,94,	0,0,	
0,0,	0,0,	171,94,	172,94,	
171,94,	172,94,	169,193,	168,192,	
170,194,	171,94,	172,94,	173,94,	
174,94,	223,223,	171,94,	172,94,	
173,94,	174,94,	173,94,	174,94,	
175,94,	254,254,	254,254,	173,94,	
174,94,	175,94,	0,0,	175,94,	
173,94,	174,94,	0,0,	0,0,	
175,94,	172,196,	0,0,	0,0,	
176,94,	175,94,	0,0,	0,0,	
223,224,	176,94,	0,0,	176,94,	
254,254,	0,0,	173,197,	171,195,	
176,94,	0,0,	0,0,	177,94,	
178,94,	176,94,	254,266,	0,0,	
177,94,	178,94,	177,94,	178,94,	
0,0,	174,198,	175,199,	177,94,	
178,94,	179,94,	0,0,	176,200,	
177,94,	178,94,	179,94,	191,209,	
179,94,	180,94,	192,94,	0,0,	
0,0,	179,94,	180,94,	192,94,	
180,94,	192,94,	179,94,	177,201,	
0,0,	180,94,	192,94,	178,202,	
0,0,	193,94,	180,94,	192,94,	
194,94,	0,0,	193,94,	195,94,	
193,94,	194,94,	0,0,	194,94,	
195,94,	193,94,	195,94,	0,0,	
194,94,	192,210,	193,94,	195,94,	
180,204,	194,94,	0,0,	196,94,	
195,94,	0,0,	197,94,	179,203,	
196,94,	0,0,	196,94,	197,94,	
0,0,	197,94,	193,211,	196,94,	
0,0,	0,0,	197,94,	0,0,	
196,94,	198,94,	197,215,	197,94,	
194,212,	0,0,	198,94,	199,94,	
198,94,	0,0,	0,0,	0,0,	
199,94,	198,94,	199,94,	195,213,	
196,214,	0,0,	198,94,	199,94,	
200,94,	0,0,	0,0,	201,94,	
199,94,	200,94,	0,0,	200,94,	
201,94,	0,0,	201,94,	0,0,	
200,94,	0,0,	202,94,	201,94,	
198,216,	200,94,	199,217,	202,94,	
201,94,	202,94,	203,94,	204,94,	
0,0,	0,0,	202,94,	203,94,	
204,94,	203,94,	204,94,	202,94,	
0,0,	200,218,	203,94,	204,94,	
205,223,	205,223,	0,0,	203,94,	
204,94,	210,94,	0,0,	201,219,	
0,0,	0,0,	210,94,	0,0,	
210,94,	0,0,	202,220,	0,0,	
0,0,	210,94,	0,0,	212,94,	
204,222,	0,0,	210,94,	205,223,	
212,94,	0,0,	212,94,	0,0,	
0,0,	0,0,	0,0,	212,94,	
0,0,	0,0,	203,221,	0,0,	
212,94,	0,0,	0,0,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	210,226,	205,224,	0,0,	
0,0,	0,0,	212,228,	0,0,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	0,0,	0,0,	
0,0,	0,0,	205,205,	0,0,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	205,205,	205,205,	
205,205,	205,205,	211,94,	213,94,	
0,0,	0,0,	214,94,	211,94,	
213,94,	211,94,	213,94,	214,94,	
0,0,	214,94,	211,94,	213,94,	
0,0,	215,94,	214,94,	211,94,	
213,94,	216,94,	215,94,	214,94,	
215,94,	0,0,	216,94,	0,0,	
216,94,	215,94,	0,0,	0,0,	
217,94,	216,94,	215,94,	0,0,	
218,94,	217,94,	216,94,	217,94,	
0,0,	218,94,	213,229,	218,94,	
217,94,	0,0,	0,0,	0,0,	
218,94,	217,94,	211,227,	0,0,	
215,231,	218,94,	219,94,	216,232,	
214,230,	220,94,	220,236,	219,94,	
221,94,	219,94,	220,94,	0,0,	
220,94,	221,94,	219,94,	221,94,	
0,0,	220,94,	0,0,	219,94,	
221,94,	222,94,	220,94,	217,233,	
226,94,	221,94,	222,94,	0,0,	
222,94,	226,94,	0,0,	226,94,	
218,234,	222,94,	0,0,	226,240,	
226,94,	219,235,	222,94,	227,94,	
0,0,	226,94,	0,0,	228,94,	
227,94,	0,0,	227,94,	221,237,	
228,94,	0,0,	228,94,	227,94,	
229,94,	0,0,	232,246,	228,94,	
227,94,	229,94,	230,94,	229,94,	
228,94,	231,94,	232,247,	230,94,	
229,94,	230,94,	231,94,	0,0,	
231,94,	229,94,	230,94,	222,238,	
0,0,	231,94,	233,94,	230,94,	
228,242,	0,0,	231,94,	233,94,	
234,94,	233,94,	0,0,	0,0,	
227,241,	234,94,	233,94,	234,94,	
232,94,	0,0,	0,0,	233,94,	
234,94,	232,94,	235,94,	232,94,	
229,243,	234,94,	231,245,	235,94,	
232,94,	235,94,	236,94,	230,244,	
0,0,	232,94,	235,94,	236,94,	
0,0,	236,94,	0,0,	235,94,	
0,0,	0,0,	236,94,	0,0,	
237,94,	238,94,	0,0,	236,94,	
233,248,	237,94,	238,94,	237,94,	
238,94,	234,249,	0,0,	235,250,	
237,94,	238,94,	0,0,	241,94,	
240,94,	237,94,	238,94,	0,0,	
241,94,	240,94,	241,94,	240,94,	
0,0,	0,0,	0,0,	241,94,	
240,94,	0,0,	0,0,	236,251,	
241,94,	240,94,	0,0,	0,0,	
242,94,	0,0,	0,0,	0,0,	
0,0,	242,94,	0,0,	242,94,	
237,252,	243,94,	238,253,	240,255,	
242,94,	244,94,	243,94,	0,0,	
243,94,	242,94,	244,94,	0,0,	
244,94,	243,94,	0,0,	0,0,	
245,94,	244,94,	243,94,	0,0,	
248,94,	245,94,	244,94,	245,94,	
0,0,	248,94,	247,247,	248,94,	
245,94,	266,266,	266,266,	248,261,	
248,94,	245,94,	247,247,	247,247,	
0,0,	248,94,	243,257,	249,94,	
0,0,	0,0,	242,256,	0,0,	
249,94,	0,0,	249,94,	0,0,	
0,0,	0,0,	0,0,	249,94,	
266,266,	244,258,	0,0,	245,259,	
249,94,	247,247,	0,0,	247,247,	
250,94,	266,280,	0,0,	247,247,	
251,94,	250,94,	247,260,	250,94,	
0,0,	251,94,	0,0,	251,94,	
250,94,	247,247,	247,247,	252,94,	
251,94,	250,94,	0,0,	253,94,	
252,94,	251,94,	252,94,	0,0,	
253,94,	0,0,	253,94,	252,94,	
255,94,	0,0,	247,247,	253,94,	
252,94,	255,94,	0,0,	255,94,	
253,94,	256,94,	0,0,	0,0,	
255,94,	0,0,	256,94,	0,0,	
256,94,	255,94,	257,94,	0,0,	
256,268,	256,94,	250,262,	257,94,	
252,264,	257,94,	256,94,	251,263,	
0,0,	257,269,	257,94,	258,94,	
0,0,	255,267,	0,0,	257,94,	
258,94,	0,0,	258,94,	253,265,	
259,94,	0,0,	258,270,	258,94,	
261,94,	259,94,	0,0,	259,94,	
258,94,	261,94,	0,0,	261,94,	
259,94,	262,94,	0,0,	0,0,	
261,94,	259,94,	262,94,	0,0,	
262,94,	261,94,	263,94,	0,0,	
262,273,	262,94,	264,94,	263,94,	
0,0,	263,94,	262,94,	264,94,	
0,0,	264,94,	263,94,	261,272,	
0,0,	264,275,	264,94,	263,94,	
265,276,	265,277,	265,278,	264,94,	
0,0,	259,271,	0,0,	267,94,	
268,94,	0,0,	0,0,	0,0,	
267,94,	268,94,	267,94,	268,94,	
265,94,	0,0,	0,0,	267,94,	
268,94,	265,94,	0,0,	265,94,	
267,94,	268,94,	263,274,	265,279,	
265,94,	269,94,	270,94,	0,0,	
0,0,	265,94,	269,94,	270,94,	
269,94,	270,94,	0,0,	268,282,	
0,0,	269,94,	270,94,	0,0,	
0,0,	271,94,	269,94,	270,94,	
0,0,	0,0,	271,94,	272,94,	
271,94,	0,0,	0,0,	267,281,	
272,94,	271,94,	272,94,	273,94,	
269,283,	270,284,	271,94,	272,94,	
273,94,	274,94,	273,94,	0,0,	
272,94,	0,0,	274,94,	273,94,	
274,94,	0,0,	275,94,	0,0,	
273,94,	274,94,	0,0,	275,94,	
0,0,	275,94,	274,94,	0,0,	
272,286,	0,0,	275,94,	276,94,	
0,0,	271,285,	273,287,	275,94,	
276,94,	0,0,	276,94,	277,94,	
279,94,	0,0,	276,290,	276,94,	
277,94,	279,94,	277,94,	279,94,	
276,94,	275,289,	277,291,	277,94,	
279,94,	278,94,	0,0,	0,0,	
277,94,	279,94,	278,94,	274,288,	
278,94,	281,94,	282,94,	0,0,	
278,292,	278,94,	281,94,	282,94,	
281,94,	282,94,	278,94,	279,293,	
0,0,	281,94,	282,94,	283,94,	
284,94,	0,0,	281,94,	282,94,	
283,94,	284,94,	283,94,	284,94,	
285,94,	0,0,	0,0,	283,94,	
284,94,	285,94,	0,0,	285,94,	
283,94,	284,94,	0,0,	282,295,	
285,94,	0,0,	286,94,	0,0,	
0,0,	285,298,	0,0,	286,94,	
0,0,	286,94,	281,294,	287,94,	
283,296,	284,297,	286,94,	0,0,	
287,94,	288,94,	287,94,	286,94,	
289,94,	0,0,	288,94,	287,94,	
288,94,	289,94,	290,94,	289,94,	
287,94,	288,94,	0,0,	290,94,	
289,94,	290,94,	288,94,	0,0,	
0,0,	289,94,	290,94,	0,0,	
0,0,	0,0,	291,94,	290,94,	
287,300,	0,0,	0,0,	291,94,	
0,0,	291,94,	286,299,	292,94,	
288,301,	289,302,	291,94,	293,94,	
292,94,	290,303,	292,94,	291,94,	
293,94,	0,0,	293,94,	292,94,	
294,94,	0,0,	0,0,	293,94,	
292,94,	294,94,	0,0,	294,94,	
293,94,	291,304,	0,0,	0,0,	
294,94,	0,0,	295,94,	296,94,	
0,0,	294,94,	292,305,	295,94,	
296,94,	295,94,	296,94,	297,94,	
293,306,	0,0,	295,94,	296,94,	
297,94,	298,94,	297,94,	295,94,	
296,94,	0,0,	298,94,	297,94,	
298,94,	299,94,	300,94,	0,0,	
297,94,	298,94,	299,94,	300,94,	
299,94,	300,94,	298,94,	0,0,	
0,0,	299,94,	300,94,	0,0,	
0,0,	301,94,	299,94,	300,94,	
0,0,	0,0,	301,94,	0,0,	
301,94,	302,94,	295,307,	296,308,	
298,310,	301,94,	302,94,	303,94,	
302,94,	0,0,	301,94,	297,309,	
303,94,	302,94,	303,94,	0,0,	
306,94,	0,0,	302,94,	303,94,	
0,0,	306,94,	299,311,	306,94,	
303,94,	304,94,	300,312,	301,313,	
306,94,	305,94,	304,94,	0,0,	
304,94,	306,94,	305,94,	0,0,	
305,94,	304,94,	307,94,	0,0,	
303,315,	305,94,	304,94,	307,94,	
0,0,	307,94,	305,94,	0,0,	
0,0,	302,314,	307,94,	0,0,	
0,0,	0,0,	308,94,	307,94,	
0,0,	0,0,	304,316,	308,94,	
309,94,	308,94,	305,317,	310,94,	
306,318,	309,94,	308,94,	309,94,	
310,94,	311,94,	310,94,	308,94,	
309,94,	0,0,	311,94,	310,94,	
311,94,	309,94,	0,0,	0,0,	
310,94,	311,94,	0,0,	307,319,	
312,94,	313,94,	311,94,	0,0,	
0,0,	312,94,	313,94,	312,94,	
313,94,	0,0,	0,0,	0,0,	
312,94,	313,94,	313,324,	308,320,	
0,0,	312,94,	313,94,	314,94,	
315,94,	309,321,	0,0,	316,94,	
314,94,	315,94,	314,94,	315,94,	
316,94,	310,322,	316,94,	314,94,	
315,94,	0,0,	317,94,	316,94,	
314,94,	315,94,	0,0,	317,94,	
316,94,	317,94,	318,94,	319,94,	
0,0,	312,323,	317,94,	318,94,	
319,94,	318,94,	319,94,	317,94,	
0,0,	0,0,	318,94,	319,94,	
0,0,	0,0,	320,94,	318,94,	
319,94,	0,0,	321,94,	320,94,	
314,325,	320,94,	0,0,	321,94,	
315,326,	321,94,	320,94,	316,327,	
0,0,	322,330,	321,94,	320,94,	
323,94,	0,0,	322,94,	321,94,	
322,94,	323,94,	317,328,	323,94,	
322,331,	322,94,	0,0,	318,329,	
323,94,	324,94,	322,94,	0,0,	
325,94,	323,94,	324,94,	326,94,	
324,94,	325,94,	327,94,	325,94,	
326,94,	324,94,	326,94,	327,94,	
325,94,	327,94,	324,94,	326,94,	
0,0,	325,94,	327,94,	328,94,	
326,94,	0,0,	0,0,	327,94,	
328,94,	329,94,	328,94,	0,0,	
330,94,	324,332,	329,94,	328,94,	
329,94,	330,94,	0,0,	330,94,	
328,94,	329,94,	0,0,	0,0,	
330,94,	0,0,	329,94,	0,0,	
0,0,	330,94,	0,0,	0,0,	
326,333,	331,94,	332,94,	327,334,	
0,0,	333,94,	331,94,	332,94,	
331,94,	332,94,	333,94,	0,0,	
333,94,	331,94,	332,94,	0,0,	
328,335,	333,94,	331,94,	332,94,	
0,0,	334,94,	333,94,	0,0,	
335,94,	330,336,	334,94,	0,0,	
334,94,	335,94,	0,0,	335,94,	
331,337,	334,94,	0,0,	0,0,	
335,94,	0,0,	334,94,	336,94,	
337,94,	335,94,	332,338,	338,94,	
336,94,	337,94,	336,94,	337,94,	
338,94,	0,0,	338,94,	336,94,	
337,94,	0,0,	339,94,	338,94,	
336,94,	337,94,	340,94,	339,94,	
338,94,	339,94,	341,94,	340,94,	
0,0,	340,94,	339,94,	341,94,	
0,0,	341,94,	340,94,	339,94,	
0,0,	337,340,	341,94,	340,94,	
342,94,	343,94,	338,341,	341,94,	
344,94,	342,94,	343,94,	342,94,	
343,94,	344,94,	0,0,	344,94,	
342,94,	343,94,	336,339,	0,0,	
344,94,	342,94,	343,94,	341,344,	
345,94,	344,94,	339,342,	0,0,	
346,94,	345,94,	0,0,	345,94,	
347,94,	346,94,	340,343,	346,94,	
345,94,	347,94,	342,345,	347,94,	
346,94,	345,94,	0,0,	347,349,	
347,94,	346,94,	0,0,	0,0,	
348,94,	347,94,	343,346,	0,0,	
0,0,	348,94,	0,0,	348,94,	
0,0,	349,94,	344,347,	348,350,	
348,94,	0,0,	349,94,	350,94,	
349,94,	348,94,	0,0,	0,0,	
350,94,	349,94,	350,94,	0,0,	
351,94,	345,348,	349,94,	350,94,	
0,0,	351,94,	352,94,	351,94,	
350,94,	0,0,	0,0,	352,94,	
351,94,	352,94,	353,94,	0,0,	
349,351,	351,94,	352,94,	353,94,	
354,94,	353,94,	350,352,	352,94,	
0,0,	354,94,	353,94,	354,94,	
0,0,	355,94,	0,0,	353,94,	
354,94,	351,353,	355,94,	356,94,	
355,94,	354,94,	357,94,	352,354,	
356,94,	355,94,	356,94,	357,94,	
358,94,	357,94,	355,94,	356,94,	
0,0,	358,94,	357,94,	358,94,	
356,94,	0,0,	0,0,	357,94,	
358,94,	0,0,	0,0,	0,0,	
0,0,	358,94,	353,355,	0,0,	
0,0,	0,0,	0,0,	0,0,	
354,356,	0,0,	0,0,	0,0,	
0,0,	0,0,	355,357,	0,0,	
0,0,	0,0,	0,0,	0,0,	
356,358,	0,0,	0,0,	0,0,	
0,0};
struct yysvf yysvec[] = {
0,	0,	0,
yycrank+-1,	0,		0,	
yycrank+-24,	yysvec+1,	0,	
yycrank+0,	0,		yyvstop+1,
yycrank+4,	0,		yyvstop+3,
yycrank+0,	yysvec+4,	yyvstop+6,
yycrank+-19,	0,		yyvstop+8,
yycrank+0,	0,		yyvstop+10,
yycrank+5,	0,		yyvstop+13,
yycrank+24,	0,		yyvstop+15,
yycrank+95,	0,		yyvstop+18,
yycrank+209,	0,		yyvstop+21,
yycrank+0,	0,		yyvstop+24,
yycrank+3,	yysvec+10,	yyvstop+27,
yycrank+9,	yysvec+10,	yyvstop+30,
yycrank+13,	yysvec+10,	yyvstop+33,
yycrank+12,	yysvec+10,	yyvstop+36,
yycrank+14,	yysvec+10,	yyvstop+39,
yycrank+16,	yysvec+10,	yyvstop+42,
yycrank+16,	yysvec+10,	yyvstop+45,
yycrank+116,	yysvec+10,	yyvstop+48,
yycrank+21,	yysvec+10,	yyvstop+51,
yycrank+117,	yysvec+10,	yyvstop+54,
yycrank+-235,	0,		yyvstop+57,
yycrank+-3,	yysvec+6,	0,	
yycrank+0,	0,		yyvstop+60,
yycrank+-247,	0,		yyvstop+62,
yycrank+0,	yysvec+9,	yyvstop+64,
yycrank+0,	yysvec+10,	yyvstop+66,
yycrank+0,	yysvec+11,	0,	
yycrank+242,	yysvec+11,	0,	
yycrank+13,	yysvec+10,	yyvstop+68,
yycrank+37,	yysvec+10,	yyvstop+70,
yycrank+27,	yysvec+10,	yyvstop+72,
yycrank+21,	yysvec+10,	yyvstop+74,
yycrank+29,	yysvec+10,	yyvstop+76,
yycrank+48,	yysvec+10,	yyvstop+78,
yycrank+38,	yysvec+10,	yyvstop+80,
yycrank+45,	yysvec+10,	yyvstop+82,
yycrank+55,	yysvec+10,	yyvstop+84,
yycrank+116,	yysvec+10,	yyvstop+86,
yycrank+59,	yysvec+10,	yyvstop+88,
yycrank+47,	yysvec+10,	yyvstop+90,
yycrank+62,	yysvec+10,	yyvstop+92,
yycrank+71,	yysvec+10,	yyvstop+94,
yycrank+73,	yysvec+10,	yyvstop+96,
yycrank+83,	yysvec+10,	yyvstop+98,
yycrank+82,	yysvec+10,	yyvstop+100,
yycrank+0,	yysvec+23,	yyvstop+102,
yycrank+0,	0,		yyvstop+104,
yycrank+0,	0,		yyvstop+106,
yycrank+-277,	0,		yyvstop+108,
yycrank+0,	0,		yyvstop+110,
yycrank+-13,	yysvec+51,	yyvstop+112,
yycrank+214,	0,		0,	
yycrank+0,	0,		yyvstop+114,
yycrank+77,	yysvec+10,	yyvstop+116,
yycrank+110,	yysvec+10,	yyvstop+118,
yycrank+111,	yysvec+10,	yyvstop+120,
yycrank+131,	yysvec+10,	yyvstop+122,
yycrank+133,	yysvec+10,	yyvstop+124,
yycrank+130,	yysvec+10,	yyvstop+126,
yycrank+240,	yysvec+10,	yyvstop+128,
yycrank+131,	yysvec+10,	yyvstop+131,
yycrank+121,	yysvec+10,	yyvstop+133,
yycrank+119,	yysvec+10,	yyvstop+135,
yycrank+122,	yysvec+10,	yyvstop+137,
yycrank+131,	yysvec+10,	yyvstop+139,
yycrank+126,	yysvec+10,	yyvstop+141,
yycrank+126,	yysvec+10,	yyvstop+143,
yycrank+138,	yysvec+10,	yyvstop+145,
yycrank+131,	yysvec+10,	yyvstop+147,
yycrank+153,	yysvec+10,	yyvstop+149,
yycrank+308,	0,		yyvstop+151,
yycrank+-14,	yysvec+51,	yyvstop+154,
yycrank+0,	yysvec+10,	yyvstop+156,
yycrank+139,	yysvec+10,	yyvstop+159,
yycrank+139,	yysvec+10,	yyvstop+161,
yycrank+160,	yysvec+10,	yyvstop+163,
yycrank+154,	yysvec+10,	yyvstop+165,
yycrank+161,	yysvec+10,	yyvstop+167,
yycrank+334,	0,		0,	
yycrank+164,	yysvec+10,	yyvstop+169,
yycrank+173,	yysvec+10,	yyvstop+171,
yycrank+295,	yysvec+10,	yyvstop+173,
yycrank+183,	yysvec+10,	yyvstop+176,
yycrank+185,	yysvec+10,	yyvstop+178,
yycrank+188,	yysvec+10,	yyvstop+180,
yycrank+186,	yysvec+10,	yyvstop+182,
yycrank+176,	yysvec+10,	yyvstop+184,
yycrank+188,	yysvec+10,	yyvstop+186,
yycrank+191,	yysvec+10,	yyvstop+188,
yycrank+180,	yysvec+10,	yyvstop+190,
yycrank+53,	yysvec+10,	yyvstop+192,
yycrank+250,	yysvec+73,	yyvstop+195,
yycrank+258,	yysvec+73,	yyvstop+198,
yycrank+365,	yysvec+73,	yyvstop+201,
yycrank+368,	yysvec+73,	yyvstop+204,
yycrank+374,	yysvec+73,	yyvstop+207,
yycrank+387,	yysvec+73,	yyvstop+210,
yycrank+-458,	0,		yyvstop+213,
yycrank+50,	yysvec+10,	yyvstop+216,
yycrank+0,	yysvec+10,	yyvstop+219,
yycrank+197,	yysvec+10,	yyvstop+222,
yycrank+0,	yysvec+10,	yyvstop+224,
yycrank+199,	yysvec+10,	yyvstop+227,
yycrank+190,	0,		0,	
yycrank+198,	0,		0,	
yycrank+193,	yysvec+10,	yyvstop+229,
yycrank+195,	yysvec+10,	yyvstop+231,
yycrank+423,	0,		0,	
yycrank+198,	yysvec+10,	yyvstop+233,
yycrank+194,	yysvec+10,	yyvstop+235,
yycrank+198,	yysvec+10,	yyvstop+237,
yycrank+218,	yysvec+10,	yyvstop+239,
yycrank+220,	yysvec+10,	yyvstop+241,
yycrank+452,	yysvec+10,	yyvstop+243,
yycrank+221,	yysvec+10,	yyvstop+246,
yycrank+212,	yysvec+10,	yyvstop+248,
yycrank+233,	yysvec+10,	yyvstop+250,
yycrank+454,	0,		0,	
yycrank+463,	0,		0,	
yycrank+431,	yysvec+73,	yyvstop+252,
yycrank+437,	yysvec+73,	yyvstop+255,
yycrank+446,	yysvec+73,	yyvstop+258,
yycrank+461,	yysvec+73,	yyvstop+261,
yycrank+467,	yysvec+73,	yyvstop+264,
yycrank+0,	0,		yyvstop+267,
yycrank+336,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+270,
yycrank+0,	yysvec+10,	yyvstop+273,
yycrank+222,	0,		0,	
yycrank+223,	0,		0,	
yycrank+234,	yysvec+10,	yyvstop+276,
yycrank+224,	yysvec+10,	yyvstop+278,
yycrank+229,	0,		0,	
yycrank+224,	yysvec+10,	yyvstop+280,
yycrank+236,	yysvec+10,	yyvstop+282,
yycrank+248,	yysvec+10,	yyvstop+284,
yycrank+234,	yysvec+10,	yyvstop+286,
yycrank+0,	yysvec+10,	yyvstop+288,
yycrank+466,	0,		0,	
yycrank+241,	yysvec+10,	yyvstop+291,
yycrank+0,	yysvec+10,	yyvstop+293,
yycrank+251,	yysvec+10,	yyvstop+296,
yycrank+245,	yysvec+10,	yyvstop+298,
yycrank+537,	0,		0,	
yycrank+489,	yysvec+73,	yyvstop+300,
yycrank+594,	yysvec+73,	yyvstop+303,
yycrank+492,	yysvec+73,	yyvstop+306,
yycrank+483,	yysvec+73,	yyvstop+309,
yycrank+608,	yysvec+73,	yyvstop+312,
yycrank+238,	0,		0,	
yycrank+264,	0,		0,	
yycrank+256,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+315,
yycrank+255,	yysvec+10,	yyvstop+318,
yycrank+256,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+320,
yycrank+285,	yysvec+10,	yyvstop+323,
yycrank+0,	yysvec+10,	yyvstop+325,
yycrank+300,	yysvec+10,	yyvstop+328,
yycrank+292,	0,		0,	
yycrank+288,	yysvec+10,	yyvstop+330,
yycrank+335,	yysvec+10,	yyvstop+332,
yycrank+0,	yysvec+10,	yyvstop+334,
yycrank+652,	0,		0,	
yycrank+542,	0,		0,	
yycrank+626,	yysvec+73,	yyvstop+337,
yycrank+629,	yysvec+73,	yyvstop+340,
yycrank+630,	yysvec+73,	yyvstop+343,
yycrank+651,	yysvec+73,	yyvstop+346,
yycrank+652,	yysvec+73,	yyvstop+349,
yycrank+665,	yysvec+73,	yyvstop+352,
yycrank+666,	yysvec+73,	yyvstop+355,
yycrank+674,	yysvec+73,	yyvstop+358,
yycrank+690,	yysvec+73,	yyvstop+361,
yycrank+705,	yysvec+73,	yyvstop+364,
yycrank+706,	yysvec+73,	yyvstop+367,
yycrank+719,	yysvec+73,	yyvstop+370,
yycrank+727,	yysvec+73,	yyvstop+373,
yycrank+330,	0,		0,	
yycrank+0,	0,		yyvstop+376,
yycrank+328,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+378,
yycrank+0,	0,		yyvstop+381,
yycrank+0,	yysvec+10,	yyvstop+383,
yycrank+350,	yysvec+10,	yyvstop+386,
yycrank+340,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+388,
yycrank+0,	yysvec+10,	yyvstop+391,
yycrank+673,	0,		0,	
yycrank+728,	yysvec+73,	yyvstop+394,
yycrank+743,	yysvec+73,	yyvstop+397,
yycrank+746,	yysvec+73,	yyvstop+400,
yycrank+749,	yysvec+73,	yyvstop+403,
yycrank+765,	yysvec+73,	yyvstop+406,
yycrank+768,	yysvec+73,	yyvstop+409,
yycrank+783,	yysvec+73,	yyvstop+412,
yycrank+789,	yysvec+73,	yyvstop+415,
yycrank+802,	yysvec+73,	yyvstop+418,
yycrank+805,	yysvec+73,	yyvstop+421,
yycrank+816,	yysvec+73,	yyvstop+424,
yycrank+824,	yysvec+73,	yyvstop+427,
yycrank+825,	yysvec+73,	yyvstop+430,
yycrank+895,	0,		0,	
yycrank+0,	0,		yyvstop+433,
yycrank+0,	yysvec+10,	yyvstop+435,
yycrank+0,	0,		yyvstop+438,
yycrank+358,	0,		0,	
yycrank+843,	yysvec+73,	yyvstop+440,
yycrank+952,	yysvec+73,	yyvstop+443,
yycrank+857,	yysvec+73,	yyvstop+446,
yycrank+953,	yysvec+73,	yyvstop+449,
yycrank+956,	yysvec+73,	yyvstop+452,
yycrank+967,	yysvec+73,	yyvstop+455,
yycrank+971,	yysvec+73,	yyvstop+458,
yycrank+982,	yysvec+73,	yyvstop+461,
yycrank+986,	yysvec+73,	yyvstop+464,
yycrank+1004,	yysvec+73,	yyvstop+467,
yycrank+1007,	yysvec+73,	yyvstop+470,
yycrank+1010,	yysvec+73,	yyvstop+473,
yycrank+1023,	yysvec+73,	yyvstop+476,
yycrank+701,	0,		0,	
yycrank+0,	0,		yyvstop+479,
yycrank+366,	0,		0,	
yycrank+1026,	yysvec+73,	yyvstop+481,
yycrank+1041,	yysvec+73,	yyvstop+484,
yycrank+1045,	yysvec+73,	yyvstop+487,
yycrank+1054,	yysvec+73,	yyvstop+490,
yycrank+1060,	yysvec+73,	yyvstop+493,
yycrank+1063,	yysvec+73,	yyvstop+496,
yycrank+1090,	yysvec+73,	yyvstop+499,
yycrank+1076,	yysvec+73,	yyvstop+502,
yycrank+1082,	yysvec+73,	yyvstop+505,
yycrank+1096,	yysvec+73,	yyvstop+508,
yycrank+1104,	yysvec+73,	yyvstop+511,
yycrank+1118,	yysvec+73,	yyvstop+514,
yycrank+1119,	yysvec+73,	yyvstop+517,
yycrank+374,	0,		0,	
yycrank+1134,	yysvec+73,	yyvstop+520,
yycrank+1133,	yysvec+73,	yyvstop+523,
yycrank+1154,	yysvec+73,	yyvstop+527,
yycrank+1163,	yysvec+73,	yyvstop+530,
yycrank+1167,	yysvec+73,	yyvstop+533,
yycrank+1178,	yysvec+73,	yyvstop+536,
yycrank+447,	0,		0,	
yycrank+-1253,	0,		0,	
yycrank+1182,	yysvec+73,	yyvstop+539,
yycrank+1201,	yysvec+73,	yyvstop+542,
yycrank+1222,	yysvec+73,	yyvstop+546,
yycrank+1226,	yysvec+73,	yyvstop+549,
yycrank+1237,	yysvec+73,	yyvstop+552,
yycrank+1241,	yysvec+73,	yyvstop+555,
yycrank+732,	0,		0,	
yycrank+1250,	yysvec+73,	yyvstop+558,
yycrank+1259,	yysvec+73,	yyvstop+561,
yycrank+1268,	yysvec+73,	yyvstop+564,
yycrank+1281,	yysvec+73,	yyvstop+567,
yycrank+1290,	yysvec+73,	yyvstop+570,
yycrank+0,	0,		yyvstop+573,
yycrank+1294,	yysvec+73,	yyvstop+575,
yycrank+1303,	yysvec+73,	yyvstop+578,
yycrank+1312,	yysvec+73,	yyvstop+581,
yycrank+1316,	yysvec+73,	yyvstop+584,
yycrank+1346,	yysvec+73,	yyvstop+587,
yycrank+1248,	0,		0,	
yycrank+1337,	yysvec+73,	yyvstop+590,
yycrank+1338,	yysvec+73,	yyvstop+593,
yycrank+1359,	yysvec+73,	yyvstop+596,
yycrank+1360,	yysvec+73,	yyvstop+599,
yycrank+1375,	yysvec+73,	yyvstop+602,
yycrank+1381,	yysvec+73,	yyvstop+605,
yycrank+1389,	yysvec+73,	yyvstop+608,
yycrank+1395,	yysvec+73,	yyvstop+611,
yycrank+1404,	yysvec+73,	yyvstop+614,
yycrank+1417,	yysvec+73,	yyvstop+617,
yycrank+1425,	yysvec+73,	yyvstop+620,
yycrank+1439,	yysvec+73,	yyvstop+623,
yycrank+1426,	yysvec+73,	yyvstop+626,
yycrank+0,	0,		yyvstop+629,
yycrank+1447,	yysvec+73,	yyvstop+631,
yycrank+1448,	yysvec+73,	yyvstop+634,
yycrank+1461,	yysvec+73,	yyvstop+637,
yycrank+1462,	yysvec+73,	yyvstop+640,
yycrank+1470,	yysvec+73,	yyvstop+643,
yycrank+1484,	yysvec+73,	yyvstop+646,
yycrank+1493,	yysvec+73,	yyvstop+649,
yycrank+1499,	yysvec+73,	yyvstop+652,
yycrank+1502,	yysvec+73,	yyvstop+655,
yycrank+1508,	yysvec+73,	yyvstop+658,
yycrank+1524,	yysvec+73,	yyvstop+661,
yycrank+1533,	yysvec+73,	yyvstop+664,
yycrank+1537,	yysvec+73,	yyvstop+667,
yycrank+1546,	yysvec+73,	yyvstop+670,
yycrank+1560,	yysvec+73,	yyvstop+674,
yycrank+1561,	yysvec+73,	yyvstop+677,
yycrank+1569,	yysvec+73,	yyvstop+680,
yycrank+1575,	yysvec+73,	yyvstop+683,
yycrank+1583,	yysvec+73,	yyvstop+686,
yycrank+1584,	yysvec+73,	yyvstop+689,
yycrank+1599,	yysvec+73,	yyvstop+692,
yycrank+1607,	yysvec+73,	yyvstop+695,
yycrank+1613,	yysvec+73,	yyvstop+698,
yycrank+1631,	yysvec+73,	yyvstop+701,
yycrank+1635,	yysvec+73,	yyvstop+704,
yycrank+1622,	yysvec+73,	yyvstop+707,
yycrank+1644,	yysvec+73,	yyvstop+710,
yycrank+1660,	yysvec+73,	yyvstop+713,
yycrank+1666,	yysvec+73,	yyvstop+716,
yycrank+1669,	yysvec+73,	yyvstop+719,
yycrank+1675,	yysvec+73,	yyvstop+722,
yycrank+1690,	yysvec+73,	yyvstop+726,
yycrank+1691,	yysvec+73,	yyvstop+729,
yycrank+1709,	yysvec+73,	yyvstop+732,
yycrank+1710,	yysvec+73,	yyvstop+735,
yycrank+1713,	yysvec+73,	yyvstop+738,
yycrank+1724,	yysvec+73,	yyvstop+741,
yycrank+1732,	yysvec+73,	yyvstop+744,
yycrank+1733,	yysvec+73,	yyvstop+747,
yycrank+1748,	yysvec+73,	yyvstop+751,
yycrank+1752,	yysvec+73,	yyvstop+755,
yycrank+1763,	yysvec+73,	yyvstop+759,
yycrank+1766,	yysvec+73,	yyvstop+762,
yycrank+1779,	yysvec+73,	yyvstop+766,
yycrank+1782,	yysvec+73,	yyvstop+769,
yycrank+1785,	yysvec+73,	yyvstop+773,
yycrank+1788,	yysvec+73,	yyvstop+776,
yycrank+1801,	yysvec+73,	yyvstop+779,
yycrank+1807,	yysvec+73,	yyvstop+782,
yycrank+1810,	yysvec+73,	yyvstop+786,
yycrank+1831,	yysvec+73,	yyvstop+789,
yycrank+1832,	yysvec+73,	yyvstop+792,
yycrank+1835,	yysvec+73,	yyvstop+795,
yycrank+1851,	yysvec+73,	yyvstop+799,
yycrank+1854,	yysvec+73,	yyvstop+803,
yycrank+1869,	yysvec+73,	yyvstop+807,
yycrank+1870,	yysvec+73,	yyvstop+810,
yycrank+1873,	yysvec+73,	yyvstop+813,
yycrank+1884,	yysvec+73,	yyvstop+816,
yycrank+1888,	yysvec+73,	yyvstop+819,
yycrank+1892,	yysvec+73,	yyvstop+822,
yycrank+1906,	yysvec+73,	yyvstop+825,
yycrank+1907,	yysvec+73,	yyvstop+828,
yycrank+1910,	yysvec+73,	yyvstop+831,
yycrank+1926,	yysvec+73,	yyvstop+834,
yycrank+1930,	yysvec+73,	yyvstop+837,
yycrank+1934,	yysvec+73,	yyvstop+841,
yycrank+1950,	yysvec+73,	yyvstop+844,
yycrank+1959,	yysvec+73,	yyvstop+847,
yycrank+1965,	yysvec+73,	yyvstop+850,
yycrank+1974,	yysvec+73,	yyvstop+853,
yycrank+1980,	yysvec+73,	yyvstop+856,
yycrank+1988,	yysvec+73,	yyvstop+859,
yycrank+1994,	yysvec+73,	yyvstop+862,
yycrank+2003,	yysvec+73,	yyvstop+865,
yycrank+2009,	yysvec+73,	yyvstop+868,
yycrank+2012,	yysvec+73,	yyvstop+871,
yycrank+2018,	yysvec+73,	yyvstop+875,
0,	0,	0};
struct yywork *yytop = yycrank+2120;
struct yysvf *yybgin = yysvec+1;
char yymatch[] = {
  0,   1,   1,   1,   1,   1,   1,   1, 
  1,   9,  10,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
 32,   1,  34,   1,   1,   1,  38,   1, 
 38,  41,  38,   1,  38,   1,  38,   1, 
 48,  49,  49,  49,  49,  49,  49,  49, 
 49,  49,  38,  38,   1,  38,   1,   1, 
  1,  65,  65,  65,  65,  65,  65,  65, 
 65,  65,  65,  65,  65,  65,  65,  65, 
 65,  65,  65,  65,  65,  65,  65,  65, 
 65,  65,  65,   1,   1,   1,   1,  65, 
  1,  65,  65,  65,  65,  65,  65,  65, 
 65,  65,  65,  65,  65,  65,  65,  65, 
 65,  65,  65,  65,  65,  65,  65,  65, 
 65,  65,  65,  38,   1,  38,  38,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
0};
char yyextra[] = {
0,0,1,1,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,1,
0,0,0,0,0,0,0,0,
0};
/*	Copyright (c) 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)ncform	6.8	95/02/11 SMI"

int yylineno =1;
# define YYU(x) x
# define NLSTATE yyprevious=YYNEWLINE
struct yysvf *yylstate [YYLMAX], **yylsp, **yyolsp;
char yysbuf[YYLMAX];
char *yysptr = yysbuf;
int *yyfnd;
extern struct yysvf *yyestate;
int yyprevious = YYNEWLINE;
#if defined(__cplusplus) || defined(__STDC__)
int yylook(void)
#else
yylook()
#endif
{
	register struct yysvf *yystate, **lsp;
	register struct yywork *yyt;
	struct yysvf *yyz;
	int yych, yyfirst;
	struct yywork *yyr;
# ifdef LEXDEBUG
	int debug;
# endif
	char *yylastch;
	/* start off machines */
# ifdef LEXDEBUG
	debug = 0;
# endif
	yyfirst=1;
	if (!yymorfg)
		yylastch = yytext;
	else {
		yymorfg=0;
		yylastch = yytext+yyleng;
		}
	for(;;){
		lsp = yylstate;
		yyestate = yystate = yybgin;
		if (yyprevious==YYNEWLINE) yystate++;
		for (;;){
# ifdef LEXDEBUG
			if(debug)fprintf(yyout,"state %d\n",yystate-yysvec-1);
# endif
			yyt = yystate->yystoff;
			if(yyt == yycrank && !yyfirst){  /* may not be any transitions */
				yyz = yystate->yyother;
				if(yyz == 0)break;
				if(yyz->yystoff == yycrank)break;
				}
#ifndef __cplusplus
			*yylastch++ = yych = input();
#else
			*yylastch++ = yych = lex_input();
#endif
			if(yylastch > &yytext[YYLMAX]) {
				fprintf(yyout,"Input string too long, limit %d\n",YYLMAX);
				exit(1);
			}
			yyfirst=0;
		tryagain:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"char ");
				allprint(yych);
				putchar('\n');
				}
# endif
			yyr = yyt;
			if ( (int)yyt > (int)yycrank){
				yyt = yyr + yych;
				if (yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					if(lsp > &yylstate[YYLMAX]) {
						fprintf(yyout,"Input string too long, limit %d\n",YYLMAX);
						exit(1);
					}
					goto contin;
					}
				}
# ifdef YYOPTIM
			else if((int)yyt < (int)yycrank) {		/* r < yycrank */
				yyt = yyr = yycrank+(yycrank-yyt);
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"compressed state\n");
# endif
				yyt = yyt + yych;
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					if(lsp > &yylstate[YYLMAX]) {
						fprintf(yyout,"Input string too long, limit %d\n",YYLMAX);
						exit(1);
					}
					goto contin;
					}
				yyt = yyr + YYU(yymatch[yych]);
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"try fall back character ");
					allprint(YYU(yymatch[yych]));
					putchar('\n');
					}
# endif
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transition */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					if(lsp > &yylstate[YYLMAX]) {
						fprintf(yyout,"Input string too long, limit %d\n",YYLMAX);
						exit(1);
					}
					goto contin;
					}
				}
			if ((yystate = yystate->yyother) && (yyt= yystate->yystoff) != yycrank){
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"fall back to state %d\n",yystate-yysvec-1);
# endif
				goto tryagain;
				}
# endif
			else
				{unput(*--yylastch);break;}
		contin:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"state %d char ",yystate-yysvec-1);
				allprint(yych);
				putchar('\n');
				}
# endif
			;
			}
# ifdef LEXDEBUG
		if(debug){
			fprintf(yyout,"stopped at %d with ",*(lsp-1)-yysvec-1);
			allprint(yych);
			putchar('\n');
			}
# endif
		while (lsp-- > yylstate){
			*yylastch-- = 0;
			if (*lsp != 0 && (yyfnd= (*lsp)->yystops) && *yyfnd > 0){
				yyolsp = lsp;
				if(yyextra[*yyfnd]){		/* must backup */
					while(yyback((*lsp)->yystops,-*yyfnd) != 1 && lsp > yylstate){
						lsp--;
						unput(*yylastch--);
						}
					}
				yyprevious = YYU(*yylastch);
				yylsp = lsp;
				yyleng = yylastch-yytext+1;
				yytext[yyleng] = 0;
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"\nmatch ");
					sprint(yytext);
					fprintf(yyout," action %d\n",*yyfnd);
					}
# endif
				return(*yyfnd++);
				}
			unput(*yylastch);
			}
		if (yytext[0] == 0  /* && feof(yyin) */)
			{
			yysptr=yysbuf;
			return(0);
			}
#ifndef __cplusplus
		yyprevious = yytext[0] = input();
		if (yyprevious>0)
			output(yyprevious);
#else
		yyprevious = yytext[0] = lex_input();
		if (yyprevious>0)
			lex_output(yyprevious);
#endif
		yylastch=yytext;
# ifdef LEXDEBUG
		if(debug)putchar('\n');
# endif
		}
	}
#if defined(__cplusplus) || defined(__STDC__)
int yyback(int *p, int m)
#else
yyback(p, m)
	int *p;
#endif
{
	if (p==0) return(0);
	while (*p) {
		if (*p++ == m)
			return(1);
	}
	return(0);
}
	/* the following are only used in the lex library */
#if defined(__cplusplus) || defined(__STDC__)
int yyinput(void)
#else
yyinput()
#endif
{
#ifndef __cplusplus
	return(input());
#else
	return(lex_input());
#endif
	}
#if defined(__cplusplus) || defined(__STDC__)
void yyoutput(int c)
#else
yyoutput(c)
  int c; 
#endif
{
#ifndef __cplusplus
	output(c);
#else
	lex_output(c);
#endif
	}
#if defined(__cplusplus) || defined(__STDC__)
void yyunput(int c)
#else
yyunput(c)
   int c; 
#endif
{
	unput(c);
	}
