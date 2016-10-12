%option yylineno
%{

/*

This file must be translated to C and modified to build everywhere.

Run flex like this:

  flex --nodefault -olex.yy.c vtkParse.l

Modify lex.yy.c:
  - convert tabs to spaces (8 spaces per tab)
  - remove extra space from end of lines
  - remove blank lines from end of file
  - replace "int yyl;" with "yy_size_t yyl;"
  - compile with gcc and "-Wsign-compare", there should be no warnings
  - replace "#line" lines with blank lines
  - remove usage of the "register" keyword
  - add a prototype for isatty
  - fixed 2 cppcheck warnings about "checking if unsigned variable
    is less than zero" around lines 3670 and 3683 by changing <= to ==

*/

/* We do not care of interactive mode */
#define YY_NEVER_INTERACTIVE 1

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
static int skip_comment();
static int skip_trailing_comment(const char *text, size_t l);
static int skip_ahead_multi(const char *strings[]);
static int skip_ahead_until(const char *text);
static int skip_to_next_directive();
static int skip_conditional_block();

static void doxygen_comment();
static void doxygen_cpp_comment();
static void doxygen_group_start();
static void doxygen_group_end();
static void vtk_comment();
static void vtk_name_comment();
static void vtk_section_comment();
static void cpp_comment_line();
static void blank_line();

static const char *raw_string(const char *begin);

static void preprocessor_directive(const char *text, size_t l);
static void print_preprocessor_error(int result, const char *cp, size_t n);
static const char *get_macro_arguments();

static void push_buffer();
static int pop_buffer();

static void push_include(const char *filename);
static void pop_include();

static void push_macro(MacroInfo *macro);
static void pop_macro();
static int in_macro();

%}

WS [\t\n\r ]+
PS [\t\n\r ]*
ID [a-zA-Z_\x80-\xff][0-9a-zA-Z_\x80-\xff]*

%%

"/*"[*!]"<" { doxygen_comment(); }

^[\t ]*"/*"[*!] { doxygen_comment(); }

"/*" { skip_comment(); }

^[\t ]*("@interface"|"@implementation") {
      skip_ahead_until("@end");
    }

"//"[/!]"<".* { doxygen_cpp_comment(); }

^[\t ]*"//"[/!].* { doxygen_cpp_comment(); }
^[\t ]*"//@{".* { doxygen_group_start(); }
^[\t ]*"//@}".* { doxygen_group_end(); }

^[\t ]*"// Description:".* { vtk_comment(); }
^[\t ]*"// .NAME ".* { vtk_name_comment(); }
^[\t ]*"// .SECTION ".* { vtk_section_comment(); }

^[\t ]*"//".* { cpp_comment_line(); }

^[\t ]*[\r\n] { blank_line(); }

"//".*  /* eat trailing C++ comments */

^[\t ]*"#"(\\\n|\\\r\n|[^\n])*  {
      skip_trailing_comment(yytext, yyleng);
      preprocessor_directive(yytext, yyleng);
   }

("u8"|"u"|"U"|"L")?\"([^\\"]|\\\r\n|\\\n|\\.)*\"(_{ID})?  {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(STRING_LITERAL);
    }

("u8"|"u"|"U"|"L")?\'([^\\']|\\.)+\'(_{ID})?  {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(CHAR_LITERAL);
    }

("R"|"u8R"|"uR"|"UR"|"LR")?\"[^ ()\t\v\f\n]*"(" {
      yylval.str = raw_string(yytext);
      return(STRING_LITERAL);
    }

{ID}"_EXPORT"  /* ignore EXPORT macros */

"vtkNotUsed" { /* let the wrappers see the parameter */
      const char *args = NULL;
      const char *cp;
      size_t l = 0;
      args = get_macro_arguments();
      if (args)
      {
        cp = args;
        if (*cp == '(') { cp++; }
        while (*cp == ' ' || *cp == '\t') { cp++; }
        l = vtkParse_SkipId(cp);
        if (l)
        {
          yylval.str = vtkstrndup(cp, l);
          free((char *)args);
          return(ID);
        }
        free((char *)args);
      }
    }

"auto"          return(AUTO);

"double"        return(DOUBLE);
"float"         return(FLOAT);
"__int64"       return(INT64__);
"short"         return(SHORT);
"long"          return(LONG);
"char"          return(CHAR);
"int"           return(INT);

"unsigned"      return(UNSIGNED);
"signed"        return(SIGNED);

"void"          return(VOID);
"bool"          return(BOOL);

"char16_t"      return(CHAR16_T);
"char32_t"      return(CHAR32_T);
"wchar_t"       return(WCHAR_T);

"std::"?"size_t"/[^a-zA-Z_] {
      yylval.str = (yytext[3] == ':' ? "std::size_t" : "size_t");
      return(SIZE_T);
    }

"std::"?"ssize_t"/[^a-zA-Z_] {
      yylval.str = (yytext[3] == ':' ? "std::ssize_t" : "ssize_t");
      return(SSIZE_T);
    }

"std::"?"nullptr_t"/[^a-zA-Z_] {
      yylval.str = (yytext[3] == ':' ? "std::nullptr_t" : "nullptr_t");
      return(NULLPTR_T);
    }

"Q_OBJECT"      /* ignore the Q_OBJECT macro from Qt */
"public"{WS}"slots"{PS}/:    return(PUBLIC);
"private"{WS}"slots"{PS}/:   return(PRIVATE);
"protected"{WS}"slots"{PS}/: return(PROTECTED);
"signals"{PS}/:              return(PROTECTED);

"class"         return(CLASS);
"struct"        return(STRUCT);
"public"        return(PUBLIC);
"private"       return(PRIVATE);
"protected"     return(PROTECTED);
"enum"          return(ENUM);
"union"         return(UNION);
"virtual"       return(VIRTUAL);
"const"         return(CONST);
"volatile"      return(VOLATILE);
"mutable"       return(MUTABLE);
"operator"      return(OPERATOR);
"friend"        return(FRIEND);
"inline"        return(INLINE);
"constexpr"     return(CONSTEXPR);
"static"        return(STATIC);
"thread_local"  return(THREAD_LOCAL);
"extern"        return(EXTERN);
"template"      return(TEMPLATE);
"typename"      return(TYPENAME);
"typedef"       return(TYPEDEF);
"namespace"     return(NAMESPACE);
"using"         return(USING);
"new"           return(NEW);
"delete"        return(DELETE);
"explicit"      return(EXPLICIT);
"throw"         return(THROW);
"try"           return(TRY);
"catch"         return(CATCH);
"noexcept"      return(NOEXCEPT);
"decltype"      return(DECLTYPE);
"default"       return(DEFAULT);

"static_cast"   return(STATIC_CAST);
"dynamic_cast"  return(DYNAMIC_CAST);
"const_cast"    return(CONST_CAST);
"reinterpret_cast" return(REINTERPRET_CAST);

"register"      /* irrelevant to wrappers */

"and"           return(OP_LOGIC_AND);
"and_eq"        return(OP_AND_EQ);
"or"            return(OP_LOGIC_OR);
"or_eq"         return(OP_OR_EQ);
"not"           return('!');
"not_eq"        return(OP_LOGIC_NEQ);
"xor"           return('^');
"xor_eq"        return(OP_XOR_EQ);
"bitand"        return('&');
"bitor"         return('|');
"compl"         return('~');

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

"("{PS}({ID}"::")*"&" {
      size_t i = 1;
      size_t j;
      while (yytext[i]==' ' || yytext[i]=='\t' ||
             yytext[i]=='\r' || yytext[i]=='\n') { i++; }
      j = i;
      while (yytext[j]!='&') { j++; }
      yylval.str = vtkstrndup(&yytext[i], j-i);
      return(LA);
    }

"("{PS}({ID}"::")*"*" {
      size_t i = 1;
      size_t j;
      while (yytext[i]==' ' || yytext[i]=='\t' ||
             yytext[i]=='\r' || yytext[i]=='\n') { i++; }
      j = i;
      while (yytext[j]!='*') { j++; }
      yylval.str = vtkstrndup(&yytext[i], j-i);
      return(LP);
    }

"("{PS}("__cdecl"|"__clrcall"|"__stdcall"|"__fastcall"|"__thiscall"){PS}"*" {
      yylval.str = "";
      return(LP);
    }

"("{PS}("APIENTRY"|"CALLBACK"|"WINAPI"){PS}"*" {
      yylval.str = "";
      return(LP);
    }

"("{PS}("APIENTRYP"|"CALLBACKP"|"WINAPIP") {
      yylval.str = "";
      return(LP);
    }

("APIENTRYP"|"CALLBACKP"|"WINAPIP") return('*');

("APIENTRY"|"CALLBACK"|"WINAPI") /* misc unused win32 macros */

("std::")?"ostream"/[^a-zA-Z_] {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(OSTREAM);
    }

("std::")?"istream"/[^a-zA-Z_] {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(ISTREAM);
    }

("std::")?"string"/[^a-zA-Z_] {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(StdString);
    }

"vtkStdString" {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(StdString);
    }

"vtkUnicodeString" {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(UnicodeString);
    }

"Qt::"{ID} {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(QT_ID);
    }

"static_assert"  get_macro_arguments(); /* C++11 */

"alignas"  get_macro_arguments(); /* C++11 */

"_Alignas" get_macro_arguments(); /* C11 */

"_Generic" get_macro_arguments(); /* C11 */

"_Thread_local" return(THREAD_LOCAL); /* C11 */

"_Atomic"    /* C11 */

"_Noreturn"  /* C11 */

"__attribute__"  get_macro_arguments(); /* gcc attributes */

"__declspec"  get_macro_arguments(); /* Windows linkage */

("__cdecl"|"__clrcall"|"__stdcall"|"__fastcall"|"__thiscall") /* Windows */

"__restrict__" /* gcc/clang/other extension */

"__restrict" /* MSVC extension */

"VTK_LEGACY" {
      const char *args = get_macro_arguments();
      if (args && args[0] != '\0' && args[1] != '\0')
      {
        size_t l = strlen(args);
        push_macro(NULL);
        push_buffer();
        macroName = "VTK_LEGACY";
        macroUsed = 0;
        macroEnded = 0;
        yy_switch_to_buffer(yy_scan_bytes(&args[1], l-2));
      }
      free((char *)args);
    }

"nullptr" {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(NULLPTR);
    }

{ID}  {
      const char *name = vtkstrndup(yytext, yyleng);
      MacroInfo *macro = vtkParsePreprocess_GetMacro(preprocessor, name);
      int expanded = 0;
      if (macro)
      {
        const char *args = NULL;
        const char *emacro = NULL;

        /* make sure that vtkNotUsed is never expanded */
        MacroInfo *ex;
        ex = vtkParsePreprocess_GetMacro(preprocessor, "vtkNotUsed");
        if (ex)
        {
          ex->IsExcluded = 1;
        }

        if (macro->IsFunction)
        {
          args = get_macro_arguments();
          if (args)
          {
            emacro = vtkParsePreprocess_ExpandMacro(preprocessor, macro, args);
            if (!emacro)
            {
              print_preprocessor_error(VTK_PARSE_MACRO_NUMARGS, NULL, 0);
              exit(1);
            }
            free((char *)args);
          }
        }
        else if (macro->Definition && macro->Definition[0])
        {
          /* first see if macro evaluates to a constant value */
          preproc_int_t val;
          int is_unsigned;
          int r;
          macro->IsExcluded = 1;
          r = vtkParsePreprocess_EvaluateExpression(
             preprocessor, macro->Definition, &val, &is_unsigned);
          macro->IsExcluded = 0;
          /* if it isn't a constant expression, then expand it */
          if (r >= VTK_PARSE_MACRO_UNDEFINED)
          {
            emacro = vtkParsePreprocess_ExpandMacro(preprocessor, macro, NULL);
            if (!emacro)
            {
              print_preprocessor_error(r, NULL, 0);
              exit(1);
            }
          }
        }
        else
        {
          /* macros with no definition expand to nothing */
          expanded = 1;
        }
        if (emacro)
        {
          /* invoke the parser on any expanded macros */
          push_macro(macro);
          push_buffer();
          yy_switch_to_buffer(yy_scan_string(emacro));
          vtkParsePreprocess_FreeMacroExpansion(preprocessor, macro, emacro);
          expanded = 1;
        }
      }
      if (!expanded)
      {
        /* if no macro expansion occurred, return the ID */
        yylval.str = name;
        if (yyleng > 3 && name[0] == 'v' && name[1] == 't' && name[2] == 'k')
        {
          return(VTK_ID);
        }
        else if (name[0] == 'Q')
        {
          return(QT_ID);
        }
        else
        {
          return(ID);
        }
      }
    }

"."[0-9]([0-9]|\'[0-9])*([eE][+-]?[0-9]([0-9]|\'[0-9])*)?{ID}?  {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(FLOAT_LITERAL);
    }

[0-9]([0-9]|\'[0-9])*"."([eE][+-]?[0-9]([0-9]|\'[0-9])*)?{ID}?  {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(FLOAT_LITERAL);
    }

[0-9]([0-9]|\'[0-9])*"."[0-9]([0-9]|\'[0-9])*([eE][+-]?[0-9]([0-9]|\'[0-9])*)?{ID}?  {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(FLOAT_LITERAL);
    }

[0-9]([0-9]|\'[0-9])*[eE][+-]?[0-9]([0-9]|\'[0-9])*{ID}?  {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(FLOAT_LITERAL);
    }

"0"[xX][0-9a-fA-F]([0-9a-fA-F]|\'[0-9a-fA-F])*{ID}?  {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(HEX_LITERAL);
    }

"0"[bB][0-1]([0-1]|\'[0-1])*{ID}?  {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(BIN_LITERAL);
    }

"0"([0-8]|\'[0-8])+{ID}? {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(OCT_LITERAL);
    }

[1-9]([0-9]|\'[0-9])*{ID}? {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(INT_LITERAL);
    }

"0" {
      yylval.str = vtkstrndup(yytext, yyleng);
      return(ZERO);
    }

("\\\n"|"\\\r\n") /* escaped newlines */
[\t ]+            /* whitespace */
[\n\r\v\f]        /* whitespace */

"["{PS}"["        return(BEGIN_ATTRIB);

"<%"              return('{');
"%>"              return('}');
"<:"              return('[');
":>"              return(']');
"%:"              return('#');

">"/>             return(OP_RSHIFT_A);

"<<="             return(OP_LSHIFT_EQ);
">>="             return(OP_RSHIFT_EQ);
"<<"              return(OP_LSHIFT);
".*"              return(OP_DOT_POINTER);
"->*"             return(OP_ARROW_POINTER);
"->"              return(OP_ARROW);
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
"&&"              return(OP_LOGIC_AND);
"||"              return(OP_LOGIC_OR);
"=="              return(OP_LOGIC_EQ);
"!="              return(OP_LOGIC_NEQ);
"<="              return(OP_LOGIC_LEQ);
">="              return(OP_LOGIC_GEQ);
"..."             return(ELLIPSIS);
"::"              return(DOUBLE_COLON);

"["               return('[');
"]"               return(']');

[~{}()<>:;*/%=,&.!+|^\-] return(yytext[0]);

<<EOF>> { if (!pop_buffer()) { yyterminate(); } }

.  { return(OTHER); }

%%

/*
 * Return a parenthetical macro arg list as a new string.
 */
const char *get_macro_arguments()
{
  char *cp = NULL;
  size_t i = 0;
  int depth;
  int ws = 0;
  int sl = 0;
  int c1 = input();

  if (c1 == '\0')
  {
    if (pop_buffer() == 0)
    {
      return NULL;
    }
  }

  while (c1 == ' ' || c1 == '\t' || c1 == '\r' || c1 == '\n')
  {
    c1 = input();
  }

  cp = (char *)malloc(4);

  if (c1 != '(')
  {
    unput(c1);
    free(cp);
    return NULL;
  }

  cp[i++] = '(';
  depth = 1;
  c1 = input();

  for (;;)
  {
    ws = 0;
    sl = 0;
    /* skip all whitespace */
    while (c1 == ' ' || c1 == '\t' || c1 == '\r' || c1 == '\n')
    {
      ws = 1;
      c1 = input();
    }
    if (c1 == '/')
    {
      c1 = input();
      if (c1 == '*')
      {
        /* skip a C style comment */
        ws = 1;
        if (skip_comment() == 0)
        {
          return NULL;
        }
        c1 = input();
      }
      else if (c1 == '/')
      {
        /* skip a C++ style comment */
        ws = 1;
        do { c1 = input(); }
        while (c1 != '\n' && c1 != '\0');
        if (c1 == '\0')
        {
          return NULL;
        }
        c1 = input();
      }
      else
      {
        sl = 1;
      }
    }
    if (ws)
    {
      /* add a single space to replace any whitespace */
      cp[i++] = ' ';
      if (i >= 4 && (i & (i-1)) == 0)
      {
        char *oldcp = cp;
        cp = (char *)realloc(cp, 2*i);
        if (!cp)
        {
          free(oldcp);
          return NULL;
        }
      }
    }
    if (sl)
    {
      /* add a single space to replace any whitespace */
      cp[i++] = '/';
      if (i >= 4 && (i & (i-1)) == 0)
      {
        char *oldcp = cp;
        cp = (char *)realloc(cp, 2*i);
        if (!cp)
        {
          free(oldcp);
          return NULL;
        }
      }
    }
    if (c1 == '\"' || c1 == '\'')
    {
      int c2 = c1;
      int escaped = 2;
      int firstloop = 1;
      do
      {
        if (escaped)
        {
          --escaped;
        }
        if (!firstloop)
        {
          c1 = input();
        }
        firstloop = 0;
        if (c1 == '\0')
        {
          break;
        }
        if (escaped == 0 && c1 == '\\')
        {
          escaped = 2;
        }
        cp[i++] = (char)c1;
        if (i >= 4 && (i & (i-1)) == 0)
        {
          char *oldcp = cp;
          cp = (char *)realloc(cp, 2*i);
          if (!cp)
          {
            free(oldcp);
            return NULL;
          }
        }
      }
      while (c1 != c2 || escaped);
    }
    else if (c1 != '\0')
    {
      cp[i++] = (char)c1;
      if (i >= 4 && (i & (i-1)) == 0)
      {
        char *oldcp = cp;
        cp = (char *)realloc(cp, 2*i);
        if (!cp)
        {
          free(oldcp);
          return NULL;
        }
      }
      cp[i] = '\0';
      if (c1 == '(')
      {
        depth++;
      }
      if (c1 == ')')
      {
        if (--depth == 0)
        {
          break;
        }
      }
    }
    else
    {
      return NULL;
    }
    c1 = input();
  }

  return cp;
}

/*
 * Skip a C-style comment, return 0 if unterminated.
 */
int skip_comment()
{
   int savelineno = yylineno;
   int c1 = 0, c2 = input();
   for (;;)
   {
     if (c2 == 0 || c2 == EOF)
     {
       yylineno = savelineno;
       print_preprocessor_error(VTK_PARSE_SYNTAX_ERROR,
         "Cannot find end of comment.", 27);
       exit(1);
     }
     if (c1 == '*' && c2 == '/') break;
     c1 = c2; c2 = input();
   }
   return 1;
}

/*
 * If token contains a comment, make sure whole comment is skipped.
 */
int skip_trailing_comment(const char *text, size_t l)
{
  const char *cp = text;
  const char *ep = text + l;
  int incomment = 0;

  while (cp < ep)
  {
    while (cp < ep && *cp != '/' && *cp != '\"') { cp++; };
    if (cp >= ep)
    {
      break;
    }
    else if (cp[0] == '/' && cp[1] == '*')
    {
      incomment = 1;
      cp += 2;
      while (cp < ep && *cp != '*') { cp++; };
      if (cp[0] == '*' && cp[1] == '/')
      {
        incomment = 0;
        cp += 2;
      }
      else
      {
        cp++;
      }
    }
    else if (cp[0] == '\"')
    {
      cp++;
      while (cp < ep)
      {
        while (cp < ep && *cp != '\\' && *cp != '\"') { cp++; };
        if (cp >= ep)
        {
          break;
        }
        else if (*cp == '\"')
        {
          cp++;
          break;
        }
        else /* if (*cp == '\\') */
        {
          cp += 2;
        }
      }
    }
    else
    {
      cp++;
    }
  }

  if (incomment)
  {
    return skip_comment();
  }

  return 1;
}

/*
 * Skip ahead until the next preprocessor directive.
 * This will eat the '#' that starts the directive.
 * Return 0 if none found.
 */
int skip_to_next_directive()
{
  /* state == 0 at the start of a line */
  int state = 0;
  int c;

  c = input();

  while (c != 0 && c != EOF)
  {
    /* whitespace */
    if (c == ' ' || c == '\t')
    {
      c = input();
    }
    /* newline renews the start-of-line state */
    else if (c == '\n')
    {
      state = 0;
      c = input();
    }
    /* skip comments */
    else if (c == '/')
    {
      state = 1;
      if ( (c = input()) == '*')
      {
        if (skip_comment() == 0)
        {
          return 0;
        }
        c = input();
      }
    }
    /* skip escaped characters */
    else if (c == '\\')
    {
      state = 1;
      if ( (c = input()) == '\r')
      {
        if ( (c = input()) == '\n')
        {
          c = input();
        }
      }
      else if (c != 0 && c != EOF)
      {
        c = input();
      }
    }
    /* any other chars except '#' at start of line */
    else if (c != '#' || state != 0)
    {
      state = 1;
      c = input();
    }
    else
    {
      break;
    }
  }

  return c;
}

/*
 * Skip to the next #else or #elif or #endif
 */
int skip_conditional_block()
{
  static char *linebuf = NULL;
  static size_t linemaxlen = 80;
  size_t i;
  int c;
  int result;

  if (linebuf == 0)
  {
    linebuf = (char *)malloc(linemaxlen);
  }

  for (;;)
  {
    if (skip_to_next_directive() == 0)
    {
      return 0;
    }
    c = input();
    while (c == ' ' || c == '\t')
    {
      c = input();
    }
    if (c == 0 || c == EOF)
    {
      return 0;
    }

    /* eat the whole line */
    i = 0;
    linebuf[i++] = '#';
    while (c != 0 && c != EOF && c != '\n')
    {
      if (i >= linemaxlen-5)
      {
        char *oldlinebuf = linebuf;
        linemaxlen += i+5;
        linebuf = (char *)realloc(linebuf, linemaxlen);
        if (!linebuf)
        {
          free(oldlinebuf);
          return 0;
        }
      }
      linebuf[i++] = c;
      /* be sure to skip escaped newlines */
      if (c == '\\')
      {
        c = input();
        linebuf[i++] = c;
        if (c == '\r')
        {
          c = input();
          linebuf[i++] = c;
        }
      }
      c = input();
    }
    linebuf[i++] = c;

    result = vtkParsePreprocess_HandleDirective(preprocessor, linebuf);
    if (result != VTK_PARSE_SKIP && result != VTK_PARSE_OK)
    {
      print_preprocessor_error(result, linebuf, i);
    }
    else if (result != VTK_PARSE_SKIP)
    {
      break;
    }
  }

  return 1;
}

/*
 * Skip ahead until one of the strings is found,
 * then skip to the end of the line.
 */
int skip_ahead_multi(const char *strings[])
{
  char textbuf[SKIP_MATCH_MAXLEN+1];
  int c = 0;
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

    c = input();
    if (c == 0 || c == EOF)
    {
      print_preprocessor_error(VTK_PARSE_SYNTAX_ERROR, NULL, 0);
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

  while (c != 0 && c != EOF && c != '\n')
  {
    c = input();
  }

  return 1;
}

/*
 * Skip ahead until the string is found.
 */
int skip_ahead_until(const char *text)
{
  const char *strings[2];
  strings[0] = text;
  strings[1] = NULL;

  return skip_ahead_multi(strings);
}


/*
 * Called for doxygen C-style comments
 */
void doxygen_comment()
{
  char linetext[256];
  int savelineno = yylineno;
  int asterisk, isfirstline = 1;
  int type = DoxygenComment;
  size_t l = 0, i = 0, base = yyleng;
  int c1 = 0, c2 = input();
  for (l = 0; l < yyleng; l++)
  {
    linetext[l] = yytext[l];
  }
  if (l > 0 && yytext[l-1] == '<')
  {
    type = TrailingComment;
  }
  for (;;)
  {
    if (c2 == 0 || c2 == EOF)
    {
      yylineno = savelineno;
      print_preprocessor_error(VTK_PARSE_SYNTAX_ERROR,
        "Cannot find end of comment.", 27);
      exit(1);
    }
    if (l < 256)
    {
      linetext[l++] = c2;
    }
    if (c2 == '\n' || (c1 == '*' && c2 == '/'))
    {
      if (l >= 2 &&
          linetext[l-2] == '*' &&
          linetext[l-1] == '/')
      {
        l -= 2;
      }
      while (l > 0 &&
             (linetext[l-1] == '\n' ||
              linetext[l-1] == '\r' ||
              linetext[l-1] == '\t' ||
              linetext[l-1] == ' '))
      {
        l--;
      }
      if (!isfirstline)
      {
        /* reduce the base indentation if chars occur before base */
        asterisk = 0;
        for (i = yyleng-3; i < base && i < l; i++)
        {
          if (linetext[i] == '*' && asterisk == 0)
          {
            asterisk = 1;
          }
          else if (linetext[i] != ' ')
          {
            break;
          }
        }
        if (i > yyleng-3 && i < l &&
            linetext[i] != ' ' && linetext[i-1] == ' ')
        {
          i--;
        }
        base = i;
      }
      if (l > base)
      {
        i = base;
        l -= base;
        addCommentLine(&linetext[i], l, type);
      }
      else if (c1 != '*' || c2 != '/')
      {
        addCommentLine("", 0, type);
      }
      if (isfirstline)
      {
        isfirstline = 0;
        base = 256;
      }
      l = 0;
      if (c1 == '*' && c2 == '/')
      {
        break;
      }
    }
    c1 = c2; c2 = input();
  }
}

/*
 * Called for //! and /// doxygen comments (handles just one line)
 */
void doxygen_cpp_comment()
{
  int type = DoxygenComment;
  size_t pos = 2;
  while (yytext[pos-2] != '/' || yytext[pos-1] != '/') pos++;
  while (pos < yyleng && yytext[pos-1] == '/' && yytext[pos] == '/') pos++;
  if (pos < yyleng && yytext[pos] == '!') pos++;
  if (pos < yyleng && yytext[pos] == '<')
  {
    pos++;
    type = TrailingComment;
  }
  addCommentLine(&yytext[pos], yyleng - pos, type);
}

/*
 * Called for //@{
 */
void doxygen_group_start()
{
  /* Set the "ingroup" marker */
  setCommentMemberGroup(1);
}

/*
 * Called for //@}
 */
void doxygen_group_end()
{
  /* Clear the "ingroup" marker */
  setCommentMemberGroup(0);
}

/*
 * Called for // Description:
 */
void vtk_comment()
{
  setCommentState(NormalComment);
}

/*
 * Called for // .NAME
 */
void vtk_name_comment()
{
  size_t pos = 1;
  while (yytext[pos-1] != 'M' || yytext[pos] != 'E')
  {
    pos++;
  }
  pos++;
  setCommentState(NameComment);
  addCommentLine(&yytext[pos], yyleng - pos, NormalComment);
}

/*
 * Called for // .SECTION
 */
void vtk_section_comment()
{
  size_t pos = 1;
  while (yytext[pos-1] != 'O' || yytext[pos] != 'N')
  {
    pos++;
  }
  pos++;
  if (pos < yyleng && yytext[pos] == ' ')
  {
    pos++;
  }

  if (yyleng - pos >= 11 &&
      strncmp(&yytext[pos], "Description", 11) == 0)
  {
    setCommentState(DescriptionComment);
  }
  else if (yyleng - pos >= 8 &&
           (strncmp(&yytext[pos], "See Also", 8) == 0 ||
            strncmp(&yytext[pos], "see also", 8) == 0))
  {
    setCommentState(SeeAlsoComment);
  }
  else if (yyleng - pos >= 7 &&
           strncmp(&yytext[pos], "Caveats", 7) == 0)
  {
    setCommentState(CaveatsComment);
  }
  else
  {
    cpp_comment_line();
  }
}

/*
 * Called for each line that has a C++ comment but no code, unless the
 * comment is recognized as beginning a VTK or doxygen comment.
 */
void cpp_comment_line()
{
  size_t pos = 2;
  while (yytext[pos-2] != '/' || yytext[pos-1] != '/') pos++;
  addCommentLine(&yytext[pos], yyleng - pos, NormalComment);
}

/*
 * Called whenever a blank line is encountered.
 */
void blank_line()
{
  commentBreak();
}


/*
 * Convert a raw string into a non-raw string.
 */
const char *raw_string(const char *begin)
{
  int savelineno = yylineno;
  char *textbuf;
  int c = 0;
  const char *delim;
  const char *cp = begin;
  char *dp;
  char *result;
  size_t i, j, n;
  size_t m = 1024;

  result = (char *)malloc(m);
  dp = result;

  while (*cp != '\"')
  {
    *dp++ = *cp++;
  }
  --dp;
  *dp++ = *cp++;

  delim = cp;

  for (n = 0;; n++)
  {
    if (delim[n] == '(') { break; }
  }

  textbuf = (char *)malloc(n+1);

  for (i = 0; i < n+1; i++)
  {
    c = input();
    textbuf[i] = c;
  }

  while (c != EOF)
  {
    if (textbuf[0] == ')' && (n == 0 || strncmp(&textbuf[1], delim, n) == 0))
    {
      break;
    }

    j = dp - result;
    if (j > m - 8)
    {
      m += 1024;
      result = (char *)realloc(result, m);
      if (!result)
      {
        print_preprocessor_error(VTK_PARSE_OUT_OF_MEMORY, NULL, 0);
        exit(1);
      }
      dp = result + j;
    }

    if ((*textbuf >= ' ' && *textbuf <= '~') ||
        (*textbuf & 0x80) != 0)
    {
      *dp++ = *textbuf;
    }
    else switch (*textbuf)
    {
      case '\a': *dp++ = '\\'; *dp++ = 'a'; break;
      case '\b': *dp++ = '\\'; *dp++ = 'b'; break;
      case '\f': *dp++ = '\\'; *dp++ = 'f'; break;
      case '\n': *dp++ = '\\'; *dp++ = 'n'; break;
      case '\r': *dp++ = '\\'; *dp++ = 'r'; break;
      case '\t': *dp++ = '\\'; *dp++ = 't'; break;
      case '\v': *dp++ = '\\'; *dp++ = 'v'; break;
      case '\\': *dp++ = '\\'; *dp++ = '\\'; break;
      case '\'': *dp++ = '\\'; *dp++ = '\''; break;
      case '\"': *dp++ = '\\'; *dp++ = '\"'; break;
      default:
        sprintf(dp, "\\%3.3o", *textbuf);
        dp += 4;
        break;
    }

    for (i = 0; i < n; i++)
    {
      textbuf[i] = textbuf[i+1];
    }

    c = input();
    textbuf[n] = c;
  }

  if (c == EOF || '\"' != input())
  {
    yylineno = savelineno;
    print_preprocessor_error(VTK_PARSE_SYNTAX_ERROR,
      "Unterminated raw string.", 24);
    exit(1);
  }

  *dp++ = '\"';

  c = input();
  if (c == '_')
  {
    do
    {
      *dp++ = c;
      c = input();
    }
    while (vtkParse_CharType(c, CPRE_XID));
  }
  unput(c);

  *dp = '\0';
  cp = vtkstrdup(result);
  free(result);
  free(textbuf);

  return cp;
}

/*
 * buffer stack, used for macro expansion and include files
 */
static size_t buffer_stack_size = 0;
static YY_BUFFER_STATE *buffer_stack = NULL;

/*
 * push the current buffer onto the buffer stack.
 */
void push_buffer()
{
  size_t n = buffer_stack_size;
  if (buffer_stack == NULL)
  {
    buffer_stack = (YY_BUFFER_STATE *)malloc(4*sizeof(YY_BUFFER_STATE));
  }
  /* grow the stack whenever size reaches a power of two */
  else if (n >= 4 && (n & (n-1)) == 0)
  {
    buffer_stack = (YY_BUFFER_STATE *)realloc(
      buffer_stack, 2*n*sizeof(YY_BUFFER_STATE));
    if (!buffer_stack)
    {
      print_preprocessor_error(VTK_PARSE_OUT_OF_MEMORY, NULL, 0);
      exit(1);
    }
  }
  buffer_stack[buffer_stack_size++] = YY_CURRENT_BUFFER;
}

/*
 * pop the buffer stack and restore the previous buffer
 */
int pop_buffer()
{
  if (in_macro())
  {
    pop_macro();
  }
  else
  {
    pop_include();
  }
  if (buffer_stack_size == 0)
  {
    return 0;
  }
  yy_delete_buffer(YY_CURRENT_BUFFER);
  yy_switch_to_buffer(buffer_stack[--buffer_stack_size]);
  return 1;
}

/*
 * include stack, to tell what include is being evaluated
 */
static size_t include_stack_size = 0;
static FileInfo **include_stack = NULL;
static int *lineno_stack = NULL;

/*
 * push the current include onto the include stack.
 */
void push_include(const char *filename)
{
  FileInfo *file_info = NULL;
  int same_file = 0;
  size_t n = include_stack_size;

  if (include_stack == NULL)
  {
    include_stack = (FileInfo **)malloc(4*sizeof(FileInfo *));
    lineno_stack = (int *)malloc(4*sizeof(int));
  }

  /* grow the stack whenever size reaches a power of two */
  else if (n >= 4 && (n & (n-1)) == 0)
  {
    include_stack = (FileInfo **)realloc(
      include_stack, 2*n*sizeof(FileInfo *));
    if (!include_stack)
    {
      print_preprocessor_error(VTK_PARSE_OUT_OF_MEMORY, NULL, 0);
      exit(1);
    }
    lineno_stack = (int *)realloc(
      lineno_stack, 2*n*sizeof(int));
    if (!lineno_stack)
    {
      print_preprocessor_error(VTK_PARSE_OUT_OF_MEMORY, NULL, 0);
      exit(1);
    }
  }

  lineno_stack[include_stack_size] = yyget_lineno();
  yyset_lineno(0);
  include_stack[include_stack_size++] = data;

  /* if the file is including itself */
  if (filename == data->FileName ||
      (filename != 0 && data->FileName != 0 &&
       strcmp(filename, data->FileName) == 0))
  {
    same_file = 1;
  }

  /* make a new fileinfo, but only if we are in the base namespace
   * and only if the only items added so far are constants */
  if (!same_file && currentNamespace == data->Contents &&
      data->Contents->NumberOfItems == data->Contents->NumberOfConstants)
  {
    file_info = (FileInfo *)malloc(sizeof(FileInfo));
    vtkParse_InitFile(file_info);
    file_info->FileName = vtkstrdup(filename);
    file_info->Contents = (NamespaceInfo *)malloc(sizeof(NamespaceInfo));
    vtkParse_InitNamespace(file_info->Contents);
    vtkParse_AddIncludeToFile(data, file_info);
    file_info->Strings = data->Strings;
    data = file_info;
    currentNamespace = file_info->Contents;
  }
}

/*
 * pop the include stack
 */
void pop_include()
{
  if (include_stack_size > 0)
  {
    --include_stack_size;
    fclose(yyin);
    yyset_lineno(lineno_stack[include_stack_size]);
    if (data != include_stack[include_stack_size])
    {
      data = include_stack[include_stack_size];
      currentNamespace = data->Contents;
    }
  }
}


/*
 * macro stack, to tell what macro is being evaluated
 */
static size_t macro_stack_size = 0;
static MacroInfo **macro_stack = NULL;

/*
 * push the current macro onto the macro stack.
 */
void push_macro(MacroInfo *macro)
{
  size_t n = macro_stack_size;
  if (macro_stack == NULL)
  {
    macro_stack = (MacroInfo **)malloc(4*sizeof(MacroInfo *));
  }
  /* grow the stack whenever size reaches a power of two */
  else if (n >= 4 && (n & (n-1)) == 0)
  {
    macro_stack = (MacroInfo **)realloc(
      macro_stack, 2*n*sizeof(MacroInfo *));
    if (!macro_stack)
    {
      print_preprocessor_error(VTK_PARSE_OUT_OF_MEMORY, NULL, 0);
      exit(1);
    }
  }
  macro_stack[macro_stack_size++] = macro;
  if (macro)
  {
    macro->IsExcluded = 1;
    if (macro_stack_size == 1)
    {
      macroName = macro->Name;
      macroUsed = 0;
      macroEnded = 0;
    }
  }
}

/*
 * pop the macro stack
 */
void pop_macro()
{
  MacroInfo *macro;

  if (macro_stack_size > 0)
  {
    macro = macro_stack[--macro_stack_size];
    if (macro)
    {
      macro->IsExcluded = 0;
    }
  }
  macroEnded = 1;
}

/*
 * are we currently processing a macro?
 */
int in_macro()
{
  return (macro_stack_size > 0);
}

/*
 * print a preprocessor error code with filename and line number.
 */
void print_preprocessor_error(int result, const char *cp, size_t n)
{
  const char *text = "";

  switch (result)
  {
    case VTK_PARSE_OK:
    case VTK_PARSE_SKIP:
      return;
    case VTK_PARSE_PREPROC_DOUBLE:
      text = "double in preprocessor conditional";
      break;
    case VTK_PARSE_PREPROC_FLOAT:
      text = "float in preprocessor conditional";
      break;
    case VTK_PARSE_PREPROC_STRING:
      text = "string in preprocessor conditional";
      break;
    case VTK_PARSE_MACRO_UNDEFINED:
      text = "undefined macro";
      break;
    case VTK_PARSE_MACRO_REDEFINED:
      text = "redefined macro";
      break;
    case VTK_PARSE_FILE_NOT_FOUND:
      text = "file not found";
      break;
    case VTK_PARSE_FILE_OPEN_ERROR:
      text = "can\'t open file";
      break;
    case VTK_PARSE_FILE_READ_ERROR:
      text = "input/output error";
      break;
    case VTK_PARSE_MACRO_NUMARGS:
      text = "wrong number of macro args";
      break;
    case VTK_PARSE_SYNTAX_ERROR:
      text = "syntax error";
      break;
    case VTK_PARSE_OUT_OF_MEMORY:
      text = "out of memory";
      break;
  }

  /* be silent about missing include files */
  if (result == VTK_PARSE_FILE_NOT_FOUND)
  {
    return;
  }

  print_parser_error(text, cp, n);
}

/*
 * print an error with filename and line number.
 */
void print_parser_error(const char *text, const char *cp, size_t n)
{
  size_t j = 0;
  const char *fn = "(none)";

  if (CommandName)
  {
    fprintf(yyout, "%s: ", CommandName);
  }

  if (data->FileName)
  {
    fn = data->FileName;
  }
  fprintf(yyout, "In %s:", fn);
  for (j = 0; j < include_stack_size; j++)
  {
    fprintf(yyout, "%i:\nIn %s:",
            lineno_stack[j], include_stack[j]->FileName);
  }
  fprintf(yyout, "%i:", yylineno);

  if (cp)
  {
    fprintf(yyout, " %s: %*.*s\n", text, (int)n, (int)n, cp);
  }
  else if (text)
  {
    fprintf(yyout, " %s.\n", text);
  }
  else
  {
    fprintf(yyout, "\n");
  }
}

/*
 * Execute a preprocessor directive.
 */
void preprocessor_directive(const char *text, size_t l)
{
  int result = 0;
  size_t n = 0;
  const char *cp = text;
  const char *ep = text + l;
  const char *directive = NULL;

  /* find the directive, store its length in "n" */
  while (*cp == ' ' || *cp == '\t') { cp++; }
  if (*cp == '#') { cp++; }
  while ((*cp == ' ' || *cp == '\t') && cp < ep) { cp++; }
  directive = cp;
  while (*cp >= 'a' && *cp <= 'z' && cp < ep) { cp++; }
  n = cp - directive;
  while ((*cp == ' ' || *cp == '\t') && cp < ep) { cp++; }

  if (n == 7 && strncmp(directive, "include", n) == 0)
  {
    /* include files */
    int already_loaded = 0;
    if (*cp == '<' || *cp == '\"')
    {
      /* if asked to recurse into header files */
      if (Recursive && ep - cp > 3)
      {
        const char *dp;
        dp = vtkParsePreprocess_FindIncludeFile(preprocessor,
          &cp[1], (*cp != '\"'), &already_loaded);
        if (dp)
        {
          yyin = fopen(dp, "r");
          if (yyin)
          {
            push_include(dp);
            push_buffer();
            yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
            return;
          }
        }
      }
    }
  }

  /* let the preprocessor handle the directive */
  result = vtkParsePreprocess_HandleDirective(preprocessor, text);

  if (result == VTK_PARSE_SKIP)
  {
    skip_conditional_block();
  }
  else if (result != VTK_PARSE_OK)
  {
    print_preprocessor_error(result, text, l);
    if ((result & VTK_PARSE_FATAL_ERROR) != 0)
    {
      exit(1);
    }
  }
  else if (n == 6 && strncmp(directive, "define", n) == 0)
  {
    closeComment();
    if (ep - cp > 4 && strncmp(cp, "VTK", 3) == 0)
    {
      /* macros that start with "VTK" */
      MacroInfo *macro;

      macro = vtkParsePreprocess_GetMacro(preprocessor, cp);
      if (macro && macro->Definition && !macro->IsFunction)
      {
        /* if macro evaluates to a constant, add it as a constant */
        macro->IsExcluded = 1;
        if (guess_constant_type(macro->Definition) == 0)
        {
          result = VTK_PARSE_MACRO_UNDEFINED;
        }
        macro->IsExcluded = 0;
        if (result < VTK_PARSE_MACRO_UNDEFINED)
        {
          add_constant(
            vtkstrdup(macro->Name), vtkstrdup(macro->Definition),
            0, NULL, 1);
        }
      }
    }
  }
}
