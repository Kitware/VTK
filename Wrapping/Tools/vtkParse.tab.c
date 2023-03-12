/* A Bison parser, made by GNU Bison 3.2.3.  */

/* Skeleton implementation for Bison GLR parsers in C

   Copyright (C) 2002-2015, 2018 Free Software Foundation, Inc.

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "glr.c"

/* Pure parsers.  */
#define YYPURE 0

/* First part of user prologue.  */

/*

This file must be translated to C and modified to build everywhere.

Run bison like this (use bison 3.2.3 or later)

  bison --no-lines -b vtkParse vtkParse.y

Modify vtkParse.tab.c:
  - replace all instances of "static inline" with "static"
  - replace "#if ! defined lint || defined __GNUC__" with "#if 1"
  - remove YY_ATTRIBUTE_UNUSED from yyfillin, yyfill, and yynormal
  - remove the "break;" after "return yyreportAmbiguity"
  - replace "(1-yyrhslen)" with "(1-(int)yyrhslen)"
  - replace "sizeof yynewStates[0] with "sizeof (yyGLRState*)"
  - replace "sizeof yynewLookaheadNeeds[0] with "sizeof (yybool)"
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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define yyerror(a) print_parser_error(a, NULL, 0)
#define yywrap() 1

/* Make sure yacc-generated code knows we have included stdlib.h.  */
#ifndef _STDLIB_H
#define _STDLIB_H
#endif
#define YYINCLUDED_STDLIB_H

/* MSVC does not define __STDC__ properly. */
#if !defined(__STDC__)
#if defined(_MSC_VER)
#define __STDC__ 1
#endif
#endif

/* Disable warnings in generated code. */
#if defined(_MSC_VER)
#pragma warning(disable : 4127) /* conditional expression is constant */
#pragma warning(disable : 4244) /* conversion to smaller integer type */
#endif

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParsePreprocess.h"
#include "vtkParseSystem.h"

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

#define vtkParseDebug(s1, s2)                                                                      \
  if (parseDebug)                                                                                  \
  {                                                                                                \
    fprintf(stderr, "   %s %s\n", s1, s2);                                                         \
  }

/* the tokenizer */
int yylex(void);

/* global variables */
FileInfo* data = NULL;
int parseDebug;

/* globals for cacheing directory listings */
static StringCache system_strings = { 0, 0, 0, 0 };
static SystemInfo system_cache = { &system_strings, NULL, NULL };

/* the "preprocessor" */
PreprocessInfo* preprocessor = NULL;

/* whether to pre-define platform-specific macros */
int PredefinePlatformMacros = 1;

/* include dirs specified on the command line */
int NumberOfIncludeDirectories = 0;
const char** IncludeDirectories;

/* macros specified on the command line */
int NumberOfDefinitions = 0;
const char** Definitions;

/* include specified on the command line */
int NumberOfMacroIncludes = 0;
const char** MacroIncludes;

/* for dumping diagnostics about macros */
int DumpMacros = 0;
const char* DumpFileName = NULL;

/* options that can be set by the programs that use the parser */
int Recursive = 0;
const char* CommandName = NULL;

/* various state variables */
NamespaceInfo* currentNamespace = NULL;
ClassInfo* currentClass = NULL;
FunctionInfo* currentFunction = NULL;
TemplateInfo* currentTemplate = NULL;
const char* currentEnumName = NULL;
const char* currentEnumValue = NULL;
unsigned int currentEnumType = 0;
const char* deprecationReason = NULL;
const char* deprecationVersion = NULL;
parse_access_t access_level = VTK_ACCESS_PUBLIC;

/* functions from vtkParse.l */
void print_parser_error(const char* text, const char* cp, size_t n);

/* helper functions */
static const char* type_class(unsigned int type, const char* classname);
static void start_class(const char* classname, int is_struct_or_union);
static void end_class(void);
static void add_base_class(ClassInfo* cls, const char* name, int access_lev, unsigned int extra);
static void output_friend_function(void);
static void output_function(void);
static void reject_function(void);
static void set_return(
  FunctionInfo* func, unsigned int attributes, unsigned int type, const char* typeclass, int count);
static void add_template_parameter(unsigned int datatype, unsigned int extra, const char* funcSig);
static void add_using(const char* name, int is_namespace);
static void start_enum(const char* name, int is_scoped, unsigned int type, const char* basename);
static void add_enum(const char* name, const char* value);
static void end_enum(void);
static unsigned int guess_constant_type(const char* valstring);
static void add_constant(const char* name, const char* value, unsigned int attributes,
  unsigned int type, const char* typeclass, int flag);
static unsigned int guess_id_type(const char* cp);
static unsigned int add_indirection(unsigned int type1, unsigned int type2);
static unsigned int add_indirection_to_array(unsigned int type);
static void handle_complex_type(ValueInfo* val, unsigned int attributes, unsigned int datatype,
  unsigned int extra, const char* funcSig);
static void handle_attribute(const char* att, int pack);
static void add_legacy_parameter(FunctionInfo* func, ValueInfo* param);

/*----------------------------------------------------------------
 * String utility methods
 *
 * Strings are centrally allocated and are const, and they are not
 * freed until the program exits.  If they need to be freed before
 * then, vtkParse_FreeStringCache() can be called.
 */

/* duplicate the first n bytes of a string and terminate */
static const char* vtkstrndup(const char* in, size_t n)
{
  return vtkParse_CacheString(data->Strings, in, n);
}

/* duplicate a string */
static const char* vtkstrdup(const char* in)
{
  if (in)
  {
    in = vtkParse_CacheString(data->Strings, in, strlen(in));
  }

  return in;
}

/* helper function for concatenating strings */
static const char* vtkstrncat(size_t n, const char** str)
{
  char* cp;
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
static const char* vtkstrcat(const char* str1, const char* str2)
{
  const char* cp[2];

  cp[0] = str1;
  cp[1] = str2;
  return vtkstrncat(2, cp);
}

static const char* vtkstrcat3(const char* str1, const char* str2, const char* str3)
{
  const char* cp[3];

  cp[0] = str1;
  cp[1] = str2;
  cp[2] = str3;
  return vtkstrncat(3, cp);
}

static const char* vtkstrcat4(
  const char* str1, const char* str2, const char* str3, const char* str4)
{
  const char* cp[4];

  cp[0] = str1;
  cp[1] = str2;
  cp[2] = str3;
  cp[3] = str4;
  return vtkstrncat(4, cp);
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
char* commentText = NULL;
size_t commentLength = 0;
size_t commentAllocatedLength = 0;
int commentState = 0;
int commentMemberGroup = 0;
int commentGroupDepth = 0;
parse_dox_t commentType = DOX_COMMAND_OTHER;
const char* commentTarget = NULL;

/* Struct for recognizing certain doxygen commands */
struct DoxygenCommandInfo
{
  const char* name;
  size_t length;
  parse_dox_t type;
};

/* List of doxygen commands (@cond is not handled yet) */
/* clang-format off */
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
/* clang-format on */

void closeComment(void);

/* Clear the comment buffer */
void clearComment(void)
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
static const char* getComment(void)
{
  const char* text = commentText;
  const char* cp = commentText;
  size_t l = commentLength;

  if (commentText != NULL && commentState != 0)
  {
    /* strip trailing blank lines */
    while (
      l > 0 && (cp[l - 1] == ' ' || cp[l - 1] == '\t' || cp[l - 1] == '\r' || cp[l - 1] == '\n'))
    {
      if (cp[l - 1] == '\n')
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
static parse_dox_t checkDoxygenCommand(const char* text, size_t n)
{
  struct DoxygenCommandInfo* info;
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
void addCommentLine(const char* line, size_t n, int type)
{
  size_t i, j;
  parse_dox_t t = DOX_COMMAND_OTHER;

  if (type == DoxygenComment || commentState == DoxygenComment)
  {
    if (type == DoxygenComment)
    {
      /* search for '@' and backslash */
      for (i = 0; i + 1 < n; i++)
      {
        if (line[i] == '@' || line[i] == '\\')
        {
          j = ++i;
          while (i < n && line[i] >= 'a' && line[i] <= 'z')
          {
            i++;
          }
          if (line[i - 1] == '@' && (line[i] == '{' || line[i] == '}'))
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
            t = checkDoxygenCommand(&line[j], i - j);
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
              commentTarget = vtkstrndup(&line[j], i - j);
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
  else if (commentState == 0 || commentState == StickyComment || commentState == ClosedComment)
  {
    clearComment();
    return;
  }

  if (commentText == NULL)
  {
    commentAllocatedLength = n + 80;
    commentText = (char*)malloc(commentAllocatedLength);
    commentLength = 0;
    commentText[0] = '\0';
  }
  else if (commentLength + n + 2 > commentAllocatedLength)
  {
    commentAllocatedLength = commentAllocatedLength + commentLength + n + 2;
    commentText = (char*)realloc(commentText, commentAllocatedLength);
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
static void storeComment(void)
{
  CommentInfo* info = (CommentInfo*)malloc(sizeof(CommentInfo));
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
static void applyComment(ClassInfo* cls)
{
  int i;
  ItemInfo* item;
  const char* comment = vtkstrdup(getComment());

  i = cls->NumberOfItems;
  if (i > 0)
  {
    item = &cls->Items[--i];
    if (item->Type == VTK_NAMESPACE_INFO)
    {
      cls->Namespaces[item->Index]->Comment = comment;
    }
    else if (item->Type == VTK_CLASS_INFO || item->Type == VTK_STRUCT_INFO ||
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
void closeComment(void)
{
  const char* cp;
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
      while (l > 0 && (cp[l - 1] == '\n' || cp[l - 1] == '\r' || cp[l - 1] == ' '))
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
void commentBreak(void)
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
void assignComments(ClassInfo* cls)
{
  int i, j;
  int t;
  const char* name;
  const char* comment;

  for (i = 0; i < cls->NumberOfComments; i++)
  {
    t = cls->Comments[i]->Type;
    name = cls->Comments[i]->Name;
    comment = cls->Comments[i]->Comment;
    /* find the item the comment applies to */
    if (t == DOX_COMMAND_CLASS || t == DOX_COMMAND_STRUCT || t == DOX_COMMAND_UNION)
    {
      for (j = 0; j < cls->NumberOfClasses; j++)
      {
        if (cls->Classes[j]->Name && name && strcmp(cls->Classes[j]->Name, name) == 0)
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
        if (cls->Enums[j]->Name && name && strcmp(cls->Enums[j]->Name, name) == 0)
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
        if (cls->Typedefs[j]->Name && name && strcmp(cls->Typedefs[j]->Name, name) == 0)
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
        if (cls->Functions[j]->Name && name && strcmp(cls->Functions[j]->Name, name) == 0)
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
        if (cls->Variables[j]->Name && name && strcmp(cls->Variables[j]->Name, name) == 0)
        {
          cls->Variables[j]->Comment = comment;
          break;
        }
      }
      for (j = 0; j < cls->NumberOfConstants; j++)
      {
        if (cls->Constants[j]->Name && name && strcmp(cls->Constants[j]->Name, name) == 0)
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
        if (cls->Namespaces[j]->Name && name && strcmp(cls->Namespaces[j]->Name, name) == 0)
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
const char* macroName = NULL;
int macroUsed = 0;
int macroEnded = 0;

static const char* getMacro(void)
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
NamespaceInfo* namespaceStack[10];
int namespaceDepth = 0;

/* enter a namespace */
static void pushNamespace(const char* name)
{
  int i;
  NamespaceInfo* oldNamespace = currentNamespace;

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
    currentNamespace = (NamespaceInfo*)malloc(sizeof(NamespaceInfo));
    vtkParse_InitNamespace(currentNamespace);
    currentNamespace->Name = name;
    vtkParse_AddNamespaceToNamespace(oldNamespace, currentNamespace);
  }

  namespaceStack[namespaceDepth++] = oldNamespace;
}

/* leave the namespace */
static void popNamespace(void)
{
  currentNamespace = namespaceStack[--namespaceDepth];
}

/*----------------------------------------------------------------
 * Classes
 *
 * operates on: currentClass, access_level
 */

/* "private" variables */
ClassInfo* classStack[10];
parse_access_t classAccessStack[10];
int classDepth = 0;

/* start an internal class definition */
static void pushClass(void)
{
  classAccessStack[classDepth] = access_level;
  classStack[classDepth++] = currentClass;
}

/* leave the internal class */
static void popClass(void)
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
TemplateInfo* templateStack[10];
int templateDepth = 0;

/* begin a template */
static void startTemplate(void)
{
  currentTemplate = (TemplateInfo*)malloc(sizeof(TemplateInfo));
  vtkParse_InitTemplate(currentTemplate);
}

/* clear a template, if set */
static void clearTemplate(void)
{
  if (currentTemplate)
  {
    free(currentTemplate);
  }
  currentTemplate = NULL;
}

/* push the template onto the stack, and start a new one */
static void pushTemplate(void)
{
  templateStack[templateDepth++] = currentTemplate;
  startTemplate();
}

/* pop a template off the stack */
static void popTemplate(void)
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
char* signature = NULL;

/* start a new signature */
static void startSig(void)
{
  signature = NULL;
  sigLength = 0;
  sigAllocatedLength = 0;
  sigClosed = 0;
  sigMarkDepth = 0;
  sigMark[0] = 0;
}

/* get the signature */
static const char* getSig(void)
{
  return signature;
}

/* get the signature length */
static size_t getSigLength(void)
{
  return sigLength;
}

/* reallocate Signature if n chars cannot be appended */
static void checkSigSize(size_t n)
{
  const char* ccp;

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
static void closeSig(void)
{
  sigClosed = 1;
}

/* re-open the signature */
static void openSig(void)
{
  sigClosed = 0;
}

/* append text to the end of the signature */
static void postSig(const char* arg)
{
  if (!sigClosed)
  {
    size_t n = strlen(arg);
    checkSigSize(n);
    if (n > 0)
    {
      strncpy(&signature[sigLength], arg, n + 1);
      sigLength += n;
    }
  }
}

/* set a mark in the signature for later operations */
static void markSig(void)
{
  sigMark[sigMarkDepth] = 0;
  if (signature)
  {
    sigMark[sigMarkDepth] = sigLength;
  }
  sigMarkDepth++;
}

/* get the contents of the sig from the mark, and clear the mark */
static const char* copySig(void)
{
  const char* cp = NULL;
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
static const char* cutSig(void)
{
  const char* cp = NULL;
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

/* chop the last space from the signature */
static void chopSig(void)
{
  if (signature)
  {
    size_t n = sigLength;
    if (n > 0 && signature[n - 1] == ' ')
    {
      signature[n - 1] = '\0';
      sigLength--;
    }
  }
}

/* chop the last space from the signature unless the preceding token
   is an operator (used to remove spaces before argument lists) */
static void postSigLeftBracket(const char* s)
{
  if (signature)
  {
    size_t n = sigLength;
    if (n > 1 && signature[n - 1] == ' ')
    {
      const char* ops = "%*/-+!~&|^<>=.,:;{}";
      char c = signature[n - 2];
      const char* cp;
      for (cp = ops; *cp != '\0'; cp++)
      {
        if (*cp == c)
        {
          break;
        }
      }
      if (*cp == '\0')
      {
        signature[n - 1] = '\0';
        sigLength--;
      }
    }
  }
  postSig(s);
}

/* chop trailing space and add a right bracket */
static void postSigRightBracket(const char* s)
{
  chopSig();
  postSig(s);
}

/*----------------------------------------------------------------
 * Subroutines for building a type
 */

/* "private" variables */
unsigned int storedType;
unsigned int typeStack[10];
unsigned int declAttributes;
unsigned int attributeStack[10];
int typeDepth = 0;

/* save the type on the stack */
static void pushType(void)
{
  attributeStack[typeDepth] = declAttributes;
  typeStack[typeDepth++] = storedType;
}

/* pop the type stack */
static void popType(void)
{
  storedType = typeStack[--typeDepth];
  declAttributes = attributeStack[typeDepth];
}

/* clear the storage type */
static void clearType(void)
{
  storedType = 0;
  declAttributes = 0;
}

/* save the type */
static void setTypeBase(unsigned int base)
{
  storedType &= ~(unsigned int)(VTK_PARSE_BASE_TYPE);
  storedType |= base;
}

/* set a type modifier bit */
static void setTypeMod(unsigned int mod)
{
  storedType |= mod;
}

/* modify the indirection (pointers, refs) in the storage type */
static void setTypePtr(unsigned int ind)
{
  storedType &= ~(unsigned int)(VTK_PARSE_INDIRECT | VTK_PARSE_RVALUE);
  ind &= (VTK_PARSE_INDIRECT | VTK_PARSE_RVALUE);
  storedType |= ind;
}

/* retrieve the storage type */
static unsigned int getType(void)
{
  return storedType;
}

/* combine two primitive type parts, e.g. "long int" */
static unsigned int buildTypeBase(unsigned int a, unsigned int b)
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

/* add an attribute specifier to the current declaration */
static void addAttribute(unsigned int flags)
{
  declAttributes |= flags;
}

/* check if an attribute is set for the current declaration */
static int getAttributes(void)
{
  return declAttributes;
}

/*----------------------------------------------------------------
 * Array information
 */

/* "private" variables */
int numberOfDimensions = 0;
const char** arrayDimensions = NULL;

/* clear the array counter */
static void clearArray(void)
{
  numberOfDimensions = 0;
  arrayDimensions = NULL;
}

/* add another dimension */
static void pushArraySize(const char* size)
{
  vtkParse_AddStringToArray(&arrayDimensions, &numberOfDimensions, size);
}

/* add another dimension to the front */
static void pushArrayFront(const char* size)
{
  int i;

  vtkParse_AddStringToArray(&arrayDimensions, &numberOfDimensions, 0);

  for (i = numberOfDimensions - 1; i > 0; i--)
  {
    arrayDimensions[i] = arrayDimensions[i - 1];
  }

  arrayDimensions[0] = size;
}

/* get the number of dimensions */
static int getArrayNDims(void)
{
  return numberOfDimensions;
}

/* get the whole array */
static const char** getArray(void)
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
const char* currentVarName = 0;
const char* currentVarValue = 0;
const char* currentId = 0;

/* clear the var Id */
static void clearVarName(void)
{
  currentVarName = NULL;
}

/* set the var Id */
static void setVarName(const char* text)
{
  currentVarName = text;
}

/* return the var id */
static const char* getVarName(void)
{
  return currentVarName;
}

/* variable value -------------- */

/* clear the var value */
static void clearVarValue(void)
{
  currentVarValue = NULL;
}

/* set the var value */
static void setVarValue(const char* text)
{
  currentVarValue = text;
}

/* return the var value */
static const char* getVarValue(void)
{
  return currentVarValue;
}

/* variable type -------------- */

/* clear the current Id */
static void clearTypeId(void)
{
  currentId = NULL;
}

/* set the current Id, it is sticky until cleared */
static void setTypeId(const char* text)
{
  if (currentId == NULL)
  {
    currentId = text;
  }
}

/* set the signature and type together */
static void typeSig(const char* text)
{
  postSig(text);
  postSig(" ");

  if (currentId == 0)
  {
    setTypeId(text);
  }
}

/* return the current Id */
static const char* getTypeId(void)
{
  return currentId;
}

/*----------------------------------------------------------------
 * Specifically for function pointers, the scope (i.e. class) that
 * the function is a method of.
 */

const char* pointerScopeStack[10];
int pointerScopeDepth = 0;

/* save the scope for scoped method pointers */
static void scopeSig(const char* scope)
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
static const char* getScope(void)
{
  return pointerScopeStack[--pointerScopeDepth];
}

/*----------------------------------------------------------------
 * Function stack
 *
 * operates on: currentFunction
 */

/* "private" variables */
FunctionInfo* functionStack[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const char* functionVarNameStack[10];
const char* functionTypeIdStack[10];
int functionDepth = 0;

static void pushFunction(void)
{
  functionStack[functionDepth] = currentFunction;
  currentFunction = (FunctionInfo*)malloc(sizeof(FunctionInfo));
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

static void popFunction(void)
{
  FunctionInfo* newFunction = currentFunction;

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

  functionStack[functionDepth + 1] = newFunction;
}

static FunctionInfo* getFunction(void)
{
  return functionStack[functionDepth + 1];
}

/*----------------------------------------------------------------
 * Attributes
 */

int attributeRole = 0;
const char* attributePrefix = NULL;

/* Set kind of attributes to collect in attribute_specifier_seq */
static void setAttributeRole(int x)
{
  attributeRole = x;
}

/* Get the current kind of attribute */
static int getAttributeRole(void)
{
  return attributeRole;
}

/* Ignore attributes until further notice */
static void clearAttributeRole(void)
{
  attributeRole = 0;
}

/* Set the "using" prefix for attributes */
static void setAttributePrefix(const char* x)
{
  attributePrefix = x;
}

/* Get the "using" prefix for attributes */
static const char* getAttributePrefix(void)
{
  return attributePrefix;
}

/*----------------------------------------------------------------
 * Utility methods
 */

/* expand a type by including pointers from another */
static unsigned int add_indirection(unsigned int type1, unsigned int type2)
{
  unsigned int ptr1 = (type1 & VTK_PARSE_POINTER_MASK);
  unsigned int ptr2 = (type2 & VTK_PARSE_POINTER_MASK);
  unsigned int reverse = 0;
  unsigned int result;

  /* one of type1 or type2 will only have VTK_PARSE_INDIRECT, but
   * we don't know which one. */
  result = ((type1 & ~VTK_PARSE_POINTER_MASK) | (type2 & ~VTK_PARSE_POINTER_MASK));

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
static unsigned int add_indirection_to_array(unsigned int type)
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

#ifndef YY_NULLPTR
#if defined __cplusplus
#if 201103L <= __cplusplus
#define YY_NULLPTR nullptr
#else
#define YY_NULLPTR 0
#endif
#else
#define YY_NULLPTR ((void*)0)
#endif
#endif

/* Debug traces.  */
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
#define YYTOKENTYPE
enum yytokentype
{
  ID = 258,
  VTK_ID = 259,
  QT_ID = 260,
  StdString = 261,
  OSTREAM = 262,
  ISTREAM = 263,
  LP = 264,
  LA = 265,
  STRING_LITERAL = 266,
  INT_LITERAL = 267,
  HEX_LITERAL = 268,
  BIN_LITERAL = 269,
  OCT_LITERAL = 270,
  FLOAT_LITERAL = 271,
  CHAR_LITERAL = 272,
  ZERO = 273,
  NULLPTR = 274,
  SSIZE_T = 275,
  SIZE_T = 276,
  NULLPTR_T = 277,
  BEGIN_ATTRIB = 278,
  STRUCT = 279,
  CLASS = 280,
  UNION = 281,
  ENUM = 282,
  PUBLIC = 283,
  PRIVATE = 284,
  PROTECTED = 285,
  CONST = 286,
  VOLATILE = 287,
  MUTABLE = 288,
  STATIC = 289,
  THREAD_LOCAL = 290,
  VIRTUAL = 291,
  EXPLICIT = 292,
  INLINE = 293,
  CONSTEXPR = 294,
  FRIEND = 295,
  EXTERN = 296,
  OPERATOR = 297,
  TEMPLATE = 298,
  THROW = 299,
  TRY = 300,
  CATCH = 301,
  NOEXCEPT = 302,
  DECLTYPE = 303,
  TYPENAME = 304,
  TYPEDEF = 305,
  NAMESPACE = 306,
  USING = 307,
  NEW = 308,
  DELETE = 309,
  DEFAULT = 310,
  STATIC_CAST = 311,
  DYNAMIC_CAST = 312,
  CONST_CAST = 313,
  REINTERPRET_CAST = 314,
  OP_LSHIFT_EQ = 315,
  OP_RSHIFT_EQ = 316,
  OP_LSHIFT = 317,
  OP_RSHIFT_A = 318,
  OP_DOT_POINTER = 319,
  OP_ARROW_POINTER = 320,
  OP_ARROW = 321,
  OP_INCR = 322,
  OP_DECR = 323,
  OP_PLUS_EQ = 324,
  OP_MINUS_EQ = 325,
  OP_TIMES_EQ = 326,
  OP_DIVIDE_EQ = 327,
  OP_REMAINDER_EQ = 328,
  OP_AND_EQ = 329,
  OP_OR_EQ = 330,
  OP_XOR_EQ = 331,
  OP_LOGIC_AND = 332,
  OP_LOGIC_OR = 333,
  OP_LOGIC_EQ = 334,
  OP_LOGIC_NEQ = 335,
  OP_LOGIC_LEQ = 336,
  OP_LOGIC_GEQ = 337,
  ELLIPSIS = 338,
  DOUBLE_COLON = 339,
  OTHER = 340,
  AUTO = 341,
  VOID = 342,
  BOOL = 343,
  FLOAT = 344,
  DOUBLE = 345,
  INT = 346,
  SHORT = 347,
  LONG = 348,
  INT64__ = 349,
  CHAR = 350,
  CHAR16_T = 351,
  CHAR32_T = 352,
  WCHAR_T = 353,
  SIGNED = 354,
  UNSIGNED = 355
};
#endif

/* Value type.  */
#if !defined YYSTYPE && !defined YYSTYPE_IS_DECLARED

union YYSTYPE {

  const char* str;
  unsigned int integer;
};

typedef union YYSTYPE YYSTYPE;
#define YYSTYPE_IS_TRIVIAL 1
#define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

int yyparse(void);

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
#undef YYERROR_VERBOSE
#define YYERROR_VERBOSE 1
#else
#define YYERROR_VERBOSE 0
#endif

/* Default (constant) value used for initialization for null
   right-hand sides.  Unlike the standard yacc.c template, here we set
   the default value of $$ to a zeroed-out value.  Since the default
   value is undefined, this behavior is technically correct.  */
static YYSTYPE yyval_default;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef YY_
#if defined YYENABLE_NLS && YYENABLE_NLS
#if ENABLE_NLS
#include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#define YY_(Msgid) dgettext("bison-runtime", Msgid)
#endif
#endif
#ifndef YY_
#define YY_(Msgid) Msgid
#endif
#endif

#ifndef YYFREE
#define YYFREE free
#endif
#ifndef YYMALLOC
#define YYMALLOC malloc
#endif
#ifndef YYREALLOC
#define YYREALLOC realloc
#endif

#define YYSIZEMAX ((size_t)-1)

#ifdef __cplusplus
typedef bool yybool;
#define yytrue true
#define yyfalse false
#else
/* When we move to stdbool, get rid of the various casts to yybool.  */
typedef unsigned char yybool;
#define yytrue 1
#define yyfalse 0
#endif

#ifndef YYSETJMP
#include <setjmp.h>
#define YYJMP_BUF jmp_buf
#define YYSETJMP(Env) setjmp(Env)
/* Pacify Clang and ICC.  */
#define YYLONGJMP(Env, Val)                                                                        \
  do                                                                                               \
  {                                                                                                \
    longjmp(Env, Val);                                                                             \
    YYASSERT(0);                                                                                   \
  } while (yyfalse)
#endif

#ifndef YY_ATTRIBUTE
#if (defined __GNUC__ && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__))) ||             \
  defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#define YY_ATTRIBUTE(Spec) __attribute__(Spec)
#else
#define YY_ATTRIBUTE(Spec) /* empty */
#endif
#endif

#ifndef YY_ATTRIBUTE_PURE
#define YY_ATTRIBUTE_PURE YY_ATTRIBUTE((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
#define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE((__unused__))
#endif

/* The _Noreturn keyword of C11.  */
#if !defined _Noreturn
#if defined __cplusplus && 201103L <= __cplusplus
#define _Noreturn [[noreturn]]
#elif !(defined __STDC_VERSION__ && 201112 <= __STDC_VERSION__)
#if (3 <= __GNUC__ || (__GNUC__ == 2 && 8 <= __GNUC_MINOR__) || 0x5110 <= __SUNPRO_C)
#define _Noreturn __attribute__((__noreturn__))
#elif defined _MSC_VER && 1200 <= _MSC_VER
#define _Noreturn __declspec(noreturn)
#else
#define _Noreturn
#endif
#endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if 1
#define YYUSE(E) ((void)(E))
#else
#define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && !defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                                                        \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")             \
    _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#define YY_IGNORE_MAYBE_UNINITIALIZED_END _Pragma("GCC diagnostic pop")
#else
#define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
#define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
#define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#ifndef YYASSERT
#define YYASSERT(Condition) ((void)((Condition) || (abort(), 0)))
#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL 3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST 6587

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 124
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 277
/* YYNRULES -- Number of rules.  */
#define YYNRULES 673
/* YYNRULES -- Number of states.  */
#define YYNSTATES 1051
/* YYMAXRHS -- Maximum number of symbols on right-hand side of rule.  */
#define YYMAXRHS 8
/* YYMAXLEFT -- Maximum number of symbols to the left of a handle
   accessed by $0, $-1, etc., in any rule.  */
#define YYMAXLEFT 0

/* YYTRANSLATE(X) -- Bison symbol number corresponding to X.  */
#define YYUNDEFTOK 2
#define YYMAXUTOK 355

#define YYTRANSLATE(YYX) ((unsigned)(YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] = { 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 120, 2, 2, 2, 116, 110, 2, 107, 108, 114, 119, 106,
  118, 123, 117, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 105, 101, 109, 104, 115, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 111, 2, 112, 122, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 102, 121, 103, 113, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
  34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
  82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100 };

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] = { 0, 1797, 1797, 1799, 1801, 1800, 1811, 1812, 1813, 1814,
  1815, 1816, 1817, 1818, 1819, 1820, 1821, 1822, 1823, 1824, 1825, 1826, 1827, 1830, 1831, 1832,
  1833, 1834, 1835, 1838, 1839, 1846, 1853, 1854, 1854, 1856, 1859, 1866, 1867, 1870, 1871, 1872,
  1875, 1876, 1879, 1879, 1894, 1893, 1899, 1905, 1904, 1909, 1915, 1916, 1917, 1920, 1922, 1924,
  1927, 1928, 1931, 1932, 1934, 1936, 1935, 1944, 1948, 1949, 1950, 1953, 1954, 1955, 1956, 1957,
  1958, 1959, 1960, 1961, 1962, 1963, 1964, 1965, 1966, 1969, 1970, 1971, 1972, 1973, 1974, 1977,
  1978, 1979, 1980, 1984, 1985, 1988, 1990, 1993, 1998, 1999, 2002, 2003, 2006, 2007, 2008, 2019,
  2020, 2021, 2025, 2026, 2030, 2030, 2043, 2050, 2059, 2060, 2061, 2064, 2065, 2065, 2069, 2070,
  2072, 2073, 2074, 2074, 2082, 2086, 2087, 2090, 2092, 2094, 2095, 2098, 2099, 2107, 2108, 2111,
  2112, 2114, 2116, 2118, 2122, 2124, 2125, 2128, 2131, 2132, 2135, 2136, 2135, 2140, 2182, 2185,
  2186, 2187, 2189, 2191, 2193, 2197, 2200, 2200, 2233, 2232, 2236, 2244, 2235, 2254, 2256, 2255,
  2260, 2262, 2260, 2264, 2266, 2264, 2268, 2271, 2268, 2282, 2283, 2286, 2287, 2289, 2290, 2293,
  2293, 2303, 2304, 2312, 2313, 2314, 2315, 2318, 2321, 2322, 2323, 2326, 2327, 2328, 2331, 2332,
  2333, 2337, 2338, 2339, 2340, 2343, 2344, 2345, 2349, 2354, 2348, 2366, 2370, 2381, 2380, 2389,
  2393, 2396, 2406, 2410, 2411, 2414, 2415, 2417, 2418, 2419, 2422, 2423, 2425, 2426, 2427, 2429,
  2430, 2433, 2446, 2447, 2448, 2449, 2456, 2457, 2460, 2460, 2468, 2469, 2470, 2473, 2475, 2476,
  2480, 2479, 2496, 2520, 2492, 2531, 2531, 2534, 2535, 2538, 2539, 2542, 2543, 2549, 2550, 2550,
  2553, 2554, 2554, 2556, 2558, 2562, 2564, 2562, 2588, 2589, 2592, 2592, 2594, 2594, 2596, 2596,
  2601, 2602, 2602, 2610, 2613, 2683, 2684, 2686, 2687, 2687, 2690, 2693, 2694, 2698, 2710, 2709,
  2731, 2733, 2733, 2754, 2754, 2756, 2760, 2761, 2762, 2761, 2767, 2769, 2770, 2771, 2772, 2773,
  2774, 2777, 2778, 2782, 2783, 2787, 2788, 2791, 2792, 2796, 2797, 2798, 2799, 2802, 2803, 2806,
  2806, 2809, 2810, 2813, 2813, 2817, 2818, 2818, 2825, 2826, 2829, 2830, 2831, 2832, 2833, 2836,
  2838, 2840, 2844, 2846, 2848, 2850, 2852, 2854, 2856, 2856, 2861, 2864, 2867, 2870, 2870, 2878,
  2878, 2887, 2888, 2889, 2890, 2891, 2892, 2893, 2894, 2895, 2902, 2903, 2904, 2905, 2906, 2907,
  2913, 2914, 2917, 2918, 2920, 2921, 2924, 2925, 2928, 2929, 2930, 2931, 2934, 2935, 2936, 2937,
  2938, 2942, 2943, 2944, 2947, 2948, 2951, 2952, 2960, 2963, 2963, 2965, 2965, 2969, 2970, 2972,
  2976, 2977, 2979, 2979, 2982, 2984, 2988, 2991, 2991, 2993, 2993, 2997, 3000, 3000, 3002, 3002,
  3006, 3007, 3009, 3011, 3013, 3015, 3017, 3021, 3022, 3025, 3026, 3027, 3028, 3029, 3030, 3031,
  3032, 3033, 3036, 3037, 3038, 3039, 3040, 3041, 3042, 3043, 3044, 3045, 3046, 3047, 3048, 3049,
  3050, 3070, 3071, 3072, 3073, 3076, 3080, 3084, 3084, 3088, 3089, 3104, 3105, 3130, 3130, 3134,
  3134, 3138, 3138, 3142, 3142, 3146, 3146, 3150, 3150, 3153, 3154, 3157, 3161, 3162, 3165, 3168,
  3169, 3170, 3171, 3174, 3174, 3178, 3179, 3182, 3183, 3186, 3187, 3194, 3195, 3196, 3197, 3198,
  3199, 3200, 3201, 3202, 3203, 3204, 3205, 3208, 3209, 3210, 3211, 3212, 3213, 3214, 3215, 3216,
  3217, 3218, 3219, 3220, 3221, 3222, 3223, 3224, 3225, 3226, 3227, 3228, 3229, 3230, 3231, 3232,
  3233, 3234, 3235, 3236, 3237, 3238, 3239, 3240, 3241, 3244, 3245, 3246, 3247, 3248, 3249, 3250,
  3251, 3252, 3253, 3254, 3255, 3256, 3257, 3258, 3259, 3260, 3261, 3262, 3263, 3264, 3265, 3266,
  3267, 3268, 3269, 3270, 3271, 3272, 3273, 3276, 3277, 3278, 3279, 3280, 3281, 3282, 3283, 3284,
  3291, 3292, 3295, 3296, 3297, 3298, 3298, 3299, 3302, 3303, 3306, 3307, 3308, 3309, 3345, 3345,
  3346, 3347, 3348, 3349, 3351, 3352, 3355, 3356, 3357, 3358, 3361, 3362, 3363, 3366, 3367, 3369,
  3370, 3372, 3373, 3376, 3377, 3380, 3381, 3382, 3386, 3385, 3399, 3400, 3403, 3403, 3405, 3405,
  3409, 3409, 3411, 3411, 3413, 3413, 3417, 3417, 3422, 3423, 3425, 3426, 3429, 3430, 3433, 3434,
  3437, 3438, 3439, 3440, 3441, 3442, 3443, 3444, 3444, 3444, 3444, 3444, 3445, 3446, 3447, 3448,
  3449, 3452, 3455, 3456, 3459, 3462, 3462, 3462 };
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char* const yytname[] = { "$end", "error", "$undefined", "ID", "VTK_ID", "QT_ID",
  "StdString", "OSTREAM", "ISTREAM", "LP", "LA", "STRING_LITERAL", "INT_LITERAL", "HEX_LITERAL",
  "BIN_LITERAL", "OCT_LITERAL", "FLOAT_LITERAL", "CHAR_LITERAL", "ZERO", "NULLPTR", "SSIZE_T",
  "SIZE_T", "NULLPTR_T", "BEGIN_ATTRIB", "STRUCT", "CLASS", "UNION", "ENUM", "PUBLIC", "PRIVATE",
  "PROTECTED", "CONST", "VOLATILE", "MUTABLE", "STATIC", "THREAD_LOCAL", "VIRTUAL", "EXPLICIT",
  "INLINE", "CONSTEXPR", "FRIEND", "EXTERN", "OPERATOR", "TEMPLATE", "THROW", "TRY", "CATCH",
  "NOEXCEPT", "DECLTYPE", "TYPENAME", "TYPEDEF", "NAMESPACE", "USING", "NEW", "DELETE", "DEFAULT",
  "STATIC_CAST", "DYNAMIC_CAST", "CONST_CAST", "REINTERPRET_CAST", "OP_LSHIFT_EQ", "OP_RSHIFT_EQ",
  "OP_LSHIFT", "OP_RSHIFT_A", "OP_DOT_POINTER", "OP_ARROW_POINTER", "OP_ARROW", "OP_INCR",
  "OP_DECR", "OP_PLUS_EQ", "OP_MINUS_EQ", "OP_TIMES_EQ", "OP_DIVIDE_EQ", "OP_REMAINDER_EQ",
  "OP_AND_EQ", "OP_OR_EQ", "OP_XOR_EQ", "OP_LOGIC_AND", "OP_LOGIC_OR", "OP_LOGIC_EQ",
  "OP_LOGIC_NEQ", "OP_LOGIC_LEQ", "OP_LOGIC_GEQ", "ELLIPSIS", "DOUBLE_COLON", "OTHER", "AUTO",
  "VOID", "BOOL", "FLOAT", "DOUBLE", "INT", "SHORT", "LONG", "INT64__", "CHAR", "CHAR16_T",
  "CHAR32_T", "WCHAR_T", "SIGNED", "UNSIGNED", "';'", "'{'", "'}'", "'='", "':'", "','", "'('",
  "')'", "'<'", "'&'", "'['", "']'", "'~'", "'*'", "'>'", "'%'", "'/'", "'-'", "'+'", "'!'", "'|'",
  "'^'", "'.'", "$accept", "translation_unit", "opt_declaration_seq", "$@1", "declaration",
  "template_declaration", "explicit_instantiation", "linkage_specification", "namespace_definition",
  "$@2", "namespace_alias_definition", "forward_declaration", "simple_forward_declaration",
  "class_definition", "class_specifier", "$@3", "class_head", "$@4", "$@5", "class_key",
  "class_head_name", "class_name", "opt_final", "member_specification", "$@6",
  "member_access_specifier", "member_declaration", "template_member_declaration",
  "friend_declaration", "base_specifier_list", "base_specifier", "opt_virtual",
  "opt_access_specifier", "access_specifier", "opaque_enum_declaration", "enum_definition",
  "enum_specifier", "$@7", "enum_head", "enum_key", "opt_enum_base", "$@8", "enumerator_list",
  "enumerator_definition", "$@9", "nested_variable_initialization", "ignored_initializer",
  "ignored_class", "ignored_class_body", "typedef_declaration", "basic_typedef_declaration",
  "typedef_declarator_list", "typedef_declarator_list_cont", "typedef_declarator",
  "typedef_direct_declarator", "function_direct_declarator", "$@10", "$@11",
  "typedef_declarator_id", "using_declaration", "using_id", "using_directive", "alias_declaration",
  "$@12", "template_head", "$@13", "$@14", "$@15", "template_parameter_list", "$@16",
  "template_parameter", "$@17", "$@18", "$@19", "$@20", "$@21", "$@22", "opt_ellipsis",
  "class_or_typename", "opt_template_parameter_initializer", "template_parameter_initializer",
  "$@23", "template_parameter_value", "function_definition", "function_declaration",
  "nested_method_declaration", "nested_operator_declaration", "method_definition",
  "method_declaration", "operator_declaration", "conversion_function", "$@24", "$@25",
  "conversion_function_id", "operator_function_nr", "operator_function_sig", "$@26",
  "operator_function_id", "operator_sig", "function_nr", "function_trailer_clause",
  "func_cv_qualifier_seq", "func_cv_qualifier", "opt_noexcept_specifier", "noexcept_sig",
  "opt_ref_qualifier", "virt_specifier_seq", "virt_specifier", "opt_body_as_trailer",
  "opt_trailing_return_type", "trailing_return_type", "$@27", "function_body", "function_try_block",
  "handler_seq", "function_sig", "$@28", "structor_declaration", "$@29", "$@30", "structor_sig",
  "$@31", "opt_ctor_initializer", "mem_initializer_list", "mem_initializer",
  "parameter_declaration_clause", "$@32", "parameter_list", "$@33", "parameter_declaration", "$@34",
  "$@35", "opt_initializer", "initializer", "$@36", "$@37", "$@38", "constructor_args", "$@39",
  "variable_declaration", "init_declarator_id", "opt_declarator_list", "declarator_list_cont",
  "$@40", "init_declarator", "opt_ptr_operator_seq", "direct_abstract_declarator", "$@41",
  "direct_declarator", "$@42", "lp_or_la", "$@43", "opt_array_or_parameters", "$@44", "$@45",
  "function_qualifiers", "abstract_declarator", "declarator", "opt_declarator_id", "declarator_id",
  "bitfield_size", "opt_array_decorator_seq", "array_decorator_seq", "$@46",
  "array_decorator_seq_impl", "array_decorator", "$@47", "array_size_specifier", "$@48",
  "id_expression", "unqualified_id", "qualified_id", "nested_name_specifier", "$@49", "tilde_sig",
  "identifier_sig", "scope_operator_sig", "template_id", "$@50", "decltype_specifier", "$@51",
  "simple_id", "identifier", "opt_decl_specifier_seq", "decl_specifier2", "decl_specifier_seq",
  "decl_specifier", "storage_class_specifier", "function_specifier", "cv_qualifier",
  "cv_qualifier_seq", "store_type", "store_type_specifier", "$@52", "$@53", "type_specifier",
  "trailing_type_specifier", "$@54", "trailing_type_specifier_seq", "trailing_type_specifier_seq2",
  "$@55", "$@56", "tparam_type", "tparam_type_specifier2", "$@57", "$@58", "tparam_type_specifier",
  "simple_type_specifier", "type_name", "primitive_type", "ptr_operator_seq", "reference",
  "rvalue_reference", "pointer", "$@59", "ptr_cv_qualifier_seq", "pointer_seq",
  "decl_attribute_specifier_seq", "$@60", "id_attribute_specifier_seq", "$@61",
  "ref_attribute_specifier_seq", "$@62", "func_attribute_specifier_seq", "$@63",
  "array_attribute_specifier_seq", "$@64", "class_attribute_specifier_seq", "$@65",
  "attribute_specifier_seq", "attribute_specifier", "attribute_specifier_contents",
  "attribute_using_prefix", "attribute_list", "attribute", "$@66", "attribute_pack",
  "attribute_sig", "attribute_token", "operator_id", "operator_id_no_delim", "keyword", "literal",
  "constant_expression", "constant_expression_item", "$@67", "common_bracket_item",
  "common_bracket_item_no_scope_operator", "any_bracket_contents", "bracket_pitem",
  "any_bracket_item", "braces_item", "angle_bracket_contents", "braces_contents",
  "angle_bracket_pitem", "angle_bracket_item", "angle_brackets_sig", "$@68", "right_angle_bracket",
  "brackets_sig", "$@69", "$@70", "parentheses_sig", "$@71", "$@72", "$@73", "braces_sig", "$@74",
  "ignored_items", "ignored_expression", "ignored_item", "ignored_item_no_semi",
  "ignored_item_no_angle", "ignored_braces", "ignored_brackets", "ignored_parentheses",
  "ignored_left_parenthesis", YY_NULLPTR };
#endif

#define YYPACT_NINF -808
#define YYTABLE_NINF -627

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const short yypact[] = { -808, 98, 145, -808, -808, 1485, -808, 180, 263, 333, 382, 477, 505,
  100, 150, 225, -808, -808, -808, 312, -808, -808, -808, -808, -808, -808, -808, 66, -808, 123,
  -808, 3527, -808, -808, 6169, 546, 1274, -808, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, -808, -808, -808, -808, -808, -808, 52, -808, -808, -808, -808, -808, -808, 5875,
  -808, 202, 202, 202, 202, -808, 82, 6169, -808, 72, -808, 80, 1266, 1718, 116, 1348, 195, 281,
  -808, 120, 5973, -808, -808, -808, -808, 127, 196, -808, -808, -808, -808, -808, 216, -808, -808,
  705, 168, 3890, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, -808, -808, -808, -808, -808, -1, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, 85,
  1348, -13, 48, 176, 208, 236, 239, -808, 279, -808, -808, -808, -808, -808, 1621, 116, 116, 6169,
  127, -808, -808, -808, -808, -808, -808, -808, 271, -13, 48, 176, 208, 236, 239, -808, -808, -808,
  1348, 1348, 256, 292, -808, 1266, 1348, 116, 116, 6389, 298, 5597, -808, 6389, -808, 1120, 301,
  1348, -808, -808, -808, -808, -808, -808, 5875, -808, -808, 6071, 127, 296, -808, -808, -808,
  -808, -808, -808, -808, -808, -808, 6169, -808, -808, -808, -808, -808, -808, 68, 323, 116, 116,
  116, -808, -808, -808, -808, 120, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, -808, 1266, -808, -808, -808, -808, -808, -808, 1630, -808, 331, 81, -808, -808, -808,
  -808, -808, -808, -808, -808, 350, -808, -808, -808, 99, -808, 344, -808, -808, 1954, 2075, -808,
  -808, 219, -808, 2196, 3043, 2317, -808, -808, -808, -808, -808, -808, 6473, 5793, 6473, 1600,
  -808, -808, -808, -808, -808, -808, 1151, -808, 2438, 826, 352, -808, 346, -808, 363, -808, -808,
  -808, 5221, 1266, -808, -808, 367, -808, 127, -808, -808, -808, -808, -808, -808, 58, -808, 1408,
  659, 116, 116, 216, 375, 1537, -808, -808, -808, 368, -808, 1348, 6071, 1630, 1348, 396, 2559,
  376, 632, 705, -808, -808, -808, 85, -808, -808, -808, -808, -808, 6389, 5793, 6389, 1600, -808,
  -808, -808, -808, 189, -808, 335, -808, 1739, -808, 335, 402, -808, 1266, 223, -808, -808, -808,
  412, 432, 1151, -808, 452, 127, -808, -808, -808, -808, -808, -808, 5951, 733, 467, 308, 491,
  -808, 705, -808, 437, 3164, -808, -808, 471, -808, -808, -808, -808, 29, -808, 6267, 191, 555,
  -808, -808, -808, -808, -808, -808, -808, -808, -808, 498, -808, 127, 97, 501, 373, 6473, 6473,
  357, 416, -808, -808, -808, -808, 506, 116, -808, -808, -808, 216, 598, 497, 502, 34, -808, -808,
  503, -808, 508, -808, -808, -808, -808, -808, -808, 509, -808, -808, 198, 1416, -808, -808, 504,
  -808, -808, 116, 116, 1408, -808, -808, -808, -808, -808, -808, -808, 390, -808, -808, 6169, 516,
  -808, -808, 1266, 512, -808, 146, -808, -808, 513, 540, -808, 116, -808, -808, -808, 376, 4616,
  523, 74, 524, 368, 5951, -808, 189, -808, -808, -808, -808, 32, -808, 521, -808, -808, -808, 520,
  417, -808, -808, -808, -808, -808, 4858, -808, -808, 1674, -808, -808, 216, 189, 527, -808, 530,
  491, 345, 116, -808, 550, 85, 541, -808, -808, -808, -808, -808, 1348, 1348, 1348, -808, 116, 116,
  6169, 127, 196, -808, -808, -808, -808, 127, 191, -808, 4011, 4132, 4253, -808, 537, -808, -808,
  -808, 544, 549, -808, 196, -808, 552, -808, 548, 6169, -808, 543, 557, -808, -808, -808, -808,
  -808, -808, -808, -808, -808, 553, -808, -808, -808, 539, 554, -808, 634, 589, -808, -808, -808,
  -808, 1537, 569, -808, -808, 486, 1348, 589, 589, 2680, -808, -808, 570, -808, -808, -808, 672,
  216, 574, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808, 576, -808, -808, -808, 68, -808,
  -808, 520, -808, 372, -808, 579, 196, -808, 4737, -808, 4858, -808, -808, -808, -808, 485, -808,
  434, -808, -808, -808, -808, 705, -808, -808, -808, -808, 219, -808, -808, -808, -808, -808, 1151,
  -808, -808, -808, -808, -808, 127, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, -808, -808, 376, -808, 127, -808, -808, 5341, -808, 1348, -808, -808, -808, 1348,
  -808, 1416, -808, -808, -808, -808, 582, -808, -808, -808, -808, -808, 335, 604, 6169, -808, -808,
  296, -808, -808, -808, -808, -808, -808, 376, 577, -808, -808, -808, -808, -808, -808, 376, -808,
  5100, -808, 3648, -808, -808, -808, -808, -808, -808, -808, -808, -808, 448, -808, 584, 81, 5951,
  584, -808, 583, 592, -808, 212, 733, -808, -808, -808, -808, -808, -808, -808, -808, -808, -808,
  -808, -808, 5440, -808, 202, -808, -808, -808, 596, 323, 1266, 5538, 127, 589, 1416, 589, 554,
  4858, 3769, -808, 653, -808, -808, -808, 127, -808, 4374, 4616, 4495, 635, 594, 588, 4858, 606,
  -808, -808, -808, -808, -808, 4858, 376, 5951, -808, -808, -808, -808, -808, 607, 127, -808, 584,
  -808, -808, 5636, -808, -808, -808, -808, 5440, -808, -808, 323, 5734, -808, -808, -808, -808,
  1266, 1630, -808, -808, -808, 4858, 96, -808, -808, 600, 605, -808, -808, -808, -808, -808, -808,
  -808, 4858, -808, 4858, 613, 4979, -808, -808, -808, -808, -808, -808, -808, 1441, 202, 5734, 589,
  5734, 621, -808, -808, 622, 331, 253, -808, -808, 6365, 61, -808, -808, -808, 4979, -808, 470,
  487, 5401, -808, -808, 1441, -808, -808, 1630, -808, 624, -808, -808, -808, -808, -808, 6365,
  -808, -808, 196, -808, 216, -808, -808, -808, -808, -808, 96, 117, -808, -808, 130, -808, 5401,
  -808, 5787, -808, 2801, -808, -808, -808, 487, -808, -808, 2922, 3285, 428, 60, 5787, 144, -808,
  -808, -808, 5951, -808, -808, -808, -808, 67, 428, 5951, 3406, -808, -808 };

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short yydefact[] = { 3, 0, 4, 1, 471, 0, 483, 438, 439, 440, 435, 436, 437,
  442, 443, 441, 53, 52, 54, 114, 398, 399, 390, 393, 394, 396, 397, 395, 389, 391, 218, 0, 361,
  412, 0, 0, 0, 358, 444, 445, 446, 447, 448, 453, 454, 455, 456, 449, 450, 451, 452, 457, 458, 22,
  356, 5, 19, 20, 13, 11, 12, 9, 37, 17, 378, 44, 481, 10, 16, 378, 0, 481, 14, 135, 7, 6, 8, 0, 18,
  0, 0, 0, 0, 207, 0, 0, 15, 0, 338, 471, 0, 0, 0, 0, 471, 411, 340, 357, 0, 471, 386, 387, 388,
  179, 293, 403, 407, 410, 471, 471, 472, 116, 115, 0, 392, 0, 438, 439, 440, 435, 436, 437, 672,
  673, 583, 578, 579, 580, 577, 581, 582, 584, 585, 442, 443, 441, 642, 550, 549, 551, 570, 553,
  555, 554, 556, 557, 558, 559, 562, 563, 561, 560, 566, 569, 552, 571, 572, 564, 548, 547, 568,
  567, 523, 524, 565, 575, 574, 573, 576, 525, 526, 527, 656, 528, 529, 530, 536, 537, 531, 532,
  533, 534, 535, 538, 539, 540, 541, 542, 543, 544, 545, 546, 654, 653, 666, 642, 660, 657, 661,
  671, 164, 520, 642, 519, 514, 659, 513, 515, 516, 517, 518, 521, 522, 658, 665, 664, 655, 662,
  663, 644, 650, 652, 651, 642, 0, 0, 438, 439, 440, 435, 436, 437, 395, 391, 378, 481, 378, 481,
  471, 0, 471, 411, 0, 179, 372, 374, 373, 377, 376, 375, 642, 33, 365, 363, 364, 368, 367, 366,
  371, 370, 369, 0, 0, 0, 473, 339, 0, 0, 341, 342, 293, 0, 51, 483, 293, 110, 117, 0, 0, 26, 38,
  23, 481, 25, 27, 0, 24, 28, 0, 179, 257, 246, 642, 189, 245, 191, 192, 190, 210, 481, 0, 213, 21,
  415, 354, 197, 195, 225, 345, 0, 341, 342, 343, 59, 344, 58, 0, 348, 346, 347, 349, 414, 350, 359,
  378, 481, 378, 481, 136, 208, 0, 471, 405, 384, 301, 303, 180, 0, 289, 274, 179, 475, 475, 475,
  402, 294, 459, 460, 469, 461, 378, 434, 433, 493, 484, 0, 3, 644, 0, 0, 629, 628, 170, 162, 0, 0,
  0, 636, 638, 634, 362, 471, 392, 293, 51, 293, 117, 345, 378, 378, 151, 147, 143, 0, 146, 0, 0, 0,
  154, 0, 152, 0, 483, 156, 155, 0, 0, 383, 382, 0, 289, 179, 471, 380, 381, 62, 40, 49, 408, 471,
  0, 0, 59, 0, 482, 0, 122, 106, 118, 113, 471, 473, 0, 0, 0, 0, 0, 0, 264, 0, 0, 229, 228, 477,
  227, 255, 351, 352, 353, 617, 293, 51, 293, 117, 198, 196, 385, 378, 467, 209, 221, 473, 0, 193,
  221, 327, 473, 0, 0, 276, 286, 275, 0, 0, 0, 317, 0, 179, 464, 483, 463, 465, 462, 470, 404, 0, 0,
  493, 487, 490, 0, 3, 4, 0, 647, 649, 0, 643, 646, 648, 667, 0, 167, 0, 0, 0, 471, 668, 30, 645,
  670, 606, 606, 606, 413, 0, 143, 179, 408, 0, 471, 293, 293, 0, 327, 473, 341, 342, 32, 0, 0, 3,
  159, 160, 474, 0, 523, 524, 0, 508, 507, 0, 505, 0, 506, 217, 512, 158, 157, 42, 288, 292, 379,
  63, 0, 61, 39, 48, 57, 471, 59, 0, 0, 108, 365, 363, 364, 368, 367, 366, 0, 120, 473, 0, 112, 409,
  471, 0, 258, 259, 0, 642, 244, 0, 471, 408, 0, 233, 483, 226, 264, 0, 0, 408, 0, 471, 406, 400,
  468, 302, 223, 224, 214, 230, 222, 0, 219, 298, 328, 0, 321, 199, 194, 473, 285, 290, 0, 640, 279,
  0, 299, 318, 476, 467, 0, 153, 0, 486, 493, 499, 357, 495, 497, 4, 31, 29, 669, 168, 165, 0, 0, 0,
  429, 428, 427, 0, 179, 293, 422, 426, 181, 182, 179, 0, 163, 0, 0, 0, 138, 142, 145, 140, 112, 0,
  0, 137, 293, 148, 321, 36, 4, 0, 511, 0, 0, 510, 509, 501, 502, 66, 67, 68, 45, 471, 0, 102, 103,
  104, 100, 50, 93, 98, 179, 46, 55, 471, 111, 122, 123, 119, 105, 340, 0, 179, 179, 0, 211, 270,
  265, 266, 271, 355, 252, 478, 0, 632, 595, 624, 600, 625, 626, 630, 601, 605, 604, 599, 602, 603,
  622, 594, 623, 618, 621, 360, 596, 597, 598, 43, 41, 109, 112, 401, 232, 231, 225, 215, 333, 330,
  331, 0, 250, 0, 293, 593, 590, 591, 277, 586, 588, 589, 619, 0, 282, 304, 466, 488, 485, 492, 0,
  496, 494, 498, 35, 170, 471, 430, 431, 432, 424, 319, 171, 475, 421, 378, 174, 179, 611, 613, 614,
  637, 609, 610, 608, 612, 607, 639, 635, 139, 141, 144, 264, 34, 179, 503, 504, 0, 65, 0, 101, 471,
  99, 0, 95, 0, 56, 121, 124, 644, 0, 128, 260, 262, 261, 248, 221, 267, 0, 235, 234, 257, 256, 606,
  617, 606, 107, 477, 264, 336, 332, 324, 325, 326, 323, 322, 264, 291, 0, 587, 0, 283, 281, 305,
  300, 308, 500, 169, 166, 378, 304, 320, 183, 179, 423, 183, 177, 0, 0, 471, 391, 0, 82, 80, 71,
  77, 64, 79, 73, 72, 76, 74, 69, 70, 0, 78, 0, 204, 205, 75, 0, 338, 0, 0, 179, 179, 0, 179, 47, 0,
  127, 126, 247, 212, 269, 471, 179, 253, 0, 0, 0, 240, 0, 0, 0, 0, 592, 616, 641, 615, 620, 0, 264,
  425, 295, 185, 172, 184, 315, 0, 179, 175, 183, 149, 161, 0, 83, 85, 88, 86, 0, 84, 87, 0, 0, 200,
  81, 473, 206, 0, 0, 96, 94, 97, 125, 0, 268, 272, 236, 0, 627, 631, 242, 233, 241, 216, 479, 337,
  251, 284, 0, 0, 296, 316, 178, 309, 91, 481, 89, 0, 0, 0, 179, 0, 0, 473, 203, 0, 274, 0, 254,
  633, 0, 236, 334, 483, 306, 186, 187, 304, 150, 0, 481, 90, 0, 92, 481, 0, 201, 0, 642, 273, 239,
  237, 238, 0, 417, 243, 293, 220, 480, 309, 188, 297, 311, 310, 0, 314, 642, 644, 408, 131, 0, 481,
  0, 202, 0, 419, 378, 416, 477, 312, 313, 0, 0, 0, 60, 0, 408, 132, 249, 378, 418, 307, 644, 134,
  129, 60, 0, 420, 0, 130, 133 };

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] = { -808, -808, -303, -808, -808, 718, -69, -808, -808, -808, -808,
  -740, -76, 0, -25, -808, -808, -808, -808, 334, -291, -48, -736, -808, -808, -808, -808, -67, -63,
  -57, -150, -808, -808, 69, -47, -45, -30, -808, -808, 23, -371, -808, -808, 64, -808, -808, -808,
  -211, -691, -37, -95, -327, 255, 106, -808, -808, -808, -808, 258, -32, 291, -808, 10, -808, -3,
  -808, -808, -808, -808, -808, 5, -808, -808, -808, -808, -808, -808, 500, 128, -779, -808, -808,
  -808, 765, -808, -808, -808, -23, -149, 45, -11, -808, -808, -214, -397, -808, -808, -234, -253,
  -439, -420, -808, -808, 39, -808, -808, -176, -808, -204, -808, -808, -808, -74, -808, -808, -808,
  -808, -71, -808, -808, -808, -808, -41, -808, 87, -560, -808, -808, -808, -113, -808, -808, -193,
  -808, -808, -808, -808, -808, -808, 11, 384, -227, 387, -808, 49, -62, -567, -808, -118, -808,
  -523, -808, -807, -808, -808, -215, -808, -808, -808, -356, -808, -808, -374, -808, -808, 62,
  -808, -808, -808, 1028, 439, 1072, 71, -808, -808, 107, 709, -5, -808, 33, -808, 200, -9, -38,
  -808, 12, 738, -808, -808, -394, -808, 31, 234, -808, -808, 92, -645, -808, -808, -808, -808,
  -808, -808, -808, -808, -808, -808, 305, 178, 207, -326, 459, -808, 463, -808, 201, -808, 990,
  -808, -416, -808, -318, -808, -786, -808, -808, -808, -49, -808, -263, -808, -808, -808, 338, 203,
  -808, -808, -808, -808, -808, 143, 38, 121, -526, -701, -808, -544, -17, -444, -808, -18, -808,
  15, -808, -792, -808, -537, -808, -470, -808, -808, -808, -191, -808, -808, -808, 365, -808, -172,
  -346, -808, -353, 57, -514, -808, -504, -808 };

/* YYDEFGOTO[NTERM-NUM].  */
static const short yydefgoto[] = { -1, 1, 2, 4, 55, 279, 57, 58, 59, 387, 60, 61, 62, 281, 64, 271,
  65, 803, 544, 299, 408, 409, 547, 543, 673, 674, 864, 925, 926, 679, 680, 801, 797, 681, 67, 68,
  69, 416, 70, 282, 419, 563, 560, 561, 887, 283, 808, 966, 1019, 72, 73, 505, 513, 506, 380, 381,
  790, 963, 382, 74, 263, 75, 284, 661, 285, 496, 362, 764, 491, 763, 492, 493, 850, 494, 853, 495,
  920, 769, 642, 914, 915, 959, 985, 286, 79, 80, 81, 929, 874, 875, 83, 428, 814, 84, 449, 450,
  826, 451, 85, 453, 592, 593, 594, 433, 434, 734, 702, 818, 978, 951, 952, 980, 293, 294, 890, 454,
  834, 876, 819, 946, 307, 580, 426, 568, 569, 573, 574, 698, 893, 699, 816, 976, 460, 461, 606,
  462, 463, 751, 909, 287, 338, 399, 458, 742, 400, 401, 770, 987, 339, 753, 340, 448, 842, 910,
  1009, 988, 917, 466, 848, 455, 833, 597, 843, 599, 737, 738, 827, 901, 902, 682, 88, 238, 239,
  430, 91, 92, 93, 268, 439, 269, 224, 96, 97, 270, 402, 300, 99, 100, 101, 102, 588, 882, 104, 350,
  447, 105, 106, 225, 1005, 1006, 1026, 1039, 636, 637, 773, 847, 638, 107, 108, 109, 345, 346, 347,
  348, 613, 589, 349, 565, 6, 391, 392, 468, 469, 577, 578, 982, 983, 272, 273, 110, 354, 476, 477,
  478, 479, 480, 760, 621, 622, 535, 715, 716, 717, 746, 747, 836, 748, 719, 645, 783, 784, 908,
  581, 838, 720, 721, 749, 822, 363, 724, 823, 821, 725, 503, 501, 502, 726, 750, 358, 365, 487,
  488, 489, 220, 221, 222, 223 };

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const short yytable[] = { 94, 280, 77, 328, 236, 63, 295, 296, 297, 234, 414, 483, 499, 395,
  467, 76, 86, 98, 602, 306, 704, 628, 276, 359, 470, 471, 251, 514, 71, 240, 364, 274, 394, 370,
  596, 595, 103, 718, 95, 899, 600, 912, 344, 312, 722, 837, 242, 415, 509, 564, 82, 366, 393, 482,
  587, 862, 693, 237, 646, 647, 601, 545, 360, 545, 817, 243, 694, 241, 326, 217, 545, -372, 240,
  324, 919, 774, 90, 545, 385, 305, 240, 598, 316, 508, 452, 310, 313, 329, 219, 288, 334, 335, 360,
  240, 367, 368, -372, 665, 3, 658, 545, 782, 782, 782, 355, 122, 123, 266, 289, 732, 241, 723, 431,
  771, 361, 432, 301, 113, 241, 771, 427, 327, 82, 311, 314, 384, 122, 123, 602, 524, 499, 241,
  -374, 545, 114, 627, 334, 335, 575, 652, 598, 962, 733, 395, 361, -2, 688, 545, 90, 666, 583, 475,
  218, 217, 275, 122, 123, -374, 341, 546, -60, 546, 538, -60, 336, 977, 115, 986, 728, 330, 601,
  452, 357, 302, 216, 728, -60, 809, 623, -60, 1011, -339, 537, 964, 653, 654, 374, 741, 376, 298,
  332, 342, 369, 1010, 731, 343, 373, 309, 375, -60, 37, -371, -60, 199, 452, -489, 612, -371, 854,
  214, 336, -489, 379, 582, 730, 584, 640, 378, 660, 587, 20, 21, 465, 372, 199, 856, 669, 670, 671,
  323, 855, 546, -60, 422, 310, -60, 218, 240, 215, 353, 641, 837, 1041, 579, -173, 728, -60, 290,
  195, -60, 429, -370, 771, 199, 837, 115, 837, -370, 216, 445, -373, 310, -176, 324, -372, 237,
  900, 412, -173, 771, 356, 1000, 311, 341, 903, 241, 441, 329, 443, 37, 240, -365, 1033, 240, 916,
  -373, 440, -365, 442, -372, 372, 315, -377, 214, 782, 240, -341, 423, 1033, 311, 1046, 672, -341,
  291, 292, 413, 342, 1001, 1002, 1046, 343, 507, 474, 507, 452, 703, 289, -377, 241, 444, -376,
  241, 215, -375, 604, 310, -369, 945, 771, 605, 82, 1037, -369, 241, 332, 1004, 111, 112, 396, 66,
  511, 512, 1044, 410, 1037, -376, 309, -374, -375, 611, 958, 961, 782, 718, 782, 1049, 90, 390,
  1025, 330, 722, 942, 549, 311, -363, 37, 590, 591, 235, 412, -363, 771, -374, 309, 316, 386, 955,
  896, 516, 898, 332, 520, -342, 957, 829, 830, 831, 832, -342, 384, 240, 310, 332, -153, 891, 771,
  217, 217, 324, 695, 405, 425, 217, 217, 217, 550, 520, 413, 457, 586, 200, 66, 329, 849, -489,
  718, 517, -373, 240, 413, -489, 316, 722, 217, 412, 576, 301, 948, 241, 311, 435, 761, 325, -278,
  -364, 459, 412, 309, -280, 975, -364, 718, -373, 551, 413, 410, 481, 522, 722, 507, 507, -491,
  310, 851, 521, 316, 241, -491, 655, 516, 342, 888, 413, 656, 343, 217, -377, 523, 539, -117, -117,
  620, 315, 418, 413, -117, 553, 404, 418, 218, 218, 404, 552, -368, -263, 218, 218, 218, 633, -368,
  311, -377, 643, 686, 330, 567, 687, 517, 571, 974, 410, 216, 216, 309, 685, 635, 218, 216, 216,
  216, 1029, 1030, 410, -329, 607, 332, 631, 309, 1012, 972, 1013, 217, 739, 657, 740, 918, 634,
  -329, 216, 308, 1016, 1014, 318, 1017, 1015, 499, 214, 214, 536, 609, 624, 841, 214, 214, 214,
  -329, 266, 550, 218, 244, 245, 246, 247, 248, 249, 841, 974, 997, 240, -329, 610, -376, 310, 214,
  309, 215, 215, 675, 676, 677, 216, 215, 215, 215, 315, 772, 235, 841, -367, 616, 404, -329, 404,
  626, -367, 551, -376, 619, 195, -375, 807, 839, 215, 840, 507, 536, 241, 618, 277, 648, 311, 662,
  651, 337, 218, 214, -366, 659, 663, 683, 315, 667, -366, 664, -375, 605, 562, 690, 692, 235, 668,
  696, 325, 697, 727, 729, 216, 452, 735, 315, 240, 736, 755, 759, 215, 252, 253, 254, 255, 256,
  257, 315, 756, 656, 762, 787, 935, 404, 250, 404, 788, 791, 258, 259, 260, 793, 240, 739, 796,
  309, 799, 214, 244, 245, 246, 247, 248, 249, 241, 794, 800, 1032, 336, 806, 309, 817, 815, 824,
  377, 499, 32, 404, 820, 889, 452, 452, 740, 892, 913, -335, 215, 921, 792, 922, 241, 499, 1048,
  934, 1018, 943, 954, 950, 953, 973, 977, 377, 318, 32, 244, 245, 246, 247, 248, 249, 956, 960, 37,
  979, 404, 404, 1008, 984, 996, 56, 998, 1023, 861, 768, 865, 1034, 752, 1036, 866, 217, 940, 852,
  252, 253, 254, 255, 256, 257, 452, 383, 1045, 267, 886, 798, 867, 620, 868, 805, 315, 258, 259,
  260, 991, 235, 869, 713, 316, 649, 789, 870, 516, 650, 614, 691, 845, 308, 78, 775, 873, 825, 968,
  981, 456, 1007, 895, 810, 944, 32, 261, 999, 713, 541, 328, 540, 714, 424, 94, 835, 872, 404,
  1028, 863, 280, 689, 639, 828, 933, 317, 517, 320, 322, 871, 877, 881, 472, 911, 936, 240, 473,
  714, 754, 617, 218, 37, 71, 904, 907, 757, 515, 713, 713, 713, 1024, 608, 95, 244, 245, 246, 247,
  248, 249, 377, 331, 897, 216, 0, 464, 0, 0, 0, 1031, 0, 54, 894, 0, 241, 0, 326, 714, 714, 714, 0,
  324, 0, 0, 0, 0, 936, 0, 456, 0, 844, 880, 94, 0, 928, 329, 0, 924, 214, 32, 310, 94, 0, 0, 0, 0,
  0, 927, 930, 932, 0, 0, 562, 0, 0, 0, 280, 0, 0, 993, 0, 603, 0, 936, 0, 936, 464, 215, 515, 327,
  95, 0, 324, 0, 0, 37, 0, 0, 311, 95, 0, 989, 0, 94, 0, 967, 329, 0, 94, 713, 928, 217, 94, 924,
  316, 266, 0, 310, 516, 0, 969, 0, 0, 927, 930, 971, 1020, 0, 880, 1027, 1022, 324, 0, 0, 320, 322,
  0, 937, 714, 970, 0, 95, 0, 329, 0, 329, 95, 0, 0, 94, 95, 94, 464, 0, 0, 311, 517, 1035, 332, 0,
  240, 0, 320, 322, 992, 331, 411, 0, 0, 412, 0, 316, 309, 1040, 0, 516, 0, 1003, 0, 5, 0, 0, 0,
  240, 880, 0, 1047, 95, 937, 95, 0, 603, 383, 403, 218, 0, 0, 403, 241, 713, 412, 713, 412, 436,
  437, 438, 0, 413, 0, 332, 0, 331, 0, 517, 0, 412, 216, 0, 87, 0, 0, 241, 0, 331, 0, 0, 0, 937,
  714, 309, 714, 0, 0, 0, 0, 0, 0, 0, 413, 0, 413, 0, 0, 0, 404, 410, 332, 217, 332, 264, 713, 214,
  0, 413, 217, 217, 0, 0, 0, 713, 713, 713, 89, 0, 303, 713, 0, 0, 411, 321, 0, 217, 713, 0, 333, 0,
  410, 0, 410, 714, 215, 0, 0, 351, 352, 0, 0, 0, 714, 714, 714, 410, 0, 265, 714, 0, 403, 0, 403,
  0, 0, 714, 0, 404, 0, 713, 320, 322, 252, 253, 254, 255, 256, 257, 66, 0, 0, 0, 713, 0, 713, 0,
  713, 411, 0, 258, 259, 260, 0, 0, 218, 0, 0, 0, 714, 411, 0, 218, 218, 252, 253, 254, 255, 256,
  257, 0, 331, 714, 713, 714, 319, 714, 216, 32, 218, 0, 258, 259, 260, 216, 216, 0, 0, 403, 0, 403,
  0, 802, 0, 267, 0, 0, 0, 0, 315, 0, 216, 714, 811, 812, 0, 0, 0, 0, 32, 0, 0, 214, 0, 37, 0, 66,
  0, 515, 214, 214, 0, 403, 0, 0, 325, 0, 0, 0, 0, 315, 417, 315, 0, 0, 418, 214, 0, 303, 320, 321,
  215, 0, 54, 879, 315, 0, 0, 215, 215, 0, 0, 0, 0, 0, 0, 0, 404, 0, 403, 403, 0, 0, 371, 404, 215,
  0, 965, 0, 437, 438, 0, 235, 0, 54, 0, 325, 0, 0, 252, 253, 254, 255, 256, 257, 0, 0, 252, 253,
  254, 255, 256, 257, 0, 0, 701, 258, 259, 260, 388, 389, 0, 0, 0, 258, 259, 260, 265, 0, 0, 0, 990,
  0, 994, 420, 235, 421, 0, 30, 304, 0, 931, 0, 0, 32, 0, 0, 0, 0, 377, 931, 446, 32, 261, 403, 262,
  1021, 0, 758, 0, 0, 0, 0, 265, 265, 0, 0, 0, 0, 319, 0, 0, 320, 322, 0, 0, 0, 0, 265, 0, 265, 252,
  253, 254, 255, 256, 257, 0, 37, 0, 0, 504, 931, 0, 0, 0, 0, 931, 258, 259, 260, 931, 0, 331, 0, 0,
  377, 456, 0, 54, 0, 0, 938, 939, 0, 941, 0, 54, 0, 0, 0, 0, 542, 0, 0, 0, 32, 0, 0, 548, 0, 0, 0,
  0, 510, 0, 0, 0, 931, 0, 931, 252, 253, 254, 255, 256, 257, 0, 0, 252, 253, 254, 255, 256, 257, 0,
  0, 0, 258, 259, 260, 0, 37, 0, 603, 0, 258, 259, 260, 0, 0, 0, 0, 0, 675, 676, 677, 0, 265, 0,
  566, 304, 678, 570, 0, 0, 32, 0, 519, 0, 0, 54, 0, 0, 32, 16, 17, 18, 0, 0, 995, 585, 20, 21, 22,
  23, 24, 25, 26, 232, 28, 0, 233, 0, 0, 0, 644, 0, 7, 8, 9, 10, 11, 12, 265, 0, 0, 265, 0, 0, 37,
  0, 0, 615, 0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 265, 20, 21, 22, 23, 24, 25, 26, 27, 28, 0, 29,
  30, 31, 54, 0, 0, 0, 32, 33, 34, 35, 36, 0, 684, 554, 555, 556, 557, 558, 559, 0, 265, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 258, 259, 260, 0, 0, 0, 0, 700, 632, 0, 0, 267, 37, 0, 38, 39, 40, 41, 42, 43, 44,
  45, 46, 47, 48, 49, 50, 51, 52, 53, 0, 0, 0, 403, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0, 0, 252, 253,
  254, 255, 256, 257, 0, 0, 0, 0, 0, 0, 0, 265, 0, 0, 331, 258, 259, 260, 0, 252, 253, 254, 255,
  256, 257, 0, 0, 0, 252, 253, 254, 255, 256, 257, 0, 0, 258, 259, 260, 0, 0, 0, 0, 32, 403, 258,
  259, 260, 0, 0, 0, 0, 765, 766, 767, 0, 0, 0, 795, 304, 0, 0, 0, 0, 32, 331, 0, 30, 0, 0, 804, 0,
  0, 32, 0, 0, 0, 0, 0, 37, 124, 125, 126, 127, 128, 129, 130, 131, 132, 0, 0, 0, 0, 411, 0, 0, 265,
  265, 265, 0, 418, 0, 331, 0, 331, 0, 0, 0, 54, 0, 0, 0, 0, 0, 0, 570, 252, 253, 254, 255, 256,
  257, 0, 0, 411, 331, 411, 0, 0, 54, 0, 0, 0, 258, 259, 260, 331, 0, 54, 411, 0, 0, 0, 0, 0, 525,
  0, 0, 0, 846, 0, 0, 0, 0, 0, 0, 0, 0, 0, 265, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 403, 0, 0,
  0, 0, 0, 0, 403, 0, 0, 0, 884, 0, 0, 526, 527, 0, 0, 0, 0, 0, 169, 170, 171, 528, 173, 174, 175,
  176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 878, 0, 883, 0,
  0, 0, 885, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 529, 0, 530, 531, 923, 532, 201, 533, 0, 203,
  204, 534, 206, 207, 208, 209, 210, 211, 212, 0, 0, 0, 0, 0, 89, 0, 265, 0, 0, 0, 265, 0, 265, 0,
  0, 0, 0, 0, 0, 0, 700, 0, 0, 0, 264, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 265, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 265, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 484, 195, 0, 196, 197, 198, 199, 0, 485, 201, 202, 486, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 484, 195, 490, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 484, 195, 0, 196, 197, 198, 199, 0, 485, 201, 202, 497, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 484, 195, 0, 196, 197, 198, 199, 500, 485, 201, 202, 0, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 484, 195, 518, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 484, 195, 572, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 484, 195, 813, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 484, 195, 1038, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 484, 195, 1042, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 498, 195, 0, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 625, 195, 0, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 1043, 195, 0, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 1050, 195, 0, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203,
  204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
  180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 195, 0, 196, 197, 198, 199, 0, 200, 201, 202, 0, 203, 204,
  205, 206, 207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 367, 368, 124, 125,
  126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 705, 137, 138, 139, 140, 141, 142, 143, 144,
  145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161,
  162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 776, 173, 174, 175, 176, 177, 178, 179, 180,
  181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 706, 0, 38, 39, 40, 41, 42, 43, 44, 45,
  46, 47, 48, 49, 50, 51, 52, 905, 607, 906, 777, 708, 778, 369, 0, 780, 201, 711, 0, 203, 204, 781,
  206, 207, 208, 209, 210, 211, 212, 712, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
  127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0,
  146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163,
  164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182,
  183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46,
  47, 48, 49, 50, 51, 52, 0, 195, 0, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206,
  207, 208, 209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
  128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146,
  147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164,
  165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183,
  184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
  49, 50, 51, 52, 0, 195, 0, 196, 197, 198, 199, 0, 0, 201, 202, 0, 203, 204, 205, 206, 207, 208,
  209, 210, 211, 212, 213, 116, 117, 118, 119, 120, 121, 367, 368, 124, 125, 126, 127, 128, 129,
  130, 131, 132, 133, 134, 135, 705, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148,
  149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166,
  167, 168, 169, 170, 171, 776, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185,
  186, 187, 188, 189, 190, 191, 0, 706, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
  52, 0, 607, 0, 777, 708, 778, 369, 779, 780, 201, 711, 0, 203, 204, 781, 206, 207, 208, 209, 210,
  211, 212, 712, 116, 117, 118, 119, 120, 121, 367, 368, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 705, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 776, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 0, 706, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 607,
  0, 777, 708, 778, 369, 785, 780, 201, 711, 0, 203, 204, 781, 206, 207, 208, 209, 210, 211, 212,
  712, 116, 117, 118, 119, 120, 121, 367, 368, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 705, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 776, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 0, 706, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 607, 0,
  777, 708, 778, 369, 786, 780, 201, 711, 0, 203, 204, 781, 206, 207, 208, 209, 210, 211, 212, 712,
  116, 117, 118, 119, 120, 121, 367, 368, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
  135, 705, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152,
  153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 776, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
  190, 191, 0, 706, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 607, 0, 777,
  708, 778, 369, 0, 780, 201, 711, 947, 203, 204, 781, 206, 207, 208, 209, 210, 211, 212, 712, 116,
  117, 118, 119, 120, 121, 367, 368, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
  705, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153,
  154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171,
  776, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
  191, 0, 706, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 607, 0, 777, 708,
  778, 369, 0, 780, 201, 711, 949, 203, 204, 781, 206, 207, 208, 209, 210, 211, 212, 712, 116, 117,
  118, 119, 120, 121, 367, 368, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 705,
  137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154,
  155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 360,
  173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0,
  706, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 607, 0, 707, 708, 709, 369,
  0, 710, 201, 711, 0, 203, 204, 361, 206, 207, 208, 209, 210, 211, 212, 712, -590, -590, -590,
  -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590,
  -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, 0, -590, -590, -590, -590, -590,
  -590, 0, -590, -590, -590, -590, 0, 0, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590,
  -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590,
  -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, 0, -590, 0, -590, -590, -590, -590,
  -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, 0, -590, 0, -626, -590, -590,
  -590, 0, -590, -590, -590, 0, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590,
  116, 117, 118, 119, 120, 121, 367, 368, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
  135, 705, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152,
  153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 743, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
  190, 191, 0, 706, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 607, 0, 0,
  708, 0, 369, 0, 744, 201, 711, 0, 203, 204, 745, 206, 207, 208, 209, 210, 211, 212, 712, 116, 117,
  118, 119, 120, 121, 367, 368, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 705,
  137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154,
  155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 0, 173,
  174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 706,
  0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 607, 0, 0, 708, 0, 369, 0, 710,
  201, 711, 0, 203, 204, 0, 206, 207, 208, 209, 210, 211, 212, 712, 116, 117, 118, 119, 120, 121,
  367, 368, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 705, 137, 138, 139, 140,
  141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157,
  158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 0, 173, 174, 175, 176, 177,
  178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 0, 0, 38, 39, 40, 41, 42,
  43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 607, 0, 0, 708, 0, 369, 0, 0, 201, 711, 0, 203, 204, 0,
  206, 207, 208, 209, 210, 211, 212, 712, 226, 227, 228, 229, 230, 231, 0, 0, 525, 0, 0, 0, 0, 0, 0,
  0, 0, 133, 134, 135, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 0,
  0, 0, 0, 0, 0, 32, 33, 0, 0, 0, 526, 527, 0, 0, 0, 0, 0, 169, 170, 171, 528, 173, 174, 175, 176,
  177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 37, 0, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 0, 0, 529, 0, 530, 531, 0, 532, 201, 533, 0,
  203, 204, 534, 206, 207, 208, 209, 210, 211, 212, 7, 8, 9, 10, 11, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 857, 858, 30,
  31, 0, 0, 0, 0, 32, 33, 34, 0, 859, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 252, 253, 254, 255, 256, 257, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 258, 259, 260, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
  49, 50, 51, 52, 860, 7, 8, 9, 10, 11, 12, 32, 0, 0, 0, 0, 54, 0, 0, 0, 0, 0, 13, 14, 15, 0, 16,
  17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 857, 233, 30, 277, 0, 37, 0, 0, 32, 33,
  0, 0, 278, 0, 0, 0, 0, 0, 0, 0, 0, 0, 406, 1016, 0, 0, 1017, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 7, 8, 9, 10, 11,
  12, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0, 0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24,
  25, 26, 232, 28, 0, 233, 30, 0, 0, 0, 0, 0, 32, 33, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 252, 253,
  254, 255, 256, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 258, 259, 260, 0, 0, 37, 0, 38, 39, 40, 41,
  42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 7, 8, 9, 10, 11, 12, 32, 0, 0, 0, 0, 0, 54, 0, 0, 0,
  0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 277,
  0, 37, 0, 0, 32, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 406, 0, 0, 0, 407, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
  7, 8, 9, 10, 11, 12, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0, 0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20,
  21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 0, 0, 0, 0, 0, 32, 33, 0, 0, 0, 0, 0, 0, 252, 253,
  254, 255, 256, 257, 252, 253, 254, 255, 256, 257, 0, 0, 0, 0, 0, 258, 259, 260, 0, 0, 0, 258, 259,
  260, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 32, 0, 0, 0, 0, 0,
  32, 0, 0, 0, 0, 0, 54, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0,
  0, 0, 0, 0, 37, 226, 227, 228, 229, 230, 231, 0, 0, 0, 0, 0, 1016, 0, 0, 1017, 0, 0, 133, 134,
  135, 407, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 277, 0, 0, 0,
  0, 32, 33, 0, 0, 278, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 226, 227,
  228, 229, 230, 231, 20, 21, 22, 23, 24, 25, 26, 232, 28, 397, 233, 133, 134, 135, 0, 16, 17, 18,
  19, 398, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 0, 0, 0, 0, 0, 32, 33, 34, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 0,
  0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 226, 227, 228, 229,
  230, 231, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 133, 134, 135, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22,
  23, 24, 25, 26, 232, 28, 0, 233, 30, 0, 0, 0, 0, 0, 32, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44,
  45, 46, 47, 48, 49, 50, 51, 52, 226, 227, 228, 229, 230, 231, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  133, 134, 135, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 0, 0, 0,
  0, 0, 0, 32, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 226, 227,
  228, 229, 230, 231, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 133, 134, 135, 0, 629, 0, 630, 19, 0, 0, 0,
  20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41,
  42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 226, 227, 228, 229, 230, 231, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 133, 134, 135, 0, 0, 0, 0, 0, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 0, 0,
  0, 0, 0, 0, 32, 33, 0, 0, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 397, 233, 0, 0, 0, 0, 0,
  0, 0, 0, 398, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
  50, 51, 52, 341, 0, 0, 0, 0, 0, 0, 0, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
  52, -287, 0, 0, 0, 0, 0, 0, 0, 0, 342, 0, 0, 0, 343, 20, 21, 22, 23, 24, 25, 26, 232, 28, 397,
  233, 0, 0, 0, 0, 0, 0, 0, 0, 398, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 341, 0, 0, 0, 0, 0, 0, 0, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
  51, 52, 0, 0, 0, 0, 0, 0, 0, 0, 0, 342, 0, 0, 0, 343 };

static const short yycheck[] = { 5, 77, 5, 98, 34, 5, 80, 81, 82, 34, 273, 357, 365, 266, 340, 5, 5,
  5, 457, 90, 580, 491, 71, 195, 342, 343, 35, 383, 5, 34, 202, 69, 266, 224, 454, 451, 5, 581, 5,
  825, 456, 848, 104, 91, 581, 746, 34, 274, 375, 420, 5, 223, 266, 356, 448, 795, 570, 34, 502,
  503, 457, 3, 63, 3, 3, 34, 570, 34, 98, 31, 3, 84, 77, 98, 853, 642, 5, 3, 250, 90, 85, 455, 91,
  374, 337, 90, 91, 98, 31, 77, 9, 10, 63, 98, 9, 10, 109, 63, 0, 515, 3, 645, 646, 647, 113, 9, 10,
  36, 77, 77, 77, 581, 44, 636, 115, 47, 85, 51, 85, 642, 292, 98, 77, 90, 91, 243, 9, 10, 567, 392,
  483, 98, 84, 3, 11, 106, 9, 10, 429, 510, 514, 920, 110, 396, 115, 0, 562, 3, 77, 115, 441, 52,
  31, 115, 102, 9, 10, 109, 77, 101, 102, 101, 396, 105, 83, 104, 43, 959, 101, 98, 567, 424, 115,
  101, 31, 101, 102, 691, 481, 105, 987, 101, 396, 923, 511, 512, 235, 603, 237, 107, 98, 110, 107,
  985, 588, 114, 234, 90, 236, 102, 84, 101, 105, 107, 457, 106, 469, 107, 775, 31, 83, 112, 242,
  440, 585, 442, 25, 242, 521, 613, 31, 32, 340, 11, 107, 792, 28, 29, 30, 109, 790, 101, 102, 282,
  239, 105, 115, 242, 31, 23, 49, 942, 1028, 434, 25, 101, 102, 45, 102, 105, 299, 101, 775, 107,
  955, 43, 957, 107, 115, 330, 84, 266, 43, 288, 84, 242, 826, 272, 49, 792, 102, 18, 239, 77, 834,
  242, 325, 288, 327, 84, 285, 101, 1018, 288, 851, 109, 324, 107, 326, 109, 11, 91, 84, 115, 838,
  300, 101, 285, 1034, 266, 1036, 103, 107, 101, 102, 272, 110, 54, 55, 1045, 114, 373, 350, 375,
  567, 578, 285, 109, 285, 330, 84, 288, 115, 84, 101, 330, 101, 894, 851, 106, 285, 1022, 107, 300,
  242, 980, 24, 25, 267, 5, 378, 379, 1033, 272, 1035, 109, 239, 84, 109, 467, 910, 918, 896, 897,
  898, 1046, 285, 101, 1003, 288, 897, 887, 410, 330, 101, 84, 31, 32, 34, 374, 107, 894, 109, 266,
  383, 104, 902, 821, 383, 823, 288, 386, 101, 909, 12, 13, 14, 15, 107, 507, 395, 396, 300, 101,
  814, 918, 358, 359, 423, 571, 102, 105, 364, 365, 366, 410, 411, 374, 337, 447, 109, 77, 423, 769,
  106, 959, 383, 84, 423, 386, 112, 430, 959, 385, 429, 430, 395, 897, 395, 396, 107, 622, 98, 102,
  101, 104, 441, 330, 107, 943, 107, 985, 109, 410, 411, 374, 102, 101, 985, 511, 512, 106, 457,
  771, 102, 464, 423, 112, 101, 464, 110, 807, 429, 106, 114, 427, 84, 104, 101, 101, 102, 480, 272,
  105, 441, 102, 101, 270, 105, 358, 359, 274, 411, 101, 108, 364, 365, 366, 493, 107, 457, 109,
  495, 103, 423, 424, 106, 464, 102, 938, 429, 358, 359, 396, 552, 493, 385, 364, 365, 366, 1014,
  1015, 441, 111, 102, 423, 493, 410, 31, 935, 33, 483, 105, 107, 107, 851, 493, 111, 385, 90, 102,
  44, 93, 105, 47, 888, 358, 359, 395, 107, 103, 107, 364, 365, 366, 111, 475, 552, 427, 3, 4, 5, 6,
  7, 8, 107, 995, 973, 563, 111, 108, 84, 567, 385, 457, 358, 359, 28, 29, 30, 427, 364, 365, 366,
  374, 637, 242, 107, 101, 112, 373, 111, 375, 112, 107, 552, 109, 480, 102, 84, 104, 106, 385, 108,
  656, 452, 563, 106, 43, 101, 567, 3, 101, 103, 483, 427, 101, 101, 111, 105, 410, 108, 107, 111,
  109, 106, 416, 101, 106, 285, 112, 108, 288, 83, 101, 101, 483, 880, 107, 429, 635, 111, 105, 83,
  427, 3, 4, 5, 6, 7, 8, 441, 112, 106, 103, 101, 880, 440, 102, 442, 101, 103, 20, 21, 22, 112,
  661, 105, 105, 552, 106, 483, 3, 4, 5, 6, 7, 8, 635, 112, 36, 1017, 83, 104, 567, 3, 106, 101,
  239, 1032, 48, 474, 108, 101, 937, 938, 107, 83, 104, 112, 483, 108, 661, 101, 661, 1048, 1042,
  101, 989, 46, 112, 66, 108, 937, 104, 266, 267, 48, 3, 4, 5, 6, 7, 8, 108, 108, 84, 112, 511, 512,
  983, 108, 101, 5, 102, 101, 795, 635, 795, 1020, 609, 1022, 795, 695, 884, 773, 3, 4, 5, 6, 7, 8,
  995, 243, 1035, 36, 803, 678, 795, 758, 795, 687, 552, 20, 21, 22, 967, 423, 795, 581, 769, 506,
  656, 795, 769, 507, 475, 567, 763, 330, 5, 643, 795, 734, 923, 951, 337, 981, 819, 692, 893, 48,
  49, 976, 606, 401, 881, 400, 581, 289, 795, 742, 795, 586, 1009, 795, 872, 563, 493, 737, 874, 92,
  769, 94, 95, 795, 795, 795, 349, 847, 881, 816, 349, 606, 613, 477, 695, 84, 795, 836, 838, 618,
  383, 645, 646, 647, 998, 462, 795, 3, 4, 5, 6, 7, 8, 396, 98, 822, 695, -1, 340, -1, -1, -1, 1016,
  -1, 113, 816, -1, 816, -1, 881, 645, 646, 647, -1, 881, -1, -1, -1, -1, 932, -1, 424, -1, 758,
  795, 872, -1, 872, 881, -1, 872, 695, 48, 880, 881, -1, -1, -1, -1, -1, 872, 872, 872, -1, -1,
  687, -1, -1, -1, 967, -1, -1, 968, -1, 457, -1, 969, -1, 971, 401, 695, 464, 881, 872, -1, 932,
  -1, -1, 84, -1, -1, 880, 881, -1, 965, -1, 923, -1, 923, 932, -1, 928, 746, 928, 888, 932, 928,
  938, 859, -1, 937, 938, -1, 923, -1, -1, 928, 928, 928, 990, -1, 872, 1006, 994, 971, -1, -1, 240,
  241, -1, 881, 746, 923, -1, 923, -1, 969, -1, 971, 928, -1, -1, 969, 932, 971, 467, -1, -1, 937,
  938, 1021, 881, -1, 980, -1, 268, 269, 967, 242, 272, -1, -1, 989, -1, 995, 880, 1026, -1, 995,
  -1, 980, -1, 4, -1, -1, -1, 1003, 928, -1, 1039, 969, 932, 971, -1, 567, 507, 270, 888, -1, -1,
  274, 980, 836, 1020, 838, 1022, 309, 310, 311, -1, 989, -1, 932, -1, 288, -1, 995, -1, 1035, 888,
  -1, 5, -1, -1, 1003, -1, 300, -1, -1, -1, 971, 836, 937, 838, -1, -1, -1, -1, -1, -1, -1, 1020,
  -1, 1022, -1, -1, -1, 852, 989, 969, 1024, 971, 36, 887, 888, -1, 1035, 1031, 1032, -1, -1, -1,
  896, 897, 898, 5, -1, 89, 902, -1, -1, 374, 94, -1, 1048, 909, -1, 99, -1, 1020, -1, 1022, 887,
  888, -1, -1, 108, 109, -1, -1, -1, 896, 897, 898, 1035, -1, 36, 902, -1, 373, -1, 375, -1, -1,
  909, -1, 911, -1, 942, 412, 413, 3, 4, 5, 6, 7, 8, 795, -1, -1, -1, 955, -1, 957, -1, 959, 429,
  -1, 20, 21, 22, -1, -1, 1024, -1, -1, -1, 942, 441, -1, 1031, 1032, 3, 4, 5, 6, 7, 8, -1, 423,
  955, 985, 957, 93, 959, 1024, 48, 1048, -1, 20, 21, 22, 1031, 1032, -1, -1, 440, -1, 442, -1, 682,
  -1, 475, -1, -1, -1, -1, 989, -1, 1048, 985, 693, 694, -1, -1, -1, -1, 48, -1, -1, 1024, -1, 84,
  -1, 872, -1, 769, 1031, 1032, -1, 474, -1, -1, 881, -1, -1, -1, -1, 1020, 101, 1022, -1, -1, 105,
  1048, -1, 238, 520, 240, 1024, -1, 113, 795, 1035, -1, -1, 1031, 1032, -1, -1, -1, -1, -1, -1, -1,
  1040, -1, 511, 512, -1, -1, 225, 1047, 1048, -1, 923, -1, 550, 551, -1, 928, -1, 113, -1, 932, -1,
  -1, 3, 4, 5, 6, 7, 8, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, 576, 20, 21, 22, 261, 262, -1, -1, -1, 20,
  21, 22, 225, -1, -1, -1, 967, -1, 969, 276, 971, 278, -1, 42, 43, -1, 872, -1, -1, 48, -1, -1, -1,
  -1, 880, 881, 331, 48, 49, 586, 51, 992, -1, 619, -1, -1, -1, -1, 261, 262, -1, -1, -1, -1, 267,
  -1, -1, 633, 634, -1, -1, -1, -1, 276, -1, 278, 3, 4, 5, 6, 7, 8, -1, 84, -1, -1, 371, 923, -1,
  -1, -1, -1, 928, 20, 21, 22, 932, -1, 635, -1, -1, 937, 938, -1, 113, -1, -1, 882, 883, -1, 885,
  -1, 113, -1, -1, -1, -1, 402, -1, -1, -1, 48, -1, -1, 409, -1, -1, -1, -1, 376, -1, -1, -1, 969,
  -1, 971, 3, 4, 5, 6, 7, 8, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, -1, 20, 21, 22, -1, 84, -1, 995, -1,
  20, 21, 22, -1, -1, -1, -1, -1, 28, 29, 30, -1, 376, -1, 422, 43, 36, 425, -1, -1, 48, -1, 386,
  -1, -1, 113, -1, -1, 48, 24, 25, 26, -1, -1, 970, 443, 31, 32, 33, 34, 35, 36, 37, 38, 39, -1, 41,
  -1, -1, -1, 496, -1, 3, 4, 5, 6, 7, 8, 422, -1, -1, 425, -1, -1, 84, -1, -1, 475, -1, 20, 21, 22,
  -1, 24, 25, 26, 27, -1, -1, 443, 31, 32, 33, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 113, -1, -1,
  -1, 48, 49, 50, 51, 52, -1, 549, 3, 4, 5, 6, 7, 8, -1, 475, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  20, 21, 22, -1, -1, -1, -1, 574, 493, -1, -1, 859, 84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, 101, -1, -1, -1, 852, -1, -1, -1, -1, -1, -1, -1, 113, -1, -1, -1, -1, 3, 4,
  5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, 544, -1, -1, 881, 20, 21, 22, -1, 3, 4, 5, 6, 7, 8, -1,
  -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, 20, 21, 22, -1, -1, -1, -1, 48, 911, 20, 21, 22, -1, -1, -1, -1,
  629, 630, 631, -1, -1, -1, 673, 43, -1, -1, -1, -1, 48, 932, -1, 42, -1, -1, 685, -1, -1, 48, -1,
  -1, -1, -1, -1, 84, 11, 12, 13, 14, 15, 16, 17, 18, 19, -1, -1, -1, -1, 989, -1, -1, 629, 630,
  631, -1, 105, -1, 969, -1, 971, -1, -1, -1, 113, -1, -1, -1, -1, -1, -1, 692, 3, 4, 5, 6, 7, 8,
  -1, -1, 1020, 992, 1022, -1, -1, 113, -1, -1, -1, 20, 21, 22, 1003, -1, 113, 1035, -1, -1, -1, -1,
  -1, 11, -1, -1, -1, 764, -1, -1, -1, -1, -1, -1, -1, -1, -1, 692, -1, 48, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, 1040, -1, -1, -1, -1, -1, -1, 1047, -1, -1, -1, 799, -1, -1, 53, 54, -1, -1,
  -1, -1, -1, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
  81, 82, -1, 795, -1, 797, -1, -1, -1, 801, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  104, -1, 106, 107, 857, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, -1,
  -1, -1, -1, -1, 795, -1, 797, -1, -1, -1, 801, -1, 803, -1, -1, -1, -1, -1, -1, -1, 893, -1, -1,
  -1, 859, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 859, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 884, 3, 4,
  5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102,
  -1, 104, 105, 106, 107, -1, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
  123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
  76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
  100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
  72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, 101, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, 112, 113, 114, 115, 116,
  117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43,
  44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
  68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104, 105, 106, 107, 108, 109, 110, 111, -1,
  113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38,
  39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
  63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
  87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, -1,
  109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34,
  35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
  59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
  83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105,
  106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5,
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102,
  103, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
  123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
  76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
  100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
  72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116,
  117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43,
  44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
  68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113,
  114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1,
  41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
  65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104, 105, 106, 107, -1, 109, 110,
  111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36,
  37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
  85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104, 105, 106, 107,
  -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1,
  34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
  82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104,
  105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4,
  5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102,
  -1, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
  123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
  76, 77, 78, 79, 80, 81, 82, -1, 84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
  100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
  72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116,
  117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43,
  44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
  68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, -1, -1, 110, 111, -1, 113,
  114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1,
  41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
  65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, -1, 84, -1, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, 108, 109, 110,
  111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36,
  37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, -1, 84,
  -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107,
  108, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8,
  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1,
  34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
  82, -1, 84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104,
  105, 106, 107, 108, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3,
  4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
  30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53,
  54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77,
  78, 79, 80, 81, 82, -1, 84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1,
  102, -1, 104, 105, 106, 107, -1, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
  122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50,
  51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
  75, 76, 77, 78, 79, 80, 81, 82, -1, 84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
  99, 100, -1, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
  119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1,
  47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
  71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, -1, 84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94,
  95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115,
  116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38, 39, -1, 41, 42,
  43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
  67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, -1, 84, -1, 86, 87, 88, 89, 90,
  91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, -1,
  113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35, 36, 37, 38,
  39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
  63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, -1, 84, -1, 86,
  87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, -1, 105, -1, 107, -1, 109,
  110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, 34, 35,
  36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
  60, 61, 62, -1, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, -1,
  84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, -1, 105, -1,
  107, -1, 109, 110, 111, -1, 113, 114, -1, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
  -1, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, 44, -1, -1, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
  57, 58, 59, 60, 61, 62, -1, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
  81, 82, -1, -1, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, -1,
  105, -1, 107, -1, -1, 110, 111, -1, 113, 114, -1, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5,
  6, 7, 8, -1, -1, 11, -1, -1, -1, -1, -1, -1, -1, -1, 20, 21, 22, -1, 24, 25, 26, 27, -1, -1, -1,
  31, 32, 33, 34, 35, 36, 37, 38, 39, -1, 41, -1, -1, -1, -1, -1, -1, 48, 49, -1, -1, -1, 53, 54,
  -1, -1, -1, -1, -1, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, -1, 84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, -1,
  -1, 104, -1, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 3,
  4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 20, 21, 22, -1, 24, 25, 26, 27, -1, -1,
  -1, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, -1, -1, -1, 48, 49, 50, -1, 52, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  20, 21, 22, -1, 84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 3, 4, 5,
  6, 7, 8, 48, -1, -1, -1, -1, 113, -1, -1, -1, -1, -1, 20, 21, 22, -1, 24, 25, 26, 27, -1, -1, -1,
  31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 84, -1, -1, 48, 49, -1, -1, 52, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, 101, 102, -1, -1, 105, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, 84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5,
  6, 7, 8, -1, -1, -1, -1, -1, -1, 113, -1, -1, -1, -1, 20, 21, 22, -1, 24, 25, 26, 27, -1, -1, -1,
  31, 32, 33, 34, 35, 36, 37, 38, 39, -1, 41, 42, -1, -1, -1, -1, -1, 48, 49, 50, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 20, 21,
  22, -1, -1, 84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8,
  48, -1, -1, -1, -1, -1, 113, -1, -1, -1, -1, 20, 21, 22, -1, 24, 25, 26, 27, -1, -1, -1, 31, 32,
  33, 34, 35, 36, 37, 38, 39, -1, 41, 42, 43, -1, 84, -1, -1, 48, 49, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, 101, -1, -1, -1, 105, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 84, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8,
  -1, -1, -1, -1, -1, -1, 113, -1, -1, -1, -1, 20, 21, 22, -1, 24, 25, 26, 27, -1, -1, -1, 31, 32,
  33, 34, 35, 36, 37, 38, 39, -1, 41, 42, -1, -1, -1, -1, -1, 48, 49, -1, -1, -1, -1, -1, -1, 3, 4,
  5, 6, 7, 8, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, 20, 21, 22, -1, -1, -1, 20, 21, 22, -1, -1, 84,
  -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 48, -1, -1, -1, -1, -1, 48, -1,
  -1, -1, -1, -1, 113, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, 84, -1, -1, -1, -1, -1, 84, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, 102, -1, -1,
  105, -1, -1, 20, 21, 22, 105, 24, 25, 26, 27, -1, -1, -1, 31, 32, 33, 34, 35, 36, 37, 38, 39, -1,
  41, 42, 43, -1, -1, -1, -1, 48, 49, -1, -1, 52, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 84, -1, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, 31, 32, 33, 34, 35, 36, 37, 38,
  39, 40, 41, 20, 21, 22, -1, 24, 25, 26, 27, 50, -1, -1, 31, 32, 33, 34, 35, 36, 37, 38, 39, -1,
  41, 42, -1, -1, -1, -1, -1, 48, 49, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 86,
  87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, -1, -1, -1, -1, 84, -1, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 20, 21, 22, -1, 24, 25, 26, 27, -1, -1, -1, 31, 32, 33, 34, 35, 36, 37, 38, 39, -1,
  41, 42, -1, -1, -1, -1, -1, 48, 49, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 84, -1, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 20, 21, 22, -1, 24, 25, 26, 27, -1, -1, -1, 31, 32, 33, 34, 35, 36, 37, 38, 39, -1,
  41, -1, -1, -1, -1, -1, -1, 48, 49, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 84, -1, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 20, 21, 22, -1, 24, -1, 26, 27, -1, -1, -1, 31, 32, 33, 34, 35, 36, 37, 38, 39, -1,
  41, -1, -1, -1, -1, -1, -1, 48, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 84, -1, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 20, 21, 22, -1, -1, -1, -1, -1, -1, -1, -1, 31, 32, 33, 34, 35, 36, 37, 38, 39, -1,
  41, -1, -1, -1, -1, -1, -1, 48, 49, -1, -1, -1, -1, -1, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, -1, -1, -1, -1, -1, -1, -1, -1, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, 84, -1, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 77, -1, -1, -1, -1, -1, -1, -1, -1, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1, -1, -1, -1, -1, -1, -1, -1, 110, -1, -1,
  -1, 114, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, -1, -1, -1, -1, -1, -1, -1, -1, 50, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  77, -1, -1, -1, -1, -1, -1, -1, -1, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, 110, -1, -1, -1, 114 };

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned short yystos[] = { 0, 125, 126, 0, 127, 341, 342, 3, 4, 5, 6, 7, 8, 20, 21,
  22, 24, 25, 26, 27, 31, 32, 33, 34, 35, 36, 37, 38, 39, 41, 42, 43, 48, 49, 50, 51, 52, 84, 86,
  87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 113, 128, 129, 130, 131, 132, 134,
  135, 136, 137, 138, 140, 143, 158, 159, 160, 162, 163, 173, 174, 183, 185, 186, 188, 207, 208,
  209, 210, 213, 214, 217, 222, 263, 293, 294, 295, 296, 298, 299, 300, 301, 303, 305, 306, 309,
  310, 311, 312, 313, 315, 316, 319, 320, 331, 332, 333, 353, 24, 25, 51, 11, 43, 3, 4, 5, 6, 7, 8,
  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 34,
  35, 36, 37, 38, 39, 41, 42, 43, 44, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
  62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
  102, 104, 105, 106, 107, 109, 110, 111, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
  332, 333, 364, 365, 366, 396, 397, 398, 399, 400, 304, 321, 3, 4, 5, 6, 7, 8, 38, 41, 138, 143,
  160, 163, 295, 296, 301, 303, 309, 315, 3, 4, 5, 6, 7, 8, 102, 306, 3, 4, 5, 6, 7, 8, 20, 21, 22,
  49, 51, 184, 293, 295, 296, 300, 301, 303, 307, 139, 351, 352, 307, 102, 351, 43, 52, 129, 136,
  137, 163, 169, 186, 188, 207, 263, 309, 315, 45, 101, 102, 236, 237, 236, 236, 236, 107, 143, 309,
  315, 101, 341, 43, 214, 241, 244, 294, 299, 301, 303, 145, 301, 303, 305, 306, 300, 294, 295, 300,
  341, 300, 109, 138, 143, 160, 163, 174, 214, 296, 310, 319, 341, 9, 10, 83, 201, 264, 272, 274,
  77, 110, 114, 269, 334, 335, 336, 337, 340, 317, 341, 341, 23, 354, 306, 102, 396, 392, 392, 63,
  115, 190, 382, 392, 393, 392, 9, 10, 107, 386, 293, 11, 307, 351, 307, 351, 294, 138, 160, 178,
  179, 182, 201, 272, 392, 104, 133, 293, 293, 101, 343, 344, 217, 221, 222, 296, 40, 50, 265, 268,
  269, 308, 310, 333, 102, 101, 105, 144, 145, 296, 300, 301, 303, 353, 265, 161, 101, 105, 164,
  293, 293, 351, 309, 201, 105, 246, 392, 215, 351, 297, 44, 47, 227, 228, 107, 300, 300, 300, 302,
  307, 351, 307, 351, 214, 241, 341, 318, 275, 218, 219, 221, 222, 223, 239, 283, 294, 296, 266,
  104, 256, 257, 259, 260, 201, 272, 281, 334, 345, 346, 345, 345, 335, 337, 307, 52, 355, 356, 357,
  358, 359, 102, 126, 393, 101, 109, 112, 394, 395, 396, 103, 192, 194, 195, 197, 199, 189, 112,
  101, 395, 108, 388, 389, 387, 341, 175, 177, 269, 144, 175, 293, 307, 307, 176, 283, 294, 301,
  303, 103, 295, 301, 102, 101, 104, 353, 11, 53, 54, 63, 104, 106, 107, 109, 111, 115, 363, 364,
  217, 221, 101, 266, 264, 341, 147, 142, 3, 101, 146, 341, 145, 301, 303, 296, 101, 3, 4, 5, 6, 7,
  8, 166, 167, 305, 165, 164, 341, 293, 296, 247, 248, 293, 102, 103, 249, 250, 144, 301, 347, 348,
  386, 245, 376, 265, 144, 265, 293, 307, 313, 314, 339, 31, 32, 224, 225, 226, 343, 224, 285, 286,
  287, 343, 218, 223, 294, 101, 106, 258, 102, 390, 107, 108, 272, 353, 338, 184, 293, 112, 357,
  106, 299, 306, 361, 362, 126, 103, 101, 112, 106, 382, 24, 26, 163, 295, 301, 303, 309, 326, 327,
  330, 331, 25, 49, 202, 188, 341, 372, 372, 372, 101, 176, 182, 101, 164, 175, 175, 101, 106, 107,
  343, 101, 126, 187, 3, 111, 111, 63, 115, 108, 112, 28, 29, 30, 103, 148, 149, 28, 29, 30, 36,
  153, 154, 157, 293, 105, 341, 145, 103, 106, 343, 316, 101, 305, 106, 397, 399, 392, 108, 83, 251,
  253, 341, 300, 230, 353, 249, 23, 84, 104, 105, 106, 109, 111, 123, 332, 333, 364, 365, 366, 370,
  371, 378, 379, 380, 382, 383, 386, 390, 101, 101, 101, 164, 313, 77, 110, 229, 107, 111, 288, 289,
  105, 107, 343, 267, 63, 109, 115, 367, 368, 370, 380, 391, 261, 366, 273, 339, 105, 112, 358, 300,
  83, 360, 386, 103, 193, 191, 293, 293, 293, 319, 201, 270, 274, 269, 328, 270, 202, 63, 104, 106,
  108, 109, 115, 370, 373, 374, 108, 108, 101, 101, 177, 180, 103, 315, 112, 112, 341, 105, 156,
  157, 106, 36, 155, 201, 141, 341, 167, 104, 104, 170, 397, 248, 201, 201, 103, 216, 106, 254, 3,
  231, 242, 108, 385, 381, 384, 101, 227, 220, 290, 289, 12, 13, 14, 15, 284, 240, 268, 369, 368,
  377, 106, 108, 107, 276, 286, 299, 194, 341, 329, 282, 283, 196, 345, 307, 198, 270, 249, 270, 40,
  41, 52, 101, 130, 135, 137, 150, 151, 152, 158, 159, 173, 183, 186, 188, 211, 212, 213, 241, 263,
  293, 294, 296, 309, 315, 293, 341, 293, 153, 168, 393, 101, 238, 224, 83, 252, 315, 246, 372, 376,
  372, 347, 249, 291, 292, 249, 371, 101, 103, 374, 375, 262, 277, 307, 276, 104, 203, 204, 270,
  280, 334, 203, 200, 108, 101, 341, 137, 151, 152, 186, 188, 211, 263, 294, 309, 236, 101, 221,
  241, 296, 201, 201, 154, 201, 367, 46, 253, 270, 243, 112, 382, 112, 66, 233, 234, 108, 112, 367,
  108, 367, 249, 205, 108, 270, 203, 181, 135, 143, 171, 188, 212, 309, 315, 309, 343, 221, 223,
  399, 255, 104, 232, 112, 235, 230, 349, 350, 108, 206, 378, 271, 279, 351, 143, 171, 309, 236,
  143, 201, 101, 343, 102, 256, 18, 54, 55, 309, 320, 322, 323, 232, 353, 278, 378, 276, 31, 33, 44,
  47, 102, 105, 144, 172, 351, 143, 351, 101, 392, 320, 324, 269, 279, 399, 399, 392, 393, 146, 144,
  351, 144, 172, 103, 325, 307, 347, 103, 101, 172, 144, 146, 307, 393, 172, 101 };

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned short yyr1[] = { 0, 124, 125, 126, 127, 126, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 129, 129, 129, 129, 129, 129, 130, 130,
  131, 132, 133, 132, 132, 134, 135, 135, 136, 136, 136, 137, 137, 139, 138, 141, 140, 140, 142,
  140, 140, 143, 143, 143, 144, 144, 144, 145, 145, 146, 146, 147, 148, 147, 147, 149, 149, 149,
  150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 151, 151, 151, 151, 151,
  151, 152, 152, 152, 152, 153, 153, 154, 154, 154, 155, 155, 156, 156, 157, 157, 157, 158, 158,
  158, 159, 159, 161, 160, 162, 162, 163, 163, 163, 164, 165, 164, 166, 166, 167, 167, 168, 167,
  169, 170, 170, 171, 171, 171, 171, 172, 172, 173, 173, 174, 174, 174, 174, 174, 175, 176, 176,
  177, 178, 178, 180, 181, 179, 182, 183, 184, 184, 184, 184, 184, 184, 185, 187, 186, 189, 188,
  190, 191, 188, 192, 193, 192, 195, 196, 194, 197, 198, 194, 199, 200, 194, 201, 201, 202, 202,
  203, 203, 205, 204, 206, 206, 207, 207, 207, 207, 208, 209, 209, 209, 210, 210, 210, 211, 211,
  211, 212, 212, 212, 212, 213, 213, 213, 215, 216, 214, 217, 218, 220, 219, 221, 222, 223, 224,
  225, 225, 226, 226, 227, 227, 227, 228, 228, 229, 229, 229, 230, 230, 231, 232, 232, 232, 232,
  233, 233, 235, 234, 236, 236, 236, 237, 238, 238, 240, 239, 242, 243, 241, 245, 244, 246, 246,
  247, 247, 248, 248, 249, 250, 249, 251, 252, 251, 251, 251, 254, 255, 253, 256, 256, 258, 257,
  259, 257, 260, 257, 261, 262, 261, 263, 264, 265, 265, 266, 267, 266, 268, 269, 269, 270, 271,
  270, 272, 273, 272, 275, 274, 274, 276, 277, 278, 276, 276, 279, 279, 279, 279, 279, 279, 280,
  280, 281, 281, 282, 282, 283, 283, 284, 284, 284, 284, 285, 285, 287, 286, 288, 288, 290, 289,
  291, 292, 291, 293, 293, 294, 294, 294, 294, 294, 295, 295, 295, 296, 296, 296, 296, 296, 296,
  297, 296, 298, 299, 300, 302, 301, 304, 303, 305, 305, 305, 305, 305, 305, 305, 305, 305, 306,
  306, 306, 306, 306, 306, 307, 307, 308, 308, 308, 308, 309, 309, 310, 310, 310, 310, 311, 311,
  311, 311, 311, 312, 312, 312, 313, 313, 314, 314, 315, 317, 316, 318, 316, 319, 319, 319, 320,
  320, 321, 320, 320, 320, 322, 324, 323, 325, 323, 326, 328, 327, 329, 327, 330, 330, 330, 330,
  330, 330, 330, 331, 331, 332, 332, 332, 332, 332, 332, 332, 332, 332, 333, 333, 333, 333, 333,
  333, 333, 333, 333, 333, 333, 333, 333, 333, 333, 334, 334, 334, 334, 335, 336, 338, 337, 339,
  339, 340, 340, 342, 341, 344, 343, 346, 345, 348, 347, 350, 349, 352, 351, 353, 353, 354, 355,
  355, 356, 357, 357, 357, 357, 359, 358, 360, 360, 361, 361, 362, 362, 363, 363, 363, 363, 363,
  363, 363, 363, 363, 363, 363, 363, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364,
  364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364,
  364, 364, 364, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365,
  365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 366, 366, 366, 366, 366,
  366, 366, 366, 366, 367, 367, 368, 368, 368, 369, 368, 368, 370, 370, 371, 371, 371, 371, 371,
  371, 371, 371, 371, 371, 372, 372, 373, 373, 373, 373, 374, 374, 374, 375, 375, 376, 376, 377,
  377, 378, 378, 379, 379, 379, 381, 380, 382, 382, 384, 383, 385, 383, 387, 386, 388, 386, 389,
  386, 391, 390, 392, 392, 393, 393, 394, 394, 395, 395, 396, 396, 396, 396, 396, 396, 396, 396,
  396, 396, 396, 396, 396, 396, 396, 396, 396, 397, 398, 398, 399, 400, 400, 400 };

/* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const unsigned char yyr2[] = { 0, 2, 1, 0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 1, 2, 2, 2, 2, 2, 2, 5, 4, 5, 4, 0, 6, 6, 5, 1, 2, 4, 3, 5, 4, 5, 0, 5, 0, 7, 4, 0, 5, 2, 1, 1,
  1, 3, 4, 2, 1, 1, 0, 1, 0, 0, 4, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 2,
  2, 2, 3, 4, 3, 4, 1, 4, 2, 4, 4, 0, 1, 0, 1, 1, 1, 1, 5, 3, 6, 4, 5, 0, 5, 4, 3, 1, 2, 2, 0, 0, 3,
  1, 3, 0, 2, 0, 5, 6, 2, 1, 5, 6, 3, 4, 5, 3, 1, 2, 5, 5, 6, 5, 6, 2, 0, 3, 2, 1, 1, 0, 0, 8, 1, 3,
  1, 2, 2, 2, 3, 3, 4, 0, 8, 0, 5, 0, 0, 7, 1, 0, 4, 0, 0, 5, 0, 0, 5, 0, 0, 6, 0, 1, 1, 1, 0, 1, 0,
  3, 1, 2, 2, 2, 2, 2, 3, 4, 2, 3, 2, 3, 4, 2, 4, 5, 3, 1, 1, 2, 1, 2, 3, 0, 0, 7, 2, 2, 0, 6, 2, 1,
  2, 7, 0, 1, 1, 1, 0, 2, 1, 1, 1, 0, 1, 1, 0, 2, 1, 0, 2, 2, 2, 0, 1, 0, 3, 3, 1, 1, 6, 0, 6, 0, 6,
  0, 0, 8, 0, 5, 0, 2, 1, 3, 3, 3, 0, 0, 2, 1, 0, 4, 3, 1, 0, 0, 6, 0, 1, 0, 3, 0, 2, 0, 4, 1, 0, 4,
  4, 2, 0, 2, 0, 0, 4, 2, 0, 1, 3, 0, 6, 3, 0, 5, 0, 3, 1, 0, 0, 0, 7, 1, 0, 2, 2, 3, 3, 2, 1, 2, 1,
  2, 0, 1, 2, 4, 1, 1, 1, 1, 0, 1, 0, 2, 1, 2, 0, 5, 0, 0, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 0, 5, 1, 1, 1, 0, 5, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 3, 1, 1, 1, 1,
  2, 3, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 0, 3, 0, 4, 1, 3, 4, 1, 1, 0, 4, 2, 2, 2,
  0, 3, 0, 4, 2, 0, 3, 0, 4, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 0, 4, 0, 1, 1, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2,
  0, 2, 4, 2, 1, 3, 0, 1, 2, 3, 0, 3, 0, 1, 1, 2, 1, 3, 2, 2, 3, 3, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 2, 1, 1, 1, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 0, 2, 0, 2, 1, 1, 1, 1, 1, 0, 4, 1, 1, 0, 4, 0, 5, 0, 4, 0, 4, 0, 4, 0, 4, 0, 2, 0, 2, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 4, 3, 1, 1, 1 };

/* YYDPREC[RULE-NUM] -- Dynamic precedence of rule #RULE-NUM (0 if none).  */
static const unsigned char yydprec[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* YYMERGER[RULE-NUM] -- Index of merging function for rule #RULE-NUM.  */
static const unsigned char yymerger[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* YYIMMEDIATE[RULE-NUM] -- True iff rule #RULE-NUM is not to be deferred, as
   in the case of predicates.  */
static const yybool yyimmediate[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* YYCONFLP[YYPACT[STATE-NUM]] -- Pointer into YYCONFL of start of
   list of conflicting reductions corresponding to action entry for
   state STATE-NUM in yytable.  0 means no conflicts.  The list in
   yyconfl is terminated by a rule number of 0.  */
static const unsigned char yyconflp[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 237, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 239, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 229, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 231, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 233, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45,
  47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 0, 69, 71, 73, 75, 77, 79, 0, 81, 83, 85, 87, 0, 0,
  89, 91, 93, 95, 97, 99, 101, 103, 105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125, 127, 129,
  131, 133, 135, 137, 139, 141, 143, 145, 147, 149, 151, 153, 155, 157, 159, 0, 161, 0, 163, 165,
  167, 169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 189, 191, 0, 193, 0, 0, 195, 197, 199, 0,
  201, 203, 205, 0, 207, 209, 211, 213, 215, 217, 219, 221, 223, 225, 227, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0 };

/* YYCONFL[I] -- lists of conflicting rule numbers, each terminated by
   0, pointed into by YYCONFLP.  */
static const short yyconfl[] = { 0, 408, 0, 408, 0, 408, 0, 321, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 304, 0, 304, 0,
  304, 0, 314, 0, 408, 0, 408, 0 };

/* Error token number */
#define YYTERROR 1

YYSTYPE yylval;

int yynerrs;
int yychar;

static const int YYEOF = 0;
static const int YYEMPTY = -2;

typedef enum
{
  yyok,
  yyaccept,
  yyabort,
  yyerr
} YYRESULTTAG;

#define YYCHK(YYE)                                                                                 \
  do                                                                                               \
  {                                                                                                \
    YYRESULTTAG yychk_flag = YYE;                                                                  \
    if (yychk_flag != yyok)                                                                        \
      return yychk_flag;                                                                           \
  } while (0)

#if YYDEBUG

#ifndef YYFPRINTF
#define YYFPRINTF fprintf
#endif

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
#define YY_LOCATION_PRINT(File, Loc) ((void)0)
#endif

#define YYDPRINTF(Args)                                                                            \
  do                                                                                               \
  {                                                                                                \
    if (yydebug)                                                                                   \
      YYFPRINTF Args;                                                                              \
  } while (0)

/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void yy_symbol_value_print(FILE* yyo, int yytype, YYSTYPE const* const yyvaluep)
{
  FILE* yyoutput = yyo;
  YYUSE(yyoutput);
  if (!yyvaluep)
    return;
  YYUSE(yytype);
}

/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void yy_symbol_print(FILE* yyo, int yytype, YYSTYPE const* const yyvaluep)
{
  YYFPRINTF(yyo, "%s %s (", yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print(yyo, yytype, yyvaluep);
  YYFPRINTF(yyo, ")");
}

#define YY_SYMBOL_PRINT(Title, Type, Value, Location)                                              \
  do                                                                                               \
  {                                                                                                \
    if (yydebug)                                                                                   \
    {                                                                                              \
      YYFPRINTF(stderr, "%s ", Title);                                                             \
      yy_symbol_print(stderr, Type, Value);                                                        \
      YYFPRINTF(stderr, "\n");                                                                     \
    }                                                                                              \
  } while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;

struct yyGLRStack;
static void yypstack(struct yyGLRStack* yystackp, size_t yyk) YY_ATTRIBUTE_UNUSED;
static void yypdumpstack(struct yyGLRStack* yystackp) YY_ATTRIBUTE_UNUSED;

#else /* !YYDEBUG */

#define YYDPRINTF(Args)
#define YY_SYMBOL_PRINT(Title, Type, Value, Location)

#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYMAXDEPTH * sizeof (GLRStackItem)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Minimum number of free items on the stack allowed after an
   allocation.  This is to allow allocation and initialization
   to be completed by functions that call yyexpandGLRStack before the
   stack is expanded, thus insuring that all necessary pointers get
   properly redirected to new data.  */
#define YYHEADROOM 2

#ifndef YYSTACKEXPANDABLE
#define YYSTACKEXPANDABLE 1
#endif

#if YYSTACKEXPANDABLE
#define YY_RESERVE_GLRSTACK(Yystack)                                                               \
  do                                                                                               \
  {                                                                                                \
    if (Yystack->yyspaceLeft < YYHEADROOM)                                                         \
      yyexpandGLRStack(Yystack);                                                                   \
  } while (0)
#else
#define YY_RESERVE_GLRSTACK(Yystack)                                                               \
  do                                                                                               \
  {                                                                                                \
    if (Yystack->yyspaceLeft < YYHEADROOM)                                                         \
      yyMemoryExhausted(Yystack);                                                                  \
  } while (0)
#endif

#if YYERROR_VERBOSE

#ifndef yystpcpy
#if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#define yystpcpy stpcpy
#else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char* yystpcpy(char* yydest, const char* yysrc)
{
  char* yyd = yydest;
  const char* yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static size_t yytnamerr(char* yyres, const char* yystr)
{
  if (*yystr == '"')
  {
    size_t yyn = 0;
    char const* yyp = yystr;

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
  do_not_strip_quotes:;
  }

  if (!yyres)
    return strlen(yystr);

  return (size_t)(yystpcpy(yyres, yystr) - yyres);
}
#endif

#endif /* !YYERROR_VERBOSE */

/** State numbers, as in LALR(1) machine */
typedef int yyStateNum;

/** Rule numbers, as in LALR(1) machine */
typedef int yyRuleNum;

/** Grammar symbol */
typedef int yySymbol;

/** Item references, as in LALR(1) machine */
typedef short yyItemNum;

typedef struct yyGLRState yyGLRState;
typedef struct yyGLRStateSet yyGLRStateSet;
typedef struct yySemanticOption yySemanticOption;
typedef union yyGLRStackItem yyGLRStackItem;
typedef struct yyGLRStack yyGLRStack;

struct yyGLRState
{
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

struct yyGLRStateSet
{
  yyGLRState** yystates;
  /** During nondeterministic operation, yylookaheadNeeds tracks which
   *  stacks have actually needed the current lookahead.  During deterministic
   *  operation, yylookaheadNeeds[0] is not maintained since it would merely
   *  duplicate yychar != YYEMPTY.  */
  yybool* yylookaheadNeeds;
  size_t yysize, yycapacity;
};

struct yySemanticOption
{
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

struct yyGLRStack
{
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
static void yyexpandGLRStack(yyGLRStack* yystackp);
#endif

_Noreturn static void yyFail(yyGLRStack* yystackp, const char* yymsg)
{
  if (yymsg != YY_NULLPTR)
    yyerror(yymsg);
  YYLONGJMP(yystackp->yyexception_buffer, 1);
}

_Noreturn static void yyMemoryExhausted(yyGLRStack* yystackp)
{
  YYLONGJMP(yystackp->yyexception_buffer, 2);
}

#if YYDEBUG || YYERROR_VERBOSE
/** A printable representation of TOKEN.  */
static const char* yytokenName(yySymbol yytoken)
{
  if (yytoken == YYEMPTY)
    return "";

  return yytname[yytoken];
}
#endif

/** Fill in YYVSP[YYLOW1 .. YYLOW0-1] from the chain of states starting
 *  at YYVSP[YYLOW0].yystate.yypred.  Leaves YYVSP[YYLOW1].yystate.yypred
 *  containing the pointer to the next state in the chain.  */
static void yyfillin(yyGLRStackItem*, int, int);
static void yyfillin(yyGLRStackItem* yyvsp, int yylow0, int yylow1)
{
  int i;
  yyGLRState* s = yyvsp[yylow0].yystate.yypred;
  for (i = yylow0 - 1; i >= yylow1; i -= 1)
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
static int yyfill(yyGLRStackItem*, int*, int, yybool);
static int yyfill(yyGLRStackItem* yyvsp, int* yylow, int yylow1, yybool yynormal)
{
  if (!yynormal && yylow1 < *yylow)
  {
    yyfillin(yyvsp, *yylow, yylow1);
    *yylow = yylow1;
  }
  return yylow1;
}

/** Perform user action for rule number YYN, with RHS length YYRHSLEN,
 *  and top stack item YYVSP.  YYLVALP points to place to put semantic
 *  value ($$), and yylocp points to place for location information
 *  (@$).  Returns yyok for normal return, yyaccept for YYACCEPT,
 *  yyerr for YYERROR, yyabort for YYABORT.  */
static YYRESULTTAG yyuserAction(
  yyRuleNum yyn, int yyrhslen, yyGLRStackItem* yyvsp, yyGLRStack* yystackp, YYSTYPE* yyvalp)
{
  yybool yynormal = (yybool)(yystackp->yysplitPoint == YY_NULLPTR);
  int yylow;
  YYUSE(yyvalp);
  YYUSE(yyrhslen);
#undef yyerrok
#define yyerrok (yystackp->yyerrState = 0)
#undef YYACCEPT
#define YYACCEPT return yyaccept
#undef YYABORT
#define YYABORT return yyabort
#undef YYERROR
#define YYERROR return yyerrok, yyerr
#undef YYRECOVERING
#define YYRECOVERING() (yystackp->yyerrState != 0)
#undef yyclearin
#define yyclearin (yychar = YYEMPTY)
#undef YYFILL
#define YYFILL(N) yyfill(yyvsp, &yylow, (N), yynormal)
#undef YYBACKUP
#define YYBACKUP(Token, Value) return yyerror(YY_("syntax error: cannot back up")), yyerrok, yyerr

  yylow = 1;
  if (yyrhslen == 0)
    *yyvalp = yyval_default;
  else
    *yyvalp = yyvsp[YYFILL(1 - (int)yyrhslen)].yystate.yysemantics.yysval;
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

    case 33:

    {
      pushNamespace((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 34:

    {
      popNamespace();
    }

    break;

    case 44:

    {
      pushType();
    }

    break;

    case 45:

    {
      const char* name = (currentClass ? currentClass->Name : NULL);
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
      start_class((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-4)].yystate.yysemantics.yysval.integer));
      currentClass->IsFinal =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer);
    }

    break;

    case 48:

    {
      start_class((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.integer));
      currentClass->IsFinal =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer);
    }

    break;

    case 49:

    {
      start_class(
        NULL, (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 51:

    {
      start_class(
        NULL, (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 52:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 53:

    {
      ((*yyvalp).integer) = 1;
    }

    break;

    case 54:

    {
      ((*yyvalp).integer) = 2;
    }

    break;

    case 55:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
    }

    break;

    case 56:

    {
      ((*yyvalp).str) = vtkstrcat3(
        "::", (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
    }

    break;

    case 60:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 61:

    {
      ((*yyvalp).integer) =
        (strcmp((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str),
           "final") == 0);
    }

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

    {
      access_level = VTK_ACCESS_PUBLIC;
    }

    break;

    case 67:

    {
      access_level = VTK_ACCESS_PRIVATE;
    }

    break;

    case 68:

    {
      access_level = VTK_ACCESS_PROTECTED;
    }

    break;

    case 92:

    {
      output_friend_function();
    }

    break;

    case 95:

    {
      add_base_class(currentClass,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str), access_level,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 96:

    {
      add_base_class(currentClass,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer),
        (VTK_PARSE_VIRTUAL |
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer)));
    }

    break;

    case 97:

    {
      add_base_class(currentClass,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.integer),
        ((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer) |
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer)));
    }

    break;

    case 98:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 99:

    {
      ((*yyvalp).integer) = VTK_PARSE_VIRTUAL;
    }

    break;

    case 100:

    {
      ((*yyvalp).integer) = access_level;
    }

    break;

    case 102:

    {
      ((*yyvalp).integer) = VTK_ACCESS_PUBLIC;
    }

    break;

    case 103:

    {
      ((*yyvalp).integer) = VTK_ACCESS_PRIVATE;
    }

    break;

    case 104:

    {
      ((*yyvalp).integer) = VTK_ACCESS_PROTECTED;
    }

    break;

    case 110:

    {
      pushType();
    }

    break;

    case 111:

    {
      popType();
      clearTypeId();
      if ((((yyGLRStackItem const*)yyvsp)[YYFILL(-4)].yystate.yysemantics.yysval.str) != NULL)
      {
        setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(-4)].yystate.yysemantics.yysval.str));
        setTypeBase(guess_id_type(
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-4)].yystate.yysemantics.yysval.str)));
      }
      end_enum();
    }

    break;

    case 112:

    {
      start_enum((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.integer),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer),
        getTypeId());
      clearType();
      clearTypeId();
      ((*yyvalp).str) = (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str);
    }

    break;

    case 113:

    {
      start_enum(NULL,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer),
        getTypeId());
      clearType();
      clearTypeId();
      ((*yyvalp).str) = NULL;
    }

    break;

    case 114:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 115:

    {
      ((*yyvalp).integer) = 1;
    }

    break;

    case 116:

    {
      ((*yyvalp).integer) = 1;
    }

    break;

    case 117:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 118:

    {
      pushType();
    }

    break;

    case 119:

    {
      ((*yyvalp).integer) = getType();
      popType();
    }

    break;

    case 123:

    {
      closeComment();
      add_enum((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str), NULL);
      clearType();
    }

    break;

    case 124:

    {
      postSig("=");
      markSig();
      closeComment();
    }

    break;

    case 125:

    {
      chopSig();
      add_enum(
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-4)].yystate.yysemantics.yysval.str), copySig());
      clearType();
    }

    break;

    case 148:

    {
      pushFunction();
      postSig("(");
    }

    break;

    case 149:

    {
      postSig(")");
    }

    break;

    case 150:

    {
      ((*yyvalp).integer) = (VTK_PARSE_FUNCTION |
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-7)].yystate.yysemantics.yysval.integer));
      popFunction();
    }

    break;

    case 151:

    {
      ValueInfo* item = (ValueInfo*)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(item);
      item->ItemType = VTK_TYPEDEF_INFO;
      item->Access = access_level;

      handle_complex_type(item, getAttributes(), getType(),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer), getSig());

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

    case 152:

    {
      add_using((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str), 0);
    }

    break;

    case 154:

    {
      ((*yyvalp).str) = (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str);
    }

    break;

    case 155:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 156:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 157:

    {
      ((*yyvalp).str) =
        vtkstrcat3((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 158:

    {
      ((*yyvalp).str) =
        vtkstrcat3((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 159:

    {
      add_using((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str), 1);
    }

    break;

    case 160:

    {
      markSig();
    }

    break;

    case 161:

    {
      ValueInfo* item = (ValueInfo*)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(item);
      item->ItemType = VTK_TYPEDEF_INFO;
      item->Access = access_level;

      handle_complex_type(item, getAttributes(), getType(),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer), copySig());

      item->Name = (((yyGLRStackItem const*)yyvsp)[YYFILL(-6)].yystate.yysemantics.yysval.str);
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

    case 162:

    {
      postSig("template<> ");
      clearTypeId();
    }

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
      if (getSig()[getSigLength() - 1] == '>')
      {
        postSig(" ");
      }
      postSig("> ");
      clearTypeId();
      popType();
    }

    break;

    case 168:

    {
      chopSig();
      postSig(", ");
      clearType();
      clearTypeId();
    }

    break;

    case 170:

    {
      markSig();
    }

    break;

    case 171:

    {
      add_template_parameter(getType(),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer), copySig());
    }

    break;

    case 173:

    {
      markSig();
    }

    break;

    case 174:

    {
      add_template_parameter(0,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer), copySig());
    }

    break;

    case 176:

    {
      pushTemplate();
      markSig();
    }

    break;

    case 177:

    {
      int i;
      TemplateInfo* newTemplate = currentTemplate;
      popTemplate();
      add_template_parameter(0,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer), copySig());
      i = currentTemplate->NumberOfParameters - 1;
      currentTemplate->Parameters[i]->Template = newTemplate;
    }

    break;

    case 179:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 180:

    {
      postSig("...");
      ((*yyvalp).integer) = VTK_PARSE_PACK;
    }

    break;

    case 181:

    {
      postSig("class ");
    }

    break;

    case 182:

    {
      postSig("typename ");
    }

    break;

    case 185:

    {
      postSig("=");
      markSig();
    }

    break;

    case 186:

    {
      int i = currentTemplate->NumberOfParameters - 1;
      ValueInfo* param = currentTemplate->Parameters[i];
      chopSig();
      param->Value = copySig();
    }

    break;

    case 189:

    {
      output_function();
    }

    break;

    case 190:

    {
      output_function();
    }

    break;

    case 191:

    {
      reject_function();
    }

    break;

    case 192:

    {
      reject_function();
    }

    break;

    case 200:

    {
      output_function();
    }

    break;

    case 210:

    {
      postSig("(");
      currentFunction->IsExplicit = ((getType() & VTK_PARSE_EXPLICIT) != 0);
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }

    break;

    case 211:

    {
      postSig(")");
    }

    break;

    case 212:

    {
      postSig(";");
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", "operator typecast");
    }

    break;

    case 213:

    {
      ((*yyvalp).str) = copySig();
    }

    break;

    case 214:

    {
      postSig(";");
      closeSig();
      currentFunction->Name =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }

    break;

    case 215:

    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }

    break;

    case 216:

    {
      postSig(")");
    }

    break;

    case 217:

    {
      chopSig();
      ((*yyvalp).str) = vtkstrcat(
        copySig(), (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 218:

    {
      markSig();
      postSig("operator ");
    }

    break;

    case 219:

    {
      postSig(";");
      closeSig();
      currentFunction->Name =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }

    break;

    case 223:

    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }

    break;

    case 224:

    {
      postSig(" volatile");
    }

    break;

    case 226:

    {
      chopSig();
    }

    break;

    case 228:

    {
      postSig(" noexcept");
    }

    break;

    case 229:

    {
      postSig(" throw");
    }

    break;

    case 231:

    {
      postSig("&");
    }

    break;

    case 232:

    {
      postSig("&&");
    }

    break;

    case 235:

    {
      postSig(" ");
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      if (strcmp((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str),
            "final") == 0)
      {
        currentFunction->IsFinal = 1;
      }
      else if (strcmp((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str),
                 "override") == 0)
      {
        currentFunction->IsOverride = 1;
      }
    }

    break;

    case 237:

    {
      currentFunction->IsDeleted = 1;
    }

    break;

    case 239:

    {
      postSig(" = 0");
      currentFunction->IsPureVirtual = 1;
      if (currentClass)
      {
        currentClass->IsAbstract = 1;
      }
    }

    break;

    case 242:

    {
      postSig(" -> ");
      clearType();
      clearTypeId();
    }

    break;

    case 243:

    {
      chopSig();
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }

    break;

    case 250:

    {
      postSig("(");
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }

    break;

    case 251:

    {
      postSig(")");
    }

    break;

    case 252:

    {
      closeSig();
      if (getType() & VTK_PARSE_VIRTUAL)
      {
        currentFunction->IsVirtual = 1;
      }
      if (getType() & VTK_PARSE_EXPLICIT)
      {
        currentFunction->IsExplicit = 1;
      }
      if (getAttributes() & VTK_PARSE_WRAPEXCLUDE)
      {
        currentFunction->IsExcluded = 1;
      }
      if (getAttributes() & VTK_PARSE_DEPRECATED)
      {
        currentFunction->IsDeprecated = 1;
        currentFunction->DeprecatedReason = deprecationReason;
        currentFunction->DeprecatedVersion = deprecationVersion;
      }
      currentFunction->Name =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.str);
      currentFunction->Comment = vtkstrdup(getComment());
    }

    break;

    case 253:

    {
      openSig();
    }

    break;

    case 254:

    {
      postSig(";");
      closeSig();
      vtkParseDebug("Parsed func", currentFunction->Name);
    }

    break;

    case 255:

    {
      pushType();
      postSig("(");
    }

    break;

    case 256:

    {
      postSig(")");
      popType();
    }

    break;

    case 264:

    {
      clearType();
      clearTypeId();
    }

    break;

    case 266:

    {
      clearType();
      clearTypeId();
    }

    break;

    case 267:

    {
      clearType();
      clearTypeId();
      postSig(", ");
    }

    break;

    case 269:

    {
      currentFunction->IsVariadic = 1;
      postSig(", ...");
    }

    break;

    case 270:

    {
      currentFunction->IsVariadic = 1;
      postSig("...");
    }

    break;

    case 271:

    {
      markSig();
    }

    break;

    case 272:

    {
      ValueInfo* param = (ValueInfo*)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(param);

      handle_complex_type(param, getAttributes(), getType(),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer), copySig());
      add_legacy_parameter(currentFunction, param);

      if (getVarName())
      {
        param->Name = getVarName();
      }

      vtkParse_AddParameterToFunction(currentFunction, param);
    }

    break;

    case 273:

    {
      int i = currentFunction->NumberOfParameters - 1;
      if (getVarValue())
      {
        currentFunction->Parameters[i]->Value = getVarValue();
      }
    }

    break;

    case 274:

    {
      clearVarValue();
    }

    break;

    case 276:

    {
      postSig("=");
      clearVarValue();
      markSig();
    }

    break;

    case 277:

    {
      chopSig();
      setVarValue(copySig());
    }

    break;

    case 278:

    {
      clearVarValue();
      markSig();
    }

    break;

    case 279:

    {
      chopSig();
      setVarValue(copySig());
    }

    break;

    case 280:

    {
      clearVarValue();
      markSig();
      postSig("(");
    }

    break;

    case 281:

    {
      chopSig();
      postSig(")");
      setVarValue(copySig());
    }

    break;

    case 282:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 283:

    {
      postSig(", ");
    }

    break;

    case 286:

    {
      unsigned int attributes = getAttributes();
      unsigned int type = getType();
      ValueInfo* var = (ValueInfo*)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(var);
      var->ItemType = VTK_VARIABLE_INFO;
      var->Access = access_level;

      handle_complex_type(var, attributes, type,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer), getSig());

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
        (((type & VTK_PARSE_INDIRECT) == 0) || ((type & VTK_PARSE_INDIRECT) == VTK_PARSE_ARRAY)))
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

    case 290:

    {
      postSig(", ");
    }

    break;

    case 293:

    {
      setTypePtr(0);
    }

    break;

    case 294:

    {
      setTypePtr((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 295:

    {
      if ((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer) ==
        VTK_PARSE_FUNCTION)
      {
        ((*yyvalp).integer) = (VTK_PARSE_FUNCTION_PTR |
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer));
      }
      else
      {
        ((*yyvalp).integer) =
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer);
      }
    }

    break;

    case 296:

    {
      postSig(")");
    }

    break;

    case 297:

    {
      const char* scope = getScope();
      unsigned int parens = add_indirection(
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-5)].yystate.yysemantics.yysval.integer),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.integer));
      if ((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer) ==
        VTK_PARSE_FUNCTION)
      {
        if (scope)
        {
          scope = vtkstrndup(scope, strlen(scope) - 2);
        }
        getFunction()->Class = scope;
        ((*yyvalp).integer) = (parens | VTK_PARSE_FUNCTION);
      }
      else if ((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer) ==
        VTK_PARSE_ARRAY)
      {
        ((*yyvalp).integer) = add_indirection_to_array(parens);
      }
    }

    break;

    case 298:

    {
      ((*yyvalp).integer) =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer);
    }

    break;

    case 299:

    {
      postSig(")");
    }

    break;

    case 300:

    {
      const char* scope = getScope();
      unsigned int parens = add_indirection(
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-4)].yystate.yysemantics.yysval.integer),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.integer));
      if ((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer) ==
        VTK_PARSE_FUNCTION)
      {
        if (scope)
        {
          scope = vtkstrndup(scope, strlen(scope) - 2);
        }
        getFunction()->Class = scope;
        ((*yyvalp).integer) = (parens | VTK_PARSE_FUNCTION);
      }
      else if ((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer) ==
        VTK_PARSE_ARRAY)
      {
        ((*yyvalp).integer) = add_indirection_to_array(parens);
      }
    }

    break;

    case 301:

    {
      postSig("(");
      scopeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      postSig("*");
    }

    break;

    case 302:

    {
      ((*yyvalp).integer) =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer);
    }

    break;

    case 303:

    {
      postSig("(");
      scopeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      postSig("&");
      ((*yyvalp).integer) = VTK_PARSE_REF;
    }

    break;

    case 304:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 305:

    {
      pushFunction();
      postSig("(");
    }

    break;

    case 306:

    {
      postSig(")");
    }

    break;

    case 307:

    {
      ((*yyvalp).integer) = VTK_PARSE_FUNCTION;
      popFunction();
    }

    break;

    case 308:

    {
      ((*yyvalp).integer) = VTK_PARSE_ARRAY;
    }

    break;

    case 311:

    {
      currentFunction->IsConst = 1;
    }

    break;

    case 316:

    {
      ((*yyvalp).integer) = add_indirection(
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 318:

    {
      ((*yyvalp).integer) = add_indirection(
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 319:

    {
      clearVarName();
      chopSig();
    }

    break;

    case 321:

    {
      setVarName((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
    }

    break;

    case 322:

    {
      setVarName((((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.str));
    }

    break;

    case 327:

    {
      clearArray();
    }

    break;

    case 329:

    {
      clearArray();
    }

    break;

    case 333:

    {
      postSig("[");
    }

    break;

    case 334:

    {
      postSig("]");
    }

    break;

    case 335:

    {
      pushArraySize("");
    }

    break;

    case 336:

    {
      markSig();
    }

    break;

    case 337:

    {
      chopSig();
      pushArraySize(copySig());
    }

    break;

    case 343:

    {
      ((*yyvalp).str) =
        vtkstrcat("~", (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 344:

    {
      ((*yyvalp).str) =
        vtkstrcat("~", (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 345:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 346:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 347:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 348:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 349:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 350:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 351:

    {
      ((*yyvalp).str) =
        vtkstrcat3((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 352:

    {
      ((*yyvalp).str) =
        vtkstrcat3((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 353:

    {
      ((*yyvalp).str) =
        vtkstrcat3((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 354:

    {
      postSig("template ");
    }

    break;

    case 355:

    {
      ((*yyvalp).str) =
        vtkstrcat4((((yyGLRStackItem const*)yyvsp)[YYFILL(-4)].yystate.yysemantics.yysval.str),
          "template ", (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 356:

    {
      postSig("~");
    }

    break;

    case 357:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 358:

    {
      ((*yyvalp).str) = "::";
      postSig(((*yyvalp).str));
    }

    break;

    case 359:

    {
      markSig();
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
      postSig("<");
    }

    break;

    case 360:

    {
      chopSig();
      if (getSig()[getSigLength() - 1] == '>')
      {
        postSig(" ");
      }
      postSig(">");
      ((*yyvalp).str) = copySig();
      clearTypeId();
    }

    break;

    case 361:

    {
      markSig();
      postSig("decltype");
    }

    break;

    case 362:

    {
      chopSig();
      ((*yyvalp).str) = copySig();
      clearTypeId();
    }

    break;

    case 363:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 364:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 365:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 366:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 367:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 368:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 369:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 370:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 371:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 381:

    {
      setTypeBase(buildTypeBase(
        getType(), (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer)));
    }

    break;

    case 382:

    {
      setTypeMod(VTK_PARSE_TYPEDEF);
    }

    break;

    case 383:

    {
      setTypeMod(VTK_PARSE_FRIEND);
    }

    break;

    case 386:

    {
      setTypeMod((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 387:

    {
      setTypeMod((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 388:

    {
      setTypeMod((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 389:

    {
      postSig("constexpr ");
      ((*yyvalp).integer) = 0;
    }

    break;

    case 390:

    {
      postSig("mutable ");
      ((*yyvalp).integer) = VTK_PARSE_MUTABLE;
    }

    break;

    case 391:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 392:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 393:

    {
      postSig("static ");
      ((*yyvalp).integer) = VTK_PARSE_STATIC;
    }

    break;

    case 394:

    {
      postSig("thread_local ");
      ((*yyvalp).integer) = VTK_PARSE_THREAD_LOCAL;
    }

    break;

    case 395:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 396:

    {
      postSig("virtual ");
      ((*yyvalp).integer) = VTK_PARSE_VIRTUAL;
    }

    break;

    case 397:

    {
      postSig("explicit ");
      ((*yyvalp).integer) = VTK_PARSE_EXPLICIT;
    }

    break;

    case 398:

    {
      postSig("const ");
      ((*yyvalp).integer) = VTK_PARSE_CONST;
    }

    break;

    case 399:

    {
      postSig("volatile ");
      ((*yyvalp).integer) = VTK_PARSE_VOLATILE;
    }

    break;

    case 401:

    {
      ((*yyvalp).integer) =
        ((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer) |
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 403:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 405:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 408:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 409:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
    }

    break;

    case 411:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = 0;
    }

    break;

    case 412:

    {
      postSig("typename ");
    }

    break;

    case 413:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
    }

    break;

    case 414:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
    }

    break;

    case 415:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
    }

    break;

    case 417:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 419:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 422:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 424:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 427:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = 0;
    }

    break;

    case 428:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 429:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 430:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 431:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 432:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 433:

    {
      setTypeId("");
    }

    break;

    case 435:

    {
      typeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = VTK_PARSE_STRING;
    }

    break;

    case 436:

    {
      typeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = VTK_PARSE_OSTREAM;
    }

    break;

    case 437:

    {
      typeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = VTK_PARSE_ISTREAM;
    }

    break;

    case 438:

    {
      typeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = VTK_PARSE_UNKNOWN;
    }

    break;

    case 439:

    {
      typeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = VTK_PARSE_OBJECT;
    }

    break;

    case 440:

    {
      typeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = VTK_PARSE_QOBJECT;
    }

    break;

    case 441:

    {
      typeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = VTK_PARSE_NULLPTR_T;
    }

    break;

    case 442:

    {
      typeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = VTK_PARSE_SSIZE_T;
    }

    break;

    case 443:

    {
      typeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = VTK_PARSE_SIZE_T;
    }

    break;

    case 444:

    {
      postSig("auto ");
      ((*yyvalp).integer) = 0;
    }

    break;

    case 445:

    {
      postSig("void ");
      ((*yyvalp).integer) = VTK_PARSE_VOID;
    }

    break;

    case 446:

    {
      postSig("bool ");
      ((*yyvalp).integer) = VTK_PARSE_BOOL;
    }

    break;

    case 447:

    {
      postSig("float ");
      ((*yyvalp).integer) = VTK_PARSE_FLOAT;
    }

    break;

    case 448:

    {
      postSig("double ");
      ((*yyvalp).integer) = VTK_PARSE_DOUBLE;
    }

    break;

    case 449:

    {
      postSig("char ");
      ((*yyvalp).integer) = VTK_PARSE_CHAR;
    }

    break;

    case 450:

    {
      postSig("char16_t ");
      ((*yyvalp).integer) = VTK_PARSE_CHAR16_T;
    }

    break;

    case 451:

    {
      postSig("char32_t ");
      ((*yyvalp).integer) = VTK_PARSE_CHAR32_T;
    }

    break;

    case 452:

    {
      postSig("wchar_t ");
      ((*yyvalp).integer) = VTK_PARSE_WCHAR_T;
    }

    break;

    case 453:

    {
      postSig("int ");
      ((*yyvalp).integer) = VTK_PARSE_INT;
    }

    break;

    case 454:

    {
      postSig("short ");
      ((*yyvalp).integer) = VTK_PARSE_SHORT;
    }

    break;

    case 455:

    {
      postSig("long ");
      ((*yyvalp).integer) = VTK_PARSE_LONG;
    }

    break;

    case 456:

    {
      postSig("__int64 ");
      ((*yyvalp).integer) = VTK_PARSE___INT64;
    }

    break;

    case 457:

    {
      postSig("signed ");
      ((*yyvalp).integer) = VTK_PARSE_INT;
    }

    break;

    case 458:

    {
      postSig("unsigned ");
      ((*yyvalp).integer) = VTK_PARSE_UNSIGNED_INT;
    }

    break;

    case 462:

    {
      ((*yyvalp).integer) =
        ((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer) |
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 463:

    {
      postSig("&");
      ((*yyvalp).integer) = VTK_PARSE_REF;
    }

    break;

    case 464:

    {
      postSig("&&");
      ((*yyvalp).integer) = (VTK_PARSE_RVALUE | VTK_PARSE_REF);
    }

    break;

    case 465:

    {
      postSig("*");
    }

    break;

    case 466:

    {
      ((*yyvalp).integer) =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer);
    }

    break;

    case 467:

    {
      ((*yyvalp).integer) = VTK_PARSE_POINTER;
    }

    break;

    case 468:

    {
      if (((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer) &
            VTK_PARSE_CONST) != 0)
      {
        ((*yyvalp).integer) = VTK_PARSE_CONST_POINTER;
      }
      if (((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer) &
            VTK_PARSE_VOLATILE) != 0)
      {
        ((*yyvalp).integer) = VTK_PARSE_BAD_INDIRECT;
      }
    }

    break;

    case 470:

    {
      unsigned int n;
      n = (((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer) << 2) |
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
      if ((n & VTK_PARSE_INDIRECT) != n)
      {
        n = VTK_PARSE_BAD_INDIRECT;
      }
      ((*yyvalp).integer) = n;
    }

    break;

    case 471:

    {
      setAttributeRole(VTK_PARSE_ATTRIB_DECL);
    }

    break;

    case 472:

    {
      clearAttributeRole();
    }

    break;

    case 473:

    {
      setAttributeRole(VTK_PARSE_ATTRIB_ID);
    }

    break;

    case 474:

    {
      clearAttributeRole();
    }

    break;

    case 475:

    {
      setAttributeRole(VTK_PARSE_ATTRIB_REF);
    }

    break;

    case 476:

    {
      clearAttributeRole();
    }

    break;

    case 477:

    {
      setAttributeRole(VTK_PARSE_ATTRIB_FUNC);
    }

    break;

    case 478:

    {
      clearAttributeRole();
    }

    break;

    case 479:

    {
      setAttributeRole(VTK_PARSE_ATTRIB_ARRAY);
    }

    break;

    case 480:

    {
      clearAttributeRole();
    }

    break;

    case 481:

    {
      setAttributeRole(VTK_PARSE_ATTRIB_CLASS);
    }

    break;

    case 482:

    {
      clearAttributeRole();
    }

    break;

    case 485:

    {
      setAttributePrefix(NULL);
    }

    break;

    case 488:

    {
      setAttributePrefix(vtkstrcat(
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str), "::"));
    }

    break;

    case 493:

    {
      markSig();
    }

    break;

    case 494:

    {
      handle_attribute(
        cutSig(), (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 495:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 496:

    {
      ((*yyvalp).integer) = VTK_PARSE_PACK;
    }

    break;

    case 501:

    {
      ((*yyvalp).str) = "()";
    }

    break;

    case 502:

    {
      ((*yyvalp).str) = "[]";
    }

    break;

    case 503:

    {
      ((*yyvalp).str) = " new[]";
    }

    break;

    case 504:

    {
      ((*yyvalp).str) = " delete[]";
    }

    break;

    case 505:

    {
      ((*yyvalp).str) = "<";
    }

    break;

    case 506:

    {
      ((*yyvalp).str) = ">";
    }

    break;

    case 507:

    {
      ((*yyvalp).str) = ",";
    }

    break;

    case 508:

    {
      ((*yyvalp).str) = "=";
    }

    break;

    case 509:

    {
      ((*yyvalp).str) = ">>";
    }

    break;

    case 510:

    {
      ((*yyvalp).str) = ">>";
    }

    break;

    case 511:

    {
      ((*yyvalp).str) = vtkstrcat(
        "\"\" ", (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 513:

    {
      ((*yyvalp).str) = "%";
    }

    break;

    case 514:

    {
      ((*yyvalp).str) = "*";
    }

    break;

    case 515:

    {
      ((*yyvalp).str) = "/";
    }

    break;

    case 516:

    {
      ((*yyvalp).str) = "-";
    }

    break;

    case 517:

    {
      ((*yyvalp).str) = "+";
    }

    break;

    case 518:

    {
      ((*yyvalp).str) = "!";
    }

    break;

    case 519:

    {
      ((*yyvalp).str) = "~";
    }

    break;

    case 520:

    {
      ((*yyvalp).str) = "&";
    }

    break;

    case 521:

    {
      ((*yyvalp).str) = "|";
    }

    break;

    case 522:

    {
      ((*yyvalp).str) = "^";
    }

    break;

    case 523:

    {
      ((*yyvalp).str) = " new";
    }

    break;

    case 524:

    {
      ((*yyvalp).str) = " delete";
    }

    break;

    case 525:

    {
      ((*yyvalp).str) = "<<=";
    }

    break;

    case 526:

    {
      ((*yyvalp).str) = ">>=";
    }

    break;

    case 527:

    {
      ((*yyvalp).str) = "<<";
    }

    break;

    case 528:

    {
      ((*yyvalp).str) = ".*";
    }

    break;

    case 529:

    {
      ((*yyvalp).str) = "->*";
    }

    break;

    case 530:

    {
      ((*yyvalp).str) = "->";
    }

    break;

    case 531:

    {
      ((*yyvalp).str) = "+=";
    }

    break;

    case 532:

    {
      ((*yyvalp).str) = "-=";
    }

    break;

    case 533:

    {
      ((*yyvalp).str) = "*=";
    }

    break;

    case 534:

    {
      ((*yyvalp).str) = "/=";
    }

    break;

    case 535:

    {
      ((*yyvalp).str) = "%=";
    }

    break;

    case 536:

    {
      ((*yyvalp).str) = "++";
    }

    break;

    case 537:

    {
      ((*yyvalp).str) = "--";
    }

    break;

    case 538:

    {
      ((*yyvalp).str) = "&=";
    }

    break;

    case 539:

    {
      ((*yyvalp).str) = "|=";
    }

    break;

    case 540:

    {
      ((*yyvalp).str) = "^=";
    }

    break;

    case 541:

    {
      ((*yyvalp).str) = "&&";
    }

    break;

    case 542:

    {
      ((*yyvalp).str) = "||";
    }

    break;

    case 543:

    {
      ((*yyvalp).str) = "==";
    }

    break;

    case 544:

    {
      ((*yyvalp).str) = "!=";
    }

    break;

    case 545:

    {
      ((*yyvalp).str) = "<=";
    }

    break;

    case 546:

    {
      ((*yyvalp).str) = ">=";
    }

    break;

    case 547:

    {
      ((*yyvalp).str) = "typedef";
    }

    break;

    case 548:

    {
      ((*yyvalp).str) = "typename";
    }

    break;

    case 549:

    {
      ((*yyvalp).str) = "class";
    }

    break;

    case 550:

    {
      ((*yyvalp).str) = "struct";
    }

    break;

    case 551:

    {
      ((*yyvalp).str) = "union";
    }

    break;

    case 552:

    {
      ((*yyvalp).str) = "template";
    }

    break;

    case 553:

    {
      ((*yyvalp).str) = "public";
    }

    break;

    case 554:

    {
      ((*yyvalp).str) = "protected";
    }

    break;

    case 555:

    {
      ((*yyvalp).str) = "private";
    }

    break;

    case 556:

    {
      ((*yyvalp).str) = "const";
    }

    break;

    case 557:

    {
      ((*yyvalp).str) = "volatile";
    }

    break;

    case 558:

    {
      ((*yyvalp).str) = "static";
    }

    break;

    case 559:

    {
      ((*yyvalp).str) = "thread_local";
    }

    break;

    case 560:

    {
      ((*yyvalp).str) = "constexpr";
    }

    break;

    case 561:

    {
      ((*yyvalp).str) = "inline";
    }

    break;

    case 562:

    {
      ((*yyvalp).str) = "virtual";
    }

    break;

    case 563:

    {
      ((*yyvalp).str) = "explicit";
    }

    break;

    case 564:

    {
      ((*yyvalp).str) = "decltype";
    }

    break;

    case 565:

    {
      ((*yyvalp).str) = "default";
    }

    break;

    case 566:

    {
      ((*yyvalp).str) = "extern";
    }

    break;

    case 567:

    {
      ((*yyvalp).str) = "using";
    }

    break;

    case 568:

    {
      ((*yyvalp).str) = "namespace";
    }

    break;

    case 569:

    {
      ((*yyvalp).str) = "operator";
    }

    break;

    case 570:

    {
      ((*yyvalp).str) = "enum";
    }

    break;

    case 571:

    {
      ((*yyvalp).str) = "throw";
    }

    break;

    case 572:

    {
      ((*yyvalp).str) = "noexcept";
    }

    break;

    case 573:

    {
      ((*yyvalp).str) = "const_cast";
    }

    break;

    case 574:

    {
      ((*yyvalp).str) = "dynamic_cast";
    }

    break;

    case 575:

    {
      ((*yyvalp).str) = "static_cast";
    }

    break;

    case 576:

    {
      ((*yyvalp).str) = "reinterpret_cast";
    }

    break;

    case 590:

    {
      postSig("< ");
    }

    break;

    case 591:

    {
      postSig("> ");
    }

    break;

    case 593:

    {
      postSig(">");
    }

    break;

    case 595:

    {
      chopSig();
      postSig("::");
    }

    break;

    case 599:

    {
      const char* op = (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str);
      if ((op[0] == '+' || op[0] == '-' || op[0] == '*' || op[0] == '&') && op[1] == '\0')
      {
        int c1 = 0;
        size_t l;
        const char* cp;
        chopSig();
        cp = getSig();
        l = getSigLength();
        if (l > 0)
        {
          c1 = cp[l - 1];
        }
        if (c1 != 0 && c1 != '(' && c1 != '[' && c1 != '=')
        {
          postSig(" ");
        }
        postSig(op);
        if (vtkParse_CharType(c1, (CPRE_XID | CPRE_QUOTE)) || c1 == ')' || c1 == ']')
        {
          postSig(" ");
        }
      }
      else if ((op[0] == '-' && op[1] == '>') || op[0] == '.')
      {
        chopSig();
        postSig(op);
      }
      else
      {
        postSig(op);
        postSig(" ");
      }
    }

    break;

    case 600:

    {
      postSig(":");
      postSig(" ");
    }

    break;

    case 601:

    {
      postSig(".");
    }

    break;

    case 602:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      postSig(" ");
    }

    break;

    case 603:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      postSig(" ");
    }

    break;

    case 605:

    {
      chopSig();
      postSig(" ");
    }

    break;

    case 609:

    {
      postSig("< ");
    }

    break;

    case 610:

    {
      postSig("> ");
    }

    break;

    case 611:

    {
      postSig(">");
    }

    break;

    case 613:

    {
      postSig("= ");
    }

    break;

    case 614:

    {
      chopSig();
      postSig(", ");
    }

    break;

    case 616:

    {
      chopSig();
      postSig(";");
    }

    break;

    case 624:

    {
      postSig("= ");
    }

    break;

    case 625:

    {
      chopSig();
      postSig(", ");
    }

    break;

    case 626:

    {
      chopSig();
      if (getSig()[getSigLength() - 1] == '<')
      {
        postSig(" ");
      }
      postSig("<");
    }

    break;

    case 627:

    {
      chopSig();
      if (getSig()[getSigLength() - 1] == '>')
      {
        postSig(" ");
      }
      postSig("> ");
    }

    break;

    case 630:

    {
      postSigLeftBracket("[");
    }

    break;

    case 631:

    {
      postSigRightBracket("] ");
    }

    break;

    case 632:

    {
      postSig("[[");
    }

    break;

    case 633:

    {
      chopSig();
      postSig("]] ");
    }

    break;

    case 634:

    {
      postSigLeftBracket("(");
    }

    break;

    case 635:

    {
      postSigRightBracket(") ");
    }

    break;

    case 636:

    {
      postSigLeftBracket("(");
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      postSig("*");
    }

    break;

    case 637:

    {
      postSigRightBracket(") ");
    }

    break;

    case 638:

    {
      postSigLeftBracket("(");
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      postSig("&");
    }

    break;

    case 639:

    {
      postSigRightBracket(") ");
    }

    break;

    case 640:

    {
      postSig("{ ");
    }

    break;

    case 641:

    {
      postSig("} ");
    }

    break;

    default:
      break;
  }

  return yyok;
#undef yyerrok
#undef YYABORT
#undef YYACCEPT
#undef YYERROR
#undef YYBACKUP
#undef yyclearin
#undef YYRECOVERING
}

static void yyuserMerge(int yyn, YYSTYPE* yy0, YYSTYPE* yy1)
{
  YYUSE(yy0);
  YYUSE(yy1);

  switch (yyn)
  {

    default:
      break;
  }
}

/* Bison grammar-table manipulation.  */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void yydestruct(const char* yymsg, int yytype, YYSTYPE* yyvaluep)
{
  YYUSE(yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT(yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE(yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}

/** Number of symbols composing the right hand side of rule #RULE.  */
static int yyrhsLength(yyRuleNum yyrule)
{
  return yyr2[yyrule];
}

static void yydestroyGLRState(char const* yymsg, yyGLRState* yys)
{
  if (yys->yyresolved)
    yydestruct(yymsg, yystos[yys->yylrState], &yys->yysemantics.yysval);
  else
  {
#if YYDEBUG
    if (yydebug)
    {
      if (yys->yysemantics.yyfirstVal)
        YYFPRINTF(stderr, "%s unresolved", yymsg);
      else
        YYFPRINTF(stderr, "%s incomplete", yymsg);
      YY_SYMBOL_PRINT("", yystos[yys->yylrState], YY_NULLPTR, &yys->yyloc);
    }
#endif

    if (yys->yysemantics.yyfirstVal)
    {
      yySemanticOption* yyoption = yys->yysemantics.yyfirstVal;
      yyGLRState* yyrh;
      int yyn;
      for (yyrh = yyoption->yystate, yyn = yyrhsLength(yyoption->yyrule); yyn > 0;
           yyrh = yyrh->yypred, yyn -= 1)
        yydestroyGLRState(yymsg, yyrh);
    }
  }
}

/** Left-hand-side symbol for rule #YYRULE.  */
static yySymbol yylhsNonterm(yyRuleNum yyrule)
{
  return yyr1[yyrule];
}

#define yypact_value_is_default(Yystate) (!!((Yystate) == (-808)))

/** True iff LR state YYSTATE has only a default reduction (regardless
 *  of token).  */
static yybool yyisDefaultedState(yyStateNum yystate)
{
  return (yybool)yypact_value_is_default(yypact[yystate]);
}

/** The default reduction for YYSTATE, assuming it has one.  */
static yyRuleNum yydefaultAction(yyStateNum yystate)
{
  return yydefact[yystate];
}

#define yytable_value_is_error(Yytable_value) 0

/** Set *YYACTION to the action to take in YYSTATE on seeing YYTOKEN.
 *  Result R means
 *    R < 0:  Reduce on rule -R.
 *    R = 0:  Error.
 *    R > 0:  Shift to state R.
 *  Set *YYCONFLICTS to a pointer into yyconfl to a 0-terminated list
 *  of conflicting reductions.
 */
static void yygetLRActions(
  yyStateNum yystate, int yytoken, int* yyaction, const short** yyconflicts)
{
  int yyindex = yypact[yystate] + yytoken;
  if (yyisDefaultedState(yystate) || yyindex < 0 || YYLAST < yyindex || yycheck[yyindex] != yytoken)
  {
    *yyaction = -yydefact[yystate];
    *yyconflicts = yyconfl;
  }
  else if (!yytable_value_is_error(yytable[yyindex]))
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
static yyStateNum yyLRgotoState(yyStateNum yystate, yySymbol yysym)
{
  int yyr = yypgoto[yysym - YYNTOKENS] + yystate;
  if (0 <= yyr && yyr <= YYLAST && yycheck[yyr] == yystate)
    return yytable[yyr];
  else
    return yydefgoto[yysym - YYNTOKENS];
}

static yybool yyisShiftAction(int yyaction)
{
  return (yybool)(0 < yyaction);
}

static yybool yyisErrorAction(int yyaction)
{
  return (yybool)(yyaction == 0);
}

/* GLRStates */

/** Return a fresh GLRStackItem in YYSTACKP.  The item is an LR state
 *  if YYISSTATE, and otherwise a semantic option.  Callers should call
 *  YY_RESERVE_GLRSTACK afterwards to make sure there is sufficient
 *  headroom.  */

static yyGLRStackItem* yynewGLRStackItem(yyGLRStack* yystackp, yybool yyisState)
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
static void yyaddDeferredAction(
  yyGLRStack* yystackp, size_t yyk, yyGLRState* yystate, yyGLRState* yyrhs, yyRuleNum yyrule)
{
  yySemanticOption* yynewOption = &yynewGLRStackItem(yystackp, yyfalse)->yyoption;
  YYASSERT(!yynewOption->yyisState);
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

  YY_RESERVE_GLRSTACK(yystackp);
}

/* GLRStacks */

/** Initialize YYSET to a singleton set containing an empty stack.  */
static yybool yyinitStateSet(yyGLRStateSet* yyset)
{
  yyset->yysize = 1;
  yyset->yycapacity = 16;
  yyset->yystates = (yyGLRState**)YYMALLOC(16 * sizeof yyset->yystates[0]);
  if (!yyset->yystates)
    return yyfalse;
  yyset->yystates[0] = YY_NULLPTR;
  yyset->yylookaheadNeeds = (yybool*)YYMALLOC(16 * sizeof yyset->yylookaheadNeeds[0]);
  if (!yyset->yylookaheadNeeds)
  {
    YYFREE(yyset->yystates);
    return yyfalse;
  }
  return yytrue;
}

static void yyfreeStateSet(yyGLRStateSet* yyset)
{
  YYFREE(yyset->yystates);
  YYFREE(yyset->yylookaheadNeeds);
}

/** Initialize *YYSTACKP to a single empty stack, with total maximum
 *  capacity for all stacks of YYSIZE.  */
static yybool yyinitGLRStack(yyGLRStack* yystackp, size_t yysize)
{
  yystackp->yyerrState = 0;
  yynerrs = 0;
  yystackp->yyspaceLeft = yysize;
  yystackp->yyitems = (yyGLRStackItem*)YYMALLOC(yysize * sizeof yystackp->yynextFree[0]);
  if (!yystackp->yyitems)
    return yyfalse;
  yystackp->yynextFree = yystackp->yyitems;
  yystackp->yysplitPoint = YY_NULLPTR;
  yystackp->yylastDeleted = YY_NULLPTR;
  return yyinitStateSet(&yystackp->yytops);
}

#if YYSTACKEXPANDABLE
#define YYRELOC(YYFROMITEMS, YYTOITEMS, YYX, YYTYPE)                                               \
  &((YYTOITEMS) - ((YYFROMITEMS) - (yyGLRStackItem*)(YYX)))->YYTYPE

/** If *YYSTACKP is expandable, extend it.  WARNING: Pointers into the
    stack from outside should be considered invalid after this call.
    We always expand when there are 1 or fewer items left AFTER an
    allocation, so that we can avoid having external pointers exist
    across an allocation.  */
static void yyexpandGLRStack(yyGLRStack* yystackp)
{
  yyGLRStackItem* yynewItems;
  yyGLRStackItem *yyp0, *yyp1;
  size_t yynewSize;
  size_t yyn;
  size_t yysize = (size_t)(yystackp->yynextFree - yystackp->yyitems);
  if (YYMAXDEPTH - YYHEADROOM < yysize)
    yyMemoryExhausted(yystackp);
  yynewSize = 2 * yysize;
  if (YYMAXDEPTH < yynewSize)
    yynewSize = YYMAXDEPTH;
  yynewItems = (yyGLRStackItem*)YYMALLOC(yynewSize * sizeof yynewItems[0]);
  if (!yynewItems)
    yyMemoryExhausted(yystackp);
  for (yyp0 = yystackp->yyitems, yyp1 = yynewItems, yyn = yysize; 0 < yyn;
       yyn -= 1, yyp0 += 1, yyp1 += 1)
  {
    *yyp1 = *yyp0;
    if (*(yybool*)yyp0)
    {
      yyGLRState* yys0 = &yyp0->yystate;
      yyGLRState* yys1 = &yyp1->yystate;
      if (yys0->yypred != YY_NULLPTR)
        yys1->yypred = YYRELOC(yyp0, yyp1, yys0->yypred, yystate);
      if (!yys0->yyresolved && yys0->yysemantics.yyfirstVal != YY_NULLPTR)
        yys1->yysemantics.yyfirstVal = YYRELOC(yyp0, yyp1, yys0->yysemantics.yyfirstVal, yyoption);
    }
    else
    {
      yySemanticOption* yyv0 = &yyp0->yyoption;
      yySemanticOption* yyv1 = &yyp1->yyoption;
      if (yyv0->yystate != YY_NULLPTR)
        yyv1->yystate = YYRELOC(yyp0, yyp1, yyv0->yystate, yystate);
      if (yyv0->yynext != YY_NULLPTR)
        yyv1->yynext = YYRELOC(yyp0, yyp1, yyv0->yynext, yyoption);
    }
  }
  if (yystackp->yysplitPoint != YY_NULLPTR)
    yystackp->yysplitPoint =
      YYRELOC(yystackp->yyitems, yynewItems, yystackp->yysplitPoint, yystate);

  for (yyn = 0; yyn < yystackp->yytops.yysize; yyn += 1)
    if (yystackp->yytops.yystates[yyn] != YY_NULLPTR)
      yystackp->yytops.yystates[yyn] =
        YYRELOC(yystackp->yyitems, yynewItems, yystackp->yytops.yystates[yyn], yystate);
  YYFREE(yystackp->yyitems);
  yystackp->yyitems = yynewItems;
  yystackp->yynextFree = yynewItems + yysize;
  yystackp->yyspaceLeft = yynewSize - yysize;
}
#endif

static void yyfreeGLRStack(yyGLRStack* yystackp)
{
  YYFREE(yystackp->yyitems);
  yyfreeStateSet(&yystackp->yytops);
}

/** Assuming that YYS is a GLRState somewhere on *YYSTACKP, update the
 *  splitpoint of *YYSTACKP, if needed, so that it is at least as deep as
 *  YYS.  */
static void yyupdateSplit(yyGLRStack* yystackp, yyGLRState* yys)
{
  if (yystackp->yysplitPoint != YY_NULLPTR && yystackp->yysplitPoint > yys)
    yystackp->yysplitPoint = yys;
}

/** Invalidate stack #YYK in *YYSTACKP.  */
static void yymarkStackDeleted(yyGLRStack* yystackp, size_t yyk)
{
  if (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
    yystackp->yylastDeleted = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yystates[yyk] = YY_NULLPTR;
}

/** Undelete the last stack in *YYSTACKP that was marked as deleted.  Can
    only be done once after a deletion, and only when all other stacks have
    been deleted.  */
static void yyundeleteLastStack(yyGLRStack* yystackp)
{
  if (yystackp->yylastDeleted == YY_NULLPTR || yystackp->yytops.yysize != 0)
    return;
  yystackp->yytops.yystates[0] = yystackp->yylastDeleted;
  yystackp->yytops.yysize = 1;
  YYDPRINTF((stderr, "Restoring last deleted stack as stack #0.\n"));
  yystackp->yylastDeleted = YY_NULLPTR;
}

static void yyremoveDeletes(yyGLRStack* yystackp)
{
  size_t yyi, yyj;
  yyi = yyj = 0;
  while (yyj < yystackp->yytops.yysize)
  {
    if (yystackp->yytops.yystates[yyi] == YY_NULLPTR)
    {
      if (yyi == yyj)
      {
        YYDPRINTF((stderr, "Removing dead stacks.\n"));
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
      yystackp->yytops.yylookaheadNeeds[yyj] = yystackp->yytops.yylookaheadNeeds[yyi];
      if (yyj != yyi)
      {
        YYDPRINTF((stderr, "Rename stack %lu -> %lu.\n", (unsigned long)yyi, (unsigned long)yyj));
      }
      yyj += 1;
    }
    yyi += 1;
  }
}

/** Shift to a new state on stack #YYK of *YYSTACKP, corresponding to LR
 * state YYLRSTATE, at input position YYPOSN, with (resolved) semantic
 * value *YYVALP and source location *YYLOCP.  */
static void yyglrShift(
  yyGLRStack* yystackp, size_t yyk, yyStateNum yylrState, size_t yyposn, YYSTYPE* yyvalp)
{
  yyGLRState* yynewState = &yynewGLRStackItem(yystackp, yytrue)->yystate;

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yytrue;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yysval = *yyvalp;
  yystackp->yytops.yystates[yyk] = yynewState;

  YY_RESERVE_GLRSTACK(yystackp);
}

/** Shift stack #YYK of *YYSTACKP, to a new state corresponding to LR
 *  state YYLRSTATE, at input position YYPOSN, with the (unresolved)
 *  semantic value of YYRHS under the action for YYRULE.  */
static void yyglrShiftDefer(yyGLRStack* yystackp, size_t yyk, yyStateNum yylrState, size_t yyposn,
  yyGLRState* yyrhs, yyRuleNum yyrule)
{
  yyGLRState* yynewState = &yynewGLRStackItem(yystackp, yytrue)->yystate;
  YYASSERT(yynewState->yyisState);

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yyfalse;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yyfirstVal = YY_NULLPTR;
  yystackp->yytops.yystates[yyk] = yynewState;

  /* Invokes YY_RESERVE_GLRSTACK.  */
  yyaddDeferredAction(yystackp, yyk, yynewState, yyrhs, yyrule);
}

#if !YYDEBUG
#define YY_REDUCE_PRINT(Args)
#else
#define YY_REDUCE_PRINT(Args)                                                                      \
  do                                                                                               \
  {                                                                                                \
    if (yydebug)                                                                                   \
      yy_reduce_print Args;                                                                        \
  } while (0)

/*----------------------------------------------------------------------.
| Report that stack #YYK of *YYSTACKP is going to be reduced by YYRULE. |
`----------------------------------------------------------------------*/

static void yy_reduce_print(yybool yynormal, yyGLRStackItem* yyvsp, size_t yyk, yyRuleNum yyrule)
{
  int yynrhs = yyrhsLength(yyrule);
  int yyi;
  YYFPRINTF(stderr, "Reducing stack %lu by rule %d (line %lu):\n", (unsigned long)yyk, yyrule - 1,
    (unsigned long)yyrline[yyrule]);
  if (!yynormal)
    yyfillin(yyvsp, 1, -yynrhs);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
  {
    YYFPRINTF(stderr, "   $%d = ", yyi + 1);
    yy_symbol_print(stderr, yystos[yyvsp[yyi - yynrhs + 1].yystate.yylrState],
      &yyvsp[yyi - yynrhs + 1].yystate.yysemantics.yysval);
    if (!yyvsp[yyi - yynrhs + 1].yystate.yyresolved)
      YYFPRINTF(stderr, " (unresolved)");
    YYFPRINTF(stderr, "\n");
  }
}
#endif

/** Pop the symbols consumed by reduction #YYRULE from the top of stack
 *  #YYK of *YYSTACKP, and perform the appropriate semantic action on their
 *  semantic values.  Assumes that all ambiguities in semantic values
 *  have been previously resolved.  Set *YYVALP to the resulting value,
 *  and *YYLOCP to the computed location (if any).  Return value is as
 *  for userAction.  */
static YYRESULTTAG yydoAction(yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule, YYSTYPE* yyvalp)
{
  int yynrhs = yyrhsLength(yyrule);

  if (yystackp->yysplitPoint == YY_NULLPTR)
  {
    /* Standard special case: single stack.  */
    yyGLRStackItem* yyrhs = (yyGLRStackItem*)yystackp->yytops.yystates[yyk];
    YYASSERT(yyk == 0);
    yystackp->yynextFree -= yynrhs;
    yystackp->yyspaceLeft += (size_t)yynrhs;
    yystackp->yytops.yystates[0] = &yystackp->yynextFree[-1].yystate;
    YY_REDUCE_PRINT((yytrue, yyrhs, yyk, yyrule));
    return yyuserAction(yyrule, yynrhs, yyrhs, yystackp, yyvalp);
  }
  else
  {
    int yyi;
    yyGLRState* yys;
    yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
    yys = yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred = yystackp->yytops.yystates[yyk];
    for (yyi = 0; yyi < yynrhs; yyi += 1)
    {
      yys = yys->yypred;
      YYASSERT(yys);
    }
    yyupdateSplit(yystackp, yys);
    yystackp->yytops.yystates[yyk] = yys;
    YY_REDUCE_PRINT((yyfalse, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1, yyk, yyrule));
    return yyuserAction(yyrule, yynrhs, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1, yystackp, yyvalp);
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
static YYRESULTTAG yyglrReduce(
  yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule, yybool yyforceEval)
{
  size_t yyposn = yystackp->yytops.yystates[yyk]->yyposn;

  if (yyforceEval || yystackp->yysplitPoint == YY_NULLPTR)
  {
    YYSTYPE yysval;

    YYRESULTTAG yyflag = yydoAction(yystackp, yyk, yyrule, &yysval);
    if (yyflag == yyerr && yystackp->yysplitPoint != YY_NULLPTR)
    {
      YYDPRINTF(
        (stderr, "Parse on stack %lu rejected by rule #%d.\n", (unsigned long)yyk, yyrule - 1));
    }
    if (yyflag != yyok)
      return yyflag;
    YY_SYMBOL_PRINT("-> $$ =", yyr1[yyrule], &yysval, &yyloc);
    yyglrShift(yystackp, yyk,
      yyLRgotoState(yystackp->yytops.yystates[yyk]->yylrState, yylhsNonterm(yyrule)), yyposn,
      &yysval);
  }
  else
  {
    size_t yyi;
    int yyn;
    yyGLRState *yys, *yys0 = yystackp->yytops.yystates[yyk];
    yyStateNum yynewLRState;

    for (yys = yystackp->yytops.yystates[yyk], yyn = yyrhsLength(yyrule); 0 < yyn; yyn -= 1)
    {
      yys = yys->yypred;
      YYASSERT(yys);
    }
    yyupdateSplit(yystackp, yys);
    yynewLRState = yyLRgotoState(yys->yylrState, yylhsNonterm(yyrule));
    YYDPRINTF((stderr,
      "Reduced stack %lu by rule #%d; action deferred.  "
      "Now in state %d.\n",
      (unsigned long)yyk, yyrule - 1, yynewLRState));
    for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
      if (yyi != yyk && yystackp->yytops.yystates[yyi] != YY_NULLPTR)
      {
        yyGLRState* yysplit = yystackp->yysplitPoint;
        yyGLRState* yyp = yystackp->yytops.yystates[yyi];
        while (yyp != yys && yyp != yysplit && yyp->yyposn >= yyposn)
        {
          if (yyp->yylrState == yynewLRState && yyp->yypred == yys)
          {
            yyaddDeferredAction(yystackp, yyk, yyp, yys0, yyrule);
            yymarkStackDeleted(yystackp, yyk);
            YYDPRINTF((stderr, "Merging stack %lu into stack %lu.\n", (unsigned long)yyk,
              (unsigned long)yyi));
            return yyok;
          }
          yyp = yyp->yypred;
        }
      }
    yystackp->yytops.yystates[yyk] = yys;
    yyglrShiftDefer(yystackp, yyk, yynewLRState, yyposn, yys0, yyrule);
  }
  return yyok;
}

static size_t yysplitStack(yyGLRStack* yystackp, size_t yyk)
{
  if (yystackp->yysplitPoint == YY_NULLPTR)
  {
    YYASSERT(yyk == 0);
    yystackp->yysplitPoint = yystackp->yytops.yystates[yyk];
  }
  if (yystackp->yytops.yysize >= yystackp->yytops.yycapacity)
  {
    yyGLRState** yynewStates = YY_NULLPTR;
    yybool* yynewLookaheadNeeds;

    if (yystackp->yytops.yycapacity > (YYSIZEMAX / (2 * sizeof(yyGLRState*))))
      yyMemoryExhausted(yystackp);
    yystackp->yytops.yycapacity *= 2;

    yynewStates = (yyGLRState**)YYREALLOC(
      yystackp->yytops.yystates, (yystackp->yytops.yycapacity * sizeof(yyGLRState*)));
    if (yynewStates == YY_NULLPTR)
      yyMemoryExhausted(yystackp);
    yystackp->yytops.yystates = yynewStates;

    yynewLookaheadNeeds = (yybool*)YYREALLOC(
      yystackp->yytops.yylookaheadNeeds, (yystackp->yytops.yycapacity * sizeof(yybool)));
    if (yynewLookaheadNeeds == YY_NULLPTR)
      yyMemoryExhausted(yystackp);
    yystackp->yytops.yylookaheadNeeds = yynewLookaheadNeeds;
  }
  yystackp->yytops.yystates[yystackp->yytops.yysize] = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yylookaheadNeeds[yystackp->yytops.yysize] =
    yystackp->yytops.yylookaheadNeeds[yyk];
  yystackp->yytops.yysize += 1;
  return yystackp->yytops.yysize - 1;
}

/** True iff YYY0 and YYY1 represent identical options at the top level.
 *  That is, they represent the same rule applied to RHS symbols
 *  that produce the same terminal symbols.  */
static yybool yyidenticalOptions(yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  if (yyy0->yyrule == yyy1->yyrule)
  {
    yyGLRState *yys0, *yys1;
    int yyn;
    for (yys0 = yyy0->yystate, yys1 = yyy1->yystate, yyn = yyrhsLength(yyy0->yyrule); yyn > 0;
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
static void yymergeOptionSets(yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  yyGLRState *yys0, *yys1;
  int yyn;
  for (yys0 = yyy0->yystate, yys1 = yyy1->yystate, yyn = yyrhsLength(yyy0->yyrule); yyn > 0;
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
static int yypreference(yySemanticOption* y0, yySemanticOption* y1)
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

static YYRESULTTAG yyresolveValue(yyGLRState* yys, yyGLRStack* yystackp);

/** Resolve the previous YYN states starting at and including state YYS
 *  on *YYSTACKP. If result != yyok, some states may have been left
 *  unresolved possibly with empty semantic option chains.  Regardless
 *  of whether result = yyok, each state has been left with consistent
 *  data so that yydestroyGLRState can be invoked if necessary.  */
static YYRESULTTAG yyresolveStates(yyGLRState* yys, int yyn, yyGLRStack* yystackp)
{
  if (0 < yyn)
  {
    YYASSERT(yys->yypred);
    YYCHK(yyresolveStates(yys->yypred, yyn - 1, yystackp));
    if (!yys->yyresolved)
      YYCHK(yyresolveValue(yys, yystackp));
  }
  return yyok;
}

/** Resolve the states for the RHS of YYOPT on *YYSTACKP, perform its
 *  user action, and return the semantic value and location in *YYVALP
 *  and *YYLOCP.  Regardless of whether result = yyok, all RHS states
 *  have been destroyed (assuming the user action destroys all RHS
 *  semantic values if invoked).  */
static YYRESULTTAG yyresolveAction(yySemanticOption* yyopt, yyGLRStack* yystackp, YYSTYPE* yyvalp)
{
  yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
  int yynrhs = yyrhsLength(yyopt->yyrule);
  YYRESULTTAG yyflag = yyresolveStates(yyopt->yystate, yynrhs, yystackp);
  if (yyflag != yyok)
  {
    yyGLRState* yys;
    for (yys = yyopt->yystate; yynrhs > 0; yys = yys->yypred, yynrhs -= 1)
      yydestroyGLRState("Cleanup: popping", yys);
    return yyflag;
  }

  yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred = yyopt->yystate;
  {
    int yychar_current = yychar;
    YYSTYPE yylval_current = yylval;
    yychar = yyopt->yyrawchar;
    yylval = yyopt->yyval;
    yyflag =
      yyuserAction(yyopt->yyrule, yynrhs, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1, yystackp, yyvalp);
    yychar = yychar_current;
    yylval = yylval_current;
  }
  return yyflag;
}

#if YYDEBUG
static void yyreportTree(yySemanticOption* yyx, int yyindent)
{
  int yynrhs = yyrhsLength(yyx->yyrule);
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
    YYFPRINTF(stderr, "%*s%s -> <Rule %d, empty>\n", yyindent, "",
      yytokenName(yylhsNonterm(yyx->yyrule)), yyx->yyrule - 1);
  else
    YYFPRINTF(stderr, "%*s%s -> <Rule %d, tokens %lu .. %lu>\n", yyindent, "",
      yytokenName(yylhsNonterm(yyx->yyrule)), yyx->yyrule - 1, (unsigned long)(yys->yyposn + 1),
      (unsigned long)yyx->yystate->yyposn);
  for (yyi = 1; yyi <= yynrhs; yyi += 1)
  {
    if (yystates[yyi]->yyresolved)
    {
      if (yystates[yyi - 1]->yyposn + 1 > yystates[yyi]->yyposn)
        YYFPRINTF(stderr, "%*s%s <empty>\n", yyindent + 2, "",
          yytokenName(yystos[yystates[yyi]->yylrState]));
      else
        YYFPRINTF(stderr, "%*s%s <tokens %lu .. %lu>\n", yyindent + 2, "",
          yytokenName(yystos[yystates[yyi]->yylrState]),
          (unsigned long)(yystates[yyi - 1]->yyposn + 1), (unsigned long)yystates[yyi]->yyposn);
    }
    else
      yyreportTree(yystates[yyi]->yysemantics.yyfirstVal, yyindent + 2);
  }
}
#endif

static YYRESULTTAG yyreportAmbiguity(yySemanticOption* yyx0, yySemanticOption* yyx1)
{
  YYUSE(yyx0);
  YYUSE(yyx1);

#if YYDEBUG
  YYFPRINTF(stderr, "Ambiguity detected.\n");
  YYFPRINTF(stderr, "Option 1,\n");
  yyreportTree(yyx0, 2);
  YYFPRINTF(stderr, "\nOption 2,\n");
  yyreportTree(yyx1, 2);
  YYFPRINTF(stderr, "\n");
#endif

  yyerror(YY_("syntax is ambiguous"));
  return yyabort;
}

/** Resolve the ambiguity represented in state YYS in *YYSTACKP,
 *  perform the indicated actions, and set the semantic value of YYS.
 *  If result != yyok, the chain of semantic options in YYS has been
 *  cleared instead or it has been left unmodified except that
 *  redundant options may have been removed.  Regardless of whether
 *  result = yyok, YYS has been left with consistent data so that
 *  yydestroyGLRState can be invoked if necessary.  */
static YYRESULTTAG yyresolveValue(yyGLRState* yys, yyGLRStack* yystackp)
{
  yySemanticOption* yyoptionList = yys->yysemantics.yyfirstVal;
  yySemanticOption* yybest = yyoptionList;
  yySemanticOption** yypp;
  yybool yymerge = yyfalse;
  YYSTYPE yysval;
  YYRESULTTAG yyflag;

  for (yypp = &yyoptionList->yynext; *yypp != YY_NULLPTR;)
  {
    yySemanticOption* yyp = *yypp;

    if (yyidenticalOptions(yybest, yyp))
    {
      yymergeOptionSets(yybest, yyp);
      *yypp = yyp->yynext;
    }
    else
    {
      switch (yypreference(yybest, yyp))
      {
        case 0:
          return yyreportAmbiguity(yybest, yyp);
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
    yyflag = yyresolveAction(yybest, yystackp, &yysval);
    if (yyflag == yyok)
      for (yyp = yybest->yynext; yyp != YY_NULLPTR; yyp = yyp->yynext)
      {
        if (yyprec == yydprec[yyp->yyrule])
        {
          YYSTYPE yysval_other;
          yyflag = yyresolveAction(yyp, yystackp, &yysval_other);
          if (yyflag != yyok)
          {
            yydestruct(
              "Cleanup: discarding incompletely merged value for", yystos[yys->yylrState], &yysval);
            break;
          }
          yyuserMerge(yymerger[yyp->yyrule], &yysval, &yysval_other);
        }
      }
  }
  else
    yyflag = yyresolveAction(yybest, yystackp, &yysval);

  if (yyflag == yyok)
  {
    yys->yyresolved = yytrue;
    yys->yysemantics.yysval = yysval;
  }
  else
    yys->yysemantics.yyfirstVal = YY_NULLPTR;
  return yyflag;
}

static YYRESULTTAG yyresolveStack(yyGLRStack* yystackp)
{
  if (yystackp->yysplitPoint != YY_NULLPTR)
  {
    yyGLRState* yys;
    int yyn;

    for (yyn = 0, yys = yystackp->yytops.yystates[0]; yys != yystackp->yysplitPoint;
         yys = yys->yypred, yyn += 1)
      continue;
    YYCHK(yyresolveStates(yystackp->yytops.yystates[0], yyn, yystackp));
  }
  return yyok;
}

static void yycompressStack(yyGLRStack* yystackp)
{
  yyGLRState *yyp, *yyq, *yyr;

  if (yystackp->yytops.yysize != 1 || yystackp->yysplitPoint == YY_NULLPTR)
    return;

  for (yyp = yystackp->yytops.yystates[0], yyq = yyp->yypred, yyr = YY_NULLPTR;
       yyp != yystackp->yysplitPoint; yyr = yyp, yyp = yyq, yyq = yyp->yypred)
    yyp->yypred = yyr;

  yystackp->yyspaceLeft += (size_t)(yystackp->yynextFree - yystackp->yyitems);
  yystackp->yynextFree = ((yyGLRStackItem*)yystackp->yysplitPoint) + 1;
  yystackp->yyspaceLeft -= (size_t)(yystackp->yynextFree - yystackp->yyitems);
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

static YYRESULTTAG yyprocessOneStack(yyGLRStack* yystackp, size_t yyk, size_t yyposn)
{
  while (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
  {
    yyStateNum yystate = yystackp->yytops.yystates[yyk]->yylrState;
    YYDPRINTF((stderr, "Stack %lu Entering state %d\n", (unsigned long)yyk, yystate));

    YYASSERT(yystate != YYFINAL);

    if (yyisDefaultedState(yystate))
    {
      YYRESULTTAG yyflag;
      yyRuleNum yyrule = yydefaultAction(yystate);
      if (yyrule == 0)
      {
        YYDPRINTF((stderr, "Stack %lu dies.\n", (unsigned long)yyk));
        yymarkStackDeleted(yystackp, yyk);
        return yyok;
      }
      yyflag = yyglrReduce(yystackp, yyk, yyrule, yyimmediate[yyrule]);
      if (yyflag == yyerr)
      {
        YYDPRINTF((stderr,
          "Stack %lu dies "
          "(predicate failure or explicit user error).\n",
          (unsigned long)yyk));
        yymarkStackDeleted(yystackp, yyk);
        return yyok;
      }
      if (yyflag != yyok)
        return yyflag;
    }
    else
    {
      yySymbol yytoken;
      int yyaction;
      const short* yyconflicts;

      yystackp->yytops.yylookaheadNeeds[yyk] = yytrue;
      if (yychar == YYEMPTY)
      {
        YYDPRINTF((stderr, "Reading a token: "));
        yychar = yylex();
      }

      if (yychar <= YYEOF)
      {
        yychar = yytoken = YYEOF;
        YYDPRINTF((stderr, "Now at end of input.\n"));
      }
      else
      {
        yytoken = YYTRANSLATE(yychar);
        YY_SYMBOL_PRINT("Next token is", yytoken, &yylval, &yylloc);
      }

      yygetLRActions(yystate, yytoken, &yyaction, &yyconflicts);

      while (*yyconflicts != 0)
      {
        YYRESULTTAG yyflag;
        size_t yynewStack = yysplitStack(yystackp, yyk);
        YYDPRINTF((stderr, "Splitting off stack %lu from %lu.\n", (unsigned long)yynewStack,
          (unsigned long)yyk));
        yyflag = yyglrReduce(yystackp, yynewStack, *yyconflicts, yyimmediate[*yyconflicts]);
        if (yyflag == yyok)
          YYCHK(yyprocessOneStack(yystackp, yynewStack, yyposn));
        else if (yyflag == yyerr)
        {
          YYDPRINTF((stderr, "Stack %lu dies.\n", (unsigned long)yynewStack));
          yymarkStackDeleted(yystackp, yynewStack);
        }
        else
          return yyflag;
        yyconflicts += 1;
      }

      if (yyisShiftAction(yyaction))
        break;
      else if (yyisErrorAction(yyaction))
      {
        YYDPRINTF((stderr, "Stack %lu dies.\n", (unsigned long)yyk));
        yymarkStackDeleted(yystackp, yyk);
        break;
      }
      else
      {
        YYRESULTTAG yyflag = yyglrReduce(yystackp, yyk, -yyaction, yyimmediate[-yyaction]);
        if (yyflag == yyerr)
        {
          YYDPRINTF((stderr,
            "Stack %lu dies "
            "(predicate failure or explicit user error).\n",
            (unsigned long)yyk));
          yymarkStackDeleted(yystackp, yyk);
          break;
        }
        else if (yyflag != yyok)
          return yyflag;
      }
    }
  }
  return yyok;
}

static void yyreportSyntaxError(yyGLRStack* yystackp)
{
  if (yystackp->yyerrState != 0)
    return;
#if !YYERROR_VERBOSE
  yyerror(YY_("syntax error"));
#else
  {
    yySymbol yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE(yychar);
    size_t yysize0 = yytnamerr(YY_NULLPTR, yytokenName(yytoken));
    size_t yysize = yysize0;
    yybool yysize_overflow = yyfalse;
    char* yymsg = YY_NULLPTR;
    enum
    {
      YYERROR_VERBOSE_ARGS_MAXIMUM = 5
    };
    /* Internationalized format string. */
    const char* yyformat = YY_NULLPTR;
    /* Arguments of yyformat. */
    char const* yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
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
      yyarg[yycount++] = yytokenName(yytoken);
      if (!yypact_value_is_default(yyn))
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
          if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR &&
            !yytable_value_is_error(yytable[yyx + yyn]))
          {
            if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
            {
              yycount = 1;
              yysize = yysize0;
              break;
            }
            yyarg[yycount++] = yytokenName(yyx);
            {
              size_t yysz = yysize + yytnamerr(YY_NULLPTR, yytokenName(yyx));
              if (yysz < yysize)
                yysize_overflow = yytrue;
              yysize = yysz;
            }
          }
      }
    }

    switch (yycount)
    {
#define YYCASE_(N, S)                                                                              \
  case N:                                                                                          \
    yyformat = S;                                                                                  \
    break
      default: /* Avoid compiler warnings. */
        YYCASE_(0, YY_("syntax error"));
        YYCASE_(1, YY_("syntax error, unexpected %s"));
        YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

    {
      size_t yysz = yysize + strlen(yyformat);
      if (yysz < yysize)
        yysize_overflow = yytrue;
      yysize = yysz;
    }

    if (!yysize_overflow)
      yymsg = (char*)YYMALLOC(yysize);

    if (yymsg)
    {
      char* yyp = yymsg;
      int yyi = 0;
      while ((*yyp = *yyformat))
      {
        if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr(yyp, yyarg[yyi++]);
          yyformat += 2;
        }
        else
        {
          yyp++;
          yyformat++;
        }
      }
      yyerror(yymsg);
      YYFREE(yymsg);
    }
    else
    {
      yyerror(YY_("syntax error"));
      yyMemoryExhausted(yystackp);
    }
  }
#endif /* YYERROR_VERBOSE */
  yynerrs += 1;
}

/* Recover from a syntax error on *YYSTACKP, assuming that *YYSTACKP->YYTOKENP,
   yylval, and yylloc are the syntactic category, semantic value, and location
   of the lookahead.  */
static void yyrecoverSyntaxError(yyGLRStack* yystackp)
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
        yyFail(yystackp, YY_NULLPTR);
      if (yychar != YYEMPTY)
      {
        yytoken = YYTRANSLATE(yychar);
        yydestruct("Error: discarding", yytoken, &yylval);
      }
      YYDPRINTF((stderr, "Reading a token: "));
      yychar = yylex();
      if (yychar <= YYEOF)
      {
        yychar = yytoken = YYEOF;
        YYDPRINTF((stderr, "Now at end of input.\n"));
      }
      else
      {
        yytoken = YYTRANSLATE(yychar);
        YY_SYMBOL_PRINT("Next token is", yytoken, &yylval, &yylloc);
      }
      yyj = yypact[yystackp->yytops.yystates[0]->yylrState];
      if (yypact_value_is_default(yyj))
        return;
      yyj += yytoken;
      if (yyj < 0 || YYLAST < yyj || yycheck[yyj] != yytoken)
      {
        if (yydefact[yystackp->yytops.yystates[0]->yylrState] != 0)
          return;
      }
      else if (!yytable_value_is_error(yytable[yyj]))
        return;
    }

  /* Reduce to one stack.  */
  for (yyk = 0; yyk < yystackp->yytops.yysize; yyk += 1)
    if (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
      break;
  if (yyk >= yystackp->yytops.yysize)
    yyFail(yystackp, YY_NULLPTR);
  for (yyk += 1; yyk < yystackp->yytops.yysize; yyk += 1)
    yymarkStackDeleted(yystackp, yyk);
  yyremoveDeletes(yystackp);
  yycompressStack(yystackp);

  /* Now pop stack until we find a state that shifts the error token.  */
  yystackp->yyerrState = 3;
  while (yystackp->yytops.yystates[0] != YY_NULLPTR)
  {
    yyGLRState* yys = yystackp->yytops.yystates[0];
    yyj = yypact[yys->yylrState];
    if (!yypact_value_is_default(yyj))
    {
      yyj += YYTERROR;
      if (0 <= yyj && yyj <= YYLAST && yycheck[yyj] == YYTERROR && yyisShiftAction(yytable[yyj]))
      {
        /* Shift the error token.  */
        YY_SYMBOL_PRINT("Shifting", yystos[yytable[yyj]], &yylval, &yyerrloc);
        yyglrShift(yystackp, 0, yytable[yyj], yys->yyposn, &yylval);
        yys = yystackp->yytops.yystates[0];
        break;
      }
    }
    if (yys->yypred != YY_NULLPTR)
      yydestroyGLRState("Error: popping", yys);
    yystackp->yytops.yystates[0] = yys->yypred;
    yystackp->yynextFree -= 1;
    yystackp->yyspaceLeft += 1;
  }
  if (yystackp->yytops.yystates[0] == YY_NULLPTR)
    yyFail(yystackp, YY_NULLPTR);
}

#define YYCHK1(YYE)                                                                                \
  do                                                                                               \
  {                                                                                                \
    switch (YYE)                                                                                   \
    {                                                                                              \
      case yyok:                                                                                   \
        break;                                                                                     \
      case yyabort:                                                                                \
        goto yyabortlab;                                                                           \
      case yyaccept:                                                                               \
        goto yyacceptlab;                                                                          \
      case yyerr:                                                                                  \
        goto yyuser_error;                                                                         \
      default:                                                                                     \
        goto yybuglab;                                                                             \
    }                                                                                              \
  } while (0)

/*----------.
| yyparse.  |
`----------*/

int yyparse(void)
{
  int yyresult;
  yyGLRStack yystack;
  yyGLRStack* const yystackp = &yystack;
  size_t yyposn;

  YYDPRINTF((stderr, "Starting parse\n"));

  yychar = YYEMPTY;
  yylval = yyval_default;

  if (!yyinitGLRStack(yystackp, YYINITDEPTH))
    goto yyexhaustedlab;
  switch (YYSETJMP(yystack.yyexception_buffer))
  {
    case 0:
      break;
    case 1:
      goto yyabortlab;
    case 2:
      goto yyexhaustedlab;
    default:
      goto yybuglab;
  }
  yyglrShift(&yystack, 0, 0, 0, &yylval);
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
      const short* yyconflicts;

      yyStateNum yystate = yystack.yytops.yystates[0]->yylrState;
      YYDPRINTF((stderr, "Entering state %d\n", yystate));
      if (yystate == YYFINAL)
        goto yyacceptlab;
      if (yyisDefaultedState(yystate))
      {
        yyrule = yydefaultAction(yystate);
        if (yyrule == 0)
        {
          yyreportSyntaxError(&yystack);
          goto yyuser_error;
        }
        YYCHK1(yyglrReduce(&yystack, 0, yyrule, yytrue));
      }
      else
      {
        yySymbol yytoken;
        if (yychar == YYEMPTY)
        {
          YYDPRINTF((stderr, "Reading a token: "));
          yychar = yylex();
        }

        if (yychar <= YYEOF)
        {
          yychar = yytoken = YYEOF;
          YYDPRINTF((stderr, "Now at end of input.\n"));
        }
        else
        {
          yytoken = YYTRANSLATE(yychar);
          YY_SYMBOL_PRINT("Next token is", yytoken, &yylval, &yylloc);
        }

        yygetLRActions(yystate, yytoken, &yyaction, &yyconflicts);
        if (*yyconflicts != 0)
          break;
        if (yyisShiftAction(yyaction))
        {
          YY_SYMBOL_PRINT("Shifting", yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
          yyposn += 1;
          yyglrShift(&yystack, 0, yyaction, yyposn, &yylval);
          if (0 < yystack.yyerrState)
            yystack.yyerrState -= 1;
        }
        else if (yyisErrorAction(yyaction))
        {
          yyreportSyntaxError(&yystack);
          goto yyuser_error;
        }
        else
          YYCHK1(yyglrReduce(&yystack, 0, -yyaction, yytrue));
      }
    }

    while (yytrue)
    {
      yySymbol yytoken_to_shift;
      size_t yys;

      for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
        yystackp->yytops.yylookaheadNeeds[yys] = (yybool)(yychar != YYEMPTY);

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
        YYCHK1(yyprocessOneStack(&yystack, yys, yyposn));
      yyremoveDeletes(&yystack);
      if (yystack.yytops.yysize == 0)
      {
        yyundeleteLastStack(&yystack);
        if (yystack.yytops.yysize == 0)
          yyFail(&yystack, YY_("syntax error"));
        YYCHK1(yyresolveStack(&yystack));
        YYDPRINTF((stderr, "Returning to deterministic operation.\n"));
        yyreportSyntaxError(&yystack);
        goto yyuser_error;
      }

      /* If any yyglrShift call fails, it will fail after shifting.  Thus,
         a copy of yylval will already be on stack 0 in the event of a
         failure in the following loop.  Thus, yychar is set to YYEMPTY
         before the loop to make sure the user destructor for yylval isn't
         called twice.  */
      yytoken_to_shift = YYTRANSLATE(yychar);
      yychar = YYEMPTY;
      yyposn += 1;
      for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
      {
        int yyaction;
        const short* yyconflicts;
        yyStateNum yystate = yystack.yytops.yystates[yys]->yylrState;
        yygetLRActions(yystate, yytoken_to_shift, &yyaction, &yyconflicts);
        /* Note that yyconflicts were handled by yyprocessOneStack.  */
        YYDPRINTF((stderr, "On stack %lu, ", (unsigned long)yys));
        YY_SYMBOL_PRINT("shifting", yytoken_to_shift, &yylval, &yylloc);
        yyglrShift(&yystack, yys, yyaction, yyposn, &yylval);
        YYDPRINTF((stderr, "Stack %lu now in state #%d\n", (unsigned long)yys,
          yystack.yytops.yystates[yys]->yylrState));
      }

      if (yystack.yytops.yysize == 1)
      {
        YYCHK1(yyresolveStack(&yystack));
        YYDPRINTF((stderr, "Returning to deterministic operation.\n"));
        yycompressStack(&yystack);
        break;
      }
    }
    continue;
  yyuser_error:
    yyrecoverSyntaxError(&yystack);
    yyposn = yystack.yytops.yystates[0]->yyposn;
  }

yyacceptlab:
  yyresult = 0;
  goto yyreturn;

yybuglab:
  YYASSERT(yyfalse);
  goto yyabortlab;

yyabortlab:
  yyresult = 1;
  goto yyreturn;

yyexhaustedlab:
  yyerror(YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;

yyreturn:
  if (yychar != YYEMPTY)
    yydestruct("Cleanup: discarding lookahead", YYTRANSLATE(yychar), &yylval);

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
            yyGLRState* yys = yystates[yyk];
            if (yys->yypred != YY_NULLPTR)
              yydestroyGLRState("Cleanup: popping", yys);
            yystates[yyk] = yys->yypred;
            yystack.yynextFree -= 1;
            yystack.yyspaceLeft += 1;
          }
          break;
        }
    }
    yyfreeGLRStack(&yystack);
  }

  return yyresult;
}

/* DEBUGGING ONLY */
#if YYDEBUG
static void yy_yypstack(yyGLRState* yys)
{
  if (yys->yypred)
  {
    yy_yypstack(yys->yypred);
    YYFPRINTF(stderr, " -> ");
  }
  YYFPRINTF(stderr, "%d@%lu", yys->yylrState, (unsigned long)yys->yyposn);
}

static void yypstates(yyGLRState* yyst)
{
  if (yyst == YY_NULLPTR)
    YYFPRINTF(stderr, "<null>");
  else
    yy_yypstack(yyst);
  YYFPRINTF(stderr, "\n");
}

static void yypstack(yyGLRStack* yystackp, size_t yyk)
{
  yypstates(yystackp->yytops.yystates[yyk]);
}

#define YYINDEX(YYX) ((YYX) == YY_NULLPTR ? -1 : (yyGLRStackItem*)(YYX)-yystackp->yyitems)

static void yypdumpstack(yyGLRStack* yystackp)
{
  yyGLRStackItem* yyp;
  size_t yyi;
  for (yyp = yystackp->yyitems; yyp < yystackp->yynextFree; yyp += 1)
  {
    YYFPRINTF(stderr, "%3lu. ", (unsigned long)(yyp - yystackp->yyitems));
    if (*(yybool*)yyp)
    {
      YYASSERT(yyp->yystate.yyisState);
      YYASSERT(yyp->yyoption.yyisState);
      YYFPRINTF(stderr, "Res: %d, LR State: %d, posn: %lu, pred: %ld", yyp->yystate.yyresolved,
        yyp->yystate.yylrState, (unsigned long)yyp->yystate.yyposn,
        (long)YYINDEX(yyp->yystate.yypred));
      if (!yyp->yystate.yyresolved)
        YYFPRINTF(stderr, ", firstVal: %ld", (long)YYINDEX(yyp->yystate.yysemantics.yyfirstVal));
    }
    else
    {
      YYASSERT(!yyp->yystate.yyisState);
      YYASSERT(!yyp->yyoption.yyisState);
      YYFPRINTF(stderr, "Option. rule: %d, state: %ld, next: %ld", yyp->yyoption.yyrule - 1,
        (long)YYINDEX(yyp->yyoption.yystate), (long)YYINDEX(yyp->yyoption.yynext));
    }
    YYFPRINTF(stderr, "\n");
  }
  YYFPRINTF(stderr, "Tops:");
  for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
    YYFPRINTF(
      stderr, "%lu: %ld; ", (unsigned long)yyi, (long)YYINDEX(yystackp->yytops.yystates[yyi]));
  YYFPRINTF(stderr, "\n");
}
#endif

#undef yylval
#undef yychar
#undef yynerrs

#include "lex.yy.c"
#include <string.h>

/* fill in the type name if none given */
static const char* type_class(unsigned int type, const char* classname)
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
static void start_class(const char* classname, int is_struct_or_union)
{
  ClassInfo* outerClass = currentClass;
  pushClass();
  currentClass = (ClassInfo*)malloc(sizeof(ClassInfo));
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

  if (getAttributes() & VTK_PARSE_WRAPEXCLUDE)
  {
    currentClass->IsExcluded = 1;
  }

  if (getAttributes() & VTK_PARSE_DEPRECATED)
  {
    currentClass->IsDeprecated = 1;
    currentClass->DeprecatedReason = deprecationReason;
    currentClass->DeprecatedVersion = deprecationVersion;
  }

  if (classname && classname[0] != '\0')
  {
    /* if name of class being defined contains "::" or "<..>", then skip it */
    const char* cp = classname;
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
  clearType();
  clearTypeId();
}

/* reached the end of a class definition */
static void end_class(void)
{
  /* add default constructors */
  vtkParse_AddDefaultConstructors(currentClass, data->Strings);

  popClass();
}

/* add a base class to the specified class */
static void add_base_class(ClassInfo* cls, const char* name, int access_lev, unsigned int extra)
{
  /* "extra" can contain VTK_PARSE_VIRTUAL and VTK_PARSE_PACK */
  if (cls && access_lev == VTK_ACCESS_PUBLIC && (extra & VTK_PARSE_VIRTUAL) == 0 &&
    (extra & VTK_PARSE_PACK) == 0)
  {
    vtkParse_AddStringToArray(&cls->SuperClasses, &cls->NumberOfSuperClasses, name);
  }
}

/* add a using declaration or directive */
static void add_using(const char* name, int is_namespace)
{
  size_t i;
  UsingInfo* item;

  item = (UsingInfo*)malloc(sizeof(UsingInfo));
  vtkParse_InitUsing(item);
  if (is_namespace)
  {
    item->Name = NULL;
    item->Scope = name;
  }
  else
  {
    i = strlen(name);
    while (i > 0 && name[i - 1] != ':')
    {
      i--;
    }
    item->Name = vtkstrdup(&name[i]);
    while (i > 0 && name[i - 1] == ':')
    {
      i--;
    }
    item->Scope = vtkstrndup(name, i);
    item->Access = access_level;
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
static void start_enum(const char* name, int is_scoped, unsigned int type, const char* basename)
{
  EnumInfo* item;

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
    item = (EnumInfo*)malloc(sizeof(EnumInfo));
    vtkParse_InitEnum(item);
    item->Name = name;
    item->Comment = vtkstrdup(getComment());
    item->Access = access_level;

    if (getAttributes() & VTK_PARSE_WRAPEXCLUDE)
    {
      item->IsExcluded = 1;
    }

    if (getAttributes() & VTK_PARSE_DEPRECATED)
    {
      item->IsDeprecated = 1;
      item->DeprecatedReason = deprecationReason;
      item->DeprecatedVersion = deprecationVersion;
    }

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
      vtkParse_AddStringToArray(
        &item->SuperClasses, &item->NumberOfSuperClasses, type_class(type, basename));
    }

    if (is_scoped)
    {
      pushClass();
      currentClass = item;
    }
  }
}

/* finish the enum */
static void end_enum(void)
{
  if (currentClass && currentClass->ItemType == VTK_ENUM_INFO)
  {
    popClass();
  }

  currentEnumName = NULL;
  currentEnumValue = NULL;
}

/* add a constant to the enum */
static void add_enum(const char* name, const char* value)
{
  static char text[2048];
  unsigned int attribs = getAttributes();
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
    while (i > 0 && text[i - 1] >= '0' && text[i - 1] <= '9')
    {
      i--;
    }

    if (i == 0 || text[i - 1] == ' ' ||
      (i > 1 && text[i - 2] == ' ' && (text[i - 1] == '-' || text[i - 1] == '+')))
    {
      if (i > 0 && text[i - 1] != ' ')
      {
        i--;
      }
      j = (int)strtol(&text[i], NULL, 10);
      sprintf(&text[i], "%li", j + 1);
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

  add_constant(name, currentEnumValue, attribs, currentEnumType, currentEnumName, 2);
}

/* for a macro constant, guess the constant type, doesn't do any math */
static unsigned int guess_constant_type(const char* valstring)
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
    strncmp(valstring, "static_cast<", 12) == 0 || strncmp(valstring, "const_cast<", 11) == 0 ||
    strncmp(valstring, "(", 1) == 0)
  {
    const char* cp;
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
    for (k = 0; k < n && cp[k] != ',' && cp[k] != '>' && cp[k] != ')'; k++)
    {
    }

    if (strncmp(cp, "long long", k) == 0)
    {
      valtype = VTK_PARSE_LONG_LONG;
    }
    else if (strncmp(cp, "__int64", k) == 0)
    {
      valtype = VTK_PARSE___INT64;
    }
    else if (strncmp(cp, "long", k) == 0)
    {
      valtype = VTK_PARSE_LONG;
    }
    else if (strncmp(cp, "short", k) == 0)
    {
      valtype = VTK_PARSE_SHORT;
    }
    else if (strncmp(cp, "signed char", k) == 0)
    {
      valtype = VTK_PARSE_SIGNED_CHAR;
    }
    else if (strncmp(cp, "char", k) == 0)
    {
      valtype = VTK_PARSE_CHAR;
    }
    else if (strncmp(cp, "int", k) == 0 || strncmp(cp, "signed", k) == 0)
    {
      valtype = VTK_PARSE_INT;
    }
    else if (strncmp(cp, "float", k) == 0)
    {
      valtype = VTK_PARSE_FLOAT;
    }
    else if (strncmp(cp, "double", k) == 0)
    {
      valtype = VTK_PARSE_DOUBLE;
    }
    else if (strncmp(cp, "char *", k) == 0)
    {
      valtype = VTK_PARSE_CHAR_PTR;
    }

    if (is_unsigned)
    {
      if (valtype == 0)
      {
        valtype = VTK_PARSE_INT;
      }
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
    NamespaceInfo* scope = currentNamespace;
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
    MacroInfo* macro = vtkParsePreprocess_GetMacro(preprocessor, valstring);

    if (macro && !macro->IsFunction)
    {
      return guess_constant_type(macro->Definition);
    }
  }

  /* fall back to the preprocessor to evaluate the constant */
  {
    preproc_int_t val;
    int is_unsigned;
    int result = vtkParsePreprocess_EvaluateExpression(preprocessor, valstring, &val, &is_unsigned);

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
        if ((preproc_uint_t)val <= UINT_MAX)
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
        if (val >= INT_MIN && val <= INT_MAX)
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
static void add_constant(const char* name, const char* value, unsigned int attributes,
  unsigned int type, const char* typeclass, int flag)
{
  ValueInfo* con = (ValueInfo*)malloc(sizeof(ValueInfo));
  vtkParse_InitValue(con);
  con->ItemType = VTK_CONSTANT_INFO;
  con->Name = name;
  con->Comment = vtkstrdup(getComment());
  con->Value = value;
  con->Attributes = attributes;
  con->Type = type;
  con->Class = type_class(type, typeclass);

  if (flag == 2)
  {
    con->IsEnum = 1;
  }

  if (flag == 1)
  {
    /* actually a macro, need to guess the type */
    ValueInfo** cptr = data->Contents->Constants;
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

/* guess the type from the ID */
static unsigned int guess_id_type(const char* cp)
{
  unsigned int t = 0;

  if (cp)
  {
    size_t i;
    const char* dp;

    i = strlen(cp);
    while (i > 0 && cp[i - 1] != ':')
    {
      i--;
    }
    dp = &cp[i];

    if (strcmp(dp, "vtkStdString") == 0 || strcmp(cp, "std::string") == 0)
    {
      t = VTK_PARSE_STRING;
    }
    else if (strncmp(dp, "vtk", 3) == 0)
    {
      t = VTK_PARSE_OBJECT;
    }
    else if (strncmp(dp, "Q", 1) == 0 || strncmp(cp, "Qt::", 4) == 0)
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
static void add_template_parameter(unsigned int datatype, unsigned int extra, const char* funcSig)
{
  ValueInfo* param = (ValueInfo*)malloc(sizeof(ValueInfo));
  vtkParse_InitValue(param);
  handle_complex_type(param, 0, datatype, extra, funcSig);
  param->Name = getVarName();
  vtkParse_AddParameterToTemplate(currentTemplate, param);
}

/* set the return type for the current function */
static void set_return(
  FunctionInfo* func, unsigned int attributes, unsigned int type, const char* typeclass, int count)
{
  char text[64];
  ValueInfo* val = (ValueInfo*)malloc(sizeof(ValueInfo));

  vtkParse_InitValue(val);
  val->Attributes = attributes;
  val->Type = type;
  val->Class = type_class(type, typeclass);

  if (count)
  {
    val->Count = count;
    snprintf(text, sizeof(text), "%i", count);
    vtkParse_AddStringToArray(&val->Dimensions, &val->NumberOfDimensions, vtkstrdup(text));
  }

  func->ReturnValue = val;

#ifndef VTK_PARSE_LEGACY_REMOVE
  func->ReturnType = val->Type;
  func->ReturnClass = val->Class;
  func->HaveHint = (count > 0);
  func->HintSize = count;
#endif
}

static int count_from_dimensions(ValueInfo* val)
{
  int count, i, n;
  const char* cp;

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
        while (*cp >= '0' && *cp <= '9')
        {
          cp++;
        }
        while (*cp == 'u' || *cp == 'l' || *cp == 'U' || *cp == 'L')
        {
          cp++;
        }
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
static void handle_complex_type(ValueInfo* val, unsigned int attributes, unsigned int datatype,
  unsigned int extra, const char* funcSig)
{
  FunctionInfo* func = 0;

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
    func->ReturnValue = (ValueInfo*)malloc(sizeof(ValueInfo));
    vtkParse_InitValue(func->ReturnValue);
    func->ReturnValue->Attributes = attributes;
    func->ReturnValue->Type = datatype;
    func->ReturnValue->Class = type_class(datatype, getTypeId());
    if (funcSig)
    {
      func->Signature = vtkstrdup(funcSig);
    }
    val->Function = func;

#ifndef VTK_PARSE_LEGACY_REMOVE
    func->ReturnType = func->ReturnValue->Type;
    func->ReturnClass = func->ReturnValue->Class;
#endif

    /* the val type is whatever was inside the parentheses */
    clearTypeId();
    setTypeId(func->Class ? "method" : "function");
    datatype = (extra & (VTK_PARSE_UNQUALIFIED_TYPE | VTK_PARSE_RVALUE));
    attributes = 0;
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

  /* get the attributes */
  val->Attributes = attributes;

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
static void handle_attribute(const char* att, int pack)
{
  /* the role means "this is what the attribute applies to" */
  int role = getAttributeRole();

  size_t l = 0;
  size_t la = 0;
  const char* args = NULL;

  if (!att)
  {
    return;
  }

  /* append the prefix from the "using" statement */
  if (getAttributePrefix())
  {
    att = vtkstrcat(getAttributePrefix(), att);
  }

  /* search for arguments */
  l = vtkParse_SkipId(att);
  while (att[l] == ':' && att[l + 1] == ':')
  {
    l += 2;
    l += vtkParse_SkipId(&att[l]);
  }
  if (att[l] == '(')
  {
    /* strip the parentheses and whitespace from the args */
    args = &att[l + 1];
    while (*args == ' ')
    {
      args++;
    }
    la = strlen(args);
    while (la > 0 && args[la - 1] == ' ')
    {
      la--;
    }
    if (la > 0 && args[la - 1] == ')')
    {
      la--;
    }
    while (la > 0 && args[la - 1] == ' ')
    {
      la--;
    }
  }

  /* check for namespace */
  if (strncmp(att, "vtk::", 5) == 0)
  {
    if (pack)
    {
      /* no current vtk attributes use '...' */
      print_parser_error("attribute takes no ...", att, l);
      exit(1);
    }
    else if (l == 16 && strncmp(att, "vtk::wrapexclude", l) == 0 && !args &&
      (role == VTK_PARSE_ATTRIB_DECL || role == VTK_PARSE_ATTRIB_CLASS))
    {
      addAttribute(VTK_PARSE_WRAPEXCLUDE);
    }
    else if (l == 16 && strncmp(att, "vtk::newinstance", l) == 0 && !args &&
      role == VTK_PARSE_ATTRIB_DECL)
    {
      addAttribute(VTK_PARSE_NEWINSTANCE);
    }
    else if (l == 13 && strncmp(att, "vtk::zerocopy", l) == 0 && !args &&
      role == VTK_PARSE_ATTRIB_DECL)
    {
      addAttribute(VTK_PARSE_ZEROCOPY);
    }
    else if (l == 13 && strncmp(att, "vtk::filepath", l) == 0 && !args &&
      role == VTK_PARSE_ATTRIB_DECL)
    {
      addAttribute(VTK_PARSE_FILEPATH);
    }
    else if (l == 15 && strncmp(att, "vtk::deprecated", l) == 0 &&
      (role == VTK_PARSE_ATTRIB_DECL || role == VTK_PARSE_ATTRIB_CLASS ||
        role == VTK_PARSE_ATTRIB_ID))
    {
      addAttribute(VTK_PARSE_DEPRECATED);
      deprecationReason = NULL;
      deprecationVersion = NULL;
      if (args)
      {
        size_t lr = vtkParse_SkipQuotes(args);
        deprecationReason = vtkstrndup(args, lr);
        if (lr < la && args[lr] == ',')
        {
          /* skip spaces and get the next argument */
          do
          {
            ++lr;
          } while (lr < la && args[lr] == ' ');
          deprecationVersion = vtkstrndup(&args[lr], vtkParse_SkipQuotes(&args[lr]));
        }
      }
    }
    else if (l == 12 && strncmp(att, "vtk::expects", l) == 0 && args &&
      role == VTK_PARSE_ATTRIB_FUNC)
    {
      /* add to the preconditions */
      vtkParse_AddStringToArray(
        &currentFunction->Preconds, &currentFunction->NumberOfPreconds, vtkstrndup(args, la));
    }
    else if (l == 13 && strncmp(att, "vtk::sizehint", l) == 0 && args &&
      role == VTK_PARSE_ATTRIB_FUNC)
    {
      /* first arg is parameter name, unless return value hint */
      ValueInfo* arg = currentFunction->ReturnValue;
      size_t n = vtkParse_SkipId(args);
      preproc_int_t count;
      int is_unsigned;
      int i;

      l = n;
      while (args[n] == ' ')
      {
        n++;
      }
      if (l > 0 && args[n] == ',')
      {
        do
        {
          n++;
        } while (args[n] == ' ');
        /* find the named parameter */
        for (i = 0; i < currentFunction->NumberOfParameters; i++)
        {
          arg = currentFunction->Parameters[i];
          if (arg->Name && strlen(arg->Name) == l && strncmp(arg->Name, args, l) == 0)
          {
            break;
          }
        }
        if (i == currentFunction->NumberOfParameters)
        {
          print_parser_error("unrecognized parameter name", args, l);
          exit(1);
        }
        /* advance args to second attribute arg */
        args += n;
        la -= n;
      }
      /* set the size hint */
      arg->CountHint = vtkstrndup(args, la);
      /* see if hint is an integer */
      if (VTK_PARSE_OK ==
        vtkParsePreprocess_EvaluateExpression(preprocessor, arg->CountHint, &count, &is_unsigned))
      {
        if (count > 0 && count < 127)
        {
          arg->CountHint = NULL;
          arg->Count = (int)count;
#ifndef VTK_PARSE_LEGACY_REMOVE
          if (arg == currentFunction->ReturnValue)
          {
            currentFunction->HaveHint = 1;
            currentFunction->HintSize = arg->Count;
          }
#endif
        }
      }
    }
    else
    {
      print_parser_error("attribute cannot be used here", att, l);
      exit(1);
    }
  }
}

/* add a parameter to the legacy part of the FunctionInfo struct */
static void add_legacy_parameter(FunctionInfo* func, ValueInfo* param)
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
static void reject_function(void)
{
  vtkParse_FreeFunction(currentFunction);
  currentFunction = (FunctionInfo*)malloc(sizeof(FunctionInfo));
  vtkParse_InitFunction(currentFunction);
  startSig();
  getMacro();
}

/* a simple routine that updates a few variables */
static void output_function(void)
{
  size_t n;
  int i, j;
  int match;

  /* reject template specializations */
  n = strlen(currentFunction->Name);
  if (currentFunction->Name[n - 1] == '>')
  {
    /* make sure there is a matching angle bracket */
    while (n > 0 && currentFunction->Name[n - 1] != '<')
    {
      n--;
    }
    if (n > 0)
    {
      reject_function();
      return;
    }
  }

  /* check return value for specifiers that apply to the function */
  if (currentFunction->ReturnValue)
  {
    if (currentFunction->ReturnValue->Attributes & VTK_PARSE_WRAPEXCLUDE)
    {
      /* remove "wrapexclude" attrib from ReturnValue, attach it to function */
      currentFunction->ReturnValue->Attributes ^= VTK_PARSE_WRAPEXCLUDE;
      currentFunction->IsExcluded = 1;
    }

    if (currentFunction->ReturnValue->Attributes & VTK_PARSE_DEPRECATED)
    {
      /* remove "deprecated" attrib from ReturnValue, attach it to function */
      currentFunction->ReturnValue->Attributes ^= VTK_PARSE_DEPRECATED;
      currentFunction->IsDeprecated = 1;
      currentFunction->DeprecatedReason = deprecationReason;
      currentFunction->DeprecatedVersion = deprecationVersion;
    }

    if (currentFunction->ReturnValue->Type & VTK_PARSE_FRIEND)
    {
      /* remove "friend" specifier from ReturnValue */
      currentFunction->ReturnValue->Type ^= VTK_PARSE_FRIEND;
      /* handle the function declaration (ignore the "friend" part) */
      output_friend_function();
      return;
    }

    if (currentFunction->ReturnValue->Type & VTK_PARSE_TYPEDEF)
    {
      /* remove 'typedef' specifier from return value */
      currentFunction->ReturnValue->Type ^= VTK_PARSE_TYPEDEF;
      /* we ignore function typedefs, they're exceedingly rare */
      reject_function();
      return;
    }

    if (currentFunction->ReturnValue->Type & VTK_PARSE_STATIC)
    {
      /* mark function or method as "static" */
      currentFunction->IsStatic = 1;
    }

    if (currentFunction->ReturnValue->Type & VTK_PARSE_VIRTUAL)
    {
      /* mark method as "virtual" */
      currentFunction->IsVirtual = 1;
    }
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
    (currentFunction->Parameters[0]->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID)
  {
    vtkParse_FreeValue(currentFunction->Parameters[0]);
    free(currentFunction->Parameters);
    currentFunction->NumberOfParameters = 0;
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
    (currentFunction->ArgTypes[0] & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID)
  {
    currentFunction->NumberOfArguments = 0;
  }

  /* if return type is void, set return class to void */
  if (currentFunction->ReturnClass == NULL &&
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID)
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
    ValueInfo* param = currentFunction->Parameters[i];
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
    if (currentFunction->Name && !strcmp("Delete", currentFunction->Name))
    {
      currentClass->HasDelete = 1;
    }

    currentFunction->Class = currentClass->Name;
    vtkParse_AddFunctionToClass(currentClass, currentFunction);

    currentFunction = (FunctionInfo*)malloc(sizeof(FunctionInfo));
  }
  else
  {
    /* make sure this function isn't a repeat */
    match = 0;
    for (i = 0; i < currentNamespace->NumberOfFunctions; i++)
    {
      if (currentNamespace->Functions[i]->Name &&
        strcmp(currentNamespace->Functions[i]->Name, currentFunction->Name) == 0)
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

      currentFunction = (FunctionInfo*)malloc(sizeof(FunctionInfo));
    }
  }

  vtkParse_InitFunction(currentFunction);
  startSig();
}

/* output a function that is not a method of the current class */
static void output_friend_function(void)
{
  ClassInfo* tmpc = currentClass;
  currentClass = NULL;
  output_function();
  currentClass = tmpc;
}

/* dump predefined macros to the specified file. */
static void dump_macros(const char* filename)
{
  MacroInfo* macro = NULL;
  FILE* ofile = stdout;
  int i;

  if (filename)
  {
    ofile = vtkParse_FileOpen(filename, "w");
    if (!ofile)
    {
      fprintf(stderr, "Error opening output file %s\n", filename);
      return;
    }
  }

  while ((macro = vtkParsePreprocess_NextMacro(preprocessor, macro)) != 0)
  {
    if (macro->IsFunction)
    {
      fprintf(ofile, "#define %s(", macro->Name);
      for (i = 0; i < macro->NumberOfParameters; i++)
      {
        fprintf(ofile, "%s%s", (i == 0 ? "" : ","), macro->Parameters[i]);
      }
      fprintf(ofile, ")%s%s\n", (macro->Definition ? " " : ""), macro->Definition);
    }
    else if (macro->Definition)
    {
      fprintf(ofile, "#define %s %s\n", macro->Name, macro->Definition);
    }
    else
    {
      fprintf(ofile, "#define %s\n", macro->Name);
    }
  }

  if (filename)
  {
    fclose(ofile);
  }
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
void vtkParse_SetCommandName(const char* name)
{
  CommandName = name;
}

/* Parse a header file and return a FileInfo struct */
FileInfo* vtkParse_ParseFile(const char* filename, FILE* ifile, FILE* errfile)
{
  int i, j;
  int ret;
  FileInfo* file_info;
  char* main_class;

  /* "data" is a global variable used by the parser */
  data = (FileInfo*)malloc(sizeof(FileInfo));
  vtkParse_InitFile(data);
  data->Strings = &system_strings;

  /* "preprocessor" is a global struct used by the parser */
  preprocessor = (PreprocessInfo*)malloc(sizeof(PreprocessInfo));
  vtkParsePreprocess_Init(preprocessor, filename);
  preprocessor->Strings = data->Strings;
  preprocessor->System = &system_cache;
  vtkParsePreprocess_AddStandardMacros(
    preprocessor, PredefinePlatformMacros ? VTK_PARSE_NATIVE : VTK_PARSE_UNDEF);

  /* add include files specified on the command line */
  for (i = 0; i < NumberOfIncludeDirectories; i++)
  {
    vtkParsePreprocess_IncludeDirectory(preprocessor, IncludeDirectories[i]);
  }

  /* add macros specified on the command line */
  for (i = 0; i < NumberOfDefinitions; i++)
  {
    const char* cp = Definitions[i];

    if (*cp == 'U')
    {
      vtkParsePreprocess_RemoveMacro(preprocessor, &cp[1]);
    }
    else if (*cp == 'D')
    {
      const char* definition = &cp[1];
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

  /* add include files that contain macros to pre-define */
  for (i = 0; i < NumberOfMacroIncludes; i++)
  {
    vtkParsePreprocess_IncludeFile(preprocessor, MacroIncludes[i], VTK_PARSE_CURDIR_INCLUDE);
  }

  data->FileName = vtkstrdup(filename);

  clearComment();

  namespaceDepth = 0;
  currentNamespace = (NamespaceInfo*)malloc(sizeof(NamespaceInfo));
  vtkParse_InitNamespace(currentNamespace);
  data->Contents = currentNamespace;

  templateDepth = 0;
  currentTemplate = NULL;

  currentFunction = (FunctionInfo*)malloc(sizeof(FunctionInfo));
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
  main_class = (char*)malloc(j - i + 1);
  strncpy(main_class, &filename[i], j - i);
  main_class[j - i] = '\0';

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

  /* dump macros, for diagnostic purposes */
  if (DumpMacros)
  {
    dump_macros(DumpFileName);
  }

  vtkParsePreprocess_Free(preprocessor);
  preprocessor = NULL;
  macroName = NULL;

  file_info = data;
  data = NULL;

  return file_info;
}

/* Read a hints file and update the FileInfo */
int vtkParse_ReadHints(FileInfo* file_info, FILE* hfile, FILE* errfile)
{
  char h_cls[512];
  char h_func[512];
  unsigned int h_type, type;
  int h_value;
  FunctionInfo* func_info;
  ClassInfo* class_info;
  NamespaceInfo* contents;
  int i, j;
  int lineno = 0;
  int n;

  contents = file_info->Contents;

  /* read each hint line in succession */
  while ((n = fscanf(hfile, "%511s %511s %x %i", h_cls, h_func, &h_type, &h_value)) != EOF)
  {
    lineno++;
    if (n < 4)
    {
      fprintf(errfile, "Wrapping: error parsing hints file line %i\n", lineno);
      exit(1);
    }

    /* erase "ref" and qualifiers from hint type */
    type = ((h_type & VTK_PARSE_BASE_TYPE) | (h_type & VTK_PARSE_POINTER_LOWMASK));

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

          if ((strcmp(h_func, func_info->Name) == 0) && func_info->ReturnValue &&
            (type ==
              ((func_info->ReturnValue->Type & ~VTK_PARSE_REF) & VTK_PARSE_UNQUALIFIED_TYPE)))
          {
            /* types that hints are accepted for */
            switch (func_info->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE)
            {
              case VTK_PARSE_FLOAT_PTR:
              case VTK_PARSE_VOID_PTR:
              case VTK_PARSE_DOUBLE_PTR:
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
                  snprintf(text, sizeof(text), "%i", h_value);
                  func_info->ReturnValue->Count = h_value;
                  vtkParse_AddStringToArray(&func_info->ReturnValue->Dimensions,
                    &func_info->ReturnValue->NumberOfDimensions,
                    vtkParse_CacheString(file_info->Strings, text, strlen(text)));
#ifndef VTK_PARSE_LEGACY_REMOVE
                  func_info->HaveHint = 1;
                  func_info->HintSize = h_value;
#endif
                }
                break;
              }
              default:
              {
                fprintf(errfile, "Wrapping: unhandled hint type %#x\n", h_type);
              }
            }
          }
        }
      }
    }
  }

  return 1;
}

/* Free any caches or buffers, call just before program exits */
void vtkParse_FinalCleanup(void)
{
  vtkParse_FreeFileCache(&system_cache);
  vtkParse_FreeStringCache(&system_strings);
}

/* Free the FileInfo struct returned by vtkParse_ParseFile() */
void vtkParse_Free(FileInfo* file_info)
{
  vtkParse_FreeFile(file_info);
  // system_strings will be released at program exit
  if (file_info->Strings && file_info->Strings != &system_strings)
  {
    vtkParse_FreeStringCache(file_info->Strings);
    free(file_info->Strings);
  }
  free(file_info);
}

/** Define a preprocessor macro. Function macros are not supported.  */
void vtkParse_DefineMacro(const char* name, const char* definition)
{
  size_t n = vtkParse_SkipId(name);
  size_t l;
  char* cp;

  if (definition == NULL)
  {
    definition = "";
  }

  l = n + strlen(definition) + 2;
  cp = (char*)malloc(l + 1);
  cp[0] = 'D';
  strncpy(&cp[1], name, n);
  cp[n + 1] = '\0';
  if (definition[0] != '\0')
  {
    cp[n + 1] = '=';
    strcpy(&cp[n + 2], definition);
  }
  cp[l] = '\0';

  vtkParse_AddStringToArray(&Definitions, &NumberOfDefinitions, cp);
}

/** Undefine a preprocessor macro.  */
void vtkParse_UndefineMacro(const char* name)
{
  size_t n = vtkParse_SkipId(name);
  char* cp;

  cp = (char*)malloc(n + 2);
  cp[0] = 'U';
  strncpy(&cp[1], name, n);
  cp[n + 1] = '\0';

  vtkParse_AddStringToArray(&Definitions, &NumberOfDefinitions, cp);
}

/** Do not define any platform-specific macros.  */
void vtkParse_UndefinePlatformMacros()
{
  PredefinePlatformMacros = 0;
}

/** Add an include file to read macros from, for use with -imacro. */
void vtkParse_IncludeMacros(const char* filename)
{
  size_t n = strlen(filename);
  char* cp;

  cp = (char*)malloc(n + 1);
  strcpy(cp, filename);

  vtkParse_AddStringToArray(&MacroIncludes, &NumberOfMacroIncludes, cp);
}

/** Dump macros to the specified file (stdout if NULL). */
void vtkParse_DumpMacros(const char* filename)
{
  DumpMacros = 1;
  DumpFileName = filename;
}

/** Add an include directory, for use with the "-I" option.  */
void vtkParse_IncludeDirectory(const char* dirname)
{
  size_t n = strlen(dirname);
  char* cp;
  int i;

  for (i = 0; i < NumberOfIncludeDirectories; i++)
  {
    if (strncmp(IncludeDirectories[i], dirname, n) == 0 && IncludeDirectories[i][n] == '\0')
    {
      return;
    }
  }

  cp = (char*)malloc(n + 1);
  strcpy(cp, dirname);

  vtkParse_AddStringToArray(&IncludeDirectories, &NumberOfIncludeDirectories, cp);
}

/** Return the full path to a header file.  */
const char* vtkParse_FindIncludeFile(const char* filename)
{
  static StringCache string_cache = { 0, 0, 0, 0 };
  static PreprocessInfo info = { 0, 0, 0, 0, 0, 0, &string_cache, 0, 0, 0, 0, 0, 0, &system_cache };
  int val;
  int i;

  /* add include files specified on the command line */
  for (i = 0; i < NumberOfIncludeDirectories; i++)
  {
    vtkParsePreprocess_IncludeDirectory(&info, IncludeDirectories[i]);
  }

  return vtkParsePreprocess_FindIncludeFile(&info, filename, VTK_PARSE_SOURCE_INCLUDE, &val);
}
