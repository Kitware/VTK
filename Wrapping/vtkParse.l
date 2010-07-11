%option yylineno
%{

/*

This file must be translated to C and modified to build everywhere.

Run flex like this:

  flex --nounput -olex.yy.c vtkParse.l

Modify lex.yy.c:
  - convert tabs to spaces (8 spaces per tab)
  - remove extra space from end of lines
  - remove blank lines from end of file
  - change yy_n_chars declarations from "int yy_n_chars;" to
    "size_t yy_n_chars;" in both the yy_buffer_state structure
    and the global scope.
  - in YY_INPUT change "int n" to "size_t n"
  - compile with gcc and "-Wsign-compare", there should be no warnings

*/

/* We do not care of interactive mode */
#define YY_NEVER_INTERACTIVE 1
#define YY_NO_UNPUT 1

/* Do not include unistd.h in generated source. */
#define YY_NO_UNISTD_H

/* Skip declaring this function.  It is a macro.  */
#define YY_SKIP_YYWRAP

#ifdef _WIN32
#pragma warning ( disable : 4018 )
#pragma warning ( disable : 4127 )
#pragma warning ( disable : 4131 )
#pragma warning ( disable : 4244 )
#pragma warning ( disable : 4251 )
#pragma warning ( disable : 4267 )
#pragma warning ( disable : 4305 )
#pragma warning ( disable : 4309 )
#pragma warning ( disable : 4706 )
#pragma warning ( disable : 4786 )
#endif

#define SKIP_MATCH_MAXLEN 15

/*
 * Skip ahead until one of the strings is found,
 * then skip to the end of the line.
 * Return 0 if no match found.
 */
static int skip_ahead_multi(const char *strings[]);
static int skip_ahead_until(const char *text);

%}


%%

"/*" { int c1 = 0, c2 = input();
       for (;;)
         {
         if (c2 == 0)
           {
           fprintf(yyout,"Cannot find closing comment.\n");
           break;
           }
         if (c1 == '*' && c2 == '/') break;
         c1 = c2; c2 = input();
         }
     };

^[\t ]*"//BTX".*$ {
       if (!IgnoreBTX) {
         skip_ahead_until("//ETX");
       }
     };

^[\t ]*"//ETX".*$ ;


^[\t ]*"#ifdef"[\t ]*"VTK_WORKAROUND_WINDOWS_MANGLE".*$ {
       const char *strings[] = {"#endif", "#elif", "#else", NULL};
       skip_ahead_multi(strings);
     };


^[\t ]*"#ifdef "("WIN32"|"_WIN32").*$ {
#ifndef _WIN32
       const char *strings[] = {"#endif", "#elif", "#else", NULL};
       skip_ahead_multi(strings);
#endif
     };

^[\t ]*"#ifdef VTK_USE_64BIT_IDS".*$ {
#ifndef VTK_USE_64BIT_IDS
       const char *strings[] = {"#endif", "#elif", "#else", NULL};
       skip_ahead_multi(strings);
#endif
     };

^[\t ]*"#"("else"|"elif").*$ {
       skip_ahead_until("#endif");
     };

^[\t ]*("@interface"|"@implementation") {
       skip_ahead_until("@end");
     };

^[\t ]*"// .NAME".* {
     size_t pos = 1;
     while (yytext[pos-1] != 'M' || yytext[pos] != 'E')
       {
       pos++;
       }
     data.NameComment = vtkstrndup(&yytext[pos + 1], yyleng - pos - 1);
     };

^[\t ]*"// Description:".*         { setCommentState(1); };
^[\t ]*"// .SECTION Description".* { setCommentState(2); };
^[\t ]*"// .SECTION See Also".*    { setCommentState(3); };
^[\t ]*"// .SECTION see also".*    { setCommentState(3); };
^[\t ]*"// .SECTION Caveats".*     { setCommentState(4); };
^[\t ]*[\r\n]                      { closeOrClearComment(); };
^[\t ]*"//".*  {
     size_t pos = 2;
     while (yytext[pos-2] != '/' || yytext[pos-1] != '/') pos++;
     addCommentLine(&yytext[pos], yyleng - pos);
     };

"//".* ;

^[\t ]*"#"[\t ]*"define"[\t ]+"VTK_"[a-zA-Z0-9_]+.*$ {
     const char *name;
     size_t i = 0;
     size_t j = 0;
     while (yytext[i] != 'V') { i++; }
     j = i;
     while ((yytext[j] >= 'a' && yytext[j] <= 'z') ||
            (yytext[j] >= 'A' && yytext[j] <= 'Z') ||
            (yytext[j] >= '0' && yytext[j] <= '9') ||
            yytext[j] == '_') { j++; }
     name = vtkstrndup(&yytext[i], j-i);
     while (yytext[j] == ' ' || yytext[j] == '\t') { j++; }
     i = yyleng;
     while (i > 0 && (yytext[i-1] == '\n' || yytext[i-1] == '\r' ||
                      yytext[i-1] == '\t' || yytext[i-1] == ' ')) { i--; }
     if (i > j)
       {
       add_constant(name, vtkstrndup(&yytext[j], i-j), 0, NULL, 1);
       }
     else
       {
       add_constant(name, "", 0, NULL, 1);
       }
     };

^[\t ]*"#"(\\\n|\\\r\n|[^\n])*$     ;

\"([^\"]|\\\")*\"      {
                yylval.str = vtkstrndup(yytext, yyleng);
                return(STRING_LITERAL); }

\'([^\']|\\\')+\'      {
                yylval.str = vtkstrndup(yytext, yyleng);
                return(CHAR_LITERAL); }

[A-Za-z0-9_]*"_EXPORT" ;

"vtkNotUsed"[ ]*"("[^)]*")" {
     size_t i; size_t j = 0;
     while (yytext[j]!='(') { j++; }
     j++; i = j;
     while (yytext[j]!=')') { j++; }
     yylval.str = vtkstrndup(&yytext[i], j-i);
     return(ID);
     };

("friend"[\t\n\r ]+)?("template"[\t\n\r ]*"<"[^>]*">"[\t\n\r ]*)?"class"[\t\n\r ]+[a-zA-Z_][a-zA-Z0-9_]*("::"[a-zA-Z_][a-zA-Z0-9_]*)*[\t\n\r ]*";" {
     return(CLASS_REF);
     };

("friend"[\t\n\r ]+)?("template"[\t\n\r ]*"<"[^>]*">"[\t\n\r ]*)?"struct"[\t\n\r ]+[a-zA-Z_][a-zA-Z0-9_]*("::"[a-zA-Z_][a-zA-Z0-9_]*)*[\t\n\r ]*";" {
     return(CLASS_REF);
     };

"void"[\t\n\r ]*"("[\t\n\r ]*"*"[a-zA-Z0-9_]*[\t\n\r ]*")"[\t\n\r ]*"("[\t\n\r ]*"void"[\t\n\r ]*"*"[\n\t ]*")" {
     size_t i = 0; size_t j = 0;
     while (yytext[j] != '*') { i++; j++; }
     i++; j++;
     while (yytext[i] != ' ' && yytext[i] != '\t' &&
            yytext[i] != '\n' && yytext[i] != '\r' && yytext[i] != ')') {i++;}
     yylval.str = vtkstrndup(&yytext[j], i-j);
     return(VAR_FUNCTION);
     };

"long"[\t\n\r ]*"double"                       return(LONG_DOUBLE);
"double"                                       return(DOUBLE);
"float"                                        return(FLOAT);

"unsigned"[\t\n\r ]*"__int64"                  return(UNSIGNED_INT64__);
"__int64"[\t\n\r ]*"unsigned"                  return(UNSIGNED_INT64__);

"signed"[\t\n\r ]*"__int64"                    return(INT64__);
"__int64"[\t\n\r ]*"signed"                    return(INT64__);
"__int64"                                      return(INT64__);

"int"[\t\n\r ]*"unsigned"[\t\n\r ]*"long"[\t\n\r ]*"long" return(UNSIGNED_LONG_LONG);
"unsigned"[\t\n\r ]*"int"[\t\n\r ]*"long"[\t\n\r ]*"long" return(UNSIGNED_LONG_LONG);
"long"[\t\n\r ]*"long"[\t\n\r ]*"unsigned"[\t\n\r ]*"int" return(UNSIGNED_LONG_LONG);
"long"[\t\n\r ]*"long"[\t\n\r ]*"int"[\t\n\r ]*"unsigned" return(UNSIGNED_LONG_LONG);
"int"[\t\n\r ]*"long"[\t\n\r ]*"long"[\t\n\r ]*"unsigned" return(UNSIGNED_LONG_LONG);
"unsigned"[\t\n\r ]*"long"[\t\n\r ]*"long"[\t\n\r ]*"int" return(UNSIGNED_LONG_LONG);
"unsigned"[\t\n\r ]*"long"[\t\n\r ]*"long"     return(UNSIGNED_LONG_LONG);
"long"[\t\n\r ]*"long"[\t\n\r ]*"unsigned"     return(UNSIGNED_LONG_LONG);

"int"[\t\n\r ]*"signed"[\t\n\r ]*"long"[\t\n\r ]*"long"  return(LONG_LONG);
"signed"[\t\n\r ]*"int"[\t\n\r ]*"long"[\t\n\r ]*"long"  return(LONG_LONG);
"long"[\t\n\r ]*"long"[\t\n\r ]*"signed"[\t\n\r ]*"int"  return(LONG_LONG);
"long"[\t\n\r ]*"long"[\t\n\r ]*"int"[\t\n\r ]*"signed"  return(LONG_LONG);
"int"[\t\n\r ]*"long"[\t\n\r ]*"long"[\t\n\r ]*"signed"  return(LONG_LONG);
"signed"[\t\n\r ]*"long"[\t\n\r ]*"long"[\t\n\r ]*"int"  return(LONG_LONG);
"signed"[\t\n\r ]*"long"[\t\n\r ]*"long"       return(LONG_LONG);
"long"[\t\n\r ]*"long"[\t\n\r ]*"signed"       return(LONG_LONG);
"int"[\t\n\r ]*"long"[\t\n\r ]*"long"          return(LONG_LONG);
"long"[\t\n\r ]*"long"[\t\n\r ]*"int"          return(LONG_LONG);
"long"[\t\n\r ]*"long"                         return(LONG_LONG);

"int"[\t\n\r ]*"unsigned"[\t\n\r ]*"short"     return(UNSIGNED_SHORT);
"unsigned"[\t\n\r ]*"int"[\t\n\r ]*"short"     return(UNSIGNED_SHORT);
"short"[\t\n\r ]*"unsigned"[\t\n\r ]*"int"     return(UNSIGNED_SHORT);
"short"[\t\n\r ]*"int"[\t\n\r ]*"unsigned"     return(UNSIGNED_SHORT);
"int"[\t\n\r ]*"short"[\t\n\r ]*"unsigned"     return(UNSIGNED_SHORT);
"unsigned"[\t\n\r ]*"short"[\t\n\r ]*"int"     return(UNSIGNED_SHORT);
"unsigned"[\t\n\r ]*"short"                    return(UNSIGNED_SHORT);
"short"[\t\n\r ]*"unsigned"                    return(UNSIGNED_SHORT);

"int"[\t\n\r ]*"signed"[\t\n\r ]*"short"       return(SHORT);
"signed"[\t\n\r ]*"int"[\t\n\r ]*"short"       return(SHORT);
"short"[\t\n\r ]*"signed"[\t\n\r ]*"int"       return(SHORT);
"short"[\t\n\r ]*"int"[\t\n\r ]*"signed"       return(SHORT);
"int"[\t\n\r ]*"short"[\t\n\r ]*"signed"       return(SHORT);
"signed"[\t\n\r ]*"short"[\t\n\r ]*"int"       return(SHORT);
"signed"[\t\n\r ]*"short"                      return(SHORT);
"short"[\t\n\r ]*"signed"                      return(SHORT);
"int"[\t\n\r ]*"short"                         return(SHORT);
"short"[\t\n\r ]*"int"                         return(SHORT);
"short"                                        return(SHORT);

"int"[\t\n\r ]*"unsigned"[\t\n\r ]*"long"      return(UNSIGNED_LONG);
"unsigned"[\t\n\r ]*"int"[\t\n\r ]*"long"      return(UNSIGNED_LONG);
"long"[\t\n\r ]*"unsigned"[\t\n\r ]*"int"      return(UNSIGNED_LONG);
"long"[\t\n\r ]*"int"[\t\n\r ]*"unsigned"      return(UNSIGNED_LONG);
"int"[\t\n\r ]*"long"[\t\n\r ]*"unsigned"      return(UNSIGNED_LONG);
"unsigned"[\t\n\r ]*"long"[\t\n\r ]*"int"      return(UNSIGNED_LONG);
"unsigned"[\t\n\r ]*"long"                     return(UNSIGNED_LONG);
"long"[\t\n\r ]*"unsigned"                     return(UNSIGNED_LONG);

"int"[\t\n\r ]*"signed"[\t\n\r ]*"long"        return(LONG);
"signed"[\t\n\r ]*"int"[\t\n\r ]*"long"        return(LONG);
"long"[\t\n\r ]*"signed"[\t\n\r ]*"int"        return(LONG);
"long"[\t\n\r ]*"int"[\t\n\r ]*"signed"        return(LONG);
"int"[\t\n\r ]*"long"[\t\n\r ]*"signed"        return(LONG);
"signed"[\t\n\r ]*"long"[\t\n\r ]*"int"        return(LONG);
"signed"[\t\n\r ]*"long"                       return(LONG);
"long"[\t\n\r ]*"signed"                       return(LONG);
"int"[\t\n\r ]*"long"                          return(LONG);
"long"[\t\n\r ]*"int"                          return(LONG);
"long"                                         return(LONG);

"unsigned"[\t\n\r ]*"char"                     return(UNSIGNED_CHAR);
"char"[\t\n\r ]*"unsigned"                     return(UNSIGNED_CHAR);
"char"[\t\n\r ]*"signed"                       return(SIGNED_CHAR);
"signed"[\t\n\r ]*"char"                       return(SIGNED_CHAR);
"char"                                         return(CHAR);

"unsigned"[\t\n\r ]*"int"                      return(UNSIGNED_INT);
"int"[\t\n\r ]*"unsigned"                      return(UNSIGNED_INT);

"signed"[\t\n\r ]*"int"                        return(INT);
"int"[\t\n\r ]*"signed"                        return(INT);
"int"                                          return(INT);

"unsigned"      return(UNSIGNED);
"signed"        return(SIGNED);

"void"          return(VOID);
"bool"          return(BOOL);

"size_t"        return(SIZE_T);
"ssize_t"       return(SSIZE_T);

"class"         return(CLASS);
"struct"        return(STRUCT);
"public"        return(PUBLIC);
"private"       return(PRIVATE);
"protected"     return(PROTECTED);
"enum"          return(ENUM);
"union"         return(UNION);
"virtual"       return(VIRTUAL);
"const"         return(CONST);
"mutable"       return(MUTABLE);
"operator"      return(OPERATOR);
"friend"        return(FRIEND);
"inline"        return(INLINE);
"static"        return(STATIC);
"extern"        return(EXTERN);
"template"      return(TEMPLATE);
"typename"      return(TYPENAME);
"typedef"       return(TYPEDEF);
"namespace"     return(NAMESPACE);
"using"         return(USING);
"new"           return(NEW);
"delete"        return(DELETE);

"vtkFloatingPointType" return(DOUBLE);
"vtkIdType"            return(IdType);
"vtkSetMacro"          return(SetMacro);
"vtkGetMacro"          return(GetMacro);
"vtkSetStringMacro"    return(SetStringMacro);
"vtkGetStringMacro"    return(GetStringMacro);
"vtkSetClampMacro"     return(SetClampMacro);
"vtkSetObjectMacro"    return(SetObjectMacro);
"vtkGetObjectMacro"    return(GetObjectMacro);
"vtkBooleanMacro"      return(BooleanMacro);
"vtkSetVector2Macro"   return(SetVector2Macro);
"vtkSetVector3Macro"   return(SetVector3Macro);
"vtkSetVector4Macro"   return(SetVector4Macro);
"vtkSetVector6Macro"   return(SetVector6Macro);
"vtkGetVector2Macro"   return(GetVector2Macro);
"vtkGetVector3Macro"   return(GetVector3Macro);
"vtkGetVector4Macro"   return(GetVector4Macro);
"vtkGetVector6Macro"   return(GetVector6Macro);
"vtkSetVectorMacro"    return(SetVectorMacro);
"vtkGetVectorMacro"    return(GetVectorMacro);
"vtkViewportCoordinateMacro" return(ViewportCoordinateMacro);
"vtkWorldCoordinateMacro" return(WorldCoordinateMacro);
"vtkExportedTypeMacro" return(TypeMacro);
"vtkExportedTypeRevisionMacro" return(TypeMacro);
"vtkTypeMacro"         return(TypeMacro);
"vtkTypeRevisionMacro" return(TypeMacro);
"VTK_LEGACY"           return(VTK_LEGACY);
"VTK_WRAP_EXTERN"      ;
"VTK_BYTE_SWAP_DECL"   return(VTK_BYTE_SWAP_DECL);
"vtkTypeInt8"          return(TypeInt8);
"vtkTypeUInt8"         return(TypeUInt8);
"vtkTypeInt16"         return(TypeInt16);
"vtkTypeUInt16"        return(TypeUInt16);
"vtkTypeInt32"         return(TypeInt32);
"vtkTypeUInt32"        return(TypeUInt32);
"vtkTypeInt64"         return(TypeInt64);
"vtkTypeUInt64"        return(TypeUInt64);
"vtkTypeFloat32"       return(TypeFloat32);
"vtkTypeFloat64"       return(TypeFloat64);

"*"[\r\n\t ]*"const"   return(CONST_PTR);
"const"[\r\n\t ]*"="   return(CONST_EQUAL);

"("[\t\n\r ]*([a-zA-Z_][a-zA-Z0-9_]*::)*"&" {
                size_t i = 1;
                size_t j;
                while (yytext[i]==' ' || yytext[i]=='\t' ||
                       yytext[i]=='\r' || yytext[i]=='\n') { i++; }
                j = i;
                while (yytext[j]!='&') { j++; }
                yylval.str = vtkstrndup(&yytext[i], j-i);
                return(LA); }

"("[\t\n\r ]*([a-zA-Z_][a-zA-Z0-9_]*::)*"*" {
                size_t i = 1;
                size_t j;
                while (yytext[i]==' ' || yytext[i]=='\t' ||
                       yytext[i]=='\r' || yytext[i]=='\n') { i++; }
                j = i;
                while (yytext[j]!='*') { j++; }
                yylval.str = vtkstrndup(&yytext[i], j-i);
                return(LP); }

"("[\t\n\r ]*("APIENTRY"|"CALLBACK"|"WINAPI")[\t\n\r ]*"*" {
                yylval.str = "";
                return(LP); }

"("[\t\n\r ]*("APIENTRYP"|"CALLBACKP"|"WINAPIP") {
                yylval.str = "";
                return(LP); }

("APIENTRYP"|"CALLBACKP"|"WINAPIP") { return('*'); }

"APIENTRY" ;
"CALLBACK" ;
"WINAPI" ;

("vtkstd::"|"std::")?"ostream" {
                     yylval.str = vtkstrndup(yytext, yyleng);
                     return(OSTREAM); }

("vtkstd::"|"std::")?"istream" {
                     yylval.str = vtkstrndup(yytext, yyleng);
                     return(ISTREAM); }

("vtkStdString"|("vtkstd::"|"std::")?"string") {
                     yylval.str = vtkstrndup(yytext, yyleng);
                     return(StdString); }

"vtkUnicodeString" { yylval.str = vtkstrndup(yytext, yyleng);
                     return(UnicodeString); }

"vtk"[0-9a-zA-Z_]* { yylval.str = vtkstrndup(yytext, yyleng);
                     return(VTK_ID); }

[a-zA-Z_][0-9a-zA-Z_]*  { yylval.str = vtkstrndup(yytext, yyleng);
                          return(ID); }

[0-9]+"."[0-9]*([eE][+-]?[0-9]+)?[a-zA-Z_]*  {
                     yylval.str = vtkstrndup(yytext, yyleng);
                     return(FLOAT_LITERAL); }

"0"[xX][0-9a-fA-F]+[g-zG-Z_]*      {
                     yylval.str = vtkstrndup(yytext, yyleng);
                     return(HEX_LITERAL); }

"0"[0-9]+[a-zA-Z_]*   {
                     yylval.str = vtkstrndup(yytext, yyleng);
                     return(OCT_LITERAL); }

[1-9][0-9]*[a-zA-Z_]* {
                     yylval.str = vtkstrndup(yytext, yyleng);
                     return(INT_LITERAL); }

"0"                { yylval.str = vtkstrndup(yytext, yyleng);
                     return(ZERO); };

[\t\n\r ] ;

"<<="             return(OP_LSHIFT_EQ);
">>="             return(OP_RSHIFT_EQ);
"<<"              return(OP_LSHIFT);
"->*"             return(OP_ARROW_POINTER);
"->"              return(OP_ARROW);
">>"              return(OP_RSHIFT);
"++"              return(OP_INCR);
"--"              return(OP_DECR);
"+="              return(OP_PLUS_EQ);
"-="              return(OP_MINUS_EQ);
"*="              return(OP_TIMES_EQ);
"/="              return(OP_DIVIDE_EQ);
"%="              return(OP_REMAINDER_EQ);
"&="              return(OP_AND_EQ);
"|="              return(OP_OR_EQ);
"^="              return(OP_XOR_EQ);
"&&="             return(OP_LOGIC_AND_EQ);
"||="             return(OP_LOGIC_OR_EQ);
"&&"              return(OP_LOGIC_AND);
"||"              return(OP_LOGIC_OR);
"=="              return(OP_LOGIC_EQ);
"!="              return(OP_LOGIC_NEQ);
"<="              return(OP_LOGIC_LEQ);
">="              return(OP_LOGIC_GEQ);
"..."             return(ELLIPSIS);
"::"              return(DOUBLE_COLON);

"[" { return(yytext[0]); }
"]" { return(yytext[0]); }

[~{}()<>:;*/%=,&.!+|^\-] { return(yytext[0]); }

.                return(OTHER);
%%

/*
 * Skip ahead until one of the strings is found,
 * then skip to the end of the line.
 */
int skip_ahead_multi(const char *strings[])
{
  char textbuf[SKIP_MATCH_MAXLEN+1];
  char c = 0;
  size_t i;

  for (i = 0; i < (SKIP_MATCH_MAXLEN+1); i++)
    {
    textbuf[i] = '\0';
    }

  for (;;)
    {
    for (i = 0; i < SKIP_MATCH_MAXLEN; i++)
      {
      textbuf[i] = textbuf[i+1];
      }

    if ((c = (char)input()) == '\0')
      {
      fprintf(yyout, "Cannot find matching %s.\n", strings[0]);
      return 0;
      }

    textbuf[SKIP_MATCH_MAXLEN-1] = c;

    for (i = 0; strings[i]; i++)
      {
      if (strcmp(&textbuf[SKIP_MATCH_MAXLEN-strlen(strings[i])],
                 strings[i]) == 0)
        {
        break;
        }
      }
    if (strings[i])
      {
      break;
      }
    }

  while (c != '\0' && c != '\n')
    {
    c = (char)input();
    }

  return 1;
}

/*
 * Skip ahead until the string is found,
 */
int skip_ahead_until(const char *text)
{
  const char *strings[2];
  strings[0] = text;
  strings[1] = NULL;

  return skip_ahead_multi(strings);
}
