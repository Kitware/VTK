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

# line 85 "concrete.l"
    return(ImageSetMacro);
break;
case 48:

# line 86 "concrete.l"
return(ImageSetExtentMacro);
break;
case 49:

# line 88 "concrete.l"
	{ sscanf(yytext,"%d",&yylval.integer); return(NUM);}
break;
case 50:

# line 90 "concrete.l"
{
		yylval.str =  strdup(yytext + 1);
		yylval.str[strlen(yytext)-2] = '\0';
		return(STRING);
		}
break;
case 51:

# line 96 "concrete.l"
    ;
break;
case 52:

# line 99 "concrete.l"
 { yylval.str = (char *)malloc(yyleng + 1);
                     memcpy(yylval.str,yytext,yyleng);
                     yylval.str[yyleng] = '\0';
                     return(VTK_ID);
                    }
break;
case 53:

# line 105 "concrete.l"
 { yylval.str = (char *)malloc(yyleng + 1);
                          memcpy(yylval.str,yytext,yyleng);
                          yylval.str[yyleng] = '\0';
                          return(ID);
                        }
break;
case 54:

# line 111 "concrete.l"
;
break;
case 55:

# line 113 "concrete.l"
return(yytext[0]);
break;
case 56:

# line 114 "concrete.l"
return(yytext[0]);
break;
case 57:

# line 117 "concrete.l"
return(yytext[0]);
break;
case 58:

# line 119 "concrete.l"
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

58,
0,

54,
58,
0,

54,
0,

58,
0,

57,
58,
0,

58,
0,

49,
58,
0,

53,
58,
0,

53,
58,
0,

55,
58,
0,

56,
58,
0,

53,
58,
0,

53,
58,
0,

53,
58,
0,

53,
58,
0,

53,
58,
0,

53,
58,
0,

53,
58,
0,

53,
58,
0,

53,
58,
0,

53,
58,
0,

58,
-51,
0,

1,
0,

-3,
0,

49,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

-51,
0,

51,
0,

50,
0,

-3,
0,

3,
0,

-3,
0,

53,
0,

6,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

17,
53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

52,
53,
0,

-3,
0,

53,
0,

23,
53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

20,
53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

22,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

-2,
-3,
0,

53,
0,

13,
53,
0,

25,
53,
0,

53,
0,

18,
53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

19,
53,
0,

53,
0,

53,
0,

53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

2,
3,
0,

53,
0,

21,
53,
0,

28,
53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

53,
0,

14,
53,
0,

53,
0,

29,
53,
0,

53,
0,

53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

53,
0,

12,
53,
0,

53,
0,

10,
53,
0,

53,
0,

15,
53,
0,

53,
0,

53,
0,

53,
0,

24,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

53,
0,

12,
0,

11,
53,
0,

10,
0,

26,
53,
0,

53,
0,

9,
53,
0,

27,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

53,
0,

11,
0,

16,
53,
0,

9,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

4,
53,
0,

7,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

31,
52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

30,
52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

5,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

8,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

38,
52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

47,
52,
53,
0,

34,
52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

37,
52,
53,
0,

33,
52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

46,
52,
53,
0,

52,
53,
0,

35,
52,
53,
0,

52,
53,
0,

32,
52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

45,
52,
53,
0,

42,
52,
53,
0,

43,
52,
53,
0,

44,
52,
53,
0,

52,
53,
0,

52,
53,
0,

39,
52,
53,
0,

40,
52,
53,
0,

41,
52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

48,
52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

52,
53,
0,

36,
52,
53,
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
9,28,	9,28,	78,105,	106,134,	
6,25,	11,30,	0,0,	1,11,	
30,56,	0,0,	0,0,	0,0,	
1,12,	0,0,	1,13,	0,0,	
0,0,	55,77,	0,0,	0,0,	
1,14,	1,15,	77,104,	1,16,	
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
32,57,	74,96,	75,97,	80,106,	
27,53,	27,54,	12,32,	12,32,	
12,32,	12,32,	12,32,	12,32,	
12,32,	12,32,	12,32,	24,50,	
81,107,	24,50,	82,108,	83,109,	
65,85,	24,50,	32,57,	84,110,	
24,50,	86,113,	53,53,	27,53,	
87,114,	27,53,	89,117,	24,50,	
24,50,	27,53,	53,53,	53,54,	
27,53,	90,118,	32,32,	91,119,	
92,120,	93,121,	94,122,	27,53,	
27,53,	95,123,	96,124,	105,133,	
24,50,	97,125,	97,125,	108,135,	
88,115,	88,115,	110,136,	57,58,	
111,137,	53,53,	112,138,	53,53,	
27,53,	27,55,	113,139,	53,53,	
98,98,	114,140,	53,53,	116,142,	
117,143,	98,98,	118,144,	98,98,	
97,125,	53,53,	53,53,	88,115,	
98,98,	119,145,	120,146,	122,149,	
97,126,	98,98,	100,98,	32,58,	
123,150,	124,151,	133,158,	100,98,	
137,160,	100,98,	53,53,	85,85,	
85,85,	138,161,	100,98,	139,162,	
65,86,	140,163,	141,164,	100,98,	
142,165,	143,166,	144,167,	65,87,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	85,85,	145,168,	
148,170,	100,128,	150,171,	151,172,	
158,188,	76,98,	76,99,	76,98,	
76,98,	76,98,	76,98,	76,100,	
76,98,	76,101,	76,98,	76,98,	
76,98,	76,98,	76,102,	76,98,	
76,98,	76,98,	76,98,	76,103,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	159,189,	
88,116,	160,190,	161,191,	76,98,	
163,192,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	76,98,	
76,98,	76,98,	76,98,	99,98,	
115,115,	115,115,	101,98,	164,193,	
99,98,	166,194,	99,98,	101,98,	
102,98,	101,98,	85,111,	99,98,	
168,195,	102,98,	101,98,	102,98,	
99,98,	85,112,	169,196,	101,98,	
102,98,	103,98,	170,197,	115,115,	
171,198,	102,98,	103,98,	104,104,	
103,98,	121,147,	121,147,	125,125,	
125,125,	103,98,	188,213,	104,104,	
104,132,	189,214,	103,98,	191,215,	
126,126,	126,126,	195,216,	196,217,	
99,127,	101,129,	134,134,	134,134,	
213,232,	218,235,	235,249,	249,264,	
121,147,	102,130,	125,125,	0,0,	
103,131,	0,0,	104,104,	0,0,	
104,104,	0,0,	125,126,	126,126,	
104,104,	127,98,	0,0,	104,104,	
0,0,	134,134,	127,98,	128,98,	
127,98,	126,152,	104,104,	104,104,	
128,98,	127,98,	128,98,	0,0,	
129,98,	256,256,	127,98,	128,98,	
0,0,	129,98,	0,0,	129,98,	
128,98,	256,257,	0,0,	104,104,	
129,98,	147,147,	147,147,	130,98,	
115,141,	129,98,	0,0,	0,0,	
130,98,	131,98,	130,98,	0,0,	
0,0,	0,0,	131,98,	130,98,	
131,98,	0,0,	127,153,	129,155,	
130,98,	131,98,	0,0,	0,0,	
147,147,	0,0,	131,98,	153,98,	
0,0,	128,154,	0,0,	0,0,	
153,98,	121,148,	153,98,	0,0,	
0,0,	0,0,	0,0,	153,98,	
155,98,	0,0,	152,173,	152,173,	
153,98,	155,98,	0,0,	155,98,	
173,173,	173,173,	199,199,	199,199,	
155,98,	130,156,	0,0,	154,98,	
0,0,	155,98,	0,0,	131,157,	
154,98,	0,0,	154,98,	134,159,	
0,0,	152,173,	154,176,	154,98,	
154,177,	153,175,	0,0,	173,173,	
154,178,	199,199,	152,174,	154,179,	
0,0,	155,180,	0,0,	0,0,	
173,174,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	147,169,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
0,0,	0,0,	0,0,	0,0,	
152,152,	0,0,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
152,152,	152,152,	152,152,	152,152,	
156,98,	174,174,	174,174,	199,218,	
0,0,	156,98,	175,98,	156,98,	
0,0,	157,98,	157,182,	175,98,	
156,98,	175,98,	157,98,	0,0,	
157,98,	156,98,	175,98,	156,181,	
157,183,	157,98,	157,184,	175,98,	
174,174,	157,185,	157,186,	0,0,	
176,98,	157,187,	0,0,	177,98,	
174,199,	176,98,	0,0,	176,98,	
177,98,	178,98,	177,98,	0,0,	
176,98,	175,200,	178,98,	177,98,	
178,98,	176,98,	0,0,	0,0,	
177,98,	178,98,	179,98,	233,233,	
233,233,	180,98,	178,98,	179,98,	
0,0,	179,98,	180,98,	176,201,	
180,98,	181,98,	179,98,	177,202,	
182,98,	180,98,	181,98,	179,98,	
181,98,	182,98,	180,98,	182,98,	
183,98,	181,98,	233,233,	0,0,	
182,98,	183,98,	181,98,	183,98,	
0,0,	182,98,	277,277,	277,277,	
183,98,	179,204,	0,0,	178,203,	
180,205,	183,98,	184,98,	0,0,	
0,0,	0,0,	0,0,	184,98,	
0,0,	184,98,	0,0,	0,0,	
0,0,	233,234,	184,98,	183,208,	
185,98,	277,277,	182,207,	184,98,	
186,98,	185,98,	181,206,	185,98,	
187,98,	186,98,	277,295,	186,98,	
185,98,	187,98,	0,0,	187,98,	
186,98,	185,98,	184,209,	200,98,	
187,98,	186,98,	201,98,	0,0,	
200,98,	187,98,	200,98,	201,98,	
0,0,	201,98,	0,0,	200,98,	
0,0,	0,0,	201,98,	185,210,	
200,98,	264,264,	264,264,	201,98,	
202,98,	203,98,	0,0,	187,212,	
204,98,	202,98,	203,98,	202,98,	
203,98,	204,98,	200,219,	204,98,	
202,98,	203,98,	186,211,	201,220,	
204,98,	202,98,	203,98,	205,98,	
264,264,	204,98,	206,98,	0,0,	
205,98,	0,0,	205,98,	206,98,	
0,0,	206,98,	264,277,	205,98,	
0,0,	0,0,	206,98,	0,0,	
205,224,	204,223,	207,98,	206,98,	
202,221,	208,98,	0,0,	207,98,	
0,0,	207,98,	208,98,	209,98,	
208,98,	203,222,	207,98,	0,0,	
209,98,	208,98,	209,98,	207,98,	
210,98,	206,225,	208,98,	209,98,	
211,98,	210,98,	0,0,	210,98,	
209,98,	211,98,	0,0,	211,98,	
210,98,	207,226,	0,0,	212,98,	
211,98,	210,98,	208,227,	0,0,	
212,98,	211,98,	212,98,	214,233,	
214,233,	0,0,	0,0,	212,98,	
219,98,	0,0,	0,0,	209,228,	
212,98,	219,98,	0,0,	219,98,	
210,229,	0,0,	0,0,	0,0,	
219,98,	0,0,	221,98,	0,0,	
0,0,	219,98,	214,233,	221,98,	
212,231,	221,98,	0,0,	0,0,	
211,230,	0,0,	221,98,	0,0,	
0,0,	0,0,	0,0,	221,98,	
0,0,	0,0,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
219,236,	214,234,	0,0,	0,0,	
0,0,	221,238,	0,0,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	0,0,	0,0,	0,0,	
0,0,	214,214,	0,0,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	214,214,	214,214,	214,214,	
214,214,	220,98,	222,98,	0,0,	
0,0,	223,98,	220,98,	222,98,	
220,98,	222,98,	223,98,	0,0,	
223,98,	220,98,	222,98,	0,0,	
224,98,	223,98,	220,98,	222,98,	
225,98,	224,98,	223,98,	224,98,	
0,0,	225,98,	0,0,	225,98,	
224,98,	0,0,	0,0,	226,98,	
225,98,	224,98,	0,0,	227,98,	
226,98,	225,98,	226,98,	0,0,	
227,98,	222,239,	227,98,	226,98,	
0,0,	0,0,	0,0,	227,98,	
226,98,	220,237,	0,0,	224,241,	
227,98,	228,98,	225,242,	223,240,	
229,98,	229,246,	228,98,	230,98,	
228,98,	229,98,	0,0,	229,98,	
230,98,	228,98,	230,98,	0,0,	
229,98,	0,0,	228,98,	230,98,	
231,98,	229,98,	226,243,	236,98,	
230,98,	231,98,	0,0,	231,98,	
236,98,	0,0,	236,98,	227,244,	
231,98,	0,0,	236,250,	236,98,	
228,245,	231,98,	237,98,	0,0,	
236,98,	0,0,	238,98,	237,98,	
0,0,	237,98,	230,247,	238,98,	
0,0,	238,98,	237,98,	239,98,	
0,0,	242,256,	238,98,	237,98,	
239,98,	240,98,	239,98,	238,98,	
241,98,	242,257,	240,98,	239,98,	
240,98,	241,98,	0,0,	241,98,	
239,98,	240,98,	231,248,	0,0,	
241,98,	243,98,	240,98,	238,252,	
0,0,	241,98,	243,98,	244,98,	
243,98,	0,0,	0,0,	237,251,	
244,98,	243,98,	244,98,	242,98,	
0,0,	0,0,	243,98,	244,98,	
242,98,	0,0,	242,98,	239,253,	
244,98,	0,0,	0,0,	242,98,	
245,98,	246,98,	240,254,	0,0,	
242,98,	245,98,	246,98,	245,98,	
246,98,	247,98,	241,255,	0,0,	
245,98,	246,98,	247,98,	248,98,	
247,98,	245,98,	246,98,	243,258,	
248,98,	247,98,	248,98,	250,98,	
244,259,	0,0,	247,98,	248,98,	
250,98,	0,0,	250,98,	251,98,	
248,98,	245,260,	0,0,	250,98,	
251,98,	0,0,	251,98,	0,0,	
250,98,	0,0,	0,0,	251,98,	
252,98,	253,98,	246,261,	0,0,	
251,98,	252,98,	253,98,	252,98,	
253,98,	247,262,	250,265,	0,0,	
252,98,	253,98,	254,98,	0,0,	
248,263,	252,98,	253,98,	254,98,	
0,0,	254,98,	255,98,	0,0,	
0,0,	255,269,	254,98,	255,98,	
257,257,	255,98,	0,0,	254,98,	
0,0,	255,270,	255,98,	258,98,	
257,257,	257,257,	253,267,	255,98,	
258,98,	0,0,	258,98,	0,0,	
0,0,	0,0,	258,272,	258,98,	
0,0,	0,0,	252,266,	0,0,	
258,98,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	257,257,	
0,0,	257,257,	254,268,	0,0,	
0,0,	257,257,	259,98,	260,98,	
257,271,	0,0,	0,0,	259,98,	
260,98,	259,98,	260,98,	257,257,	
257,257,	0,0,	259,98,	260,98,	
0,0,	261,98,	262,98,	259,98,	
260,98,	0,0,	261,98,	262,98,	
261,98,	262,98,	263,98,	0,0,	
257,257,	261,98,	262,98,	263,98,	
265,98,	263,98,	261,98,	262,98,	
0,0,	265,98,	263,98,	265,98,	
266,98,	0,0,	0,0,	263,98,	
265,98,	266,98,	0,0,	266,98,	
0,0,	265,98,	0,0,	266,279,	
266,98,	260,273,	0,0,	262,275,	
0,0,	266,98,	0,0,	267,98,	
0,0,	268,281,	268,282,	268,283,	
267,98,	265,278,	267,98,	0,0,	
261,274,	0,0,	267,280,	267,98,	
0,0,	269,98,	263,276,	0,0,	
267,98,	268,98,	269,98,	0,0,	
269,98,	0,0,	268,98,	0,0,	
268,98,	269,98,	270,98,	0,0,	
268,284,	268,98,	269,98,	270,98,	
272,98,	270,98,	268,98,	273,98,	
0,0,	272,98,	270,98,	272,98,	
273,98,	0,0,	273,98,	270,98,	
272,98,	0,0,	273,288,	273,98,	
274,98,	272,98,	0,0,	0,0,	
273,98,	274,98,	0,0,	274,98,	
0,0,	270,286,	0,0,	275,98,	
274,98,	0,0,	0,0,	272,287,	
275,98,	274,98,	275,98,	269,285,	
0,0,	0,0,	275,290,	275,98,	
276,291,	276,292,	276,293,	0,0,	
275,98,	278,98,	0,0,	0,0,	
0,0,	0,0,	278,98,	0,0,	
278,98,	0,0,	0,0,	0,0,	
276,98,	278,98,	0,0,	279,98,	
274,289,	276,98,	278,98,	276,98,	
279,98,	280,98,	279,98,	276,294,	
276,98,	0,0,	280,98,	279,98,	
280,98,	276,98,	281,98,	0,0,	
279,98,	280,98,	0,0,	281,98,	
0,0,	281,98,	280,98,	0,0,	
0,0,	281,299,	281,98,	282,98,	
0,0,	0,0,	279,297,	281,98,	
282,98,	278,296,	282,98,	283,98,	
280,298,	0,0,	282,300,	282,98,	
283,98,	0,0,	283,98,	284,98,	
282,98,	0,0,	283,301,	283,98,	
284,98,	285,98,	284,98,	0,0,	
283,98,	0,0,	285,98,	284,98,	
285,98,	0,0,	286,98,	0,0,	
284,98,	285,98,	0,0,	286,98,	
287,98,	286,98,	285,98,	0,0,	
0,0,	287,98,	286,98,	287,98,	
288,98,	289,98,	284,302,	286,98,	
287,98,	288,98,	289,98,	288,98,	
289,98,	287,98,	0,0,	0,0,	
288,98,	289,98,	0,0,	0,0,	
290,98,	288,98,	289,98,	286,304,	
0,0,	290,98,	0,0,	290,98,	
0,0,	287,305,	0,0,	285,303,	
290,98,	291,98,	0,0,	288,306,	
292,98,	290,98,	291,98,	0,0,	
291,98,	292,98,	0,0,	292,98,	
291,309,	291,98,	0,0,	292,310,	
292,98,	293,98,	291,98,	290,308,	
294,98,	292,98,	293,98,	289,307,	
293,98,	294,98,	296,98,	294,98,	
293,311,	293,98,	0,0,	296,98,	
294,98,	296,98,	293,98,	297,98,	
0,0,	294,98,	296,98,	0,0,	
297,98,	298,98,	297,98,	296,98,	
0,0,	0,0,	298,98,	297,98,	
298,98,	299,98,	300,98,	294,312,	
297,98,	298,98,	299,98,	300,98,	
299,98,	300,98,	298,98,	0,0,	
0,0,	299,98,	300,98,	0,0,	
0,0,	301,98,	299,98,	300,98,	
297,314,	0,0,	301,98,	296,313,	
301,98,	0,0,	298,315,	0,0,	
0,0,	301,98,	302,98,	0,0,	
299,316,	300,317,	301,98,	302,98,	
0,0,	302,98,	303,98,	304,98,	
0,0,	0,0,	302,98,	303,98,	
304,98,	303,98,	304,98,	302,98,	
301,318,	0,0,	303,98,	304,98,	
305,98,	306,98,	0,0,	303,98,	
304,98,	305,98,	306,98,	305,98,	
306,98,	0,0,	0,0,	302,319,	
305,98,	306,98,	0,0,	307,98,	
0,0,	305,98,	306,98,	308,98,	
307,98,	303,320,	307,98,	0,0,	
308,98,	0,0,	308,98,	307,98,	
0,0,	0,0,	0,0,	308,98,	
307,98,	0,0,	306,323,	304,321,	
308,98,	309,98,	0,0,	0,0,	
0,0,	0,0,	309,98,	0,0,	
309,98,	0,0,	310,98,	0,0,	
305,322,	309,98,	307,324,	310,98,	
308,325,	310,98,	309,98,	311,98,	
0,0,	0,0,	310,98,	312,98,	
311,98,	0,0,	311,98,	310,98,	
312,98,	0,0,	312,98,	311,98,	
309,326,	0,0,	0,0,	312,98,	
311,98,	313,98,	0,0,	0,0,	
312,98,	310,327,	313,98,	0,0,	
313,98,	0,0,	314,98,	0,0,	
0,0,	313,98,	311,328,	314,98,	
315,98,	314,98,	313,98,	316,98,	
312,329,	315,98,	314,98,	315,98,	
316,98,	0,0,	316,98,	314,98,	
315,98,	0,0,	317,98,	316,98,	
0,0,	315,98,	318,98,	317,98,	
316,98,	317,98,	319,98,	318,98,	
0,0,	318,98,	317,98,	319,98,	
0,0,	319,98,	318,98,	317,98,	
0,0,	320,98,	319,98,	318,98,	
316,332,	0,0,	320,98,	319,98,	
320,98,	0,0,	314,330,	0,0,	
0,0,	320,98,	0,0,	317,333,	
315,331,	321,98,	320,98,	318,334,	
322,98,	0,0,	321,98,	323,98,	
321,98,	322,98,	0,0,	322,98,	
323,98,	321,98,	323,98,	0,0,	
322,98,	0,0,	321,98,	323,98,	
324,98,	322,98,	319,335,	325,98,	
323,98,	324,98,	326,98,	324,98,	
325,98,	320,336,	325,98,	326,98,	
324,98,	326,98,	0,0,	325,98,	
0,0,	324,98,	326,98,	0,0,	
325,98,	0,0,	0,0,	326,98,	
0,0,	327,98,	321,337,	0,0,	
0,0,	322,338,	327,98,	328,98,	
327,98,	0,0,	324,340,	323,339,	
328,98,	327,98,	328,98,	326,342,	
329,98,	0,0,	327,98,	328,98,	
0,0,	329,98,	0,0,	329,98,	
328,98,	330,98,	0,0,	325,341,	
329,98,	0,0,	330,98,	331,98,	
330,98,	329,98,	327,343,	0,0,	
331,98,	330,98,	331,98,	332,98,	
328,344,	0,0,	330,98,	331,98,	
332,98,	333,98,	332,98,	0,0,	
331,98,	0,0,	333,98,	332,98,	
333,98,	0,0,	334,98,	0,0,	
332,98,	333,98,	0,0,	334,98,	
335,98,	334,98,	333,98,	0,0,	
329,345,	335,98,	334,98,	335,98,	
336,98,	0,0,	330,346,	334,98,	
335,98,	336,98,	337,98,	336,98,	
331,347,	335,98,	0,0,	337,98,	
336,98,	337,98,	0,0,	338,98,	
0,0,	336,98,	337,98,	332,348,	
338,98,	339,98,	338,98,	337,98,	
341,98,	333,349,	339,98,	338,98,	
339,98,	341,98,	0,0,	341,98,	
338,98,	339,98,	334,350,	340,98,	
341,98,	335,351,	339,98,	342,98,	
340,98,	341,98,	340,98,	0,0,	
342,98,	0,0,	342,98,	340,98,	
340,354,	0,0,	336,352,	342,98,	
340,98,	343,98,	0,0,	0,0,	
342,98,	0,0,	343,98,	0,0,	
343,98,	0,0,	0,0,	0,0,	
0,0,	343,98,	339,353,	344,98,	
345,98,	341,355,	343,98,	346,98,	
344,98,	345,98,	344,98,	345,98,	
346,98,	0,0,	346,98,	344,98,	
345,98,	0,0,	0,0,	346,98,	
344,98,	345,98,	347,98,	342,356,	
346,98,	0,0,	348,98,	347,98,	
0,0,	347,98,	0,0,	348,98,	
0,0,	348,98,	347,98,	349,98,	
0,0,	343,357,	348,98,	347,98,	
349,98,	350,98,	349,98,	348,98,	
0,0,	0,0,	350,98,	349,98,	
350,98,	345,359,	351,98,	344,358,	
349,98,	350,98,	0,0,	351,98,	
0,0,	351,98,	350,98,	0,0,	
0,0,	352,98,	351,98,	0,0,	
0,0,	0,0,	352,98,	351,98,	
352,98,	0,0,	353,98,	348,360,	
352,363,	352,98,	0,0,	353,98,	
354,98,	353,98,	352,98,	0,0,	
349,361,	354,98,	353,98,	354,98,	
355,98,	356,98,	350,362,	353,98,	
354,98,	355,98,	356,98,	355,98,	
356,98,	354,98,	0,0,	0,0,	
355,98,	356,98,	357,98,	358,98,	
0,0,	355,98,	356,98,	357,98,	
358,98,	357,98,	358,98,	359,98,	
354,364,	0,0,	357,98,	358,98,	
359,98,	0,0,	359,98,	357,98,	
358,98,	0,0,	0,0,	359,98,	
360,98,	361,98,	0,0,	0,0,	
359,98,	360,98,	361,98,	360,98,	
361,98,	362,98,	356,365,	0,0,	
360,98,	361,98,	362,98,	363,98,	
362,98,	360,98,	361,98,	0,0,	
363,98,	362,98,	363,98,	357,366,	
358,367,	0,0,	362,98,	363,98,	
364,98,	0,0,	0,0,	365,98,	
363,98,	364,98,	0,0,	364,98,	
365,98,	366,98,	365,98,	0,0,	
364,98,	0,0,	366,98,	365,98,	
366,98,	364,98,	363,368,	0,0,	
365,98,	366,98,	367,98,	0,0,	
0,0,	368,98,	366,98,	367,98,	
0,0,	367,98,	368,98,	369,98,	
368,98,	0,0,	367,98,	0,0,	
369,98,	368,98,	369,98,	367,98,	
364,369,	0,0,	368,98,	369,98,	
370,98,	371,98,	0,0,	0,0,	
369,98,	370,98,	371,98,	370,98,	
371,98,	0,0,	0,0,	0,0,	
370,98,	371,98,	368,370,	0,0,	
372,98,	370,98,	371,98,	373,98,	
374,98,	372,98,	369,371,	372,98,	
373,98,	374,98,	373,98,	374,98,	
372,98,	0,0,	0,0,	373,98,	
374,98,	372,98,	371,373,	375,98,	
373,98,	374,98,	0,0,	0,0,	
375,98,	0,0,	375,98,	0,0,	
0,0,	0,0,	375,376,	375,98,	
370,372,	0,0,	376,98,	377,98,	
375,98,	0,0,	0,0,	376,98,	
377,98,	376,98,	377,98,	378,98,	
0,0,	372,374,	376,98,	377,98,	
378,98,	379,98,	378,98,	376,98,	
377,98,	373,375,	379,98,	378,98,	
379,98,	380,98,	0,0,	0,0,	
378,98,	379,98,	380,98,	0,0,	
380,98,	376,377,	379,98,	0,0,	
377,378,	380,98,	0,0,	0,0,	
0,0,	0,0,	380,98,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	378,379,	
0,0,	0,0,	379,380,	0,0,	
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
yycrank+268,	yysvec+76,	yyvstop+210,
yycrank+368,	yysvec+76,	yyvstop+213,
yycrank+374,	yysvec+76,	yyvstop+216,
yycrank+387,	yysvec+76,	yyvstop+219,
yycrank+-458,	0,		yyvstop+222,
yycrank+211,	yysvec+10,	yyvstop+225,
yycrank+51,	yysvec+10,	yyvstop+227,
yycrank+0,	yysvec+10,	yyvstop+230,
yycrank+202,	yysvec+10,	yyvstop+233,
yycrank+0,	yysvec+10,	yyvstop+235,
yycrank+206,	yysvec+10,	yyvstop+238,
yycrank+197,	0,		0,	
yycrank+206,	0,		0,	
yycrank+204,	yysvec+10,	yyvstop+240,
yycrank+206,	yysvec+10,	yyvstop+242,
yycrank+423,	0,		0,	
yycrank+209,	yysvec+10,	yyvstop+244,
yycrank+204,	yysvec+10,	yyvstop+246,
yycrank+206,	yysvec+10,	yyvstop+248,
yycrank+230,	yysvec+10,	yyvstop+250,
yycrank+231,	yysvec+10,	yyvstop+252,
yycrank+452,	yysvec+10,	yyvstop+254,
yycrank+232,	yysvec+10,	yyvstop+257,
yycrank+226,	yysvec+10,	yyvstop+259,
yycrank+240,	yysvec+10,	yyvstop+261,
yycrank+454,	0,		0,	
yycrank+463,	0,		0,	
yycrank+431,	yysvec+76,	yyvstop+263,
yycrank+437,	yysvec+76,	yyvstop+266,
yycrank+446,	yysvec+76,	yyvstop+269,
yycrank+461,	yysvec+76,	yyvstop+272,
yycrank+467,	yysvec+76,	yyvstop+275,
yycrank+0,	0,		yyvstop+278,
yycrank+258,	yysvec+10,	yyvstop+281,
yycrank+469,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+283,
yycrank+0,	yysvec+10,	yyvstop+286,
yycrank+230,	0,		0,	
yycrank+234,	0,		0,	
yycrank+244,	yysvec+10,	yyvstop+289,
yycrank+235,	yysvec+10,	yyvstop+291,
yycrank+240,	0,		0,	
yycrank+236,	yysvec+10,	yyvstop+293,
yycrank+242,	yysvec+10,	yyvstop+295,
yycrank+253,	yysvec+10,	yyvstop+297,
yycrank+251,	yysvec+10,	yyvstop+299,
yycrank+0,	yysvec+10,	yyvstop+301,
yycrank+516,	0,		0,	
yycrank+258,	yysvec+10,	yyvstop+304,
yycrank+0,	yysvec+10,	yyvstop+306,
yycrank+269,	yysvec+10,	yyvstop+309,
yycrank+263,	yysvec+10,	yyvstop+311,
yycrank+557,	0,		0,	
yycrank+485,	yysvec+76,	yyvstop+313,
yycrank+513,	yysvec+76,	yyvstop+316,
yycrank+498,	yysvec+76,	yyvstop+319,
yycrank+614,	yysvec+76,	yyvstop+322,
yycrank+623,	yysvec+76,	yyvstop+325,
yycrank+293,	yysvec+10,	yyvstop+328,
yycrank+283,	0,		0,	
yycrank+298,	0,		0,	
yycrank+288,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+330,
yycrank+288,	yysvec+10,	yyvstop+333,
yycrank+319,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+335,
yycrank+323,	yysvec+10,	yyvstop+338,
yycrank+0,	yysvec+10,	yyvstop+340,
yycrank+343,	yysvec+10,	yyvstop+343,
yycrank+340,	0,		0,	
yycrank+338,	yysvec+10,	yyvstop+345,
yycrank+356,	yysvec+10,	yyvstop+347,
yycrank+0,	yysvec+10,	yyvstop+349,
yycrank+563,	0,		0,	
yycrank+672,	0,		0,	
yycrank+620,	yysvec+76,	yyvstop+352,
yycrank+642,	yysvec+76,	yyvstop+355,
yycrank+645,	yysvec+76,	yyvstop+358,
yycrank+651,	yysvec+76,	yyvstop+361,
yycrank+664,	yysvec+76,	yyvstop+364,
yycrank+667,	yysvec+76,	yyvstop+367,
yycrank+675,	yysvec+76,	yyvstop+370,
yycrank+678,	yysvec+76,	yyvstop+373,
yycrank+686,	yysvec+76,	yyvstop+376,
yycrank+704,	yysvec+76,	yyvstop+379,
yycrank+718,	yysvec+76,	yyvstop+382,
yycrank+722,	yysvec+76,	yyvstop+385,
yycrank+726,	yysvec+76,	yyvstop+388,
yycrank+384,	yysvec+10,	yyvstop+391,
yycrank+362,	0,		0,	
yycrank+0,	0,		yyvstop+393,
yycrank+355,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+395,
yycrank+0,	0,		yyvstop+398,
yycrank+0,	yysvec+10,	yyvstop+400,
yycrank+374,	yysvec+10,	yyvstop+403,
yycrank+359,	0,		0,	
yycrank+0,	yysvec+10,	yyvstop+405,
yycrank+0,	yysvec+10,	yyvstop+408,
yycrank+565,	0,		0,	
yycrank+737,	yysvec+76,	yyvstop+411,
yycrank+740,	yysvec+76,	yyvstop+414,
yycrank+758,	yysvec+76,	yyvstop+417,
yycrank+759,	yysvec+76,	yyvstop+420,
yycrank+762,	yysvec+76,	yyvstop+423,
yycrank+777,	yysvec+76,	yyvstop+426,
yycrank+780,	yysvec+76,	yyvstop+429,
yycrank+796,	yysvec+76,	yyvstop+432,
yycrank+799,	yysvec+76,	yyvstop+435,
yycrank+805,	yysvec+76,	yyvstop+438,
yycrank+814,	yysvec+76,	yyvstop+441,
yycrank+818,	yysvec+76,	yyvstop+444,
yycrank+829,	yysvec+76,	yyvstop+447,
yycrank+396,	yysvec+10,	yyvstop+450,
yycrank+894,	0,		0,	
yycrank+0,	0,		yyvstop+452,
yycrank+0,	yysvec+10,	yyvstop+454,
yycrank+0,	0,		yyvstop+457,
yycrank+370,	0,		0,	
yycrank+842,	yysvec+76,	yyvstop+459,
yycrank+951,	yysvec+76,	yyvstop+462,
yycrank+856,	yysvec+76,	yyvstop+465,
yycrank+952,	yysvec+76,	yyvstop+468,
yycrank+955,	yysvec+76,	yyvstop+471,
yycrank+966,	yysvec+76,	yyvstop+474,
yycrank+970,	yysvec+76,	yyvstop+477,
yycrank+981,	yysvec+76,	yyvstop+480,
yycrank+985,	yysvec+76,	yyvstop+483,
yycrank+1003,	yysvec+76,	yyvstop+486,
yycrank+1006,	yysvec+76,	yyvstop+489,
yycrank+1009,	yysvec+76,	yyvstop+492,
yycrank+1022,	yysvec+76,	yyvstop+495,
yycrank+0,	yysvec+10,	yyvstop+498,
yycrank+722,	0,		0,	
yycrank+0,	0,		yyvstop+501,
yycrank+377,	0,		0,	
yycrank+1025,	yysvec+76,	yyvstop+503,
yycrank+1040,	yysvec+76,	yyvstop+506,
yycrank+1044,	yysvec+76,	yyvstop+509,
yycrank+1053,	yysvec+76,	yyvstop+512,
yycrank+1059,	yysvec+76,	yyvstop+515,
yycrank+1062,	yysvec+76,	yyvstop+518,
yycrank+1089,	yysvec+76,	yyvstop+521,
yycrank+1075,	yysvec+76,	yyvstop+524,
yycrank+1081,	yysvec+76,	yyvstop+527,
yycrank+1102,	yysvec+76,	yyvstop+530,
yycrank+1103,	yysvec+76,	yyvstop+533,
yycrank+1111,	yysvec+76,	yyvstop+536,
yycrank+1117,	yysvec+76,	yyvstop+539,
yycrank+383,	0,		0,	
yycrank+1125,	yysvec+76,	yyvstop+542,
yycrank+1133,	yysvec+76,	yyvstop+545,
yycrank+1146,	yysvec+76,	yyvstop+549,
yycrank+1147,	yysvec+76,	yyvstop+552,
yycrank+1160,	yysvec+76,	yyvstop+555,
yycrank+1168,	yysvec+76,	yyvstop+558,
yycrank+481,	0,		0,	
yycrank+-1239,	0,		0,	
yycrank+1181,	yysvec+76,	yyvstop+561,
yycrank+1212,	yysvec+76,	yyvstop+564,
yycrank+1213,	yysvec+76,	yyvstop+568,
yycrank+1227,	yysvec+76,	yyvstop+571,
yycrank+1228,	yysvec+76,	yyvstop+574,
yycrank+1236,	yysvec+76,	yyvstop+577,
yycrank+812,	0,		0,	
yycrank+1242,	yysvec+76,	yyvstop+580,
yycrank+1250,	yysvec+76,	yyvstop+583,
yycrank+1269,	yysvec+76,	yyvstop+586,
yycrank+1287,	yysvec+76,	yyvstop+589,
yycrank+1283,	yysvec+76,	yyvstop+592,
yycrank+1296,	yysvec+76,	yyvstop+595,
yycrank+0,	0,		yyvstop+598,
yycrank+1302,	yysvec+76,	yyvstop+600,
yycrank+1305,	yysvec+76,	yyvstop+603,
yycrank+1318,	yysvec+76,	yyvstop+606,
yycrank+1329,	yysvec+76,	yyvstop+609,
yycrank+1358,	yysvec+76,	yyvstop+612,
yycrank+753,	0,		0,	
yycrank+1347,	yysvec+76,	yyvstop+615,
yycrank+1361,	yysvec+76,	yyvstop+618,
yycrank+1367,	yysvec+76,	yyvstop+621,
yycrank+1376,	yysvec+76,	yyvstop+624,
yycrank+1389,	yysvec+76,	yyvstop+627,
yycrank+1397,	yysvec+76,	yyvstop+630,
yycrank+1405,	yysvec+76,	yyvstop+633,
yycrank+1411,	yysvec+76,	yyvstop+636,
yycrank+1420,	yysvec+76,	yyvstop+639,
yycrank+1426,	yysvec+76,	yyvstop+642,
yycrank+1434,	yysvec+76,	yyvstop+645,
yycrank+1435,	yysvec+76,	yyvstop+648,
yycrank+1450,	yysvec+76,	yyvstop+651,
yycrank+1463,	yysvec+76,	yyvstop+654,
yycrank+1466,	yysvec+76,	yyvstop+657,
yycrank+1479,	yysvec+76,	yyvstop+660,
yycrank+1482,	yysvec+76,	yyvstop+663,
yycrank+0,	0,		yyvstop+666,
yycrank+1488,	yysvec+76,	yyvstop+668,
yycrank+1497,	yysvec+76,	yyvstop+671,
yycrank+1503,	yysvec+76,	yyvstop+674,
yycrank+1511,	yysvec+76,	yyvstop+677,
yycrank+1512,	yysvec+76,	yyvstop+680,
yycrank+1527,	yysvec+76,	yyvstop+683,
yycrank+1540,	yysvec+76,	yyvstop+686,
yycrank+1548,	yysvec+76,	yyvstop+689,
yycrank+1549,	yysvec+76,	yyvstop+692,
yycrank+1562,	yysvec+76,	yyvstop+695,
yycrank+1563,	yysvec+76,	yyvstop+698,
yycrank+1577,	yysvec+76,	yyvstop+701,
yycrank+1581,	yysvec+76,	yyvstop+704,
yycrank+1599,	yysvec+76,	yyvstop+707,
yycrank+1608,	yysvec+76,	yyvstop+710,
yycrank+1617,	yysvec+76,	yyvstop+713,
yycrank+1621,	yysvec+76,	yyvstop+716,
yycrank+1635,	yysvec+76,	yyvstop+719,
yycrank+1644,	yysvec+76,	yyvstop+723,
yycrank+1650,	yysvec+76,	yyvstop+726,
yycrank+1653,	yysvec+76,	yyvstop+729,
yycrank+1664,	yysvec+76,	yyvstop+732,
yycrank+1668,	yysvec+76,	yyvstop+735,
yycrank+1672,	yysvec+76,	yyvstop+738,
yycrank+1683,	yysvec+76,	yyvstop+741,
yycrank+1699,	yysvec+76,	yyvstop+744,
yycrank+1702,	yysvec+76,	yyvstop+747,
yycrank+1705,	yysvec+76,	yyvstop+750,
yycrank+1718,	yysvec+76,	yyvstop+753,
yycrank+1721,	yysvec+76,	yyvstop+756,
yycrank+1724,	yysvec+76,	yyvstop+759,
yycrank+1743,	yysvec+76,	yyvstop+762,
yycrank+1749,	yysvec+76,	yyvstop+765,
yycrank+1758,	yysvec+76,	yyvstop+768,
yycrank+1767,	yysvec+76,	yyvstop+771,
yycrank+1773,	yysvec+76,	yyvstop+774,
yycrank+1781,	yysvec+76,	yyvstop+777,
yycrank+1787,	yysvec+76,	yyvstop+780,
yycrank+1796,	yysvec+76,	yyvstop+783,
yycrank+1802,	yysvec+76,	yyvstop+786,
yycrank+1810,	yysvec+76,	yyvstop+789,
yycrank+1816,	yysvec+76,	yyvstop+792,
yycrank+1825,	yysvec+76,	yyvstop+796,
yycrank+1831,	yysvec+76,	yyvstop+800,
yycrank+1845,	yysvec+76,	yyvstop+803,
yycrank+1834,	yysvec+76,	yyvstop+806,
yycrank+1849,	yysvec+76,	yyvstop+809,
yycrank+1863,	yysvec+76,	yyvstop+812,
yycrank+1877,	yysvec+76,	yyvstop+815,
yycrank+1878,	yysvec+76,	yyvstop+818,
yycrank+1881,	yysvec+76,	yyvstop+821,
yycrank+1896,	yysvec+76,	yyvstop+825,
yycrank+1900,	yysvec+76,	yyvstop+829,
yycrank+1909,	yysvec+76,	yyvstop+832,
yycrank+1915,	yysvec+76,	yyvstop+835,
yycrank+1924,	yysvec+76,	yyvstop+838,
yycrank+1935,	yysvec+76,	yyvstop+842,
yycrank+1944,	yysvec+76,	yyvstop+845,
yycrank+1950,	yysvec+76,	yyvstop+849,
yycrank+1958,	yysvec+76,	yyvstop+852,
yycrank+1959,	yysvec+76,	yyvstop+856,
yycrank+1972,	yysvec+76,	yyvstop+859,
yycrank+1973,	yysvec+76,	yyvstop+862,
yycrank+1981,	yysvec+76,	yyvstop+865,
yycrank+1994,	yysvec+76,	yyvstop+869,
yycrank+1995,	yysvec+76,	yyvstop+873,
yycrank+2003,	yysvec+76,	yyvstop+877,
yycrank+2009,	yysvec+76,	yyvstop+881,
yycrank+2022,	yysvec+76,	yyvstop+884,
yycrank+2025,	yysvec+76,	yyvstop+887,
yycrank+2031,	yysvec+76,	yyvstop+891,
yycrank+2044,	yysvec+76,	yyvstop+895,
yycrank+2047,	yysvec+76,	yyvstop+899,
yycrank+2053,	yysvec+76,	yyvstop+902,
yycrank+2066,	yysvec+76,	yyvstop+905,
yycrank+2067,	yysvec+76,	yyvstop+908,
yycrank+2082,	yysvec+76,	yyvstop+911,
yycrank+2085,	yysvec+76,	yyvstop+914,
yycrank+2086,	yysvec+76,	yyvstop+917,
yycrank+2101,	yysvec+76,	yyvstop+921,
yycrank+2116,	yysvec+76,	yyvstop+924,
yycrank+2117,	yysvec+76,	yyvstop+927,
yycrank+2125,	yysvec+76,	yyvstop+930,
yycrank+2131,	yysvec+76,	yyvstop+933,
yycrank+2139,	yysvec+76,	yyvstop+936,
0,	0,	0};
struct yywork *yytop = yycrank+2242;
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
0,0,0,1,0,0,0,0,
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
