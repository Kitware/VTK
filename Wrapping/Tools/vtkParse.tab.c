/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Skeleton implementation for Bison GLR parsers in C

   Copyright (C) 2002-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C GLR parser skeleton written by Paul Hilfinger.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "glr.c"

/* Pure parsers.  */
#define YYPURE 0






/* First part of user declarations.  */



/*

This file must be translated to C and modified to build everywhere.

Run bison like this (use bison 3.0.2 or later)

  bison --no-lines -b vtkParse vtkParse.y

Modify vtkParse.tab.c:
  - convert TABs to spaces (eight per tab)
  - replace all instances of "static inline" with "static"
  - replace "#if ! defined lint || defined __GNUC__" with "#if 1"
  - remove YY_ATTRIBUTE_UNUSED from yyfillin, yyfill, and yynormal
  - remove the "break;" after "return yyreportAmbiguity"
  - replace "(1-yyrhslen)" with "(1-(int)yyrhslen)"
  - remove dead store "yynewStates = YY_NULLPTR;"
  - replace "sizeof yynewStates[0] with "sizeof (yyGLRState*)"
  - remove 'struct yy_trans_info', which is unneeded (despite the comment)
*/

/*
The purpose of this parser is to read C++ header files in order to
generate data structures that describe the C++ interface of a library,
one header file at a time.  As such, it is not a complete C++ parser.
It only parses what is relevant to the interface and skips the rest.

While the parser reads method definitions, type definitions, and
template definitions it generates a "signature" which is a string
that matches (apart from whitespace) the text that was parsed.

While parsing types, the parser creates an unsigned int that describes
the type as well as creating other data structures for arrays, function
pointers, etc.  The parser also creates a typeId string, which is either
a simple id that gives the class name or type name, or is "function" for
function pointer types, or "method" for method pointer types.
*/

/*
Conformance Notes:

This parser was designed empirically and incrementally.  It has been
refactored to make it more similar to the C++ 1998 grammar, but there
are still many very significant differences.

The most significant difference between this parser and a "standard"
parser is that it only parses declarations in detail.  All other
statements and expressions are parsed as arbitrary sequences of symbols,
without any syntactic analysis.

The "unqualified_id" does not directly include "operator_function_id" or
"conversion_function_id" (e.g. ids like "operator=" or "operator int*").
Instead, these two id types are used to allow operator functions to be
handled by their own rules, rather than by the generic function rules.
These ids can only be used in function declarations and using declarations.

Types are handled quite differently from the C++ BNF.  These differences
represent a prolonged (and ultimately successful) attempt to empirically
create a yacc parser without any shift/reduce conflicts.  The rules for
types are organized according to the way that types are usually defined
in working code, rather than strictly according to C++ grammar.

The declaration specifier "typedef" can only appear at the beginning
of a declaration sequence.  There are also restrictions on where class
and enum specifiers can be used: you can declare a new struct within a
variable declaration, but not within a parameter declaration.

The lexer returns each of "(scope::*", "(*", "(a::b::*", etc. as single
tokens.  The C++ BNF, in contrast, would consider these to be a "("
followed by a "ptr_operator".  The lexer concatenates these tokens in
order to eliminate shift/reduce conflicts in the parser.  However, this
means that this parser will only recognize "scope::*" as valid if it is
preceded by "(", e.g. as part of a member function pointer specification.

Variables that are initialized via constructor arguments, for example
"someclass variablename(arglist)", must take a literals as the first
argument.  If an identifier is used as the first argument, then the
parser will interpret the variable declaration as a function declaration,
since the parser will assume the identifier names a type.

An odd bit of C++ ambiguity is that y(x); can be interpreted variously
as declaration of variable "x" of type "y", as a function call if "y"
is the name of a function, or as a constructor if "y" is the name of
a class.  This parser always interprets this pattern as a constructor
declaration, because function calls are ignored by the parser, and
variable declarations of the form y(x); are exceedingly rare compared
to the more usual form y x; without parentheses.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) print_parser_error(a, NULL, 0)
#define yywrap() 1

/* Make sure yacc-generated code knows we have included stdlib.h.  */
#ifndef _STDLIB_H
# define _STDLIB_H
#endif
#define YYINCLUDED_STDLIB_H

/* Borland and MSVC do not define __STDC__ properly. */
#if !defined(__STDC__)
# if defined(_MSC_VER) || defined(__BORLANDC__)
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

/* Map from the type anonymous_enumeration in vtkType.h to the
   VTK wrapping type system number for the type. */

#include "vtkParse.h"
#include "vtkParsePreprocess.h"
#include "vtkParseData.h"
#include "vtkType.h"

static unsigned int vtkParseTypeMap[] =
  {
  VTK_PARSE_VOID,               /* VTK_VOID                0 */
  0,                            /* VTK_BIT                 1 */
  VTK_PARSE_CHAR,               /* VTK_CHAR                2 */
  VTK_PARSE_UNSIGNED_CHAR,      /* VTK_UNSIGNED_CHAR       3 */
  VTK_PARSE_SHORT,              /* VTK_SHORT               4 */
  VTK_PARSE_UNSIGNED_SHORT,     /* VTK_UNSIGNED_SHORT      5 */
  VTK_PARSE_INT,                /* VTK_INT                 6 */
  VTK_PARSE_UNSIGNED_INT,       /* VTK_UNSIGNED_INT        7 */
  VTK_PARSE_LONG,               /* VTK_LONG                8 */
  VTK_PARSE_UNSIGNED_LONG,      /* VTK_UNSIGNED_LONG       9 */
  VTK_PARSE_FLOAT,              /* VTK_FLOAT              10 */
  VTK_PARSE_DOUBLE,             /* VTK_DOUBLE             11 */
  VTK_PARSE_ID_TYPE,            /* VTK_ID_TYPE            12 */
  VTK_PARSE_STRING,             /* VTK_STRING             13 */
  0,                            /* VTK_OPAQUE             14 */
  VTK_PARSE_SIGNED_CHAR,        /* VTK_SIGNED_CHAR        15 */
  VTK_PARSE_LONG_LONG,          /* VTK_LONG_LONG          16 */
  VTK_PARSE_UNSIGNED_LONG_LONG, /* VTK_UNSIGNED_LONG_LONG 17 */
  VTK_PARSE___INT64,            /* VTK___INT64            18 */
  VTK_PARSE_UNSIGNED___INT64,   /* VTK_UNSIGNED___INT64   19 */
  0,                            /* VTK_VARIANT            20 */
  0,                            /* VTK_OBJECT             21 */
  VTK_PARSE_UNICODE_STRING      /* VTK_UNICODE_STRING     22 */
  };

/* Define some constants to simplify references to the table lookup in
   the primitive_type production rule code.  */
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

/* Define the kinds of [[attributes]] to collect */
enum
  {
  VTK_PARSE_ATTRIB_NONE,
  VTK_PARSE_ATTRIB_DECL,  /* modify a declaration */
  VTK_PARSE_ATTRIB_ID,    /* modify an id */
  VTK_PARSE_ATTRIB_REF,   /* modify *, &, or && */
  VTK_PARSE_ATTRIB_FUNC,  /* modify a function or method */
  VTK_PARSE_ATTRIB_ARRAY, /* modify an array size specifier */
  VTK_PARSE_ATTRIB_CLASS  /* modify class, struct, union, or enum */
  };

#define vtkParseDebug(s1, s2) \
  if ( parseDebug ) { fprintf(stderr, "   %s %s\n", s1, s2); }

/* the tokenizer */
int yylex(void);

/* global variables */
FileInfo      *data = NULL;
int            parseDebug;

/* the "preprocessor" */
PreprocessInfo *preprocessor = NULL;

/* include dirs specified on the command line */
int            NumberOfIncludeDirectories= 0;
const char   **IncludeDirectories;

/* macros specified on the command line */
int            NumberOfDefinitions = 0;
const char   **Definitions;

/* options that can be set by the programs that use the parser */
int            Recursive = 0;
const char    *CommandName = NULL;

/* various state variables */
NamespaceInfo *currentNamespace = NULL;
ClassInfo     *currentClass = NULL;
FunctionInfo  *currentFunction = NULL;
TemplateInfo  *currentTemplate = NULL;
const char    *currentEnumName = NULL;
const char    *currentEnumValue = NULL;
unsigned int   currentEnumType = 0;
parse_access_t access_level = VTK_ACCESS_PUBLIC;

/* functions from vtkParse.l */
void print_parser_error(const char *text, const char *cp, size_t n);

/* helper functions */
const char *type_class(unsigned int type, const char *classname);
void start_class(const char *classname, int is_struct_or_union);
void end_class();
void add_base_class(ClassInfo *cls, const char *name, int access_lev,
                    unsigned int extra);
void output_friend_function(void);
void output_function(void);
void reject_function(void);
void set_return(FunctionInfo *func, unsigned int type,
                const char *typeclass, int count);
void add_parameter(FunctionInfo *func, unsigned int type,
                   const char *classname, int count);
void add_template_parameter(unsigned int datatype,
                            unsigned int extra, const char *funcSig);
void add_using(const char *name, int is_namespace);
void start_enum(const char *name, int is_scoped,
                unsigned int type, const char *basename);
void add_enum(const char *name, const char *value);
void end_enum();
unsigned int guess_constant_type(const char *value);
void add_constant(const char *name, const char *value,
                  unsigned int type, const char *typeclass, int global);
const char *add_const_scope(const char *name);
void prepend_scope(char *cp, const char *arg);
unsigned int guess_id_type(const char *cp);
unsigned int add_indirection(unsigned int tval, unsigned int ptr);
unsigned int add_indirection_to_array(unsigned int ptr);
void handle_complex_type(ValueInfo *val, unsigned int datatype,
                         unsigned int extra, const char *funcSig);
void handle_function_type(ValueInfo *param, const char *name,
                          const char *funcSig);
void handle_attribute(const char *att, int pack);
void add_legacy_parameter(FunctionInfo *func, ValueInfo *param);

void outputSetVectorMacro(const char *var, unsigned int paramType,
                          const char *typeText, int n);
void outputGetVectorMacro(const char *var, unsigned int paramType,
                          const char *typeText, int n);


/*----------------------------------------------------------------
 * String utility methods
 *
 * Strings are centrally allocated and are const, and they are not
 * freed until the program exits.  If they need to be freed before
 * then, vtkParse_FreeStringCache() can be called.
 */

/* duplicate the first n bytes of a string and terminate */
static const char *vtkstrndup(const char *in, size_t n)
{
  return vtkParse_CacheString(data->Strings, in, n);
}

/* duplicate a string */
static const char *vtkstrdup(const char *in)
{
  if (in)
    {
    in = vtkParse_CacheString(data->Strings, in, strlen(in));
    }

  return in;
}

/* helper function for concatenating strings */
static const char *vtkstrncat(size_t n, const char **str)
{
  char *cp;
  size_t i;
  size_t j[8];
  size_t m = 0;

  for (i = 0; i < n; i++)
    {
    j[i] = 0;
    if (str[i])
      {
      j[i] = strlen(str[i]);
      m += j[i];
      }
    }
  cp = vtkParse_NewString(data->Strings, m);
  m = 0;
  for (i = 0; i < n; i++)
    {
    if (j[i])
      {
      strncpy(&cp[m], str[i], j[i]);
      m += j[i];
      }
    }
  cp[m] = '\0';

  return cp;
}

/* concatenate strings */
static const char *vtkstrcat(const char *str1, const char *str2)
{
  const char *cp[2];

  cp[0] = str1;
  cp[1] = str2;
  return vtkstrncat(2, cp);
}

static const char *vtkstrcat3(const char *str1, const char *str2,
                              const char *str3)
{
  const char *cp[3];

  cp[0] = str1;
  cp[1] = str2;
  cp[2] = str3;
  return vtkstrncat(3, cp);
}

static const char *vtkstrcat4(const char *str1, const char *str2,
                              const char *str3, const char *str4)
{
  const char *cp[4];

  cp[0] = str1;
  cp[1] = str2;
  cp[2] = str3;
  cp[3] = str4;
  return vtkstrncat(4, cp);
}

static const char *vtkstrcat5(const char *str1, const char *str2,
                              const char *str3, const char *str4,
                              const char *str5)
{
  const char *cp[5];

  cp[0] = str1;
  cp[1] = str2;
  cp[2] = str3;
  cp[3] = str4;
  cp[4] = str5;
  return vtkstrncat(5, cp);
}

static const char *vtkstrcat7(const char *str1, const char *str2,
                              const char *str3, const char *str4,
                              const char *str5, const char *str6,
                              const char *str7)
{
  const char *cp[7];

  cp[0] = str1;
  cp[1] = str2;
  cp[2] = str3;
  cp[3] = str4;
  cp[4] = str5;
  cp[5] = str6;
  cp[6] = str7;
  return vtkstrncat(7, cp);
}

/*----------------------------------------------------------------
 * Comments
 */

enum comment_enum
{
  ClosedComment = -2,
  StickyComment = -1,
  NoComment = 0,
  NormalComment = 1,
  NameComment = 2,
  DescriptionComment = 3,
  SeeAlsoComment = 4,
  CaveatsComment = 5,
  DoxygenComment = 6,
  TrailingComment = 7
};

/* "private" variables */
char          *commentText = NULL;
size_t         commentLength = 0;
size_t         commentAllocatedLength = 0;
int            commentState = 0;
int            commentMemberGroup = 0;
int            commentGroupDepth = 0;
parse_dox_t    commentType = DOX_COMMAND_OTHER;
const char    *commentTarget = NULL;

/* Struct for recognizing certain doxygen commands */
struct DoxygenCommandInfo
{
  const char *name;
  size_t length;
  parse_dox_t type;
};

/* List of doxygen commands (@cond is not handled yet) */
struct DoxygenCommandInfo doxygenCommands[] = {
  { "def", 3, DOX_COMMAND_DEF },
  { "category", 8, DOX_COMMAND_CATEGORY },
  { "interface", 9, DOX_COMMAND_INTERFACE },
  { "protocol", 8, DOX_COMMAND_PROTOCOL },
  { "class", 5, DOX_COMMAND_CLASS },
  { "enum", 4, DOX_COMMAND_ENUM },
  { "struct", 6, DOX_COMMAND_STRUCT },
  { "union", 5, DOX_COMMAND_UNION },
  { "namespace", 9, DOX_COMMAND_NAMESPACE },
  { "typedef", 7, DOX_COMMAND_TYPEDEF },
  { "fn", 2, DOX_COMMAND_FN },
  { "property", 8, DOX_COMMAND_PROPERTY },
  { "var", 3, DOX_COMMAND_VAR },
  { "name", 4, DOX_COMMAND_NAME },
  { "defgroup", 8, DOX_COMMAND_DEFGROUP },
  { "addtogroup", 10, DOX_COMMAND_ADDTOGROUP },
  { "weakgroup", 9, DOX_COMMAND_WEAKGROUP },
  { "example", 7, DOX_COMMAND_EXAMPLE },
  { "file", 4, DOX_COMMAND_FILE },
  { "dir", 3, DOX_COMMAND_DIR },
  { "mainpage", 8, DOX_COMMAND_MAINPAGE },
  { "page", 4, DOX_COMMAND_PAGE },
  { "subpage", 7, DOX_COMMAND_SUBPAGE },
  { "internal", 8, DOX_COMMAND_INTERNAL },
  { "package", 7, DOX_COMMAND_PACKAGE },
  { "privatesection", 14, DOX_COMMAND_PRIVATESECTION },
  { "protectedsection", 16, DOX_COMMAND_PROTECTEDSECTION },
  { "publicsection", 13, DOX_COMMAND_PUBLICSECTION },
  { NULL, 0, DOX_COMMAND_OTHER }
};

void closeComment();

/* Clear the comment buffer */
void clearComment()
{
  commentLength = 0;
  if (commentText)
    {
    commentText[commentLength] = '\0';
    }
  commentState = 0;
  commentType = DOX_COMMAND_OTHER;
}

/* This is called when entering or leaving a comment block */
void setCommentState(int state)
{
  switch (state)
    {
    case 0:
      closeComment();
      break;
    default:
      closeComment();
      clearComment();
      break;
    }

  commentState = state;
}

/* Get the text from the comment buffer */
const char *getComment()
{
  const char *text = commentText;
  const char *cp = commentText;
  size_t l = commentLength;

  if (commentText != NULL && commentState != 0)
    {
    /* strip trailing blank lines */
    while (l > 0 && (cp[l-1] == ' ' || cp[l-1] == '\t' ||
                     cp[l-1] == '\r' || cp[l-1] == '\n'))
      {
      if (cp[l-1] == '\n')
        {
        commentLength = l;
        }
      l--;
      }
    commentText[commentLength] = '\0';
    /* strip leading blank lines */
    while (*cp == ' ' || *cp == '\t' || *cp == '\r' || *cp == '\n')
      {
      if (*cp == '\n')
        {
        text = cp + 1;
        }
      cp++;
      }
    return text;
    }

  return NULL;
}

/* Check for doxygen commands that mark unwanted comments */
parse_dox_t checkDoxygenCommand(const char *text, size_t n)
{
  struct DoxygenCommandInfo *info;
  for (info = doxygenCommands; info->name; info++)
    {
    if (info->length == n && strncmp(text, info->name, n) == 0)
      {
      return info->type;
      }
    }
  return DOX_COMMAND_OTHER;
}

/* This is called whenever a comment line is encountered */
void addCommentLine(const char *line, size_t n, int type)
{
  size_t i, j;
  parse_dox_t t = DOX_COMMAND_OTHER;

  if (type == DoxygenComment || commentState == DoxygenComment)
    {
    if (type == DoxygenComment)
      {
      /* search for '@' and backslash */
      for (i = 0; i+1 < n; i++)
        {
        if (line[i] == '@' || line[i] == '\\')
          {
          j = ++i;
          while (i < n && line[i] >= 'a' && line[i] <= 'z')
            {
            i++;
            }
          if (line[i-1] == '@' && (line[i] == '{' || line[i] == '}'))
            {
            if (line[i] == '{')
              {
              commentGroupDepth++;
              }
            else
              {
              --commentGroupDepth;
              }
            closeComment();
            return;
            }
          else
            {
            /* record the type of this comment */
            t = checkDoxygenCommand(&line[j], i-j);
            if (t != DOX_COMMAND_OTHER)
              {
              while (i < n && line[i] == ' ')
                {
                i++;
                }
              j = i;
              while (i < n && vtkParse_CharType(line[i], CPRE_XID))
                {
                i++;
                }
              commentTarget = vtkstrndup(&line[j], i-j);
              /* remove this line from the comment */
              n = 0;
              }
            }
          }
        }
      }
    else if (commentState == DoxygenComment)
      {
      return;
      }
    if (commentState != type)
      {
      setCommentState(type);
      }
    if (t != DOX_COMMAND_OTHER)
      {
      commentType = t;
      }
    }
  else if (type == TrailingComment)
    {
    if (commentState != type)
      {
      setCommentState(type);
      }
    }
  else if (commentState == 0 ||
           commentState == StickyComment ||
           commentState == ClosedComment)
    {
    clearComment();
    return;
    }

  if (commentText == NULL)
    {
    commentAllocatedLength = n+80;
    commentText = (char *)malloc(commentAllocatedLength);
    commentLength = 0;
    commentText[0] = '\0';
    }
  else if (commentLength + n + 2 > commentAllocatedLength)
    {
    commentAllocatedLength = commentAllocatedLength + commentLength + n + 2;
    commentText = (char *)realloc(commentText, commentAllocatedLength);
    if (!commentText)
      {
      fprintf(stderr, "Wrapping: out of memory\n");
      exit(1);
      }
    }

  if (n > 0)
    {
    memcpy(&commentText[commentLength], line, n);
    }
  commentLength += n;
  commentText[commentLength++] = '\n';
  commentText[commentLength] = '\0';
}

/* Store a doxygen comment */
void storeComment()
{
  CommentInfo *info = (CommentInfo *)malloc(sizeof(CommentInfo));
  vtkParse_InitComment(info);
  info->Type = commentType;
  info->Name = commentTarget;
  info->Comment = vtkstrdup(getComment());

  if (commentType >= DOX_COMMAND_DEFGROUP)
    {
    /* comment has no scope, it is global to the project */
    vtkParse_AddCommentToNamespace(data->Contents, info);
    }
  else
    {
    /* comment is scoped to current namespace */
    if (currentClass)
      {
      vtkParse_AddCommentToClass(currentClass, info);
      }
    else
      {
      vtkParse_AddCommentToNamespace(currentNamespace, info);
      }
    }
}

/* Apply a doxygen trailing comment to the previous item */
void applyComment(ClassInfo *cls)
{
  int i;
  ItemInfo *item;
  const char *comment = vtkstrdup(getComment());

  i = cls->NumberOfItems;
  if (i > 0)
    {
    item = &cls->Items[--i];
    if (item->Type == VTK_NAMESPACE_INFO)
      {
      cls->Namespaces[item->Index]->Comment = comment;
      }
    else if (item->Type == VTK_CLASS_INFO ||
             item->Type == VTK_STRUCT_INFO ||
             item->Type == VTK_UNION_INFO)
      {
      cls->Classes[item->Index]->Comment = comment;
      }
    else if (item->Type == VTK_ENUM_INFO)
      {
      cls->Enums[item->Index]->Comment = comment;
      }
    else if (item->Type == VTK_FUNCTION_INFO)
      {
      cls->Functions[item->Index]->Comment = comment;
      }
    else if (item->Type == VTK_VARIABLE_INFO)
      {
      cls->Variables[item->Index]->Comment = comment;
      }
    else if (item->Type == VTK_CONSTANT_INFO)
      {
      cls->Constants[item->Index]->Comment = comment;
      }
    else if (item->Type == VTK_TYPEDEF_INFO)
      {
      cls->Typedefs[item->Index]->Comment = comment;
      }
    else if (item->Type == VTK_USING_INFO)
      {
      cls->Usings[item->Index]->Comment = comment;
      }
    }
}

/* This is called when a comment block ends */
void closeComment()
{
  const char *cp;
  size_t l;

  switch (commentState)
    {
    case ClosedComment:
      clearComment();
      break;
    case NormalComment:
      /* Make comment persist until a new comment starts */
      commentState = StickyComment;
      break;
    case NameComment:
      /* For NameComment, strip the comment */
      cp = getComment();
      l = strlen(cp);
      while (l > 0 &&
             (cp[l-1] == '\n' || cp[l-1] == '\r' || cp[l-1] == ' '))
        {
        l--;
        }
      data->NameComment = vtkstrndup(cp, l);
      clearComment();
      break;
    case DescriptionComment:
      data->Description = vtkstrdup(getComment());
      clearComment();
      break;
    case SeeAlsoComment:
      data->SeeAlso = vtkstrdup(getComment());
      clearComment();
      break;
    case CaveatsComment:
      data->Caveats = vtkstrdup(getComment());
      clearComment();
      break;
    case DoxygenComment:
      if (commentType == DOX_COMMAND_OTHER)
        {
        /* Apply only to next item unless within a member group */
        commentState = (commentMemberGroup ? StickyComment : ClosedComment);
        }
      else
        {
        /* Comment might not apply to next item, so store it */
        storeComment();
        clearComment();
        }
      break;
    case TrailingComment:
      if (currentClass)
        {
        applyComment(currentClass);
        }
      else
        {
        applyComment(currentNamespace);
        }
      clearComment();
      break;
    }
}

/* This is called when a blank line occurs in the header file */
void commentBreak()
{
  if (!commentMemberGroup && commentState == StickyComment)
    {
    clearComment();
    }
  else if (commentState == DoxygenComment)
    {
    /* blank lines only end targeted doxygen comments */
    if (commentType != DOX_COMMAND_OTHER)
      {
      closeComment();
      }
    }
  else
    {
    /* blank lines end VTK comments */
    closeComment();
    }
}

/* This is called when doxygen @{ or @} are encountered */
void setCommentMemberGroup(int g)
{
  commentMemberGroup = g;
  clearComment();
}

/* Assign comments to the items that they apply to */
void assignComments(ClassInfo *cls)
{
  int i, j;
  int t;
  const char *name;
  const char *comment;

  for (i = 0; i < cls->NumberOfComments; i++)
    {
    t = cls->Comments[i]->Type;
    name = cls->Comments[i]->Name;
    comment = cls->Comments[i]->Comment;
    /* find the item the comment applies to */
    if (t == DOX_COMMAND_CLASS ||
        t == DOX_COMMAND_STRUCT ||
        t == DOX_COMMAND_UNION)
      {
      for (j = 0; j < cls->NumberOfClasses; j++)
        {
        if (cls->Classes[j]->Name && name &&
            strcmp(cls->Classes[j]->Name, name) == 0)
          {
          cls->Classes[j]->Comment = comment;
          break;
          }
        }
      }
    else if (t == DOX_COMMAND_ENUM)
      {
      for (j = 0; j < cls->NumberOfEnums; j++)
        {
        if (cls->Enums[j]->Name && name &&
            strcmp(cls->Enums[j]->Name, name) == 0)
          {
          cls->Enums[j]->Comment = comment;
          break;
          }
        }
      }
    else if (t == DOX_COMMAND_TYPEDEF)
      {
      for (j = 0; j < cls->NumberOfTypedefs; j++)
        {
        if (cls->Typedefs[j]->Name && name &&
            strcmp(cls->Typedefs[j]->Name, name) == 0)
          {
          cls->Typedefs[j]->Comment = comment;
          break;
          }
        }
      }
    else if (t == DOX_COMMAND_FN)
      {
      for (j = 0; j < cls->NumberOfFunctions; j++)
        {
        if (cls->Functions[j]->Name && name &&
            strcmp(cls->Functions[j]->Name, name) == 0)
          {
          cls->Functions[j]->Comment = comment;
          break;
          }
        }
      }
    else if (t == DOX_COMMAND_VAR)
      {
      for (j = 0; j < cls->NumberOfVariables; j++)
        {
        if (cls->Variables[j]->Name && name &&
            strcmp(cls->Variables[j]->Name, name) == 0)
          {
          cls->Variables[j]->Comment = comment;
          break;
          }
        }
      for (j = 0; j < cls->NumberOfConstants; j++)
        {
        if (cls->Constants[j]->Name && name &&
            strcmp(cls->Constants[j]->Name, name) == 0)
          {
          cls->Constants[j]->Comment = comment;
          break;
          }
        }
      }
    else if (t == DOX_COMMAND_NAMESPACE)
      {
      for (j = 0; j < cls->NumberOfNamespaces; j++)
        {
        if (cls->Namespaces[j]->Name && name &&
            strcmp(cls->Namespaces[j]->Name, name) == 0)
          {
          cls->Namespaces[j]->Comment = comment;
          break;
          }
        }
      }
    }

  /* recurse into child classes */
  for (i = 0; i < cls->NumberOfClasses; i++)
    {
    if (cls->Classes[i])
      {
      assignComments(cls->Classes[i]);
      }
    }

  /* recurse into child namespaces */
  for (i = 0; i < cls->NumberOfNamespaces; i++)
    {
    if (cls->Namespaces[i])
      {
      assignComments(cls->Namespaces[i]);
      }
    }
}

/*----------------------------------------------------------------
 * Macros
 */

/* "private" variables */
const char *macroName = NULL;
int macroUsed = 0;
int macroEnded = 0;

const char *getMacro()
{
  if (macroUsed == 0)
    {
    macroUsed = macroEnded;
    return macroName;
    }
  return NULL;
}


/*----------------------------------------------------------------
 * Namespaces
 *
 * operates on: currentNamespace
 */

/* "private" variables */
NamespaceInfo *namespaceStack[10];
int namespaceDepth = 0;

/* enter a namespace */
void pushNamespace(const char *name)
{
  int i;
  NamespaceInfo *oldNamespace = currentNamespace;

  for (i = 0; i < oldNamespace->NumberOfNamespaces; i++)
    {
    /* see if the namespace already exists */
    if (strcmp(name, oldNamespace->Namespaces[i]->Name) == 0)
      {
      currentNamespace = oldNamespace->Namespaces[i];
      }
    }

  /* create a new namespace */
  if (i == oldNamespace->NumberOfNamespaces)
    {
    currentNamespace = (NamespaceInfo *)malloc(sizeof(NamespaceInfo));
    vtkParse_InitNamespace(currentNamespace);
    currentNamespace->Name = name;
    vtkParse_AddNamespaceToNamespace(oldNamespace, currentNamespace);
    }

  namespaceStack[namespaceDepth++] = oldNamespace;
}

/* leave the namespace */
void popNamespace()
{
  currentNamespace = namespaceStack[--namespaceDepth];
}


/*----------------------------------------------------------------
 * Classes
 *
 * operates on: currentClass, access_level
 */

/* "private" variables */
ClassInfo *classStack[10];
parse_access_t classAccessStack[10];
int classDepth = 0;

/* start an internal class definition */
void pushClass()
{
  classAccessStack[classDepth] = access_level;
  classStack[classDepth++] = currentClass;
}

/* leave the internal class */
void popClass()
{
  currentClass = classStack[--classDepth];
  access_level = classAccessStack[classDepth];
}


/*----------------------------------------------------------------
 * Templates
 *
 * operates on: currentTemplate
 */

/* "private" variables */
TemplateInfo *templateStack[10];
int templateDepth = 0;

/* begin a template */
void startTemplate()
{
  currentTemplate = (TemplateInfo *)malloc(sizeof(TemplateInfo));
  vtkParse_InitTemplate(currentTemplate);
}

/* clear a template, if set */
void clearTemplate()
{
  if (currentTemplate)
    {
    free(currentTemplate);
    }
  currentTemplate = NULL;
}

/* push the template onto the stack, and start a new one */
void pushTemplate()
{
  templateStack[templateDepth++] = currentTemplate;
  startTemplate();
}

/* pop a template off the stack */
void popTemplate()
{
  currentTemplate = templateStack[--templateDepth];
}

/*----------------------------------------------------------------
 * Function signatures
 *
 * operates on: currentFunction
 */

/* "private" variables */
int sigClosed = 0;
size_t sigMark[10];
size_t sigLength = 0;
size_t sigAllocatedLength = 0;
int sigMarkDepth = 0;
char *signature = NULL;

/* start a new signature */
void startSig()
{
  signature = NULL;
  sigLength = 0;
  sigAllocatedLength = 0;
  sigClosed = 0;
  sigMarkDepth = 0;
  sigMark[0] = 0;
}

/* get the signature */
const char *getSig()
{
  return signature;
}

/* get the signature length */
size_t getSigLength()
{
  return sigLength;
}

/* reset the sig to the specified length */
void resetSig(size_t n)
{
  if (n < sigLength)
    {
    sigLength = n;
    }
}

/* reallocate Signature if n chars cannot be appended */
void checkSigSize(size_t n)
{
  const char *ccp;

  if (sigAllocatedLength == 0)
    {
    sigLength = 0;
    sigAllocatedLength = 80 + n;
    signature = vtkParse_NewString(data->Strings, sigAllocatedLength);
    signature[0] = '\0';
    }
  else if (sigLength + n > sigAllocatedLength)
    {
    sigAllocatedLength += sigLength + n;
    ccp = signature;
    signature = vtkParse_NewString(data->Strings, sigAllocatedLength);
    strncpy(signature, ccp, sigLength);
    signature[sigLength] = '\0';
    }
}

/* close the signature, i.e. allow no more additions to it */
void closeSig()
{
  sigClosed = 1;
}

/* re-open the signature */
void openSig()
{
  sigClosed = 0;
}

/* insert text at the beginning of the signature */
void preSig(const char *arg)
{
  if (!sigClosed)
    {
    size_t n = strlen(arg);
    checkSigSize(n);
    if (n > 0)
      {
      memmove(&signature[n], signature, sigLength);
      strncpy(signature, arg, n);
      sigLength += n;
      }
    signature[sigLength] = '\0';
    }
}

/* append text to the end of the signature */
void postSig(const char *arg)
{
  if (!sigClosed)
    {
    size_t n = strlen(arg);
    checkSigSize(n);
    if (n > 0)
      {
      strncpy(&signature[sigLength], arg, n);
      sigLength += n;
      }
    signature[sigLength] = '\0';
    }
}

/* set a mark in the signature for later operations */
void markSig()
{
  sigMark[sigMarkDepth] = 0;
  if (signature)
    {
    sigMark[sigMarkDepth] = sigLength;
    }
  sigMarkDepth++;
}

/* get the contents of the sig from the mark, and clear the mark */
const char *copySig()
{
  const char *cp = NULL;
  if (sigMarkDepth > 0)
    {
    sigMarkDepth--;
    }
  if (signature)
    {
    cp = &signature[sigMark[sigMarkDepth]];
    }
  return vtkstrdup(cp);
}

/* cut the sig from the mark to the current location, and clear the mark */
const char *cutSig()
{
  const char *cp = NULL;
  if (sigMarkDepth > 0)
    {
    sigMarkDepth--;
    }
  if (signature)
    {
    sigLength = sigMark[sigMarkDepth];
    cp = vtkstrdup(&signature[sigLength]);
    signature[sigLength] = 0;
    }
  return cp;
}

/* swap the signature text using the mark as the radix */
void swapSig()
{
  if (sigMarkDepth > 0)
    {
    sigMarkDepth--;
    }
  if (signature && sigMark[sigMarkDepth] > 0)
    {
    size_t i, m, n, nn;
    char c;
    char *cp;
    cp = signature;
    n = sigLength;
    m = sigMark[sigMarkDepth];
    nn = m/2;
    for (i = 0; i < nn; i++)
      {
      c = cp[i]; cp[i] = cp[m-i-1]; cp[m-i-1] = c;
      }
    nn = (n-m)/2;
    for (i = 0; i < nn; i++)
      {
      c = cp[i+m]; cp[i+m] = cp[n-i-1]; cp[n-i-1] = c;
      }
    nn = n/2;
    for (i = 0; i < nn; i++)
      {
      c = cp[i]; cp[i] = cp[n-i-1]; cp[n-i-1] = c;
      }
    }
}

/* chop the last space from the signature */
void chopSig(void)
{
  if (signature)
    {
    size_t n = sigLength;
    if (n > 0 && signature[n-1] == ' ')
      {
      signature[n-1] = '\0';
      sigLength--;
      }
    }
}

/*----------------------------------------------------------------
 * Subroutines for building a type
 */

/* "private" variables */
unsigned int storedType;
unsigned int typeStack[10];
int typeDepth = 0;

/* save the type on the stack */
void pushType()
{
  typeStack[typeDepth++] = storedType;
}

/* pop the type stack */
void popType()
{
  storedType = typeStack[--typeDepth];
}

/* clear the storage type */
void clearType()
{
  storedType = 0;
}

/* save the type */
void setTypeBase(unsigned int base)
{
  storedType &= ~(unsigned int)(VTK_PARSE_BASE_TYPE);
  storedType |= base;
}

/* set a type modifier bit */
void setTypeMod(unsigned int mod)
{
  storedType |= mod;
}

/* modify the indirection (pointers, refs) in the storage type */
void setTypePtr(unsigned int ind)
{
  storedType &= ~(unsigned int)(VTK_PARSE_INDIRECT | VTK_PARSE_RVALUE);
  ind &= (VTK_PARSE_INDIRECT | VTK_PARSE_RVALUE);
  storedType |= ind;
}

/* retrieve the storage type */
unsigned int getType()
{
  return storedType;
}

/* combine two primitive type parts, e.g. "long int" */
unsigned int buildTypeBase(unsigned int a, unsigned int b)
{
  unsigned int base = (a & VTK_PARSE_BASE_TYPE);
  unsigned int basemod = (b & VTK_PARSE_BASE_TYPE);

  switch (base)
    {
    case 0:
      base = basemod;
      break;
    case VTK_PARSE_UNSIGNED_INT:
      base = (basemod | VTK_PARSE_UNSIGNED);
      break;
    case VTK_PARSE_INT:
      base = basemod;
      if (base == VTK_PARSE_CHAR)
        {
        base = VTK_PARSE_SIGNED_CHAR;
        }
      break;
    case VTK_PARSE_CHAR:
      if (basemod == VTK_PARSE_INT)
        {
        base = VTK_PARSE_SIGNED_CHAR;
        }
      else if (basemod == VTK_PARSE_UNSIGNED_INT)
        {
        base = VTK_PARSE_UNSIGNED_CHAR;
        }
      break;
    case VTK_PARSE_SHORT:
      if (basemod == VTK_PARSE_UNSIGNED_INT)
        {
        base = VTK_PARSE_UNSIGNED_SHORT;
        }
      break;
    case VTK_PARSE_LONG:
      if (basemod == VTK_PARSE_UNSIGNED_INT)
        {
        base = VTK_PARSE_UNSIGNED_LONG;
        }
      else if (basemod == VTK_PARSE_LONG)
        {
        base = VTK_PARSE_LONG_LONG;
        }
      else if (basemod == VTK_PARSE_DOUBLE)
        {
        base = VTK_PARSE_LONG_DOUBLE;
        }
      break;
    case VTK_PARSE_UNSIGNED_LONG:
      if (basemod == VTK_PARSE_LONG)
        {
        base = VTK_PARSE_UNSIGNED_LONG_LONG;
        }
      break;
    case VTK_PARSE_LONG_LONG:
      if (basemod == VTK_PARSE_UNSIGNED_INT)
        {
        base = VTK_PARSE_UNSIGNED_LONG_LONG;
        }
      break;
    case VTK_PARSE___INT64:
      if (basemod == VTK_PARSE_UNSIGNED_INT)
        {
        base = VTK_PARSE_UNSIGNED___INT64;
        }
      break;
    case VTK_PARSE_DOUBLE:
      if (basemod == VTK_PARSE_LONG)
        {
        base = VTK_PARSE_LONG_DOUBLE;
        }
      break;
    }

  return ((a & ~(unsigned int)(VTK_PARSE_BASE_TYPE)) | base);
}


/*----------------------------------------------------------------
 * Array information
 */

/* "private" variables */
int numberOfDimensions = 0;
const char **arrayDimensions = NULL;

/* clear the array counter */
void clearArray(void)
{
  numberOfDimensions = 0;
  arrayDimensions = NULL;
}

/* add another dimension */
void pushArraySize(const char *size)
{
  vtkParse_AddStringToArray(&arrayDimensions, &numberOfDimensions,
                            size);
}

/* add another dimension to the front */
void pushArrayFront(const char *size)
{
  int i;

  vtkParse_AddStringToArray(&arrayDimensions, &numberOfDimensions, 0);

  for (i = numberOfDimensions-1; i > 0; i--)
    {
    arrayDimensions[i] = arrayDimensions[i-1];
    }

  arrayDimensions[0] = size;
}

/* get the number of dimensions */
int getArrayNDims()
{
  return numberOfDimensions;
}

/* get the whole array */
const char **getArray()
{
  if (numberOfDimensions > 0)
    {
    return arrayDimensions;
    }
  return NULL;
}

/*----------------------------------------------------------------
 * Variables and Parameters
 */

/* "private" variables */
const char *currentVarName = 0;
const char *currentVarValue = 0;
const char *currentId = 0;

/* clear the var Id */
void clearVarName(void)
{
  currentVarName = NULL;
}

/* set the var Id */
void setVarName(const char *text)
{
  currentVarName = text;
}

/* return the var id */
const char *getVarName()
{
  return currentVarName;
}

/* variable value -------------- */

/* clear the var value */
void clearVarValue(void)
{
  currentVarValue = NULL;
}

/* set the var value */
void setVarValue(const char *text)
{
  currentVarValue = text;
}

/* return the var value */
const char *getVarValue()
{
  return currentVarValue;
}

/* variable type -------------- */

/* clear the current Id */
void clearTypeId(void)
{
  currentId = NULL;
}

/* set the current Id, it is sticky until cleared */
void setTypeId(const char *text)
{
  if (currentId == NULL)
    {
    currentId = text;
    }
}

/* set the signature and type together */
void typeSig(const char *text)
{
  postSig(text);
  postSig(" ");

  if (currentId == 0)
    {
    setTypeId(text);
    }
}

/* return the current Id */
const char *getTypeId()
{
  return currentId;
}

/*----------------------------------------------------------------
 * Specifically for function pointers, the scope (i.e. class) that
 * the function is a method of.
 */

const char *pointerScopeStack[10];
int pointerScopeDepth = 0;

/* save the scope for scoped method pointers */
void scopeSig(const char *scope)
{
  if (scope && scope[0] != '\0')
    {
    postSig(scope);
    }
  else
    {
    scope = NULL;
    }
  pointerScopeStack[pointerScopeDepth++] = vtkstrdup(scope);
}

/* get the scope back */
const char *getScope()
{
  return pointerScopeStack[--pointerScopeDepth];
}

/*----------------------------------------------------------------
 * Function stack
 *
 * operates on: currentFunction
 */

/* "private" variables */
FunctionInfo *functionStack[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const char *functionVarNameStack[10];
const char *functionTypeIdStack[10];
int functionDepth = 0;

void pushFunction()
{
  functionStack[functionDepth] = currentFunction;
  currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
  vtkParse_InitFunction(currentFunction);
  if (!functionStack[functionDepth])
    {
    startSig();
    }
  functionVarNameStack[functionDepth] = getVarName();
  functionTypeIdStack[functionDepth] = getTypeId();
  pushType();
  clearType();
  clearVarName();
  clearTypeId();
  functionDepth++;
  functionStack[functionDepth] = 0;
}

void popFunction()
{
  FunctionInfo *newFunction = currentFunction;

  --functionDepth;
  currentFunction = functionStack[functionDepth];
  clearVarName();
  if (functionVarNameStack[functionDepth])
    {
    setVarName(functionVarNameStack[functionDepth]);
    }
  clearTypeId();
  if (functionTypeIdStack[functionDepth])
    {
    setTypeId(functionTypeIdStack[functionDepth]);
    }
  popType();

  functionStack[functionDepth+1] = newFunction;
}

FunctionInfo *getFunction()
{
  return functionStack[functionDepth+1];
}

/*----------------------------------------------------------------
 * Attributes
 */

int attributeRole = 0;

/* Set kind of attributes to collect in attribute_specifier_seq */
void setAttributeRole(int x)
{
  attributeRole = x;
}

/* Get the current kind of attribute */
int getAttributeRole()
{
  return attributeRole;
}

/* Ignore attributes until further notice */
void clearAttributeRole()
{
  attributeRole = 0;
}

/*----------------------------------------------------------------
 * Utility methods
 */

/* prepend a scope:: to a name */
void prepend_scope(char *cp, const char *arg)
{
  size_t i, j, m, n;
  int depth;

  m = strlen(cp);
  n = strlen(arg);
  i = m;
  while (i > 0 &&
         (vtkParse_CharType(cp[i-1], CPRE_XID) ||
          cp[i-1] == ':' || cp[i-1] == '>'))
    {
    i--;
    if (cp[i] == '>')
      {
      depth = 1;
      while (i > 0)
        {
        i--;
        if (cp[i] == '<')
          {
          if (--depth == 0)
            {
            break;
            }
          }
        if (cp[i] == '>')
          {
          depth++;
          }
        }
      }
    }

  for (j = m; j > i; j--)
    {
    cp[j+n+1] = cp[j-1];
    }
  for (j = 0; j < n; j++)
    {
    cp[j+i] = arg[j];
    }
  cp[n+i] = ':'; cp[n+i+1] = ':';
  cp[m+n+2] = '\0';
}

/* expand a type by including pointers from another */
unsigned int add_indirection(unsigned int type1, unsigned int type2)
{
  unsigned int ptr1 = (type1 & VTK_PARSE_POINTER_MASK);
  unsigned int ptr2 = (type2 & VTK_PARSE_POINTER_MASK);
  unsigned int reverse = 0;
  unsigned int result;

  /* one of type1 or type2 will only have VTK_PARSE_INDIRECT, but
   * we don't know which one. */
  result = ((type1 & ~VTK_PARSE_POINTER_MASK) |
            (type2 & ~VTK_PARSE_POINTER_MASK));

  /* if there are two ampersands, it is an rvalue reference */
  if ((type1 & type2 & VTK_PARSE_REF) != 0)
    {
    result |= VTK_PARSE_RVALUE;
    }

  while (ptr2)
    {
    reverse = ((reverse << 2) | (ptr2 & VTK_PARSE_POINTER_LOWMASK));
    ptr2 = ((ptr2 >> 2) & VTK_PARSE_POINTER_MASK);
    }

  while (reverse)
    {
    ptr1 = ((ptr1 << 2) | (reverse & VTK_PARSE_POINTER_LOWMASK));
    reverse = ((reverse >> 2) & VTK_PARSE_POINTER_MASK);

    /* make sure we don't exceed the VTK_PARSE_POINTER bitfield */
    if ((ptr1 & VTK_PARSE_POINTER_MASK) != ptr1)
      {
      ptr1 = VTK_PARSE_BAD_INDIRECT;
      break;
      }
    }

  return (ptr1 | result);
}

/* There is only one array, so add any parenthetical indirection to it */
unsigned int add_indirection_to_array(unsigned int type)
{
  unsigned int ptrs = (type & VTK_PARSE_POINTER_MASK);
  unsigned int result = (type & ~VTK_PARSE_POINTER_MASK);
  unsigned int reverse = 0;

  if ((type & VTK_PARSE_INDIRECT) == VTK_PARSE_BAD_INDIRECT)
    {
    return (result | VTK_PARSE_BAD_INDIRECT);
    }

  while (ptrs)
    {
    reverse = ((reverse << 2) | (ptrs & VTK_PARSE_POINTER_LOWMASK));
    ptrs = ((ptrs >> 2) & VTK_PARSE_POINTER_MASK);
    }

  /* I know the reversal makes no difference, but it is still
   * nice to add the pointers in the correct order, just in
   * case const pointers are thrown into the mix. */
  while (reverse)
    {
    pushArrayFront("");
    reverse = ((reverse >> 2) & VTK_PARSE_POINTER_MASK);
    }

  return result;
}




# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ID = 258,
    VTK_ID = 259,
    QT_ID = 260,
    StdString = 261,
    UnicodeString = 262,
    OSTREAM = 263,
    ISTREAM = 264,
    LP = 265,
    LA = 266,
    STRING_LITERAL = 267,
    INT_LITERAL = 268,
    HEX_LITERAL = 269,
    BIN_LITERAL = 270,
    OCT_LITERAL = 271,
    FLOAT_LITERAL = 272,
    CHAR_LITERAL = 273,
    ZERO = 274,
    NULLPTR = 275,
    SSIZE_T = 276,
    SIZE_T = 277,
    NULLPTR_T = 278,
    BEGIN_ATTRIB = 279,
    STRUCT = 280,
    CLASS = 281,
    UNION = 282,
    ENUM = 283,
    PUBLIC = 284,
    PRIVATE = 285,
    PROTECTED = 286,
    CONST = 287,
    VOLATILE = 288,
    MUTABLE = 289,
    STATIC = 290,
    THREAD_LOCAL = 291,
    VIRTUAL = 292,
    EXPLICIT = 293,
    INLINE = 294,
    CONSTEXPR = 295,
    FRIEND = 296,
    EXTERN = 297,
    OPERATOR = 298,
    TEMPLATE = 299,
    THROW = 300,
    TRY = 301,
    CATCH = 302,
    NOEXCEPT = 303,
    DECLTYPE = 304,
    TYPENAME = 305,
    TYPEDEF = 306,
    NAMESPACE = 307,
    USING = 308,
    NEW = 309,
    DELETE = 310,
    DEFAULT = 311,
    STATIC_CAST = 312,
    DYNAMIC_CAST = 313,
    CONST_CAST = 314,
    REINTERPRET_CAST = 315,
    OP_LSHIFT_EQ = 316,
    OP_RSHIFT_EQ = 317,
    OP_LSHIFT = 318,
    OP_RSHIFT_A = 319,
    OP_DOT_POINTER = 320,
    OP_ARROW_POINTER = 321,
    OP_ARROW = 322,
    OP_INCR = 323,
    OP_DECR = 324,
    OP_PLUS_EQ = 325,
    OP_MINUS_EQ = 326,
    OP_TIMES_EQ = 327,
    OP_DIVIDE_EQ = 328,
    OP_REMAINDER_EQ = 329,
    OP_AND_EQ = 330,
    OP_OR_EQ = 331,
    OP_XOR_EQ = 332,
    OP_LOGIC_AND = 333,
    OP_LOGIC_OR = 334,
    OP_LOGIC_EQ = 335,
    OP_LOGIC_NEQ = 336,
    OP_LOGIC_LEQ = 337,
    OP_LOGIC_GEQ = 338,
    ELLIPSIS = 339,
    DOUBLE_COLON = 340,
    OTHER = 341,
    AUTO = 342,
    VOID = 343,
    BOOL = 344,
    FLOAT = 345,
    DOUBLE = 346,
    INT = 347,
    SHORT = 348,
    LONG = 349,
    INT64__ = 350,
    CHAR = 351,
    CHAR16_T = 352,
    CHAR32_T = 353,
    WCHAR_T = 354,
    SIGNED = 355,
    UNSIGNED = 356,
    IdType = 357,
    TypeInt8 = 358,
    TypeUInt8 = 359,
    TypeInt16 = 360,
    TypeUInt16 = 361,
    TypeInt32 = 362,
    TypeUInt32 = 363,
    TypeInt64 = 364,
    TypeUInt64 = 365,
    TypeFloat32 = 366,
    TypeFloat64 = 367,
    SetMacro = 368,
    GetMacro = 369,
    SetStringMacro = 370,
    GetStringMacro = 371,
    SetClampMacro = 372,
    SetObjectMacro = 373,
    GetObjectMacro = 374,
    BooleanMacro = 375,
    SetVector2Macro = 376,
    SetVector3Macro = 377,
    SetVector4Macro = 378,
    SetVector6Macro = 379,
    GetVector2Macro = 380,
    GetVector3Macro = 381,
    GetVector4Macro = 382,
    GetVector6Macro = 383,
    SetVectorMacro = 384,
    GetVectorMacro = 385,
    ViewportCoordinateMacro = 386,
    WorldCoordinateMacro = 387,
    TypeMacro = 388,
    VTK_BYTE_SWAP_DECL = 389
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{


  const char   *str;
  unsigned int  integer;


};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);


/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Default (constant) value used for initialization for null
   right-hand sides.  Unlike the standard yacc.c template, here we set
   the default value of $$ to a zeroed-out value.  Since the default
   value is undefined, this behavior is technically correct.  */
static YYSTYPE yyval_default;

/* Copy the second part of user declarations.  */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YYFREE
# define YYFREE free
#endif
#ifndef YYMALLOC
# define YYMALLOC malloc
#endif
#ifndef YYREALLOC
# define YYREALLOC realloc
#endif

#define YYSIZEMAX ((size_t) -1)

#ifdef __cplusplus
   typedef bool yybool;
#else
   typedef unsigned char yybool;
#endif
#define yytrue 1
#define yyfalse 0

#ifndef YYSETJMP
# include <setjmp.h>
# define YYJMP_BUF jmp_buf
# define YYSETJMP(Env) setjmp (Env)
/* Pacify clang.  */
# define YYLONGJMP(Env, Val) (longjmp (Env, Val), YYASSERT (0))
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__)                                              \
     || defined __SUNPRO_C                                           \
     && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if 1
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#ifndef YYASSERT
# define YYASSERT(Condition) ((void) ((Condition) || (abort (), 0)))
#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   8723

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  158
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  294
/* YYNRULES -- Number of rules.  */
#define YYNRULES  731
/* YYNRULES -- Number of states.  */
#define YYNSTATES  1217
/* YYMAXRHS -- Maximum number of symbols on right-hand side of rule.  */
#define YYMAXRHS 10
/* YYMAXLEFT -- Maximum number of symbols to the left of a handle
   accessed by $0, $-1, etc., in any rule.  */
#define YYMAXLEFT 0

/* YYTRANSLATE(X) -- Bison symbol number corresponding to X.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   389

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   154,     2,     2,     2,   150,   147,     2,
     141,   142,   148,   153,   140,   152,   157,   151,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   139,   135,
     143,   138,   149,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   144,     2,   145,   156,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   136,   155,   137,   146,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134
};

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,  1972,  1972,  1974,  1976,  1975,  1986,  1987,  1988,  1989,
    1990,  1991,  1992,  1993,  1994,  1995,  1996,  1997,  1998,  1999,
    2000,  2001,  2002,  2003,  2006,  2007,  2008,  2009,  2010,  2011,
    2014,  2015,  2022,  2029,  2030,  2030,  2034,  2041,  2042,  2045,
    2046,  2047,  2050,  2051,  2054,  2054,  2069,  2068,  2074,  2080,
    2079,  2084,  2090,  2091,  2092,  2095,  2097,  2099,  2102,  2103,
    2106,  2107,  2109,  2111,  2110,  2119,  2123,  2124,  2125,  2128,
    2129,  2130,  2131,  2132,  2133,  2134,  2135,  2136,  2137,  2138,
    2139,  2140,  2141,  2142,  2143,  2146,  2147,  2148,  2149,  2150,
    2153,  2154,  2155,  2156,  2159,  2160,  2163,  2165,  2168,  2173,
    2174,  2177,  2178,  2181,  2182,  2183,  2194,  2195,  2196,  2200,
    2201,  2205,  2205,  2218,  2224,  2232,  2233,  2234,  2237,  2238,
    2238,  2242,  2243,  2245,  2246,  2247,  2247,  2255,  2259,  2260,
    2263,  2265,  2267,  2268,  2271,  2272,  2280,  2281,  2284,  2285,
    2287,  2289,  2291,  2295,  2297,  2298,  2301,  2304,  2305,  2308,
    2309,  2308,  2313,  2354,  2357,  2358,  2359,  2361,  2363,  2365,
    2369,  2372,  2372,  2404,  2407,  2406,  2424,  2426,  2425,  2430,
    2432,  2430,  2434,  2436,  2434,  2438,  2441,  2438,  2452,  2453,
    2456,  2457,  2459,  2460,  2463,  2463,  2473,  2474,  2482,  2483,
    2484,  2485,  2488,  2491,  2492,  2493,  2496,  2497,  2498,  2501,
    2502,  2503,  2507,  2508,  2509,  2510,  2513,  2514,  2515,  2519,
    2525,  2518,  2536,  2540,  2540,  2552,  2551,  2560,  2564,  2567,
    2576,  2577,  2580,  2580,  2581,  2582,  2588,  2593,  2594,  2595,
    2598,  2601,  2602,  2604,  2605,  2608,  2608,  2616,  2617,  2618,
    2621,  2623,  2624,  2628,  2627,  2641,  2642,  2641,  2661,  2661,
    2665,  2666,  2669,  2670,  2673,  2679,  2680,  2680,  2683,  2684,
    2684,  2686,  2688,  2692,  2694,  2692,  2718,  2719,  2722,  2722,
    2724,  2724,  2726,  2726,  2731,  2732,  2732,  2740,  2743,  2813,
    2814,  2816,  2817,  2817,  2820,  2823,  2824,  2828,  2840,  2839,
    2858,  2860,  2860,  2878,  2878,  2880,  2884,  2885,  2886,  2885,
    2891,  2893,  2894,  2895,  2896,  2897,  2898,  2901,  2902,  2906,
    2907,  2911,  2912,  2915,  2916,  2920,  2921,  2922,  2923,  2926,
    2927,  2930,  2930,  2933,  2934,  2937,  2937,  2941,  2942,  2942,
    2949,  2950,  2953,  2954,  2955,  2956,  2957,  2960,  2962,  2964,
    2968,  2970,  2972,  2974,  2976,  2978,  2980,  2980,  2985,  2988,
    2991,  2994,  2994,  3002,  3002,  3011,  3012,  3013,  3014,  3015,
    3016,  3017,  3018,  3019,  3020,  3021,  3022,  3023,  3024,  3025,
    3026,  3027,  3028,  3029,  3030,  3031,  3038,  3039,  3040,  3041,
    3042,  3043,  3044,  3050,  3051,  3054,  3055,  3057,  3058,  3061,
    3062,  3065,  3066,  3067,  3068,  3071,  3072,  3073,  3074,  3075,
    3079,  3080,  3081,  3084,  3085,  3088,  3089,  3097,  3100,  3100,
    3102,  3102,  3106,  3107,  3109,  3113,  3114,  3116,  3116,  3119,
    3121,  3125,  3128,  3128,  3130,  3130,  3134,  3137,  3137,  3139,
    3139,  3143,  3144,  3146,  3148,  3150,  3152,  3154,  3158,  3159,
    3162,  3163,  3164,  3165,  3166,  3167,  3168,  3169,  3170,  3171,
    3172,  3173,  3174,  3175,  3176,  3177,  3178,  3179,  3180,  3181,
    3182,  3185,  3186,  3187,  3188,  3189,  3190,  3191,  3192,  3193,
    3194,  3195,  3196,  3197,  3198,  3199,  3219,  3220,  3221,  3222,
    3225,  3229,  3233,  3233,  3237,  3238,  3253,  3254,  3279,  3279,
    3283,  3283,  3287,  3287,  3291,  3291,  3295,  3295,  3299,  3299,
    3302,  3303,  3306,  3308,  3309,  3310,  3311,  3314,  3314,  3318,
    3319,  3322,  3323,  3326,  3327,  3335,  3335,  3345,  3346,  3346,
    3345,  3355,  3355,  3365,  3365,  3374,  3374,  3374,  3407,  3406,
    3417,  3418,  3418,  3417,  3427,  3445,  3445,  3450,  3450,  3455,
    3455,  3460,  3460,  3465,  3465,  3470,  3470,  3475,  3475,  3480,
    3480,  3485,  3485,  3502,  3502,  3516,  3553,  3591,  3628,  3629,
    3636,  3637,  3638,  3639,  3640,  3641,  3642,  3643,  3644,  3645,
    3646,  3647,  3650,  3651,  3652,  3653,  3654,  3655,  3656,  3657,
    3658,  3659,  3660,  3661,  3662,  3663,  3664,  3665,  3666,  3667,
    3668,  3669,  3670,  3671,  3672,  3673,  3674,  3675,  3676,  3677,
    3678,  3679,  3680,  3681,  3682,  3683,  3686,  3687,  3688,  3689,
    3690,  3691,  3692,  3693,  3694,  3695,  3696,  3697,  3698,  3699,
    3700,  3701,  3702,  3703,  3704,  3705,  3706,  3707,  3708,  3709,
    3710,  3711,  3712,  3713,  3714,  3717,  3718,  3719,  3720,  3721,
    3722,  3723,  3724,  3725,  3732,  3733,  3736,  3737,  3738,  3739,
    3739,  3740,  3743,  3744,  3747,  3748,  3749,  3750,  3780,  3780,
    3781,  3782,  3783,  3784,  3807,  3808,  3811,  3812,  3813,  3814,
    3817,  3818,  3819,  3822,  3823,  3825,  3826,  3828,  3829,  3832,
    3833,  3836,  3837,  3838,  3842,  3841,  3855,  3856,  3859,  3859,
    3861,  3861,  3865,  3865,  3867,  3867,  3869,  3869,  3873,  3873,
    3878,  3879,  3881,  3882,  3885,  3886,  3889,  3890,  3893,  3894,
    3895,  3896,  3897,  3898,  3899,  3900,  3900,  3900,  3900,  3900,
    3901,  3902,  3903,  3904,  3905,  3908,  3911,  3912,  3915,  3918,
    3918,  3918
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ID", "VTK_ID", "QT_ID", "StdString",
  "UnicodeString", "OSTREAM", "ISTREAM", "LP", "LA", "STRING_LITERAL",
  "INT_LITERAL", "HEX_LITERAL", "BIN_LITERAL", "OCT_LITERAL",
  "FLOAT_LITERAL", "CHAR_LITERAL", "ZERO", "NULLPTR", "SSIZE_T", "SIZE_T",
  "NULLPTR_T", "BEGIN_ATTRIB", "STRUCT", "CLASS", "UNION", "ENUM",
  "PUBLIC", "PRIVATE", "PROTECTED", "CONST", "VOLATILE", "MUTABLE",
  "STATIC", "THREAD_LOCAL", "VIRTUAL", "EXPLICIT", "INLINE", "CONSTEXPR",
  "FRIEND", "EXTERN", "OPERATOR", "TEMPLATE", "THROW", "TRY", "CATCH",
  "NOEXCEPT", "DECLTYPE", "TYPENAME", "TYPEDEF", "NAMESPACE", "USING",
  "NEW", "DELETE", "DEFAULT", "STATIC_CAST", "DYNAMIC_CAST", "CONST_CAST",
  "REINTERPRET_CAST", "OP_LSHIFT_EQ", "OP_RSHIFT_EQ", "OP_LSHIFT",
  "OP_RSHIFT_A", "OP_DOT_POINTER", "OP_ARROW_POINTER", "OP_ARROW",
  "OP_INCR", "OP_DECR", "OP_PLUS_EQ", "OP_MINUS_EQ", "OP_TIMES_EQ",
  "OP_DIVIDE_EQ", "OP_REMAINDER_EQ", "OP_AND_EQ", "OP_OR_EQ", "OP_XOR_EQ",
  "OP_LOGIC_AND", "OP_LOGIC_OR", "OP_LOGIC_EQ", "OP_LOGIC_NEQ",
  "OP_LOGIC_LEQ", "OP_LOGIC_GEQ", "ELLIPSIS", "DOUBLE_COLON", "OTHER",
  "AUTO", "VOID", "BOOL", "FLOAT", "DOUBLE", "INT", "SHORT", "LONG",
  "INT64__", "CHAR", "CHAR16_T", "CHAR32_T", "WCHAR_T", "SIGNED",
  "UNSIGNED", "IdType", "TypeInt8", "TypeUInt8", "TypeInt16", "TypeUInt16",
  "TypeInt32", "TypeUInt32", "TypeInt64", "TypeUInt64", "TypeFloat32",
  "TypeFloat64", "SetMacro", "GetMacro", "SetStringMacro",
  "GetStringMacro", "SetClampMacro", "SetObjectMacro", "GetObjectMacro",
  "BooleanMacro", "SetVector2Macro", "SetVector3Macro", "SetVector4Macro",
  "SetVector6Macro", "GetVector2Macro", "GetVector3Macro",
  "GetVector4Macro", "GetVector6Macro", "SetVectorMacro", "GetVectorMacro",
  "ViewportCoordinateMacro", "WorldCoordinateMacro", "TypeMacro",
  "VTK_BYTE_SWAP_DECL", "';'", "'{'", "'}'", "'='", "':'", "','", "'('",
  "')'", "'<'", "'['", "']'", "'~'", "'&'", "'*'", "'>'", "'%'", "'/'",
  "'-'", "'+'", "'!'", "'|'", "'^'", "'.'", "$accept", "translation_unit",
  "opt_declaration_seq", "$@1", "declaration", "template_declaration",
  "explicit_instantiation", "linkage_specification",
  "namespace_definition", "$@2", "namespace_alias_definition",
  "forward_declaration", "simple_forward_declaration", "class_definition",
  "class_specifier", "$@3", "class_head", "$@4", "$@5", "class_key",
  "class_head_name", "class_name", "opt_final", "member_specification",
  "$@6", "member_access_specifier", "member_declaration",
  "template_member_declaration", "friend_declaration",
  "base_specifier_list", "base_specifier", "opt_virtual",
  "opt_access_specifier", "access_specifier", "opaque_enum_declaration",
  "enum_definition", "enum_specifier", "$@7", "enum_head", "enum_key",
  "opt_enum_base", "$@8", "enumerator_list", "enumerator_definition",
  "$@9", "nested_variable_initialization", "ignored_initializer",
  "ignored_class", "ignored_class_body", "typedef_declaration",
  "basic_typedef_declaration", "typedef_declarator_list",
  "typedef_declarator_list_cont", "typedef_declarator",
  "typedef_direct_declarator", "function_direct_declarator", "$@10",
  "$@11", "typedef_declarator_id", "using_declaration", "using_id",
  "using_directive", "alias_declaration", "$@12", "template_head", "$@13",
  "template_parameter_list", "$@14", "template_parameter", "$@15", "$@16",
  "$@17", "$@18", "$@19", "$@20", "opt_ellipsis", "class_or_typename",
  "opt_template_parameter_initializer", "template_parameter_initializer",
  "$@21", "template_parameter_value", "function_definition",
  "function_declaration", "nested_method_declaration",
  "nested_operator_declaration", "method_definition", "method_declaration",
  "operator_declaration", "conversion_function", "$@22", "$@23",
  "conversion_function_id", "operator_function_nr", "$@24",
  "operator_function_sig", "$@25", "operator_function_id", "operator_sig",
  "function_nr", "function_trailer_clause", "function_trailer", "$@26",
  "noexcept_sig", "function_body_as_trailer", "opt_trailing_return_type",
  "trailing_return_type", "$@27", "function_body", "function_try_block",
  "handler_seq", "function_sig", "$@28", "structor_declaration", "$@29",
  "$@30", "structor_sig", "$@31", "opt_ctor_initializer",
  "mem_initializer_list", "mem_initializer",
  "parameter_declaration_clause", "$@32", "parameter_list", "$@33",
  "parameter_declaration", "$@34", "$@35", "opt_initializer",
  "initializer", "$@36", "$@37", "$@38", "constructor_args", "$@39",
  "variable_declaration", "init_declarator_id", "opt_declarator_list",
  "declarator_list_cont", "$@40", "init_declarator",
  "opt_ptr_operator_seq", "direct_abstract_declarator", "$@41",
  "direct_declarator", "$@42", "lp_or_la", "$@43",
  "opt_array_or_parameters", "$@44", "$@45", "function_qualifiers",
  "abstract_declarator", "declarator", "opt_declarator_id",
  "declarator_id", "bitfield_size", "opt_array_decorator_seq",
  "array_decorator_seq", "$@46", "array_decorator_seq_impl",
  "array_decorator", "$@47", "array_size_specifier", "$@48",
  "id_expression", "unqualified_id", "qualified_id",
  "nested_name_specifier", "$@49", "tilde_sig", "identifier_sig",
  "scope_operator_sig", "template_id", "$@50", "decltype_specifier",
  "$@51", "simple_id", "identifier", "opt_decl_specifier_seq",
  "decl_specifier2", "decl_specifier_seq", "decl_specifier",
  "storage_class_specifier", "function_specifier", "cv_qualifier",
  "cv_qualifier_seq", "store_type", "store_type_specifier", "$@52", "$@53",
  "type_specifier", "trailing_type_specifier", "$@54",
  "trailing_type_specifier_seq", "trailing_type_specifier_seq2", "$@55",
  "$@56", "tparam_type", "tparam_type_specifier2", "$@57", "$@58",
  "tparam_type_specifier", "simple_type_specifier", "type_name",
  "primitive_type", "ptr_operator_seq", "reference", "rvalue_reference",
  "pointer", "$@59", "ptr_cv_qualifier_seq", "pointer_seq",
  "decl_attribute_specifier_seq", "$@60", "id_attribute_specifier_seq",
  "$@61", "ref_attribute_specifier_seq", "$@62",
  "func_attribute_specifier_seq", "$@63", "array_attribute_specifier_seq",
  "$@64", "class_attribute_specifier_seq", "$@65",
  "attribute_specifier_seq", "attribute_specifier", "attribute_list",
  "attribute", "$@66", "attribute_pack", "attribute_sig",
  "attribute_token", "declaration_macro", "$@67", "$@68", "$@69", "$@70",
  "$@71", "$@72", "$@73", "$@74", "$@75", "$@76", "$@77", "$@78", "$@79",
  "$@80", "$@81", "$@82", "$@83", "$@84", "$@85", "$@86", "$@87", "$@88",
  "opt_comma", "operator_id", "operator_id_no_delim", "keyword", "literal",
  "constant_expression", "constant_expression_item", "$@89",
  "common_bracket_item", "common_bracket_item_no_scope_operator",
  "any_bracket_contents", "bracket_pitem", "any_bracket_item",
  "braces_item", "angle_bracket_contents", "braces_contents",
  "angle_bracket_pitem", "angle_bracket_item", "angle_brackets_sig",
  "$@90", "right_angle_bracket", "brackets_sig", "$@91", "$@92",
  "parentheses_sig", "$@93", "$@94", "$@95", "braces_sig", "$@96",
  "ignored_items", "ignored_expression", "ignored_item",
  "ignored_item_no_semi", "ignored_item_no_angle", "ignored_braces",
  "ignored_brackets", "ignored_parentheses", "ignored_left_parenthesis", YY_NULLPTR
};
#endif

#define YYPACT_NINF -1011
#define YYTABLE_NINF -685

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const short int yypact[] =
{
   -1011,    99,   124, -1011, -1011,  6666, -1011,   144,   154,   174,
     205,   225,   259,   282,   194,   286,   289, -1011, -1011, -1011,
     236, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
      72, -1011,  4209, -1011, -1011,  8256,    74,  7254, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011,   291,   312,   319,   344,   346,   360,
     373,   374,   386,   391,   393,    -7,     8,    31,    58,   109,
     193,   217,   224,   237,   241,   249,   260,   267,   301,   309,
     310,   326,   333,   345,   357,   359, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011,   170, -1011, -1011, -1011, -1011, -1011, -1011,
    7926, -1011,    50,    50,    50,    50, -1011,   364,  8256, -1011,
     242, -1011,   398,  7275,  8611,   399,  7531,   130,   190, -1011,
     410,  8036, -1011, -1011, -1011, -1011,   135,   126, -1011, -1011,
   -1011, -1011, -1011, -1011,   518, -1011, -1011,   411,  4674, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011,    10, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,    91,  7531,
     -26,    19,    25,    45,    71,   139,   158,   543, -1011, -1011,
   -1011, -1011, -1011,  7578,   399,   399,  8256,   135, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011,   424,   -26,    19,    25,
      45,    71,   139,   158, -1011, -1011, -1011,  7531,  7531, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
     434,   436, -1011,  7275,  7531,   399,   399,  6489, -1011, -1011,
   -1011,  6489,  6489, -1011,  6489,  6489,  6489,  6489,  6489,  6489,
    6489,  6489,  6489,  6489,  6489,  6489,  6489,  6489,  2135,   437,
    7724, -1011,  2135, -1011,  7304,   432,  7531, -1011, -1011, -1011,
   -1011, -1011, -1011,  7926, -1011, -1011,  8146,   135,   442, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,  8256, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011,   444,   399,   399,   399,
   -1011, -1011, -1011, -1011,   410, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,  7275, -1011,
   -1011, -1011, -1011, -1011, -1011,  7602, -1011,   405,    79, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011,   133, -1011, -1011,
   -1011,    36, -1011, -1011, -1011,  1829,  2054, -1011, -1011,   330,
   -1011,  2349,  3434,  2504, -1011, -1011, -1011, -1011, -1011, -1011,
    6892,  7834,  6892,  7419, -1011, -1011, -1011, -1011, -1011, -1011,
    7649, -1011,  2659,   877,   447, -1011,   452, -1011,   458, -1011,
   -1011, -1011,  6379,  7275, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011,   450,  6489,  6489,  6489,   457,   462,  6489,   465,   470,
     471,   472,   473,   474,   475,   482,   488,   489,   490,   491,
     493,   496, -1011, -1011,   502, -1011,   135, -1011, -1011, -1011,
   -1011, -1011, -1011,    55, -1011,  8498,   896,   399,   399,   518,
     503,  6489, -1011, -1011, -1011,   277, -1011,  7531,  8146,  7602,
    7531,   507,  2814,   497,  8590,   812,   442, -1011, -1011, -1011,
   -1011, -1011,  2135,  7834,  2135,  7419, -1011, -1011, -1011, -1011,
     337, -1011, -1011, -1011,  1550, -1011, -1011,   500, -1011,  7275,
     137, -1011, -1011, -1011,   509,   505,  7649, -1011,   508,   135,
   -1011, -1011, -1011, -1011, -1011, -1011,  1692,   159, -1011,   812,
     515,  3589, -1011, -1011,   510, -1011, -1011, -1011, -1011,   118,
   -1011,  8366,   121,   610, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011,   521, -1011,   135,    62,   526,   -25,  6892,  6892,
     187,   248, -1011, -1011, -1011, -1011,   527,   399, -1011, -1011,
   -1011,   518,   650,   519,   520,    24, -1011, -1011,   524, -1011,
     522, -1011, -1011, -1011, -1011, -1011, -1011,   529,   528,   537,
   -1011, -1011,   531,  8256, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011,  7531, -1011,   538, -1011,
   -1011,    76,  2236, -1011, -1011,   541, -1011, -1011,   399,   399,
    8498, -1011,   210, -1011,   545,  8256,   546, -1011, -1011,  7275,
     548, -1011,   116, -1011, -1011,   556,   606, -1011,   399, -1011,
     497,  5604,   567,    70,   569,   277,  1692, -1011,   337, -1011,
   -1011,   564,   122, -1011, -1011,   562,   153, -1011, -1011, -1011,
   -1011, -1011,  5914, -1011, -1011,  1246, -1011, -1011,   518,   337,
     198,   565,   399, -1011,   623,    91, -1011, -1011, -1011, -1011,
   -1011,  7531,  7531,  7531, -1011,   399,   399,  8256,   135,   126,
   -1011, -1011, -1011, -1011,   135,   121,  4829,  4984,  5139, -1011,
     571, -1011, -1011, -1011,   574,   577, -1011,   126, -1011,   576,
   -1011,   579,  8256, -1011,   572,   573, -1011, -1011, -1011, -1011,
    8256, -1011, -1011, -1011,  8256,  8256, -1011,   578,  8256,  8256,
    8256,  8256,  8256,  8256,  8256,  8256,  8256,  8256,   589, -1011,
   -1011, -1011, -1011, -1011,   591, -1011, -1011, -1011,   549,   592,
   -1011,   694,   651, -1011, -1011, -1011, -1011,  6489, -1011, -1011,
   -1011,   203,  7531,   651,  2969, -1011, -1011,   594, -1011, -1011,
   -1011, -1011,   597, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,   601, -1011,
     122, -1011, -1011, -1011, -1011, -1011, -1011,   214, -1011,    91,
   -1011, -1011, -1011, -1011,   562, -1011,   536, -1011,   600,   126,
   -1011,  5759, -1011,  5914, -1011, -1011, -1011, -1011,   293, -1011,
     287, -1011, -1011, -1011,   812, -1011, -1011, -1011,   330, -1011,
   -1011, -1011, -1011,  7649, -1011, -1011, -1011, -1011, -1011,   135,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011,   497, -1011,   135, -1011, -1011,   602,
    8256, -1011,   608,  8256, -1011,   611,   617,   621,   624,   625,
     627,   630,   631,   603,   612, -1011,   632,  6533, -1011,  7531,
   -1011, -1011, -1011,  7531, -1011,  2236, -1011, -1011,  5914, -1011,
     607, -1011, -1011, -1011, -1011, -1011, -1011,   664,  8256,   122,
   -1011, -1011, -1011, -1011, -1011, -1011,   497,    91,  8476, -1011,
   -1011, -1011, -1011,   620, -1011, -1011, -1011, -1011, -1011, -1011,
     497, -1011,  6224, -1011,  4364, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011,   325, -1011,   613,    79,  1692,   613, -1011,
     633,   642, -1011, -1011,   638, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011,   766,   767, -1011,  6914,    96,
    7466,   116, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011,  6799, -1011,    50, -1011, -1011, -1011,
     646,   444,  7275,  7029,   135, -1011,   651,  2236,   651,   592,
    5914,  4519, -1011,   735, -1011,   518, -1011, -1011,   135, -1011,
   -1011,  5294,  5604,  5449,   644, -1011,  8476, -1011, -1011,   126,
     645,  5914,   649, -1011, -1011, -1011, -1011, -1011,  5914,   497,
    1692, -1011, -1011, -1011, -1011, -1011,   654,   135, -1011,   613,
   -1011, -1011,   655, -1011,   658,   659,   661, -1011, -1011, -1011,
    1484,    50,   444,  7144,   651, -1011, -1011, -1011, -1011,  6799,
   -1011, -1011,  7144, -1011, -1011, -1011, -1011,  7275,  7602, -1011,
   -1011, -1011,   116,   122, -1011, -1011,   660, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011,  5914, -1011,  5914,   668,  6069, -1011,
   -1011, -1011, -1011, -1011,  3744, -1011, -1011, -1011,  7713, -1011,
   -1011,  1484, -1011, -1011,  7602,  7144,   671, -1011, -1011,   675,
   -1011,   405, -1011, -1011, -1011,  1692, -1011, -1011, -1011, -1011,
    6069, -1011,   416,   404, -1011, -1011, -1011,   110, -1011,  7713,
   -1011,  7762, -1011,   677, -1011, -1011,  1692,   518, -1011, -1011,
   -1011, -1011, -1011,   116,   127,  3124,  3899,   354,    61,  7762,
     112, -1011, -1011,  3279, -1011, -1011, -1011, -1011, -1011, -1011,
      83,   354, -1011,   404,  4054, -1011, -1011
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const unsigned short int yydefact[] =
{
       3,     0,     4,     1,   488,     0,   500,   444,   445,   446,
     440,   441,   442,   443,   448,   449,   447,    53,    52,    54,
     115,   403,   404,   395,   398,   399,   401,   402,   400,   394,
     396,   218,     0,   353,   417,     0,     0,     0,   350,   461,
     462,   463,   464,   465,   470,   471,   472,   473,   466,   467,
     468,   469,   474,   475,   460,   450,   451,   452,   453,   454,
     455,   456,   457,   458,   459,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    23,   348,     5,    19,
      20,    13,    11,    12,     9,    37,    17,   383,    44,   498,
      10,    16,   383,     0,   498,    14,   136,     7,     6,     8,
       0,    18,     0,     0,     0,     0,   206,     0,     0,    15,
       0,   330,   488,     0,     0,     0,     0,   488,   416,   332,
     349,     0,   488,   391,   392,   393,   178,   285,   408,   412,
     415,   488,   488,    21,   489,   117,   116,   397,     0,   444,
     445,   446,   440,   441,   442,   443,   730,   731,   641,   636,
     637,   638,   635,   639,   640,   642,   643,   448,   449,   447,
     700,   609,   608,   610,   628,   612,   614,   613,   615,   616,
     617,   620,   621,   619,   618,   624,   627,   611,   629,   630,
     622,   607,   606,   626,   625,   582,   583,   623,   633,   632,
     631,   634,   584,   585,   586,   714,   587,   588,   589,   595,
     596,   590,   591,   592,   593,   594,   597,   598,   599,   600,
     601,   602,   603,   604,   605,   712,   711,   724,   460,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   700,
     718,   715,   719,   729,   164,   700,   578,   579,   573,   717,
     572,   574,   575,   576,   577,   580,   581,   716,   723,   722,
     713,   720,   721,   702,   708,   710,   709,   700,     0,     0,
     444,   445,   446,   440,   441,   442,   443,   396,   383,   498,
     383,   498,   488,     0,   488,   416,     0,   178,   376,   378,
     377,   381,   382,   380,   379,   700,    34,   357,   355,   356,
     360,   361,   359,   358,   364,   363,   362,     0,     0,   375,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
       0,   490,   331,     0,     0,   333,   334,     0,   517,   521,
     523,     0,     0,   530,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   285,     0,
      51,   500,   285,   111,   118,     0,     0,    27,    38,    24,
     498,    26,    28,     0,    25,    29,     0,   178,   250,   239,
     700,   188,   238,   190,   191,   189,   209,   498,     0,   212,
      22,   420,   346,   196,   194,   245,   337,     0,   333,   334,
     335,    59,   336,    58,     0,   340,   338,   339,   341,   419,
     342,   351,   383,   498,   383,   498,   137,   207,     0,   488,
     410,   389,   293,   295,   179,     0,   281,   266,   178,   492,
     492,   492,   407,   286,   476,   477,   486,   478,   383,   439,
     438,   507,   501,     3,   702,     0,     0,   687,   686,   169,
     163,     0,     0,     0,   694,   696,   692,   354,   488,   397,
     285,    51,   285,   118,   337,   383,   383,   152,   148,   144,
       0,   147,     0,     0,     0,   155,     0,   153,     0,   500,
     157,   156,     0,     0,   357,   355,   356,   360,   361,   359,
     358,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   388,   387,     0,   281,   178,   488,   385,   386,
      62,    40,    49,   413,   488,     0,     0,    59,     0,   499,
       0,   123,   107,   119,   114,   488,   490,     0,     0,     0,
       0,     0,     0,   256,     0,     0,   250,   248,   343,   344,
     345,   675,   285,    51,   285,   118,   197,   195,   390,   383,
     484,   208,   213,   490,     0,   192,   220,   319,   490,     0,
       0,   268,   278,   267,     0,     0,     0,   309,     0,   178,
     481,   500,   480,   482,   479,   487,   409,     0,   504,     0,
       4,     0,   705,   707,     0,   701,   704,   706,   725,     0,
     166,     0,     0,     0,   726,    31,   703,   728,   664,   664,
     664,   418,     0,   144,   178,   413,     0,   488,   285,   285,
       0,   319,   490,   333,   334,    33,     0,     0,     3,   160,
     161,   491,     0,   582,   583,     0,   567,   566,     0,   564,
       0,   565,   217,   571,   159,   158,   515,     0,     0,     0,
     525,   528,     0,     0,   535,   539,   543,   547,   537,   541,
     545,   549,   551,   553,   555,   556,     0,    42,   280,   284,
     384,    63,     0,    61,    39,    48,    57,   488,    59,     0,
       0,   109,     0,   121,   124,     0,   113,   414,   488,     0,
     251,   252,     0,   700,   237,     0,   488,   413,     0,   246,
     256,     0,     0,   413,     0,   488,   411,   405,   485,   294,
     220,     0,   233,   290,   320,     0,   313,   198,   193,   490,
     277,   282,     0,   698,   271,     0,   291,   310,   493,   484,
     507,     0,   513,   349,   509,   511,    32,    30,   727,   167,
     165,     0,     0,     0,   434,   433,   432,     0,   178,   285,
     427,   431,   180,   181,   178,     0,     0,     0,     0,   139,
     143,   146,   141,   113,     0,     0,   138,   285,   149,   313,
      36,     4,     0,   570,     0,     0,   569,   568,   560,   561,
       0,   518,   522,   524,     0,     0,   531,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   558,    66,
      67,    68,    45,   488,     0,   103,   104,   105,   101,    50,
      94,    99,   178,    46,    55,   488,   112,   123,   125,   120,
     106,   332,     0,   178,     0,   494,   262,   257,   258,   263,
     347,   220,     0,   690,   653,   682,   658,   683,   684,   688,
     659,   663,   662,   657,   660,   661,   680,   652,   681,   676,
     679,   352,   654,   655,   656,    43,    41,   110,   113,   406,
     233,   215,   226,   224,   222,   230,   235,     0,   221,   228,
     229,   219,   234,   325,   322,   323,     0,   243,     0,   285,
     651,   648,   649,   269,   644,   646,   647,   677,     0,   274,
     296,   483,   506,   502,     0,   510,   508,   512,   169,   435,
     436,   437,   429,   311,   170,   492,   426,   383,   173,   178,
     669,   671,   672,   695,   667,   668,   666,   670,   665,   697,
     693,   140,   142,   145,   256,    35,   178,   562,   563,     0,
       0,   526,     0,     0,   534,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   559,     0,     0,    65,     0,
     102,   488,   100,     0,    96,     0,    56,   122,     0,   702,
       0,   129,   253,   254,   241,   210,   500,   259,     0,   233,
     494,   664,   675,   664,   108,   214,   256,     0,     0,   225,
     231,   232,   227,   328,   324,   316,   317,   318,   315,   314,
     256,   283,     0,   645,     0,   275,   273,   297,   292,   300,
     514,   168,   383,   296,   312,   182,   178,   428,   182,   176,
       0,     0,   516,   519,     0,   529,   532,   536,   540,   544,
     548,   538,   542,   546,   550,     0,     0,   557,     0,   396,
       0,     0,    84,    80,    71,    77,    64,    79,    73,    72,
      76,    74,    69,    70,     0,    78,     0,   203,   204,    75,
       0,   330,     0,     0,   178,    81,   178,     0,   178,    47,
     126,   128,   127,   240,   220,   495,   261,   488,   178,   247,
     249,     0,     0,     0,     0,   223,     0,   422,   236,   285,
       0,     0,     0,   650,   674,   699,   673,   678,     0,   256,
     430,   287,   184,   171,   183,   307,     0,   178,   174,   182,
     150,   162,     0,   702,     0,     0,     0,    92,   498,    90,
       0,     0,     0,     0,   178,    82,    85,    87,    88,     0,
      86,    89,     0,   199,    83,   490,   205,     0,     0,    97,
      95,    98,     0,   233,   260,   264,     0,   685,   689,   494,
     424,   383,   421,   496,   329,   494,   276,     0,     0,   288,
     308,   177,   301,   520,     0,   533,   552,   554,     0,   498,
      91,     0,    93,   498,     0,     0,     0,   490,   202,     0,
     211,   266,   691,   216,   383,   423,   326,   500,   244,   494,
     185,   186,   296,   151,   527,   700,   702,   413,   132,     0,
     498,     0,   200,     0,   700,   265,   425,   497,   298,   187,
     289,   303,   302,     0,   306,     0,     0,     0,    60,     0,
     413,   133,   201,     0,   301,   304,   305,   702,   135,   130,
      60,     0,   242,   299,     0,   131,   134
};

  /* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
   -1011, -1011,  -388, -1011, -1011,   808,  -114, -1011, -1011, -1011,
   -1011,  -834,  -104,     2,   -34, -1011, -1011, -1011, -1011,    56,
    -411,  -100,  -914, -1011, -1011, -1011, -1011,  -109, -1011,  -116,
    -217, -1011, -1011,    37,  -101,   -99,   -33, -1011, -1011,    34,
    -474, -1011, -1011,    26, -1011, -1011, -1011,  -261,  -824,   -97,
    -123,  -397,   239,    86, -1011, -1011, -1011, -1011,   240,   -92,
   -1011, -1011,     6, -1011,    -1, -1011, -1011, -1011,   -42, -1011,
   -1011, -1011, -1011, -1011, -1011,  -113,   102,  -911, -1011, -1011,
   -1011,   843, -1011, -1011, -1011,   -87,  -164,    32,   -11, -1011,
   -1011,  -266,  -484, -1011, -1011, -1011,  -309,  -293,  -550,  -673,
   -1011, -1011, -1011, -1011,  -815, -1011, -1011,   -95, -1011, -1011,
   -1011, -1011,  -120, -1011, -1011, -1011, -1011,   320, -1011,    43,
    -659, -1011, -1011, -1011,  -200, -1011, -1011,  -303, -1011, -1011,
   -1011, -1011, -1011, -1011,     7,   358,  -300,   361, -1011,    -4,
     -43,  -696, -1011,  -241, -1011,  -682, -1011,  -944, -1011, -1011,
    -337, -1011, -1011, -1011,  -450, -1011, -1011,  -281, -1011, -1011,
       5, -1011, -1011, -1011,  1018,   349,  1009,    48, -1011, -1011,
     311,   334,    -5, -1011,    29, -1011,  1152,   -31,   343, -1011,
      33,   609, -1011, -1011,  -496, -1011,   799,   196, -1011, -1011,
     -55,  -897, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011,   281,   206,   251,  -403,   446, -1011,   448, -1011,
     155, -1011,     9, -1011,  -517, -1011,  -392, -1011,  -913, -1011,
   -1011, -1011,   -44, -1011,  -330, -1011, -1011,   156, -1011, -1011,
   -1011, -1011,   -58, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011, -1011,
   -1011, -1011, -1011, -1011, -1011, -1011, -1011,    35,   165,    77,
    -878,  -810, -1011,  -307,   -93,  -557, -1011,   -91, -1011,   -75,
   -1011, -1010, -1011,  -674, -1011,  -556, -1011, -1011, -1011,  -242,
   -1011, -1011, -1011,   332, -1011,  -195,  -418, -1011,  -420,    60,
      87, -1011,  -657, -1011
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const short int yydefgoto[] =
{
      -1,     1,     2,     4,    88,   357,    90,    91,    92,   464,
      93,    94,    95,   359,    97,   349,    98,   945,   662,   377,
     513,   514,   665,   661,   793,   794,  1026,  1107,  1028,   799,
     800,   943,   939,   801,   100,   101,   102,   521,   103,   360,
     524,   675,   672,   673,   948,   361,   950,  1099,  1178,   105,
     106,   602,   610,   603,   457,   458,   914,  1142,   459,   107,
     320,   108,   362,   762,   363,   439,   589,   888,   590,   591,
     995,   592,   998,   593,  1089,   893,   744,  1083,  1084,  1138,
    1170,   364,   112,   113,   114,  1110,  1036,  1037,   116,   533,
    1054,   117,   551,   700,   552,   966,   553,   118,   555,   702,
     858,   967,   859,   860,   861,   862,   968,   371,   372,  1053,
     556,   980,  1038,   536,   821,   385,   690,   531,   680,   681,
     685,   686,   817,  1057,   818,   958,  1161,   562,   563,   712,
     564,   565,   878,  1078,   365,   416,   504,   560,   869,   505,
     506,   894,  1172,   417,   880,   418,   550,   988,  1079,  1204,
    1173,  1086,   568,   993,   557,   979,   703,   989,   705,   864,
     865,   973,  1070,  1071,   802,   121,   282,   283,   535,   124,
     125,   126,   284,   541,   285,   268,   129,   130,   348,   507,
     378,   132,   133,   134,   135,   698,  1044,   137,   428,   549,
     138,   139,   269,  1068,  1069,  1131,  1164,   738,   739,   897,
     992,   740,   140,   141,   142,   423,   424,   425,   426,   719,
     699,   427,   677,     6,   468,   469,   570,   571,   955,   956,
    1166,  1167,   350,   351,   144,   432,   577,   578,   579,   886,
     724,   725,   143,   770,   482,   920,  1092,   483,   484,   774,
    1004,   775,   487,   923,  1094,   778,   782,   779,   783,   780,
     784,   781,   785,   786,   787,   936,   632,   833,   834,   835,
     873,   874,   982,   875,   837,   746,   907,   908,  1077,   691,
     984,   838,   839,   876,   962,   440,   842,   963,   961,   843,
     600,   598,   599,   844,   877,   435,   442,   585,   586,   587,
     264,   265,   266,   267
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const short int yytable[] =
{
     127,   278,   280,   384,   110,   296,   358,    96,   406,   708,
     611,   109,   119,     5,   471,   569,   581,   840,   373,   374,
     375,   519,   596,   415,   390,   813,   447,   850,   572,   573,
     472,   822,   325,   730,   128,   965,   701,   115,   131,   104,
     605,   706,   747,   748,   436,   580,   461,  1060,   898,  1081,
     441,   676,   520,   123,   697,   606,   895,   470,   663,  -376,
     354,    99,   895,   983,   663,   663,   326,   260,   286,   281,
    1050,  1067,   443,   663,   437,   707,   410,   288,   289,   290,
     291,   292,   293,   294,   147,   323,   663,  1088,   766,   412,
     413,   279,   263,   394,   422,   759,   368,   402,   404,     3,
     462,   444,   445,  1024,  -378,   789,   790,   791,   449,   262,
    -377,  -118,   383,   663,   523,   663,   148,  -376,   388,   391,
     407,   325,   554,   687,    -2,   852,   156,   157,  1171,   708,
    -381,   381,   693,   753,   327,   841,   399,   156,   157,   621,
     148,   411,   115,   366,  1059,   412,   413,   742,   959,   328,
     429,   430,   389,   392,   853,   326,  -382,   419,   123,   438,
    1189,   596,  -378,   414,   635,   405,    99,   854,  -377,  1130,
     855,   743,   329,   767,   460,   532,  -503,   567,  1141,   408,
     472,  -503,   437,   260,  1097,   369,   370,   403,  -381,   856,
     664,   -60,   868,  1134,   -60,   707,   664,   261,   -60,   330,
    1136,   -60,   849,   999,   419,   846,   -60,   634,   434,   -60,
     295,   754,   755,   792,  -382,    38,  1163,   895,   846,   414,
    1001,   848,  1168,   697,  -380,   262,   420,   421,  1190,  -376,
     761,   410,   446,   969,   895,   451,   554,   453,   258,  -378,
     983,   718,   692,  -379,   694,   664,   -60,   846,   -60,   -60,
     331,   -60,   455,   456,   529,  1000,  1188,   243,   729,  -377,
     857,   145,   146,  1197,   325,  -333,   554,   438,   243,   970,
     971,  -333,   710,   420,   421,    38,   704,   711,   388,  -357,
     420,   421,  -380,   259,  1197,  -357,  1211,  -376,   547,  -355,
    -381,   381,   866,   399,   867,  -355,  1211,  -378,   326,   720,
    1085,  -379,   325,   325,   721,   566,   353,  1064,  1160,  -356,
    -382,   410,   389,   261,   895,  -356,   527,  -377,   388,   325,
     281,  1072,   756,   410,   983,  -334,   983,   757,   717,  -364,
     704,  -334,   402,   534,   332,  -364,   326,   326,  -505,   239,
    -360,   949,   279,  -505,  -380,   517,  -360,   806,  -381,   325,
     807,   325,   389,   326,   258,   407,  -172,  1201,   333,   543,
    -361,   545,  1125,   461,  1105,   334,  -361,  -379,  -382,    21,
      22,   324,   473,  1209,  -175,  1201,   895,   380,   335,   518,
    -172,  1123,   336,   326,   836,   326,   554,  1215,   840,   758,
     337,  1140,  -321,   566,  -359,   115,   528,   546,   515,   259,
    -359,   338,  -380,   388,  1061,   895,  1063,   604,   339,   604,
     325,   123,  -118,  -118,   408,   667,   523,  -358,   548,   279,
    1137,  -363,   403,  -358,  -362,  -379,  -375,  -363,   987,   394,
    -362,  -321,  -375,   985,   387,   986,  1191,   389,  1192,   906,
     906,   906,   340,   994,   326,   352,   517,  -365,   325,  1193,
     341,   342,  1194,  -365,  -366,   613,   566,   601,   617,   395,
    -366,   398,   400,   559,   840,  1159,   987,   343,   388,  -321,
     260,   260,   386,   410,   344,   396,   260,   260,   260,  -367,
     518,  -368,   326,   887,    38,  -367,   345,  -368,   814,   614,
    1175,   460,   518,  1176,   402,  -369,   840,   260,   346,   515,
     347,  -369,   389,   996,   394,   376,  1127,   633,  -370,  -371,
     668,   617,   262,   262,  -370,  -371,   660,   407,   262,   262,
     262,  -372,   325,   666,   325,   325,  -373,  -372,  -374,   517,
     688,  1051,  -373,  -331,  -374,   394,  1205,  1206,   517,   262,
     325,  -270,   431,   561,   669,   518,  -272,   433,   723,   975,
     976,   977,   978,   401,   388,   449,   326,   987,   326,   326,
    -321,   613,   463,   518,   670,   604,   604,   260,  1158,   467,
     805,  -154,   518,   510,   326,   244,   408,   679,   795,   796,
     797,   530,   515,   618,   279,   537,   735,   619,   389,   633,
     636,   515,   745,  1087,   387,   614,   620,   640,  1156,   509,
     261,   261,   641,   509,  1158,   643,   261,   261,   261,   262,
     644,   645,   646,   647,   648,   649,   260,   972,   398,   400,
     736,   450,   650,   452,   737,   733,  1055,   261,   651,   652,
     653,   596,   454,   654,   387,   655,   656,   657,   671,  -255,
    1183,   258,   258,   683,  -321,   713,   715,   258,   258,   258,
     716,   325,   726,   763,   355,   728,   749,   325,   262,   398,
     400,   752,   760,   764,   765,   668,   768,   769,   258,   771,
     772,   776,   454,   396,   388,  1144,   804,   906,   711,   773,
     803,   810,   892,   808,   516,   326,   259,   259,   812,   944,
     816,   326,   259,   259,   259,   819,   896,   261,   815,   669,
     953,   509,   845,   509,   847,   851,   863,   885,   389,   911,
     883,   757,   912,   259,   604,   866,   915,   917,   918,   387,
     924,   538,   539,   540,   596,  1065,   325,   325,   325,   935,
     938,   942,   941,  1115,   957,   414,   964,  1177,   258,   960,
     409,   867,  1052,  1015,  1002,   542,   261,   544,  1056,   554,
    1005,  1082,  1016,  1007,   906,   836,   906,   386,  1196,  1008,
     326,   326,   326,  1009,   558,  -327,  1010,  1011,  1198,  1012,
    1200,   576,  1013,  1014,  1017,  1090,   596,  1091,  1093,  1095,
    1096,  1114,  1122,   259,   387,   516,  1129,   258,  1210,  1214,
    1133,  1135,   879,   509,   596,   509,  1139,  1143,   608,   609,
    1145,  1146,   937,  1147,   136,  1162,  1182,   325,  1157,   612,
    1169,  1184,  1202,    89,   946,   288,   289,   290,   291,   292,
     293,   294,   454,  1023,   554,   554,   387,   509,  1027,  1049,
    1120,   836,   259,   947,   287,   940,  1029,  1187,  1030,  1150,
    1031,   326,   750,   913,   751,  1032,   991,   899,   111,   260,
    1035,   398,   400,   723,  1101,   952,   689,  1124,  1185,   509,
     509,   554,   394,   836,   659,   981,   658,  1213,   516,   974,
     387,   809,   741,   574,   881,   575,   882,   516,   558,  1045,
     288,   289,   290,   291,   292,   293,   294,  1062,   613,  1073,
     722,   262,   696,  1076,     0,   409,   714,   831,   951,   288,
     289,   290,   291,   292,   293,   294,     0,     0,   709,   367,
       0,     0,     0,     0,     0,   612,     0,   379,   831,     0,
     406,     0,   614,  1116,     0,     0,    33,     0,     0,     0,
     358,  1118,   127,  1119,   325,  1121,  1034,     0,   325,  1025,
     325,  1113,   832,  1033,  1039,    33,     0,   509,     0,     0,
    1047,   398,   831,   831,   831,     0,     0,   508,     0,     0,
       0,   508,    38,   832,     0,     0,   128,     0,   326,     0,
    1043,   104,   326,     0,   326,   409,     0,     0,     0,   261,
    1195,   387,     0,  1116,     0,  1042,     0,   409,   410,  1203,
     387,  1154,  1116,    99,     0,     0,   358,   832,   832,   832,
       0,  1066,   539,   540,     0,     0,  1152,     0,     0,   402,
     404,     0,     0,   127,   122,   325,     0,  1100,     0,     0,
     258,     0,   820,   120,     0,     0,  1132,     0,   709,   127,
       0,     0,   407,  1109,     0,  1116,  1106,   388,   127,     0,
    1108,  1111,   325,     0,     0,     0,   322,   128,   410,   326,
       0,  1103,     0,     0,  1148,   321,   884,   410,     0,   508,
       0,   508,     0,   128,     0,   259,   819,  1112,   323,   398,
     400,   389,   128,     0,  1098,     0,   326,   405,   402,   831,
       0,     0,  1042,     0,     0,     0,   260,   394,     0,     0,
      99,  1117,   407,     0,     0,     0,     0,     0,   127,   403,
     410,   407,     0,     0,   127,  1179,     0,   127,  1109,  1181,
       0,  1106,   388,   613,     0,  1108,  1111,     0,     0,     0,
       0,   402,     0,   394,   832,     0,     0,     0,   262,     0,
       0,     0,   128,  1151,     0,   397,  1199,   409,   128,     0,
       0,   128,  1155,   517,   407,     0,   389,   614,     0,   613,
     127,   508,     0,   508,   831,     0,  1149,  1042,     0,  1153,
    1117,     0,   367,     0,     0,   279,     0,     0,   403,     0,
       0,     0,     0,     0,   517,     0,   517,   518,     0,   260,
       0,     0,     0,   614,   128,   508,     0,     0,   831,     0,
     831,     0,     0,     0,   517,   990,   515,     0,     0,   832,
       0,     0,     0,  1117,     0,     0,     0,  1180,   518,     0,
     518,   279,     0,     0,     0,     0,   261,   508,   508,     0,
       0,   262,     0,     0,     0,     0,     0,   515,   518,   515,
     260,   260,     0,   832,     0,   832,     0,     0,   260,     0,
     997,     0,   612,     0,     0,     0,     0,   515,   509,   260,
       0,     0,     0,     0,     0,     0,   831,   258,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   831,   831,   831,
       0,   379,   262,   262,     0,     0,   393,   831,   322,     0,
     262,     0,     0,     0,   831,     0,  1041,   448,     0,     0,
       0,   262,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   832,   259,     0,     0,   508,     0,     0,     0,   261,
       0,     0,   832,   832,   832,     0,   322,   322,     0,     0,
       0,     0,   832,     0,     0,   465,   466,     0,     0,   832,
       0,   509,     0,   397,     0,  1080,     0,     0,     0,     0,
     831,     0,   831,     0,   831,     0,   409,     0,     0,     0,
     258,     0,     0,   387,   324,     0,     0,     0,     0,     0,
     261,   261,     0,   322,     0,   322,     0,  1102,   261,     0,
       0,     0,   525,     0,   526,     0,   831,     0,     0,   261,
       0,     0,     0,  1102,     0,   832,     0,   832,     0,   832,
       0,   454,  1102,     0,     0,   259,     0,     0,     0,     0,
       0,   258,   258,     0,     0,     0,     0,     0,     0,   258,
       0,     0,     0,     0,     0,     0,   509,     0,     0,     0,
     258,   832,     0,     0,     0,     0,     0,     0,   387,     0,
       0,     0,     0,     0,     0,     0,     0,   509,     0,     0,
       0,     0,   777,     0,     0,     0,   259,   259,     0,     0,
       0,     0,  1102,     0,   259,     0,     0,     0,  1102,     0,
       0,  1102,   322,     0,     0,   259,   454,   558,     0,     0,
       0,   607,   616,     0,  1165,     0,     0,     0,     0,   481,
       0,     0,   516,   485,   486,     0,   488,   489,   490,   491,
     492,   493,   494,   495,   496,   497,   498,   499,   500,   501,
       0,     0,   393,   709,  1102,     0,     0,  1186,     0,    17,
      18,    19,     0,   516,     0,   516,    21,    22,    23,    24,
      25,    26,    27,    28,    29,     0,   277,     0,     0,     0,
       0,     0,     0,   516,     0,     0,   322,     0,     0,   322,
       0,     0,     0,     0,     0,   678,     0,     0,   682,     0,
       0,     0,     0,     0,   322,     0,     0,     0,     0,     0,
       0,   916,   622,   695,     0,     0,     0,     0,     0,   919,
       0,     0,     0,   921,   922,     0,     0,   925,   926,   927,
     928,   929,   930,   931,   932,   933,   934,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     734,     0,     0,   393,   623,   624,   508,     0,     0,     0,
       0,   202,   203,   204,   625,   206,   207,   208,   209,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   637,   638,   639,     0,     0,   642,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   409,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   322,     0,   393,     0,     0,
       0,   322,     0,   674,   788,   409,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   393,     0,   626,   508,
     627,   628,     0,   629,   630,   393,   246,   247,   248,   631,
     250,   251,   252,   253,   254,   255,   256,     0,     0,     0,
       0,     0,   409,     0,     0,     0,     0,     0,     0,  1003,
       0,   409,  1006,     0,    21,    22,    23,    24,    25,    26,
      27,    28,    29,   502,   277,     0,     0,     0,     0,     0,
     322,   322,   322,   503,     0,     0,     0,     0,     0,   889,
     890,   891,     0,     0,     0,     0,     0,  1058,     0,     0,
     409,     0,     0,     0,   409,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   508,     0,     0,     0,     0,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,     0,   508,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1104,     0,     0,
       0,   322,   393,     0,     0,     0,     0,     0,     0,     0,
     682,   811,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,   122,     0,   322,     0,
       0,     0,   322,     0,   322,  1040,     0,  1046,     0,   674,
       0,  1048,     0,     0,   582,   239,     0,   240,   241,   242,
     243,     0,   583,   245,   584,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   322,
       0,     0,     0,     0,     0,     0,     0,     0,   321,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   322,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,    21,    22,    23,
      24,    25,    26,    27,    28,    29,   502,   277,     0,     0,
       0,     0,     0,     0,     0,     0,   503,     0,     0,   582,
     239,   588,   240,   241,   242,   243,     0,   583,   245,     0,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,     0,   419,     0,     0,     0,     0,     0,     0,
       0,     0,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,     0,     0,   297,
     298,   299,   300,   301,   302,   303,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   304,   305,   306,
       0,     0,     0,     0,     0,   795,   796,   797,     0,     0,
    -279,     0,     0,   798,     0,     0,     0,     0,     0,     0,
       0,     0,   420,   421,     0,    33,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     393,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    38,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   393,     0,   393,     0,     0,     0,     0,   309,   310,
     311,   312,   313,   314,   315,   316,   317,   318,   319,     0,
       0,   393,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,    87,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   582,   239,     0,   240,   241,   242,
     243,     0,   583,   245,   594,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   582,
     239,     0,   240,   241,   242,   243,   597,   583,   245,     0,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   582,   239,   615,   240,   241,   242,
     243,     0,   583,   245,     0,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   582,
     239,   684,   240,   241,   242,   243,     0,   583,   245,     0,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   582,   239,   954,   240,   241,   242,
     243,     0,   583,   245,     0,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   582,
     239,  1207,   240,   241,   242,   243,     0,   583,   245,     0,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   582,   239,  1212,   240,   241,   242,
     243,     0,   583,   245,     0,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   595,
     239,     0,   240,   241,   242,   243,     0,   583,   245,     0,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   727,   239,     0,   240,   241,   242,
     243,     0,   583,   245,     0,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     239,     0,   240,   241,   242,   243,  1174,   583,   245,     0,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1208,   239,     0,   240,   241,   242,
     243,     0,   583,   245,     0,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1216,
     239,     0,   240,   241,   242,   243,     0,   583,   245,     0,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   239,     0,   240,   241,   242,
     243,     0,   244,   245,     0,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   149,   150,   151,
     152,   153,   154,   155,   444,   445,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   823,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   900,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,     0,   824,
       0,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1074,
     713,  1075,   901,   826,   902,   446,     0,   904,   829,     0,
     246,   247,   248,   905,   250,   251,   252,   253,   254,   255,
     256,   830,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   239,     0,   240,   241,   242,
     243,     0,   583,   245,     0,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     239,     0,   240,   241,   242,   243,     0,     0,   245,     0,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   149,   150,   151,   152,   153,   154,   155,   444,
     445,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   823,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   900,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,     0,   824,     0,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   713,     0,   901,   826,   902,
     446,   903,   904,   829,     0,   246,   247,   248,   905,   250,
     251,   252,   253,   254,   255,   256,   830,   149,   150,   151,
     152,   153,   154,   155,   444,   445,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   823,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   900,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,     0,   824,
       0,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     713,     0,   901,   826,   902,   446,   909,   904,   829,     0,
     246,   247,   248,   905,   250,   251,   252,   253,   254,   255,
     256,   830,   149,   150,   151,   152,   153,   154,   155,   444,
     445,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   823,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   900,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,     0,   824,     0,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   713,     0,   901,   826,   902,
     446,   910,   904,   829,     0,   246,   247,   248,   905,   250,
     251,   252,   253,   254,   255,   256,   830,   149,   150,   151,
     152,   153,   154,   155,   444,   445,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   823,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   900,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,     0,   824,
       0,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     713,     0,   901,   826,   902,   446,     0,   904,   829,  1126,
     246,   247,   248,   905,   250,   251,   252,   253,   254,   255,
     256,   830,   149,   150,   151,   152,   153,   154,   155,   444,
     445,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   823,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   900,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,     0,   824,     0,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   713,     0,   901,   826,   902,
     446,     0,   904,   829,  1128,   246,   247,   248,   905,   250,
     251,   252,   253,   254,   255,   256,   830,   149,   150,   151,
     152,   153,   154,   155,   444,   445,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   823,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   437,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,     0,   824,
       0,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     713,     0,   825,   826,   827,   446,     0,   828,   829,     0,
     246,   247,   248,   438,   250,   251,   252,   253,   254,   255,
     256,   830,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,     0,     0,  -648,  -648,  -648,  -648,  -648,  -648,
       0,  -648,  -648,  -648,  -648,     0,     0,  -648,  -648,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,  -648,     0,  -648,     0,  -648,  -648,  -648,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  -648,     0,  -684,  -648,  -648,
    -648,     0,  -648,  -648,     0,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,   149,   150,   151,
     152,   153,   154,   155,   444,   445,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   823,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   870,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,     0,   824,
       0,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     713,     0,     0,   826,     0,   446,     0,   871,   829,     0,
     246,   247,   248,   872,   250,   251,   252,   253,   254,   255,
     256,   830,   149,   150,   151,   152,   153,   154,   155,   444,
     445,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   823,   171,   172,   173,   174,   175,   176,
     177,   178,     0,     0,   179,   180,   181,   182,   183,   184,
       0,   185,   186,   187,   188,     0,     0,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,     0,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,     0,   824,     0,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   713,     0,     0,   826,     0,
     446,     0,   828,   829,     0,   246,   247,   248,     0,   250,
     251,   252,   253,   254,   255,   256,   830,   149,   150,   151,
     152,   153,   154,   155,   444,   445,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   823,   171,
     172,   173,   174,   175,   176,   177,   178,     0,     0,   179,
     180,   181,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,     0,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,     0,     0,
       0,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     713,     0,     0,   826,     0,   446,     0,     0,   829,     0,
     246,   247,   248,     0,   250,   251,   252,   253,   254,   255,
     256,   830,   270,   271,   272,   273,   274,   275,   276,     0,
       0,   622,     0,     0,     0,     0,     0,     0,     0,     0,
     167,   168,   169,     0,    17,    18,    19,    20,     0,     0,
       0,    21,    22,    23,    24,    25,    26,    27,    28,    29,
       0,   277,     0,     0,     0,     0,     0,     0,    33,    34,
       0,     0,     0,   623,   624,     0,     0,     0,     0,     0,
     202,   203,   204,   625,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,     0,    38,     0,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,   474,   475,   476,   477,   478,   479,   480,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     304,   305,   306,     0,     0,     0,     0,   626,     0,   627,
     628,     0,   629,   630,     0,   246,   247,   248,   631,   250,
     251,   252,   253,   254,   255,   256,     7,     8,     9,    10,
      11,    12,    13,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    14,    15,    16,     0,    17,    18,
      19,    20,     0,     0,     0,    21,    22,    23,    24,    25,
      26,    27,    28,    29,  1018,  1019,    31,    32,     0,     0,
       0,     0,    33,    34,    35,     0,  1020,     0,     0,     0,
       0,   309,   310,   311,   312,   313,   314,   315,   316,   317,
     318,   319,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    38,     0,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,  1021,  1022,     7,
       8,     9,    10,    11,    12,    13,     0,     0,     0,    87,
       0,     0,     0,     0,     0,     0,     0,    14,    15,    16,
       0,    17,    18,    19,    20,     0,     0,     0,    21,    22,
      23,    24,    25,    26,    27,    28,    29,     0,    30,    31,
      32,     0,     0,     0,     0,    33,    34,    35,    36,    37,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    38,     0,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
       0,    86,     7,     8,     9,    10,    11,    12,    13,     0,
       0,     0,    87,     0,     0,     0,     0,     0,     0,     0,
      14,    15,    16,     0,    17,    18,    19,    20,     0,     0,
       0,    21,    22,    23,    24,    25,    26,    27,    28,    29,
       0,   277,    31,   355,     0,     0,     0,     0,    33,    34,
       0,     0,   356,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    38,     0,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,     0,     0,     0,     0,     0,     7,     8,     9,
      10,    11,    12,    13,    21,    22,    23,    24,    25,    26,
      27,    28,    29,   502,   277,    14,    15,    16,     0,    17,
      18,    19,    20,   503,     0,    87,    21,    22,    23,    24,
      25,    26,    27,    28,    29,     0,   277,    31,   355,     0,
       0,     0,     0,    33,    34,     0,     0,     0,     0,     0,
     419,     0,     0,     0,     0,     0,     0,     0,     0,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,     0,     0,     0,     0,     0,    38,
       0,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,     0,     0,     0,
       0,     0,     7,     8,     9,    10,    11,    12,    13,   420,
     421,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      14,    15,    16,     0,    17,    18,    19,    20,     0,     0,
      87,    21,    22,    23,    24,    25,    26,    27,    28,    29,
       0,   277,    31,     0,     0,     0,     0,     0,    33,    34,
      35,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    38,     0,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,     0,     0,     0,     0,     0,     7,     8,     9,
      10,    11,    12,    13,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    14,    15,    16,     0,    17,
      18,    19,    20,     0,     0,    87,    21,    22,    23,    24,
      25,    26,    27,    28,    29,     0,   277,    31,     0,     0,
       0,     0,     0,    33,    34,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    38,
       0,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,   297,   298,   299,
     300,   301,   302,   303,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   304,   305,   306,   297,   298,
     299,   300,   301,   302,   303,     0,     0,     0,     0,     0,
      87,     0,     0,     0,     0,     0,   304,   305,   306,     0,
       0,     0,     0,    33,   307,     0,   308,   297,   298,   299,
     300,   301,   302,   303,     0,     0,     0,     0,    31,   382,
       0,     0,     0,     0,    33,   304,   305,   306,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    38,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    33,     0,     0,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,   319,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,   319,     0,    38,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      87,     0,     0,     0,     0,     0,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,   319,     0,     0,     0,
       0,    87,   297,   298,   299,   300,   301,   302,   303,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   522,
     304,   305,   306,   523,     0,     0,     0,     0,     0,     0,
      87,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    33,   297,
     298,   299,   300,   301,   302,   303,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   304,   305,   306,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    38,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    33,   307,     0,     0,     0,
       0,   309,   310,   311,   312,   313,   314,   315,   316,   317,
     318,   319,     0,     0,   297,   298,   299,   300,   301,   302,
     303,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    38,   304,   305,   306,     0,     0,     0,   523,     0,
       0,     0,     0,     0,     0,    87,     0,     0,   309,   310,
     311,   312,   313,   314,   315,   316,   317,   318,   319,     0,
      33,   297,   298,   299,   300,   301,   302,   303,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   304,
     305,   306,     0,     0,     0,   297,   298,   299,   300,   301,
     302,   303,    87,     0,     0,     0,    38,     0,     0,     0,
       0,     0,   382,   304,   305,   306,     0,    33,     0,     0,
       0,     0,     0,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,   319,     0,    31,     0,     0,     0,     0,
       0,    33,   297,   298,   299,   300,   301,   302,   303,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     304,   305,   306,     0,     0,     0,     0,    87,     0,     0,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
     319,     0,     0,     0,     0,     0,     0,     0,    33,     0,
       0,     0,     0,     0,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,     0,   297,   298,   299,   300,
     301,   302,   303,     0,    87,     0,     0,   297,   298,   299,
     300,   301,   302,   303,   304,   305,   306,     0,     0,     0,
       0,     0,     0,     0,     0,   304,   305,   306,    87,     0,
       0,   309,   310,   311,   312,   313,   314,   315,   316,   317,
     318,   319,    33,     0,     0,   297,   298,   299,   300,   301,
     302,   303,     0,    33,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   304,   305,   306,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    87,     0,     0,    38,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    38,
       0,    33,     0,     0,     0,   309,   310,   311,   312,   313,
     314,   315,   316,   317,   318,   319,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,   319,   297,   298,   299,
     300,   301,   302,   303,     0,     0,     0,    38,   511,  1175,
       0,     0,  1176,     0,     0,   304,   305,   306,     0,   511,
       0,     0,     0,   512,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,     0,     0,     0,     0,     0,
       0,     0,     0,    33,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1175,     0,
       0,  1176,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    38,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   270,
     271,   272,   273,   274,   275,   276,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,   319,   167,   168,   169,
       0,    17,    18,    19,    20,     0,     0,     0,    21,    22,
      23,    24,    25,    26,    27,    28,    29,     0,   277,    31,
     355,     0,     0,   512,     0,    33,    34,     0,     0,   356,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    38,     0,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   270,
     271,   272,   273,   274,   275,   276,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   167,   168,   169,
       0,    17,    18,    19,    20,     0,     0,     0,    21,    22,
      23,    24,    25,    26,    27,    28,    29,     0,   277,    31,
       0,     0,     0,     0,     0,    33,    34,    35,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    38,     0,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   270,
     271,   272,   273,   274,   275,   276,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   167,   168,   169,
       0,    17,    18,    19,    20,     0,     0,     0,    21,    22,
      23,    24,    25,    26,    27,    28,    29,     0,   277,    31,
       0,     0,     0,     0,     0,    33,    34,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    38,     0,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   270,
     271,   272,   273,   274,   275,   276,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   167,   168,   169,
       0,    17,    18,    19,    20,     0,     0,     0,    21,    22,
      23,    24,    25,    26,    27,    28,    29,     0,   277,     0,
       0,     0,     0,     0,     0,    33,    34,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    38,     0,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   270,
     271,   272,   273,   274,   275,   276,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   167,   168,   169,
       0,   731,     0,   732,    20,     0,     0,     0,    21,    22,
      23,    24,    25,    26,    27,    28,    29,     0,   277,     0,
       0,     0,     0,     0,     0,    33,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    38,     0,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   270,
     271,   272,   273,   274,   275,   276,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   167,   168,   169,
       0,   297,   298,   299,   300,   301,   302,   303,    21,    22,
      23,    24,    25,    26,    27,    28,    29,     0,   277,   304,
     305,   306,     0,     0,     0,    33,    34,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   382,     0,     0,     0,     0,    33,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    38,     0,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,     0,
       0,     0,     0,   297,   298,   299,   300,   301,   302,   303,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
     319,   304,   305,   306,   297,   298,   299,   300,   301,   302,
     303,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   304,   305,   306,     0,     0,     0,     0,    33,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      33,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    38,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   309,   310,   311,   312,   313,   314,   315,   316,
     317,   318,   319,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,   319
};

static const short int yycheck[] =
{
       5,    35,    35,   123,     5,    36,   110,     5,   131,   559,
     460,     5,     5,     4,   323,   418,   434,   691,   113,   114,
     115,   351,   442,   136,   124,   682,   268,   700,   420,   421,
     323,   690,    37,   589,     5,   850,   553,     5,     5,     5,
     451,   558,   599,   600,   239,   433,   287,   960,   744,   993,
     245,   525,   352,     5,   550,   452,   738,   323,     3,    85,
     104,     5,   744,   873,     3,     3,    37,    32,    35,    35,
     948,   968,   267,     3,    64,   559,   131,     3,     4,     5,
       6,     7,     8,     9,    12,    37,     3,   998,    64,    10,
      11,    35,    32,   124,   137,   612,    46,   131,   131,     0,
     295,    10,    11,   937,    85,    29,    30,    31,    12,    32,
      85,   136,   123,     3,   139,     3,    44,   143,   123,   124,
     131,   126,   415,   534,     0,     3,    10,    11,  1138,   679,
      85,   122,   543,   607,   141,   691,   127,    10,    11,   469,
      44,   132,   110,   110,   959,    10,    11,    26,   821,   141,
     141,   142,   123,   124,    32,   126,    85,    78,   110,   149,
    1170,   581,   143,    84,   473,   131,   110,    45,   143,  1066,
      48,    50,   141,   149,   287,   370,   140,   418,  1089,   131,
     473,   145,    64,   148,  1018,   135,   136,   131,   143,    67,
     135,   136,   709,  1071,   139,   679,   135,    32,   136,   141,
    1078,   139,   698,   899,    78,   135,   136,   473,   148,   139,
     136,   608,   609,   137,   143,    85,  1129,   899,   135,    84,
     916,   695,  1135,   719,    85,   148,   147,   148,  1172,    85,
     618,   286,   141,    19,   916,   279,   529,   281,    32,    85,
    1050,   571,   542,    85,   544,   135,   136,   135,   136,   139,
     141,   139,   286,   286,   367,   914,  1169,   141,   140,    85,
     138,    25,    26,  1177,   269,   135,   559,   149,   141,    55,
      56,   141,   135,   147,   148,    85,   557,   140,   283,   135,
     147,   148,   143,    32,  1198,   141,  1200,   143,   408,   135,
      85,   282,   139,   284,   141,   141,  1210,   143,   269,   140,
     996,   143,   307,   308,   145,   418,   136,   966,  1123,   135,
      85,   366,   283,   148,   996,   141,   360,   143,   323,   324,
     286,   980,   135,   378,  1134,   135,  1136,   140,   569,   135,
     611,   141,   366,   377,   141,   141,   307,   308,   140,   136,
     135,   138,   286,   145,    85,   350,   141,   137,   143,   354,
     140,   356,   323,   324,   148,   366,    26,  1181,   141,   403,
     135,   405,  1058,   604,  1021,   141,   141,    85,   143,    32,
      33,    37,   324,  1197,    44,  1199,  1058,   135,   141,   350,
      50,  1054,   141,   354,   691,   356,   679,  1211,  1062,   141,
     141,  1087,   144,   506,   135,   363,   363,   408,   350,   148,
     141,   141,   143,   408,   961,  1087,   963,   450,   141,   452,
     415,   363,   135,   136,   366,   515,   139,   135,   409,   363,
    1079,   135,   366,   141,   135,   143,   135,   141,   141,   460,
     141,   144,   141,   140,   123,   142,    32,   408,    34,   746,
     747,   748,   141,   893,   415,   102,   451,   135,   453,    45,
     141,   141,    48,   141,   135,   460,   569,   448,   463,   125,
     141,   127,   128,   415,  1138,  1122,   141,   141,   473,   144,
     435,   436,   123,   528,   141,   126,   441,   442,   443,   135,
     451,   135,   453,   725,    85,   141,   141,   141,   683,   460,
     136,   604,   463,   139,   528,   135,  1170,   462,   141,   451,
     141,   141,   473,   895,   535,   141,  1062,   472,   135,   135,
     515,   516,   435,   436,   141,   141,   507,   528,   441,   442,
     443,   135,   527,   514,   529,   530,   135,   141,   135,   534,
     535,   949,   141,   135,   141,   566,  1193,  1194,   543,   462,
     545,   136,    24,   138,   515,   516,   141,   136,   579,    13,
      14,    15,    16,   143,   559,    12,   527,   141,   529,   530,
     144,   566,   138,   534,   516,   608,   609,   532,  1118,   135,
     670,   135,   543,   136,   545,   143,   528,   529,    29,    30,
      31,   139,   534,   136,   528,   141,   591,   135,   559,   554,
     140,   543,   593,   996,   283,   566,   138,   140,  1115,   348,
     435,   436,   140,   352,  1154,   140,   441,   442,   443,   532,
     140,   140,   140,   140,   140,   140,   581,   859,   284,   285,
     591,   278,   140,   280,   591,   591,   956,   462,   140,   140,
     140,  1051,   283,   142,   323,   142,   140,   135,   135,   142,
    1157,   435,   436,   136,   144,   136,   141,   441,   442,   443,
     142,   656,   137,     3,    44,   145,   135,   662,   581,   325,
     326,   135,   135,   144,   144,   670,   142,   145,   462,   140,
     142,   140,   323,   324,   679,  1093,   667,   984,   140,   142,
     139,   135,   737,   138,   350,   656,   435,   436,   140,   802,
      84,   662,   441,   442,   443,   686,   739,   532,   142,   670,
     813,   450,   135,   452,   135,   141,   144,    84,   679,   135,
     145,   140,   135,   462,   757,   139,   137,   145,   145,   408,
     142,   387,   388,   389,  1144,   967,   731,   732,   733,   140,
     139,    37,   140,  1042,   140,    84,   135,  1148,   532,   142,
     131,   141,   135,   140,   142,   402,   581,   404,    84,  1042,
     142,   138,   140,   142,  1061,  1062,  1063,   408,  1176,   142,
     731,   732,   733,   142,   415,   145,   142,   142,  1179,   142,
    1181,   428,   142,   142,   142,   142,  1196,   135,   140,    13,
      13,   135,    47,   532,   473,   451,   142,   581,  1199,  1207,
     145,   142,   715,   542,  1214,   544,   142,   142,   455,   456,
     142,   142,   793,   142,     5,   145,   135,   812,  1117,   460,
     142,   136,   135,     5,   805,     3,     4,     5,     6,     7,
       8,     9,   473,   937,  1117,  1118,   515,   576,   937,   945,
    1047,  1138,   581,   807,    35,   798,   937,  1167,   937,  1100,
     937,   812,   603,   757,   604,   937,   888,   745,     5,   814,
     937,   517,   518,   884,  1018,   812,   536,  1057,  1161,   608,
     609,  1154,   893,  1170,   506,   869,   505,  1204,   534,   864,
     559,   675,   591,   427,   719,   427,   720,   543,   529,   937,
       3,     4,     5,     6,     7,     8,     9,   962,   893,   982,
     579,   814,   549,   984,    -1,   286,   564,   691,   811,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,   559,   110,
      -1,    -1,    -1,    -1,    -1,   566,    -1,   118,   712,    -1,
    1043,    -1,   893,  1043,    -1,    -1,    49,    -1,    -1,    -1,
    1034,  1044,   937,  1046,   939,  1048,   937,    -1,   943,   937,
     945,  1036,   691,   937,   937,    49,    -1,   696,    -1,    -1,
     941,   617,   746,   747,   748,    -1,    -1,   348,    -1,    -1,
      -1,   352,    85,   712,    -1,    -1,   937,    -1,   939,    -1,
     937,   937,   943,    -1,   945,   366,    -1,    -1,    -1,   814,
    1175,   670,    -1,  1103,    -1,   937,    -1,   378,  1043,  1184,
     679,  1104,  1112,   937,    -1,    -1,  1100,   746,   747,   748,
      -1,   968,   668,   669,    -1,    -1,  1101,    -1,    -1,  1043,
    1043,    -1,    -1,  1018,     5,  1020,    -1,  1018,    -1,    -1,
     814,    -1,   688,     5,    -1,    -1,  1069,    -1,   679,  1034,
      -1,    -1,  1043,  1034,    -1,  1155,  1034,  1042,  1043,    -1,
    1034,  1034,  1047,    -1,    -1,    -1,    37,  1018,  1103,  1020,
      -1,  1018,    -1,    -1,  1098,    37,   722,  1112,    -1,   450,
      -1,   452,    -1,  1034,    -1,   814,  1057,  1034,  1020,   735,
     736,  1042,  1043,    -1,  1018,    -1,  1047,  1043,  1112,   873,
      -1,    -1,  1034,    -1,    -1,    -1,  1051,  1118,    -1,    -1,
    1034,  1043,  1103,    -1,    -1,    -1,    -1,    -1,  1103,  1043,
    1155,  1112,    -1,    -1,  1109,  1149,    -1,  1112,  1109,  1153,
      -1,  1109,  1117,  1118,    -1,  1109,  1109,    -1,    -1,    -1,
      -1,  1155,    -1,  1154,   873,    -1,    -1,    -1,  1051,    -1,
      -1,    -1,  1103,  1100,    -1,   126,  1180,   528,  1109,    -1,
      -1,  1112,  1109,  1148,  1155,    -1,  1117,  1118,    -1,  1154,
    1155,   542,    -1,   544,   948,    -1,  1100,  1109,    -1,  1103,
    1112,    -1,   363,    -1,    -1,  1109,    -1,    -1,  1112,    -1,
      -1,    -1,    -1,    -1,  1179,    -1,  1181,  1148,    -1,  1144,
      -1,    -1,    -1,  1154,  1155,   576,    -1,    -1,   982,    -1,
     984,    -1,    -1,    -1,  1199,   884,  1148,    -1,    -1,   948,
      -1,    -1,    -1,  1155,    -1,    -1,    -1,  1151,  1179,    -1,
    1181,  1155,    -1,    -1,    -1,    -1,  1051,   608,   609,    -1,
      -1,  1144,    -1,    -1,    -1,    -1,    -1,  1179,  1199,  1181,
    1195,  1196,    -1,   982,    -1,   984,    -1,    -1,  1203,    -1,
     897,    -1,   893,    -1,    -1,    -1,    -1,  1199,   997,  1214,
      -1,    -1,    -1,    -1,    -1,    -1,  1050,  1051,    12,    13,
      14,    15,    16,    17,    18,    19,    20,  1061,  1062,  1063,
      -1,   472,  1195,  1196,    -1,    -1,   124,  1071,   269,    -1,
    1203,    -1,    -1,    -1,  1078,    -1,   937,   269,    -1,    -1,
      -1,  1214,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1050,  1051,    -1,    -1,   696,    -1,    -1,    -1,  1144,
      -1,    -1,  1061,  1062,  1063,    -1,   307,   308,    -1,    -1,
      -1,    -1,  1071,    -1,    -1,   307,   308,    -1,    -1,  1078,
      -1,  1080,    -1,   324,    -1,   992,    -1,    -1,    -1,    -1,
    1134,    -1,  1136,    -1,  1138,    -1,   737,    -1,    -1,    -1,
    1144,    -1,    -1,  1042,  1020,    -1,    -1,    -1,    -1,    -1,
    1195,  1196,    -1,   354,    -1,   356,    -1,  1018,  1203,    -1,
      -1,    -1,   354,    -1,   356,    -1,  1170,    -1,    -1,  1214,
      -1,    -1,    -1,  1034,    -1,  1134,    -1,  1136,    -1,  1138,
      -1,  1042,  1043,    -1,    -1,  1144,    -1,    -1,    -1,    -1,
      -1,  1195,  1196,    -1,    -1,    -1,    -1,    -1,    -1,  1203,
      -1,    -1,    -1,    -1,    -1,    -1,  1165,    -1,    -1,    -1,
    1214,  1170,    -1,    -1,    -1,    -1,    -1,    -1,  1117,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1186,    -1,    -1,
      -1,    -1,   643,    -1,    -1,    -1,  1195,  1196,    -1,    -1,
      -1,    -1,  1103,    -1,  1203,    -1,    -1,    -1,  1109,    -1,
      -1,  1112,   453,    -1,    -1,  1214,  1117,  1118,    -1,    -1,
      -1,   453,   463,    -1,  1131,    -1,    -1,    -1,    -1,   327,
      -1,    -1,  1148,   331,   332,    -1,   334,   335,   336,   337,
     338,   339,   340,   341,   342,   343,   344,   345,   346,   347,
      -1,    -1,   350,  1154,  1155,    -1,    -1,  1164,    -1,    25,
      26,    27,    -1,  1179,    -1,  1181,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    42,    -1,    -1,    -1,
      -1,    -1,    -1,  1199,    -1,    -1,   527,    -1,    -1,   530,
      -1,    -1,    -1,    -1,    -1,   527,    -1,    -1,   530,    -1,
      -1,    -1,    -1,    -1,   545,    -1,    -1,    -1,    -1,    -1,
      -1,   762,    12,   545,    -1,    -1,    -1,    -1,    -1,   770,
      -1,    -1,    -1,   774,   775,    -1,    -1,   778,   779,   780,
     781,   782,   783,   784,   785,   786,   787,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     591,    -1,    -1,   451,    54,    55,   997,    -1,    -1,    -1,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,   482,   483,   484,    -1,    -1,   487,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1043,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   656,    -1,   515,    -1,    -1,
      -1,   662,    -1,   521,   656,  1066,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   534,    -1,   138,  1080,
     140,   141,    -1,   143,   144,   543,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,    -1,    -1,    -1,
      -1,    -1,  1103,    -1,    -1,    -1,    -1,    -1,    -1,   920,
      -1,  1112,   923,    -1,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    -1,    -1,    -1,    -1,    -1,
     731,   732,   733,    51,    -1,    -1,    -1,    -1,    -1,   731,
     732,   733,    -1,    -1,    -1,    -1,    -1,   958,    -1,    -1,
    1151,    -1,    -1,    -1,  1155,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1165,    -1,    -1,    -1,    -1,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,    -1,  1186,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1018,    -1,    -1,
      -1,   812,   670,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     812,   679,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,   937,    -1,   939,    -1,
      -1,    -1,   943,    -1,   945,   937,    -1,   939,    -1,   807,
      -1,   943,    -1,    -1,   135,   136,    -1,   138,   139,   140,
     141,    -1,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1020,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1020,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1047,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    51,    -1,    -1,   135,
     136,   137,   138,   139,   140,   141,    -1,   143,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,    -1,    78,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,    -1,    -1,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,
      -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,    -1,
     135,    -1,    -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   147,   148,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1148,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1179,    -1,  1181,    -1,    -1,    -1,    -1,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,    -1,
      -1,  1199,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,   146,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   135,   136,    -1,   138,   139,   140,
     141,    -1,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   135,
     136,    -1,   138,   139,   140,   141,   142,   143,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   135,   136,   137,   138,   139,   140,
     141,    -1,   143,   144,    -1,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   135,
     136,   137,   138,   139,   140,   141,    -1,   143,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   135,   136,   137,   138,   139,   140,
     141,    -1,   143,   144,    -1,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   135,
     136,   137,   138,   139,   140,   141,    -1,   143,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   135,   136,   137,   138,   139,   140,
     141,    -1,   143,   144,    -1,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   135,
     136,    -1,   138,   139,   140,   141,    -1,   143,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   135,   136,    -1,   138,   139,   140,
     141,    -1,   143,   144,    -1,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,   140,   141,   142,   143,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   135,   136,    -1,   138,   139,   140,
     141,    -1,   143,   144,    -1,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   135,
     136,    -1,   138,   139,   140,   141,    -1,   143,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,   140,
     141,    -1,   143,   144,    -1,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    -1,    85,
      -1,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   135,
     136,   137,   138,   139,   140,   141,    -1,   143,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,   140,
     141,    -1,   143,   144,    -1,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,   140,   141,    -1,    -1,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    -1,    85,    -1,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,   140,
     141,   142,   143,   144,    -1,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    -1,    85,
      -1,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,   140,   141,   142,   143,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    -1,    85,    -1,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,   140,
     141,   142,   143,   144,    -1,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    -1,    85,
      -1,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,   140,   141,    -1,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    -1,    85,    -1,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,   140,
     141,    -1,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    -1,    85,
      -1,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,   140,   141,    -1,   143,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    -1,    85,    -1,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,   140,
     141,    -1,   143,   144,    -1,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    -1,    85,
      -1,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     136,    -1,    -1,   139,    -1,   141,    -1,   143,   144,    -1,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    45,    -1,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    -1,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    -1,    85,    -1,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   136,    -1,    -1,   139,    -1,
     141,    -1,   143,   144,    -1,   146,   147,   148,    -1,   150,
     151,   152,   153,   154,   155,   156,   157,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    -1,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    -1,    -1,
      -1,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     136,    -1,    -1,   139,    -1,   141,    -1,    -1,   144,    -1,
     146,   147,   148,    -1,   150,   151,   152,   153,   154,   155,
     156,   157,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    23,    -1,    25,    26,    27,    28,    -1,    -1,
      -1,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      -1,    42,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,
      -1,    -1,    -1,    54,    55,    -1,    -1,    -1,    -1,    -1,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    -1,    85,    -1,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    23,    -1,    -1,    -1,    -1,   138,    -1,   140,
     141,    -1,   143,   144,    -1,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    23,    -1,    25,    26,
      27,    28,    -1,    -1,    -1,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    -1,    -1,
      -1,    -1,    49,    50,    51,    -1,    53,    -1,    -1,    -1,
      -1,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,    -1,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,
      -1,    25,    26,    27,    28,    -1,    -1,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    42,    43,
      44,    -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    -1,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
      -1,   135,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    23,    -1,    25,    26,    27,    28,    -1,    -1,
      -1,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    44,    -1,    -1,    -1,    -1,    49,    50,
      -1,    -1,    53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    85,    -1,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,     7,     8,     9,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    21,    22,    23,    -1,    25,
      26,    27,    28,    51,    -1,   146,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    44,    -1,
      -1,    -1,    -1,    49,    50,    -1,    -1,    -1,    -1,    -1,
      78,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,    -1,    -1,    -1,    -1,    -1,    85,
      -1,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,     7,     8,     9,   147,
     148,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    23,    -1,    25,    26,    27,    28,    -1,    -1,
     146,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      -1,    42,    43,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    85,    -1,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    23,    -1,    25,
      26,    27,    28,    -1,    -1,   146,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    42,    43,    -1,    -1,
      -1,    -1,    -1,    49,    50,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,
      -1,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    23,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,
     146,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,    -1,
      -1,    -1,    -1,    49,    50,    -1,    52,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    -1,    43,    44,
      -1,    -1,    -1,    -1,    49,    21,    22,    23,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    49,    -1,    -1,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,    -1,    85,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     146,    -1,    -1,    -1,    -1,    -1,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    -1,    -1,    -1,
      -1,   146,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   135,
      21,    22,    23,   139,    -1,    -1,    -1,    -1,    -1,    -1,
     146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    85,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    50,    -1,    -1,    -1,
      -1,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    21,    22,    23,    -1,    -1,    -1,   139,    -1,
      -1,    -1,    -1,    -1,    -1,   146,    -1,    -1,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,    -1,
      49,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      22,    23,    -1,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,   146,    -1,    -1,    -1,    85,    -1,    -1,    -1,
      -1,    -1,    44,    21,    22,    23,    -1,    49,    -1,    -1,
      -1,    -1,    -1,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,    -1,    43,    -1,    -1,    -1,    -1,
      -1,    49,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    23,    -1,    -1,    -1,    -1,   146,    -1,    -1,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    -1,
      -1,    -1,    -1,    -1,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,    -1,     3,     4,     5,     6,
       7,     8,     9,    -1,   146,    -1,    -1,     3,     4,     5,
       6,     7,     8,     9,    21,    22,    23,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    23,   146,    -1,
      -1,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    49,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    22,    23,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   146,    -1,    -1,    85,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,
      -1,    49,    -1,    -1,    -1,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,     3,     4,     5,
       6,     7,     8,     9,    -1,    -1,    -1,    85,   135,   136,
      -1,    -1,   139,    -1,    -1,    21,    22,    23,    -1,   135,
      -1,    -1,    -1,   139,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   136,    -1,
      -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,     7,     8,     9,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,    21,    22,    23,
      -1,    25,    26,    27,    28,    -1,    -1,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    42,    43,
      44,    -1,    -1,   139,    -1,    49,    50,    -1,    -1,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    -1,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,
      -1,    25,    26,    27,    28,    -1,    -1,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    42,    43,
      -1,    -1,    -1,    -1,    -1,    49,    50,    51,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    -1,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,
      -1,    25,    26,    27,    28,    -1,    -1,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    42,    43,
      -1,    -1,    -1,    -1,    -1,    49,    50,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    -1,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,
      -1,    25,    26,    27,    28,    -1,    -1,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    42,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    50,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    -1,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,
      -1,    25,    -1,    27,    28,    -1,    -1,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    42,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    -1,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,
      -1,     3,     4,     5,     6,     7,     8,     9,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    42,    21,
      22,    23,    -1,    -1,    -1,    49,    50,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    -1,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,     7,     8,     9,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,    21,    22,    23,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    23,    -1,    -1,    -1,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    85,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const unsigned short int yystos[] =
{
       0,   159,   160,     0,   161,   370,   371,     3,     4,     5,
       6,     7,     8,     9,    21,    22,    23,    25,    26,    27,
      28,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      42,    43,    44,    49,    50,    51,    52,    53,    85,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   135,   146,   162,   163,
     164,   165,   166,   168,   169,   170,   171,   172,   174,   177,
     192,   193,   194,   196,   197,   207,   208,   217,   219,   220,
     222,   239,   240,   241,   242,   245,   246,   249,   255,   292,
     322,   323,   324,   325,   327,   328,   329,   330,   332,   334,
     335,   338,   339,   340,   341,   342,   344,   345,   348,   349,
     360,   361,   362,   390,   382,    25,    26,    12,    44,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    35,
      36,    37,    38,    39,    40,    42,    43,    44,    45,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   136,
     138,   139,   140,   141,   143,   144,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,   361,   362,
     415,   416,   417,   447,   448,   449,   450,   451,   333,   350,
       3,     4,     5,     6,     7,     8,     9,    42,   172,   177,
     194,   197,   324,   325,   330,   332,   338,   344,     3,     4,
       5,     6,     7,     8,     9,   136,   335,     3,     4,     5,
       6,     7,     8,     9,    21,    22,    23,    50,    52,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     218,   322,   324,   325,   329,   330,   332,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   336,   173,
     380,   381,   336,   136,   380,    44,    53,   163,   170,   171,
     197,   203,   220,   222,   239,   292,   338,   344,    46,   135,
     136,   265,   266,   265,   265,   265,   141,   177,   338,   344,
     135,   370,    44,   246,   270,   273,   323,   328,   330,   332,
     179,   330,   332,   334,   335,   329,   323,   324,   329,   370,
     329,   143,   172,   177,   194,   197,   208,   246,   325,   339,
     348,   370,    10,    11,    84,   233,   293,   301,   303,    78,
     147,   148,   298,   363,   364,   365,   366,   369,   346,   370,
     370,    24,   383,   136,   447,   443,   443,    64,   149,   223,
     433,   443,   444,   443,    10,    11,   141,   437,   322,    12,
     336,   380,   336,   380,   323,   172,   194,   212,   213,   216,
     233,   301,   443,   138,   167,   322,   322,   135,   372,   373,
     249,   254,   255,   325,     3,     4,     5,     6,     7,     8,
       9,   334,   392,   395,   396,   334,   334,   400,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,    41,    51,   294,   297,   298,   337,   339,   362,
     136,   135,   139,   178,   179,   325,   329,   330,   332,   382,
     294,   195,   135,   139,   198,   322,   322,   380,   338,   233,
     139,   275,   443,   247,   380,   326,   271,   141,   329,   329,
     329,   331,   336,   380,   336,   380,   246,   270,   370,   347,
     304,   250,   252,   254,   255,   256,   268,   312,   323,   325,
     295,   138,   285,   286,   288,   289,   233,   301,   310,   363,
     374,   375,   374,   374,   364,   366,   336,   384,   385,   386,
     160,   444,   135,   143,   145,   445,   446,   447,   137,   224,
     226,   227,   229,   231,   145,   135,   446,   142,   439,   440,
     438,   370,   209,   211,   298,   178,   209,   322,   336,   336,
     210,   312,   323,   330,   332,   137,   324,   330,   136,   135,
     138,   382,    12,    54,    55,    64,   138,   140,   141,   143,
     144,   149,   414,   415,   249,   254,   140,   334,   334,   334,
     140,   140,   334,   140,   140,   140,   140,   140,   140,   140,
     140,   140,   140,   140,   142,   142,   140,   135,   295,   293,
     370,   181,   176,     3,   135,   180,   370,   179,   330,   332,
     325,   135,   200,   201,   334,   199,   198,   370,   322,   325,
     276,   277,   322,   136,   137,   278,   279,   178,   330,   275,
     274,   427,   294,   178,   294,   322,   336,   342,   343,   368,
     251,   372,   257,   314,   315,   316,   372,   250,   256,   323,
     135,   140,   287,   136,   441,   141,   142,   301,   382,   367,
     140,   145,   328,   335,   388,   389,   137,   135,   145,   140,
     433,    25,    27,   197,   324,   330,   332,   338,   355,   356,
     359,   360,    26,    50,   234,   222,   423,   423,   423,   135,
     210,   216,   135,   198,   209,   209,   135,   140,   141,   372,
     135,   160,   221,     3,   144,   144,    64,   149,   142,   145,
     391,   140,   142,   142,   397,   399,   140,   344,   403,   405,
     407,   409,   404,   406,   408,   410,   411,   412,   322,    29,
      30,    31,   137,   182,   183,    29,    30,    31,    37,   187,
     188,   191,   322,   139,   370,   179,   137,   140,   138,   345,
     135,   334,   140,   450,   443,   142,    84,   280,   282,   370,
     329,   272,   278,    24,    85,   138,   139,   140,   143,   144,
     157,   361,   362,   415,   416,   417,   421,   422,   429,   430,
     431,   433,   434,   437,   441,   135,   135,   135,   198,   342,
     257,   141,     3,    32,    45,    48,    67,   138,   258,   260,
     261,   262,   263,   144,   317,   318,   139,   141,   372,   296,
      64,   143,   149,   418,   419,   421,   431,   442,   290,   417,
     302,   368,   385,   145,   329,    84,   387,   437,   225,   322,
     322,   322,   348,   233,   299,   303,   298,   357,   299,   234,
      64,   138,   140,   142,   143,   149,   421,   424,   425,   142,
     142,   135,   135,   211,   214,   137,   344,   145,   145,   344,
     393,   344,   344,   401,   142,   344,   344,   344,   344,   344,
     344,   344,   344,   344,   344,   140,   413,   370,   139,   190,
     191,   140,    37,   189,   233,   175,   370,   201,   202,   138,
     204,   448,   277,   233,   137,   376,   377,   140,   283,   257,
     142,   436,   432,   435,   135,   262,   253,   259,   264,    19,
      55,    56,   437,   319,   318,    13,    14,    15,    16,   313,
     269,   297,   420,   419,   428,   140,   142,   141,   305,   315,
     328,   226,   358,   311,   312,   228,   374,   336,   230,   299,
     278,   299,   142,   344,   398,   142,   344,   142,   142,   142,
     142,   142,   142,   142,   142,   140,   140,   142,    41,    42,
      53,   134,   135,   164,   169,   171,   184,   185,   186,   192,
     193,   207,   217,   220,   222,   243,   244,   245,   270,   292,
     322,   323,   325,   338,   344,   390,   322,   370,   322,   187,
     418,   444,   135,   267,   248,   382,    84,   281,   344,   262,
     376,   423,   427,   423,   278,   437,   338,   349,   351,   352,
     320,   321,   278,   422,   135,   137,   425,   426,   291,   306,
     336,   305,   138,   235,   236,   299,   309,   363,   235,   232,
     142,   135,   394,   140,   402,    13,    13,   169,   177,   205,
     222,   244,   323,   338,   344,   450,   171,   185,   220,   222,
     243,   292,   338,   265,   135,   254,   270,   325,   233,   233,
     188,   233,    47,   257,   282,   299,   145,   433,   145,   142,
     349,   353,   298,   145,   418,   142,   418,   278,   237,   142,
     299,   235,   215,   142,   444,   142,   142,   142,   380,   177,
     205,   338,   265,   177,   233,   338,   372,   254,   256,   450,
     262,   284,   145,   376,   354,   336,   378,   379,   376,   142,
     238,   429,   300,   308,   142,   136,   139,   178,   206,   380,
     177,   380,   135,   372,   136,   285,   336,   382,   376,   429,
     305,    32,    34,    45,    48,   443,   444,   180,   178,   380,
     178,   206,   135,   443,   307,   450,   450,   137,   135,   206,
     178,   180,   137,   308,   444,   206,   135
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned short int yyr1[] =
{
       0,   158,   159,   160,   161,   160,   162,   162,   162,   162,
     162,   162,   162,   162,   162,   162,   162,   162,   162,   162,
     162,   162,   162,   162,   163,   163,   163,   163,   163,   163,
     164,   164,   165,   166,   167,   166,   168,   169,   169,   170,
     170,   170,   171,   171,   173,   172,   175,   174,   174,   176,
     174,   174,   177,   177,   177,   178,   178,   178,   179,   179,
     180,   180,   181,   182,   181,   181,   183,   183,   183,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   185,   185,   185,   185,   185,
     186,   186,   186,   186,   187,   187,   188,   188,   188,   189,
     189,   190,   190,   191,   191,   191,   192,   192,   192,   193,
     193,   195,   194,   196,   196,   197,   197,   197,   198,   199,
     198,   200,   200,   201,   201,   202,   201,   203,   204,   204,
     205,   205,   205,   205,   206,   206,   207,   207,   208,   208,
     208,   208,   208,   209,   210,   210,   211,   212,   212,   214,
     215,   213,   216,   217,   218,   218,   218,   218,   218,   218,
     219,   221,   220,   222,   223,   222,   224,   225,   224,   227,
     228,   226,   229,   230,   226,   231,   232,   226,   233,   233,
     234,   234,   235,   235,   237,   236,   238,   238,   239,   239,
     239,   239,   240,   241,   241,   241,   242,   242,   242,   243,
     243,   243,   244,   244,   244,   244,   245,   245,   245,   247,
     248,   246,   249,   251,   250,   253,   252,   254,   255,   256,
     257,   257,   259,   258,   258,   258,   258,   258,   258,   258,
     260,   261,   261,   262,   262,   264,   263,   265,   265,   265,
     266,   267,   267,   269,   268,   271,   272,   270,   274,   273,
     275,   275,   276,   276,   277,   278,   279,   278,   280,   281,
     280,   280,   280,   283,   284,   282,   285,   285,   287,   286,
     288,   286,   289,   286,   290,   291,   290,   292,   293,   294,
     294,   295,   296,   295,   297,   298,   298,   299,   300,   299,
     301,   302,   301,   304,   303,   303,   305,   306,   307,   305,
     305,   308,   308,   308,   308,   308,   308,   309,   309,   310,
     310,   311,   311,   312,   312,   313,   313,   313,   313,   314,
     314,   316,   315,   317,   317,   319,   318,   320,   321,   320,
     322,   322,   323,   323,   323,   323,   323,   324,   324,   324,
     325,   325,   325,   325,   325,   325,   326,   325,   327,   328,
     329,   331,   330,   333,   332,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   335,   335,   335,   335,
     335,   335,   335,   336,   336,   337,   337,   337,   337,   338,
     338,   339,   339,   339,   339,   340,   340,   340,   340,   340,
     341,   341,   341,   342,   342,   343,   343,   344,   346,   345,
     347,   345,   348,   348,   348,   349,   349,   350,   349,   349,
     349,   351,   353,   352,   354,   352,   355,   357,   356,   358,
     356,   359,   359,   359,   359,   359,   359,   359,   360,   360,
     361,   361,   361,   361,   361,   361,   361,   361,   361,   361,
     361,   361,   361,   361,   361,   361,   361,   361,   361,   361,
     361,   362,   362,   362,   362,   362,   362,   362,   362,   362,
     362,   362,   362,   362,   362,   362,   363,   363,   363,   363,
     364,   365,   367,   366,   368,   368,   369,   369,   371,   370,
     373,   372,   375,   374,   377,   376,   379,   378,   381,   380,
     382,   382,   383,   384,   384,   384,   384,   386,   385,   387,
     387,   388,   388,   389,   389,   391,   390,   392,   393,   394,
     390,   395,   390,   396,   390,   397,   398,   390,   399,   390,
     400,   401,   402,   390,   390,   403,   390,   404,   390,   405,
     390,   406,   390,   407,   390,   408,   390,   409,   390,   410,
     390,   411,   390,   412,   390,   390,   390,   390,   413,   413,
     414,   414,   414,   414,   414,   414,   414,   414,   414,   414,
     414,   414,   415,   415,   415,   415,   415,   415,   415,   415,
     415,   415,   415,   415,   415,   415,   415,   415,   415,   415,
     415,   415,   415,   415,   415,   415,   415,   415,   415,   415,
     415,   415,   415,   415,   415,   415,   416,   416,   416,   416,
     416,   416,   416,   416,   416,   416,   416,   416,   416,   416,
     416,   416,   416,   416,   416,   416,   416,   416,   416,   416,
     416,   416,   416,   416,   416,   417,   417,   417,   417,   417,
     417,   417,   417,   417,   418,   418,   419,   419,   419,   420,
     419,   419,   421,   421,   422,   422,   422,   422,   422,   422,
     422,   422,   422,   422,   423,   423,   424,   424,   424,   424,
     425,   425,   425,   426,   426,   427,   427,   428,   428,   429,
     429,   430,   430,   430,   432,   431,   433,   433,   435,   434,
     436,   434,   438,   437,   439,   437,   440,   437,   442,   441,
     443,   443,   444,   444,   445,   445,   446,   446,   447,   447,
     447,   447,   447,   447,   447,   447,   447,   447,   447,   447,
     447,   447,   447,   447,   447,   448,   449,   449,   450,   451,
     451,   451
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     0,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     2,     2,     2,     2,     2,     2,
       5,     4,     5,     4,     0,     6,     5,     1,     2,     4,
       3,     5,     4,     5,     0,     5,     0,     7,     4,     0,
       5,     2,     1,     1,     1,     3,     4,     2,     1,     1,
       0,     1,     0,     0,     4,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     1,     2,     2,     2,     2,     2,
       2,     3,     2,     3,     1,     4,     2,     4,     4,     0,
       1,     0,     1,     1,     1,     1,     5,     3,     6,     4,
       5,     0,     5,     4,     3,     1,     2,     2,     0,     0,
       3,     1,     3,     0,     1,     0,     4,     6,     2,     1,
       5,     6,     3,     4,     5,     3,     1,     2,     5,     5,
       6,     5,     6,     2,     0,     3,     2,     1,     1,     0,
       0,     8,     1,     3,     1,     2,     2,     2,     3,     3,
       4,     0,     8,     3,     0,     5,     1,     0,     4,     0,
       0,     5,     0,     0,     5,     0,     0,     6,     0,     1,
       1,     1,     0,     1,     0,     3,     1,     2,     2,     2,
       2,     2,     3,     4,     2,     3,     2,     3,     4,     2,
       4,     5,     3,     1,     1,     2,     1,     2,     3,     0,
       0,     9,     2,     0,     4,     0,     7,     2,     1,     3,
       0,     2,     0,     3,     1,     2,     1,     2,     1,     1,
       1,     2,     2,     0,     1,     0,     3,     3,     1,     1,
       6,     0,     6,     0,     7,     0,     0,     6,     0,     6,
       0,     2,     1,     3,     3,     0,     0,     2,     1,     0,
       4,     3,     1,     0,     0,     6,     0,     1,     0,     3,
       0,     2,     0,     4,     1,     0,     4,     4,     2,     0,
       2,     0,     0,     4,     2,     0,     1,     3,     0,     6,
       3,     0,     5,     0,     3,     1,     0,     0,     0,     7,
       1,     0,     2,     2,     3,     3,     2,     1,     2,     1,
       2,     0,     1,     2,     4,     1,     1,     1,     1,     0,
       1,     0,     2,     1,     2,     0,     5,     0,     0,     2,
       1,     1,     1,     1,     1,     2,     2,     2,     2,     2,
       2,     2,     2,     3,     3,     3,     0,     5,     1,     1,
       1,     0,     5,     0,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     3,     1,     1,     1,     1,     2,
       3,     1,     1,     1,     1,     1,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     0,     3,
       0,     4,     1,     3,     4,     1,     1,     0,     4,     2,
       2,     2,     0,     3,     0,     4,     2,     0,     3,     0,
       4,     1,     1,     1,     1,     2,     2,     2,     2,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     2,     0,     4,     0,     1,     1,     2,     0,     2,
       0,     2,     0,     2,     0,     2,     0,     2,     0,     2,
       0,     2,     4,     0,     1,     2,     3,     0,     3,     0,
       1,     1,     2,     1,     3,     0,     7,     0,     0,     0,
       9,     0,     5,     0,     5,     0,     0,    10,     0,     7,
       0,     0,     0,     9,     6,     0,     7,     0,     7,     0,
       7,     0,     7,     0,     7,     0,     7,     0,     7,     0,
       7,     0,     9,     0,     9,     4,     4,     7,     0,     1,
       2,     2,     3,     3,     1,     1,     1,     1,     2,     2,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     1,     1,     0,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     0,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     2,     0,     2,     1,
       1,     1,     1,     1,     0,     4,     1,     1,     0,     4,
       0,     5,     0,     4,     0,     4,     0,     4,     0,     4,
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     4,     3,     1,
       1,     1
};


/* YYDPREC[RULE-NUM] -- Dynamic precedence of rule #RULE-NUM (0 if none).  */
static const unsigned char yydprec[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0
};

/* YYMERGER[RULE-NUM] -- Index of merging function for rule #RULE-NUM.  */
static const unsigned char yymerger[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0
};

/* YYIMMEDIATE[RULE-NUM] -- True iff rule #RULE-NUM is not to be deferred, as
   in the case of predicates.  */
static const yybool yyimmediate[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0
};

/* YYCONFLP[YYPACT[STATE-NUM]] -- Pointer into YYCONFL of start of
   list of conflicting reductions corresponding to action entry for
   state STATE-NUM in yytable.  0 means no conflicts.  The list in
   yyconfl is terminated by a rule number of 0.  */
static const unsigned short int yyconflp[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     1,     0,
       0,     0,     0,     0,     0,     3,     0,     0,     0,     0,
       0,     0,     0,     5,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   257,     0,   261,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   259,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     7,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   251,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   253,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   255,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     9,    11,    13,    15,    17,    19,    21,    23,
      25,    27,    29,    31,    33,    35,    37,    39,    41,    43,
      45,    47,    49,    51,    53,    55,    57,    59,    61,    63,
      65,    67,     0,     0,    69,    71,    73,    75,    77,    79,
       0,    81,    83,    85,    87,     0,     0,    89,    91,    93,
      95,    97,    99,   101,   103,   105,   107,   109,   111,   113,
     115,   117,   119,   121,   123,   125,   127,   129,   131,   133,
     135,   137,   139,   141,   143,   145,   147,   149,   151,   153,
     155,   157,   159,     0,   161,     0,   163,   165,   167,   169,
     171,   173,   175,   177,   179,   181,   183,   185,   187,   189,
     191,   193,   195,   197,   199,   201,   203,   205,   207,   209,
     211,   213,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   215,     0,     0,   217,   219,
     221,     0,   223,   225,     0,   227,   229,   231,   233,   235,
     237,   239,   241,   243,   245,   247,   249,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0
};

/* YYCONFL[I] -- lists of conflicting rule numbers, each terminated by
   0, pointed into by YYCONFLP.  */
static const short int yyconfl[] =
{
       0,   413,     0,   413,     0,   413,     0,   313,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   684,     0,   684,     0,   684,     0,   684,     0,   684,
       0,   296,     0,   296,     0,   296,     0,   413,     0,   306,
       0,   413,     0
};

/* Error token number */
#define YYTERROR 1



YYSTYPE yylval;

int yynerrs;
int yychar;

static const int YYEOF = 0;
static const int YYEMPTY = -2;

typedef enum { yyok, yyaccept, yyabort, yyerr } YYRESULTTAG;

#define YYCHK(YYE)                              \
  do {                                          \
    YYRESULTTAG yychk_flag = YYE;               \
    if (yychk_flag != yyok)                     \
      return yychk_flag;                        \
  } while (0)

#if YYDEBUG

# ifndef YYFPRINTF
#  define YYFPRINTF fprintf
# endif

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YYDPRINTF(Args)                        \
  do {                                          \
    if (yydebug)                                \
      YYFPRINTF Args;                           \
  } while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                  \
  do {                                                                  \
    if (yydebug)                                                        \
      {                                                                 \
        YYFPRINTF (stderr, "%s ", Title);                               \
        yy_symbol_print (stderr, Type, Value);        \
        YYFPRINTF (stderr, "\n");                                       \
      }                                                                 \
  } while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;

struct yyGLRStack;
static void yypstack (struct yyGLRStack* yystackp, size_t yyk)
  YY_ATTRIBUTE_UNUSED;
static void yypdumpstack (struct yyGLRStack* yystackp)
  YY_ATTRIBUTE_UNUSED;

#else /* !YYDEBUG */

# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)

#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYMAXDEPTH * sizeof (GLRStackItem)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

/* Minimum number of free items on the stack allowed after an
   allocation.  This is to allow allocation and initialization
   to be completed by functions that call yyexpandGLRStack before the
   stack is expanded, thus insuring that all necessary pointers get
   properly redirected to new data.  */
#define YYHEADROOM 2

#ifndef YYSTACKEXPANDABLE
#  define YYSTACKEXPANDABLE 1
#endif

#if YYSTACKEXPANDABLE
# define YY_RESERVE_GLRSTACK(Yystack)                   \
  do {                                                  \
    if (Yystack->yyspaceLeft < YYHEADROOM)              \
      yyexpandGLRStack (Yystack);                       \
  } while (0)
#else
# define YY_RESERVE_GLRSTACK(Yystack)                   \
  do {                                                  \
    if (Yystack->yyspaceLeft < YYHEADROOM)              \
      yyMemoryExhausted (Yystack);                      \
  } while (0)
#endif


#if YYERROR_VERBOSE

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static size_t
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return strlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* !YYERROR_VERBOSE */

/** State numbers, as in LALR(1) machine */
typedef int yyStateNum;

/** Rule numbers, as in LALR(1) machine */
typedef int yyRuleNum;

/** Grammar symbol */
typedef int yySymbol;

/** Item references, as in LALR(1) machine */
typedef short int yyItemNum;

typedef struct yyGLRState yyGLRState;
typedef struct yyGLRStateSet yyGLRStateSet;
typedef struct yySemanticOption yySemanticOption;
typedef union yyGLRStackItem yyGLRStackItem;
typedef struct yyGLRStack yyGLRStack;

struct yyGLRState {
  /** Type tag: always true.  */
  yybool yyisState;
  /** Type tag for yysemantics.  If true, yysval applies, otherwise
   *  yyfirstVal applies.  */
  yybool yyresolved;
  /** Number of corresponding LALR(1) machine state.  */
  yyStateNum yylrState;
  /** Preceding state in this stack */
  yyGLRState* yypred;
  /** Source position of the last token produced by my symbol */
  size_t yyposn;
  union {
    /** First in a chain of alternative reductions producing the
     *  non-terminal corresponding to this state, threaded through
     *  yynext.  */
    yySemanticOption* yyfirstVal;
    /** Semantic value for this state.  */
    YYSTYPE yysval;
  } yysemantics;
};

struct yyGLRStateSet {
  yyGLRState** yystates;
  /** During nondeterministic operation, yylookaheadNeeds tracks which
   *  stacks have actually needed the current lookahead.  During deterministic
   *  operation, yylookaheadNeeds[0] is not maintained since it would merely
   *  duplicate yychar != YYEMPTY.  */
  yybool* yylookaheadNeeds;
  size_t yysize, yycapacity;
};

struct yySemanticOption {
  /** Type tag: always false.  */
  yybool yyisState;
  /** Rule number for this reduction */
  yyRuleNum yyrule;
  /** The last RHS state in the list of states to be reduced.  */
  yyGLRState* yystate;
  /** The lookahead for this reduction.  */
  int yyrawchar;
  YYSTYPE yyval;
  /** Next sibling in chain of options.  To facilitate merging,
   *  options are chained in decreasing order by address.  */
  yySemanticOption* yynext;
};

/** Type of the items in the GLR stack.  The yyisState field
 *  indicates which item of the union is valid.  */
union yyGLRStackItem {
  yyGLRState yystate;
  yySemanticOption yyoption;
};

struct yyGLRStack {
  int yyerrState;


  YYJMP_BUF yyexception_buffer;
  yyGLRStackItem* yyitems;
  yyGLRStackItem* yynextFree;
  size_t yyspaceLeft;
  yyGLRState* yysplitPoint;
  yyGLRState* yylastDeleted;
  yyGLRStateSet yytops;
};

#if YYSTACKEXPANDABLE
static void yyexpandGLRStack (yyGLRStack* yystackp);
#endif

static _Noreturn void
yyFail (yyGLRStack* yystackp, const char* yymsg)
{
  if (yymsg != YY_NULLPTR)
    yyerror (yymsg);
  YYLONGJMP (yystackp->yyexception_buffer, 1);
}

static _Noreturn void
yyMemoryExhausted (yyGLRStack* yystackp)
{
  YYLONGJMP (yystackp->yyexception_buffer, 2);
}

#if YYDEBUG || YYERROR_VERBOSE
/** A printable representation of TOKEN.  */
static const char*
yytokenName (yySymbol yytoken)
{
  if (yytoken == YYEMPTY)
    return "";

  return yytname[yytoken];
}
#endif

/** Fill in YYVSP[YYLOW1 .. YYLOW0-1] from the chain of states starting
 *  at YYVSP[YYLOW0].yystate.yypred.  Leaves YYVSP[YYLOW1].yystate.yypred
 *  containing the pointer to the next state in the chain.  */
static void yyfillin (yyGLRStackItem *, int, int);
static void
yyfillin (yyGLRStackItem *yyvsp, int yylow0, int yylow1)
{
  int i;
  yyGLRState *s = yyvsp[yylow0].yystate.yypred;
  for (i = yylow0-1; i >= yylow1; i -= 1)
    {
#if YYDEBUG
      yyvsp[i].yystate.yylrState = s->yylrState;
#endif
      yyvsp[i].yystate.yyresolved = s->yyresolved;
      if (s->yyresolved)
        yyvsp[i].yystate.yysemantics.yysval = s->yysemantics.yysval;
      else
        /* The effect of using yysval or yyloc (in an immediate rule) is
         * undefined.  */
        yyvsp[i].yystate.yysemantics.yyfirstVal = YY_NULLPTR;
      s = yyvsp[i].yystate.yypred = s->yypred;
    }
}

/* Do nothing if YYNORMAL or if *YYLOW <= YYLOW1.  Otherwise, fill in
 * YYVSP[YYLOW1 .. *YYLOW-1] as in yyfillin and set *YYLOW = YYLOW1.
 * For convenience, always return YYLOW1.  */
static int yyfill (yyGLRStackItem *, int *, int, yybool);

static int
yyfill (yyGLRStackItem *yyvsp, int *yylow, int yylow1, yybool yynormal)
{
  if (!yynormal && yylow1 < *yylow)
    {
      yyfillin (yyvsp, *yylow, yylow1);
      *yylow = yylow1;
    }
  return yylow1;
}

/** Perform user action for rule number YYN, with RHS length YYRHSLEN,
 *  and top stack item YYVSP.  YYLVALP points to place to put semantic
 *  value ($$), and yylocp points to place for location information
 *  (@$).  Returns yyok for normal return, yyaccept for YYACCEPT,
 *  yyerr for YYERROR, yyabort for YYABORT.  */
static YYRESULTTAG
yyuserAction (yyRuleNum yyn, size_t yyrhslen, yyGLRStackItem* yyvsp,
              yyGLRStack* yystackp,
              YYSTYPE* yyvalp)
{
  yybool yynormal = (yystackp->yysplitPoint == YY_NULLPTR);
  int yylow;
  YYUSE (yyvalp);
  YYUSE (yyrhslen);
# undef yyerrok
# define yyerrok (yystackp->yyerrState = 0)
# undef YYACCEPT
# define YYACCEPT return yyaccept
# undef YYABORT
# define YYABORT return yyabort
# undef YYERROR
# define YYERROR return yyerrok, yyerr
# undef YYRECOVERING
# define YYRECOVERING() (yystackp->yyerrState != 0)
# undef yyclearin
# define yyclearin (yychar = YYEMPTY)
# undef YYFILL
# define YYFILL(N) yyfill (yyvsp, &yylow, N, yynormal)
# undef YYBACKUP
# define YYBACKUP(Token, Value)                                              \
  return yyerror (YY_("syntax error: cannot back up")),     \
         yyerrok, yyerr

  yylow = 1;
  if (yyrhslen == 0)
    *yyvalp = yyval_default;
  else
    *yyvalp = yyvsp[YYFILL (1-(int)yyrhslen)].yystate.yysemantics.yysval;
  switch (yyn)
    {
        case 4:

    {
      startSig();
      clearType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }

    break;

  case 34:

    { pushNamespace((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 35:

    { popNamespace(); }

    break;

  case 44:

    { pushType(); }

    break;

  case 45:

    {
      const char *name = (currentClass ? currentClass->Name : NULL);
      popType();
      clearTypeId();
      if (name)
        {
        setTypeId(name);
        setTypeBase(guess_id_type(name));
        }
      end_class();
    }

    break;

  case 46:

    {
      start_class((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.integer));
      currentClass->IsFinal = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.integer);
    }

    break;

  case 48:

    {
      start_class((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.integer));
      currentClass->IsFinal = (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer);
    }

    break;

  case 49:

    {
      start_class(NULL, (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.integer));
    }

    break;

  case 51:

    {
      start_class(NULL, (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.integer));
    }

    break;

  case 52:

    { ((*yyvalp).integer) = 0; }

    break;

  case 53:

    { ((*yyvalp).integer) = 1; }

    break;

  case 54:

    { ((*yyvalp).integer) = 2; }

    break;

  case 55:

    { ((*yyvalp).str) = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); }

    break;

  case 56:

    { ((*yyvalp).str) = vtkstrcat3("::", (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); }

    break;

  case 60:

    { ((*yyvalp).integer) = 0; }

    break;

  case 61:

    { ((*yyvalp).integer) = (strcmp((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str), "final") == 0); }

    break;

  case 63:

    {
      startSig();
      clearType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }

    break;

  case 66:

    { access_level = VTK_ACCESS_PUBLIC; }

    break;

  case 67:

    { access_level = VTK_ACCESS_PRIVATE; }

    break;

  case 68:

    { access_level = VTK_ACCESS_PROTECTED; }

    break;

  case 93:

    { output_friend_function(); }

    break;

  case 96:

    { add_base_class(currentClass, (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), access_level, (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 97:

    { add_base_class(currentClass, (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.integer),
                     (VTK_PARSE_VIRTUAL | (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer))); }

    break;

  case 98:

    { add_base_class(currentClass, (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.integer),
                     ((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.integer) | (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer))); }

    break;

  case 99:

    { ((*yyvalp).integer) = 0; }

    break;

  case 100:

    { ((*yyvalp).integer) = VTK_PARSE_VIRTUAL; }

    break;

  case 101:

    { ((*yyvalp).integer) = access_level; }

    break;

  case 103:

    { ((*yyvalp).integer) = VTK_ACCESS_PUBLIC; }

    break;

  case 104:

    { ((*yyvalp).integer) = VTK_ACCESS_PRIVATE; }

    break;

  case 105:

    { ((*yyvalp).integer) = VTK_ACCESS_PROTECTED; }

    break;

  case 111:

    { pushType(); }

    break;

  case 112:

    {
      popType();
      clearTypeId();
      if ((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str) != NULL)
        {
        setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str));
        setTypeBase(guess_id_type((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str)));
        }
      end_enum();
    }

    break;

  case 113:

    {
      start_enum((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.integer), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer), getTypeId());
      clearTypeId();
      ((*yyvalp).str) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str);
    }

    break;

  case 114:

    {
      start_enum(NULL, (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.integer), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer), getTypeId());
      clearTypeId();
      ((*yyvalp).str) = NULL;
    }

    break;

  case 115:

    { ((*yyvalp).integer) = 0; }

    break;

  case 116:

    { ((*yyvalp).integer) = 1; }

    break;

  case 117:

    { ((*yyvalp).integer) = 1; }

    break;

  case 118:

    { ((*yyvalp).integer) = 0; }

    break;

  case 119:

    { pushType(); }

    break;

  case 120:

    { ((*yyvalp).integer) = getType(); popType(); }

    break;

  case 124:

    { closeComment(); add_enum((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str), NULL); }

    break;

  case 125:

    { postSig("="); markSig(); closeComment(); }

    break;

  case 126:

    { chopSig(); add_enum((((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.str), copySig()); }

    break;

  case 149:

    { pushFunction(); postSig("("); }

    break;

  case 150:

    { postSig(")"); }

    break;

  case 151:

    { ((*yyvalp).integer) = (VTK_PARSE_FUNCTION | (((yyGLRStackItem const *)yyvsp)[YYFILL (-7)].yystate.yysemantics.yysval.integer)); popFunction(); }

    break;

  case 152:

    {
      ValueInfo *item = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(item);
      item->ItemType = VTK_TYPEDEF_INFO;
      item->Access = access_level;

      handle_complex_type(item, getType(), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer), getSig());

      if (currentTemplate)
        {
        item->Template = currentTemplate;
        currentTemplate = NULL;
        }

      if (getVarName())
        {
        item->Name = getVarName();
        item->Comment = vtkstrdup(getComment());
        }

      if (item->Class == NULL)
        {
        vtkParse_FreeValue(item);
        }
      else if (currentClass)
        {
        vtkParse_AddTypedefToClass(currentClass, item);
        }
      else
        {
        vtkParse_AddTypedefToNamespace(currentNamespace, item);
        }
    }

    break;

  case 153:

    { add_using((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), 0); }

    break;

  case 155:

    { ((*yyvalp).str) = (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str); }

    break;

  case 156:

    { ((*yyvalp).str) = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 157:

    { ((*yyvalp).str) = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 158:

    { ((*yyvalp).str) = vtkstrcat3((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 159:

    { ((*yyvalp).str) = vtkstrcat3((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 160:

    { add_using((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), 1); }

    break;

  case 161:

    { markSig(); }

    break;

  case 162:

    {
      ValueInfo *item = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(item);
      item->ItemType = VTK_TYPEDEF_INFO;
      item->Access = access_level;

      handle_complex_type(item, getType(), (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.integer), copySig());

      item->Name = (((yyGLRStackItem const *)yyvsp)[YYFILL (-6)].yystate.yysemantics.yysval.str);
      item->Comment = vtkstrdup(getComment());

      if (currentTemplate)
        {
        vtkParse_FreeValue(item);
        }
      else if (currentClass)
        {
        vtkParse_AddTypedefToClass(currentClass, item);
        }
      else
        {
        vtkParse_AddTypedefToNamespace(currentNamespace, item);
        }
    }

    break;

  case 163:

    { postSig("template<> "); clearTypeId(); }

    break;

  case 164:

    {
      postSig("template<");
      pushType();
      clearType();
      clearTypeId();
      startTemplate();
    }

    break;

  case 165:

    {
      chopSig();
      if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig("> ");
      clearTypeId();
      popType();
    }

    break;

  case 167:

    { chopSig(); postSig(", "); clearType(); clearTypeId(); }

    break;

  case 169:

    { markSig(); }

    break;

  case 170:

    { add_template_parameter(getType(), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer), copySig()); }

    break;

  case 172:

    { markSig(); }

    break;

  case 173:

    { add_template_parameter(0, (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer), copySig()); }

    break;

  case 175:

    { pushTemplate(); markSig(); }

    break;

  case 176:

    {
      int i;
      TemplateInfo *newTemplate = currentTemplate;
      popTemplate();
      add_template_parameter(0, (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer), copySig());
      i = currentTemplate->NumberOfParameters-1;
      currentTemplate->Parameters[i]->Template = newTemplate;
    }

    break;

  case 178:

    { ((*yyvalp).integer) = 0; }

    break;

  case 179:

    { postSig("..."); ((*yyvalp).integer) = VTK_PARSE_PACK; }

    break;

  case 180:

    { postSig("class "); }

    break;

  case 181:

    { postSig("typename "); }

    break;

  case 184:

    { postSig("="); markSig(); }

    break;

  case 185:

    {
      int i = currentTemplate->NumberOfParameters-1;
      ValueInfo *param = currentTemplate->Parameters[i];
      chopSig();
      param->Value = copySig();
    }

    break;

  case 188:

    { output_function(); }

    break;

  case 189:

    { output_function(); }

    break;

  case 190:

    { reject_function(); }

    break;

  case 191:

    { reject_function(); }

    break;

  case 199:

    { output_function(); }

    break;

  case 209:

    {
      postSig("(");
      currentFunction->IsExplicit = ((getType() & VTK_PARSE_EXPLICIT) != 0);
      set_return(currentFunction, getType(), getTypeId(), 0);
    }

    break;

  case 210:

    { postSig(")"); }

    break;

  case 211:

    {
      postSig(";");
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", "operator typecast");
    }

    break;

  case 212:

    { ((*yyvalp).str) = copySig(); }

    break;

  case 213:

    { postSig(")"); }

    break;

  case 214:

    {
      postSig(";");
      closeSig();
      currentFunction->Name = (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }

    break;

  case 215:

    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getType(), getTypeId(), 0);
    }

    break;

  case 217:

    { chopSig(); ((*yyvalp).str) = vtkstrcat(copySig(), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 218:

    { markSig(); postSig("operator "); }

    break;

  case 219:

    {
      postSig(";");
      closeSig();
      currentFunction->Name = (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }

    break;

  case 222:

    { postSig(" throw "); }

    break;

  case 223:

    { chopSig(); }

    break;

  case 224:

    { postSig(" const"); currentFunction->IsConst = 1; }

    break;

  case 225:

    {
      postSig(" = 0");
      currentFunction->IsPureVirtual = 1;
      if (currentClass) { currentClass->IsAbstract = 1; }
    }

    break;

  case 226:

    {
      postSig(" "); postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str));
      if (strcmp((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str), "final") == 0) { currentFunction->IsFinal = 1; }
    }

    break;

  case 227:

    { chopSig(); }

    break;

  case 230:

    { postSig(" noexcept"); }

    break;

  case 231:

    { currentFunction->IsDeleted = 1; }

    break;

  case 235:

    { postSig(" -> "); clearType(); clearTypeId(); }

    break;

  case 236:

    {
      chopSig();
      set_return(currentFunction, getType(), getTypeId(), 0);
    }

    break;

  case 243:

    {
      postSig("(");
      set_return(currentFunction, getType(), getTypeId(), 0);
    }

    break;

  case 244:

    { postSig(")"); }

    break;

  case 245:

    { closeSig(); }

    break;

  case 246:

    { openSig(); }

    break;

  case 247:

    {
      postSig(";");
      closeSig();
      if (getType() & VTK_PARSE_VIRTUAL)
        {
        currentFunction->IsVirtual = 1;
        }
      if (getType() & VTK_PARSE_EXPLICIT)
        {
        currentFunction->IsExplicit = 1;
        }
      currentFunction->Name = (((yyGLRStackItem const *)yyvsp)[YYFILL (-5)].yystate.yysemantics.yysval.str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }

    break;

  case 248:

    { pushType(); postSig("("); }

    break;

  case 249:

    { popType(); postSig(")"); }

    break;

  case 256:

    { clearType(); clearTypeId(); }

    break;

  case 258:

    { clearType(); clearTypeId(); }

    break;

  case 259:

    { clearType(); clearTypeId(); postSig(", "); }

    break;

  case 261:

    { currentFunction->IsVariadic = 1; postSig(", ..."); }

    break;

  case 262:

    { currentFunction->IsVariadic = 1; postSig("..."); }

    break;

  case 263:

    { markSig(); }

    break;

  case 264:

    {
      ValueInfo *param = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(param);

      handle_complex_type(param, getType(), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer), copySig());
      add_legacy_parameter(currentFunction, param);

      if (getVarName())
        {
        param->Name = getVarName();
        }

      vtkParse_AddParameterToFunction(currentFunction, param);
    }

    break;

  case 265:

    {
      int i = currentFunction->NumberOfParameters-1;
      if (getVarValue())
        {
        currentFunction->Parameters[i]->Value = getVarValue();
        }
    }

    break;

  case 266:

    { clearVarValue(); }

    break;

  case 268:

    { postSig("="); clearVarValue(); markSig(); }

    break;

  case 269:

    { chopSig(); setVarValue(copySig()); }

    break;

  case 270:

    { clearVarValue(); markSig(); }

    break;

  case 271:

    { chopSig(); setVarValue(copySig()); }

    break;

  case 272:

    { clearVarValue(); markSig(); postSig("("); }

    break;

  case 273:

    { chopSig(); postSig(")"); setVarValue(copySig()); }

    break;

  case 274:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 275:

    { postSig(", "); }

    break;

  case 278:

    {
      unsigned int type = getType();
      ValueInfo *var = (ValueInfo *)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(var);
      var->ItemType = VTK_VARIABLE_INFO;
      var->Access = access_level;

      handle_complex_type(var, type, (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.integer), getSig());

      if (currentTemplate)
        {
        var->Template = currentTemplate;
        currentTemplate = NULL;
        }

      var->Name = getVarName();
      var->Comment = vtkstrdup(getComment());

      if (getVarValue())
        {
        var->Value = getVarValue();
        }

      /* Is this a typedef? */
      if ((type & VTK_PARSE_TYPEDEF) != 0)
        {
        var->ItemType = VTK_TYPEDEF_INFO;
        if (var->Class == NULL)
          {
          vtkParse_FreeValue(var);
          }
        else if (currentClass)
          {
          vtkParse_AddTypedefToClass(currentClass, var);
          }
        else
          {
          vtkParse_AddTypedefToNamespace(currentNamespace, var);
          }
        }
      /* Is this a constant? */
      else if (((type & VTK_PARSE_CONST) != 0) && var->Value != NULL &&
          (((type & VTK_PARSE_INDIRECT) == 0) ||
           ((type & VTK_PARSE_INDIRECT) == VTK_PARSE_ARRAY)))
        {
        var->ItemType = VTK_CONSTANT_INFO;
        if (currentClass)
          {
          vtkParse_AddConstantToClass(currentClass, var);
          }
        else
          {
          vtkParse_AddConstantToNamespace(currentNamespace, var);
          }
        }
      /* This is a true variable i.e. not constant */
      else
        {
        if (currentClass)
          {
          vtkParse_AddVariableToClass(currentClass, var);
          }
        else
          {
          vtkParse_AddVariableToNamespace(currentNamespace, var);
          }
        }
    }

    break;

  case 282:

    { postSig(", "); }

    break;

  case 285:

    { setTypePtr(0); }

    break;

  case 286:

    { setTypePtr((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 287:

    {
      if ((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer) == VTK_PARSE_FUNCTION)
        {
        ((*yyvalp).integer) = (VTK_PARSE_FUNCTION_PTR | (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.integer));
        }
      else
        {
        ((*yyvalp).integer) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.integer);
        }
    }

    break;

  case 288:

    { postSig(")"); }

    break;

  case 289:

    {
      const char *scope = getScope();
      unsigned int parens = add_indirection((((yyGLRStackItem const *)yyvsp)[YYFILL (-5)].yystate.yysemantics.yysval.integer), (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.integer));
      if ((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer) == VTK_PARSE_FUNCTION)
        {
        if (scope) { scope = vtkstrndup(scope, strlen(scope) - 2); }
        getFunction()->Class = scope;
        ((*yyvalp).integer) = (parens | VTK_PARSE_FUNCTION);
        }
      else if ((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer) == VTK_PARSE_ARRAY)
        {
        ((*yyvalp).integer) = add_indirection_to_array(parens);
        }
    }

    break;

  case 290:

    { ((*yyvalp).integer) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.integer); }

    break;

  case 291:

    { postSig(")"); }

    break;

  case 292:

    {
      const char *scope = getScope();
      unsigned int parens = add_indirection((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.integer), (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.integer));
      if ((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer) == VTK_PARSE_FUNCTION)
        {
        if (scope) { scope = vtkstrndup(scope, strlen(scope) - 2); }
        getFunction()->Class = scope;
        ((*yyvalp).integer) = (parens | VTK_PARSE_FUNCTION);
        }
      else if ((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer) == VTK_PARSE_ARRAY)
        {
        ((*yyvalp).integer) = add_indirection_to_array(parens);
        }
    }

    break;

  case 293:

    { postSig("("); scopeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); postSig("*"); }

    break;

  case 294:

    { ((*yyvalp).integer) = (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer); }

    break;

  case 295:

    { postSig("("); scopeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); postSig("&");
         ((*yyvalp).integer) = VTK_PARSE_REF; }

    break;

  case 296:

    { ((*yyvalp).integer) = 0; }

    break;

  case 297:

    { pushFunction(); postSig("("); }

    break;

  case 298:

    { postSig(")"); }

    break;

  case 299:

    {
      ((*yyvalp).integer) = VTK_PARSE_FUNCTION;
      popFunction();
    }

    break;

  case 300:

    { ((*yyvalp).integer) = VTK_PARSE_ARRAY; }

    break;

  case 303:

    { currentFunction->IsConst = 1; }

    break;

  case 308:

    { ((*yyvalp).integer) = add_indirection((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.integer), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 310:

    { ((*yyvalp).integer) = add_indirection((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.integer), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 311:

    { clearVarName(); chopSig(); }

    break;

  case 313:

    { setVarName((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); }

    break;

  case 314:

    { setVarName((((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.str)); }

    break;

  case 319:

    { clearArray(); }

    break;

  case 321:

    { clearArray(); }

    break;

  case 325:

    { postSig("["); }

    break;

  case 326:

    { postSig("]"); }

    break;

  case 327:

    { pushArraySize(""); }

    break;

  case 328:

    { markSig(); }

    break;

  case 329:

    { chopSig(); pushArraySize(copySig()); }

    break;

  case 335:

    { ((*yyvalp).str) = vtkstrcat("~", (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 336:

    { ((*yyvalp).str) = vtkstrcat("~", (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 337:

    { ((*yyvalp).str) = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 338:

    { ((*yyvalp).str) = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 339:

    { ((*yyvalp).str) = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 340:

    { ((*yyvalp).str) = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 341:

    { ((*yyvalp).str) = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 342:

    { ((*yyvalp).str) = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 343:

    { ((*yyvalp).str) = vtkstrcat3((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 344:

    { ((*yyvalp).str) = vtkstrcat3((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 345:

    { ((*yyvalp).str) = vtkstrcat3((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 346:

    { postSig("template "); }

    break;

  case 347:

    { ((*yyvalp).str) = vtkstrcat4((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), "template ", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 348:

    { postSig("~"); }

    break;

  case 349:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 350:

    { ((*yyvalp).str) = "::"; postSig(((*yyvalp).str)); }

    break;

  case 351:

    { markSig(); postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); postSig("<"); }

    break;

  case 352:

    {
      chopSig(); if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig(">"); ((*yyvalp).str) = copySig(); clearTypeId();
    }

    break;

  case 353:

    { markSig(); postSig("decltype"); }

    break;

  case 354:

    { chopSig(); ((*yyvalp).str) = copySig(); clearTypeId(); }

    break;

  case 355:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 356:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 357:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 358:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 359:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 360:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 361:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 362:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 363:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 364:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 365:

    { ((*yyvalp).str) = "vtkTypeInt8"; postSig(((*yyvalp).str)); }

    break;

  case 366:

    { ((*yyvalp).str) = "vtkTypeUInt8"; postSig(((*yyvalp).str)); }

    break;

  case 367:

    { ((*yyvalp).str) = "vtkTypeInt16"; postSig(((*yyvalp).str)); }

    break;

  case 368:

    { ((*yyvalp).str) = "vtkTypeUInt16"; postSig(((*yyvalp).str)); }

    break;

  case 369:

    { ((*yyvalp).str) = "vtkTypeInt32"; postSig(((*yyvalp).str)); }

    break;

  case 370:

    { ((*yyvalp).str) = "vtkTypeUInt32"; postSig(((*yyvalp).str)); }

    break;

  case 371:

    { ((*yyvalp).str) = "vtkTypeInt64"; postSig(((*yyvalp).str)); }

    break;

  case 372:

    { ((*yyvalp).str) = "vtkTypeUInt64"; postSig(((*yyvalp).str)); }

    break;

  case 373:

    { ((*yyvalp).str) = "vtkTypeFloat32"; postSig(((*yyvalp).str)); }

    break;

  case 374:

    { ((*yyvalp).str) = "vtkTypeFloat64"; postSig(((*yyvalp).str)); }

    break;

  case 375:

    { ((*yyvalp).str) = "vtkIdType"; postSig(((*yyvalp).str)); }

    break;

  case 386:

    { setTypeBase(buildTypeBase(getType(), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer))); }

    break;

  case 387:

    { setTypeMod(VTK_PARSE_TYPEDEF); }

    break;

  case 388:

    { setTypeMod(VTK_PARSE_FRIEND); }

    break;

  case 391:

    { setTypeMod((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 392:

    { setTypeMod((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 393:

    { setTypeMod((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 394:

    { postSig("constexpr "); ((*yyvalp).integer) = 0; }

    break;

  case 395:

    { postSig("mutable "); ((*yyvalp).integer) = VTK_PARSE_MUTABLE; }

    break;

  case 396:

    { ((*yyvalp).integer) = 0; }

    break;

  case 397:

    { ((*yyvalp).integer) = 0; }

    break;

  case 398:

    { postSig("static "); ((*yyvalp).integer) = VTK_PARSE_STATIC; }

    break;

  case 399:

    { postSig("thread_local "); ((*yyvalp).integer) = VTK_PARSE_THREAD_LOCAL; }

    break;

  case 400:

    { ((*yyvalp).integer) = 0; }

    break;

  case 401:

    { postSig("virtual "); ((*yyvalp).integer) = VTK_PARSE_VIRTUAL; }

    break;

  case 402:

    { postSig("explicit "); ((*yyvalp).integer) = VTK_PARSE_EXPLICIT; }

    break;

  case 403:

    { postSig("const "); ((*yyvalp).integer) = VTK_PARSE_CONST; }

    break;

  case 404:

    { postSig("volatile "); ((*yyvalp).integer) = VTK_PARSE_VOLATILE; }

    break;

  case 406:

    { ((*yyvalp).integer) = ((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.integer) | (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 408:

    { setTypeBase((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 410:

    { setTypeBase((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 413:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = guess_id_type((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 414:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = guess_id_type((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); }

    break;

  case 416:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = 0; }

    break;

  case 417:

    { postSig("typename "); }

    break;

  case 418:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = guess_id_type((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); }

    break;

  case 419:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = guess_id_type((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); }

    break;

  case 420:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = guess_id_type((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str)); }

    break;

  case 422:

    { setTypeBase((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 424:

    { setTypeBase((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 427:

    { setTypeBase((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 429:

    { setTypeBase((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 432:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = 0; }

    break;

  case 433:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = guess_id_type((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 434:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = guess_id_type((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 435:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = guess_id_type((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 436:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = guess_id_type((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 437:

    { postSig(" "); setTypeId((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = guess_id_type((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 438:

    { setTypeId(""); }

    break;

  case 440:

    { typeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = VTK_PARSE_STRING; }

    break;

  case 441:

    { typeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = VTK_PARSE_UNICODE_STRING;}

    break;

  case 442:

    { typeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = VTK_PARSE_OSTREAM; }

    break;

  case 443:

    { typeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = VTK_PARSE_ISTREAM; }

    break;

  case 444:

    { typeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = VTK_PARSE_UNKNOWN; }

    break;

  case 445:

    { typeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = VTK_PARSE_OBJECT; }

    break;

  case 446:

    { typeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = VTK_PARSE_QOBJECT; }

    break;

  case 447:

    { typeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = VTK_PARSE_NULLPTR_T; }

    break;

  case 448:

    { typeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = VTK_PARSE_SSIZE_T; }

    break;

  case 449:

    { typeSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); ((*yyvalp).integer) = VTK_PARSE_SIZE_T; }

    break;

  case 450:

    { typeSig("vtkTypeInt8"); ((*yyvalp).integer) = VTK_PARSE_INT8; }

    break;

  case 451:

    { typeSig("vtkTypeUInt8"); ((*yyvalp).integer) = VTK_PARSE_UINT8; }

    break;

  case 452:

    { typeSig("vtkTypeInt16"); ((*yyvalp).integer) = VTK_PARSE_INT16; }

    break;

  case 453:

    { typeSig("vtkTypeUInt16"); ((*yyvalp).integer) = VTK_PARSE_UINT16; }

    break;

  case 454:

    { typeSig("vtkTypeInt32"); ((*yyvalp).integer) = VTK_PARSE_INT32; }

    break;

  case 455:

    { typeSig("vtkTypeUInt32"); ((*yyvalp).integer) = VTK_PARSE_UINT32; }

    break;

  case 456:

    { typeSig("vtkTypeInt64"); ((*yyvalp).integer) = VTK_PARSE_INT64; }

    break;

  case 457:

    { typeSig("vtkTypeUInt64"); ((*yyvalp).integer) = VTK_PARSE_UINT64; }

    break;

  case 458:

    { typeSig("vtkTypeFloat32"); ((*yyvalp).integer) = VTK_PARSE_FLOAT32; }

    break;

  case 459:

    { typeSig("vtkTypeFloat64"); ((*yyvalp).integer) = VTK_PARSE_FLOAT64; }

    break;

  case 460:

    { typeSig("vtkIdType"); ((*yyvalp).integer) = VTK_PARSE_ID_TYPE; }

    break;

  case 461:

    { postSig("auto "); ((*yyvalp).integer) = 0; }

    break;

  case 462:

    { postSig("void "); ((*yyvalp).integer) = VTK_PARSE_VOID; }

    break;

  case 463:

    { postSig("bool "); ((*yyvalp).integer) = VTK_PARSE_BOOL; }

    break;

  case 464:

    { postSig("float "); ((*yyvalp).integer) = VTK_PARSE_FLOAT; }

    break;

  case 465:

    { postSig("double "); ((*yyvalp).integer) = VTK_PARSE_DOUBLE; }

    break;

  case 466:

    { postSig("char "); ((*yyvalp).integer) = VTK_PARSE_CHAR; }

    break;

  case 467:

    { postSig("char16_t "); ((*yyvalp).integer) = VTK_PARSE_CHAR16_T; }

    break;

  case 468:

    { postSig("char32_t "); ((*yyvalp).integer) = VTK_PARSE_CHAR32_T; }

    break;

  case 469:

    { postSig("wchar_t "); ((*yyvalp).integer) = VTK_PARSE_WCHAR_T; }

    break;

  case 470:

    { postSig("int "); ((*yyvalp).integer) = VTK_PARSE_INT; }

    break;

  case 471:

    { postSig("short "); ((*yyvalp).integer) = VTK_PARSE_SHORT; }

    break;

  case 472:

    { postSig("long "); ((*yyvalp).integer) = VTK_PARSE_LONG; }

    break;

  case 473:

    { postSig("__int64 "); ((*yyvalp).integer) = VTK_PARSE___INT64; }

    break;

  case 474:

    { postSig("signed "); ((*yyvalp).integer) = VTK_PARSE_INT; }

    break;

  case 475:

    { postSig("unsigned "); ((*yyvalp).integer) = VTK_PARSE_UNSIGNED_INT; }

    break;

  case 479:

    { ((*yyvalp).integer) = ((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.integer) | (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 480:

    { postSig("&"); ((*yyvalp).integer) = VTK_PARSE_REF; }

    break;

  case 481:

    { postSig("&&"); ((*yyvalp).integer) = (VTK_PARSE_RVALUE | VTK_PARSE_REF); }

    break;

  case 482:

    { postSig("*"); }

    break;

  case 483:

    { ((*yyvalp).integer) = (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer); }

    break;

  case 484:

    { ((*yyvalp).integer) = VTK_PARSE_POINTER; }

    break;

  case 485:

    {
      if (((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer) & VTK_PARSE_CONST) != 0)
        {
        ((*yyvalp).integer) = VTK_PARSE_CONST_POINTER;
        }
      if (((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer) & VTK_PARSE_VOLATILE) != 0)
        {
        ((*yyvalp).integer) = VTK_PARSE_BAD_INDIRECT;
        }
    }

    break;

  case 487:

    {
      unsigned int n;
      n = (((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.integer) << 2) | (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer));
      if ((n & VTK_PARSE_INDIRECT) != n)
        {
        n = VTK_PARSE_BAD_INDIRECT;
        }
      ((*yyvalp).integer) = n;
    }

    break;

  case 488:

    { setAttributeRole(VTK_PARSE_ATTRIB_DECL); }

    break;

  case 489:

    { clearAttributeRole(); }

    break;

  case 490:

    { setAttributeRole(VTK_PARSE_ATTRIB_ID); }

    break;

  case 491:

    { clearAttributeRole(); }

    break;

  case 492:

    { setAttributeRole(VTK_PARSE_ATTRIB_REF); }

    break;

  case 493:

    { clearAttributeRole(); }

    break;

  case 494:

    { setAttributeRole(VTK_PARSE_ATTRIB_FUNC); }

    break;

  case 495:

    { clearAttributeRole(); }

    break;

  case 496:

    { setAttributeRole(VTK_PARSE_ATTRIB_ARRAY); }

    break;

  case 497:

    { clearAttributeRole(); }

    break;

  case 498:

    { setAttributeRole(VTK_PARSE_ATTRIB_CLASS); }

    break;

  case 499:

    { clearAttributeRole(); }

    break;

  case 507:

    { markSig(); }

    break;

  case 508:

    { handle_attribute(cutSig(), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.integer)); }

    break;

  case 509:

    { ((*yyvalp).integer) = 0; }

    break;

  case 510:

    { ((*yyvalp).integer) = VTK_PARSE_PACK; }

    break;

  case 515:

    {preSig("void Set"); postSig("(");}

    break;

  case 516:

    {
   postSig("a);");
   currentFunction->Macro = "vtkSetMacro";
   currentFunction->Name = vtkstrcat("Set", (((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, getType(), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }

    break;

  case 517:

    {postSig("Get");}

    break;

  case 518:

    {markSig();}

    break;

  case 519:

    {swapSig();}

    break;

  case 520:

    {
   postSig("();");
   currentFunction->Macro = "vtkGetMacro";
   currentFunction->Name = vtkstrcat("Get", (((yyGLRStackItem const *)yyvsp)[YYFILL (-5)].yystate.yysemantics.yysval.str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, getType(), getTypeId(), 0);
   output_function();
   }

    break;

  case 521:

    {preSig("void Set");}

    break;

  case 522:

    {
   postSig("(char *);");
   currentFunction->Macro = "vtkSetStringMacro";
   currentFunction->Name = vtkstrcat("Set", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }

    break;

  case 523:

    {preSig("char *Get");}

    break;

  case 524:

    {
   postSig("();");
   currentFunction->Macro = "vtkGetStringMacro";
   currentFunction->Name = vtkstrcat("Get", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_CHAR_PTR, "char", 0);
   output_function();
   }

    break;

  case 525:

    {startSig(); markSig();}

    break;

  case 526:

    {closeSig();}

    break;

  case 527:

    {
   const char *typeText;
   chopSig();
   typeText = copySig();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat("Set", (((yyGLRStackItem const *)yyvsp)[YYFILL (-7)].yystate.yysemantics.yysval.str));
   currentFunction->Signature =
     vtkstrcat5("void ", currentFunction->Name, "(", typeText, ");");
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, getType(), getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat3("Get", (((yyGLRStackItem const *)yyvsp)[YYFILL (-7)].yystate.yysemantics.yysval.str), "MinValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, getType(), getTypeId(), 0);
   output_function();

   currentFunction->Macro = "vtkSetClampMacro";
   currentFunction->Name = vtkstrcat3("Get", (((yyGLRStackItem const *)yyvsp)[YYFILL (-7)].yystate.yysemantics.yysval.str), "MaxValue");
   currentFunction->Signature =
     vtkstrcat4(typeText, " ", currentFunction->Name, "();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, getType(), getTypeId(), 0);
   output_function();
   }

    break;

  case 528:

    {preSig("void Set"); postSig("("); }

    break;

  case 529:

    {
   postSig("*);");
   currentFunction->Macro = "vtkSetObjectMacro";
   currentFunction->Name = vtkstrcat("Set", (((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str));
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }

    break;

  case 530:

    {postSig("*Get");}

    break;

  case 531:

    {markSig();}

    break;

  case 532:

    {swapSig();}

    break;

  case 533:

    {
   postSig("();");
   currentFunction->Macro = "vtkGetObjectMacro";
   currentFunction->Name = vtkstrcat("Get", (((yyGLRStackItem const *)yyvsp)[YYFILL (-5)].yystate.yysemantics.yysval.str));
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_OBJECT_PTR, getTypeId(), 0);
   output_function();
   }

    break;

  case 534:

    {
   currentFunction->Macro = "vtkBooleanMacro";
   currentFunction->Name = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.str), "On");
   currentFunction->Comment = vtkstrdup(getComment());
   currentFunction->Signature =
     vtkstrcat3("void ", currentFunction->Name, "();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();

   currentFunction->Macro = "vtkBooleanMacro";
   currentFunction->Name = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.str), "Off");
   currentFunction->Comment = vtkstrdup(getComment());
   currentFunction->Signature =
     vtkstrcat3("void ", currentFunction->Name, "();");
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }

    break;

  case 535:

    {startSig(); markSig();}

    break;

  case 536:

    {
   chopSig();
   outputSetVectorMacro((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), getType(), copySig(), 2);
   }

    break;

  case 537:

    {startSig(); markSig();}

    break;

  case 538:

    {
   chopSig();
   outputGetVectorMacro((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), getType(), copySig(), 2);
   }

    break;

  case 539:

    {startSig(); markSig();}

    break;

  case 540:

    {
   chopSig();
   outputSetVectorMacro((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), getType(), copySig(), 3);
   }

    break;

  case 541:

    {startSig(); markSig();}

    break;

  case 542:

    {
   chopSig();
   outputGetVectorMacro((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), getType(), copySig(), 3);
   }

    break;

  case 543:

    {startSig(); markSig();}

    break;

  case 544:

    {
   chopSig();
   outputSetVectorMacro((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), getType(), copySig(), 4);
   }

    break;

  case 545:

    {startSig(); markSig();}

    break;

  case 546:

    {
   chopSig();
   outputGetVectorMacro((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), getType(), copySig(), 4);
   }

    break;

  case 547:

    {startSig(); markSig();}

    break;

  case 548:

    {
   chopSig();
   outputSetVectorMacro((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), getType(), copySig(), 6);
   }

    break;

  case 549:

    {startSig(); markSig();}

    break;

  case 550:

    {
   chopSig();
   outputGetVectorMacro((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), getType(), copySig(), 6);
   }

    break;

  case 551:

    {startSig(); markSig();}

    break;

  case 552:

    {
   const char *typeText;
   chopSig();
   typeText = copySig();
   currentFunction->Macro = "vtkSetVectorMacro";
   currentFunction->Name = vtkstrcat("Set", (((yyGLRStackItem const *)yyvsp)[YYFILL (-6)].yystate.yysemantics.yysval.str));
   currentFunction->Signature =
     vtkstrcat7("void ", currentFunction->Name, "(", typeText,
                " a[", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), "]);");
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, (VTK_PARSE_POINTER | getType()),
                 getTypeId(), (int)strtol((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), NULL, 0));
   set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
   output_function();
   }

    break;

  case 553:

    {startSig();}

    break;

  case 554:

    {
   chopSig();
   currentFunction->Macro = "vtkGetVectorMacro";
   currentFunction->Name = vtkstrcat("Get", (((yyGLRStackItem const *)yyvsp)[YYFILL (-6)].yystate.yysemantics.yysval.str));
   postSig(" *");
   postSig(currentFunction->Name);
   postSig("();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (VTK_PARSE_POINTER | getType()),
              getTypeId(), (int)strtol((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), NULL, 0));
   output_function();
   }

    break;

  case 555:

    {
     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat3("Get", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), "Coordinate");
     currentFunction->Signature =
       vtkstrcat3("vtkCoordinate *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double, double);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[2]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_parameter(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkViewportCoordinateMacro";
     currentFunction->Name = vtkstrcat("Get", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str));
     currentFunction->Signature =
       vtkstrcat3("double *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 2);
     output_function();
   }

    break;

  case 556:

    {
     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat3("Get", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str), "Coordinate");
     currentFunction->Signature =
       vtkstrcat3("vtkCoordinate *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkCoordinate", 0);
     output_function();

     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double, double, double);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     add_parameter(currentFunction, VTK_PARSE_DOUBLE, "double", 0);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat("Set", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str));
     currentFunction->Signature =
       vtkstrcat3("void ", currentFunction->Name, "(double a[3]);");
     currentFunction->Comment = vtkstrdup(getComment());
     add_parameter(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
     output_function();

     currentFunction->Macro = "vtkWorldCoordinateMacro";
     currentFunction->Name = vtkstrcat("Get", (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.str));
     currentFunction->Signature =
       vtkstrcat3("double *", currentFunction->Name, "();");
     currentFunction->Comment = vtkstrdup(getComment());
     set_return(currentFunction, VTK_PARSE_DOUBLE_PTR, "double", 3);
     output_function();
   }

    break;

  case 557:

    {
   currentFunction->Macro = "vtkTypeMacro";
   currentFunction->Name = "GetClassName";
   currentFunction->Signature = "const char *GetClassName();";
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
              "char", 0);
   output_function();

   currentFunction->Macro = "vtkTypeMacro";
   currentFunction->Name = "IsA";
   currentFunction->Signature = "int IsA(const char *name);";
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, (VTK_PARSE_CONST | VTK_PARSE_CHAR_PTR),
                "char", 0);
   set_return(currentFunction, VTK_PARSE_INT, "int", 0);
   output_function();

   currentFunction->Macro = "vtkTypeMacro";
   currentFunction->Name = "NewInstance";
   currentFunction->Signature = vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), " *NewInstance();");
   currentFunction->Comment = vtkstrdup(getComment());
   set_return(currentFunction, VTK_PARSE_NEWINSTANCE | VTK_PARSE_OBJECT_PTR,
              (((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), 0);
   output_function();

   currentFunction->Macro = "vtkTypeMacro";
   currentFunction->Name = "SafeDownCast";
   currentFunction->Signature =
     vtkstrcat((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), " *SafeDownCast(vtkObject* o);");
   currentFunction->Comment = vtkstrdup(getComment());
   add_parameter(currentFunction, VTK_PARSE_OBJECT_PTR, "vtkObject", 0);
   set_return(currentFunction, (VTK_PARSE_STATIC | VTK_PARSE_OBJECT_PTR),
              (((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.str), 0);
   output_function();
   }

    break;

  case 560:

    { ((*yyvalp).str) = "()"; }

    break;

  case 561:

    { ((*yyvalp).str) = "[]"; }

    break;

  case 562:

    { ((*yyvalp).str) = " new[]"; }

    break;

  case 563:

    { ((*yyvalp).str) = " delete[]"; }

    break;

  case 564:

    { ((*yyvalp).str) = "<"; }

    break;

  case 565:

    { ((*yyvalp).str) = ">"; }

    break;

  case 566:

    { ((*yyvalp).str) = ","; }

    break;

  case 567:

    { ((*yyvalp).str) = "="; }

    break;

  case 568:

    { ((*yyvalp).str) = ">>"; }

    break;

  case 569:

    { ((*yyvalp).str) = ">>"; }

    break;

  case 570:

    { ((*yyvalp).str) = vtkstrcat("\"\" ", (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); }

    break;

  case 572:

    { ((*yyvalp).str) = "%"; }

    break;

  case 573:

    { ((*yyvalp).str) = "*"; }

    break;

  case 574:

    { ((*yyvalp).str) = "/"; }

    break;

  case 575:

    { ((*yyvalp).str) = "-"; }

    break;

  case 576:

    { ((*yyvalp).str) = "+"; }

    break;

  case 577:

    { ((*yyvalp).str) = "!"; }

    break;

  case 578:

    { ((*yyvalp).str) = "~"; }

    break;

  case 579:

    { ((*yyvalp).str) = "&"; }

    break;

  case 580:

    { ((*yyvalp).str) = "|"; }

    break;

  case 581:

    { ((*yyvalp).str) = "^"; }

    break;

  case 582:

    { ((*yyvalp).str) = " new"; }

    break;

  case 583:

    { ((*yyvalp).str) = " delete"; }

    break;

  case 584:

    { ((*yyvalp).str) = "<<="; }

    break;

  case 585:

    { ((*yyvalp).str) = ">>="; }

    break;

  case 586:

    { ((*yyvalp).str) = "<<"; }

    break;

  case 587:

    { ((*yyvalp).str) = ".*"; }

    break;

  case 588:

    { ((*yyvalp).str) = "->*"; }

    break;

  case 589:

    { ((*yyvalp).str) = "->"; }

    break;

  case 590:

    { ((*yyvalp).str) = "+="; }

    break;

  case 591:

    { ((*yyvalp).str) = "-="; }

    break;

  case 592:

    { ((*yyvalp).str) = "*="; }

    break;

  case 593:

    { ((*yyvalp).str) = "/="; }

    break;

  case 594:

    { ((*yyvalp).str) = "%="; }

    break;

  case 595:

    { ((*yyvalp).str) = "++"; }

    break;

  case 596:

    { ((*yyvalp).str) = "--"; }

    break;

  case 597:

    { ((*yyvalp).str) = "&="; }

    break;

  case 598:

    { ((*yyvalp).str) = "|="; }

    break;

  case 599:

    { ((*yyvalp).str) = "^="; }

    break;

  case 600:

    { ((*yyvalp).str) = "&&"; }

    break;

  case 601:

    { ((*yyvalp).str) = "||"; }

    break;

  case 602:

    { ((*yyvalp).str) = "=="; }

    break;

  case 603:

    { ((*yyvalp).str) = "!="; }

    break;

  case 604:

    { ((*yyvalp).str) = "<="; }

    break;

  case 605:

    { ((*yyvalp).str) = ">="; }

    break;

  case 606:

    { ((*yyvalp).str) = "typedef"; }

    break;

  case 607:

    { ((*yyvalp).str) = "typename"; }

    break;

  case 608:

    { ((*yyvalp).str) = "class"; }

    break;

  case 609:

    { ((*yyvalp).str) = "struct"; }

    break;

  case 610:

    { ((*yyvalp).str) = "union"; }

    break;

  case 611:

    { ((*yyvalp).str) = "template"; }

    break;

  case 612:

    { ((*yyvalp).str) = "public"; }

    break;

  case 613:

    { ((*yyvalp).str) = "protected"; }

    break;

  case 614:

    { ((*yyvalp).str) = "private"; }

    break;

  case 615:

    { ((*yyvalp).str) = "const"; }

    break;

  case 616:

    { ((*yyvalp).str) = "static"; }

    break;

  case 617:

    { ((*yyvalp).str) = "thread_local"; }

    break;

  case 618:

    { ((*yyvalp).str) = "constexpr"; }

    break;

  case 619:

    { ((*yyvalp).str) = "inline"; }

    break;

  case 620:

    { ((*yyvalp).str) = "virtual"; }

    break;

  case 621:

    { ((*yyvalp).str) = "explicit"; }

    break;

  case 622:

    { ((*yyvalp).str) = "decltype"; }

    break;

  case 623:

    { ((*yyvalp).str) = "default"; }

    break;

  case 624:

    { ((*yyvalp).str) = "extern"; }

    break;

  case 625:

    { ((*yyvalp).str) = "using"; }

    break;

  case 626:

    { ((*yyvalp).str) = "namespace"; }

    break;

  case 627:

    { ((*yyvalp).str) = "operator"; }

    break;

  case 628:

    { ((*yyvalp).str) = "enum"; }

    break;

  case 629:

    { ((*yyvalp).str) = "throw"; }

    break;

  case 630:

    { ((*yyvalp).str) = "noexcept"; }

    break;

  case 631:

    { ((*yyvalp).str) = "const_cast"; }

    break;

  case 632:

    { ((*yyvalp).str) = "dynamic_cast"; }

    break;

  case 633:

    { ((*yyvalp).str) = "static_cast"; }

    break;

  case 634:

    { ((*yyvalp).str) = "reinterpret_cast"; }

    break;

  case 648:

    { postSig("< "); }

    break;

  case 649:

    { postSig("> "); }

    break;

  case 651:

    { postSig(">"); }

    break;

  case 653:

    { chopSig(); postSig("::"); }

    break;

  case 657:

    {
      if ((((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str))[0] == '+' || ((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str))[0] == '-' ||
           ((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str))[0] == '*' || ((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str))[0] == '&') &&
          ((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str))[1] == '\0')
        {
        int c1 = 0;
        size_t l;
        const char *cp;
        chopSig();
        cp = getSig();
        l = getSigLength();
        if (l != 0) { c1 = cp[l-1]; }
        if (c1 != 0 && c1 != '(' && c1 != '[' && c1 != '=')
          {
          postSig(" ");
          }
        postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str));
        if (vtkParse_CharType(c1, (CPRE_XID|CPRE_QUOTE)) ||
            c1 == ')' || c1 == ']')
          {
          postSig(" ");
          }
        }
       else
        {
        postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str));
        postSig(" ");
        }
    }

    break;

  case 658:

    { postSig(":"); postSig(" "); }

    break;

  case 659:

    { postSig("."); }

    break;

  case 660:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); postSig(" "); }

    break;

  case 661:

    { postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); postSig(" "); }

    break;

  case 663:

    {
      int c1 = 0;
      size_t l;
      const char *cp;
      chopSig();
      cp = getSig();
      l = getSigLength();
      if (l != 0) { c1 = cp[l-1]; }
      while (vtkParse_CharType(c1, CPRE_XID) && l != 0)
        {
        --l;
        c1 = cp[l-1];
        }
      if (l < 2 || cp[l-1] != ':' || cp[l-2] != ':')
        {
        cp = add_const_scope(&cp[l]);
        resetSig(l);
        postSig(cp);
        }
      postSig(" ");
    }

    break;

  case 667:

    { postSig("< "); }

    break;

  case 668:

    { postSig("> "); }

    break;

  case 669:

    { postSig(">"); }

    break;

  case 671:

    { postSig("= "); }

    break;

  case 672:

    { chopSig(); postSig(", "); }

    break;

  case 674:

    { chopSig(); postSig(";"); }

    break;

  case 682:

    { postSig("= "); }

    break;

  case 683:

    { chopSig(); postSig(", "); }

    break;

  case 684:

    {
      chopSig();
      if (getSig()[getSigLength()-1] == '<') { postSig(" "); }
      postSig("<");
    }

    break;

  case 685:

    {
      chopSig();
      if (getSig()[getSigLength()-1] == '>') { postSig(" "); }
      postSig("> ");
    }

    break;

  case 688:

    { postSig("["); }

    break;

  case 689:

    { chopSig(); postSig("] "); }

    break;

  case 690:

    { postSig("[["); }

    break;

  case 691:

    { chopSig(); postSig("]] "); }

    break;

  case 692:

    { postSig("("); }

    break;

  case 693:

    { chopSig(); postSig(") "); }

    break;

  case 694:

    { postSig("("); postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); postSig("*"); }

    break;

  case 695:

    { chopSig(); postSig(") "); }

    break;

  case 696:

    { postSig("("); postSig((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.str)); postSig("&"); }

    break;

  case 697:

    { chopSig(); postSig(") "); }

    break;

  case 698:

    { postSig("{ "); }

    break;

  case 699:

    { postSig("} "); }

    break;



      default: break;
    }

  return yyok;
# undef yyerrok
# undef YYABORT
# undef YYACCEPT
# undef YYERROR
# undef YYBACKUP
# undef yyclearin
# undef YYRECOVERING
}


static void
yyuserMerge (int yyn, YYSTYPE* yy0, YYSTYPE* yy1)
{
  YYUSE (yy0);
  YYUSE (yy1);

  switch (yyn)
    {

      default: break;
    }
}

                              /* Bison grammar-table manipulation.  */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}

/** Number of symbols composing the right hand side of rule #RULE.  */
static int
yyrhsLength (yyRuleNum yyrule)
{
  return yyr2[yyrule];
}

static void
yydestroyGLRState (char const *yymsg, yyGLRState *yys)
{
  if (yys->yyresolved)
    yydestruct (yymsg, yystos[yys->yylrState],
                &yys->yysemantics.yysval);
  else
    {
#if YYDEBUG
      if (yydebug)
        {
          if (yys->yysemantics.yyfirstVal)
            YYFPRINTF (stderr, "%s unresolved", yymsg);
          else
            YYFPRINTF (stderr, "%s incomplete", yymsg);
          YY_SYMBOL_PRINT ("", yystos[yys->yylrState], YY_NULLPTR, &yys->yyloc);
        }
#endif

      if (yys->yysemantics.yyfirstVal)
        {
          yySemanticOption *yyoption = yys->yysemantics.yyfirstVal;
          yyGLRState *yyrh;
          int yyn;
          for (yyrh = yyoption->yystate, yyn = yyrhsLength (yyoption->yyrule);
               yyn > 0;
               yyrh = yyrh->yypred, yyn -= 1)
            yydestroyGLRState (yymsg, yyrh);
        }
    }
}

/** Left-hand-side symbol for rule #YYRULE.  */
static yySymbol
yylhsNonterm (yyRuleNum yyrule)
{
  return yyr1[yyrule];
}

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-1011)))

/** True iff LR state YYSTATE has only a default reduction (regardless
 *  of token).  */
static yybool
yyisDefaultedState (yyStateNum yystate)
{
  return yypact_value_is_default (yypact[yystate]);
}

/** The default reduction for YYSTATE, assuming it has one.  */
static yyRuleNum
yydefaultAction (yyStateNum yystate)
{
  return yydefact[yystate];
}

#define yytable_value_is_error(Yytable_value) \
  0

/** Set *YYACTION to the action to take in YYSTATE on seeing YYTOKEN.
 *  Result R means
 *    R < 0:  Reduce on rule -R.
 *    R = 0:  Error.
 *    R > 0:  Shift to state R.
 *  Set *YYCONFLICTS to a pointer into yyconfl to a 0-terminated list
 *  of conflicting reductions.
 */
static void
yygetLRActions (yyStateNum yystate, int yytoken,
                int* yyaction, const short int** yyconflicts)
{
  int yyindex = yypact[yystate] + yytoken;
  if (yypact_value_is_default (yypact[yystate])
      || yyindex < 0 || YYLAST < yyindex || yycheck[yyindex] != yytoken)
    {
      *yyaction = -yydefact[yystate];
      *yyconflicts = yyconfl;
    }
  else if (! yytable_value_is_error (yytable[yyindex]))
    {
      *yyaction = yytable[yyindex];
      *yyconflicts = yyconfl + yyconflp[yyindex];
    }
  else
    {
      *yyaction = 0;
      *yyconflicts = yyconfl + yyconflp[yyindex];
    }
}

/** Compute post-reduction state.
 * \param yystate   the current state
 * \param yysym     the nonterminal to push on the stack
 */
static yyStateNum
yyLRgotoState (yyStateNum yystate, yySymbol yysym)
{
  int yyr = yypgoto[yysym - YYNTOKENS] + yystate;
  if (0 <= yyr && yyr <= YYLAST && yycheck[yyr] == yystate)
    return yytable[yyr];
  else
    return yydefgoto[yysym - YYNTOKENS];
}

static yybool
yyisShiftAction (int yyaction)
{
  return 0 < yyaction;
}

static yybool
yyisErrorAction (int yyaction)
{
  return yyaction == 0;
}

                                /* GLRStates */

/** Return a fresh GLRStackItem in YYSTACKP.  The item is an LR state
 *  if YYISSTATE, and otherwise a semantic option.  Callers should call
 *  YY_RESERVE_GLRSTACK afterwards to make sure there is sufficient
 *  headroom.  */

static yyGLRStackItem*
yynewGLRStackItem (yyGLRStack* yystackp, yybool yyisState)
{
  yyGLRStackItem* yynewItem = yystackp->yynextFree;
  yystackp->yyspaceLeft -= 1;
  yystackp->yynextFree += 1;
  yynewItem->yystate.yyisState = yyisState;
  return yynewItem;
}

/** Add a new semantic action that will execute the action for rule
 *  YYRULE on the semantic values in YYRHS to the list of
 *  alternative actions for YYSTATE.  Assumes that YYRHS comes from
 *  stack #YYK of *YYSTACKP. */
static void
yyaddDeferredAction (yyGLRStack* yystackp, size_t yyk, yyGLRState* yystate,
                     yyGLRState* yyrhs, yyRuleNum yyrule)
{
  yySemanticOption* yynewOption =
    &yynewGLRStackItem (yystackp, yyfalse)->yyoption;
  YYASSERT (!yynewOption->yyisState);
  yynewOption->yystate = yyrhs;
  yynewOption->yyrule = yyrule;
  if (yystackp->yytops.yylookaheadNeeds[yyk])
    {
      yynewOption->yyrawchar = yychar;
      yynewOption->yyval = yylval;
    }
  else
    yynewOption->yyrawchar = YYEMPTY;
  yynewOption->yynext = yystate->yysemantics.yyfirstVal;
  yystate->yysemantics.yyfirstVal = yynewOption;

  YY_RESERVE_GLRSTACK (yystackp);
}

                                /* GLRStacks */

/** Initialize YYSET to a singleton set containing an empty stack.  */
static yybool
yyinitStateSet (yyGLRStateSet* yyset)
{
  yyset->yysize = 1;
  yyset->yycapacity = 16;
  yyset->yystates = (yyGLRState**) YYMALLOC (16 * sizeof yyset->yystates[0]);
  if (! yyset->yystates)
    return yyfalse;
  yyset->yystates[0] = YY_NULLPTR;
  yyset->yylookaheadNeeds =
    (yybool*) YYMALLOC (16 * sizeof yyset->yylookaheadNeeds[0]);
  if (! yyset->yylookaheadNeeds)
    {
      YYFREE (yyset->yystates);
      return yyfalse;
    }
  return yytrue;
}

static void yyfreeStateSet (yyGLRStateSet* yyset)
{
  YYFREE (yyset->yystates);
  YYFREE (yyset->yylookaheadNeeds);
}

/** Initialize *YYSTACKP to a single empty stack, with total maximum
 *  capacity for all stacks of YYSIZE.  */
static yybool
yyinitGLRStack (yyGLRStack* yystackp, size_t yysize)
{
  yystackp->yyerrState = 0;
  yynerrs = 0;
  yystackp->yyspaceLeft = yysize;
  yystackp->yyitems =
    (yyGLRStackItem*) YYMALLOC (yysize * sizeof yystackp->yynextFree[0]);
  if (!yystackp->yyitems)
    return yyfalse;
  yystackp->yynextFree = yystackp->yyitems;
  yystackp->yysplitPoint = YY_NULLPTR;
  yystackp->yylastDeleted = YY_NULLPTR;
  return yyinitStateSet (&yystackp->yytops);
}


#if YYSTACKEXPANDABLE
# define YYRELOC(YYFROMITEMS,YYTOITEMS,YYX,YYTYPE) \
  &((YYTOITEMS) - ((YYFROMITEMS) - (yyGLRStackItem*) (YYX)))->YYTYPE

/** If *YYSTACKP is expandable, extend it.  WARNING: Pointers into the
    stack from outside should be considered invalid after this call.
    We always expand when there are 1 or fewer items left AFTER an
    allocation, so that we can avoid having external pointers exist
    across an allocation.  */
static void
yyexpandGLRStack (yyGLRStack* yystackp)
{
  yyGLRStackItem* yynewItems;
  yyGLRStackItem* yyp0, *yyp1;
  size_t yynewSize;
  size_t yyn;
  size_t yysize = yystackp->yynextFree - yystackp->yyitems;
  if (YYMAXDEPTH - YYHEADROOM < yysize)
    yyMemoryExhausted (yystackp);
  yynewSize = 2*yysize;
  if (YYMAXDEPTH < yynewSize)
    yynewSize = YYMAXDEPTH;
  yynewItems = (yyGLRStackItem*) YYMALLOC (yynewSize * sizeof yynewItems[0]);
  if (! yynewItems)
    yyMemoryExhausted (yystackp);
  for (yyp0 = yystackp->yyitems, yyp1 = yynewItems, yyn = yysize;
       0 < yyn;
       yyn -= 1, yyp0 += 1, yyp1 += 1)
    {
      *yyp1 = *yyp0;
      if (*(yybool *) yyp0)
        {
          yyGLRState* yys0 = &yyp0->yystate;
          yyGLRState* yys1 = &yyp1->yystate;
          if (yys0->yypred != YY_NULLPTR)
            yys1->yypred =
              YYRELOC (yyp0, yyp1, yys0->yypred, yystate);
          if (! yys0->yyresolved && yys0->yysemantics.yyfirstVal != YY_NULLPTR)
            yys1->yysemantics.yyfirstVal =
              YYRELOC (yyp0, yyp1, yys0->yysemantics.yyfirstVal, yyoption);
        }
      else
        {
          yySemanticOption* yyv0 = &yyp0->yyoption;
          yySemanticOption* yyv1 = &yyp1->yyoption;
          if (yyv0->yystate != YY_NULLPTR)
            yyv1->yystate = YYRELOC (yyp0, yyp1, yyv0->yystate, yystate);
          if (yyv0->yynext != YY_NULLPTR)
            yyv1->yynext = YYRELOC (yyp0, yyp1, yyv0->yynext, yyoption);
        }
    }
  if (yystackp->yysplitPoint != YY_NULLPTR)
    yystackp->yysplitPoint = YYRELOC (yystackp->yyitems, yynewItems,
                                      yystackp->yysplitPoint, yystate);

  for (yyn = 0; yyn < yystackp->yytops.yysize; yyn += 1)
    if (yystackp->yytops.yystates[yyn] != YY_NULLPTR)
      yystackp->yytops.yystates[yyn] =
        YYRELOC (yystackp->yyitems, yynewItems,
                 yystackp->yytops.yystates[yyn], yystate);
  YYFREE (yystackp->yyitems);
  yystackp->yyitems = yynewItems;
  yystackp->yynextFree = yynewItems + yysize;
  yystackp->yyspaceLeft = yynewSize - yysize;
}
#endif

static void
yyfreeGLRStack (yyGLRStack* yystackp)
{
  YYFREE (yystackp->yyitems);
  yyfreeStateSet (&yystackp->yytops);
}

/** Assuming that YYS is a GLRState somewhere on *YYSTACKP, update the
 *  splitpoint of *YYSTACKP, if needed, so that it is at least as deep as
 *  YYS.  */
static void
yyupdateSplit (yyGLRStack* yystackp, yyGLRState* yys)
{
  if (yystackp->yysplitPoint != YY_NULLPTR && yystackp->yysplitPoint > yys)
    yystackp->yysplitPoint = yys;
}

/** Invalidate stack #YYK in *YYSTACKP.  */
static void
yymarkStackDeleted (yyGLRStack* yystackp, size_t yyk)
{
  if (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
    yystackp->yylastDeleted = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yystates[yyk] = YY_NULLPTR;
}

/** Undelete the last stack in *YYSTACKP that was marked as deleted.  Can
    only be done once after a deletion, and only when all other stacks have
    been deleted.  */
static void
yyundeleteLastStack (yyGLRStack* yystackp)
{
  if (yystackp->yylastDeleted == YY_NULLPTR || yystackp->yytops.yysize != 0)
    return;
  yystackp->yytops.yystates[0] = yystackp->yylastDeleted;
  yystackp->yytops.yysize = 1;
  YYDPRINTF ((stderr, "Restoring last deleted stack as stack #0.\n"));
  yystackp->yylastDeleted = YY_NULLPTR;
}

static void
yyremoveDeletes (yyGLRStack* yystackp)
{
  size_t yyi, yyj;
  yyi = yyj = 0;
  while (yyj < yystackp->yytops.yysize)
    {
      if (yystackp->yytops.yystates[yyi] == YY_NULLPTR)
        {
          if (yyi == yyj)
            {
              YYDPRINTF ((stderr, "Removing dead stacks.\n"));
            }
          yystackp->yytops.yysize -= 1;
        }
      else
        {
          yystackp->yytops.yystates[yyj] = yystackp->yytops.yystates[yyi];
          /* In the current implementation, it's unnecessary to copy
             yystackp->yytops.yylookaheadNeeds[yyi] since, after
             yyremoveDeletes returns, the parser immediately either enters
             deterministic operation or shifts a token.  However, it doesn't
             hurt, and the code might evolve to need it.  */
          yystackp->yytops.yylookaheadNeeds[yyj] =
            yystackp->yytops.yylookaheadNeeds[yyi];
          if (yyj != yyi)
            {
              YYDPRINTF ((stderr, "Rename stack %lu -> %lu.\n",
                          (unsigned long int) yyi, (unsigned long int) yyj));
            }
          yyj += 1;
        }
      yyi += 1;
    }
}

/** Shift to a new state on stack #YYK of *YYSTACKP, corresponding to LR
 * state YYLRSTATE, at input position YYPOSN, with (resolved) semantic
 * value *YYVALP and source location *YYLOCP.  */
static void
yyglrShift (yyGLRStack* yystackp, size_t yyk, yyStateNum yylrState,
            size_t yyposn,
            YYSTYPE* yyvalp)
{
  yyGLRState* yynewState = &yynewGLRStackItem (yystackp, yytrue)->yystate;

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yytrue;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yysval = *yyvalp;
  yystackp->yytops.yystates[yyk] = yynewState;

  YY_RESERVE_GLRSTACK (yystackp);
}

/** Shift stack #YYK of *YYSTACKP, to a new state corresponding to LR
 *  state YYLRSTATE, at input position YYPOSN, with the (unresolved)
 *  semantic value of YYRHS under the action for YYRULE.  */
static void
yyglrShiftDefer (yyGLRStack* yystackp, size_t yyk, yyStateNum yylrState,
                 size_t yyposn, yyGLRState* yyrhs, yyRuleNum yyrule)
{
  yyGLRState* yynewState = &yynewGLRStackItem (yystackp, yytrue)->yystate;
  YYASSERT (yynewState->yyisState);

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yyfalse;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yyfirstVal = YY_NULLPTR;
  yystackp->yytops.yystates[yyk] = yynewState;

  /* Invokes YY_RESERVE_GLRSTACK.  */
  yyaddDeferredAction (yystackp, yyk, yynewState, yyrhs, yyrule);
}

#if !YYDEBUG
# define YY_REDUCE_PRINT(Args)
#else
# define YY_REDUCE_PRINT(Args)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print Args;               \
} while (0)

/*----------------------------------------------------------------------.
| Report that stack #YYK of *YYSTACKP is going to be reduced by YYRULE. |
`----------------------------------------------------------------------*/

static void
yy_reduce_print (int yynormal, yyGLRStackItem* yyvsp, size_t yyk,
                 yyRuleNum yyrule)
{
  int yynrhs = yyrhsLength (yyrule);
  int yyi;
  YYFPRINTF (stderr, "Reducing stack %lu by rule %d (line %lu):\n",
             (unsigned long int) yyk, yyrule - 1,
             (unsigned long int) yyrline[yyrule]);
  if (! yynormal)
    yyfillin (yyvsp, 1, -yynrhs);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyvsp[yyi - yynrhs + 1].yystate.yylrState],
                       &yyvsp[yyi - yynrhs + 1].yystate.yysemantics.yysval
                                              );
      if (!yyvsp[yyi - yynrhs + 1].yystate.yyresolved)
        YYFPRINTF (stderr, " (unresolved)");
      YYFPRINTF (stderr, "\n");
    }
}
#endif

/** Pop the symbols consumed by reduction #YYRULE from the top of stack
 *  #YYK of *YYSTACKP, and perform the appropriate semantic action on their
 *  semantic values.  Assumes that all ambiguities in semantic values
 *  have been previously resolved.  Set *YYVALP to the resulting value,
 *  and *YYLOCP to the computed location (if any).  Return value is as
 *  for userAction.  */
static YYRESULTTAG
yydoAction (yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule,
            YYSTYPE* yyvalp)
{
  int yynrhs = yyrhsLength (yyrule);

  if (yystackp->yysplitPoint == YY_NULLPTR)
    {
      /* Standard special case: single stack.  */
      yyGLRStackItem* yyrhs = (yyGLRStackItem*) yystackp->yytops.yystates[yyk];
      YYASSERT (yyk == 0);
      yystackp->yynextFree -= yynrhs;
      yystackp->yyspaceLeft += yynrhs;
      yystackp->yytops.yystates[0] = & yystackp->yynextFree[-1].yystate;
      YY_REDUCE_PRINT ((1, yyrhs, yyk, yyrule));
      return yyuserAction (yyrule, yynrhs, yyrhs, yystackp,
                           yyvalp);
    }
  else
    {
      int yyi;
      yyGLRState* yys;
      yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
      yys = yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred
        = yystackp->yytops.yystates[yyk];
      for (yyi = 0; yyi < yynrhs; yyi += 1)
        {
          yys = yys->yypred;
          YYASSERT (yys);
        }
      yyupdateSplit (yystackp, yys);
      yystackp->yytops.yystates[yyk] = yys;
      YY_REDUCE_PRINT ((0, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1, yyk, yyrule));
      return yyuserAction (yyrule, yynrhs, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1,
                           yystackp, yyvalp);
    }
}

/** Pop items off stack #YYK of *YYSTACKP according to grammar rule YYRULE,
 *  and push back on the resulting nonterminal symbol.  Perform the
 *  semantic action associated with YYRULE and store its value with the
 *  newly pushed state, if YYFORCEEVAL or if *YYSTACKP is currently
 *  unambiguous.  Otherwise, store the deferred semantic action with
 *  the new state.  If the new state would have an identical input
 *  position, LR state, and predecessor to an existing state on the stack,
 *  it is identified with that existing state, eliminating stack #YYK from
 *  *YYSTACKP.  In this case, the semantic value is
 *  added to the options for the existing state's semantic value.
 */
static YYRESULTTAG
yyglrReduce (yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule,
             yybool yyforceEval)
{
  size_t yyposn = yystackp->yytops.yystates[yyk]->yyposn;

  if (yyforceEval || yystackp->yysplitPoint == YY_NULLPTR)
    {
      YYSTYPE yysval;

      YYRESULTTAG yyflag = yydoAction (yystackp, yyk, yyrule, &yysval);
      if (yyflag == yyerr && yystackp->yysplitPoint != YY_NULLPTR)
        {
          YYDPRINTF ((stderr, "Parse on stack %lu rejected by rule #%d.\n",
                     (unsigned long int) yyk, yyrule - 1));
        }
      if (yyflag != yyok)
        return yyflag;
      YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyrule], &yysval, &yyloc);
      yyglrShift (yystackp, yyk,
                  yyLRgotoState (yystackp->yytops.yystates[yyk]->yylrState,
                                 yylhsNonterm (yyrule)),
                  yyposn, &yysval);
    }
  else
    {
      size_t yyi;
      int yyn;
      yyGLRState* yys, *yys0 = yystackp->yytops.yystates[yyk];
      yyStateNum yynewLRState;

      for (yys = yystackp->yytops.yystates[yyk], yyn = yyrhsLength (yyrule);
           0 < yyn; yyn -= 1)
        {
          yys = yys->yypred;
          YYASSERT (yys);
        }
      yyupdateSplit (yystackp, yys);
      yynewLRState = yyLRgotoState (yys->yylrState, yylhsNonterm (yyrule));
      YYDPRINTF ((stderr,
                  "Reduced stack %lu by rule #%d; action deferred.  "
                  "Now in state %d.\n",
                  (unsigned long int) yyk, yyrule - 1, yynewLRState));
      for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
        if (yyi != yyk && yystackp->yytops.yystates[yyi] != YY_NULLPTR)
          {
            yyGLRState *yysplit = yystackp->yysplitPoint;
            yyGLRState *yyp = yystackp->yytops.yystates[yyi];
            while (yyp != yys && yyp != yysplit && yyp->yyposn >= yyposn)
              {
                if (yyp->yylrState == yynewLRState && yyp->yypred == yys)
                  {
                    yyaddDeferredAction (yystackp, yyk, yyp, yys0, yyrule);
                    yymarkStackDeleted (yystackp, yyk);
                    YYDPRINTF ((stderr, "Merging stack %lu into stack %lu.\n",
                                (unsigned long int) yyk,
                                (unsigned long int) yyi));
                    return yyok;
                  }
                yyp = yyp->yypred;
              }
          }
      yystackp->yytops.yystates[yyk] = yys;
      yyglrShiftDefer (yystackp, yyk, yynewLRState, yyposn, yys0, yyrule);
    }
  return yyok;
}

static size_t
yysplitStack (yyGLRStack* yystackp, size_t yyk)
{
  if (yystackp->yysplitPoint == YY_NULLPTR)
    {
      YYASSERT (yyk == 0);
      yystackp->yysplitPoint = yystackp->yytops.yystates[yyk];
    }
  if (yystackp->yytops.yysize >= yystackp->yytops.yycapacity)
    {
      yyGLRState** yynewStates;
      yybool* yynewLookaheadNeeds;

      /* yynewStates = YY_NULLPTR; */

      if (yystackp->yytops.yycapacity
          > (YYSIZEMAX / (2 * sizeof (yyGLRState*))))
        yyMemoryExhausted (yystackp);
      yystackp->yytops.yycapacity *= 2;

      yynewStates =
        (yyGLRState**) YYREALLOC (yystackp->yytops.yystates,
                                  (yystackp->yytops.yycapacity
                                   * sizeof (yyGLRState*)));
      if (yynewStates == YY_NULLPTR)
        yyMemoryExhausted (yystackp);
      yystackp->yytops.yystates = yynewStates;

      yynewLookaheadNeeds =
        (yybool*) YYREALLOC (yystackp->yytops.yylookaheadNeeds,
                             (yystackp->yytops.yycapacity
                              * sizeof yynewLookaheadNeeds[0]));
      if (yynewLookaheadNeeds == YY_NULLPTR)
        yyMemoryExhausted (yystackp);
      yystackp->yytops.yylookaheadNeeds = yynewLookaheadNeeds;
    }
  yystackp->yytops.yystates[yystackp->yytops.yysize]
    = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yylookaheadNeeds[yystackp->yytops.yysize]
    = yystackp->yytops.yylookaheadNeeds[yyk];
  yystackp->yytops.yysize += 1;
  return yystackp->yytops.yysize-1;
}

/** True iff YYY0 and YYY1 represent identical options at the top level.
 *  That is, they represent the same rule applied to RHS symbols
 *  that produce the same terminal symbols.  */
static yybool
yyidenticalOptions (yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  if (yyy0->yyrule == yyy1->yyrule)
    {
      yyGLRState *yys0, *yys1;
      int yyn;
      for (yys0 = yyy0->yystate, yys1 = yyy1->yystate,
           yyn = yyrhsLength (yyy0->yyrule);
           yyn > 0;
           yys0 = yys0->yypred, yys1 = yys1->yypred, yyn -= 1)
        if (yys0->yyposn != yys1->yyposn)
          return yyfalse;
      return yytrue;
    }
  else
    return yyfalse;
}

/** Assuming identicalOptions (YYY0,YYY1), destructively merge the
 *  alternative semantic values for the RHS-symbols of YYY1 and YYY0.  */
static void
yymergeOptionSets (yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  yyGLRState *yys0, *yys1;
  int yyn;
  for (yys0 = yyy0->yystate, yys1 = yyy1->yystate,
       yyn = yyrhsLength (yyy0->yyrule);
       yyn > 0;
       yys0 = yys0->yypred, yys1 = yys1->yypred, yyn -= 1)
    {
      if (yys0 == yys1)
        break;
      else if (yys0->yyresolved)
        {
          yys1->yyresolved = yytrue;
          yys1->yysemantics.yysval = yys0->yysemantics.yysval;
        }
      else if (yys1->yyresolved)
        {
          yys0->yyresolved = yytrue;
          yys0->yysemantics.yysval = yys1->yysemantics.yysval;
        }
      else
        {
          yySemanticOption** yyz0p = &yys0->yysemantics.yyfirstVal;
          yySemanticOption* yyz1 = yys1->yysemantics.yyfirstVal;
          while (yytrue)
            {
              if (yyz1 == *yyz0p || yyz1 == YY_NULLPTR)
                break;
              else if (*yyz0p == YY_NULLPTR)
                {
                  *yyz0p = yyz1;
                  break;
                }
              else if (*yyz0p < yyz1)
                {
                  yySemanticOption* yyz = *yyz0p;
                  *yyz0p = yyz1;
                  yyz1 = yyz1->yynext;
                  (*yyz0p)->yynext = yyz;
                }
              yyz0p = &(*yyz0p)->yynext;
            }
          yys1->yysemantics.yyfirstVal = yys0->yysemantics.yyfirstVal;
        }
    }
}

/** Y0 and Y1 represent two possible actions to take in a given
 *  parsing state; return 0 if no combination is possible,
 *  1 if user-mergeable, 2 if Y0 is preferred, 3 if Y1 is preferred.  */
static int
yypreference (yySemanticOption* y0, yySemanticOption* y1)
{
  yyRuleNum r0 = y0->yyrule, r1 = y1->yyrule;
  int p0 = yydprec[r0], p1 = yydprec[r1];

  if (p0 == p1)
    {
      if (yymerger[r0] == 0 || yymerger[r0] != yymerger[r1])
        return 0;
      else
        return 1;
    }
  if (p0 == 0 || p1 == 0)
    return 0;
  if (p0 < p1)
    return 3;
  if (p1 < p0)
    return 2;
  return 0;
}

static YYRESULTTAG yyresolveValue (yyGLRState* yys,
                                   yyGLRStack* yystackp);


/** Resolve the previous YYN states starting at and including state YYS
 *  on *YYSTACKP. If result != yyok, some states may have been left
 *  unresolved possibly with empty semantic option chains.  Regardless
 *  of whether result = yyok, each state has been left with consistent
 *  data so that yydestroyGLRState can be invoked if necessary.  */
static YYRESULTTAG
yyresolveStates (yyGLRState* yys, int yyn,
                 yyGLRStack* yystackp)
{
  if (0 < yyn)
    {
      YYASSERT (yys->yypred);
      YYCHK (yyresolveStates (yys->yypred, yyn-1, yystackp));
      if (! yys->yyresolved)
        YYCHK (yyresolveValue (yys, yystackp));
    }
  return yyok;
}

/** Resolve the states for the RHS of YYOPT on *YYSTACKP, perform its
 *  user action, and return the semantic value and location in *YYVALP
 *  and *YYLOCP.  Regardless of whether result = yyok, all RHS states
 *  have been destroyed (assuming the user action destroys all RHS
 *  semantic values if invoked).  */
static YYRESULTTAG
yyresolveAction (yySemanticOption* yyopt, yyGLRStack* yystackp,
                 YYSTYPE* yyvalp)
{
  yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
  int yynrhs = yyrhsLength (yyopt->yyrule);
  YYRESULTTAG yyflag =
    yyresolveStates (yyopt->yystate, yynrhs, yystackp);
  if (yyflag != yyok)
    {
      yyGLRState *yys;
      for (yys = yyopt->yystate; yynrhs > 0; yys = yys->yypred, yynrhs -= 1)
        yydestroyGLRState ("Cleanup: popping", yys);
      return yyflag;
    }

  yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred = yyopt->yystate;
  {
    int yychar_current = yychar;
    YYSTYPE yylval_current = yylval;
    yychar = yyopt->yyrawchar;
    yylval = yyopt->yyval;
    yyflag = yyuserAction (yyopt->yyrule, yynrhs,
                           yyrhsVals + YYMAXRHS + YYMAXLEFT - 1,
                           yystackp, yyvalp);
    yychar = yychar_current;
    yylval = yylval_current;
  }
  return yyflag;
}

#if YYDEBUG
static void
yyreportTree (yySemanticOption* yyx, int yyindent)
{
  int yynrhs = yyrhsLength (yyx->yyrule);
  int yyi;
  yyGLRState* yys;
  yyGLRState* yystates[1 + YYMAXRHS];
  yyGLRState yyleftmost_state;

  for (yyi = yynrhs, yys = yyx->yystate; 0 < yyi; yyi -= 1, yys = yys->yypred)
    yystates[yyi] = yys;
  if (yys == YY_NULLPTR)
    {
      yyleftmost_state.yyposn = 0;
      yystates[0] = &yyleftmost_state;
    }
  else
    yystates[0] = yys;

  if (yyx->yystate->yyposn < yys->yyposn + 1)
    YYFPRINTF (stderr, "%*s%s -> <Rule %d, empty>\n",
               yyindent, "", yytokenName (yylhsNonterm (yyx->yyrule)),
               yyx->yyrule - 1);
  else
    YYFPRINTF (stderr, "%*s%s -> <Rule %d, tokens %lu .. %lu>\n",
               yyindent, "", yytokenName (yylhsNonterm (yyx->yyrule)),
               yyx->yyrule - 1, (unsigned long int) (yys->yyposn + 1),
               (unsigned long int) yyx->yystate->yyposn);
  for (yyi = 1; yyi <= yynrhs; yyi += 1)
    {
      if (yystates[yyi]->yyresolved)
        {
          if (yystates[yyi-1]->yyposn+1 > yystates[yyi]->yyposn)
            YYFPRINTF (stderr, "%*s%s <empty>\n", yyindent+2, "",
                       yytokenName (yystos[yystates[yyi]->yylrState]));
          else
            YYFPRINTF (stderr, "%*s%s <tokens %lu .. %lu>\n", yyindent+2, "",
                       yytokenName (yystos[yystates[yyi]->yylrState]),
                       (unsigned long int) (yystates[yyi-1]->yyposn + 1),
                       (unsigned long int) yystates[yyi]->yyposn);
        }
      else
        yyreportTree (yystates[yyi]->yysemantics.yyfirstVal, yyindent+2);
    }
}
#endif

static YYRESULTTAG
yyreportAmbiguity (yySemanticOption* yyx0,
                   yySemanticOption* yyx1)
{
  YYUSE (yyx0);
  YYUSE (yyx1);

#if YYDEBUG
  YYFPRINTF (stderr, "Ambiguity detected.\n");
  YYFPRINTF (stderr, "Option 1,\n");
  yyreportTree (yyx0, 2);
  YYFPRINTF (stderr, "\nOption 2,\n");
  yyreportTree (yyx1, 2);
  YYFPRINTF (stderr, "\n");
#endif

  yyerror (YY_("syntax is ambiguous"));
  return yyabort;
}

/** Resolve the ambiguity represented in state YYS in *YYSTACKP,
 *  perform the indicated actions, and set the semantic value of YYS.
 *  If result != yyok, the chain of semantic options in YYS has been
 *  cleared instead or it has been left unmodified except that
 *  redundant options may have been removed.  Regardless of whether
 *  result = yyok, YYS has been left with consistent data so that
 *  yydestroyGLRState can be invoked if necessary.  */
static YYRESULTTAG
yyresolveValue (yyGLRState* yys, yyGLRStack* yystackp)
{
  yySemanticOption* yyoptionList = yys->yysemantics.yyfirstVal;
  yySemanticOption* yybest = yyoptionList;
  yySemanticOption** yypp;
  yybool yymerge = yyfalse;
  YYSTYPE yysval;
  YYRESULTTAG yyflag;

  for (yypp = &yyoptionList->yynext; *yypp != YY_NULLPTR; )
    {
      yySemanticOption* yyp = *yypp;

      if (yyidenticalOptions (yybest, yyp))
        {
          yymergeOptionSets (yybest, yyp);
          *yypp = yyp->yynext;
        }
      else
        {
          switch (yypreference (yybest, yyp))
            {
            case 0:
              return yyreportAmbiguity (yybest, yyp);
              /* break; */
            case 1:
              yymerge = yytrue;
              break;
            case 2:
              break;
            case 3:
              yybest = yyp;
              yymerge = yyfalse;
              break;
            default:
              /* This cannot happen so it is not worth a YYASSERT (yyfalse),
                 but some compilers complain if the default case is
                 omitted.  */
              break;
            }
          yypp = &yyp->yynext;
        }
    }

  if (yymerge)
    {
      yySemanticOption* yyp;
      int yyprec = yydprec[yybest->yyrule];
      yyflag = yyresolveAction (yybest, yystackp, &yysval);
      if (yyflag == yyok)
        for (yyp = yybest->yynext; yyp != YY_NULLPTR; yyp = yyp->yynext)
          {
            if (yyprec == yydprec[yyp->yyrule])
              {
                YYSTYPE yysval_other;
                yyflag = yyresolveAction (yyp, yystackp, &yysval_other);
                if (yyflag != yyok)
                  {
                    yydestruct ("Cleanup: discarding incompletely merged value for",
                                yystos[yys->yylrState],
                                &yysval);
                    break;
                  }
                yyuserMerge (yymerger[yyp->yyrule], &yysval, &yysval_other);
              }
          }
    }
  else
    yyflag = yyresolveAction (yybest, yystackp, &yysval);

  if (yyflag == yyok)
    {
      yys->yyresolved = yytrue;
      yys->yysemantics.yysval = yysval;
    }
  else
    yys->yysemantics.yyfirstVal = YY_NULLPTR;
  return yyflag;
}

static YYRESULTTAG
yyresolveStack (yyGLRStack* yystackp)
{
  if (yystackp->yysplitPoint != YY_NULLPTR)
    {
      yyGLRState* yys;
      int yyn;

      for (yyn = 0, yys = yystackp->yytops.yystates[0];
           yys != yystackp->yysplitPoint;
           yys = yys->yypred, yyn += 1)
        continue;
      YYCHK (yyresolveStates (yystackp->yytops.yystates[0], yyn, yystackp
                             ));
    }
  return yyok;
}

static void
yycompressStack (yyGLRStack* yystackp)
{
  yyGLRState* yyp, *yyq, *yyr;

  if (yystackp->yytops.yysize != 1 || yystackp->yysplitPoint == YY_NULLPTR)
    return;

  for (yyp = yystackp->yytops.yystates[0], yyq = yyp->yypred, yyr = YY_NULLPTR;
       yyp != yystackp->yysplitPoint;
       yyr = yyp, yyp = yyq, yyq = yyp->yypred)
    yyp->yypred = yyr;

  yystackp->yyspaceLeft += yystackp->yynextFree - yystackp->yyitems;
  yystackp->yynextFree = ((yyGLRStackItem*) yystackp->yysplitPoint) + 1;
  yystackp->yyspaceLeft -= yystackp->yynextFree - yystackp->yyitems;
  yystackp->yysplitPoint = YY_NULLPTR;
  yystackp->yylastDeleted = YY_NULLPTR;

  while (yyr != YY_NULLPTR)
    {
      yystackp->yynextFree->yystate = *yyr;
      yyr = yyr->yypred;
      yystackp->yynextFree->yystate.yypred = &yystackp->yynextFree[-1].yystate;
      yystackp->yytops.yystates[0] = &yystackp->yynextFree->yystate;
      yystackp->yynextFree += 1;
      yystackp->yyspaceLeft -= 1;
    }
}

static YYRESULTTAG
yyprocessOneStack (yyGLRStack* yystackp, size_t yyk,
                   size_t yyposn)
{
  while (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
    {
      yyStateNum yystate = yystackp->yytops.yystates[yyk]->yylrState;
      YYDPRINTF ((stderr, "Stack %lu Entering state %d\n",
                  (unsigned long int) yyk, yystate));

      YYASSERT (yystate != YYFINAL);

      if (yyisDefaultedState (yystate))
        {
          YYRESULTTAG yyflag;
          yyRuleNum yyrule = yydefaultAction (yystate);
          if (yyrule == 0)
            {
              YYDPRINTF ((stderr, "Stack %lu dies.\n",
                          (unsigned long int) yyk));
              yymarkStackDeleted (yystackp, yyk);
              return yyok;
            }
          yyflag = yyglrReduce (yystackp, yyk, yyrule, yyimmediate[yyrule]);
          if (yyflag == yyerr)
            {
              YYDPRINTF ((stderr,
                          "Stack %lu dies "
                          "(predicate failure or explicit user error).\n",
                          (unsigned long int) yyk));
              yymarkStackDeleted (yystackp, yyk);
              return yyok;
            }
          if (yyflag != yyok)
            return yyflag;
        }
      else
        {
          yySymbol yytoken;
          int yyaction;
          const short int* yyconflicts;

          yystackp->yytops.yylookaheadNeeds[yyk] = yytrue;
          if (yychar == YYEMPTY)
            {
              YYDPRINTF ((stderr, "Reading a token: "));
              yychar = yylex ();
            }

          if (yychar <= YYEOF)
            {
              yychar = yytoken = YYEOF;
              YYDPRINTF ((stderr, "Now at end of input.\n"));
            }
          else
            {
              yytoken = YYTRANSLATE (yychar);
              YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
            }

          yygetLRActions (yystate, yytoken, &yyaction, &yyconflicts);

          while (*yyconflicts != 0)
            {
              YYRESULTTAG yyflag;
              size_t yynewStack = yysplitStack (yystackp, yyk);
              YYDPRINTF ((stderr, "Splitting off stack %lu from %lu.\n",
                          (unsigned long int) yynewStack,
                          (unsigned long int) yyk));
              yyflag = yyglrReduce (yystackp, yynewStack,
                                    *yyconflicts,
                                    yyimmediate[*yyconflicts]);
              if (yyflag == yyok)
                YYCHK (yyprocessOneStack (yystackp, yynewStack,
                                          yyposn));
              else if (yyflag == yyerr)
                {
                  YYDPRINTF ((stderr, "Stack %lu dies.\n",
                              (unsigned long int) yynewStack));
                  yymarkStackDeleted (yystackp, yynewStack);
                }
              else
                return yyflag;
              yyconflicts += 1;
            }

          if (yyisShiftAction (yyaction))
            break;
          else if (yyisErrorAction (yyaction))
            {
              YYDPRINTF ((stderr, "Stack %lu dies.\n",
                          (unsigned long int) yyk));
              yymarkStackDeleted (yystackp, yyk);
              break;
            }
          else
            {
              YYRESULTTAG yyflag = yyglrReduce (yystackp, yyk, -yyaction,
                                                yyimmediate[-yyaction]);
              if (yyflag == yyerr)
                {
                  YYDPRINTF ((stderr,
                              "Stack %lu dies "
                              "(predicate failure or explicit user error).\n",
                              (unsigned long int) yyk));
                  yymarkStackDeleted (yystackp, yyk);
                  break;
                }
              else if (yyflag != yyok)
                return yyflag;
            }
        }
    }
  return yyok;
}

static void
yyreportSyntaxError (yyGLRStack* yystackp)
{
  if (yystackp->yyerrState != 0)
    return;
#if ! YYERROR_VERBOSE
  yyerror (YY_("syntax error"));
#else
  {
  yySymbol yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);
  size_t yysize0 = yytnamerr (YY_NULLPTR, yytokenName (yytoken));
  size_t yysize = yysize0;
  yybool yysize_overflow = yyfalse;
  char* yymsg = YY_NULLPTR;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected").  */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[yystackp->yytops.yystates[0]->yylrState];
      yyarg[yycount++] = yytokenName (yytoken);
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for this
             state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;
          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytokenName (yyx);
                {
                  size_t yysz = yysize + yytnamerr (YY_NULLPTR, yytokenName (yyx));
                  yysize_overflow |= yysz < yysize;
                  yysize = yysz;
                }
              }
        }
    }

  switch (yycount)
    {
#define YYCASE_(N, S)                   \
      case N:                           \
        yyformat = S;                   \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  {
    size_t yysz = yysize + strlen (yyformat);
    yysize_overflow |= yysz < yysize;
    yysize = yysz;
  }

  if (!yysize_overflow)
    yymsg = (char *) YYMALLOC (yysize);

  if (yymsg)
    {
      char *yyp = yymsg;
      int yyi = 0;
      while ((*yyp = *yyformat))
        {
          if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
            {
              yyp += yytnamerr (yyp, yyarg[yyi++]);
              yyformat += 2;
            }
          else
            {
              yyp++;
              yyformat++;
            }
        }
      yyerror (yymsg);
      YYFREE (yymsg);
    }
  else
    {
      yyerror (YY_("syntax error"));
      yyMemoryExhausted (yystackp);
    }
  }
#endif /* YYERROR_VERBOSE */
  yynerrs += 1;
}

/* Recover from a syntax error on *YYSTACKP, assuming that *YYSTACKP->YYTOKENP,
   yylval, and yylloc are the syntactic category, semantic value, and location
   of the lookahead.  */
static void
yyrecoverSyntaxError (yyGLRStack* yystackp)
{
  size_t yyk;
  int yyj;

  if (yystackp->yyerrState == 3)
    /* We just shifted the error token and (perhaps) took some
       reductions.  Skip tokens until we can proceed.  */
    while (yytrue)
      {
        yySymbol yytoken;
        if (yychar == YYEOF)
          yyFail (yystackp, YY_NULLPTR);
        if (yychar != YYEMPTY)
          {
            yytoken = YYTRANSLATE (yychar);
            yydestruct ("Error: discarding",
                        yytoken, &yylval);
          }
        YYDPRINTF ((stderr, "Reading a token: "));
        yychar = yylex ();
        if (yychar <= YYEOF)
          {
            yychar = yytoken = YYEOF;
            YYDPRINTF ((stderr, "Now at end of input.\n"));
          }
        else
          {
            yytoken = YYTRANSLATE (yychar);
            YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
          }
        yyj = yypact[yystackp->yytops.yystates[0]->yylrState];
        if (yypact_value_is_default (yyj))
          return;
        yyj += yytoken;
        if (yyj < 0 || YYLAST < yyj || yycheck[yyj] != yytoken)
          {
            if (yydefact[yystackp->yytops.yystates[0]->yylrState] != 0)
              return;
          }
        else if (! yytable_value_is_error (yytable[yyj]))
          return;
      }

  /* Reduce to one stack.  */
  for (yyk = 0; yyk < yystackp->yytops.yysize; yyk += 1)
    if (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
      break;
  if (yyk >= yystackp->yytops.yysize)
    yyFail (yystackp, YY_NULLPTR);
  for (yyk += 1; yyk < yystackp->yytops.yysize; yyk += 1)
    yymarkStackDeleted (yystackp, yyk);
  yyremoveDeletes (yystackp);
  yycompressStack (yystackp);

  /* Now pop stack until we find a state that shifts the error token.  */
  yystackp->yyerrState = 3;
  while (yystackp->yytops.yystates[0] != YY_NULLPTR)
    {
      yyGLRState *yys = yystackp->yytops.yystates[0];
      yyj = yypact[yys->yylrState];
      if (! yypact_value_is_default (yyj))
        {
          yyj += YYTERROR;
          if (0 <= yyj && yyj <= YYLAST && yycheck[yyj] == YYTERROR
              && yyisShiftAction (yytable[yyj]))
            {
              /* Shift the error token.  */
              YY_SYMBOL_PRINT ("Shifting", yystos[yytable[yyj]],
                               &yylval, &yyerrloc);
              yyglrShift (yystackp, 0, yytable[yyj],
                          yys->yyposn, &yylval);
              yys = yystackp->yytops.yystates[0];
              break;
            }
        }
      if (yys->yypred != YY_NULLPTR)
        yydestroyGLRState ("Error: popping", yys);
      yystackp->yytops.yystates[0] = yys->yypred;
      yystackp->yynextFree -= 1;
      yystackp->yyspaceLeft += 1;
    }
  if (yystackp->yytops.yystates[0] == YY_NULLPTR)
    yyFail (yystackp, YY_NULLPTR);
}

#define YYCHK1(YYE)                                                          \
  do {                                                                       \
    switch (YYE) {                                                           \
    case yyok:                                                               \
      break;                                                                 \
    case yyabort:                                                            \
      goto yyabortlab;                                                       \
    case yyaccept:                                                           \
      goto yyacceptlab;                                                      \
    case yyerr:                                                              \
      goto yyuser_error;                                                     \
    default:                                                                 \
      goto yybuglab;                                                         \
    }                                                                        \
  } while (0)

/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
  int yyresult;
  yyGLRStack yystack;
  yyGLRStack* const yystackp = &yystack;
  size_t yyposn;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY;
  yylval = yyval_default;

  if (! yyinitGLRStack (yystackp, YYINITDEPTH))
    goto yyexhaustedlab;
  switch (YYSETJMP (yystack.yyexception_buffer))
    {
    case 0: break;
    case 1: goto yyabortlab;
    case 2: goto yyexhaustedlab;
    default: goto yybuglab;
    }
  yyglrShift (&yystack, 0, 0, 0, &yylval);
  yyposn = 0;

  while (yytrue)
    {
      /* For efficiency, we have two loops, the first of which is
         specialized to deterministic operation (single stack, no
         potential ambiguity).  */
      /* Standard mode */
      while (yytrue)
        {
          yyRuleNum yyrule;
          int yyaction;
          const short int* yyconflicts;

          yyStateNum yystate = yystack.yytops.yystates[0]->yylrState;
          YYDPRINTF ((stderr, "Entering state %d\n", yystate));
          if (yystate == YYFINAL)
            goto yyacceptlab;
          if (yyisDefaultedState (yystate))
            {
              yyrule = yydefaultAction (yystate);
              if (yyrule == 0)
                {

                  yyreportSyntaxError (&yystack);
                  goto yyuser_error;
                }
              YYCHK1 (yyglrReduce (&yystack, 0, yyrule, yytrue));
            }
          else
            {
              yySymbol yytoken;
              if (yychar == YYEMPTY)
                {
                  YYDPRINTF ((stderr, "Reading a token: "));
                  yychar = yylex ();
                }

              if (yychar <= YYEOF)
                {
                  yychar = yytoken = YYEOF;
                  YYDPRINTF ((stderr, "Now at end of input.\n"));
                }
              else
                {
                  yytoken = YYTRANSLATE (yychar);
                  YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
                }

              yygetLRActions (yystate, yytoken, &yyaction, &yyconflicts);
              if (*yyconflicts != 0)
                break;
              if (yyisShiftAction (yyaction))
                {
                  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
                  yychar = YYEMPTY;
                  yyposn += 1;
                  yyglrShift (&yystack, 0, yyaction, yyposn, &yylval);
                  if (0 < yystack.yyerrState)
                    yystack.yyerrState -= 1;
                }
              else if (yyisErrorAction (yyaction))
                {

                  yyreportSyntaxError (&yystack);
                  goto yyuser_error;
                }
              else
                YYCHK1 (yyglrReduce (&yystack, 0, -yyaction, yytrue));
            }
        }

      while (yytrue)
        {
          yySymbol yytoken_to_shift;
          size_t yys;

          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            yystackp->yytops.yylookaheadNeeds[yys] = yychar != YYEMPTY;

          /* yyprocessOneStack returns one of three things:

              - An error flag.  If the caller is yyprocessOneStack, it
                immediately returns as well.  When the caller is finally
                yyparse, it jumps to an error label via YYCHK1.

              - yyok, but yyprocessOneStack has invoked yymarkStackDeleted
                (&yystack, yys), which sets the top state of yys to NULL.  Thus,
                yyparse's following invocation of yyremoveDeletes will remove
                the stack.

              - yyok, when ready to shift a token.

             Except in the first case, yyparse will invoke yyremoveDeletes and
             then shift the next token onto all remaining stacks.  This
             synchronization of the shift (that is, after all preceding
             reductions on all stacks) helps prevent double destructor calls
             on yylval in the event of memory exhaustion.  */

          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            YYCHK1 (yyprocessOneStack (&yystack, yys, yyposn));
          yyremoveDeletes (&yystack);
          if (yystack.yytops.yysize == 0)
            {
              yyundeleteLastStack (&yystack);
              if (yystack.yytops.yysize == 0)
                yyFail (&yystack, YY_("syntax error"));
              YYCHK1 (yyresolveStack (&yystack));
              YYDPRINTF ((stderr, "Returning to deterministic operation.\n"));

              yyreportSyntaxError (&yystack);
              goto yyuser_error;
            }

          /* If any yyglrShift call fails, it will fail after shifting.  Thus,
             a copy of yylval will already be on stack 0 in the event of a
             failure in the following loop.  Thus, yychar is set to YYEMPTY
             before the loop to make sure the user destructor for yylval isn't
             called twice.  */
          yytoken_to_shift = YYTRANSLATE (yychar);
          yychar = YYEMPTY;
          yyposn += 1;
          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            {
              int yyaction;
              const short int* yyconflicts;
              yyStateNum yystate = yystack.yytops.yystates[yys]->yylrState;
              yygetLRActions (yystate, yytoken_to_shift, &yyaction,
                              &yyconflicts);
              /* Note that yyconflicts were handled by yyprocessOneStack.  */
              YYDPRINTF ((stderr, "On stack %lu, ", (unsigned long int) yys));
              YY_SYMBOL_PRINT ("shifting", yytoken_to_shift, &yylval, &yylloc);
              yyglrShift (&yystack, yys, yyaction, yyposn,
                          &yylval);
              YYDPRINTF ((stderr, "Stack %lu now in state #%d\n",
                          (unsigned long int) yys,
                          yystack.yytops.yystates[yys]->yylrState));
            }

          if (yystack.yytops.yysize == 1)
            {
              YYCHK1 (yyresolveStack (&yystack));
              YYDPRINTF ((stderr, "Returning to deterministic operation.\n"));
              yycompressStack (&yystack);
              break;
            }
        }
      continue;
    yyuser_error:
      yyrecoverSyntaxError (&yystack);
      yyposn = yystack.yytops.yystates[0]->yyposn;
    }

 yyacceptlab:
  yyresult = 0;
  goto yyreturn;

 yybuglab:
  YYASSERT (yyfalse);
  goto yyabortlab;

 yyabortlab:
  yyresult = 1;
  goto yyreturn;

 yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;

 yyreturn:
  if (yychar != YYEMPTY)
    yydestruct ("Cleanup: discarding lookahead",
                YYTRANSLATE (yychar), &yylval);

  /* If the stack is well-formed, pop the stack until it is empty,
     destroying its entries as we go.  But free the stack regardless
     of whether it is well-formed.  */
  if (yystack.yyitems)
    {
      yyGLRState** yystates = yystack.yytops.yystates;
      if (yystates)
        {
          size_t yysize = yystack.yytops.yysize;
          size_t yyk;
          for (yyk = 0; yyk < yysize; yyk += 1)
            if (yystates[yyk])
              {
                while (yystates[yyk])
                  {
                    yyGLRState *yys = yystates[yyk];
                  if (yys->yypred != YY_NULLPTR)
                      yydestroyGLRState ("Cleanup: popping", yys);
                    yystates[yyk] = yys->yypred;
                    yystack.yynextFree -= 1;
                    yystack.yyspaceLeft += 1;
                  }
                break;
              }
        }
      yyfreeGLRStack (&yystack);
    }

  return yyresult;
}

/* DEBUGGING ONLY */
#if YYDEBUG
static void
yy_yypstack (yyGLRState* yys)
{
  if (yys->yypred)
    {
      yy_yypstack (yys->yypred);
      YYFPRINTF (stderr, " -> ");
    }
  YYFPRINTF (stderr, "%d@%lu", yys->yylrState,
             (unsigned long int) yys->yyposn);
}

static void
yypstates (yyGLRState* yyst)
{
  if (yyst == YY_NULLPTR)
    YYFPRINTF (stderr, "<null>");
  else
    yy_yypstack (yyst);
  YYFPRINTF (stderr, "\n");
}

static void
yypstack (yyGLRStack* yystackp, size_t yyk)
{
  yypstates (yystackp->yytops.yystates[yyk]);
}

#define YYINDEX(YYX)                                                         \
    ((YYX) == YY_NULLPTR ? -1 : (yyGLRStackItem*) (YYX) - yystackp->yyitems)


static void
yypdumpstack (yyGLRStack* yystackp)
{
  yyGLRStackItem* yyp;
  size_t yyi;
  for (yyp = yystackp->yyitems; yyp < yystackp->yynextFree; yyp += 1)
    {
      YYFPRINTF (stderr, "%3lu. ",
                 (unsigned long int) (yyp - yystackp->yyitems));
      if (*(yybool *) yyp)
        {
          YYASSERT (yyp->yystate.yyisState);
          YYASSERT (yyp->yyoption.yyisState);
          YYFPRINTF (stderr, "Res: %d, LR State: %d, posn: %lu, pred: %ld",
                     yyp->yystate.yyresolved, yyp->yystate.yylrState,
                     (unsigned long int) yyp->yystate.yyposn,
                     (long int) YYINDEX (yyp->yystate.yypred));
          if (! yyp->yystate.yyresolved)
            YYFPRINTF (stderr, ", firstVal: %ld",
                       (long int) YYINDEX (yyp->yystate
                                             .yysemantics.yyfirstVal));
        }
      else
        {
          YYASSERT (!yyp->yystate.yyisState);
          YYASSERT (!yyp->yyoption.yyisState);
          YYFPRINTF (stderr, "Option. rule: %d, state: %ld, next: %ld",
                     yyp->yyoption.yyrule - 1,
                     (long int) YYINDEX (yyp->yyoption.yystate),
                     (long int) YYINDEX (yyp->yyoption.yynext));
        }
      YYFPRINTF (stderr, "\n");
    }
  YYFPRINTF (stderr, "Tops:");
  for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
    YYFPRINTF (stderr, "%lu: %ld; ", (unsigned long int) yyi,
               (long int) YYINDEX (yystackp->yytops.yystates[yyi]));
  YYFPRINTF (stderr, "\n");
}
#endif

#undef yylval
#undef yychar
#undef yynerrs





#include <string.h>
#include "lex.yy.c"

/* fill in the type name if none given */
const char *type_class(unsigned int type, const char *classname)
{
  if (classname)
    {
    if (classname[0] == '\0')
      {
      switch ((type & VTK_PARSE_BASE_TYPE))
        {
        case 0:
          classname = "auto";
          break;
        case VTK_PARSE_VOID:
          classname = "void";
          break;
        case VTK_PARSE_BOOL:
          classname = "bool";
          break;
        case VTK_PARSE_FLOAT:
          classname = "float";
          break;
        case VTK_PARSE_DOUBLE:
          classname = "double";
          break;
        case VTK_PARSE_LONG_DOUBLE:
          classname = "long double";
          break;
        case VTK_PARSE_CHAR:
          classname = "char";
          break;
        case VTK_PARSE_CHAR16_T:
          classname = "char16_t";
          break;
        case VTK_PARSE_CHAR32_T:
          classname = "char32_t";
          break;
        case VTK_PARSE_WCHAR_T:
          classname = "wchar_t";
          break;
        case VTK_PARSE_UNSIGNED_CHAR:
          classname = "unsigned char";
          break;
        case VTK_PARSE_SIGNED_CHAR:
          classname = "signed char";
          break;
        case VTK_PARSE_SHORT:
          classname = "short";
          break;
        case VTK_PARSE_UNSIGNED_SHORT:
          classname = "unsigned short";
          break;
        case VTK_PARSE_INT:
          classname = "int";
          break;
        case VTK_PARSE_UNSIGNED_INT:
          classname = "unsigned int";
          break;
        case VTK_PARSE_LONG:
          classname = "long";
          break;
        case VTK_PARSE_UNSIGNED_LONG:
          classname = "unsigned long";
          break;
        case VTK_PARSE_LONG_LONG:
          classname = "long long";
          break;
        case VTK_PARSE_UNSIGNED_LONG_LONG:
          classname = "unsigned long long";
          break;
        case VTK_PARSE___INT64:
          classname = "__int64";
          break;
        case VTK_PARSE_UNSIGNED___INT64:
          classname = "unsigned __int64";
          break;
        }
      }
    }

  return classname;
}

/* check whether this is the class we are looking for */
void start_class(const char *classname, int is_struct_or_union)
{
  ClassInfo *outerClass = currentClass;
  pushClass();
  currentClass = (ClassInfo *)malloc(sizeof(ClassInfo));
  vtkParse_InitClass(currentClass);
  currentClass->Name = classname;
  if (is_struct_or_union == 1)
    {
    currentClass->ItemType = VTK_STRUCT_INFO;
    }
  if (is_struct_or_union == 2)
    {
    currentClass->ItemType = VTK_UNION_INFO;
    }

  if (classname && classname[0] != '\0')
    {
    /* if name of class being defined contains "::" or "<..>", then skip it */
    const char *cp = classname;
    while (*cp != '\0' && *cp != ':' && *cp != '>')
      {
      cp++;
      }
    if (*cp == '\0')
      {
      if (outerClass)
        {
        vtkParse_AddClassToClass(outerClass, currentClass);
        }
      else
        {
        vtkParse_AddClassToNamespace(currentNamespace, currentClass);
        }
      }
    }

  /* template information */
  if (currentTemplate)
    {
    currentClass->Template = currentTemplate;
    currentTemplate = NULL;
    }

  /* comment, if any */
  currentClass->Comment = vtkstrdup(getComment());

  access_level = VTK_ACCESS_PRIVATE;
  if (is_struct_or_union)
    {
    access_level = VTK_ACCESS_PUBLIC;
    }

  vtkParse_InitFunction(currentFunction);
  startSig();
  clearComment();
}

/* reached the end of a class definition */
void end_class()
{
  /* add default constructors */
  vtkParse_AddDefaultConstructors(currentClass, data->Strings);

  popClass();
}

/* add a base class to the specified class */
void add_base_class(ClassInfo *cls, const char *name, int al,
  unsigned int extra)
{
  /* "extra" can contain VTK_PARSE_VIRTUAL and VTK_PARSE_PACK */
  if (cls && al == VTK_ACCESS_PUBLIC &&
      (extra & VTK_PARSE_VIRTUAL) == 0 &&
      (extra & VTK_PARSE_PACK) == 0)
    {
    vtkParse_AddStringToArray(&cls->SuperClasses,
                              &cls->NumberOfSuperClasses,
                              name);
    }
}

/* add a using declaration or directive */
void add_using(const char *name, int is_namespace)
{
  size_t i;
  UsingInfo *item;

  item = (UsingInfo *)malloc(sizeof(UsingInfo));
  vtkParse_InitUsing(item);
  if (is_namespace)
    {
    item->Name = NULL;
    item->Scope = name;
    }
  else
    {
    i = strlen(name);
    while (i > 0 && name[i-1] != ':') { i--; }
    item->Name = vtkstrdup(&name[i]);
    while (i > 0 && name[i-1] == ':') { i--; }
    item->Scope = vtkstrndup(name, i);
    }

  if (currentClass)
    {
    vtkParse_AddUsingToClass(currentClass, item);
    }
  else
    {
    vtkParse_AddUsingToNamespace(currentNamespace, item);
    }
}

/* start a new enum */
void start_enum(const char *name, int is_scoped,
                unsigned int type, const char *basename)
{
  EnumInfo *item;

  currentEnumType = (type ? type : VTK_PARSE_INT);
  currentEnumName = "int";
  currentEnumValue = NULL;

  if (type == 0 && is_scoped)
    {
    type = VTK_PARSE_INT;
    }

  if (name)
    {
    currentEnumName = name;
    item = (EnumInfo *)malloc(sizeof(EnumInfo));
    vtkParse_InitEnum(item);
    item->Name = name;
    item->Comment = vtkstrdup(getComment());
    item->Access = access_level;

    if (currentClass)
      {
      vtkParse_AddEnumToClass(currentClass, item);
      }
    else
      {
      vtkParse_AddEnumToNamespace(currentNamespace, item);
      }

    if (type)
      {
      vtkParse_AddStringToArray(&item->SuperClasses,
                                &item->NumberOfSuperClasses,
                                type_class(type, basename));
      }

    if (is_scoped)
      {
      pushClass();
      currentClass = item;
      }
    }
}

/* finish the enum */
void end_enum()
{
  if (currentClass && currentClass->ItemType == VTK_ENUM_INFO)
    {
    popClass();
    }

  currentEnumName = NULL;
  currentEnumValue = NULL;
}

/* add a constant to the enum */
void add_enum(const char *name, const char *value)
{
  static char text[2048];
  int i;
  long j;

  if (value)
    {
    strcpy(text, value);
    currentEnumValue = value;
    }
  else if (currentEnumValue)
    {
    i = strlen(text);
    while (i > 0 && text[i-1] >= '0' &&
           text[i-1] <= '9') { i--; }

    if (i == 0 || text[i-1] == ' ' ||
        (i > 1 && text[i-2] == ' ' &&
         (text[i-1] == '-' || text[i-1] == '+')))
      {
      if (i > 0 && text[i-1] != ' ')
        {
        i--;
        }
      j = (int)strtol(&text[i], NULL, 10);
      sprintf(&text[i], "%li", j+1);
      }
    else
      {
      i = strlen(text);
      strcpy(&text[i], " + 1");
      }
    currentEnumValue = vtkstrdup(text);
    }
  else
    {
    strcpy(text, "0");
    currentEnumValue = "0";
    }

  add_constant(name, currentEnumValue, currentEnumType, currentEnumName, 2);
}

/* for a macro constant, guess the constant type, doesn't do any math */
unsigned int guess_constant_type(const char *valstring)
{
  unsigned int valtype = 0;
  size_t k;
  int i;
  int is_name = 0;

  if (valstring == NULL || valstring[0] == '\0')
    {
    return 0;
    }

  k = vtkParse_SkipId(valstring);
  if (valstring[k] == '\0')
    {
    is_name = 1;
    }

  if (strcmp(valstring, "true") == 0 || strcmp(valstring, "false") == 0)
    {
    return VTK_PARSE_BOOL;
    }

  if (strcmp(valstring, "nullptr") == 0 || strcmp(valstring, "NULL") == 0)
    {
    return VTK_PARSE_NULLPTR_T;
    }

  if (valstring[0] == '\'')
    {
    return VTK_PARSE_CHAR;
    }

  if (strncmp(valstring, "VTK_TYPE_CAST(", 14) == 0 ||
      strncmp(valstring, "static_cast<", 12) == 0 ||
      strncmp(valstring, "const_cast<", 11) == 0 ||
      strncmp(valstring, "(", 1) == 0)
    {
    const char *cp;
    size_t n;
    int is_unsigned = 0;

    cp = &valstring[1];
    if (valstring[0] == 'c')
      {
      cp = &valstring[11];
      }
    else if (valstring[0] == 's')
      {
      cp = &valstring[12];
      }
    else if (valstring[0] == 'V')
      {
      cp = &valstring[14];
      }

    if (strncmp(cp, "unsigned ", 9) == 0)
      {
      is_unsigned = 1;
      cp += 9;
      }

    n = strlen(cp);
    for (k = 0; k < n && cp[k] != ',' &&
         cp[k] != '>' && cp[k] != ')'; k++) { ; };

    if (strncmp(cp, "long long", k) == 0)
      { valtype = VTK_PARSE_LONG_LONG; }
    else if (strncmp(cp, "__int64", k) == 0)
      { valtype = VTK_PARSE___INT64; }
    else if (strncmp(cp, "long", k) == 0)
      { valtype = VTK_PARSE_LONG; }
    else if (strncmp(cp, "short", k) == 0)
      { valtype = VTK_PARSE_SHORT; }
    else if (strncmp(cp, "signed char", k) == 0)
      { valtype = VTK_PARSE_SIGNED_CHAR; }
    else if (strncmp(cp, "char", k) == 0)
      { valtype = VTK_PARSE_CHAR; }
    else if (strncmp(cp, "int", k) == 0 ||
             strncmp(cp, "signed", k) == 0)
      { valtype = VTK_PARSE_INT; }
    else if (strncmp(cp, "float", k) == 0)
      { valtype = VTK_PARSE_FLOAT; }
    else if (strncmp(cp, "double", k) == 0)
      { valtype = VTK_PARSE_DOUBLE; }
    else if (strncmp(cp, "char *", k) == 0)
      { valtype = VTK_PARSE_CHAR_PTR; }

    if (is_unsigned)
      {
      if (valtype == 0) { valtype = VTK_PARSE_INT; }
      valtype = (valtype | VTK_PARSE_UNSIGNED);
      }

    if (valtype != 0)
      {
      return valtype;
      }
    }

  /* check the current scope */
  if (is_name)
    {
    NamespaceInfo *scope = currentNamespace;
    if (namespaceDepth > 0)
      {
      scope = namespaceStack[0];
      }

    for (i = 0; i < scope->NumberOfConstants; i++)
      {
      if (strcmp(scope->Constants[i]->Name, valstring) == 0)
        {
        return scope->Constants[i]->Type;
        }
      }
    }

  /* check for preprocessor macros */
  if (is_name)
    {
    MacroInfo *macro = vtkParsePreprocess_GetMacro(
      preprocessor, valstring);

    if (macro && !macro->IsFunction)
      {
      return guess_constant_type(macro->Definition);
      }
    }

  /* fall back to the preprocessor to evaluate the constant */
    {
    preproc_int_t val;
    int is_unsigned;
    int result = vtkParsePreprocess_EvaluateExpression(
      preprocessor, valstring, &val, &is_unsigned);

    if (result == VTK_PARSE_PREPROC_DOUBLE)
      {
      return VTK_PARSE_DOUBLE;
      }
    else if (result == VTK_PARSE_PREPROC_FLOAT)
      {
      return VTK_PARSE_FLOAT;
      }
    else if (result == VTK_PARSE_PREPROC_STRING)
      {
      return VTK_PARSE_CHAR_PTR;
      }
    else if (result == VTK_PARSE_OK)
      {
      if (is_unsigned)
        {
        if ((preproc_uint_t)val <= VTK_UNSIGNED_INT_MAX)
          {
          return VTK_PARSE_UNSIGNED_INT;
          }
        else
          {
          return VTK_PARSE_UNSIGNED_LONG_LONG;
          }
        }
      else
        {
        if (val >= VTK_INT_MIN && val <= VTK_INT_MAX)
          {
          return VTK_PARSE_INT;
          }
        else
          {
          return VTK_PARSE_LONG_LONG;
          }
        }
      }
    }

  return 0;
}

/* add a constant to the current class or namespace */
void add_constant(const char *name, const char *value,
                  unsigned int type, const char *typeclass, int flag)
{
  ValueInfo *con = (ValueInfo *)malloc(sizeof(ValueInfo));
  vtkParse_InitValue(con);
  con->ItemType = VTK_CONSTANT_INFO;
  con->Name = name;
  con->Comment = vtkstrdup(getComment());
  con->Value = value;
  con->Type = type;
  con->Class = type_class(type, typeclass);

  if (flag == 2)
    {
    con->IsEnum = 1;
    }

  if (flag == 1)
    {
    /* actually a macro, need to guess the type */
    ValueInfo **cptr = data->Contents->Constants;
    int n = data->Contents->NumberOfConstants;
    int i;

    con->Access = VTK_ACCESS_PUBLIC;
    if (con->Type == 0)
      {
      con->Type = guess_constant_type(con->Value);
      }

    for (i = 0; i < n; i++)
      {
      if (strcmp(cptr[i]->Name, con->Name) == 0)
        {
        break;
        }
      }

    if (i == n)
      {
      vtkParse_AddConstantToNamespace(data->Contents, con);
      }
    else
      {
      vtkParse_FreeValue(con);
      }
    }
  else if (currentClass)
    {
    con->Access = access_level;
    vtkParse_AddConstantToClass(currentClass, con);
    }
  else
    {
    con->Access = VTK_ACCESS_PUBLIC;
    vtkParse_AddConstantToNamespace(currentNamespace, con);
    }
}

/* if the name is a const in this namespace, the scope it */
const char *add_const_scope(const char *name)
{
  static char text[256];
  NamespaceInfo *scope = currentNamespace;
  int i, j;
  int addscope = 0;

  strcpy(text, name);

  if (currentClass)
    {
    for (j = 0; j < currentClass->NumberOfConstants; j++)
      {
      if (strcmp(currentClass->Constants[j]->Name, text) == 0)
        {
        prepend_scope(text, currentClass->Name);
        addscope = 1;
        }
      }
    }
  i = namespaceDepth;
  while (scope && scope->Name)
    {
    if (addscope)
      {
      prepend_scope(text, scope->Name);
      }
    else
      {
      for (j = 0; j < scope->NumberOfConstants; j++)
        {
        if (strcmp(scope->Constants[j]->Name, text) == 0)
          {
          prepend_scope(text, scope->Name);
          addscope = 1;
          }
        }
      }

    scope = 0;
    if (i > 0)
      {
      scope = namespaceStack[--i];
      }
    }

  return text;
}

/* guess the type from the ID */
unsigned int guess_id_type(const char *cp)
{
  unsigned int t = 0;

  if (cp)
    {
    size_t i;
    const char *dp;

    i = strlen(cp);
    while (i > 0 && cp[i-1] != ':') { i--; }
    dp = &cp[i];

    if (strcmp(dp, "vtkStdString") == 0 ||
        strcmp(cp, "std::string") == 0)
      {
      t = VTK_PARSE_STRING;
      }
    else if (strcmp(dp, "vtkUnicodeString") == 0)
      {
      t = VTK_PARSE_UNICODE_STRING;
      }
    else if (strncmp(dp, "vtk", 3) == 0)
      {
      t = VTK_PARSE_OBJECT;
      }
    else if (strncmp(dp, "Q", 1) == 0 ||
             strncmp(cp, "Qt::", 4) == 0)
      {
      t = VTK_PARSE_QOBJECT;
      }
    else
      {
      t = VTK_PARSE_UNKNOWN;
      }
    }

  return t;
}

/* add a template parameter to the current template */
void add_template_parameter(
  unsigned int datatype, unsigned int extra, const char *funcSig)
{
  ValueInfo *param = (ValueInfo *)malloc(sizeof(ValueInfo));
  vtkParse_InitValue(param);
  handle_complex_type(param, datatype, extra, funcSig);
  param->Name = getVarName();
  vtkParse_AddParameterToTemplate(currentTemplate, param);
}

/* add a parameter to a function */
void add_parameter(FunctionInfo *func, unsigned int type,
                   const char *typeclass, int count)
{
  char text[64];
  ValueInfo *param = (ValueInfo *)malloc(sizeof(ValueInfo));
  vtkParse_InitValue(param);

  param->Type = type;
  param->Class = type_class(type, typeclass);

  if (count)
    {
    param->Count = count;
    sprintf(text, "%i", count);
    vtkParse_AddStringToArray(&param->Dimensions, &param->NumberOfDimensions,
                              vtkstrdup(text));
    }

  add_legacy_parameter(func, param);

  vtkParse_AddParameterToFunction(func, param);
}

/* set the return type for the current function */
void set_return(FunctionInfo *func, unsigned int type,
                const char *typeclass, int count)
{
  char text[64];
  ValueInfo *val = (ValueInfo *)malloc(sizeof(ValueInfo));

  vtkParse_InitValue(val);
  val->Type = type;
  val->Class = type_class(type, typeclass);

  if (count)
    {
    val->Count = count;
    sprintf(text, "%i", count);
    vtkParse_AddStringToArray(&val->Dimensions, &val->NumberOfDimensions,
                              vtkstrdup(text));
    }

  func->ReturnValue = val;

#ifndef VTK_PARSE_LEGACY_REMOVE
  func->ReturnType = val->Type;
  func->ReturnClass = val->Class;
  func->HaveHint = (count > 0);
  func->HintSize = count;
#endif
}

int count_from_dimensions(ValueInfo *val)
{
  int count, i, n;
  const char *cp;

  /* count is the product of the dimensions */
  count = 0;
  if (val->NumberOfDimensions)
    {
    count = 1;
    for (i = 0; i < val->NumberOfDimensions; i++)
      {
      n = 0;
      cp = val->Dimensions[i];
      if (cp[0] != '\0')
        {
        while (*cp != '\0' && *cp >= '0' && *cp <= '9') { cp++; }
        while (*cp != '\0' && (*cp == 'u' || *cp == 'l' ||
                               *cp == 'U' || *cp == 'L')) { cp++; }
        if (*cp == '\0')
          {
          n = (int)strtol(val->Dimensions[i], NULL, 0);
          }
        }
      count *= n;
      }
    }

  return count;
}

/* deal with types that include function pointers or arrays */
void handle_complex_type(
  ValueInfo *val, unsigned int datatype, unsigned int extra,
  const char *funcSig)
{
  FunctionInfo *func = 0;

  /* remove specifiers like "friend" and "typedef" */
  datatype &= VTK_PARSE_QUALIFIED_TYPE;

  /* remove the pack specifier caused by "..." */
  if ((extra & VTK_PARSE_PACK) != 0)
    {
    val->IsPack = 1;
    extra ^= VTK_PARSE_PACK;
    }

  /* if "extra" was set, parentheses were involved */
  if ((extra & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION)
    {
    /* the current type becomes the function return type */
    func = getFunction();
    func->ReturnValue = (ValueInfo *)malloc(sizeof(ValueInfo));
    vtkParse_InitValue(func->ReturnValue);
    func->ReturnValue->Type = datatype;
    func->ReturnValue->Class = type_class(datatype, getTypeId());
    if (funcSig) { func->Signature = vtkstrdup(funcSig); }
    val->Function = func;

#ifndef VTK_PARSE_LEGACY_REMOVE
    func->ReturnType = func->ReturnValue->Type;
    func->ReturnClass = func->ReturnValue->Class;
#endif

    /* the val type is whatever was inside the parentheses */
    clearTypeId();
    setTypeId(func->Class ? "method" : "function");
    datatype = (extra & (VTK_PARSE_UNQUALIFIED_TYPE | VTK_PARSE_RVALUE));
    }
  else if ((extra & VTK_PARSE_INDIRECT) == VTK_PARSE_BAD_INDIRECT)
    {
    datatype = (datatype | VTK_PARSE_BAD_INDIRECT);
    }
  else if ((extra & VTK_PARSE_INDIRECT) != 0)
    {
    extra = (extra & (VTK_PARSE_INDIRECT | VTK_PARSE_RVALUE));

    if ((extra & VTK_PARSE_REF) != 0)
      {
      datatype = (datatype | (extra & (VTK_PARSE_REF | VTK_PARSE_RVALUE)));
      extra = (extra & ~(VTK_PARSE_REF | VTK_PARSE_RVALUE));
      }

    if (extra != 0 && getArrayNDims() > 0)
      {
      /* pointer represents an unsized array bracket */
      datatype = add_indirection(datatype, VTK_PARSE_ARRAY);
      extra = ((extra >> 2) & VTK_PARSE_POINTER_MASK);
      }

    datatype = add_indirection(datatype, extra);
    }

  if (getArrayNDims() == 1)
    {
    if ((datatype & VTK_PARSE_POINTER_LOWMASK) != VTK_PARSE_ARRAY)
      {
      /* turn the first set of brackets into a pointer */
      datatype = add_indirection(datatype, VTK_PARSE_POINTER);
      }
    else
      {
      pushArrayFront("");
      }
    }
  else if (getArrayNDims() > 1)
    {
    if ((datatype & VTK_PARSE_POINTER_LOWMASK) != VTK_PARSE_ARRAY)
      {
      /* turn the first set of brackets into a pointer */
      datatype = add_indirection(datatype, VTK_PARSE_ARRAY);
      }
    else
      {
      pushArrayFront("");
      }
    }

  /* get the data type */
  val->Type = datatype;
  val->Class = type_class(datatype, getTypeId());

  /* copy contents of all brackets to the ArgDimensions */
  val->NumberOfDimensions = getArrayNDims();
  val->Dimensions = getArray();
  clearArray();

  /* count is the product of the dimensions */
  val->Count = count_from_dimensions(val);
}

/* handle [[attributes]] */
void handle_attribute(const char *att, int pack)
{
  /* the role means "this is what the attribute applies to" */
  int role = getAttributeRole();

  size_t l = 0;
  const char *args = NULL;

  if (!att)
    {
    return;
    }

  /* search for arguments */
  l = vtkParse_SkipId(att);
  while (att[l] == ':' && att[l+1] == ':')
    {
    l += 2;
    l += vtkParse_SkipId(&att[l]);
    }
  if (att[l] == '(')
    {
    args = &att[l];
    }

  /* check for namespace */
  if (strncmp(att, "vtk::", 5) == 0)
    {
    if (args)
      {
      /* no current vtk attributes use arguments */
      print_parser_error("attribute takes no args", att, l);
      exit(1);
      }
    else if (pack)
      {
      /* no current vtk attributes use '...' */
      print_parser_error("attribute takes no ...", att, l);
      exit(1);
      }
    else if (strcmp(att, "vtk::newinstance") == 0 &&
             role == VTK_PARSE_ATTRIB_DECL)
      {
      setTypeMod(VTK_PARSE_NEWINSTANCE);
      }
    else
      {
      print_parser_error("attribute cannot be used here", att, l);
      exit(1);
      }
    }
}

/* add a parameter to the legacy part of the FunctionInfo struct */
void add_legacy_parameter(FunctionInfo *func, ValueInfo *param)
{
#ifndef VTK_PARSE_LEGACY_REMOVE
  int i = func->NumberOfArguments;

  if (i < MAX_ARGS)
    {
    func->NumberOfArguments = i + 1;
    func->ArgTypes[i] = param->Type;
    func->ArgClasses[i] = param->Class;
    func->ArgCounts[i] = param->Count;

    /* legacy wrappers need VTK_PARSE_FUNCTION without POINTER */
    if (param->Type == VTK_PARSE_FUNCTION_PTR)
      {
      /* check for signature "void (*func)(void *)" */
      if (param->Function->NumberOfParameters == 1 &&
          param->Function->Parameters[0]->Type == VTK_PARSE_VOID_PTR &&
          param->Function->Parameters[0]->NumberOfDimensions == 0 &&
          param->Function->ReturnValue->Type == VTK_PARSE_VOID)
        {
        func->ArgTypes[i] = VTK_PARSE_FUNCTION;
        }
      }
    }
  else
    {
    func->ArrayFailure = 1;
    }
#endif
}


/* reject the function, do not output it */
void reject_function()
{
  vtkParse_InitFunction(currentFunction);
  startSig();
  getMacro();
}

/* a simple routine that updates a few variables */
void output_function()
{
  const char *macro = getMacro();
  size_t n;
  int i, j;
  int match;

  /* reject template specializations */
  n = strlen(currentFunction->Name);
  if (currentFunction->Name[n-1] == '>')
    {
    /* make sure there is a matching angle bracket */
    while (n > 0 && currentFunction->Name[n-1] != '<') { n--; }
    if (n > 0)
      {
      reject_function();
      return;
      }
    }

  /* friend */
  if (currentFunction->ReturnValue &&
      currentFunction->ReturnValue->Type & VTK_PARSE_FRIEND)
    {
    currentFunction->ReturnValue->Type ^= VTK_PARSE_FRIEND;
    output_friend_function();
    return;
    }

  /* typedef */
  if (currentFunction->ReturnValue &&
      currentFunction->ReturnValue->Type & VTK_PARSE_TYPEDEF)
    {
    /* for now, reject it instead of turning a method into a typedef */
    currentFunction->ReturnValue->Type ^= VTK_PARSE_TYPEDEF;
    reject_function();
    return;
    }

  /* static */
  if (currentFunction->ReturnValue &&
      currentFunction->ReturnValue->Type & VTK_PARSE_STATIC)
    {
    currentFunction->IsStatic = 1;
    }

  /* virtual */
  if (currentFunction->ReturnValue &&
      currentFunction->ReturnValue->Type & VTK_PARSE_VIRTUAL)
    {
    currentFunction->IsVirtual = 1;
    }

  /* the signature */
  if (!currentFunction->Signature)
    {
    currentFunction->Signature = getSig();
    }

  /* template information */
  if (currentTemplate)
    {
    currentFunction->Template = currentTemplate;
    currentTemplate = NULL;
    }

  /* a void argument is the same as no parameters */
  if (currentFunction->NumberOfParameters == 1 &&
      (currentFunction->Parameters[0]->Type & VTK_PARSE_UNQUALIFIED_TYPE) ==
      VTK_PARSE_VOID)
    {
    currentFunction->NumberOfParameters = 0;
    }

  /* is it defined in a legacy macro? */
  if (macro && strcmp(macro, "VTK_LEGACY") == 0)
    {
    currentFunction->IsLegacy = 1;
    }

  /* set public, protected */
  if (currentClass)
    {
    currentFunction->Access = access_level;
    }
  else
    {
    currentFunction->Access = VTK_ACCESS_PUBLIC;
    }

#ifndef VTK_PARSE_LEGACY_REMOVE
  /* a void argument is the same as no parameters */
  if (currentFunction->NumberOfArguments == 1 &&
      (currentFunction->ArgTypes[0] & VTK_PARSE_UNQUALIFIED_TYPE) ==
      VTK_PARSE_VOID)
    {
    currentFunction->NumberOfArguments = 0;
    }

  /* if return type is void, set return class to void */
  if (currentFunction->ReturnClass == NULL &&
      (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE) ==
       VTK_PARSE_VOID)
    {
    currentFunction->ReturnClass = "void";
    }

  /* set legacy flags */
  if (currentClass)
    {
    currentFunction->IsPublic = (access_level == VTK_ACCESS_PUBLIC);
    currentFunction->IsProtected = (access_level == VTK_ACCESS_PROTECTED);
    }
  else
    {
    currentFunction->IsPublic = 1;
    currentFunction->IsProtected = 0;
    }

  /* check for too many parameters */
  if (currentFunction->NumberOfParameters > MAX_ARGS)
    {
    currentFunction->ArrayFailure = 1;
    }

  for (i = 0; i < currentFunction->NumberOfParameters; i++)
    {
    ValueInfo *param = currentFunction->Parameters[i];
    /* tell old wrappers that multi-dimensional arrays are bad */
    if ((param->Type & VTK_PARSE_POINTER_MASK) != 0)
      {
      if (((param->Type & VTK_PARSE_INDIRECT) == VTK_PARSE_BAD_INDIRECT) ||
          ((param->Type & VTK_PARSE_POINTER_LOWMASK) != VTK_PARSE_POINTER))
        {
        currentFunction->ArrayFailure = 1;
        }
      }

    /* allow only "void (*func)(void *)" as a valid function pointer */
    if ((param->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION)
      {
      if (i != 0 || param->Type != VTK_PARSE_FUNCTION_PTR ||
          currentFunction->NumberOfParameters != 2 ||
          currentFunction->Parameters[1]->Type != VTK_PARSE_VOID_PTR ||
          param->Function->NumberOfParameters != 1 ||
          param->Function->Parameters[0]->Type != VTK_PARSE_VOID_PTR ||
          param->Function->Parameters[0]->NumberOfDimensions != 0 ||
          param->Function->ReturnValue->Type != VTK_PARSE_VOID)
        {
        currentFunction->ArrayFailure = 1;
        }
      }
    }
#endif /* VTK_PARSE_LEGACY_REMOVE */

  if (currentClass)
    {
    /* is it a delete function */
    if (currentFunction->Name && !strcmp("Delete",currentFunction->Name))
      {
      currentClass->HasDelete = 1;
      }

    currentFunction->Class = currentClass->Name;
    vtkParse_AddFunctionToClass(currentClass, currentFunction);

    currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
    }
  else
    {
    /* make sure this function isn't a repeat */
    match = 0;
    for (i = 0; i < currentNamespace->NumberOfFunctions; i++)
      {
      if (currentNamespace->Functions[i]->Name &&
          strcmp(currentNamespace->Functions[i]->Name,
                 currentFunction->Name) == 0)
        {
        if (currentNamespace->Functions[i]->NumberOfParameters ==
            currentFunction->NumberOfParameters)
          {
          for (j = 0; j < currentFunction->NumberOfParameters; j++)
            {
            if (currentNamespace->Functions[i]->Parameters[j]->Type ==
                currentFunction->Parameters[j]->Type)
              {
              if (currentFunction->Parameters[j]->Type == VTK_PARSE_OBJECT &&
                  strcmp(currentNamespace->Functions[i]->Parameters[j]->Class,
                         currentFunction->Parameters[j]->Class) == 0)
                {
                break;
                }
              }
            }
          if (j == currentFunction->NumberOfParameters)
            {
            match = 1;
            break;
            }
          }
        }
      }

    if (!match)
      {
      vtkParse_AddFunctionToNamespace(currentNamespace, currentFunction);

      currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
      }
    }

  vtkParse_InitFunction(currentFunction);
  startSig();
}

/* output a function that is not a method of the current class */
void output_friend_function()
{
  ClassInfo *tmpc = currentClass;
  currentClass = NULL;
  output_function();
  currentClass = tmpc;
}

void outputSetVectorMacro(const char *var, unsigned int paramType,
                          const char *typeText, int n)
{
  static const char *mnames[] = {
    NULL, NULL,
    "vtkSetVector2Macro", "vtkSetVector3Macro", "vtkSetVector4Macro",
    NULL,
    "vtkSetVector6Macro",
    NULL };
  char ntext[32];
  int i, m;
  m = (n > 7 ? 0 : n);

  sprintf(ntext, "%i", n);

  currentFunction->Macro = mnames[m];
  currentFunction->Name = vtkstrcat("Set", var);
  startSig();
  postSig("void ");
  postSig(currentFunction->Name);
  postSig("(");
  postSig(typeText);
  for (i = 1; i < n; i++)
    {
    postSig(", ");
    postSig(typeText);
    }
  postSig(");");
  for (i = 0; i < n; i++)
    {
    add_parameter(currentFunction, paramType, getTypeId(), 0);
    }
  set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
  output_function();

  currentFunction->Macro = mnames[m];
  currentFunction->Name = vtkstrcat("Set", var);
  currentFunction->Signature =
    vtkstrcat7("void ", currentFunction->Name, "(", typeText,
               " a[", ntext, "]);");
  add_parameter(currentFunction, (VTK_PARSE_POINTER | paramType),
                getTypeId(), n);
  set_return(currentFunction, VTK_PARSE_VOID, "void", 0);
  output_function();
}

void outputGetVectorMacro(const char *var, unsigned int paramType,
                          const char *typeText, int n)
{
  static const char *mnames[] = {
    NULL, NULL,
    "vtkGetVector2Macro", "vtkGetVector3Macro", "vtkGetVector4Macro",
    NULL,
    "vtkGetVector6Macro",
    NULL };
  int m;
  m = (n > 7 ? 0 : n);

  currentFunction->Macro = mnames[m];
  currentFunction->Name = vtkstrcat("Get", var);
  currentFunction->Signature =
    vtkstrcat4(typeText, " *", currentFunction->Name, "();");
  set_return(currentFunction, (VTK_PARSE_POINTER | paramType), getTypeId(), n);
  output_function();
}

/* Set a flag to recurse into included files */
void vtkParse_SetRecursive(int option)
{
  if (option)
    {
    Recursive = 1;
    }
  else
    {
    Recursive = 0;
    }
}

/* Set the global variable that stores the current executable */
void vtkParse_SetCommandName(const char *name)
{
  CommandName = name;
}

/* Parse a header file and return a FileInfo struct */
FileInfo *vtkParse_ParseFile(
  const char *filename, FILE *ifile, FILE *errfile)
{
  int i, j;
  int ret;
  FileInfo *file_info;
  char *main_class;

  /* "data" is a global variable used by the parser */
  data = (FileInfo *)malloc(sizeof(FileInfo));
  vtkParse_InitFile(data);
  data->Strings = (StringCache *)malloc(sizeof(StringCache));
  vtkParse_InitStringCache(data->Strings);

  /* "preprocessor" is a global struct used by the parser */
  preprocessor = (PreprocessInfo *)malloc(sizeof(PreprocessInfo));
  vtkParsePreprocess_Init(preprocessor, filename);
  preprocessor->Strings = data->Strings;
  vtkParsePreprocess_AddStandardMacros(preprocessor, VTK_PARSE_NATIVE);

  /* add include files specified on the command line */
  for (i = 0; i < NumberOfIncludeDirectories; i++)
    {
    vtkParsePreprocess_IncludeDirectory(preprocessor, IncludeDirectories[i]);
    }

  /* add macros specified on the command line */
  for (i = 0; i < NumberOfDefinitions; i++)
    {
    const char *cp = Definitions[i];

    if (*cp == 'U')
      {
      vtkParsePreprocess_RemoveMacro(preprocessor, &cp[1]);
      }
    else if (*cp == 'D')
      {
      const char *definition = &cp[1];
      while (*definition != '=' && *definition != '\0')
        {
        definition++;
        }
      if (*definition == '=')
        {
        definition++;
        }
      else
        {
        definition = NULL;
        }
      vtkParsePreprocess_AddMacro(preprocessor, &cp[1], definition);
      }
    }

  /* should explicitly check for vtkConfigure.h, or even explicitly load it */
#ifdef VTK_USE_64BIT_IDS
  vtkParsePreprocess_AddMacro(preprocessor, "VTK_USE_64BIT_IDS", NULL);
#endif

  data->FileName = vtkstrdup(filename);

  clearComment();

  namespaceDepth = 0;
  currentNamespace = (NamespaceInfo *)malloc(sizeof(NamespaceInfo));
  vtkParse_InitNamespace(currentNamespace);
  data->Contents = currentNamespace;

  templateDepth = 0;
  currentTemplate = NULL;

  currentFunction = (FunctionInfo *)malloc(sizeof(FunctionInfo));
  vtkParse_InitFunction(currentFunction);
  startSig();

  parseDebug = 0;
  if (getenv("DEBUG") != NULL)
    {
    parseDebug = 1;
    }

  yyset_in(ifile);
  yyset_out(errfile);
  ret = yyparse();

  if (ret)
    {
    return NULL;
    }

  free(currentFunction);
  yylex_destroy();

  /* The main class name should match the file name */
  i = strlen(filename);
  j = i;
  while (i > 0)
    {
    --i;
    if (filename[i] == '.')
      {
      j = i;
      }
    if (filename[i] == '/' || filename[i] == '\\')
      {
      i++;
      break;
      }
    }
  main_class = (char *)malloc(j-i+1);
  strncpy(main_class, &filename[i], j-i);
  main_class[j-i] = '\0';

  /* special treatment of the main class in the file */
  for (i = 0; i < currentNamespace->NumberOfClasses; i++)
    {
    if (strcmp(currentNamespace->Classes[i]->Name, main_class) == 0)
      {
      data->MainClass = currentNamespace->Classes[i];
      break;
      }
    }
  free(main_class);

  /* assign doxygen comments to their targets */
  assignComments(data->Contents);

  vtkParsePreprocess_Free(preprocessor);
  preprocessor = NULL;
  macroName = NULL;

  file_info = data;
  data = NULL;

  return file_info;
}

/* Read a hints file and update the FileInfo */
int vtkParse_ReadHints(FileInfo *file_info, FILE *hfile, FILE *errfile)
{
  char h_cls[512];
  char h_func[512];
  unsigned int h_type, type;
  int h_value;
  FunctionInfo *func_info;
  ClassInfo *class_info;
  NamespaceInfo *contents;
  int i, j;
  int lineno = 0;
  int n;

  contents = file_info->Contents;

  /* read each hint line in succession */
  while ((n = fscanf(hfile,"%s %s %x %i", h_cls, h_func, &h_type, &h_value))
         != EOF)
    {
    lineno++;
    if (n < 4)
      {
      fprintf(errfile, "Wrapping: error parsing hints file line %i\n", lineno);
      exit(1);
      }

    /* erase "ref" and qualifiers from hint type */
    type = ((h_type & VTK_PARSE_BASE_TYPE) |
            (h_type & VTK_PARSE_POINTER_LOWMASK));

    /* find the matching class */
    for (i = 0; i < contents->NumberOfClasses; i++)
      {
      class_info = contents->Classes[i];

      if (strcmp(h_cls, class_info->Name) == 0)
        {
        /* find the matching function */
        for (j = 0; j < class_info->NumberOfFunctions; j++)
          {
          func_info = class_info->Functions[j];

          if ((strcmp(h_func, func_info->Name) == 0) &&
              func_info->ReturnValue &&
              (type == ((func_info->ReturnValue->Type & ~VTK_PARSE_REF) &
                        VTK_PARSE_UNQUALIFIED_TYPE)))
            {
            /* types that hints are accepted for */
            switch (func_info->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE)
              {
              case VTK_PARSE_FLOAT_PTR:
              case VTK_PARSE_VOID_PTR:
              case VTK_PARSE_DOUBLE_PTR:
              case VTK_PARSE_ID_TYPE_PTR:
              case VTK_PARSE_LONG_LONG_PTR:
              case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
              case VTK_PARSE___INT64_PTR:
              case VTK_PARSE_UNSIGNED___INT64_PTR:
              case VTK_PARSE_INT_PTR:
              case VTK_PARSE_UNSIGNED_INT_PTR:
              case VTK_PARSE_SHORT_PTR:
              case VTK_PARSE_UNSIGNED_SHORT_PTR:
              case VTK_PARSE_LONG_PTR:
              case VTK_PARSE_UNSIGNED_LONG_PTR:
              case VTK_PARSE_SIGNED_CHAR_PTR:
              case VTK_PARSE_UNSIGNED_CHAR_PTR:
              case VTK_PARSE_CHAR_PTR:
                {
                if (func_info->ReturnValue->NumberOfDimensions == 0)
                  {
                  char text[64];
                  sprintf(text, "%i", h_value);
                  func_info->ReturnValue->Count = h_value;
                  vtkParse_AddStringToArray(
                    &func_info->ReturnValue->Dimensions,
                    &func_info->ReturnValue->NumberOfDimensions,
                    vtkParse_CacheString(
                      file_info->Strings, text, strlen(text)));
#ifndef VTK_PARSE_LEGACY_REMOVE
                  func_info->HaveHint = 1;
                  func_info->HintSize = h_value;
#endif
                  }
                break;
                }
              default:
                {
                fprintf(errfile,
                        "Wrapping: unhandled hint type %#x\n", h_type);
                }
              }
            }
          }
        }
      }
    }

  return 1;
}

/* Free the FileInfo struct returned by vtkParse_ParseFile() */
void vtkParse_Free(FileInfo *file_info)
{
  vtkParse_FreeFile(file_info);
  vtkParse_FreeStringCache(file_info->Strings);
  free(file_info->Strings);
  free(file_info);
}

/** Define a preprocessor macro. Function macros are not supported.  */
void vtkParse_DefineMacro(const char *name, const char *definition)
{
  size_t n = vtkParse_SkipId(name);
  size_t l;
  char *cp;

  if (definition == NULL)
    {
    definition = "";
    }

  l = n + strlen(definition) + 2;
  cp = (char *)malloc(l + 1);
  cp[0] = 'D';
  strncpy(&cp[1], name, n);
  cp[n+1] = '\0';
  if (definition[0] != '\0')
    {
    cp[n+1] = '=';
    strcpy(&cp[n+2], definition);
    }
  cp[l] = '\0';

  vtkParse_AddStringToArray(&Definitions, &NumberOfDefinitions, cp);
}

/** Undefine a preprocessor macro.  */
void vtkParse_UndefineMacro(const char *name)
{
  size_t n = vtkParse_SkipId(name);
  char *cp;

  cp = (char *)malloc(n+2);
  cp[0] = 'U';
  strncpy(&cp[1], name, n);
  cp[n+1] = '\0';

  vtkParse_AddStringToArray(&Definitions, &NumberOfDefinitions, cp);
}

/** Add an include directory, for use with the "-I" option.  */
void vtkParse_IncludeDirectory(const char *dirname)
{
  size_t n = strlen(dirname);
  char *cp;
  int i;

  for (i = 0; i < NumberOfIncludeDirectories; i++)
    {
    if (strncmp(IncludeDirectories[i], dirname, n) == 0 &&
        IncludeDirectories[i][n] == '\0')
      {
      return;
      }
    }

  cp = (char *)malloc(n+1);
  strcpy(cp, dirname);

  vtkParse_AddStringToArray(
    &IncludeDirectories, &NumberOfIncludeDirectories, cp);
}

/** Return the full path to a header file.  */
const char *vtkParse_FindIncludeFile(const char *filename)
{
  static StringCache cache = {0, 0, 0, 0};
  static PreprocessInfo info = {0, 0, 0, 0, 0, 0, &cache, 0, 0, 0};
  int val;
  int i;

  /* add include files specified on the command line */
  for (i = 0; i < NumberOfIncludeDirectories; i++)
    {
    vtkParsePreprocess_IncludeDirectory(&info, IncludeDirectories[i]);
    }

  return vtkParsePreprocess_FindIncludeFile(&info, filename, 0, &val);
}
