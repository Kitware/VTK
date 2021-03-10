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
const char* currentDeprecation = NULL;
parse_access_t access_level = VTK_ACCESS_PUBLIC;

/* functions from vtkParse.l */
void print_parser_error(const char* text, const char* cp, size_t n);

/* helper functions */
const char* type_class(unsigned int type, const char* classname);
void start_class(const char* classname, int is_struct_or_union);
void end_class(void);
void add_base_class(ClassInfo* cls, const char* name, int access_lev, unsigned int extra);
void output_friend_function(void);
void output_function(void);
void reject_function(void);
void set_return(
  FunctionInfo* func, unsigned int attributes, unsigned int type, const char* typeclass, int count);
void add_template_parameter(unsigned int datatype, unsigned int extra, const char* funcSig);
void add_using(const char* name, int is_namespace);
void start_enum(const char* name, int is_scoped, unsigned int type, const char* basename);
void add_enum(const char* name, const char* value);
void end_enum(void);
unsigned int guess_constant_type(const char* valstring);
void add_constant(
  const char* name, const char* value, unsigned int type, const char* typeclass, int flag);
void prepend_scope(char* cp, const char* arg);
unsigned int guess_id_type(const char* cp);
unsigned int add_indirection(unsigned int type1, unsigned int type2);
unsigned int add_indirection_to_array(unsigned int type);
void handle_complex_type(ValueInfo* val, unsigned int attributes, unsigned int datatype,
  unsigned int extra, const char* funcSig);
void handle_function_type(ValueInfo* param, const char* name, const char* funcSig);
void handle_attribute(const char* att, int pack);
void add_legacy_parameter(FunctionInfo* func, ValueInfo* param);

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
struct DoxygenCommandInfo doxygenCommands[] = { { "def", 3, DOX_COMMAND_DEF },
  { "category", 8, DOX_COMMAND_CATEGORY }, { "interface", 9, DOX_COMMAND_INTERFACE },
  { "protocol", 8, DOX_COMMAND_PROTOCOL }, { "class", 5, DOX_COMMAND_CLASS },
  { "enum", 4, DOX_COMMAND_ENUM }, { "struct", 6, DOX_COMMAND_STRUCT },
  { "union", 5, DOX_COMMAND_UNION }, { "namespace", 9, DOX_COMMAND_NAMESPACE },
  { "typedef", 7, DOX_COMMAND_TYPEDEF }, { "fn", 2, DOX_COMMAND_FN },
  { "property", 8, DOX_COMMAND_PROPERTY }, { "var", 3, DOX_COMMAND_VAR },
  { "name", 4, DOX_COMMAND_NAME }, { "defgroup", 8, DOX_COMMAND_DEFGROUP },
  { "addtogroup", 10, DOX_COMMAND_ADDTOGROUP }, { "weakgroup", 9, DOX_COMMAND_WEAKGROUP },
  { "example", 7, DOX_COMMAND_EXAMPLE }, { "file", 4, DOX_COMMAND_FILE },
  { "dir", 3, DOX_COMMAND_DIR }, { "mainpage", 8, DOX_COMMAND_MAINPAGE },
  { "page", 4, DOX_COMMAND_PAGE }, { "subpage", 7, DOX_COMMAND_SUBPAGE },
  { "internal", 8, DOX_COMMAND_INTERNAL }, { "package", 7, DOX_COMMAND_PACKAGE },
  { "privatesection", 14, DOX_COMMAND_PRIVATESECTION },
  { "protectedsection", 16, DOX_COMMAND_PROTECTEDSECTION },
  { "publicsection", 13, DOX_COMMAND_PUBLICSECTION }, { NULL, 0, DOX_COMMAND_OTHER } };

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
const char* getComment(void)
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
parse_dox_t checkDoxygenCommand(const char* text, size_t n)
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
void storeComment(void)
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
void applyComment(ClassInfo* cls)
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

const char* getMacro(void)
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
void pushNamespace(const char* name)
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
void popNamespace(void)
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
void pushClass(void)
{
  classAccessStack[classDepth] = access_level;
  classStack[classDepth++] = currentClass;
}

/* leave the internal class */
void popClass(void)
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
void startTemplate(void)
{
  currentTemplate = (TemplateInfo*)malloc(sizeof(TemplateInfo));
  vtkParse_InitTemplate(currentTemplate);
}

/* clear a template, if set */
void clearTemplate(void)
{
  if (currentTemplate)
  {
    free(currentTemplate);
  }
  currentTemplate = NULL;
}

/* push the template onto the stack, and start a new one */
void pushTemplate(void)
{
  templateStack[templateDepth++] = currentTemplate;
  startTemplate();
}

/* pop a template off the stack */
void popTemplate(void)
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
void startSig(void)
{
  signature = NULL;
  sigLength = 0;
  sigAllocatedLength = 0;
  sigClosed = 0;
  sigMarkDepth = 0;
  sigMark[0] = 0;
}

/* get the signature */
const char* getSig(void)
{
  return signature;
}

/* get the signature length */
size_t getSigLength(void)
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
void closeSig(void)
{
  sigClosed = 1;
}

/* re-open the signature */
void openSig(void)
{
  sigClosed = 0;
}

/* insert text at the beginning of the signature */
void preSig(const char* arg)
{
  if (!sigClosed)
  {
    size_t n = strlen(arg);
    checkSigSize(n);
    if (n > 0)
    {
      memmove(&signature[n], signature, sigLength);
      memmove(signature, arg, n);
      sigLength += n;
    }
    signature[sigLength] = '\0';
  }
}

/* append text to the end of the signature */
void postSig(const char* arg)
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
void markSig(void)
{
  sigMark[sigMarkDepth] = 0;
  if (signature)
  {
    sigMark[sigMarkDepth] = sigLength;
  }
  sigMarkDepth++;
}

/* get the contents of the sig from the mark, and clear the mark */
const char* copySig(void)
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
const char* cutSig(void)
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

/* swap the signature text using the mark as the radix */
void swapSig(void)
{
  if (sigMarkDepth > 0)
  {
    sigMarkDepth--;
  }
  if (signature && sigMark[sigMarkDepth] > 0)
  {
    size_t i, m, n, nn;
    char c;
    char* cp;
    cp = signature;
    n = sigLength;
    m = sigMark[sigMarkDepth];
    nn = m / 2;
    for (i = 0; i < nn; i++)
    {
      c = cp[i];
      cp[i] = cp[m - i - 1];
      cp[m - i - 1] = c;
    }
    nn = (n - m) / 2;
    for (i = 0; i < nn; i++)
    {
      c = cp[i + m];
      cp[i + m] = cp[n - i - 1];
      cp[n - i - 1] = c;
    }
    nn = n / 2;
    for (i = 0; i < nn; i++)
    {
      c = cp[i];
      cp[i] = cp[n - i - 1];
      cp[n - i - 1] = c;
    }
  }
}

/* chop the last space from the signature */
void chopSig(void)
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
void postSigLeftBracket(const char* s)
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
void postSigRightBracket(const char* s)
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
void pushType()
{
  attributeStack[typeDepth] = declAttributes;
  typeStack[typeDepth++] = storedType;
}

/* pop the type stack */
void popType()
{
  storedType = typeStack[--typeDepth];
  declAttributes = attributeStack[typeDepth];
}

/* clear the storage type */
void clearType()
{
  storedType = 0;
  declAttributes = 0;
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
unsigned int getType(void)
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

/* add an attribute specifier to the current declaration */
void addAttribute(unsigned int flags)
{
  declAttributes |= flags;
}

/* check if an attribute is set for the current declaration */
int getAttributes()
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
void clearArray(void)
{
  numberOfDimensions = 0;
  arrayDimensions = NULL;
}

/* add another dimension */
void pushArraySize(const char* size)
{
  vtkParse_AddStringToArray(&arrayDimensions, &numberOfDimensions, size);
}

/* add another dimension to the front */
void pushArrayFront(const char* size)
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
int getArrayNDims(void)
{
  return numberOfDimensions;
}

/* get the whole array */
const char** getArray(void)
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
void clearVarName(void)
{
  currentVarName = NULL;
}

/* set the var Id */
void setVarName(const char* text)
{
  currentVarName = text;
}

/* return the var id */
const char* getVarName(void)
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
void setVarValue(const char* text)
{
  currentVarValue = text;
}

/* return the var value */
const char* getVarValue(void)
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
void setTypeId(const char* text)
{
  if (currentId == NULL)
  {
    currentId = text;
  }
}

/* set the signature and type together */
void typeSig(const char* text)
{
  postSig(text);
  postSig(" ");

  if (currentId == 0)
  {
    setTypeId(text);
  }
}

/* return the current Id */
const char* getTypeId(void)
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
void scopeSig(const char* scope)
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
const char* getScope(void)
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

void pushFunction(void)
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

void popFunction(void)
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

FunctionInfo* getFunction(void)
{
  return functionStack[functionDepth + 1];
}

/*----------------------------------------------------------------
 * Attributes
 */

int attributeRole = 0;
const char* attributePrefix = NULL;

/* Set kind of attributes to collect in attribute_specifier_seq */
void setAttributeRole(int x)
{
  attributeRole = x;
}

/* Get the current kind of attribute */
int getAttributeRole(void)
{
  return attributeRole;
}

/* Ignore attributes until further notice */
void clearAttributeRole(void)
{
  attributeRole = 0;
}

/* Set the "using" prefix for attributes */
void setAttributePrefix(const char* x)
{
  attributePrefix = x;
}

/* Get the "using" prefix for attributes */
const char* getAttributePrefix(void)
{
  return attributePrefix;
}

/*----------------------------------------------------------------
 * Utility methods
 */

/* prepend a scope:: to a name */
void prepend_scope(char* cp, const char* arg)
{
  size_t i, j, m, n;
  int depth;

  m = strlen(cp);
  n = strlen(arg);
  i = m;
  while (i > 0 && (vtkParse_CharType(cp[i - 1], CPRE_XID) || cp[i - 1] == ':' || cp[i - 1] == '>'))
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
    cp[j + n + 1] = cp[j - 1];
  }
  for (j = 0; j < n; j++)
  {
    cp[j + i] = arg[j];
  }
  cp[n + i] = ':';
  cp[n + i + 1] = ':';
  cp[m + n + 2] = '\0';
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
  UNSIGNED = 356
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
#define YYLAST 6666

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 125
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 275
/* YYNRULES -- Number of rules.  */
#define YYNRULES 673
/* YYNRULES -- Number of states.  */
#define YYNSTATES 1046
/* YYMAXRHS -- Maximum number of symbols on right-hand side of rule.  */
#define YYMAXRHS 8
/* YYMAXLEFT -- Maximum number of symbols to the left of a handle
   accessed by $0, $-1, etc., in any rule.  */
#define YYMAXLEFT 0

/* YYTRANSLATE(X) -- Bison symbol number corresponding to X.  */
#define YYUNDEFTOK 2
#define YYMAXUTOK 356

#define YYTRANSLATE(YYX) ((unsigned)(YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] = { 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 121, 2, 2, 2, 117, 111, 2, 108, 109, 115, 120, 107,
  119, 124, 118, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 106, 102, 110, 105, 116, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 112, 2, 113, 123, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 103, 122, 104, 114, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
  34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
  82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101 };

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] = { 0, 1918, 1918, 1920, 1922, 1921, 1932, 1933, 1934, 1935,
  1936, 1937, 1938, 1939, 1940, 1941, 1942, 1943, 1944, 1945, 1946, 1947, 1948, 1951, 1952, 1953,
  1954, 1955, 1956, 1959, 1960, 1967, 1974, 1975, 1975, 1979, 1986, 1987, 1990, 1991, 1992, 1995,
  1996, 1999, 1999, 2014, 2013, 2019, 2025, 2024, 2029, 2035, 2036, 2037, 2040, 2042, 2044, 2047,
  2048, 2051, 2052, 2054, 2056, 2055, 2064, 2068, 2069, 2070, 2073, 2074, 2075, 2076, 2077, 2078,
  2079, 2080, 2081, 2082, 2083, 2084, 2085, 2086, 2089, 2090, 2091, 2092, 2093, 2094, 2097, 2098,
  2099, 2100, 2104, 2105, 2108, 2110, 2113, 2118, 2119, 2122, 2123, 2126, 2127, 2128, 2139, 2140,
  2141, 2145, 2146, 2150, 2150, 2163, 2169, 2177, 2178, 2179, 2182, 2183, 2183, 2187, 2188, 2190,
  2191, 2192, 2192, 2200, 2204, 2205, 2208, 2210, 2212, 2213, 2216, 2217, 2225, 2226, 2229, 2230,
  2232, 2234, 2236, 2240, 2242, 2243, 2246, 2249, 2250, 2253, 2254, 2253, 2258, 2300, 2303, 2304,
  2305, 2307, 2309, 2311, 2315, 2318, 2318, 2351, 2354, 2353, 2371, 2373, 2372, 2377, 2379, 2377,
  2381, 2383, 2381, 2385, 2388, 2385, 2399, 2400, 2403, 2404, 2406, 2407, 2410, 2410, 2420, 2421,
  2429, 2430, 2431, 2432, 2435, 2438, 2439, 2440, 2443, 2444, 2445, 2448, 2449, 2450, 2454, 2455,
  2456, 2457, 2460, 2461, 2462, 2466, 2471, 2465, 2483, 2487, 2498, 2497, 2506, 2510, 2513, 2523,
  2527, 2528, 2531, 2532, 2534, 2535, 2536, 2539, 2540, 2542, 2543, 2544, 2546, 2547, 2550, 2563,
  2564, 2565, 2566, 2573, 2574, 2577, 2577, 2585, 2586, 2587, 2590, 2592, 2593, 2597, 2596, 2613,
  2635, 2609, 2646, 2646, 2649, 2650, 2653, 2654, 2657, 2658, 2664, 2665, 2665, 2668, 2669, 2669,
  2671, 2673, 2677, 2679, 2677, 2704, 2705, 2708, 2708, 2710, 2710, 2712, 2712, 2717, 2718, 2718,
  2726, 2729, 2801, 2802, 2804, 2805, 2805, 2808, 2811, 2812, 2816, 2828, 2827, 2846, 2848, 2848,
  2866, 2866, 2868, 2872, 2873, 2874, 2873, 2879, 2881, 2882, 2883, 2884, 2885, 2886, 2889, 2890,
  2894, 2895, 2899, 2900, 2903, 2904, 2908, 2909, 2910, 2911, 2914, 2915, 2918, 2918, 2921, 2922,
  2925, 2925, 2929, 2930, 2930, 2937, 2938, 2941, 2942, 2943, 2944, 2945, 2948, 2950, 2952, 2956,
  2958, 2960, 2962, 2964, 2966, 2968, 2968, 2973, 2976, 2979, 2982, 2982, 2990, 2990, 2999, 3000,
  3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3015, 3016, 3017, 3018, 3019, 3020, 3021, 3027,
  3028, 3031, 3032, 3034, 3035, 3038, 3039, 3042, 3043, 3044, 3045, 3048, 3049, 3050, 3051, 3052,
  3056, 3057, 3058, 3061, 3062, 3065, 3066, 3074, 3077, 3077, 3079, 3079, 3083, 3084, 3086, 3090,
  3091, 3093, 3093, 3096, 3098, 3102, 3105, 3105, 3107, 3107, 3111, 3114, 3114, 3116, 3116, 3120,
  3121, 3123, 3125, 3127, 3129, 3131, 3135, 3136, 3139, 3140, 3141, 3142, 3143, 3144, 3145, 3146,
  3147, 3148, 3151, 3152, 3153, 3154, 3155, 3156, 3157, 3158, 3159, 3160, 3161, 3162, 3163, 3164,
  3165, 3185, 3186, 3187, 3188, 3191, 3195, 3199, 3199, 3203, 3204, 3219, 3220, 3245, 3245, 3249,
  3249, 3253, 3253, 3257, 3257, 3261, 3261, 3265, 3265, 3268, 3269, 3272, 3276, 3277, 3280, 3283,
  3284, 3285, 3286, 3289, 3289, 3293, 3294, 3297, 3298, 3301, 3302, 3309, 3310, 3311, 3312, 3313,
  3314, 3315, 3316, 3317, 3318, 3319, 3320, 3323, 3324, 3325, 3326, 3327, 3328, 3329, 3330, 3331,
  3332, 3333, 3334, 3335, 3336, 3337, 3338, 3339, 3340, 3341, 3342, 3343, 3344, 3345, 3346, 3347,
  3348, 3349, 3350, 3351, 3352, 3353, 3354, 3355, 3356, 3359, 3360, 3361, 3362, 3363, 3364, 3365,
  3366, 3367, 3368, 3369, 3370, 3371, 3372, 3373, 3374, 3375, 3376, 3377, 3378, 3379, 3380, 3381,
  3382, 3383, 3384, 3385, 3386, 3387, 3388, 3391, 3392, 3393, 3394, 3395, 3396, 3397, 3398, 3399,
  3406, 3407, 3410, 3411, 3412, 3413, 3413, 3414, 3417, 3418, 3421, 3422, 3423, 3424, 3459, 3459,
  3460, 3461, 3462, 3463, 3465, 3466, 3469, 3470, 3471, 3472, 3475, 3476, 3477, 3480, 3481, 3483,
  3484, 3486, 3487, 3490, 3491, 3494, 3495, 3496, 3500, 3499, 3513, 3514, 3517, 3517, 3519, 3519,
  3523, 3523, 3525, 3525, 3527, 3527, 3531, 3531, 3536, 3537, 3539, 3540, 3543, 3544, 3547, 3548,
  3551, 3552, 3553, 3554, 3555, 3556, 3557, 3558, 3558, 3558, 3558, 3558, 3559, 3560, 3561, 3562,
  3563, 3566, 3569, 3570, 3573, 3576, 3576, 3576 };
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char* const yytname[] = { "$end", "error", "$undefined", "ID", "VTK_ID", "QT_ID",
  "StdString", "UnicodeString", "OSTREAM", "ISTREAM", "LP", "LA", "STRING_LITERAL", "INT_LITERAL",
  "HEX_LITERAL", "BIN_LITERAL", "OCT_LITERAL", "FLOAT_LITERAL", "CHAR_LITERAL", "ZERO", "NULLPTR",
  "SSIZE_T", "SIZE_T", "NULLPTR_T", "BEGIN_ATTRIB", "STRUCT", "CLASS", "UNION", "ENUM", "PUBLIC",
  "PRIVATE", "PROTECTED", "CONST", "VOLATILE", "MUTABLE", "STATIC", "THREAD_LOCAL", "VIRTUAL",
  "EXPLICIT", "INLINE", "CONSTEXPR", "FRIEND", "EXTERN", "OPERATOR", "TEMPLATE", "THROW", "TRY",
  "CATCH", "NOEXCEPT", "DECLTYPE", "TYPENAME", "TYPEDEF", "NAMESPACE", "USING", "NEW", "DELETE",
  "DEFAULT", "STATIC_CAST", "DYNAMIC_CAST", "CONST_CAST", "REINTERPRET_CAST", "OP_LSHIFT_EQ",
  "OP_RSHIFT_EQ", "OP_LSHIFT", "OP_RSHIFT_A", "OP_DOT_POINTER", "OP_ARROW_POINTER", "OP_ARROW",
  "OP_INCR", "OP_DECR", "OP_PLUS_EQ", "OP_MINUS_EQ", "OP_TIMES_EQ", "OP_DIVIDE_EQ",
  "OP_REMAINDER_EQ", "OP_AND_EQ", "OP_OR_EQ", "OP_XOR_EQ", "OP_LOGIC_AND", "OP_LOGIC_OR",
  "OP_LOGIC_EQ", "OP_LOGIC_NEQ", "OP_LOGIC_LEQ", "OP_LOGIC_GEQ", "ELLIPSIS", "DOUBLE_COLON",
  "OTHER", "AUTO", "VOID", "BOOL", "FLOAT", "DOUBLE", "INT", "SHORT", "LONG", "INT64__", "CHAR",
  "CHAR16_T", "CHAR32_T", "WCHAR_T", "SIGNED", "UNSIGNED", "';'", "'{'", "'}'", "'='", "':'", "','",
  "'('", "')'", "'<'", "'&'", "'['", "']'", "'~'", "'*'", "'>'", "'%'", "'/'", "'-'", "'+'", "'!'",
  "'|'", "'^'", "'.'", "$accept", "translation_unit", "opt_declaration_seq", "$@1", "declaration",
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
  "$@12", "template_head", "$@13", "template_parameter_list", "$@14", "template_parameter", "$@15",
  "$@16", "$@17", "$@18", "$@19", "$@20", "opt_ellipsis", "class_or_typename",
  "opt_template_parameter_initializer", "template_parameter_initializer", "$@21",
  "template_parameter_value", "function_definition", "function_declaration",
  "nested_method_declaration", "nested_operator_declaration", "method_definition",
  "method_declaration", "operator_declaration", "conversion_function", "$@22", "$@23",
  "conversion_function_id", "operator_function_nr", "operator_function_sig", "$@24",
  "operator_function_id", "operator_sig", "function_nr", "function_trailer_clause",
  "func_cv_qualifier_seq", "func_cv_qualifier", "opt_noexcept_specifier", "noexcept_sig",
  "opt_ref_qualifier", "virt_specifier_seq", "virt_specifier", "opt_body_as_trailer",
  "opt_trailing_return_type", "trailing_return_type", "$@25", "function_body", "function_try_block",
  "handler_seq", "function_sig", "$@26", "structor_declaration", "$@27", "$@28", "structor_sig",
  "$@29", "opt_ctor_initializer", "mem_initializer_list", "mem_initializer",
  "parameter_declaration_clause", "$@30", "parameter_list", "$@31", "parameter_declaration", "$@32",
  "$@33", "opt_initializer", "initializer", "$@34", "$@35", "$@36", "constructor_args", "$@37",
  "variable_declaration", "init_declarator_id", "opt_declarator_list", "declarator_list_cont",
  "$@38", "init_declarator", "opt_ptr_operator_seq", "direct_abstract_declarator", "$@39",
  "direct_declarator", "$@40", "lp_or_la", "$@41", "opt_array_or_parameters", "$@42", "$@43",
  "function_qualifiers", "abstract_declarator", "declarator", "opt_declarator_id", "declarator_id",
  "bitfield_size", "opt_array_decorator_seq", "array_decorator_seq", "$@44",
  "array_decorator_seq_impl", "array_decorator", "$@45", "array_size_specifier", "$@46",
  "id_expression", "unqualified_id", "qualified_id", "nested_name_specifier", "$@47", "tilde_sig",
  "identifier_sig", "scope_operator_sig", "template_id", "$@48", "decltype_specifier", "$@49",
  "simple_id", "identifier", "opt_decl_specifier_seq", "decl_specifier2", "decl_specifier_seq",
  "decl_specifier", "storage_class_specifier", "function_specifier", "cv_qualifier",
  "cv_qualifier_seq", "store_type", "store_type_specifier", "$@50", "$@51", "type_specifier",
  "trailing_type_specifier", "$@52", "trailing_type_specifier_seq", "trailing_type_specifier_seq2",
  "$@53", "$@54", "tparam_type", "tparam_type_specifier2", "$@55", "$@56", "tparam_type_specifier",
  "simple_type_specifier", "type_name", "primitive_type", "ptr_operator_seq", "reference",
  "rvalue_reference", "pointer", "$@57", "ptr_cv_qualifier_seq", "pointer_seq",
  "decl_attribute_specifier_seq", "$@58", "id_attribute_specifier_seq", "$@59",
  "ref_attribute_specifier_seq", "$@60", "func_attribute_specifier_seq", "$@61",
  "array_attribute_specifier_seq", "$@62", "class_attribute_specifier_seq", "$@63",
  "attribute_specifier_seq", "attribute_specifier", "attribute_specifier_contents",
  "attribute_using_prefix", "attribute_list", "attribute", "$@64", "attribute_pack",
  "attribute_sig", "attribute_token", "operator_id", "operator_id_no_delim", "keyword", "literal",
  "constant_expression", "constant_expression_item", "$@65", "common_bracket_item",
  "common_bracket_item_no_scope_operator", "any_bracket_contents", "bracket_pitem",
  "any_bracket_item", "braces_item", "angle_bracket_contents", "braces_contents",
  "angle_bracket_pitem", "angle_bracket_item", "angle_brackets_sig", "$@66", "right_angle_bracket",
  "brackets_sig", "$@67", "$@68", "parentheses_sig", "$@69", "$@70", "$@71", "braces_sig", "$@72",
  "ignored_items", "ignored_expression", "ignored_item", "ignored_item_no_semi",
  "ignored_item_no_angle", "ignored_braces", "ignored_brackets", "ignored_parentheses",
  "ignored_left_parenthesis", YY_NULLPTR };
#endif

#define YYPACT_NINF -834
#define YYTABLE_NINF -627

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const short yypact[] = { -834, 99, 130, -834, -834, 1778, -834, 234, 266, 288, 386, 454, 466,
  493, 115, 128, 179, -834, -834, -834, 308, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  72, -834, 3515, -834, -834, 6245, 641, 1346, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, -834, -834, -834, -834, -834, -834, 49, -834, -834, -834, -834, -834, -834, 5948,
  -834, 97, 97, 97, 97, -834, 2, 6245, -834, 59, -834, 164, 1379, 1878, 188, 1684, 287, 350, -834,
  168, 6047, -834, -834, -834, -834, 147, 148, -834, -834, -834, -834, -834, 261, -834, -834, 204,
  3881, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, -834, -834, -834, -834, 8, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, 82, 1684, 22, 50,
  191, 203, 233, 250, 272, 334, -834, -834, -834, -834, -834, 5799, 188, 188, 6245, 147, -834, -834,
  -834, -834, -834, -834, -834, -834, 258, 22, 50, 191, 203, 233, 250, 272, -834, -834, -834, 1684,
  1684, 268, 283, -834, 1379, 1684, 188, 188, 6467, 327, 5603, -834, 6467, -834, 727, 333, 1684,
  -834, -834, -834, -834, -834, -834, 5948, -834, -834, 6146, 147, 342, -834, -834, -834, -834,
  -834, -834, -834, -834, -834, 6245, -834, -834, -834, -834, -834, -834, 257, 382, 188, 188, 188,
  -834, -834, -834, -834, 168, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, 1379, -834, -834, -834, -834, -834, -834, 5853, -834, 245, 87, -834, -834, -834, -834,
  -834, -834, -834, -834, 209, -834, -834, -834, 29, -834, -834, -834, 1929, 2051, -834, -834, 122,
  -834, 2173, 3027, 2295, -834, -834, -834, -834, -834, -834, 6551, 5883, 6551, 1637, -834, -834,
  -834, -834, -834, -834, 1741, -834, 2417, 1233, 403, -834, 410, -834, 427, -834, -834, -834, 5223,
  1379, -834, -834, 446, -834, 147, -834, -834, -834, -834, -834, -834, 126, -834, 1627, 584, 188,
  188, 261, 450, 1498, -834, -834, -834, 315, -834, 1684, 6146, 5853, 1684, 455, 2539, 457, 1448,
  1105, -834, -834, -834, 82, -834, -834, -834, -834, -834, 6467, 5883, 6467, 1637, -834, -834,
  -834, -834, 452, -834, 487, -834, 1486, -834, 487, 443, -834, 1379, 120, -834, -834, -834, 458,
  461, 1741, -834, 464, 147, -834, -834, -834, -834, -834, -834, 6025, 1459, 472, 281, 470, -834,
  1105, 475, 3149, -834, -834, 483, -834, -834, -834, -834, 177, -834, 6344, 195, 553, -834, -834,
  -834, -834, -834, -834, -834, -834, 498, -834, 147, 61, 506, 411, 6551, 6551, 142, 367, -834,
  -834, -834, -834, 507, 188, -834, -834, -834, 261, 603, 499, 500, 21, -834, -834, 512, -834, 503,
  -834, -834, -834, -834, -834, -834, 515, -834, -834, 310, 1250, -834, -834, 517, -834, -834, 188,
  188, 1627, -834, -834, -834, -834, -834, -834, -834, -834, 280, -834, 521, 6245, 525, -834, -834,
  1379, 522, -834, 79, -834, -834, 519, 548, -834, 188, -834, -834, -834, 457, 4613, 532, 137, 533,
  315, 6025, -834, 452, -834, -834, -834, -834, 40, -834, 528, -834, -834, -834, 526, 370, -834,
  -834, -834, -834, -834, 4857, -834, -834, 1428, -834, -834, 261, 452, 531, -834, 529, 470, 338,
  188, -834, 555, 82, -834, -834, -834, -834, -834, 1684, 1684, 1684, -834, 188, 188, 6245, 147,
  148, -834, -834, -834, -834, 147, 195, 4003, 4125, 4247, -834, 536, -834, -834, -834, 549, 550,
  -834, 148, -834, 547, -834, 556, 6245, -834, 541, 544, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, 562, -834, -834, -834, 589, 563, -834, 625, 587, -834, -834, -834, -834, 1498, -834,
  -834, -834, 274, 1684, 587, 587, 2661, -834, -834, 565, -834, -834, -834, 673, 261, 571, -834,
  -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, -834, -834, -834, -834, -834, -834, 575, -834, -834, -834, 257, -834, -834, 526, -834,
  568, -834, 580, 148, -834, 4735, -834, 4857, -834, -834, -834, -834, 463, -834, 401, -834, -834,
  -834, -834, 1105, -834, -834, -834, 122, -834, -834, -834, -834, 1741, -834, -834, -834, -834,
  -834, 147, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834,
  457, -834, 147, -834, -834, 5344, -834, 1684, -834, -834, -834, 1684, -834, 1250, -834, -834,
  4857, -834, 581, -834, -834, -834, -834, -834, 487, 606, 6245, -834, -834, 342, -834, -834, -834,
  -834, -834, -834, 457, 578, -834, -834, -834, -834, -834, -834, 457, -834, 5101, -834, 3637, -834,
  -834, -834, -834, -834, -834, -834, -834, 422, -834, 588, 87, 6025, 588, -834, 583, 593, -834,
  110, 1459, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, -834, 5444, -834, 97,
  -834, -834, -834, 594, 382, 1379, 5543, 147, 587, 1250, 587, 563, 4857, 3759, -834, 650, -834,
  -834, -834, 147, -834, 4369, 4613, 4491, 631, 590, 591, 4857, 592, -834, -834, -834, -834, -834,
  4857, 457, 6025, -834, -834, -834, -834, -834, 596, 147, -834, 588, -834, -834, 5642, -834, -834,
  -834, -834, 5444, -834, -834, 382, 5741, -834, -834, -834, -834, 1379, 5853, -834, -834, -834, 98,
  -834, -834, 598, 595, -834, -834, -834, -834, -834, -834, -834, 4857, -834, 4857, 600, 4979, -834,
  -834, -834, -834, -834, -834, -834, 1545, 97, 5741, 587, 5741, 605, -834, -834, 608, 245, 271,
  -834, -834, 6443, 65, -834, -834, -834, 4979, -834, 438, 490, 5405, -834, -834, 1545, -834, -834,
  5853, -834, 610, -834, -834, -834, -834, -834, 6443, -834, -834, 148, -834, 261, -834, -834, -834,
  -834, -834, 98, 139, -834, -834, 152, -834, 5405, -834, 5860, -834, 2783, -834, -834, -834, 490,
  -834, -834, 2905, 3271, 434, 71, 5860, 166, -834, -834, -834, 6025, -834, -834, -834, -834, 74,
  434, 6025, 3393, -834, -834 };

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short yydefact[] = { 3, 0, 4, 1, 471, 0, 483, 438, 439, 440, 434, 435, 436,
  437, 442, 443, 441, 52, 51, 53, 113, 397, 398, 389, 392, 393, 395, 396, 394, 388, 390, 215, 0,
  358, 411, 0, 0, 0, 355, 444, 445, 446, 447, 448, 453, 454, 455, 456, 449, 450, 451, 452, 457, 458,
  22, 353, 5, 19, 20, 13, 11, 12, 9, 36, 17, 377, 43, 481, 10, 16, 377, 0, 481, 14, 134, 7, 6, 8, 0,
  18, 0, 0, 0, 0, 204, 0, 0, 15, 0, 335, 471, 0, 0, 0, 0, 471, 410, 337, 354, 0, 471, 385, 386, 387,
  176, 290, 402, 406, 409, 471, 471, 472, 115, 114, 391, 0, 438, 439, 440, 434, 435, 436, 437, 672,
  673, 583, 578, 579, 580, 577, 581, 582, 584, 585, 442, 443, 441, 642, 550, 549, 551, 570, 553,
  555, 554, 556, 557, 558, 559, 562, 563, 561, 560, 566, 569, 552, 571, 572, 564, 548, 547, 568,
  567, 523, 524, 565, 575, 574, 573, 576, 525, 526, 527, 656, 528, 529, 530, 536, 537, 531, 532,
  533, 534, 535, 538, 539, 540, 541, 542, 543, 544, 545, 546, 654, 653, 666, 642, 660, 657, 661,
  671, 162, 520, 642, 519, 514, 659, 513, 515, 516, 517, 518, 521, 522, 658, 665, 664, 655, 662,
  663, 644, 650, 652, 651, 642, 0, 0, 438, 439, 440, 434, 435, 436, 437, 390, 377, 481, 377, 481,
  471, 0, 471, 410, 0, 176, 370, 372, 371, 375, 376, 374, 373, 642, 33, 362, 360, 361, 365, 366,
  364, 363, 369, 368, 367, 0, 0, 0, 473, 336, 0, 0, 338, 339, 290, 0, 50, 483, 290, 109, 116, 0, 0,
  26, 37, 23, 481, 25, 27, 0, 24, 28, 0, 176, 254, 243, 642, 186, 242, 188, 189, 187, 207, 481, 0,
  210, 21, 414, 351, 194, 192, 222, 342, 0, 338, 339, 340, 58, 341, 57, 0, 345, 343, 344, 346, 413,
  347, 356, 377, 481, 377, 481, 135, 205, 0, 471, 404, 383, 298, 300, 177, 0, 286, 271, 176, 475,
  475, 475, 401, 291, 459, 460, 469, 461, 377, 433, 432, 493, 484, 3, 644, 0, 0, 629, 628, 167, 161,
  0, 0, 0, 636, 638, 634, 359, 471, 391, 290, 50, 290, 116, 342, 377, 377, 150, 146, 142, 0, 145, 0,
  0, 0, 153, 0, 151, 0, 483, 155, 154, 0, 0, 382, 381, 0, 286, 176, 471, 379, 380, 61, 39, 48, 407,
  471, 0, 0, 58, 0, 482, 0, 121, 105, 117, 112, 471, 473, 0, 0, 0, 0, 0, 0, 261, 0, 0, 226, 225,
  477, 224, 252, 348, 349, 350, 617, 290, 50, 290, 116, 195, 193, 384, 377, 467, 206, 218, 473, 0,
  190, 218, 324, 473, 0, 0, 273, 283, 272, 0, 0, 0, 314, 0, 176, 464, 483, 463, 465, 462, 470, 403,
  0, 0, 493, 487, 490, 0, 4, 0, 647, 649, 0, 643, 646, 648, 667, 0, 164, 0, 0, 0, 668, 30, 645, 670,
  606, 606, 606, 412, 0, 142, 176, 407, 0, 471, 290, 290, 0, 324, 473, 338, 339, 32, 0, 0, 3, 158,
  159, 474, 0, 523, 524, 0, 508, 507, 0, 505, 0, 506, 214, 512, 157, 156, 41, 285, 289, 378, 62, 0,
  60, 38, 47, 56, 471, 58, 0, 0, 107, 362, 360, 361, 365, 366, 364, 363, 0, 119, 122, 0, 111, 408,
  471, 0, 255, 256, 0, 642, 241, 0, 471, 407, 0, 230, 483, 223, 261, 0, 0, 407, 0, 471, 405, 399,
  468, 299, 220, 221, 211, 227, 219, 0, 216, 295, 325, 0, 318, 196, 191, 473, 282, 287, 0, 640, 276,
  0, 296, 315, 476, 467, 0, 152, 0, 486, 493, 499, 354, 495, 497, 31, 29, 669, 165, 163, 0, 0, 0,
  428, 427, 426, 0, 176, 290, 421, 425, 178, 179, 176, 0, 0, 0, 0, 137, 141, 144, 139, 111, 0, 0,
  136, 290, 147, 318, 35, 4, 0, 511, 0, 0, 510, 509, 501, 502, 65, 66, 67, 44, 471, 0, 101, 102,
  103, 99, 49, 92, 97, 176, 45, 54, 471, 110, 121, 123, 118, 104, 337, 0, 176, 176, 0, 208, 267,
  262, 263, 268, 352, 249, 478, 0, 632, 595, 624, 600, 625, 626, 630, 601, 605, 604, 599, 602, 603,
  622, 594, 623, 618, 621, 357, 596, 597, 598, 42, 40, 108, 111, 400, 229, 228, 222, 212, 330, 327,
  328, 0, 247, 0, 290, 593, 590, 591, 274, 586, 588, 589, 619, 0, 279, 301, 466, 488, 485, 492, 0,
  496, 494, 498, 167, 429, 430, 431, 423, 316, 168, 475, 420, 377, 171, 176, 611, 613, 614, 637,
  609, 610, 608, 612, 607, 639, 635, 138, 140, 143, 261, 34, 176, 503, 504, 0, 64, 0, 100, 471, 98,
  0, 94, 0, 55, 120, 0, 644, 0, 127, 257, 259, 258, 245, 218, 264, 0, 232, 231, 254, 253, 606, 617,
  606, 106, 477, 261, 333, 329, 321, 322, 323, 320, 319, 261, 288, 0, 587, 0, 280, 278, 302, 297,
  305, 500, 166, 377, 301, 317, 180, 176, 422, 180, 174, 0, 0, 471, 390, 0, 81, 79, 70, 76, 63, 78,
  72, 71, 75, 73, 68, 69, 0, 77, 0, 201, 202, 74, 0, 335, 0, 0, 176, 176, 0, 176, 46, 124, 126, 125,
  244, 209, 266, 471, 176, 250, 0, 0, 0, 237, 0, 0, 0, 0, 592, 616, 641, 615, 620, 0, 261, 424, 292,
  182, 169, 181, 312, 0, 176, 172, 180, 148, 160, 0, 82, 84, 87, 85, 0, 83, 86, 0, 0, 197, 80, 473,
  203, 0, 0, 95, 93, 96, 0, 265, 269, 233, 0, 627, 631, 239, 230, 238, 213, 479, 334, 248, 281, 0,
  0, 293, 313, 175, 306, 90, 481, 88, 0, 0, 0, 176, 0, 0, 473, 200, 0, 271, 0, 251, 633, 0, 233,
  331, 483, 303, 183, 184, 301, 149, 0, 481, 89, 0, 91, 481, 0, 198, 0, 642, 270, 236, 234, 235, 0,
  416, 240, 290, 217, 480, 306, 185, 294, 308, 307, 0, 311, 642, 644, 407, 130, 0, 481, 0, 199, 0,
  418, 377, 415, 477, 309, 310, 0, 0, 0, 59, 0, 407, 131, 246, 377, 417, 304, 644, 133, 128, 59, 0,
  419, 0, 129, 132 };

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] = { -834, -834, -307, -834, -834, 708, -78, -834, -834, -834, -834,
  -722, -74, -3, -34, -834, -834, -834, -834, 78, -328, -83, -779, -834, -834, -834, -834, -76, -75,
  -82, -160, -834, -834, 42, -71, -67, -19, -834, -834, 32, -366, -834, -834, 43, -834, -834, -834,
  -236, -605, -64, -96, -319, 231, 84, -834, -834, -834, -834, 239, -45, 277, -834, 6, -834, 14,
  -834, -834, -834, -10, -834, -834, -834, -834, -834, -834, 611, 112, -771, -834, -834, -834, 752,
  -834, -834, -834, -36, -157, 23, -66, -834, -834, -207, -388, -834, -834, -259, -252, -454, -430,
  -834, -834, 30, -834, -834, -182, -834, -211, -834, -834, -834, -68, -834, -834, -834, -834, -42,
  -834, -834, -834, -834, -50, -834, 77, -531, -834, -834, -834, -120, -834, -834, -201, -834, -834,
  -834, -834, -834, -834, 7, 368, -232, 371, -834, 31, -30, -610, -834, -190, -834, -576, -834,
  -787, -834, -834, -229, -834, -834, -834, -367, -834, -834, -346, -834, -834, 41, -834, -834,
  -834, 982, 751, 1051, 24, -834, -834, 761, 90, -5, -834, 34, -834, 111, 83, 25, -834, 1, 790,
  -834, -834, -395, -834, 18, 214, -834, -834, 106, -794, -834, -834, -834, -834, -834, -834, -834,
  -834, -834, -834, 286, 392, 298, -336, 431, -834, 432, -834, 171, -834, 504, -834, -415, -834,
  -324, -834, -775, -834, -834, -834, -20, -834, -256, -834, -834, -834, 307, 169, -834, -834, -834,
  -834, -834, 73, 131, 96, -697, -699, -834, -349, -41, -462, -834, -44, -834, -23, -834, -833,
  -834, -538, -834, -465, -834, -834, -834, -194, -834, -834, -834, 337, -834, -161, -351, -834,
  -343, 44, -512, -834, -537, -834 };

/* YYDEFGOTO[NTERM-NUM].  */
static const short yydefgoto[] = { -1, 1, 2, 4, 56, 282, 58, 59, 60, 389, 61, 62, 63, 284, 65, 274,
  66, 800, 544, 302, 410, 411, 547, 543, 672, 673, 860, 921, 922, 678, 679, 798, 794, 680, 68, 69,
  70, 418, 71, 285, 421, 564, 561, 562, 803, 286, 805, 961, 1014, 73, 74, 505, 513, 506, 382, 383,
  787, 958, 384, 75, 266, 76, 287, 660, 288, 364, 492, 761, 493, 494, 846, 495, 849, 496, 916, 766,
  642, 910, 911, 954, 980, 289, 80, 81, 82, 925, 870, 871, 84, 430, 811, 85, 451, 452, 823, 453, 86,
  455, 593, 594, 595, 435, 436, 733, 701, 815, 973, 946, 947, 975, 296, 297, 886, 456, 831, 872,
  816, 941, 310, 581, 428, 569, 570, 574, 575, 697, 889, 698, 813, 971, 462, 463, 607, 464, 465,
  750, 905, 290, 341, 401, 460, 741, 402, 403, 767, 982, 342, 752, 343, 450, 839, 906, 1004, 983,
  913, 468, 844, 457, 830, 598, 840, 600, 736, 737, 824, 897, 898, 681, 89, 239, 240, 432, 92, 93,
  94, 271, 441, 272, 225, 97, 98, 273, 404, 303, 100, 101, 102, 103, 589, 878, 105, 353, 449, 106,
  107, 226, 1000, 1001, 1021, 1034, 636, 637, 770, 843, 638, 108, 109, 110, 348, 349, 350, 351, 614,
  590, 352, 566, 6, 393, 394, 470, 471, 578, 579, 977, 978, 275, 276, 111, 357, 478, 479, 480, 481,
  482, 759, 622, 623, 535, 714, 715, 716, 745, 746, 833, 747, 718, 644, 780, 781, 904, 582, 835,
  719, 720, 748, 819, 365, 723, 820, 818, 724, 503, 501, 502, 725, 749, 360, 367, 488, 489, 490,
  221, 222, 223, 224 };

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const short yytable[] = { 95, 235, 64, 331, 283, 603, 99, 469, 484, 315, 396, 77, 87, 298,
  299, 300, 237, 397, 514, 78, 416, 472, 473, 104, 499, 308, 597, 628, 83, 91, 241, 372, 771, 332,
  693, 361, 243, 72, 596, 96, 645, 646, 366, 601, 721, 417, 834, 895, 508, 309, 703, 483, 279, 244,
  386, 588, 565, 908, 509, 692, 768, 269, 395, 368, 545, 327, 768, 238, 814, 242, 858, 602, 362,
  241, 545, 347, 220, 545, 915, 291, 329, 241, 477, 67, 114, 664, 313, 316, 454, 123, 124, 387, 369,
  370, 241, 277, 292, 337, 338, 3, 657, 83, 91, 576, 304, 217, 883, -370, 123, 124, 301, 599, 242,
  236, 603, 584, 115, 722, 731, 253, 242, 981, 374, 333, 363, 314, 317, 270, 219, 545, -2, 330,
  -370, 242, 429, -372, -489, 665, 524, 538, 545, 499, -489, 293, 651, 957, 397, 1005, -170, 123,
  124, 732, 278, 467, 115, 545, 67, 337, 338, 359, -372, 305, 850, 218, -59, 344, -173, -59, 599,
  545, 972, 339, -170, 546, 454, 319, 727, 328, 806, 852, 602, 999, 196, 320, 834, 323, 325, 200,
  217, 740, 371, 537, 652, 653, 730, 1006, 768, 959, 345, 294, 295, 950, 346, 318, 1020, 335, 200,
  454, 952, 380, 583, 219, 585, 768, 659, 613, 376, -369, 378, 588, 729, 640, 605, -369, 381, 332,
  344, 606, 546, -59, -368, 339, -59, 717, 1028, 313, -368, 912, 241, 727, -59, 362, 580, -59, 654,
  641, 218, 200, 1036, 655, 1028, 834, 1041, 834, 546, -59, 851, 327, -59, 345, 375, 1041, 377, 346,
  313, 424, -336, 446, 727, -59, 414, 768, -59, 38, 314, 238, -371, 242, 326, 612, 940, -367, 431,
  241, 627, 356, 241, -367, -375, 425, 995, 447, 896, 363, 398, 779, 779, 779, 241, 412, 899, -371,
  433, 314, 956, 434, 292, 358, 443, 415, 445, 83, 91, -375, 768, 333, 454, 386, -376, -370, 345,
  236, 242, 702, 346, 242, 996, 997, 313, 549, 216, 323, 325, 112, 113, -374, -362, 242, 768, 668,
  669, 670, -362, -376, -370, 507, 374, 507, -275, 335, 461, -372, 442, -277, 444, 721, 892, -373,
  894, 332, -374, 323, 325, 388, 459, 413, 236, 314, -360, 328, 392, 414, 38, -371, -360, 953, -372,
  196, 476, 804, 516, 887, -373, 520, 685, -152, 318, 686, -489, -338, -361, 327, 241, 313, -489,
  -338, -361, 335, -371, 845, 412, 970, 438, 439, 440, 511, 512, 550, 520, 335, 415, 694, 1032, 216,
  671, 304, 721, -116, -116, 517, 241, 420, 415, 1039, 215, 1032, 414, 577, 943, 760, 407, 242, 314,
  217, 217, 38, 1044, 552, 414, 217, 217, 217, 721, 201, 847, -491, 551, 415, 427, 333, 568, -491,
  -339, 884, 313, 412, 219, 219, -339, 242, 217, 516, 219, 219, 219, 415, 413, 412, 319, 684, 536,
  -375, 1024, 1025, 587, 656, 738, 415, 739, -326, 969, 507, 507, 219, 21, 22, 779, 318, -365, 633,
  437, 218, 218, 314, -365, 635, -375, 218, 218, 218, 517, 269, 217, 236, 323, 325, 521, 215, 5,
  838, 643, 914, 522, -326, -116, 319, 967, 420, 218, 591, 592, 413, 1007, 318, 1008, 219, 631, 536,
  634, 563, 838, 335, 523, 413, -326, 1009, 969, 1011, 1010, -376, 1012, 499, 318, 779, 717, 779,
  838, 550, 539, 319, -326, -374, 553, 992, 318, -326, -366, 217, 572, 241, 218, 608, -366, 313,
  -376, 621, -260, 270, -364, 610, 836, 406, 837, 611, -364, 406, -374, 619, -373, 624, 219, 826,
  827, 828, 829, 617, 551, 245, 246, 247, 248, 249, 250, 251, 306, -363, 626, 280, 242, 324, 647,
  -363, 314, -373, 336, 717, 661, 769, 650, 658, 323, 662, 663, 354, 355, 218, 667, 931, 674, 675,
  676, 666, 606, 682, 454, 507, 687, 689, 695, 691, 241, 717, 696, 33, 726, 728, 734, 754, 735, 758,
  439, 440, 755, 655, 245, 246, 247, 248, 249, 250, 251, 784, 785, 738, 790, 241, 1013, 791, 216,
  216, 788, 1027, 797, 318, 216, 216, 216, 700, 793, 242, 796, 339, 812, 406, 968, 406, 814, 821,
  789, 690, 817, 454, 454, 885, 499, 216, 1043, 1029, 739, 1031, 888, -332, 917, 909, 242, 918, 930,
  938, 945, 948, 499, 951, 1040, 972, 949, 955, 751, 991, 974, 979, 757, 993, 1018, 57, 857, 340,
  861, 862, 882, 795, 936, 863, 1003, 323, 325, 864, 986, 216, 865, 802, 254, 255, 256, 257, 258,
  259, 260, 648, 454, 786, 406, 765, 406, 306, 252, 324, 649, 866, 261, 262, 263, 842, 215, 215,
  615, 772, 869, 79, 215, 215, 215, 516, 963, 822, 976, 1002, 891, 217, 807, 939, 994, 541, 832,
  540, 406, 1023, 33, 825, 688, 215, 639, 331, 216, 474, 475, 753, 618, 95, 756, 859, 219, 903, 900,
  877, 283, 848, 893, 563, 867, 873, 517, 609, 929, 0, 0, 0, 868, 0, 241, 406, 406, 332, 38, 0, 0,
  0, 876, 0, 0, 0, 0, 215, 0, 0, 72, 218, 96, 0, 0, 419, 0, 890, 1019, 420, 0, 932, 0, 0, 448, 0,
  621, 55, 311, 327, 0, 321, 0, 242, 0, 319, 1026, 0, 312, 0, 0, 385, 0, 0, 329, 0, 0, 0, 332, 95,
  0, 920, 0, 0, 907, 928, 67, 313, 95, 0, 923, 926, 215, 504, 0, 269, 713, 0, 924, 0, 0, 406, 932,
  0, 283, 334, 0, 0, 876, 0, 327, 988, 0, 0, 332, 0, 332, 933, 96, 426, 0, 713, 0, 0, 542, 330, 314,
  96, 0, 0, 95, 548, 0, 0, 0, 95, 964, 920, 932, 95, 932, 966, 0, 0, 313, 516, 923, 926, 327, 962,
  0, 0, 0, 965, 924, 0, 984, 0, 713, 713, 713, 270, 67, 0, 876, 0, 0, 0, 933, 96, 466, 328, 0, 217,
  96, 95, 0, 95, 96, 987, 0, 1015, 0, 314, 517, 1017, 241, 1022, 0, 0, 712, 0, 998, 0, 0, 414, 219,
  0, 0, 335, 0, 516, 0, 88, 0, 0, 933, 379, 216, 241, 0, 0, 1030, 960, 96, 712, 96, 312, 236, 0, 0,
  0, 328, 0, 412, 242, 414, 0, 414, 0, 466, 218, 0, 319, 415, 267, 379, 321, 0, 0, 517, 414, 0, 0,
  0, 0, 312, 0, 242, 334, 335, 0, 712, 712, 712, 412, 985, 412, 989, 713, 236, 0, 1035, 0, 0, 415,
  0, 415, 0, 683, 412, 0, 90, 0, 0, 1042, 0, 0, 0, 405, 415, 1016, 0, 405, 0, 0, 335, 0, 335, 319,
  413, 0, 0, 0, 0, 699, 466, 334, 0, 0, 311, 0, 215, 0, 268, 0, 0, 458, 217, 334, 312, 318, 0, 0, 0,
  217, 217, 713, 0, 0, 0, 413, 0, 413, 245, 246, 247, 248, 249, 250, 251, 219, 217, 0, 385, 0, 413,
  0, 219, 219, 0, 0, 318, 0, 318, 0, 0, 713, 0, 713, 0, 0, 515, 712, 0, 219, 0, 318, 0, 0, 0, 322,
  406, 0, 0, 379, 218, 0, 0, 0, 0, 0, 0, 218, 218, 312, 0, 0, 0, 0, 0, 405, 0, 405, 0, 0, 0, 0, 0,
  312, 218, 0, 792, 458, 0, 0, 0, 713, 216, 0, 0, 0, 0, 0, 801, 0, 713, 713, 713, 0, 0, 712, 713, 0,
  0, 0, 0, 0, 0, 713, 0, 406, 0, 0, 373, 0, 604, 0, 0, 0, 0, 334, 0, 515, 0, 0, 312, 0, 0, 0, 0,
  712, 0, 712, 0, 0, 0, 0, 405, 0, 405, 0, 245, 246, 247, 248, 249, 250, 251, 620, 0, 0, 390, 391,
  713, 0, 713, 0, 713, 254, 255, 256, 257, 258, 259, 260, 0, 422, 0, 423, 0, 0, 405, 0, 0, 0, 0,
  261, 262, 263, 0, 712, 215, 268, 713, 674, 675, 676, 33, 0, 712, 712, 712, 677, 0, 0, 712, 0, 799,
  0, 0, 0, 0, 712, 0, 33, 880, 405, 405, 808, 809, 0, 0, 0, 0, 0, 0, 0, 0, 312, 0, 268, 268, 216,
  38, 604, 0, 322, 0, 0, 216, 216, 0, 0, 0, 312, 268, 0, 268, 406, 0, 38, 0, 0, 0, 0, 406, 216, 712,
  0, 712, 0, 712, 0, 0, 254, 255, 256, 257, 258, 259, 260, 0, 919, 0, 0, 510, 0, 0, 0, 55, 0, 0,
  261, 262, 263, 0, 0, 712, 0, 0, 0, 0, 405, 0, 0, 0, 0, 254, 255, 256, 257, 258, 259, 260, 0, 0, 0,
  0, 699, 0, 33, 264, 0, 265, 0, 261, 262, 263, 0, 0, 0, 567, 0, 0, 571, 0, 215, 0, 0, 0, 0, 0, 0,
  215, 215, 0, 0, 31, 307, 0, 334, 0, 586, 33, 268, 0, 38, 0, 0, 0, 215, 0, 0, 0, 519, 125, 126,
  127, 128, 129, 130, 131, 132, 133, 0, 0, 254, 255, 256, 257, 258, 259, 260, 0, 616, 55, 0, 254,
  255, 256, 257, 258, 259, 260, 261, 262, 263, 0, 0, 0, 268, 0, 0, 268, 0, 261, 262, 263, 0, 0, 0,
  0, 0, 0, 934, 935, 0, 937, 55, 0, 0, 268, 33, 525, 0, 0, 554, 555, 556, 557, 558, 559, 560, 33,
  264, 0, 0, 0, 0, 0, 0, 0, 515, 841, 261, 262, 263, 0, 0, 0, 0, 0, 0, 268, 0, 0, 0, 0, 38, 0, 0, 0,
  0, 0, 0, 526, 527, 0, 875, 38, 632, 0, 170, 171, 172, 528, 174, 175, 176, 177, 178, 179, 180, 181,
  182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 17, 18, 19, 55, 0, 0, 990, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 0, 234, 0, 0, 0, 529, 0, 530, 531, 268, 532, 202, 533, 0, 204, 205, 534, 207,
  208, 209, 210, 211, 212, 213, 0, 762, 763, 764, 0, 0, 0, 0, 0, 927, 0, 0, 0, 0, 0, 0, 0, 379, 927,
  0, 254, 255, 256, 257, 258, 259, 260, 312, 405, 0, 254, 255, 256, 257, 258, 259, 260, 0, 261, 262,
  263, 0, 0, 0, 0, 0, 0, 0, 261, 262, 263, 0, 0, 0, 0, 0, 0, 334, 0, 0, 927, 307, 0, 571, 0, 927,
  33, 0, 0, 927, 268, 268, 268, 0, 379, 458, 33, 254, 255, 256, 257, 258, 259, 260, 312, 0, 0, 405,
  0, 0, 0, 0, 0, 0, 0, 261, 262, 263, 0, 0, 0, 0, 0, 0, 0, 927, 0, 927, 334, 0, 0, 0, 38, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 33, 0, 0, 0, 0, 0, 0, 0, 604, 268, 420, 254, 255, 256, 257, 258, 259, 260,
  55, 0, 0, 334, 0, 334, 0, 0, 0, 0, 0, 261, 262, 263, 0, 0, 0, 0, 38, 0, 0, 0, 0, 874, 0, 879, 334,
  0, 0, 881, 7, 8, 9, 10, 11, 12, 13, 334, 0, 33, 0, 0, 0, 0, 0, 0, 0, 55, 14, 15, 16, 0, 17, 18,
  19, 20, 0, 0, 0, 21, 22, 23, 24, 25, 26, 27, 28, 29, 0, 30, 31, 32, 0, 0, 405, 0, 33, 34, 35, 36,
  37, 405, 0, 0, 0, 0, 267, 0, 0, 0, 0, 0, 90, 0, 268, 0, 0, 0, 268, 0, 268, 0, 0, 0, 55, 0, 0, 0,
  0, 0, 0, 0, 38, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 254, 255, 256,
  257, 258, 259, 260, 0, 0, 0, 0, 55, 0, 0, 0, 0, 0, 0, 261, 262, 263, 0, 0, 0, 0, 268, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33, 0, 0, 0, 268, 116, 117, 118, 119, 120, 121,
  122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
  141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152, 0, 153, 154, 155, 156, 0, 0, 157,
  158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
  177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 39,
  40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 485, 196, 0, 197, 198, 199, 200, 0, 486,
  202, 203, 487, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 116, 117, 118, 119, 120,
  121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
  140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152, 0, 153, 154, 155, 156, 0, 0,
  157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
  176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
  195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 485, 196, 491, 197, 198, 199,
  200, 0, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 116, 117, 118,
  119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137,
  138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152, 0, 153, 154, 155,
  156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173,
  174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192,
  193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 485, 196, 0, 197, 198,
  199, 200, 0, 486, 202, 203, 497, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 116, 117,
  118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
  137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152, 0, 153, 154,
  155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172,
  173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
  192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 485, 196, 0, 197,
  198, 199, 200, 500, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 116,
  117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
  136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152, 0, 153,
  154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171,
  172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
  191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 485, 196,
  518, 197, 198, 199, 200, 0, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  214, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152,
  0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
  190, 191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 485,
  196, 573, 197, 198, 199, 200, 0, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209, 210, 211, 212,
  213, 214, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
  133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151,
  152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
  485, 196, 810, 197, 198, 199, 200, 0, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209, 210, 211,
  212, 213, 214, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150,
  151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
  53, 485, 196, 1033, 197, 198, 199, 200, 0, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209, 210,
  211, 212, 213, 214, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
  131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149,
  150, 151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
  168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186,
  187, 188, 189, 190, 191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
  52, 53, 485, 196, 1037, 197, 198, 199, 200, 0, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209,
  210, 211, 212, 213, 214, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
  130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148,
  149, 150, 151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166,
  167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185,
  186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
  51, 52, 53, 498, 196, 0, 197, 198, 199, 200, 0, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209,
  210, 211, 212, 213, 214, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
  130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148,
  149, 150, 151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166,
  167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185,
  186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
  51, 52, 53, 625, 196, 0, 197, 198, 199, 200, 0, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209,
  210, 211, 212, 213, 214, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
  130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148,
  149, 150, 151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166,
  167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185,
  186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
  51, 52, 53, 1038, 196, 0, 197, 198, 199, 200, 0, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209,
  210, 211, 212, 213, 214, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
  130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148,
  149, 150, 151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166,
  167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185,
  186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
  51, 52, 53, 1045, 196, 0, 197, 198, 199, 200, 0, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209,
  210, 211, 212, 213, 214, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
  130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148,
  149, 150, 151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166,
  167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185,
  186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
  51, 52, 53, 0, 196, 0, 197, 198, 199, 200, 0, 201, 202, 203, 0, 204, 205, 206, 207, 208, 209, 210,
  211, 212, 213, 214, 116, 117, 118, 119, 120, 121, 122, 369, 370, 125, 126, 127, 128, 129, 130,
  131, 132, 133, 134, 135, 136, 704, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149,
  150, 151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
  168, 169, 170, 171, 172, 773, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186,
  187, 188, 189, 190, 191, 192, 0, 705, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
  53, 901, 608, 902, 774, 707, 775, 371, 0, 777, 202, 710, 0, 204, 205, 778, 207, 208, 209, 210,
  211, 212, 213, 711, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
  131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149,
  150, 151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
  168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186,
  187, 188, 189, 190, 191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
  52, 53, 0, 196, 0, 197, 198, 199, 200, 0, 486, 202, 203, 0, 204, 205, 206, 207, 208, 209, 210,
  211, 212, 213, 214, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
  131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149,
  150, 151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
  168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186,
  187, 188, 189, 190, 191, 192, 193, 194, 195, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
  52, 53, 0, 196, 0, 197, 198, 199, 200, 0, 0, 202, 203, 0, 204, 205, 206, 207, 208, 209, 210, 211,
  212, 213, 214, 116, 117, 118, 119, 120, 121, 122, 369, 370, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 704, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150,
  151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 773, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 0, 705, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 0,
  608, 0, 774, 707, 775, 371, 776, 777, 202, 710, 0, 204, 205, 778, 207, 208, 209, 210, 211, 212,
  213, 711, 116, 117, 118, 119, 120, 121, 122, 369, 370, 125, 126, 127, 128, 129, 130, 131, 132,
  133, 134, 135, 136, 704, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151,
  152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 773, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 0, 705, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 0, 608,
  0, 774, 707, 775, 371, 782, 777, 202, 710, 0, 204, 205, 778, 207, 208, 209, 210, 211, 212, 213,
  711, 116, 117, 118, 119, 120, 121, 122, 369, 370, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 704, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152,
  0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 172, 773, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
  190, 191, 192, 0, 705, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 0, 608, 0,
  774, 707, 775, 371, 783, 777, 202, 710, 0, 204, 205, 778, 207, 208, 209, 210, 211, 212, 213, 711,
  116, 117, 118, 119, 120, 121, 122, 369, 370, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
  135, 136, 704, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152, 0,
  153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 172, 773, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
  190, 191, 192, 0, 705, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 0, 608, 0,
  774, 707, 775, 371, 0, 777, 202, 710, 942, 204, 205, 778, 207, 208, 209, 210, 211, 212, 213, 711,
  116, 117, 118, 119, 120, 121, 122, 369, 370, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
  135, 136, 704, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152, 0,
  153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 172, 773, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
  190, 191, 192, 0, 705, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 0, 608, 0,
  774, 707, 775, 371, 0, 777, 202, 710, 944, 204, 205, 778, 207, 208, 209, 210, 211, 212, 213, 711,
  116, 117, 118, 119, 120, 121, 122, 369, 370, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
  135, 136, 704, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152, 0,
  153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 172, 362, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
  190, 191, 192, 0, 705, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 0, 608, 0,
  706, 707, 708, 371, 0, 709, 202, 710, 0, 204, 205, 363, 207, 208, 209, 210, 211, 212, 213, 711,
  -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590,
  -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, 0, -590,
  -590, -590, -590, -590, -590, 0, -590, -590, -590, -590, 0, 0, -590, -590, -590, -590, -590, -590,
  -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590,
  -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, 0, -590, 0,
  -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, -590, 0, -590,
  0, -626, -590, -590, -590, 0, -590, -590, -590, 0, -590, -590, -590, -590, -590, -590, -590, -590,
  -590, -590, -590, 116, 117, 118, 119, 120, 121, 122, 369, 370, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 704, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150,
  151, 152, 0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 742, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 0, 705, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 0,
  608, 0, 0, 707, 0, 371, 0, 743, 202, 710, 0, 204, 205, 744, 207, 208, 209, 210, 211, 212, 213,
  711, 116, 117, 118, 119, 120, 121, 122, 369, 370, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 704, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152,
  0, 153, 154, 155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 172, 0, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
  191, 192, 0, 705, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 0, 608, 0, 0,
  707, 0, 371, 0, 709, 202, 710, 0, 204, 205, 0, 207, 208, 209, 210, 211, 212, 213, 711, 116, 117,
  118, 119, 120, 121, 122, 369, 370, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
  704, 138, 139, 140, 141, 142, 143, 144, 145, 146, 0, 147, 148, 149, 150, 151, 152, 0, 153, 154,
  155, 156, 0, 0, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 0,
  174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 0,
  0, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 0, 608, 0, 0, 707, 0, 371, 0, 0,
  202, 710, 0, 204, 205, 0, 207, 208, 209, 210, 211, 212, 213, 711, 227, 228, 229, 230, 231, 232,
  233, 0, 0, 525, 0, 0, 0, 0, 0, 0, 0, 0, 134, 135, 136, 0, 17, 18, 19, 20, 0, 0, 0, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 0, 234, 0, 0, 0, 0, 0, 0, 33, 34, 0, 0, 0, 526, 527, 0, 0, 0, 0, 0, 170, 171,
  172, 528, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
  191, 192, 0, 38, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 0, 0, 0, 529, 0,
  530, 531, 0, 532, 202, 533, 0, 204, 205, 534, 207, 208, 209, 210, 211, 212, 213, 7, 8, 9, 10, 11,
  12, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 15, 16, 0, 17, 18, 19, 20, 0, 0, 0, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 853, 854, 31, 32, 0, 0, 0, 0, 33, 34, 35, 0, 855, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 254, 255, 256, 257, 258, 259, 260, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 261, 262, 263, 38, 0, 39,
  40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 856, 7, 8, 9, 10, 11, 12, 13, 33, 0, 0, 0,
  55, 0, 0, 0, 0, 0, 0, 14, 15, 16, 0, 17, 18, 19, 20, 0, 0, 0, 21, 22, 23, 24, 25, 26, 27, 28, 29,
  853, 234, 31, 280, 0, 38, 0, 0, 33, 34, 0, 0, 281, 0, 0, 0, 0, 0, 0, 0, 0, 0, 408, 1011, 0, 0,
  1012, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 0, 39, 40, 41, 42, 43, 44, 45, 46,
  47, 48, 49, 50, 51, 52, 53, 7, 8, 9, 10, 11, 12, 13, 0, 0, 0, 0, 0, 55, 0, 0, 0, 0, 0, 14, 15, 16,
  0, 17, 18, 19, 20, 0, 0, 0, 21, 22, 23, 24, 25, 26, 27, 28, 29, 0, 234, 31, 0, 0, 0, 0, 0, 33, 34,
  35, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 254, 255, 256, 257, 258, 259, 260, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 261, 262, 263, 0, 38, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 7, 8,
  9, 10, 11, 12, 13, 33, 0, 0, 0, 0, 55, 0, 0, 0, 0, 0, 14, 15, 16, 0, 17, 18, 19, 20, 0, 0, 0, 21,
  22, 23, 24, 25, 26, 27, 28, 29, 0, 234, 31, 280, 0, 38, 0, 0, 33, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 408, 0, 0, 0, 409, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 0, 39, 40, 41,
  42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 7, 8, 9, 10, 11, 12, 13, 0, 0, 0, 0, 0, 55, 0, 0,
  0, 0, 0, 14, 15, 16, 0, 17, 18, 19, 20, 0, 0, 0, 21, 22, 23, 24, 25, 26, 27, 28, 29, 0, 234, 31,
  0, 0, 0, 0, 0, 33, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 254, 255, 256, 257, 258, 259, 260, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 261, 262, 263, 0, 0, 0, 38, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
  50, 51, 52, 53, 307, 0, 0, 0, 0, 33, 0, 0, 0, 0, 0, 0, 55, 254, 255, 256, 257, 258, 259, 260, 254,
  255, 256, 257, 258, 259, 260, 0, 0, 0, 0, 261, 262, 263, 0, 0, 0, 0, 261, 262, 263, 0, 0, 254,
  255, 256, 257, 258, 259, 260, 0, 0, 0, 31, 0, 0, 0, 0, 0, 33, 0, 261, 262, 263, 0, 0, 33, 0, 0, 0,
  55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  38, 0, 0, 0, 0, 0, 227, 228, 229, 230, 231, 232, 233, 0, 0, 0, 0, 0, 1011, 0, 0, 1012, 55, 38,
  134, 135, 136, 0, 17, 18, 19, 20, 0, 0, 0, 21, 22, 23, 24, 25, 26, 27, 28, 29, 409, 234, 31, 280,
  0, 0, 0, 0, 33, 34, 0, 0, 281, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 227,
  228, 229, 230, 231, 232, 233, 21, 22, 23, 24, 25, 26, 27, 28, 29, 399, 234, 134, 135, 136, 0, 17,
  18, 19, 20, 400, 0, 0, 21, 22, 23, 24, 25, 26, 27, 28, 29, 0, 234, 31, 0, 0, 0, 0, 0, 33, 34, 35,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
  0, 0, 0, 0, 0, 38, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 227, 228, 229,
  230, 231, 232, 233, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 134, 135, 136, 0, 17, 18, 19, 20, 0, 0, 0,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 0, 234, 31, 0, 0, 0, 0, 0, 33, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 0, 39, 40, 41, 42,
  43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 227, 228, 229, 230, 231, 232, 233, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 134, 135, 136, 0, 17, 18, 19, 20, 0, 0, 0, 21, 22, 23, 24, 25, 26, 27, 28, 29, 0,
  234, 0, 0, 0, 0, 0, 0, 33, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
  53, 227, 228, 229, 230, 231, 232, 233, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 134, 135, 136, 0, 629, 0,
  630, 20, 0, 0, 0, 21, 22, 23, 24, 25, 26, 27, 28, 29, 0, 234, 0, 0, 0, 0, 0, 0, 33, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 0,
  39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 227, 228, 229, 230, 231, 232, 233, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 134, 135, 136, 0, 0, 0, 0, 0, 0, 0, 0, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 0, 234, 0, 0, 0, 0, 0, 0, 33, 34, 0, 0, 0, 0, 0, 21, 22, 23, 24, 25, 26, 27, 28, 29, 399,
  234, 0, 0, 0, 0, 0, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 0, 39, 40, 41, 42, 43, 44, 45,
  46, 47, 48, 49, 50, 51, 52, 53, 344, 0, 0, 0, 0, 0, 0, 0, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, -284, 0, 0, 0, 0, 0, 0, 0, 0, 345, 0, 0, 0, 346, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 399, 234, 0, 0, 0, 0, 0, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 344, 0, 0, 0, 0, 0, 0, 0, 0, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
  49, 50, 51, 52, 53, 0, 0, 0, 0, 0, 0, 0, 0, 0, 345, 0, 0, 0, 346 };

static const short yycheck[] = { 5, 35, 5, 99, 78, 459, 5, 343, 359, 92, 269, 5, 5, 81, 82, 83, 35,
  269, 385, 5, 276, 345, 346, 5, 367, 91, 456, 492, 5, 5, 35, 225, 642, 99, 571, 196, 35, 5, 453, 5,
  502, 503, 203, 458, 582, 277, 745, 822, 376, 91, 581, 358, 72, 35, 244, 450, 422, 844, 377, 571,
  636, 37, 269, 224, 3, 99, 642, 35, 3, 35, 792, 459, 64, 78, 3, 105, 32, 3, 849, 78, 99, 86, 53, 5,
  12, 64, 91, 92, 340, 10, 11, 252, 10, 11, 99, 70, 78, 10, 11, 0, 515, 78, 78, 431, 86, 32, 803,
  85, 10, 11, 108, 457, 78, 35, 568, 443, 44, 582, 78, 36, 86, 954, 12, 99, 116, 91, 92, 37, 32, 3,
  0, 99, 110, 99, 295, 85, 107, 116, 394, 398, 3, 484, 113, 46, 510, 916, 398, 980, 26, 10, 11, 111,
  103, 343, 44, 3, 78, 10, 11, 115, 110, 102, 772, 32, 103, 78, 44, 106, 514, 3, 105, 84, 50, 102,
  426, 92, 102, 99, 690, 789, 568, 975, 103, 93, 883, 95, 96, 108, 115, 604, 108, 398, 511, 512,
  589, 982, 772, 919, 111, 102, 103, 898, 115, 92, 998, 99, 108, 459, 905, 243, 442, 115, 444, 789,
  521, 471, 236, 102, 238, 614, 586, 26, 102, 108, 243, 291, 78, 107, 102, 103, 102, 84, 106, 582,
  1013, 240, 108, 847, 243, 102, 103, 64, 436, 106, 102, 50, 115, 108, 1023, 107, 1029, 950, 1031,
  952, 102, 103, 787, 291, 106, 111, 235, 1040, 237, 115, 269, 285, 102, 333, 102, 103, 275, 847,
  106, 85, 240, 243, 85, 243, 110, 469, 890, 102, 302, 288, 107, 24, 291, 108, 85, 288, 19, 333,
  823, 116, 270, 644, 645, 646, 303, 275, 831, 110, 45, 269, 914, 48, 288, 103, 328, 275, 330, 288,
  288, 110, 890, 291, 568, 507, 85, 85, 111, 243, 288, 579, 115, 291, 55, 56, 333, 412, 32, 241,
  242, 25, 26, 85, 102, 303, 914, 29, 30, 31, 108, 110, 110, 375, 12, 377, 103, 243, 105, 85, 327,
  108, 329, 893, 818, 85, 820, 425, 110, 271, 272, 105, 340, 275, 288, 333, 102, 291, 102, 376, 85,
  85, 108, 906, 110, 103, 353, 105, 385, 811, 110, 388, 104, 102, 275, 107, 107, 102, 102, 425, 397,
  398, 113, 108, 108, 291, 110, 766, 376, 938, 312, 313, 314, 380, 381, 412, 413, 303, 376, 572,
  1017, 115, 104, 397, 954, 102, 103, 385, 425, 106, 388, 1028, 32, 1030, 431, 432, 893, 623, 103,
  397, 398, 360, 361, 85, 1041, 413, 443, 366, 367, 368, 980, 110, 768, 107, 412, 413, 106, 425,
  426, 113, 102, 804, 459, 431, 360, 361, 108, 425, 387, 466, 366, 367, 368, 431, 376, 443, 385,
  552, 397, 85, 1009, 1010, 449, 108, 106, 443, 108, 112, 934, 511, 512, 387, 32, 33, 835, 376, 102,
  494, 108, 360, 361, 459, 108, 494, 110, 366, 367, 368, 466, 477, 429, 425, 414, 415, 103, 115, 4,
  108, 496, 847, 102, 112, 103, 432, 931, 106, 387, 32, 33, 431, 32, 412, 34, 429, 494, 454, 494,
  418, 108, 425, 105, 443, 112, 45, 990, 103, 48, 85, 106, 884, 431, 892, 893, 894, 108, 552, 102,
  466, 112, 85, 102, 968, 443, 112, 102, 484, 103, 564, 429, 103, 108, 568, 110, 482, 109, 477, 102,
  108, 107, 273, 109, 109, 108, 277, 110, 107, 85, 104, 484, 13, 14, 15, 16, 113, 552, 3, 4, 5, 6,
  7, 8, 9, 90, 102, 113, 44, 564, 95, 102, 108, 568, 110, 100, 954, 3, 637, 102, 102, 520, 112, 112,
  109, 110, 484, 113, 876, 29, 30, 31, 109, 107, 106, 876, 655, 105, 102, 109, 107, 635, 980, 84,
  49, 102, 102, 108, 106, 112, 84, 550, 551, 113, 107, 3, 4, 5, 6, 7, 8, 9, 102, 102, 106, 113, 660,
  984, 113, 360, 361, 104, 1012, 37, 552, 366, 367, 368, 577, 106, 635, 107, 84, 107, 375, 933, 377,
  3, 102, 660, 568, 109, 933, 934, 102, 1027, 387, 1037, 1015, 108, 1017, 84, 113, 109, 105, 660,
  102, 102, 47, 67, 109, 1043, 109, 1030, 105, 113, 109, 610, 102, 113, 109, 620, 103, 102, 5, 792,
  104, 792, 792, 800, 677, 880, 792, 978, 633, 634, 792, 962, 429, 792, 686, 3, 4, 5, 6, 7, 8, 9,
  506, 990, 655, 442, 635, 444, 239, 103, 241, 507, 792, 21, 22, 23, 761, 360, 361, 477, 643, 792,
  5, 366, 367, 368, 766, 919, 733, 946, 976, 816, 694, 691, 889, 971, 403, 741, 402, 476, 1004, 49,
  736, 564, 387, 494, 877, 484, 352, 352, 614, 479, 792, 619, 792, 694, 835, 833, 792, 868, 770,
  819, 686, 792, 792, 766, 464, 870, -1, -1, -1, 792, -1, 813, 511, 512, 877, 85, -1, -1, -1, 792,
  -1, -1, -1, -1, 429, -1, -1, 792, 694, 792, -1, -1, 102, -1, 813, 993, 106, -1, 877, -1, -1, 334,
  -1, 757, 114, 91, 877, -1, 94, -1, 813, -1, 766, 1011, -1, 91, -1, -1, 244, -1, -1, 877, -1, -1,
  -1, 928, 868, -1, 868, -1, -1, 843, 868, 792, 876, 877, -1, 868, 868, 484, 373, -1, 855, 582, -1,
  868, -1, -1, 587, 928, -1, 962, 99, -1, -1, 868, -1, 928, 963, -1, -1, 964, -1, 966, 877, 868,
  292, -1, 607, -1, -1, 404, 877, 876, 877, -1, -1, 919, 411, -1, -1, -1, 924, 919, 924, 964, 928,
  966, 924, -1, -1, 933, 934, 924, 924, 966, 919, -1, -1, -1, 919, 924, -1, 960, -1, 644, 645, 646,
  855, 868, -1, 924, -1, -1, -1, 928, 919, 343, 877, -1, 884, 924, 964, -1, 966, 928, 962, -1, 985,
  -1, 933, 934, 989, 975, 1001, -1, -1, 582, -1, 975, -1, -1, 984, 884, -1, -1, 877, -1, 990, -1, 5,
  -1, -1, 966, 240, 694, 998, -1, -1, 1016, 919, 964, 607, 966, 240, 924, -1, -1, -1, 928, -1, 984,
  975, 1015, -1, 1017, -1, 403, 884, -1, 934, 984, 37, 269, 270, -1, -1, 990, 1030, -1, -1, -1, -1,
  269, -1, 998, 243, 928, -1, 644, 645, 646, 1015, 962, 1017, 964, 745, 966, -1, 1021, -1, -1, 1015,
  -1, 1017, -1, 549, 1030, -1, 5, -1, -1, 1034, -1, -1, -1, 273, 1030, 987, -1, 277, -1, -1, 964,
  -1, 966, 990, 984, -1, -1, -1, -1, 575, 469, 291, -1, -1, 333, -1, 694, -1, 37, -1, -1, 340, 1019,
  303, 333, 984, -1, -1, -1, 1026, 1027, 803, -1, -1, -1, 1015, -1, 1017, 3, 4, 5, 6, 7, 8, 9, 1019,
  1043, -1, 507, -1, 1030, -1, 1026, 1027, -1, -1, 1015, -1, 1017, -1, -1, 833, -1, 835, -1, -1,
  385, 745, -1, 1043, -1, 1030, -1, -1, -1, 94, 848, -1, -1, 398, 1019, -1, -1, -1, -1, -1, -1,
  1026, 1027, 398, -1, -1, -1, -1, -1, 375, -1, 377, -1, -1, -1, -1, -1, 412, 1043, -1, 672, 426,
  -1, -1, -1, 883, 884, -1, -1, -1, -1, -1, 684, -1, 892, 893, 894, -1, -1, 803, 898, -1, -1, -1,
  -1, -1, -1, 905, -1, 907, -1, -1, 226, -1, 459, -1, -1, -1, -1, 425, -1, 466, -1, -1, 459, -1, -1,
  -1, -1, 833, -1, 835, -1, -1, -1, -1, 442, -1, 444, -1, 3, 4, 5, 6, 7, 8, 9, 482, -1, -1, 264,
  265, 950, -1, 952, -1, 954, 3, 4, 5, 6, 7, 8, 9, -1, 279, -1, 281, -1, -1, 476, -1, -1, -1, -1,
  21, 22, 23, -1, 883, 884, 226, 980, 29, 30, 31, 49, -1, 892, 893, 894, 37, -1, -1, 898, -1, 681,
  -1, -1, -1, -1, 905, -1, 49, 796, 511, 512, 692, 693, -1, -1, -1, -1, -1, -1, -1, -1, 552, -1,
  264, 265, 1019, 85, 568, -1, 270, -1, -1, 1026, 1027, -1, -1, -1, 568, 279, -1, 281, 1035, -1, 85,
  -1, -1, -1, -1, 1042, 1043, 950, -1, 952, -1, 954, -1, -1, 3, 4, 5, 6, 7, 8, 9, -1, 853, -1, -1,
  378, -1, -1, -1, 114, -1, -1, 21, 22, 23, -1, -1, 980, -1, -1, -1, -1, 587, -1, -1, -1, -1, 3, 4,
  5, 6, 7, 8, 9, -1, -1, -1, -1, 889, -1, 49, 50, -1, 52, -1, 21, 22, 23, -1, -1, -1, 424, -1, -1,
  427, -1, 1019, -1, -1, -1, -1, -1, -1, 1026, 1027, -1, -1, 43, 44, -1, 635, -1, 445, 49, 378, -1,
  85, -1, -1, -1, 1043, -1, -1, -1, 388, 12, 13, 14, 15, 16, 17, 18, 19, 20, -1, -1, 3, 4, 5, 6, 7,
  8, 9, -1, 477, 114, -1, 3, 4, 5, 6, 7, 8, 9, 21, 22, 23, -1, -1, -1, 424, -1, -1, 427, -1, 21, 22,
  23, -1, -1, -1, -1, -1, -1, 878, 879, -1, 881, 114, -1, -1, 445, 49, 12, -1, -1, 3, 4, 5, 6, 7, 8,
  9, 49, 50, -1, -1, -1, -1, -1, -1, -1, 766, 757, 21, 22, 23, -1, -1, -1, -1, -1, -1, 477, -1, -1,
  -1, -1, 85, -1, -1, -1, -1, -1, -1, 54, 55, -1, 792, 85, 494, -1, 61, 62, 63, 64, 65, 66, 67, 68,
  69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 25, 26, 27, 114, -1, -1, 965, 32, 33,
  34, 35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1, 105, -1, 107, 108, 544, 110, 111, 112, -1, 114,
  115, 116, 117, 118, 119, 120, 121, 122, 123, -1, 629, 630, 631, -1, -1, -1, -1, -1, 868, -1, -1,
  -1, -1, -1, -1, -1, 876, 877, -1, 3, 4, 5, 6, 7, 8, 9, 876, 848, -1, 3, 4, 5, 6, 7, 8, 9, -1, 21,
  22, 23, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, -1, -1, -1, 877, -1, -1, 919, 44, -1,
  691, -1, 924, 49, -1, -1, 928, 629, 630, 631, -1, 933, 934, 49, 3, 4, 5, 6, 7, 8, 9, 933, -1, -1,
  907, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, -1, -1, -1, -1, 964, -1, 966, 928, -1,
  -1, -1, 85, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 49, -1, -1, -1, -1, -1, -1, -1, 990, 691, 106,
  3, 4, 5, 6, 7, 8, 9, 114, -1, -1, 964, -1, 966, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, -1,
  85, -1, -1, -1, -1, 792, -1, 794, 987, -1, -1, 798, 3, 4, 5, 6, 7, 8, 9, 998, -1, 49, -1, -1, -1,
  -1, -1, -1, -1, 114, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39,
  40, -1, 42, 43, 44, -1, -1, 1035, -1, 49, 50, 51, 52, 53, 1042, -1, -1, -1, -1, 855, -1, -1, -1,
  -1, -1, 792, -1, 794, -1, -1, -1, 798, -1, 800, -1, -1, -1, 114, -1, -1, -1, -1, -1, -1, -1, 85,
  -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 3, 4, 5, 6, 7, 8, 9, -1,
  -1, -1, -1, 114, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, -1, 855, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 49, -1, -1, -1, 880, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
  -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
  82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, -1,
  105, 106, 107, 108, -1, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
  29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52,
  53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
  77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
  101, 102, 103, 104, 105, 106, 107, 108, -1, 110, 111, 112, -1, 114, 115, 116, 117, 118, 119, 120,
  121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
  72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, 101, 102, 103, -1, 105, 106, 107, 108, -1, 110, 111, 112, 113, 114, 115, 116,
  117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42,
  43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
  67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
  91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, -1, 105, 106, 107, 108, 109, 110, 111,
  112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36,
  37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
  85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106,
  107, 108, -1, 110, 111, 112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5,
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102,
  103, 104, 105, 106, 107, 108, -1, 110, 111, 112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122,
  123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50,
  51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
  75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, -1, 110, 111, 112, -1, 114, 115, 116, 117, 118,
  119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45,
  -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
  70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93,
  94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, -1, 110, 111, 112, -1, 114,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40,
  -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
  65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, -1, 110,
  111, 112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35,
  36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
  60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
  84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, -1, 105, 106,
  107, 108, -1, 110, 111, 112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5,
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102,
  103, -1, 105, 106, 107, 108, -1, 110, 111, 112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122,
  123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50,
  51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
  75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
  99, 100, 101, 102, 103, -1, 105, 106, 107, 108, -1, 110, 111, 112, -1, 114, 115, 116, 117, 118,
  119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45,
  -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
  70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93,
  94, 95, 96, 97, 98, 99, 100, 101, 102, 103, -1, 105, 106, 107, 108, -1, 110, 111, 112, -1, 114,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40,
  -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
  65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1, 103, -1, 105, 106, 107, 108, -1, 110,
  111, 112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35,
  36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
  60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
  -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106,
  107, 108, -1, 110, 111, 112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5,
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1,
  103, -1, 105, 106, 107, 108, -1, 110, 111, 112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122,
  123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50,
  51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
  75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
  99, 100, 101, -1, 103, -1, 105, 106, 107, 108, -1, -1, 111, 112, -1, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1,
  -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
  71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94,
  95, 96, 97, 98, 99, 100, 101, -1, 103, -1, 105, 106, 107, 108, 109, 110, 111, 112, -1, 114, 115,
  116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1,
  42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
  66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89,
  90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1, 103, -1, 105, 106, 107, 108, 109, 110, 111,
  112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36,
  37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1,
  85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1, 103, -1, 105, 106, 107,
  108, 109, 110, 111, 112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
  33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56,
  57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
  81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1, 103, -1,
  105, 106, 107, 108, -1, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
  29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52,
  53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
  77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
  101, -1, 103, -1, 105, 106, 107, 108, -1, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
  121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
  72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, 101, -1, 103, -1, 105, 106, 107, 108, -1, 110, 111, 112, -1, 114, 115, 116,
  117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42,
  43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
  67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90,
  91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1, 103, -1, 105, 106, 107, 108, -1, 110, 111, 112,
  -1, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
  14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37,
  38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
  62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, 85,
  -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1, 103, -1, -1, 106, -1, 108,
  -1, 110, 111, 112, -1, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
  -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, -1, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
  82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1, 103, -1, -1,
  106, -1, 108, -1, 110, 111, 112, -1, 114, 115, -1, 117, 118, 119, 120, 121, 122, 123, 124, 3, 4,
  5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, -1, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, -1, -1, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1,
  103, -1, -1, 106, -1, 108, -1, -1, 111, 112, -1, 114, 115, -1, 117, 118, 119, 120, 121, 122, 123,
  124, 3, 4, 5, 6, 7, 8, 9, -1, -1, 12, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, 26, 27,
  28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1, -1, -1, -1, 49, 50, -1,
  -1, -1, 54, 55, -1, -1, -1, -1, -1, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
  76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
  100, 101, -1, -1, -1, 105, -1, 107, 108, -1, 110, 111, 112, -1, 114, 115, 116, 117, 118, 119, 120,
  121, 122, 123, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1,
  25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, -1, -1, -1, -1,
  49, 50, 51, -1, 53, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
  99, 100, 101, 102, 3, 4, 5, 6, 7, 8, 9, 49, -1, -1, -1, 114, -1, -1, -1, -1, -1, -1, 21, 22, 23,
  -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, -1, 85, -1,
  -1, 49, 50, -1, -1, 53, -1, -1, -1, -1, -1, -1, -1, -1, -1, 102, 103, -1, -1, 106, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, 101, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, 114, -1, -1, -1, -1, -1, 21,
  22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, 43, -1, -1,
  -1, -1, -1, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, 9, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, 101, 3, 4, 5, 6, 7, 8, 9, 49, -1, -1, -1, -1, 114, -1, -1, -1, -1, -1, 21,
  22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, -1,
  85, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 102, -1, -1, -1, 106, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93,
  94, 95, 96, 97, 98, 99, 100, 101, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, 114, -1, -1, -1, -1,
  -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, 43,
  -1, -1, -1, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, 9, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93,
  94, 95, 96, 97, 98, 99, 100, 101, 44, -1, -1, -1, -1, 49, -1, -1, -1, -1, -1, -1, 114, 3, 4, 5, 6,
  7, 8, 9, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, -1, 21, 22, 23, -1, -1, 3,
  4, 5, 6, 7, 8, 9, -1, -1, -1, 43, -1, -1, -1, -1, -1, 49, -1, 21, 22, 23, -1, -1, 49, -1, -1, -1,
  114, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 49, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, 85, -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1,
  103, -1, -1, 106, 114, 85, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38,
  39, 40, 106, 42, 43, 44, -1, -1, -1, -1, 49, 50, -1, -1, 53, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 85, -1,
  87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 3, 4, 5, 6, 7, 8, 9, 32, 33, 34, 35,
  36, 37, 38, 39, 40, 41, 42, 21, 22, 23, -1, 25, 26, 27, 28, 51, -1, -1, 32, 33, 34, 35, 36, 37,
  38, 39, 40, -1, 42, 43, -1, -1, -1, -1, -1, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1, -1, -1, -1, -1, 85,
  -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36,
  37, 38, 39, 40, -1, 42, 43, -1, -1, -1, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 3, 4, 5, 6, 7, 8, 9, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35,
  36, 37, 38, 39, 40, -1, 42, -1, -1, -1, -1, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 3, 4, 5, 6, 7, 8, 9, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, -1, 27, 28, -1, -1, -1, 32, 33, 34,
  35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1, -1, -1, -1, 49, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 3, 4, 5, 6, 7, 8, 9,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, -1, -1, -1, -1, -1, 32, 33,
  34, 35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1, -1, -1, -1, 49, 50, -1, -1, -1, -1, -1, 32, 33,
  34, 35, 36, 37, 38, 39, 40, 41, 42, -1, -1, -1, -1, -1, -1, -1, -1, 51, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 78, -1, -1, -1,
  -1, -1, -1, -1, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, -1, -1,
  -1, -1, -1, -1, -1, 111, -1, -1, -1, 115, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, -1, -1, -1,
  -1, -1, -1, -1, -1, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, 78, -1, -1, -1, -1, -1, -1, -1, -1, 87, 88, 89, 90, 91, 92, 93,
  94, 95, 96, 97, 98, 99, 100, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, 111, -1, -1, -1, 115 };

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned short yystos[] = { 0, 126, 127, 0, 128, 340, 341, 3, 4, 5, 6, 7, 8, 9, 21, 22,
  23, 25, 26, 27, 28, 32, 33, 34, 35, 36, 37, 38, 39, 40, 42, 43, 44, 49, 50, 51, 52, 53, 85, 87,
  88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 114, 129, 130, 131, 132, 133, 135,
  136, 137, 138, 139, 141, 144, 159, 160, 161, 163, 164, 174, 175, 184, 186, 187, 189, 206, 207,
  208, 209, 212, 213, 216, 221, 262, 292, 293, 294, 295, 297, 298, 299, 300, 302, 304, 305, 308,
  309, 310, 311, 312, 314, 315, 318, 319, 330, 331, 332, 352, 25, 26, 12, 44, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
  35, 36, 37, 38, 39, 40, 42, 43, 44, 45, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
  62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
  86, 103, 105, 106, 107, 108, 110, 111, 112, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
  331, 332, 363, 364, 365, 395, 396, 397, 398, 399, 303, 320, 3, 4, 5, 6, 7, 8, 9, 42, 139, 144,
  161, 164, 294, 295, 300, 302, 308, 314, 3, 4, 5, 6, 7, 8, 9, 103, 305, 3, 4, 5, 6, 7, 8, 9, 21,
  22, 23, 50, 52, 185, 292, 294, 295, 299, 300, 302, 306, 140, 350, 351, 306, 103, 350, 44, 53, 130,
  137, 138, 164, 170, 187, 189, 206, 262, 308, 314, 46, 102, 103, 235, 236, 235, 235, 235, 108, 144,
  308, 314, 102, 340, 44, 213, 240, 243, 293, 298, 300, 302, 146, 300, 302, 304, 305, 299, 293, 294,
  299, 340, 299, 110, 139, 144, 161, 164, 175, 213, 295, 309, 318, 340, 10, 11, 84, 200, 263, 271,
  273, 78, 111, 115, 268, 333, 334, 335, 336, 339, 316, 340, 340, 24, 353, 103, 395, 391, 391, 64,
  116, 190, 381, 391, 392, 391, 10, 11, 108, 385, 292, 12, 306, 350, 306, 350, 293, 139, 161, 179,
  180, 183, 200, 271, 391, 105, 134, 292, 292, 102, 342, 343, 216, 220, 221, 295, 41, 51, 264, 267,
  268, 307, 309, 332, 103, 102, 106, 145, 146, 295, 299, 300, 302, 352, 264, 162, 102, 106, 165,
  292, 292, 350, 308, 200, 106, 245, 391, 214, 350, 296, 45, 48, 226, 227, 108, 299, 299, 299, 301,
  306, 350, 306, 350, 213, 240, 340, 317, 274, 217, 218, 220, 221, 222, 238, 282, 293, 295, 265,
  105, 255, 256, 258, 259, 200, 271, 280, 333, 344, 345, 344, 344, 334, 336, 306, 53, 354, 355, 356,
  357, 358, 127, 392, 102, 110, 113, 393, 394, 395, 104, 191, 193, 194, 196, 198, 113, 102, 394,
  109, 387, 388, 386, 340, 176, 178, 268, 145, 176, 292, 306, 306, 177, 282, 293, 300, 302, 104,
  294, 300, 103, 102, 105, 352, 12, 54, 55, 64, 105, 107, 108, 110, 112, 116, 362, 363, 216, 220,
  102, 265, 263, 340, 148, 143, 3, 102, 147, 340, 146, 300, 302, 295, 102, 3, 4, 5, 6, 7, 8, 9, 167,
  168, 304, 166, 165, 340, 292, 295, 246, 247, 292, 103, 104, 248, 249, 145, 300, 346, 347, 385,
  244, 375, 264, 145, 264, 292, 306, 312, 313, 338, 32, 33, 223, 224, 225, 342, 223, 284, 285, 286,
  342, 217, 222, 293, 102, 107, 257, 103, 389, 108, 109, 271, 352, 337, 185, 292, 113, 356, 107,
  298, 305, 360, 361, 104, 102, 113, 107, 381, 25, 27, 164, 294, 300, 302, 308, 325, 326, 329, 330,
  26, 50, 201, 189, 371, 371, 371, 102, 177, 183, 102, 165, 176, 176, 102, 107, 108, 342, 102, 127,
  188, 3, 112, 112, 64, 116, 109, 113, 29, 30, 31, 104, 149, 150, 29, 30, 31, 37, 154, 155, 158,
  292, 106, 340, 146, 104, 107, 105, 315, 102, 304, 107, 396, 398, 391, 109, 84, 250, 252, 340, 299,
  229, 352, 248, 24, 85, 105, 106, 107, 110, 112, 124, 331, 332, 363, 364, 365, 369, 370, 377, 378,
  379, 381, 382, 385, 389, 102, 102, 102, 165, 312, 78, 111, 228, 108, 112, 287, 288, 106, 108, 342,
  266, 64, 110, 116, 366, 367, 369, 379, 390, 260, 365, 272, 338, 106, 113, 357, 299, 84, 359, 385,
  192, 292, 292, 292, 318, 200, 269, 273, 268, 327, 269, 201, 64, 105, 107, 109, 110, 116, 369, 372,
  373, 109, 109, 102, 102, 178, 181, 104, 314, 113, 113, 340, 106, 157, 158, 107, 37, 156, 200, 142,
  340, 168, 169, 105, 171, 396, 247, 200, 200, 104, 215, 107, 253, 3, 230, 241, 109, 384, 380, 383,
  102, 226, 219, 289, 288, 13, 14, 15, 16, 283, 239, 267, 368, 367, 376, 107, 109, 108, 275, 285,
  298, 193, 328, 281, 282, 195, 344, 306, 197, 269, 248, 269, 41, 42, 53, 102, 131, 136, 138, 151,
  152, 153, 159, 160, 174, 184, 187, 189, 210, 211, 212, 240, 262, 292, 293, 295, 308, 314, 292,
  340, 292, 154, 366, 392, 102, 237, 223, 84, 251, 314, 245, 371, 375, 371, 346, 248, 290, 291, 248,
  370, 102, 104, 373, 374, 261, 276, 306, 275, 105, 202, 203, 269, 279, 333, 202, 199, 109, 102,
  340, 138, 152, 153, 187, 189, 210, 262, 293, 308, 235, 102, 220, 240, 295, 200, 200, 155, 200, 47,
  252, 269, 242, 113, 381, 113, 67, 232, 233, 109, 113, 366, 109, 366, 248, 204, 109, 269, 202, 182,
  136, 144, 172, 189, 211, 308, 314, 308, 342, 220, 222, 398, 254, 105, 231, 113, 234, 229, 348,
  349, 109, 205, 377, 270, 278, 350, 144, 172, 308, 235, 144, 200, 102, 342, 103, 255, 19, 55, 56,
  308, 319, 321, 322, 231, 352, 277, 377, 275, 32, 34, 45, 48, 103, 106, 145, 173, 350, 144, 350,
  102, 391, 319, 323, 268, 278, 398, 398, 391, 392, 147, 145, 350, 145, 173, 104, 324, 306, 346,
  104, 102, 173, 145, 147, 306, 392, 173, 102 };

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned short yyr1[] = { 0, 125, 126, 127, 128, 127, 129, 129, 129, 129, 129, 129,
  129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 130, 130, 130, 130, 130, 130, 131, 131,
  132, 133, 134, 133, 135, 136, 136, 137, 137, 137, 138, 138, 140, 139, 142, 141, 141, 143, 141,
  141, 144, 144, 144, 145, 145, 145, 146, 146, 147, 147, 148, 149, 148, 148, 150, 150, 150, 151,
  151, 151, 151, 151, 151, 151, 151, 151, 151, 151, 151, 151, 151, 152, 152, 152, 152, 152, 152,
  153, 153, 153, 153, 154, 154, 155, 155, 155, 156, 156, 157, 157, 158, 158, 158, 159, 159, 159,
  160, 160, 162, 161, 163, 163, 164, 164, 164, 165, 166, 165, 167, 167, 168, 168, 169, 168, 170,
  171, 171, 172, 172, 172, 172, 173, 173, 174, 174, 175, 175, 175, 175, 175, 176, 177, 177, 178,
  179, 179, 181, 182, 180, 183, 184, 185, 185, 185, 185, 185, 185, 186, 188, 187, 189, 190, 189,
  191, 192, 191, 194, 195, 193, 196, 197, 193, 198, 199, 193, 200, 200, 201, 201, 202, 202, 204,
  203, 205, 205, 206, 206, 206, 206, 207, 208, 208, 208, 209, 209, 209, 210, 210, 210, 211, 211,
  211, 211, 212, 212, 212, 214, 215, 213, 216, 217, 219, 218, 220, 221, 222, 223, 224, 224, 225,
  225, 226, 226, 226, 227, 227, 228, 228, 228, 229, 229, 230, 231, 231, 231, 231, 232, 232, 234,
  233, 235, 235, 235, 236, 237, 237, 239, 238, 241, 242, 240, 244, 243, 245, 245, 246, 246, 247,
  247, 248, 249, 248, 250, 251, 250, 250, 250, 253, 254, 252, 255, 255, 257, 256, 258, 256, 259,
  256, 260, 261, 260, 262, 263, 264, 264, 265, 266, 265, 267, 268, 268, 269, 270, 269, 271, 272,
  271, 274, 273, 273, 275, 276, 277, 275, 275, 278, 278, 278, 278, 278, 278, 279, 279, 280, 280,
  281, 281, 282, 282, 283, 283, 283, 283, 284, 284, 286, 285, 287, 287, 289, 288, 290, 291, 290,
  292, 292, 293, 293, 293, 293, 293, 294, 294, 294, 295, 295, 295, 295, 295, 295, 296, 295, 297,
  298, 299, 301, 300, 303, 302, 304, 304, 304, 304, 304, 304, 304, 304, 304, 304, 305, 305, 305,
  305, 305, 305, 305, 306, 306, 307, 307, 307, 307, 308, 308, 309, 309, 309, 309, 310, 310, 310,
  310, 310, 311, 311, 311, 312, 312, 313, 313, 314, 316, 315, 317, 315, 318, 318, 318, 319, 319,
  320, 319, 319, 319, 321, 323, 322, 324, 322, 325, 327, 326, 328, 326, 329, 329, 329, 329, 329,
  329, 329, 330, 330, 331, 331, 331, 331, 331, 331, 331, 331, 331, 331, 332, 332, 332, 332, 332,
  332, 332, 332, 332, 332, 332, 332, 332, 332, 332, 333, 333, 333, 333, 334, 335, 337, 336, 338,
  338, 339, 339, 341, 340, 343, 342, 345, 344, 347, 346, 349, 348, 351, 350, 352, 352, 353, 354,
  354, 355, 356, 356, 356, 356, 358, 357, 359, 359, 360, 360, 361, 361, 362, 362, 362, 362, 362,
  362, 362, 362, 362, 362, 362, 362, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363,
  363, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363, 363,
  363, 363, 363, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364,
  364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 365, 365, 365, 365, 365,
  365, 365, 365, 365, 366, 366, 367, 367, 367, 368, 367, 367, 369, 369, 370, 370, 370, 370, 370,
  370, 370, 370, 370, 370, 371, 371, 372, 372, 372, 372, 373, 373, 373, 374, 374, 375, 375, 376,
  376, 377, 377, 378, 378, 378, 380, 379, 381, 381, 383, 382, 384, 382, 386, 385, 387, 385, 388,
  385, 390, 389, 391, 391, 392, 392, 393, 393, 394, 394, 395, 395, 395, 395, 395, 395, 395, 395,
  395, 395, 395, 395, 395, 395, 395, 395, 395, 396, 397, 397, 398, 399, 399, 399 };

/* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const unsigned char yyr2[] = { 0, 2, 1, 0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 1, 2, 2, 2, 2, 2, 2, 5, 4, 5, 4, 0, 6, 5, 1, 2, 4, 3, 5, 4, 5, 0, 5, 0, 7, 4, 0, 5, 2, 1, 1, 1,
  3, 4, 2, 1, 1, 0, 1, 0, 0, 4, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 2, 2,
  2, 3, 4, 3, 4, 1, 4, 2, 4, 4, 0, 1, 0, 1, 1, 1, 1, 5, 3, 6, 4, 5, 0, 5, 4, 3, 1, 2, 2, 0, 0, 3, 1,
  3, 0, 1, 0, 4, 6, 2, 1, 5, 6, 3, 4, 5, 3, 1, 2, 5, 5, 6, 5, 6, 2, 0, 3, 2, 1, 1, 0, 0, 8, 1, 3, 1,
  2, 2, 2, 3, 3, 4, 0, 8, 3, 0, 5, 1, 0, 4, 0, 0, 5, 0, 0, 5, 0, 0, 6, 0, 1, 1, 1, 0, 1, 0, 3, 1, 2,
  2, 2, 2, 2, 3, 4, 2, 3, 2, 3, 4, 2, 4, 5, 3, 1, 1, 2, 1, 2, 3, 0, 0, 7, 2, 2, 0, 6, 2, 1, 2, 7, 0,
  1, 1, 1, 0, 2, 1, 1, 1, 0, 1, 1, 0, 2, 1, 0, 2, 2, 2, 0, 1, 0, 3, 3, 1, 1, 6, 0, 6, 0, 6, 0, 0, 8,
  0, 5, 0, 2, 1, 3, 3, 3, 0, 0, 2, 1, 0, 4, 3, 1, 0, 0, 6, 0, 1, 0, 3, 0, 2, 0, 4, 1, 0, 4, 4, 2, 0,
  2, 0, 0, 4, 2, 0, 1, 3, 0, 6, 3, 0, 5, 0, 3, 1, 0, 0, 0, 7, 1, 0, 2, 2, 3, 3, 2, 1, 2, 1, 2, 0, 1,
  2, 4, 1, 1, 1, 1, 0, 1, 0, 2, 1, 2, 0, 5, 0, 0, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3,
  0, 5, 1, 1, 1, 0, 5, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 3, 1, 1, 1, 1, 2,
  3, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 0, 3, 0, 4, 1, 3, 4, 1, 1, 0, 4, 2, 2, 2, 0,
  3, 0, 4, 2, 0, 3, 0, 4, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
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
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 239, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 241, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  237, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  231, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 233, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45,
  47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 0, 71, 73, 75, 77, 79, 81, 0, 83, 85, 87, 89, 0,
  0, 91, 93, 95, 97, 99, 101, 103, 105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125, 127, 129,
  131, 133, 135, 137, 139, 141, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 0, 163, 0, 165,
  167, 169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 0, 195, 0, 0, 197, 199, 201,
  0, 203, 205, 207, 0, 209, 211, 213, 215, 217, 219, 221, 223, 225, 227, 229, 0, 0, 0, 0, 0, 0, 0,
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
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* YYCONFL[I] -- lists of conflicting rule numbers, each terminated by
   0, pointed into by YYCONFLP.  */
static const short yyconfl[] = { 0, 407, 0, 407, 0, 407, 0, 318, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0,
  626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 626, 0, 301, 0,
  301, 0, 301, 0, 311, 0, 407, 0, 407, 0 };

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

    case 43:

    {
      pushType();
    }

    break;

    case 44:

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

    case 45:

    {
      start_class((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-4)].yystate.yysemantics.yysval.integer));
      currentClass->IsFinal =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer);
    }

    break;

    case 47:

    {
      start_class((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.integer));
      currentClass->IsFinal =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer);
    }

    break;

    case 48:

    {
      start_class(
        NULL, (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 50:

    {
      start_class(
        NULL, (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 51:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 52:

    {
      ((*yyvalp).integer) = 1;
    }

    break;

    case 53:

    {
      ((*yyvalp).integer) = 2;
    }

    break;

    case 54:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
    }

    break;

    case 55:

    {
      ((*yyvalp).str) = vtkstrcat3(
        "::", (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
    }

    break;

    case 59:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 60:

    {
      ((*yyvalp).integer) =
        (strcmp((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str),
           "final") == 0);
    }

    break;

    case 62:

    {
      startSig();
      clearType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }

    break;

    case 65:

    {
      access_level = VTK_ACCESS_PUBLIC;
    }

    break;

    case 66:

    {
      access_level = VTK_ACCESS_PRIVATE;
    }

    break;

    case 67:

    {
      access_level = VTK_ACCESS_PROTECTED;
    }

    break;

    case 91:

    {
      output_friend_function();
    }

    break;

    case 94:

    {
      add_base_class(currentClass,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str), access_level,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 95:

    {
      add_base_class(currentClass,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer),
        (VTK_PARSE_VIRTUAL |
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer)));
    }

    break;

    case 96:

    {
      add_base_class(currentClass,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.integer),
        ((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer) |
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer)));
    }

    break;

    case 97:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 98:

    {
      ((*yyvalp).integer) = VTK_PARSE_VIRTUAL;
    }

    break;

    case 99:

    {
      ((*yyvalp).integer) = access_level;
    }

    break;

    case 101:

    {
      ((*yyvalp).integer) = VTK_ACCESS_PUBLIC;
    }

    break;

    case 102:

    {
      ((*yyvalp).integer) = VTK_ACCESS_PRIVATE;
    }

    break;

    case 103:

    {
      ((*yyvalp).integer) = VTK_ACCESS_PROTECTED;
    }

    break;

    case 109:

    {
      pushType();
    }

    break;

    case 110:

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

    case 111:

    {
      start_enum((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.integer),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer),
        getTypeId());
      clearTypeId();
      ((*yyvalp).str) = (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str);
    }

    break;

    case 112:

    {
      start_enum(NULL,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer),
        getTypeId());
      clearTypeId();
      ((*yyvalp).str) = NULL;
    }

    break;

    case 113:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 114:

    {
      ((*yyvalp).integer) = 1;
    }

    break;

    case 115:

    {
      ((*yyvalp).integer) = 1;
    }

    break;

    case 116:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 117:

    {
      pushType();
    }

    break;

    case 118:

    {
      ((*yyvalp).integer) = getType();
      popType();
    }

    break;

    case 122:

    {
      closeComment();
      add_enum((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str), NULL);
    }

    break;

    case 123:

    {
      postSig("=");
      markSig();
      closeComment();
    }

    break;

    case 124:

    {
      chopSig();
      add_enum(
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.str), copySig());
    }

    break;

    case 147:

    {
      pushFunction();
      postSig("(");
    }

    break;

    case 148:

    {
      postSig(")");
    }

    break;

    case 149:

    {
      ((*yyvalp).integer) = (VTK_PARSE_FUNCTION |
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-7)].yystate.yysemantics.yysval.integer));
      popFunction();
    }

    break;

    case 150:

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

    case 151:

    {
      add_using((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str), 0);
    }

    break;

    case 153:

    {
      ((*yyvalp).str) = (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str);
    }

    break;

    case 154:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
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
        vtkstrcat3((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
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
      add_using((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str), 1);
    }

    break;

    case 159:

    {
      markSig();
    }

    break;

    case 160:

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

    case 161:

    {
      postSig("template<> ");
      clearTypeId();
    }

    break;

    case 162:

    {
      postSig("template<");
      pushType();
      clearType();
      clearTypeId();
      startTemplate();
    }

    break;

    case 163:

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

    case 165:

    {
      chopSig();
      postSig(", ");
      clearType();
      clearTypeId();
    }

    break;

    case 167:

    {
      markSig();
    }

    break;

    case 168:

    {
      add_template_parameter(getType(),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer), copySig());
    }

    break;

    case 170:

    {
      markSig();
    }

    break;

    case 171:

    {
      add_template_parameter(0,
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer), copySig());
    }

    break;

    case 173:

    {
      pushTemplate();
      markSig();
    }

    break;

    case 174:

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

    case 176:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 177:

    {
      postSig("...");
      ((*yyvalp).integer) = VTK_PARSE_PACK;
    }

    break;

    case 178:

    {
      postSig("class ");
    }

    break;

    case 179:

    {
      postSig("typename ");
    }

    break;

    case 182:

    {
      postSig("=");
      markSig();
    }

    break;

    case 183:

    {
      int i = currentTemplate->NumberOfParameters - 1;
      ValueInfo* param = currentTemplate->Parameters[i];
      chopSig();
      param->Value = copySig();
    }

    break;

    case 186:

    {
      output_function();
    }

    break;

    case 187:

    {
      output_function();
    }

    break;

    case 188:

    {
      reject_function();
    }

    break;

    case 189:

    {
      reject_function();
    }

    break;

    case 197:

    {
      output_function();
    }

    break;

    case 207:

    {
      postSig("(");
      currentFunction->IsExplicit = ((getType() & VTK_PARSE_EXPLICIT) != 0);
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }

    break;

    case 208:

    {
      postSig(")");
    }

    break;

    case 209:

    {
      postSig(";");
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", "operator typecast");
    }

    break;

    case 210:

    {
      ((*yyvalp).str) = copySig();
    }

    break;

    case 211:

    {
      postSig(";");
      closeSig();
      currentFunction->Name =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }

    break;

    case 212:

    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }

    break;

    case 213:

    {
      postSig(")");
    }

    break;

    case 214:

    {
      chopSig();
      ((*yyvalp).str) = vtkstrcat(
        copySig(), (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 215:

    {
      markSig();
      postSig("operator ");
    }

    break;

    case 216:

    {
      postSig(";");
      closeSig();
      currentFunction->Name =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }

    break;

    case 220:

    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }

    break;

    case 221:

    {
      postSig(" volatile");
    }

    break;

    case 223:

    {
      chopSig();
    }

    break;

    case 225:

    {
      postSig(" noexcept");
    }

    break;

    case 226:

    {
      postSig(" throw");
    }

    break;

    case 228:

    {
      postSig("&");
    }

    break;

    case 229:

    {
      postSig("&&");
    }

    break;

    case 232:

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

    case 234:

    {
      currentFunction->IsDeleted = 1;
    }

    break;

    case 236:

    {
      postSig(" = 0");
      currentFunction->IsPureVirtual = 1;
      if (currentClass)
      {
        currentClass->IsAbstract = 1;
      }
    }

    break;

    case 239:

    {
      postSig(" -> ");
      clearType();
      clearTypeId();
    }

    break;

    case 240:

    {
      chopSig();
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }

    break;

    case 247:

    {
      postSig("(");
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }

    break;

    case 248:

    {
      postSig(")");
    }

    break;

    case 249:

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
        currentFunction->Deprecation = currentDeprecation;
      }
      currentFunction->Name =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.str);
      currentFunction->Comment = vtkstrdup(getComment());
    }

    break;

    case 250:

    {
      openSig();
    }

    break;

    case 251:

    {
      postSig(";");
      closeSig();
      vtkParseDebug("Parsed func", currentFunction->Name);
    }

    break;

    case 252:

    {
      pushType();
      postSig("(");
    }

    break;

    case 253:

    {
      postSig(")");
      popType();
    }

    break;

    case 261:

    {
      clearType();
      clearTypeId();
    }

    break;

    case 263:

    {
      clearType();
      clearTypeId();
    }

    break;

    case 264:

    {
      clearType();
      clearTypeId();
      postSig(", ");
    }

    break;

    case 266:

    {
      currentFunction->IsVariadic = 1;
      postSig(", ...");
    }

    break;

    case 267:

    {
      currentFunction->IsVariadic = 1;
      postSig("...");
    }

    break;

    case 268:

    {
      markSig();
    }

    break;

    case 269:

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

    case 270:

    {
      int i = currentFunction->NumberOfParameters - 1;
      if (getVarValue())
      {
        currentFunction->Parameters[i]->Value = getVarValue();
      }
    }

    break;

    case 271:

    {
      clearVarValue();
    }

    break;

    case 273:

    {
      postSig("=");
      clearVarValue();
      markSig();
    }

    break;

    case 274:

    {
      chopSig();
      setVarValue(copySig());
    }

    break;

    case 275:

    {
      clearVarValue();
      markSig();
    }

    break;

    case 276:

    {
      chopSig();
      setVarValue(copySig());
    }

    break;

    case 277:

    {
      clearVarValue();
      markSig();
      postSig("(");
    }

    break;

    case 278:

    {
      chopSig();
      postSig(")");
      setVarValue(copySig());
    }

    break;

    case 279:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 280:

    {
      postSig(", ");
    }

    break;

    case 283:

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

    case 287:

    {
      postSig(", ");
    }

    break;

    case 290:

    {
      setTypePtr(0);
    }

    break;

    case 291:

    {
      setTypePtr((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 292:

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

    case 293:

    {
      postSig(")");
    }

    break;

    case 294:

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

    case 295:

    {
      ((*yyvalp).integer) =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.integer);
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

    case 298:

    {
      postSig("(");
      scopeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      postSig("*");
    }

    break;

    case 299:

    {
      ((*yyvalp).integer) =
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer);
    }

    break;

    case 300:

    {
      postSig("(");
      scopeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      postSig("&");
      ((*yyvalp).integer) = VTK_PARSE_REF;
    }

    break;

    case 301:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 302:

    {
      pushFunction();
      postSig("(");
    }

    break;

    case 303:

    {
      postSig(")");
    }

    break;

    case 304:

    {
      ((*yyvalp).integer) = VTK_PARSE_FUNCTION;
      popFunction();
    }

    break;

    case 305:

    {
      ((*yyvalp).integer) = VTK_PARSE_ARRAY;
    }

    break;

    case 308:

    {
      currentFunction->IsConst = 1;
    }

    break;

    case 313:

    {
      ((*yyvalp).integer) = add_indirection(
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 315:

    {
      ((*yyvalp).integer) = add_indirection(
        (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer),
        (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 316:

    {
      clearVarName();
      chopSig();
    }

    break;

    case 318:

    {
      setVarName((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
    }

    break;

    case 319:

    {
      setVarName((((yyGLRStackItem const*)yyvsp)[YYFILL(-3)].yystate.yysemantics.yysval.str));
    }

    break;

    case 324:

    {
      clearArray();
    }

    break;

    case 326:

    {
      clearArray();
    }

    break;

    case 330:

    {
      postSig("[");
    }

    break;

    case 331:

    {
      postSig("]");
    }

    break;

    case 332:

    {
      pushArraySize("");
    }

    break;

    case 333:

    {
      markSig();
    }

    break;

    case 334:

    {
      chopSig();
      pushArraySize(copySig());
    }

    break;

    case 340:

    {
      ((*yyvalp).str) =
        vtkstrcat("~", (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 341:

    {
      ((*yyvalp).str) =
        vtkstrcat("~", (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 342:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 343:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 344:

    {
      ((*yyvalp).str) =
        vtkstrcat((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
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
        vtkstrcat3((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 349:

    {
      ((*yyvalp).str) =
        vtkstrcat3((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 350:

    {
      ((*yyvalp).str) =
        vtkstrcat3((((yyGLRStackItem const*)yyvsp)[YYFILL(-2)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 351:

    {
      postSig("template ");
    }

    break;

    case 352:

    {
      ((*yyvalp).str) =
        vtkstrcat4((((yyGLRStackItem const*)yyvsp)[YYFILL(-4)].yystate.yysemantics.yysval.str),
          "template ", (((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str),
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 353:

    {
      postSig("~");
    }

    break;

    case 354:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 355:

    {
      ((*yyvalp).str) = "::";
      postSig(((*yyvalp).str));
    }

    break;

    case 356:

    {
      markSig();
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
      postSig("<");
    }

    break;

    case 357:

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

    case 358:

    {
      markSig();
      postSig("decltype");
    }

    break;

    case 359:

    {
      chopSig();
      ((*yyvalp).str) = copySig();
      clearTypeId();
    }

    break;

    case 360:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 361:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 362:

    {
      postSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
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

    case 380:

    {
      setTypeBase(buildTypeBase(
        getType(), (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer)));
    }

    break;

    case 381:

    {
      setTypeMod(VTK_PARSE_TYPEDEF);
    }

    break;

    case 382:

    {
      setTypeMod(VTK_PARSE_FRIEND);
    }

    break;

    case 385:

    {
      setTypeMod((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
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
      postSig("constexpr ");
      ((*yyvalp).integer) = 0;
    }

    break;

    case 389:

    {
      postSig("mutable ");
      ((*yyvalp).integer) = VTK_PARSE_MUTABLE;
    }

    break;

    case 390:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 391:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 392:

    {
      postSig("static ");
      ((*yyvalp).integer) = VTK_PARSE_STATIC;
    }

    break;

    case 393:

    {
      postSig("thread_local ");
      ((*yyvalp).integer) = VTK_PARSE_THREAD_LOCAL;
    }

    break;

    case 394:

    {
      ((*yyvalp).integer) = 0;
    }

    break;

    case 395:

    {
      postSig("virtual ");
      ((*yyvalp).integer) = VTK_PARSE_VIRTUAL;
    }

    break;

    case 396:

    {
      postSig("explicit ");
      ((*yyvalp).integer) = VTK_PARSE_EXPLICIT;
    }

    break;

    case 397:

    {
      postSig("const ");
      ((*yyvalp).integer) = VTK_PARSE_CONST;
    }

    break;

    case 398:

    {
      postSig("volatile ");
      ((*yyvalp).integer) = VTK_PARSE_VOLATILE;
    }

    break;

    case 400:

    {
      ((*yyvalp).integer) =
        ((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.integer) |
          (((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 402:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 404:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 407:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
    }

    break;

    case 408:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
    }

    break;

    case 410:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = 0;
    }

    break;

    case 411:

    {
      postSig("typename ");
    }

    break;

    case 412:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(-1)].yystate.yysemantics.yysval.str));
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

    case 416:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 418:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 421:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 423:

    {
      setTypeBase((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.integer));
    }

    break;

    case 426:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = 0;
    }

    break;

    case 427:

    {
      postSig(" ");
      setTypeId((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) =
        guess_id_type((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
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
      setTypeId("");
    }

    break;

    case 434:

    {
      typeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = VTK_PARSE_STRING;
    }

    break;

    case 435:

    {
      typeSig((((yyGLRStackItem const*)yyvsp)[YYFILL(0)].yystate.yysemantics.yysval.str));
      ((*yyvalp).integer) = VTK_PARSE_UNICODE_STRING;
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

#define yypact_value_is_default(Yystate) (!!((Yystate) == (-834)))

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
const char* type_class(unsigned int type, const char* classname)
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
void start_class(const char* classname, int is_struct_or_union)
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
    currentClass->Deprecation = currentDeprecation;
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
void end_class(void)
{
  /* add default constructors */
  vtkParse_AddDefaultConstructors(currentClass, data->Strings);

  popClass();
}

/* add a base class to the specified class */
void add_base_class(ClassInfo* cls, const char* name, int access_lev, unsigned int extra)
{
  /* "extra" can contain VTK_PARSE_VIRTUAL and VTK_PARSE_PACK */
  if (cls && access_lev == VTK_ACCESS_PUBLIC && (extra & VTK_PARSE_VIRTUAL) == 0 &&
    (extra & VTK_PARSE_PACK) == 0)
  {
    vtkParse_AddStringToArray(&cls->SuperClasses, &cls->NumberOfSuperClasses, name);
  }
}

/* add a using declaration or directive */
void add_using(const char* name, int is_namespace)
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
void start_enum(const char* name, int is_scoped, unsigned int type, const char* basename)
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
void end_enum(void)
{
  if (currentClass && currentClass->ItemType == VTK_ENUM_INFO)
  {
    popClass();
  }

  currentEnumName = NULL;
  currentEnumValue = NULL;
}

/* add a constant to the enum */
void add_enum(const char* name, const char* value)
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

  add_constant(name, currentEnumValue, currentEnumType, currentEnumName, 2);
}

/* for a macro constant, guess the constant type, doesn't do any math */
unsigned int guess_constant_type(const char* valstring)
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
void add_constant(
  const char* name, const char* value, unsigned int type, const char* typeclass, int flag)
{
  ValueInfo* con = (ValueInfo*)malloc(sizeof(ValueInfo));
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
unsigned int guess_id_type(const char* cp)
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
    else if (strcmp(dp, "vtkUnicodeString") == 0)
    {
      t = VTK_PARSE_UNICODE_STRING;
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
void add_template_parameter(unsigned int datatype, unsigned int extra, const char* funcSig)
{
  ValueInfo* param = (ValueInfo*)malloc(sizeof(ValueInfo));
  vtkParse_InitValue(param);
  handle_complex_type(param, 0, datatype, extra, funcSig);
  param->Name = getVarName();
  vtkParse_AddParameterToTemplate(currentTemplate, param);
}

/* set the return type for the current function */
void set_return(
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
    sprintf(text, "%i", count);
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

int count_from_dimensions(ValueInfo* val)
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
void handle_complex_type(ValueInfo* val, unsigned int attributes, unsigned int datatype,
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
void handle_attribute(const char* att, int pack)
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
    else if (l == 15 && strncmp(att, "vtk::deprecated", l) == 0 && args &&
      (role == VTK_PARSE_ATTRIB_DECL || role == VTK_PARSE_ATTRIB_CLASS))
    {
      addAttribute(VTK_PARSE_DEPRECATED);
      currentDeprecation = vtkstrndup(args, la);
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
void add_legacy_parameter(FunctionInfo* func, ValueInfo* param)
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
  vtkParse_FreeFunction(currentFunction);
  currentFunction = (FunctionInfo*)malloc(sizeof(FunctionInfo));
  vtkParse_InitFunction(currentFunction);
  startSig();
  getMacro();
}

/* a simple routine that updates a few variables */
void output_function()
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
      currentFunction->Deprecation = currentDeprecation;
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
void output_friend_function()
{
  ClassInfo* tmpc = currentClass;
  currentClass = NULL;
  output_function();
  currentClass = tmpc;
}

/* dump predefined macros to the specified file. */
void dump_macros(const char* filename)
{
  MacroInfo* macro = NULL;
  FILE* ofile = stdout;
  int i;

  if (filename)
  {
    ofile = fopen(filename, "w");
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
  data->Strings = (StringCache*)malloc(sizeof(StringCache));
  vtkParse_InitStringCache(data->Strings);

  /* "preprocessor" is a global struct used by the parser */
  preprocessor = (PreprocessInfo*)malloc(sizeof(PreprocessInfo));
  vtkParsePreprocess_Init(preprocessor, filename);
  preprocessor->Strings = data->Strings;
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
  while ((n = fscanf(hfile, "%s %s %x %i", h_cls, h_func, &h_type, &h_value)) != EOF)
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
                  sprintf(text, "%i", h_value);
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

/* Free the FileInfo struct returned by vtkParse_ParseFile() */
void vtkParse_Free(FileInfo* file_info)
{
  vtkParse_FreeFile(file_info);
  vtkParse_FreeStringCache(file_info->Strings);
  free(file_info->Strings);
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
  static StringCache cache = { 0, 0, 0, 0 };
  static PreprocessInfo info = { 0, 0, 0, 0, 0, 0, &cache, 0, 0, 0, 0 };
  int val;
  int i;

  /* add include files specified on the command line */
  for (i = 0; i < NumberOfIncludeDirectories; i++)
  {
    vtkParsePreprocess_IncludeDirectory(&info, IncludeDirectories[i]);
  }

  return vtkParsePreprocess_FindIncludeFile(&info, filename, VTK_PARSE_SOURCE_INCLUDE, &val);
}
