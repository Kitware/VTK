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

# line 5 "concrete.l"
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

# line 14 "concrete.l"

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

# line 31 "concrete.l"
          ;
break;
case 4:

# line 33 "concrete.l"
;
break;
case 5:

# line 35 "concrete.l"
;
break;
case 6:

# line 37 "concrete.l"
 
  { sscanf(yytext+1,"%d",&yylval.integer); return(ARRAY_NUM);}
break;
case 7:

# line 41 "concrete.l"
 return(CLASS_REF);
break;
case 8:

# line 43 "concrete.l"
return(VAR_FUNCTION);
break;
case 9:

# line 45 "concrete.l"
return(SHORT);
break;
case 10:

# line 46 "concrete.l"
 return(LONG);
break;
case 11:

# line 47 "concrete.l"
return(SHORT);
break;
case 12:

# line 48 "concrete.l"
 return(LONG);
break;
case 13:

# line 50 "concrete.l"
	return(CLASS);
break;
case 14:

# line 51 "concrete.l"
 	return(PUBLIC);
break;
case 15:

# line 52 "concrete.l"
 	return(PRIVATE);
break;
case 16:

# line 53 "concrete.l"
	return(PROTECTED);
break;
case 17:

# line 54 "concrete.l"
	        return(INT);
break;
case 18:

# line 55 "concrete.l"
	return(FLOAT);
break;
case 19:

# line 56 "concrete.l"
	return(SHORT);
break;
case 20:

# line 57 "concrete.l"
 	return(LONG);
break;
case 21:

# line 58 "concrete.l"
	return(DOUBLE);
break;
case 22:

# line 59 "concrete.l"
	        return(VOID);
break;
case 23:

# line 60 "concrete.l"
	        return(CHAR);
break;
case 24:

# line 61 "concrete.l"
      return(VIRTUAL);
break;
case 25:

# line 62 "concrete.l"
        return(CONST);
break;
case 26:

# line 63 "concrete.l"
     return(OPERATOR);
break;
case 27:

# line 64 "concrete.l"
     return(UNSIGNED);
break;
case 28:

# line 65 "concrete.l"
       return(FRIEND);
break;
case 29:

# line 66 "concrete.l"
       return(STATIC);
break;
case 30:

# line 68 "concrete.l"
         return(SetMacro);
break;
case 31:

# line 69 "concrete.l"
         return(GetMacro);
break;
case 32:

# line 70 "concrete.l"
   return(SetStringMacro);
break;
case 33:

# line 71 "concrete.l"
   return(GetStringMacro);
break;
case 34:

# line 72 "concrete.l"
    return(SetClampMacro);
break;
case 35:

# line 73 "concrete.l"
   return(SetObjectMacro);
break;
case 36:

# line 74 "concrete.l"
return(SetReferenceCountedObjectMacro);
break;
case 37:

# line 75 "concrete.l"
   return(GetObjectMacro);
break;
case 38:

# line 76 "concrete.l"
     return(BooleanMacro);
break;
case 39:

# line 77 "concrete.l"
  return(SetVector2Macro);
break;
case 40:

# line 78 "concrete.l"
  return(SetVector3Macro);
break;
case 41:

# line 79 "concrete.l"
  return(SetVector4Macro);
break;
case 42:

# line 80 "concrete.l"
  return(GetVector2Macro);
break;
case 43:

# line 81 "concrete.l"
  return(GetVector3Macro);
break;
case 44:

# line 82 "concrete.l"
  return(GetVector4Macro);
break;
case 45:

# line 83 "concrete.l"
   return(SetVectorMacro);
break;
case 46:

# line 84 "concrete.l"
   return(GetVectorMacro);
break;
case 47:

# line 86 "concrete.l"
	{ sscanf(yytext,"%d",&yylval.integer); return(NUM);}
break;
case 48:

# line 88 "concrete.l"
{
		yylval.str =  strdup(yytext + 1);
		yylval.str[strlen(yytext)-2] = '\0';
		return(STRING);
		}
break;
case 49:

# line 94 "concrete.l"
    ;
break;
case 50:

# line 97 "concrete.l"
 { yylval.str = (char *)malloc(yyleng + 1);
                     memcpy(yylval.str,yytext,yyleng);
                     yylval.str[yyleng] = '\0';
                     return(VTK_ID);
                    }
break;
case 51:

# line 103 "concrete.l"
 { yylval.str = (char *)malloc(yyleng + 1);
                          memcpy(yylval.str,yytext,yyleng);
                          yylval.str[yyleng] = '\0';
                          return(ID);
                        }
break;
case 52:

# line 109 "concrete.l"
;
break;
case 53:

# line 111 "concrete.l"
return(yytext[0]);
break;
case 54:

# line 112 "concrete.l"
return(yytext[0]);
break;
case 55:

# line 115 "concrete.l"
return(yytext[0]);
break;
case 56:

# line 117 "concrete.l"
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

56,
0,

52,
56,
0,

52,
0,

56,
0,

55,
56,
0,

56,
0,

47,
56,
0,

51,
56,
0,

51,
56,
0,

53,
56,
0,

54,
56,
0,

51,
56,
0,

51,
56,
0,

51,
56,
0,

51,
56,
0,

51,
56,
0,

51,
56,
0,

51,
56,
0,

51,
56,
0,

51,
56,
0,

51,
56,
0,

56,
-49,
0,

1,
0,

-3,
0,

47,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

-49,
0,

49,
0,

48,
0,

-3,
0,

3,
0,

-3,
0,

51,
0,

6,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

17,
51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

50,
51,
0,

-3,
0,

51,
0,

23,
51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

20,
51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

22,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

-2,
-3,
0,

51,
0,

13,
51,
0,

25,
51,
0,

51,
0,

18,
51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

19,
51,
0,

51,
0,

51,
0,

51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

2,
3,
0,

51,
0,

21,
51,
0,

28,
51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

51,
0,

14,
51,
0,

51,
0,

29,
51,
0,

51,
0,

51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

51,
0,

12,
51,
0,

51,
0,

10,
51,
0,

51,
0,

15,
51,
0,

51,
0,

51,
0,

51,
0,

24,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

51,
0,

12,
0,

11,
51,
0,

10,
0,

26,
51,
0,

51,
0,

9,
51,
0,

27,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

51,
0,

11,
0,

16,
51,
0,

9,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

4,
51,
0,

7,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

31,
50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

30,
50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

5,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

8,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

38,
50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

34,
50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

37,
50,
51,
0,

33,
50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

46,
50,
51,
0,

35,
50,
51,
0,

50,
51,
0,

32,
50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

45,
50,
51,
0,

42,
50,
51,
0,

43,
50,
51,
0,

44,
50,
51,
0,

50,
51,
0,

39,
50,
51,
0,

40,
50,
51,
0,

41,
50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

50,
51,
0,

36,
50,
51,
0,
0};
# define YYTYPE int
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	1,3,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	1,4,	1,5,	
0,0,	4,5,	4,5,	0,0,	
0,0,	0,0,	0,0,	0,0,	
6,25,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
6,25,	6,25,	0,0,	0,0,	
0,0,	1,4,	0,0,	1,6,	
4,5,	25,52,	0,0,	1,7,	
0,0,	0,0,	1,7,	0,0,	
0,0,	0,0,	0,0,	8,26,	
1,8,	1,9,	1,9,	6,25,	
8,27,	6,0,	0,0,	0,0,	
0,0,	6,25,	0,0,	2,24,	
6,25,	0,0,	0,0,	0,0,	
0,0,	0,0,	1,10,	6,25,	
6,25,	0,0,	0,0,	2,8,	
9,28,	9,28,	9,28,	9,28,	
9,28,	9,28,	9,28,	9,28,	
9,28,	9,28,	78,104,	105,132,	
6,25,	11,30,	0,0,	1,11,	
30,56,	0,0,	0,0,	0,0,	
1,12,	0,0,	1,13,	0,0,	
0,0,	55,77,	0,0,	0,0,	
1,14,	1,15,	77,103,	1,16,	
0,0,	0,0,	1,17,	14,33,	
56,78,	1,18,	2,11,	14,34,	
1,19,	1,20,	14,35,	2,12,	
1,21,	2,13,	1,22,	1,23,	
15,36,	16,37,	17,39,	2,14,	
2,15,	18,40,	2,16,	16,38,	
19,41,	2,17,	20,42,	22,46,	
2,18,	20,43,	33,59,	2,19,	
2,20,	34,60,	35,61,	2,21,	
36,62,	2,22,	2,23,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	37,63,	38,64,	39,65,	
40,66,	41,67,	43,70,	44,71,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	45,72,	46,73,	
47,74,	48,75,	10,29,	49,76,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	10,29,	10,29,	
10,29,	10,29,	12,31,	12,31,	
21,44,	42,68,	23,47,	57,57,	
57,57,	59,79,	60,80,	42,69,	
23,48,	61,81,	62,82,	63,83,	
21,45,	23,49,	64,84,	66,88,	
24,50,	67,89,	68,90,	69,91,	
70,92,	12,31,	71,93,	72,94,	
24,50,	24,51,	57,57,	73,95,	
27,53,	65,85,	65,85,	32,57,	
32,57,	74,96,	75,97,	80,105,	
27,53,	27,54,	12,32,	12,32,	
12,32,	12,32,	12,32,	12,32,	
12,32,	12,32,	12,32,	24,50,	
81,106,	24,50,	82,107,	83,108,	
65,85,	24,50,	32,57,	84,109,	
24,50,	86,112,	53,53,	27,53,	
87,113,	27,53,	89,116,	24,50,	
24,50,	27,53,	53,53,	53,54,	
27,53,	90,117,	32,32,	91,118,	
92,119,	93,120,	94,121,	27,53,	
27,53,	95,122,	96,123,	104,131,	
24,50,	97,124,	97,124,	107,133,	
88,114,	88,114,	109,134,	57,58,	
110,135,	53,53,	111,136,	53,53,	
27,53,	27,55,	112,137,	53,53,	
98,98,	100,98,	53,53,	113,138,	
115,140,	98,98,	100,98,	116,141,	
97,124,	53,53,	53,53,	88,114,	
98,98,	100,98,	117,142,	118,143,	
97,125,	98,98,	100,98,	32,58,	
119,144,	121,147,	122,148,	123,149,	
131,155,	135,157,	53,53,	85,85,	
85,85,	114,114,	114,114,	136,158,	
65,86,	132,132,	132,132,	137,159,	
100,127,	138,160,	139,161,	65,87,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	85,85,	140,162,	
114,114,	141,163,	142,164,	143,165,	
132,132,	76,98,	76,99,	76,98,	
76,98,	76,98,	76,98,	76,100,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,101,	76,98,	
76,98,	76,98,	76,98,	76,102,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	146,167,	
88,115,	148,168,	149,169,	76,98,	
155,184,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	99,98,	
156,185,	101,98,	102,98,	157,186,	
99,98,	158,187,	101,98,	102,98,	
160,188,	114,139,	85,110,	99,98,	
103,103,	101,98,	102,98,	161,189,	
99,98,	85,111,	101,98,	102,98,	
103,103,	103,130,	163,190,	165,191,	
120,145,	120,145,	132,156,	124,124,	
124,124,	166,192,	125,125,	125,125,	
145,145,	145,145,	167,193,	168,194,	
184,208,	102,129,	185,209,	187,210,	
191,211,	192,212,	208,226,	103,103,	
99,126,	103,103,	101,128,	120,145,	
213,229,	103,103,	124,124,	229,242,	
103,103,	125,125,	242,256,	145,145,	
126,98,	127,98,	124,125,	103,103,	
103,103,	126,98,	127,98,	125,150,	
0,0,	128,98,	129,98,	151,98,	
126,98,	127,98,	128,98,	129,98,	
151,98,	126,98,	127,98,	152,98,	
103,103,	128,98,	129,98,	151,98,	
152,98,	248,248,	128,98,	129,98,	
151,98,	0,0,	152,173,	152,98,	
152,174,	248,249,	150,170,	150,170,	
152,175,	153,98,	0,0,	152,176,	
171,171,	171,171,	153,98,	0,0,	
0,0,	126,151,	195,195,	195,195,	
0,0,	153,98,	172,98,	127,152,	
0,0,	151,172,	153,98,	172,98,	
153,177,	150,170,	0,0,	128,153,	
129,154,	0,0,	172,98,	171,171,	
120,146,	0,0,	150,171,	172,98,	
0,0,	195,195,	0,0,	171,195,	
145,166,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	0,0,	
0,0,	172,196,	0,0,	0,0,	
0,0,	0,0,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
0,0,	0,0,	0,0,	0,0,	
150,150,	0,0,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
150,150,	150,150,	150,150,	150,150,	
154,98,	154,178,	170,170,	170,170,	
0,0,	154,98,	0,0,	195,213,	
0,0,	173,98,	174,98,	154,179,	
154,98,	154,180,	173,98,	174,98,	
154,181,	154,182,	175,98,	176,98,	
154,183,	173,98,	174,98,	175,98,	
176,98,	170,170,	173,98,	174,98,	
256,256,	256,256,	175,98,	176,98,	
177,98,	178,98,	170,171,	175,98,	
176,98,	177,98,	178,98,	0,0,	
173,197,	179,98,	174,198,	181,98,	
177,98,	178,98,	179,98,	180,98,	
181,98,	177,98,	178,98,	256,256,	
180,98,	179,98,	176,200,	181,98,	
0,0,	182,98,	179,98,	180,98,	
181,98,	256,267,	182,98,	0,0,	
180,98,	183,98,	0,0,	0,0,	
175,199,	182,98,	183,98,	196,98,	
179,203,	0,0,	182,98,	178,202,	
196,98,	183,98,	181,205,	180,204,	
197,98,	177,201,	183,98,	196,98,	
198,98,	197,98,	199,98,	0,0,	
196,98,	198,98,	0,0,	199,98,	
197,98,	0,0,	200,98,	0,0,	
198,98,	197,98,	199,98,	200,98,	
183,207,	198,98,	196,214,	199,98,	
201,98,	202,98,	200,98,	182,206,	
0,0,	201,98,	202,98,	200,98,	
0,0,	197,215,	203,98,	0,0,	
201,98,	202,98,	204,98,	203,98,	
205,98,	201,98,	202,98,	204,98,	
198,216,	205,98,	203,98,	200,218,	
206,98,	0,0,	204,98,	203,98,	
205,98,	206,98,	199,217,	204,98,	
202,220,	205,98,	207,98,	201,219,	
206,98,	209,227,	209,227,	207,98,	
0,0,	206,98,	0,0,	203,221,	
214,98,	0,0,	207,98,	0,0,	
0,0,	214,98,	0,0,	207,98,	
205,223,	230,98,	204,222,	0,0,	
214,98,	216,98,	230,98,	0,0,	
209,227,	214,98,	216,98,	0,0,	
230,243,	230,98,	0,0,	207,225,	
0,0,	216,98,	230,98,	0,0,	
206,224,	0,0,	216,98,	0,0,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	0,0,	209,228,	
214,230,	0,0,	0,0,	0,0,	
216,232,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	0,0,	
0,0,	0,0,	0,0,	209,209,	
0,0,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	209,209,	
209,209,	209,209,	209,209,	215,98,	
217,98,	218,98,	219,98,	220,98,	
215,98,	217,98,	218,98,	219,98,	
220,98,	227,227,	227,227,	215,98,	
217,98,	218,98,	219,98,	220,98,	
215,98,	217,98,	218,98,	219,98,	
220,98,	221,98,	0,0,	222,98,	
0,0,	223,98,	221,98,	0,0,	
222,98,	0,0,	223,98,	0,0,	
227,227,	221,98,	0,0,	222,98,	
219,235,	223,98,	221,98,	217,233,	
222,98,	0,0,	223,98,	0,0,	
224,98,	225,98,	220,236,	215,231,	
0,0,	224,98,	225,98,	218,234,	
0,0,	0,0,	0,0,	0,0,	
224,98,	225,98,	222,238,	227,228,	
223,239,	224,98,	225,98,	231,98,	
0,0,	232,98,	235,248,	233,98,	
231,98,	221,237,	232,98,	0,0,	
233,98,	234,98,	235,249,	231,98,	
236,98,	232,98,	234,98,	233,98,	
231,98,	236,98,	232,98,	224,240,	
233,98,	234,98,	0,0,	237,98,	
236,98,	239,98,	234,98,	0,0,	
237,98,	236,98,	239,98,	225,241,	
0,0,	238,98,	232,245,	237,98,	
235,98,	239,98,	238,98,	0,0,	
237,98,	235,98,	239,98,	0,0,	
231,244,	238,98,	0,0,	233,246,	
235,98,	0,0,	238,98,	0,0,	
240,98,	235,98,	234,247,	241,98,	
0,0,	240,98,	236,250,	0,0,	
241,98,	0,0,	0,0,	0,0,	
240,98,	0,0,	238,252,	241,98,	
237,251,	240,98,	243,98,	244,98,	
241,98,	239,253,	245,98,	243,98,	
244,98,	0,0,	0,0,	245,98,	
246,98,	0,0,	243,98,	244,98,	
0,0,	246,98,	245,98,	243,98,	
244,98,	0,0,	247,98,	245,98,	
246,98,	250,98,	0,0,	247,98,	
240,254,	246,98,	250,98,	0,0,	
241,255,	243,257,	247,98,	249,249,	
250,262,	250,98,	0,0,	247,98,	
251,98,	252,98,	250,98,	249,249,	
249,249,	251,98,	252,98,	0,0,	
0,0,	246,259,	253,98,	0,0,	
251,98,	252,98,	0,0,	253,98,	
245,258,	251,98,	252,98,	254,98,	
0,0,	0,0,	253,98,	0,0,	
254,98,	0,0,	249,249,	253,98,	
249,249,	255,98,	247,260,	254,98,	
249,249,	0,0,	255,98,	249,261,	
254,98,	0,0,	0,0,	0,0,	
0,0,	255,98,	249,249,	249,249,	
0,0,	253,264,	255,98,	0,0,	
257,98,	0,0,	258,98,	252,263,	
259,98,	257,98,	0,0,	258,98,	
254,265,	259,98,	0,0,	249,249,	
257,98,	258,269,	258,98,	259,270,	
259,98,	257,98,	0,0,	258,98,	
262,98,	259,98,	260,271,	260,272,	
260,273,	262,98,	0,0,	0,0,	
0,0,	255,266,	263,98,	0,0,	
262,98,	257,268,	0,0,	263,98,	
0,0,	262,98,	260,98,	0,0,	
264,98,	263,276,	263,98,	260,98,	
0,0,	264,98,	0,0,	263,98,	
265,98,	260,274,	260,98,	262,275,	
264,98,	265,98,	0,0,	260,98,	
0,0,	264,98,	0,0,	265,278,	
265,98,	266,279,	266,280,	266,281,	
268,98,	265,98,	269,98,	267,267,	
267,267,	268,98,	0,0,	269,98,	
0,0,	0,0,	270,98,	299,98,	
268,98,	266,98,	269,98,	270,98,	
299,98,	268,98,	266,98,	269,98,	
264,277,	0,0,	270,98,	299,98,	
266,282,	266,98,	267,267,	270,98,	
299,98,	0,0,	266,98,	0,0,	
271,98,	269,285,	272,98,	267,283,	
0,0,	271,98,	0,0,	272,98,	
0,0,	270,286,	273,98,	271,287,	
271,98,	272,288,	272,98,	273,98,	
268,284,	271,98,	274,98,	272,98,	
275,98,	273,289,	273,98,	274,98,	
276,98,	275,98,	277,98,	273,98,	
0,0,	276,98,	274,98,	277,98,	
275,98,	0,0,	278,98,	274,98,	
276,98,	275,98,	277,98,	278,98,	
0,0,	276,98,	0,0,	277,98,	
0,0,	279,98,	278,98,	0,0,	
0,0,	274,290,	279,98,	278,98,	
0,0,	275,291,	0,0,	276,292,	
279,295,	279,98,	280,98,	277,293,	
0,0,	281,98,	279,98,	280,98,	
0,0,	278,294,	281,98,	0,0,	
0,0,	280,296,	280,98,	282,98,	
281,297,	281,98,	284,98,	280,98,	
282,98,	285,98,	281,98,	284,98,	
286,98,	0,0,	285,98,	282,98,	
0,0,	286,98,	284,98,	0,0,	
282,98,	285,98,	0,0,	284,98,	
286,98,	0,0,	285,98,	287,98,	
0,0,	286,98,	0,0,	288,98,	
287,98,	0,0,	282,298,	289,98,	
288,98,	0,0,	0,0,	287,98,	
289,98,	0,0,	285,300,	288,98,	
287,98,	286,301,	0,0,	289,98,	
288,98,	0,0,	0,0,	284,299,	
289,98,	0,0,	0,0,	290,98,	
291,98,	0,0,	287,302,	292,98,	
290,98,	291,98,	288,303,	0,0,	
292,98,	293,98,	289,304,	290,98,	
291,98,	294,98,	293,98,	292,98,	
290,98,	291,98,	294,98,	0,0,	
292,98,	293,98,	295,98,	296,98,	
0,0,	294,98,	293,98,	295,98,	
296,98,	0,0,	294,98,	0,0,	
290,305,	297,98,	295,98,	296,98,	
292,307,	0,0,	297,98,	295,98,	
296,98,	0,0,	0,0,	0,0,	
293,308,	297,98,	294,309,	0,0,	
291,306,	0,0,	297,98,	298,98,	
0,0,	295,310,	296,311,	300,98,	
298,98,	301,98,	0,0,	0,0,	
300,98,	302,98,	301,98,	298,98,	
297,312,	303,98,	302,98,	300,98,	
298,98,	301,98,	303,98,	304,98,	
300,98,	302,98,	301,98,	305,98,	
304,98,	303,98,	302,98,	0,0,	
305,98,	0,0,	303,98,	304,98,	
298,313,	306,98,	0,0,	305,98,	
304,98,	0,0,	306,98,	0,0,	
305,98,	0,0,	302,316,	307,98,	
0,0,	306,98,	303,317,	0,0,	
307,98,	0,0,	306,98,	300,314,	
304,318,	301,315,	0,0,	307,98,	
308,98,	308,322,	309,98,	0,0,	
307,98,	308,98,	310,98,	309,98,	
311,98,	0,0,	0,0,	310,98,	
308,98,	311,98,	309,98,	305,319,	
312,98,	308,98,	310,98,	309,98,	
311,98,	312,98,	306,320,	310,98,	
0,0,	311,98,	313,98,	0,0,	
312,98,	0,0,	0,0,	313,98,	
0,0,	312,98,	0,0,	307,321,	
0,0,	0,0,	313,98,	310,324,	
314,98,	311,325,	315,98,	313,98,	
0,0,	314,98,	316,98,	315,98,	
317,98,	312,326,	309,323,	316,98,	
314,98,	317,98,	315,98,	0,0,	
318,98,	314,98,	316,98,	315,98,	
317,98,	318,98,	0,0,	316,98,	
319,98,	317,98,	320,98,	321,98,	
318,98,	319,98,	0,0,	320,98,	
321,98,	318,98,	313,327,	0,0,	
319,98,	0,0,	320,98,	321,98,	
0,0,	319,98,	0,0,	320,98,	
321,98,	314,328,	322,98,	315,329,	
323,98,	0,0,	324,98,	322,98,	
0,0,	323,98,	316,330,	324,98,	
317,331,	0,0,	322,98,	0,0,	
323,98,	325,98,	324,98,	322,98,	
318,332,	323,98,	325,98,	324,98,	
0,0,	319,333,	326,98,	327,98,	
321,334,	325,98,	0,0,	326,98,	
327,98,	0,0,	325,98,	0,0,	
328,98,	0,0,	326,98,	327,98,	
329,98,	328,98,	0,0,	326,98,	
327,98,	329,98,	330,98,	322,335,	
328,98,	323,336,	331,98,	330,98,	
329,98,	328,98,	324,337,	331,98,	
332,98,	329,98,	330,98,	333,98,	
0,0,	332,98,	331,98,	330,98,	
333,98,	325,338,	0,0,	331,98,	
332,98,	0,0,	334,98,	333,98,	
327,340,	332,98,	326,339,	334,98,	
333,98,	0,0,	0,0,	0,0,	
335,98,	336,98,	334,98,	0,0,	
0,0,	335,98,	336,98,	334,98,	
0,0,	0,0,	0,0,	330,341,	
335,98,	336,98,	337,98,	331,342,	
0,0,	335,98,	336,98,	337,98,	
338,98,	332,343,	339,98,	340,98,	
341,98,	338,98,	337,98,	339,98,	
340,98,	341,98,	0,0,	337,98,	
338,98,	342,98,	339,98,	340,98,	
341,98,	338,98,	342,98,	339,98,	
340,98,	341,98,	0,0,	343,98,	
344,98,	342,98,	0,0,	0,0,	
343,98,	344,98,	342,98,	335,344,	
345,98,	0,0,	0,0,	343,98,	
344,98,	345,98,	346,98,	337,345,	
343,98,	344,98,	0,0,	346,98,	
345,98,	338,346,	347,98,	339,347,	
348,98,	345,98,	346,98,	347,98,	
349,98,	348,98,	350,98,	346,98,	
0,0,	349,98,	347,98,	350,98,	
348,98,	0,0,	351,98,	347,98,	
349,98,	348,98,	350,98,	351,98,	
344,348,	349,98,	0,0,	350,98,	
352,98,	353,98,	351,98,	351,352,	
0,0,	352,98,	353,98,	351,98,	
0,0,	0,0,	0,0,	0,0,	
352,98,	353,98,	0,0,	349,350,	
350,351,	352,98,	353,98,	354,98,	
0,0,	0,0,	355,98,	356,98,	
354,98,	358,98,	348,349,	355,98,	
356,98,	0,0,	358,98,	354,98,	
352,353,	357,98,	355,98,	356,98,	
354,98,	358,98,	357,98,	355,98,	
356,98,	353,354,	358,98,	0,0,	
357,358,	357,98,	359,98,	360,98,	
361,98,	0,0,	357,98,	359,98,	
360,98,	361,98,	354,355,	355,356,	
358,359,	362,98,	359,98,	360,98,	
361,98,	0,0,	362,98,	359,98,	
360,98,	361,98,	0,0,	0,0,	
0,0,	362,98,	0,0,	0,0,	
0,0,	356,357,	362,98,	0,0,	
0,0,	0,0,	0,0,	359,360,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	361,362,	0,0,	360,361,	
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
yycrank+1,	yysvec+10,	yyvstop+21,
yycrank+209,	0,		yyvstop+24,
yycrank+0,	0,		yyvstop+27,
yycrank+3,	yysvec+10,	yyvstop+30,
yycrank+9,	yysvec+10,	yyvstop+33,
yycrank+13,	yysvec+10,	yyvstop+36,
yycrank+12,	yysvec+10,	yyvstop+39,
yycrank+14,	yysvec+10,	yyvstop+42,
yycrank+16,	yysvec+10,	yyvstop+45,
yycrank+16,	yysvec+10,	yyvstop+48,
yycrank+116,	yysvec+10,	yyvstop+51,
yycrank+21,	yysvec+10,	yyvstop+54,
yycrank+117,	yysvec+10,	yyvstop+57,
yycrank+-235,	0,		yyvstop+60,
yycrank+-3,	yysvec+6,	0,	
yycrank+0,	0,		yyvstop+63,
yycrank+-247,	0,		yyvstop+65,
yycrank+0,	yysvec+9,	yyvstop+67,
yycrank+0,	yysvec+10,	yyvstop+69,
yycrank+13,	yysvec+10,	yyvstop+71,
yycrank+0,	yysvec+12,	0,	
yycrank+242,	yysvec+12,	0,	
yycrank+37,	yysvec+10,	yyvstop+73,
yycrank+40,	yysvec+10,	yyvstop+75,
yycrank+28,	yysvec+10,	yyvstop+77,
yycrank+23,	yysvec+10,	yyvstop+79,
yycrank+42,	yysvec+10,	yyvstop+81,
yycrank+49,	yysvec+10,	yyvstop+83,
yycrank+39,	yysvec+10,	yyvstop+85,
yycrank+46,	yysvec+10,	yyvstop+87,
yycrank+56,	yysvec+10,	yyvstop+89,
yycrank+116,	yysvec+10,	yyvstop+91,
yycrank+60,	yysvec+10,	yyvstop+93,
yycrank+48,	yysvec+10,	yyvstop+95,
yycrank+89,	yysvec+10,	yyvstop+97,
yycrank+72,	yysvec+10,	yyvstop+99,
yycrank+74,	yysvec+10,	yyvstop+101,
yycrank+84,	yysvec+10,	yyvstop+103,
yycrank+84,	yysvec+10,	yyvstop+105,
yycrank+0,	yysvec+24,	yyvstop+107,
yycrank+0,	0,		yyvstop+109,
yycrank+0,	0,		yyvstop+111,
yycrank+-277,	0,		yyvstop+113,
yycrank+0,	0,		yyvstop+115,
yycrank+-13,	yysvec+53,	yyvstop+117,
yycrank+13,	yysvec+10,	yyvstop+119,
yycrank+214,	0,		0,	
yycrank+0,	0,		yyvstop+121,
yycrank+111,	yysvec+10,	yyvstop+123,
yycrank+111,	yysvec+10,	yyvstop+125,
yycrank+114,	yysvec+10,	yyvstop+127,
yycrank+132,	yysvec+10,	yyvstop+129,
yycrank+134,	yysvec+10,	yyvstop+131,
yycrank+133,	yysvec+10,	yyvstop+133,
yycrank+240,	yysvec+10,	yyvstop+135,
yycrank+132,	yysvec+10,	yyvstop+138,
yycrank+123,	yysvec+10,	yyvstop+140,
yycrank+120,	yysvec+10,	yyvstop+142,
yycrank+123,	yysvec+10,	yyvstop+144,
yycrank+132,	yysvec+10,	yyvstop+146,
yycrank+128,	yysvec+10,	yyvstop+148,
yycrank+127,	yysvec+10,	yyvstop+150,
yycrank+142,	yysvec+10,	yyvstop+152,
yycrank+137,	yysvec+10,	yyvstop+154,
yycrank+154,	yysvec+10,	yyvstop+156,
yycrank+308,	0,		yyvstop+158,
yycrank+-14,	yysvec+53,	yyvstop+161,
yycrank+13,	yysvec+10,	yyvstop+163,
yycrank+0,	yysvec+10,	yyvstop+165,
yycrank+140,	yysvec+10,	yyvstop+168,
yycrank+152,	yysvec+10,	yyvstop+170,
yycrank+162,	yysvec+10,	yyvstop+172,
yycrank+155,	yysvec+10,	yyvstop+174,
yycrank+165,	yysvec+10,	yyvstop+176,
yycrank+334,	0,		0,	
yycrank+166,	yysvec+10,	yyvstop+178,
yycrank+176,	yysvec+10,	yyvstop+180,
yycrank+295,	yysvec+10,	yyvstop+182,
yycrank+185,	yysvec+10,	yyvstop+185,
yycrank+192,	yysvec+10,	yyvstop+187,
yycrank+190,	yysvec+10,	yyvstop+189,
yycrank+187,	yysvec+10,	yyvstop+191,
yycrank+177,	yysvec+10,	yyvstop+193,
yycrank+189,	yysvec+10,	yyvstop+195,
yycrank+194,	yysvec+10,	yyvstop+197,
yycrank+181,	yysvec+10,	yyvstop+199,
yycrank+292,	yysvec+10,	yyvstop+201,
yycrank+250,	yysvec+76,	yyvstop+204,
yycrank+365,	yysvec+76,	yyvstop+207,
yycrank+251,	yysvec+76,	yyvstop+210,
yycrank+367,	yysvec+76,	yyvstop+213,
yycrank+368,	yysvec+76,	yyvstop+216,
yycrank+-443,	0,		yyvstop+219,
yycrank+211,	yysvec+10,	yyvstop+222,
yycrank+51,	yysvec+10,	yyvstop+224,
yycrank+0,	yysvec+10,	yyvstop+227,
yycrank+202,	yysvec+10,	yyvstop+230,
yycrank+0,	yysvec+10,	yyvstop+232,
yycrank+206,	yysvec+10,	yyvstop+235,
yycrank+197,	0,		0,	
yycrank+206,	0,		0,	
yycrank+204,	yysvec+10,	yyvstop+237,
yycrank+208,	yysvec+10,	yyvstop+239,
yycrank+336,	0,		0,	
yycrank+210,	yysvec+10,	yyvstop+241,
yycrank+207,	yysvec+10,	yyvstop+243,
yycrank+214,	yysvec+10,	yyvstop+245,
yycrank+232,	yysvec+10,	yyvstop+247,
yycrank+237,	yysvec+10,	yyvstop+249,
yycrank+447,	yysvec+10,	yyvstop+251,
yycrank+238,	yysvec+10,	yyvstop+254,
yycrank+228,	yysvec+10,	yyvstop+256,
yycrank+242,	yysvec+10,	yyvstop+258,
yycrank+450,	0,		0,	
yycrank+453,	0,		0,	
yycrank+422,	yysvec+76,	yyvstop+260,
yycrank+423,	yysvec+76,	yyvstop+263,
yycrank+431,	yysvec+76,	yyvstop+266,
yycrank+432,	yysvec+76,	yyvstop+269,
yycrank+0,	0,		yyvstop+272,
yycrank+260,	yysvec+10,	yyvstop+275,
yycrank+340,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+277,
yycrank+0,	yysvec+10,	yyvstop+280,
yycrank+231,	0,		0,	
yycrank+236,	0,		0,	
yycrank+248,	yysvec+10,	yyvstop+283,
yycrank+239,	yysvec+10,	yyvstop+285,
yycrank+244,	0,		0,	
yycrank+251,	yysvec+10,	yyvstop+287,
yycrank+258,	yysvec+10,	yyvstop+289,
yycrank+269,	yysvec+10,	yyvstop+291,
yycrank+255,	yysvec+10,	yyvstop+293,
yycrank+0,	yysvec+10,	yyvstop+295,
yycrank+455,	0,		0,	
yycrank+289,	yysvec+10,	yyvstop+298,
yycrank+0,	yysvec+10,	yyvstop+300,
yycrank+300,	yysvec+10,	yyvstop+303,
yycrank+294,	yysvec+10,	yyvstop+305,
yycrank+513,	0,		0,	
yycrank+433,	yysvec+76,	yyvstop+307,
yycrank+441,	yysvec+76,	yyvstop+310,
yycrank+459,	yysvec+76,	yyvstop+313,
yycrank+570,	yysvec+76,	yyvstop+316,
yycrank+325,	yysvec+10,	yyvstop+319,
yycrank+316,	0,		0,	
yycrank+332,	0,		0,	
yycrank+323,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+321,
yycrank+324,	yysvec+10,	yyvstop+324,
yycrank+331,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+326,
yycrank+340,	yysvec+10,	yyvstop+329,
yycrank+0,	yysvec+10,	yyvstop+331,
yycrank+354,	yysvec+10,	yyvstop+334,
yycrank+351,	0,		0,	
yycrank+350,	yysvec+10,	yyvstop+336,
yycrank+367,	yysvec+10,	yyvstop+338,
yycrank+0,	yysvec+10,	yyvstop+340,
yycrank+629,	0,		0,	
yycrank+519,	0,		0,	
yycrank+472,	yysvec+76,	yyvstop+343,
yycrank+579,	yysvec+76,	yyvstop+346,
yycrank+580,	yysvec+76,	yyvstop+349,
yycrank+588,	yysvec+76,	yyvstop+352,
yycrank+589,	yysvec+76,	yyvstop+355,
yycrank+602,	yysvec+76,	yyvstop+358,
yycrank+603,	yysvec+76,	yyvstop+361,
yycrank+611,	yysvec+76,	yyvstop+364,
yycrank+617,	yysvec+76,	yyvstop+367,
yycrank+613,	yysvec+76,	yyvstop+370,
yycrank+627,	yysvec+76,	yyvstop+373,
yycrank+635,	yysvec+76,	yyvstop+376,
yycrank+386,	yysvec+10,	yyvstop+379,
yycrank+363,	0,		0,	
yycrank+0,	0,		yyvstop+381,
yycrank+355,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+383,
yycrank+0,	0,		yyvstop+386,
yycrank+0,	yysvec+10,	yyvstop+388,
yycrank+372,	yysvec+10,	yyvstop+391,
yycrank+357,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+393,
yycrank+0,	yysvec+10,	yyvstop+396,
yycrank+525,	0,		0,	
yycrank+641,	yysvec+76,	yyvstop+399,
yycrank+650,	yysvec+76,	yyvstop+402,
yycrank+654,	yysvec+76,	yyvstop+405,
yycrank+656,	yysvec+76,	yyvstop+408,
yycrank+664,	yysvec+76,	yyvstop+411,
yycrank+674,	yysvec+76,	yyvstop+414,
yycrank+675,	yysvec+76,	yyvstop+417,
yycrank+684,	yysvec+76,	yyvstop+420,
yycrank+688,	yysvec+76,	yyvstop+423,
yycrank+690,	yysvec+76,	yyvstop+426,
yycrank+698,	yysvec+76,	yyvstop+429,
yycrank+708,	yysvec+76,	yyvstop+432,
yycrank+390,	yysvec+10,	yyvstop+435,
yycrank+768,	0,		0,	
yycrank+0,	0,		yyvstop+437,
yycrank+0,	yysvec+10,	yyvstop+439,
yycrank+0,	0,		yyvstop+442,
yycrank+369,	0,		0,	
yycrank+718,	yysvec+76,	yyvstop+444,
yycrank+825,	yysvec+76,	yyvstop+447,
yycrank+731,	yysvec+76,	yyvstop+450,
yycrank+826,	yysvec+76,	yyvstop+453,
yycrank+827,	yysvec+76,	yyvstop+456,
yycrank+828,	yysvec+76,	yyvstop+459,
yycrank+829,	yysvec+76,	yyvstop+462,
yycrank+847,	yysvec+76,	yyvstop+465,
yycrank+849,	yysvec+76,	yyvstop+468,
yycrank+851,	yysvec+76,	yyvstop+471,
yycrank+870,	yysvec+76,	yyvstop+474,
yycrank+871,	yysvec+76,	yyvstop+477,
yycrank+0,	yysvec+10,	yyvstop+480,
yycrank+892,	0,		0,	
yycrank+0,	0,		yyvstop+483,
yycrank+378,	0,		0,	
yycrank+727,	yysvec+76,	yyvstop+485,
yycrank+889,	yysvec+76,	yyvstop+488,
yycrank+891,	yysvec+76,	yyvstop+491,
yycrank+893,	yysvec+76,	yyvstop+494,
yycrank+899,	yysvec+76,	yyvstop+497,
yycrank+926,	yysvec+76,	yyvstop+500,
yycrank+902,	yysvec+76,	yyvstop+503,
yycrank+913,	yysvec+76,	yyvstop+506,
yycrank+923,	yysvec+76,	yyvstop+509,
yycrank+915,	yysvec+76,	yyvstop+512,
yycrank+942,	yysvec+76,	yyvstop+515,
yycrank+945,	yysvec+76,	yyvstop+518,
yycrank+386,	0,		0,	
yycrank+960,	yysvec+76,	yyvstop+521,
yycrank+961,	yysvec+76,	yyvstop+524,
yycrank+964,	yysvec+76,	yyvstop+528,
yycrank+970,	yysvec+76,	yyvstop+531,
yycrank+980,	yysvec+76,	yyvstop+534,
yycrank+481,	0,		0,	
yycrank+-1058,	0,		0,	
yycrank+983,	yysvec+76,	yyvstop+537,
yycrank+998,	yysvec+76,	yyvstop+540,
yycrank+999,	yysvec+76,	yyvstop+544,
yycrank+1008,	yysvec+76,	yyvstop+547,
yycrank+1017,	yysvec+76,	yyvstop+550,
yycrank+1027,	yysvec+76,	yyvstop+553,
yycrank+655,	0,		0,	
yycrank+1046,	yysvec+76,	yyvstop+556,
yycrank+1048,	yysvec+76,	yyvstop+559,
yycrank+1050,	yysvec+76,	yyvstop+562,
yycrank+1084,	yysvec+76,	yyvstop+565,
yycrank+0,	0,		yyvstop+568,
yycrank+1066,	yysvec+76,	yyvstop+570,
yycrank+1076,	yysvec+76,	yyvstop+573,
yycrank+1086,	yysvec+76,	yyvstop+576,
yycrank+1094,	yysvec+76,	yyvstop+579,
yycrank+1123,	yysvec+76,	yyvstop+582,
yycrank+1170,	0,		0,	
yycrank+1110,	yysvec+76,	yyvstop+585,
yycrank+1112,	yysvec+76,	yyvstop+588,
yycrank+1120,	yysvec+76,	yyvstop+591,
yycrank+1142,	yysvec+76,	yyvstop+594,
yycrank+1144,	yysvec+76,	yyvstop+597,
yycrank+1152,	yysvec+76,	yyvstop+600,
yycrank+1160,	yysvec+76,	yyvstop+603,
yycrank+1162,	yysvec+76,	yyvstop+606,
yycrank+1166,	yysvec+76,	yyvstop+609,
yycrank+1168,	yysvec+76,	yyvstop+612,
yycrank+1176,	yysvec+76,	yyvstop+615,
yycrank+1187,	yysvec+76,	yyvstop+618,
yycrank+1200,	yysvec+76,	yyvstop+621,
yycrank+1203,	yysvec+76,	yyvstop+624,
yycrank+1213,	yysvec+76,	yyvstop+627,
yycrank+0,	0,		yyvstop+630,
yycrank+1216,	yysvec+76,	yyvstop+632,
yycrank+1219,	yysvec+76,	yyvstop+635,
yycrank+1222,	yysvec+76,	yyvstop+638,
yycrank+1237,	yysvec+76,	yyvstop+641,
yycrank+1241,	yysvec+76,	yyvstop+644,
yycrank+1245,	yysvec+76,	yyvstop+647,
yycrank+1265,	yysvec+76,	yyvstop+650,
yycrank+1266,	yysvec+76,	yyvstop+653,
yycrank+1269,	yysvec+76,	yyvstop+656,
yycrank+1275,	yysvec+76,	yyvstop+659,
yycrank+1279,	yysvec+76,	yyvstop+662,
yycrank+1288,	yysvec+76,	yyvstop+665,
yycrank+1289,	yysvec+76,	yyvstop+668,
yycrank+1299,	yysvec+76,	yyvstop+671,
yycrank+1317,	yysvec+76,	yyvstop+674,
yycrank+1121,	yysvec+76,	yyvstop+677,
yycrank+1321,	yysvec+76,	yyvstop+681,
yycrank+1323,	yysvec+76,	yyvstop+684,
yycrank+1327,	yysvec+76,	yyvstop+687,
yycrank+1331,	yysvec+76,	yyvstop+690,
yycrank+1337,	yysvec+76,	yyvstop+693,
yycrank+1341,	yysvec+76,	yyvstop+696,
yycrank+1351,	yysvec+76,	yyvstop+699,
yycrank+1361,	yysvec+76,	yyvstop+702,
yycrank+1374,	yysvec+76,	yyvstop+705,
yycrank+1376,	yysvec+76,	yyvstop+708,
yycrank+1380,	yysvec+76,	yyvstop+711,
yycrank+1382,	yysvec+76,	yyvstop+714,
yycrank+1390,	yysvec+76,	yyvstop+717,
yycrank+1400,	yysvec+76,	yyvstop+720,
yycrank+1414,	yysvec+76,	yyvstop+723,
yycrank+1416,	yysvec+76,	yyvstop+726,
yycrank+1420,	yysvec+76,	yyvstop+729,
yycrank+1422,	yysvec+76,	yyvstop+732,
yycrank+1430,	yysvec+76,	yyvstop+735,
yycrank+1438,	yysvec+76,	yyvstop+738,
yycrank+1440,	yysvec+76,	yyvstop+741,
yycrank+1441,	yysvec+76,	yyvstop+745,
yycrank+1460,	yysvec+76,	yyvstop+748,
yycrank+1462,	yysvec+76,	yyvstop+751,
yycrank+1464,	yysvec+76,	yyvstop+754,
yycrank+1475,	yysvec+76,	yyvstop+757,
yycrank+1484,	yysvec+76,	yyvstop+760,
yycrank+1485,	yysvec+76,	yyvstop+763,
yycrank+1494,	yysvec+76,	yyvstop+766,
yycrank+1498,	yysvec+76,	yyvstop+770,
yycrank+1504,	yysvec+76,	yyvstop+774,
yycrank+1508,	yysvec+76,	yyvstop+777,
yycrank+1514,	yysvec+76,	yyvstop+780,
yycrank+1517,	yysvec+76,	yyvstop+783,
yycrank+1528,	yysvec+76,	yyvstop+787,
yycrank+1538,	yysvec+76,	yyvstop+791,
yycrank+1539,	yysvec+76,	yyvstop+794,
yycrank+1552,	yysvec+76,	yyvstop+798,
yycrank+1558,	yysvec+76,	yyvstop+801,
yycrank+1560,	yysvec+76,	yyvstop+804,
yycrank+1561,	yysvec+76,	yyvstop+807,
yycrank+1562,	yysvec+76,	yyvstop+811,
yycrank+1571,	yysvec+76,	yyvstop+815,
yycrank+1581,	yysvec+76,	yyvstop+819,
yycrank+1582,	yysvec+76,	yyvstop+823,
yycrank+1590,	yysvec+76,	yyvstop+826,
yycrank+1596,	yysvec+76,	yyvstop+830,
yycrank+1604,	yysvec+76,	yyvstop+834,
yycrank+1606,	yysvec+76,	yyvstop+838,
yycrank+1610,	yysvec+76,	yyvstop+841,
yycrank+1612,	yysvec+76,	yyvstop+844,
yycrank+1620,	yysvec+76,	yyvstop+847,
yycrank+1630,	yysvec+76,	yyvstop+850,
yycrank+1631,	yysvec+76,	yyvstop+853,
yycrank+1649,	yysvec+76,	yyvstop+856,
yycrank+1652,	yysvec+76,	yyvstop+859,
yycrank+1653,	yysvec+76,	yyvstop+862,
yycrank+1663,	yysvec+76,	yyvstop+865,
yycrank+1655,	yysvec+76,	yyvstop+868,
yycrank+1676,	yysvec+76,	yyvstop+871,
yycrank+1677,	yysvec+76,	yyvstop+874,
yycrank+1678,	yysvec+76,	yyvstop+877,
yycrank+1687,	yysvec+76,	yyvstop+880,
0,	0,	0};
struct yywork *yytop = yycrank+1791;
struct yysvf *yybgin = yysvec+1;
char yymatch[] = {
  0,   1,   1,   1,   1,   1,   1,   1, 
  1,   9,  10,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
 32,   1,  34,   1,   1,   1,  38,   1, 
 38,  41,  38,   1,  38,  38,  38,   1, 
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
0,0,0,0,0,0,0,0,
0,1,0,0,0,0,0,0,
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
