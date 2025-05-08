// SPDX-FileCopyrightText: Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.
// SPDX-License-Identifier: GPL-3.0 WITH Bison-exception-2.2

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

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
static FileInfo* data = NULL;
static int parseDebug;

/* globals for caching directory listings */
static StringCache system_strings = { 0, 0, 0, 0 };
static SystemInfo system_cache = { &system_strings, NULL, NULL };

/* the "preprocessor" */
static PreprocessInfo* preprocessor = NULL;

/* whether to pre-define platform-specific macros */
static int PredefinePlatformMacros = 1;

/* include dirs specified on the command line */
static int NumberOfIncludeDirectories = 0;
static const char** IncludeDirectories = NULL;

/* macros specified on the command line */
static int NumberOfDefinitions = 0;
static const char** Definitions = NULL;

/* include specified on the command line */
static int NumberOfMacroIncludes = 0;
static const char** MacroIncludes = NULL;

/* for dumping diagnostics about macros */
static int DumpMacros = 0;
static const char* DumpFileName = NULL;

/* options that can be set by the programs that use the parser */
static int Recursive = 0;
static const char* CommandName = NULL;

/* various state variables */
static NamespaceInfo* currentNamespace = NULL;
static ClassInfo* currentClass = NULL;
static FunctionInfo* currentFunction = NULL;
static TemplateInfo* currentTemplate = NULL;
static const char* currentEnumName = NULL;
static const char* currentEnumValue = NULL;
static unsigned int currentEnumType = 0;
static const char* deprecationReason = NULL;
static const char* deprecationVersion = NULL;
static parse_access_t access_level = VTK_ACCESS_PUBLIC;

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
      memcpy(&cp[m], str[i], j[i]);
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
static char* commentText = NULL;
static size_t commentLength = 0;
static size_t commentAllocatedLength = 0;
static int commentState = 0;
static int commentMemberGroup = 0;
static int commentGroupDepth = 0;
static parse_dox_t commentType = DOX_COMMAND_OTHER;
static const char* commentTarget = NULL;

/* Struct for recognizing certain doxygen commands */
struct DoxygenCommandInfo
{
  const char* name;
  size_t length;
  parse_dox_t type;
};

/* List of doxygen commands (@cond is not handled yet) */
/* clang-format off */
static struct DoxygenCommandInfo doxygenCommands[] = {
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
static const char* macroName = NULL;
static int macroUsed = 0;
static int macroEnded = 0;

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
static NamespaceInfo* namespaceStack[10];
static int namespaceDepth = 0;

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
static ClassInfo* classStack[10];
static parse_access_t classAccessStack[10];
static int classDepth = 0;

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
static TemplateInfo* templateStack[10];
static int templateDepth = 0;

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
    vtkParse_FreeTemplate(currentTemplate);
  }
  currentTemplate = NULL;
}

/* push the template onto the stack, and start a new one */
static void pushTemplate(void)
{
  templateStack[templateDepth++] = currentTemplate;
  currentTemplate = NULL;
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
static int sigClosed = 0;
static size_t sigMark[10];
static size_t sigLength = 0;
static size_t sigAllocatedLength = 0;
static int sigMarkDepth = 0;
static char* signature = NULL;

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
static unsigned int storedType;
static unsigned int typeStack[10];
static unsigned int declAttributes;
static unsigned int attributeStack[10];
static int typeDepth = 0;

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
static int numberOfDimensions = 0;
static const char** arrayDimensions = NULL;

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
static const char* currentVarName = 0;
static const char* currentVarValue = 0;
static const char* currentId = 0;

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

static const char* pointerScopeStack[10];
static int pointerScopeDepth = 0;

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
static FunctionInfo* functionStack[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const char* functionVarNameStack[10];
static const char* functionTypeIdStack[10];
static int functionDepth = 0;

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

static int attributeRole = 0;
static const char* attributePrefix = NULL;

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

#ifndef YY_CAST
#ifdef __cplusplus
#define YY_CAST(Type, Val) static_cast<Type>(Val)
#define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type>(Val)
#else
#define YY_CAST(Type, Val) ((Type)(Val))
#define YY_REINTERPRET_CAST(Type, Val) ((Type)(Val))
#endif
#endif
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

/* Token kinds.  */
#ifndef YYTOKENTYPE
#define YYTOKENTYPE
enum yytokentype
{
  YYEMPTY = -2,
  YYEOF = 0,               /* "end of file"  */
  YYerror = 256,           /* error  */
  YYUNDEF = 257,           /* "invalid token"  */
  ID = 258,                /* ID  */
  VTK_ID = 259,            /* VTK_ID  */
  QT_ID = 260,             /* QT_ID  */
  StdString = 261,         /* StdString  */
  OSTREAM = 262,           /* OSTREAM  */
  ISTREAM = 263,           /* ISTREAM  */
  LP = 264,                /* LP  */
  LA = 265,                /* LA  */
  STRING_LITERAL = 266,    /* STRING_LITERAL  */
  STRING_LITERAL_UD = 267, /* STRING_LITERAL_UD  */
  INT_LITERAL = 268,       /* INT_LITERAL  */
  HEX_LITERAL = 269,       /* HEX_LITERAL  */
  BIN_LITERAL = 270,       /* BIN_LITERAL  */
  OCT_LITERAL = 271,       /* OCT_LITERAL  */
  FLOAT_LITERAL = 272,     /* FLOAT_LITERAL  */
  CHAR_LITERAL = 273,      /* CHAR_LITERAL  */
  ZERO = 274,              /* ZERO  */
  NULLPTR = 275,           /* NULLPTR  */
  SSIZE_T = 276,           /* SSIZE_T  */
  SIZE_T = 277,            /* SIZE_T  */
  NULLPTR_T = 278,         /* NULLPTR_T  */
  BEGIN_ATTRIB = 279,      /* BEGIN_ATTRIB  */
  STRUCT = 280,            /* STRUCT  */
  CLASS = 281,             /* CLASS  */
  UNION = 282,             /* UNION  */
  ENUM = 283,              /* ENUM  */
  PUBLIC = 284,            /* PUBLIC  */
  PRIVATE = 285,           /* PRIVATE  */
  PROTECTED = 286,         /* PROTECTED  */
  CONST = 287,             /* CONST  */
  VOLATILE = 288,          /* VOLATILE  */
  MUTABLE = 289,           /* MUTABLE  */
  STATIC = 290,            /* STATIC  */
  THREAD_LOCAL = 291,      /* THREAD_LOCAL  */
  VIRTUAL = 292,           /* VIRTUAL  */
  EXPLICIT = 293,          /* EXPLICIT  */
  INLINE = 294,            /* INLINE  */
  CONSTEXPR = 295,         /* CONSTEXPR  */
  FRIEND = 296,            /* FRIEND  */
  EXTERN = 297,            /* EXTERN  */
  OPERATOR = 298,          /* OPERATOR  */
  TEMPLATE = 299,          /* TEMPLATE  */
  THROW = 300,             /* THROW  */
  TRY = 301,               /* TRY  */
  CATCH = 302,             /* CATCH  */
  NOEXCEPT = 303,          /* NOEXCEPT  */
  DECLTYPE = 304,          /* DECLTYPE  */
  TYPENAME = 305,          /* TYPENAME  */
  TYPEDEF = 306,           /* TYPEDEF  */
  NAMESPACE = 307,         /* NAMESPACE  */
  USING = 308,             /* USING  */
  NEW = 309,               /* NEW  */
  DELETE = 310,            /* DELETE  */
  DEFAULT = 311,           /* DEFAULT  */
  STATIC_CAST = 312,       /* STATIC_CAST  */
  DYNAMIC_CAST = 313,      /* DYNAMIC_CAST  */
  CONST_CAST = 314,        /* CONST_CAST  */
  REINTERPRET_CAST = 315,  /* REINTERPRET_CAST  */
  OP_LSHIFT_EQ = 316,      /* OP_LSHIFT_EQ  */
  OP_RSHIFT_EQ = 317,      /* OP_RSHIFT_EQ  */
  OP_LSHIFT = 318,         /* OP_LSHIFT  */
  OP_RSHIFT_A = 319,       /* OP_RSHIFT_A  */
  OP_DOT_POINTER = 320,    /* OP_DOT_POINTER  */
  OP_ARROW_POINTER = 321,  /* OP_ARROW_POINTER  */
  OP_ARROW = 322,          /* OP_ARROW  */
  OP_INCR = 323,           /* OP_INCR  */
  OP_DECR = 324,           /* OP_DECR  */
  OP_PLUS_EQ = 325,        /* OP_PLUS_EQ  */
  OP_MINUS_EQ = 326,       /* OP_MINUS_EQ  */
  OP_TIMES_EQ = 327,       /* OP_TIMES_EQ  */
  OP_DIVIDE_EQ = 328,      /* OP_DIVIDE_EQ  */
  OP_REMAINDER_EQ = 329,   /* OP_REMAINDER_EQ  */
  OP_AND_EQ = 330,         /* OP_AND_EQ  */
  OP_OR_EQ = 331,          /* OP_OR_EQ  */
  OP_XOR_EQ = 332,         /* OP_XOR_EQ  */
  OP_LOGIC_AND = 333,      /* OP_LOGIC_AND  */
  OP_LOGIC_OR = 334,       /* OP_LOGIC_OR  */
  OP_LOGIC_EQ = 335,       /* OP_LOGIC_EQ  */
  OP_LOGIC_NEQ = 336,      /* OP_LOGIC_NEQ  */
  OP_LOGIC_LEQ = 337,      /* OP_LOGIC_LEQ  */
  OP_LOGIC_GEQ = 338,      /* OP_LOGIC_GEQ  */
  ELLIPSIS = 339,          /* ELLIPSIS  */
  DOUBLE_COLON = 340,      /* DOUBLE_COLON  */
  OTHER = 341,             /* OTHER  */
  AUTO = 342,              /* AUTO  */
  VOID = 343,              /* VOID  */
  BOOL = 344,              /* BOOL  */
  FLOAT = 345,             /* FLOAT  */
  DOUBLE = 346,            /* DOUBLE  */
  INT = 347,               /* INT  */
  SHORT = 348,             /* SHORT  */
  LONG = 349,              /* LONG  */
  CHAR = 350,              /* CHAR  */
  CHAR16_T = 351,          /* CHAR16_T  */
  CHAR32_T = 352,          /* CHAR32_T  */
  WCHAR_T = 353,           /* WCHAR_T  */
  SIGNED = 354,            /* SIGNED  */
  UNSIGNED = 355           /* UNSIGNED  */
};
typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if !defined YYSTYPE && !defined YYSTYPE_IS_DECLARED
union YYSTYPE
{

  const char* str;
  unsigned int integer;
};
typedef union YYSTYPE YYSTYPE;
#define YYSTYPE_IS_TRIVIAL 1
#define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

int yyparse(void);

/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                                   /* "end of file"  */
  YYSYMBOL_YYerror = 1,                                 /* error  */
  YYSYMBOL_YYUNDEF = 2,                                 /* "invalid token"  */
  YYSYMBOL_ID = 3,                                      /* ID  */
  YYSYMBOL_VTK_ID = 4,                                  /* VTK_ID  */
  YYSYMBOL_QT_ID = 5,                                   /* QT_ID  */
  YYSYMBOL_StdString = 6,                               /* StdString  */
  YYSYMBOL_OSTREAM = 7,                                 /* OSTREAM  */
  YYSYMBOL_ISTREAM = 8,                                 /* ISTREAM  */
  YYSYMBOL_LP = 9,                                      /* LP  */
  YYSYMBOL_LA = 10,                                     /* LA  */
  YYSYMBOL_STRING_LITERAL = 11,                         /* STRING_LITERAL  */
  YYSYMBOL_STRING_LITERAL_UD = 12,                      /* STRING_LITERAL_UD  */
  YYSYMBOL_INT_LITERAL = 13,                            /* INT_LITERAL  */
  YYSYMBOL_HEX_LITERAL = 14,                            /* HEX_LITERAL  */
  YYSYMBOL_BIN_LITERAL = 15,                            /* BIN_LITERAL  */
  YYSYMBOL_OCT_LITERAL = 16,                            /* OCT_LITERAL  */
  YYSYMBOL_FLOAT_LITERAL = 17,                          /* FLOAT_LITERAL  */
  YYSYMBOL_CHAR_LITERAL = 18,                           /* CHAR_LITERAL  */
  YYSYMBOL_ZERO = 19,                                   /* ZERO  */
  YYSYMBOL_NULLPTR = 20,                                /* NULLPTR  */
  YYSYMBOL_SSIZE_T = 21,                                /* SSIZE_T  */
  YYSYMBOL_SIZE_T = 22,                                 /* SIZE_T  */
  YYSYMBOL_NULLPTR_T = 23,                              /* NULLPTR_T  */
  YYSYMBOL_BEGIN_ATTRIB = 24,                           /* BEGIN_ATTRIB  */
  YYSYMBOL_STRUCT = 25,                                 /* STRUCT  */
  YYSYMBOL_CLASS = 26,                                  /* CLASS  */
  YYSYMBOL_UNION = 27,                                  /* UNION  */
  YYSYMBOL_ENUM = 28,                                   /* ENUM  */
  YYSYMBOL_PUBLIC = 29,                                 /* PUBLIC  */
  YYSYMBOL_PRIVATE = 30,                                /* PRIVATE  */
  YYSYMBOL_PROTECTED = 31,                              /* PROTECTED  */
  YYSYMBOL_CONST = 32,                                  /* CONST  */
  YYSYMBOL_VOLATILE = 33,                               /* VOLATILE  */
  YYSYMBOL_MUTABLE = 34,                                /* MUTABLE  */
  YYSYMBOL_STATIC = 35,                                 /* STATIC  */
  YYSYMBOL_THREAD_LOCAL = 36,                           /* THREAD_LOCAL  */
  YYSYMBOL_VIRTUAL = 37,                                /* VIRTUAL  */
  YYSYMBOL_EXPLICIT = 38,                               /* EXPLICIT  */
  YYSYMBOL_INLINE = 39,                                 /* INLINE  */
  YYSYMBOL_CONSTEXPR = 40,                              /* CONSTEXPR  */
  YYSYMBOL_FRIEND = 41,                                 /* FRIEND  */
  YYSYMBOL_EXTERN = 42,                                 /* EXTERN  */
  YYSYMBOL_OPERATOR = 43,                               /* OPERATOR  */
  YYSYMBOL_TEMPLATE = 44,                               /* TEMPLATE  */
  YYSYMBOL_THROW = 45,                                  /* THROW  */
  YYSYMBOL_TRY = 46,                                    /* TRY  */
  YYSYMBOL_CATCH = 47,                                  /* CATCH  */
  YYSYMBOL_NOEXCEPT = 48,                               /* NOEXCEPT  */
  YYSYMBOL_DECLTYPE = 49,                               /* DECLTYPE  */
  YYSYMBOL_TYPENAME = 50,                               /* TYPENAME  */
  YYSYMBOL_TYPEDEF = 51,                                /* TYPEDEF  */
  YYSYMBOL_NAMESPACE = 52,                              /* NAMESPACE  */
  YYSYMBOL_USING = 53,                                  /* USING  */
  YYSYMBOL_NEW = 54,                                    /* NEW  */
  YYSYMBOL_DELETE = 55,                                 /* DELETE  */
  YYSYMBOL_DEFAULT = 56,                                /* DEFAULT  */
  YYSYMBOL_STATIC_CAST = 57,                            /* STATIC_CAST  */
  YYSYMBOL_DYNAMIC_CAST = 58,                           /* DYNAMIC_CAST  */
  YYSYMBOL_CONST_CAST = 59,                             /* CONST_CAST  */
  YYSYMBOL_REINTERPRET_CAST = 60,                       /* REINTERPRET_CAST  */
  YYSYMBOL_OP_LSHIFT_EQ = 61,                           /* OP_LSHIFT_EQ  */
  YYSYMBOL_OP_RSHIFT_EQ = 62,                           /* OP_RSHIFT_EQ  */
  YYSYMBOL_OP_LSHIFT = 63,                              /* OP_LSHIFT  */
  YYSYMBOL_OP_RSHIFT_A = 64,                            /* OP_RSHIFT_A  */
  YYSYMBOL_OP_DOT_POINTER = 65,                         /* OP_DOT_POINTER  */
  YYSYMBOL_OP_ARROW_POINTER = 66,                       /* OP_ARROW_POINTER  */
  YYSYMBOL_OP_ARROW = 67,                               /* OP_ARROW  */
  YYSYMBOL_OP_INCR = 68,                                /* OP_INCR  */
  YYSYMBOL_OP_DECR = 69,                                /* OP_DECR  */
  YYSYMBOL_OP_PLUS_EQ = 70,                             /* OP_PLUS_EQ  */
  YYSYMBOL_OP_MINUS_EQ = 71,                            /* OP_MINUS_EQ  */
  YYSYMBOL_OP_TIMES_EQ = 72,                            /* OP_TIMES_EQ  */
  YYSYMBOL_OP_DIVIDE_EQ = 73,                           /* OP_DIVIDE_EQ  */
  YYSYMBOL_OP_REMAINDER_EQ = 74,                        /* OP_REMAINDER_EQ  */
  YYSYMBOL_OP_AND_EQ = 75,                              /* OP_AND_EQ  */
  YYSYMBOL_OP_OR_EQ = 76,                               /* OP_OR_EQ  */
  YYSYMBOL_OP_XOR_EQ = 77,                              /* OP_XOR_EQ  */
  YYSYMBOL_OP_LOGIC_AND = 78,                           /* OP_LOGIC_AND  */
  YYSYMBOL_OP_LOGIC_OR = 79,                            /* OP_LOGIC_OR  */
  YYSYMBOL_OP_LOGIC_EQ = 80,                            /* OP_LOGIC_EQ  */
  YYSYMBOL_OP_LOGIC_NEQ = 81,                           /* OP_LOGIC_NEQ  */
  YYSYMBOL_OP_LOGIC_LEQ = 82,                           /* OP_LOGIC_LEQ  */
  YYSYMBOL_OP_LOGIC_GEQ = 83,                           /* OP_LOGIC_GEQ  */
  YYSYMBOL_ELLIPSIS = 84,                               /* ELLIPSIS  */
  YYSYMBOL_DOUBLE_COLON = 85,                           /* DOUBLE_COLON  */
  YYSYMBOL_OTHER = 86,                                  /* OTHER  */
  YYSYMBOL_AUTO = 87,                                   /* AUTO  */
  YYSYMBOL_VOID = 88,                                   /* VOID  */
  YYSYMBOL_BOOL = 89,                                   /* BOOL  */
  YYSYMBOL_FLOAT = 90,                                  /* FLOAT  */
  YYSYMBOL_DOUBLE = 91,                                 /* DOUBLE  */
  YYSYMBOL_INT = 92,                                    /* INT  */
  YYSYMBOL_SHORT = 93,                                  /* SHORT  */
  YYSYMBOL_LONG = 94,                                   /* LONG  */
  YYSYMBOL_CHAR = 95,                                   /* CHAR  */
  YYSYMBOL_CHAR16_T = 96,                               /* CHAR16_T  */
  YYSYMBOL_CHAR32_T = 97,                               /* CHAR32_T  */
  YYSYMBOL_WCHAR_T = 98,                                /* WCHAR_T  */
  YYSYMBOL_SIGNED = 99,                                 /* SIGNED  */
  YYSYMBOL_UNSIGNED = 100,                              /* UNSIGNED  */
  YYSYMBOL_101_ = 101,                                  /* ';'  */
  YYSYMBOL_102_ = 102,                                  /* '{'  */
  YYSYMBOL_103_ = 103,                                  /* '}'  */
  YYSYMBOL_104_ = 104,                                  /* '='  */
  YYSYMBOL_105_ = 105,                                  /* ':'  */
  YYSYMBOL_106_ = 106,                                  /* ','  */
  YYSYMBOL_107_ = 107,                                  /* '('  */
  YYSYMBOL_108_ = 108,                                  /* ')'  */
  YYSYMBOL_109_ = 109,                                  /* '<'  */
  YYSYMBOL_110_ = 110,                                  /* '&'  */
  YYSYMBOL_111_ = 111,                                  /* '['  */
  YYSYMBOL_112_ = 112,                                  /* ']'  */
  YYSYMBOL_113_ = 113,                                  /* '~'  */
  YYSYMBOL_114_ = 114,                                  /* '*'  */
  YYSYMBOL_115_ = 115,                                  /* '>'  */
  YYSYMBOL_116_ = 116,                                  /* '%'  */
  YYSYMBOL_117_ = 117,                                  /* '/'  */
  YYSYMBOL_118_ = 118,                                  /* '-'  */
  YYSYMBOL_119_ = 119,                                  /* '+'  */
  YYSYMBOL_120_ = 120,                                  /* '!'  */
  YYSYMBOL_121_ = 121,                                  /* '|'  */
  YYSYMBOL_122_ = 122,                                  /* '^'  */
  YYSYMBOL_123_ = 123,                                  /* '.'  */
  YYSYMBOL_YYACCEPT = 124,                              /* $accept  */
  YYSYMBOL_translation_unit = 125,                      /* translation_unit  */
  YYSYMBOL_opt_declaration_seq = 126,                   /* opt_declaration_seq  */
  YYSYMBOL_127_1 = 127,                                 /* $@1  */
  YYSYMBOL_declaration = 128,                           /* declaration  */
  YYSYMBOL_template_declaration = 129,                  /* template_declaration  */
  YYSYMBOL_explicit_instantiation = 130,                /* explicit_instantiation  */
  YYSYMBOL_linkage_specification = 131,                 /* linkage_specification  */
  YYSYMBOL_namespace_definition = 132,                  /* namespace_definition  */
  YYSYMBOL_133_2 = 133,                                 /* $@2  */
  YYSYMBOL_namespace_alias_definition = 134,            /* namespace_alias_definition  */
  YYSYMBOL_forward_declaration = 135,                   /* forward_declaration  */
  YYSYMBOL_simple_forward_declaration = 136,            /* simple_forward_declaration  */
  YYSYMBOL_class_definition = 137,                      /* class_definition  */
  YYSYMBOL_class_specifier = 138,                       /* class_specifier  */
  YYSYMBOL_139_3 = 139,                                 /* $@3  */
  YYSYMBOL_class_head = 140,                            /* class_head  */
  YYSYMBOL_141_4 = 141,                                 /* $@4  */
  YYSYMBOL_142_5 = 142,                                 /* $@5  */
  YYSYMBOL_class_key = 143,                             /* class_key  */
  YYSYMBOL_class_head_name = 144,                       /* class_head_name  */
  YYSYMBOL_class_name = 145,                            /* class_name  */
  YYSYMBOL_opt_final = 146,                             /* opt_final  */
  YYSYMBOL_member_specification = 147,                  /* member_specification  */
  YYSYMBOL_148_6 = 148,                                 /* $@6  */
  YYSYMBOL_member_access_specifier = 149,               /* member_access_specifier  */
  YYSYMBOL_member_declaration = 150,                    /* member_declaration  */
  YYSYMBOL_template_member_declaration = 151,           /* template_member_declaration  */
  YYSYMBOL_friend_declaration = 152,                    /* friend_declaration  */
  YYSYMBOL_base_specifier_list = 153,                   /* base_specifier_list  */
  YYSYMBOL_base_specifier = 154,                        /* base_specifier  */
  YYSYMBOL_opt_virtual = 155,                           /* opt_virtual  */
  YYSYMBOL_opt_access_specifier = 156,                  /* opt_access_specifier  */
  YYSYMBOL_access_specifier = 157,                      /* access_specifier  */
  YYSYMBOL_opaque_enum_declaration = 158,               /* opaque_enum_declaration  */
  YYSYMBOL_enum_definition = 159,                       /* enum_definition  */
  YYSYMBOL_enum_specifier = 160,                        /* enum_specifier  */
  YYSYMBOL_161_7 = 161,                                 /* $@7  */
  YYSYMBOL_enum_head = 162,                             /* enum_head  */
  YYSYMBOL_enum_key = 163,                              /* enum_key  */
  YYSYMBOL_opt_enum_base = 164,                         /* opt_enum_base  */
  YYSYMBOL_165_8 = 165,                                 /* $@8  */
  YYSYMBOL_enumerator_list = 166,                       /* enumerator_list  */
  YYSYMBOL_enumerator_definition = 167,                 /* enumerator_definition  */
  YYSYMBOL_168_9 = 168,                                 /* $@9  */
  YYSYMBOL_nested_variable_initialization = 169,        /* nested_variable_initialization  */
  YYSYMBOL_ignored_initializer = 170,                   /* ignored_initializer  */
  YYSYMBOL_ignored_class = 171,                         /* ignored_class  */
  YYSYMBOL_ignored_class_body = 172,                    /* ignored_class_body  */
  YYSYMBOL_typedef_declaration = 173,                   /* typedef_declaration  */
  YYSYMBOL_basic_typedef_declaration = 174,             /* basic_typedef_declaration  */
  YYSYMBOL_typedef_declarator_list = 175,               /* typedef_declarator_list  */
  YYSYMBOL_typedef_declarator_list_cont = 176,          /* typedef_declarator_list_cont  */
  YYSYMBOL_typedef_declarator = 177,                    /* typedef_declarator  */
  YYSYMBOL_typedef_direct_declarator = 178,             /* typedef_direct_declarator  */
  YYSYMBOL_function_direct_declarator = 179,            /* function_direct_declarator  */
  YYSYMBOL_180_10 = 180,                                /* $@10  */
  YYSYMBOL_181_11 = 181,                                /* $@11  */
  YYSYMBOL_typedef_declarator_id = 182,                 /* typedef_declarator_id  */
  YYSYMBOL_using_declaration = 183,                     /* using_declaration  */
  YYSYMBOL_using_id = 184,                              /* using_id  */
  YYSYMBOL_using_directive = 185,                       /* using_directive  */
  YYSYMBOL_alias_declaration = 186,                     /* alias_declaration  */
  YYSYMBOL_187_12 = 187,                                /* $@12  */
  YYSYMBOL_template_head = 188,                         /* template_head  */
  YYSYMBOL_189_13 = 189,                                /* $@13  */
  YYSYMBOL_190_14 = 190,                                /* $@14  */
  YYSYMBOL_191_15 = 191,                                /* $@15  */
  YYSYMBOL_template_parameter_list = 192,               /* template_parameter_list  */
  YYSYMBOL_193_16 = 193,                                /* $@16  */
  YYSYMBOL_template_parameter = 194,                    /* template_parameter  */
  YYSYMBOL_195_17 = 195,                                /* $@17  */
  YYSYMBOL_196_18 = 196,                                /* $@18  */
  YYSYMBOL_197_19 = 197,                                /* $@19  */
  YYSYMBOL_198_20 = 198,                                /* $@20  */
  YYSYMBOL_199_21 = 199,                                /* $@21  */
  YYSYMBOL_200_22 = 200,                                /* $@22  */
  YYSYMBOL_opt_ellipsis = 201,                          /* opt_ellipsis  */
  YYSYMBOL_class_or_typename = 202,                     /* class_or_typename  */
  YYSYMBOL_opt_template_parameter_initializer = 203,    /* opt_template_parameter_initializer  */
  YYSYMBOL_template_parameter_initializer = 204,        /* template_parameter_initializer  */
  YYSYMBOL_205_23 = 205,                                /* $@23  */
  YYSYMBOL_template_parameter_value = 206,              /* template_parameter_value  */
  YYSYMBOL_function_definition = 207,                   /* function_definition  */
  YYSYMBOL_function_declaration = 208,                  /* function_declaration  */
  YYSYMBOL_nested_method_declaration = 209,             /* nested_method_declaration  */
  YYSYMBOL_nested_operator_declaration = 210,           /* nested_operator_declaration  */
  YYSYMBOL_method_definition = 211,                     /* method_definition  */
  YYSYMBOL_method_declaration = 212,                    /* method_declaration  */
  YYSYMBOL_operator_declaration = 213,                  /* operator_declaration  */
  YYSYMBOL_conversion_function = 214,                   /* conversion_function  */
  YYSYMBOL_215_24 = 215,                                /* $@24  */
  YYSYMBOL_216_25 = 216,                                /* $@25  */
  YYSYMBOL_conversion_function_id = 217,                /* conversion_function_id  */
  YYSYMBOL_operator_function_nr = 218,                  /* operator_function_nr  */
  YYSYMBOL_operator_function_sig = 219,                 /* operator_function_sig  */
  YYSYMBOL_220_26 = 220,                                /* $@26  */
  YYSYMBOL_operator_function_id = 221,                  /* operator_function_id  */
  YYSYMBOL_operator_sig = 222,                          /* operator_sig  */
  YYSYMBOL_function_nr = 223,                           /* function_nr  */
  YYSYMBOL_function_trailer_clause = 224,               /* function_trailer_clause  */
  YYSYMBOL_func_cv_qualifier_seq = 225,                 /* func_cv_qualifier_seq  */
  YYSYMBOL_func_cv_qualifier = 226,                     /* func_cv_qualifier  */
  YYSYMBOL_opt_noexcept_specifier = 227,                /* opt_noexcept_specifier  */
  YYSYMBOL_noexcept_sig = 228,                          /* noexcept_sig  */
  YYSYMBOL_opt_ref_qualifier = 229,                     /* opt_ref_qualifier  */
  YYSYMBOL_virt_specifier_seq = 230,                    /* virt_specifier_seq  */
  YYSYMBOL_virt_specifier = 231,                        /* virt_specifier  */
  YYSYMBOL_opt_body_as_trailer = 232,                   /* opt_body_as_trailer  */
  YYSYMBOL_opt_trailing_return_type = 233,              /* opt_trailing_return_type  */
  YYSYMBOL_trailing_return_type = 234,                  /* trailing_return_type  */
  YYSYMBOL_235_27 = 235,                                /* $@27  */
  YYSYMBOL_function_body = 236,                         /* function_body  */
  YYSYMBOL_function_try_block = 237,                    /* function_try_block  */
  YYSYMBOL_handler_seq = 238,                           /* handler_seq  */
  YYSYMBOL_function_sig = 239,                          /* function_sig  */
  YYSYMBOL_240_28 = 240,                                /* $@28  */
  YYSYMBOL_structor_declaration = 241,                  /* structor_declaration  */
  YYSYMBOL_242_29 = 242,                                /* $@29  */
  YYSYMBOL_243_30 = 243,                                /* $@30  */
  YYSYMBOL_structor_sig = 244,                          /* structor_sig  */
  YYSYMBOL_245_31 = 245,                                /* $@31  */
  YYSYMBOL_opt_ctor_initializer = 246,                  /* opt_ctor_initializer  */
  YYSYMBOL_mem_initializer_list = 247,                  /* mem_initializer_list  */
  YYSYMBOL_mem_initializer = 248,                       /* mem_initializer  */
  YYSYMBOL_parameter_declaration_clause = 249,          /* parameter_declaration_clause  */
  YYSYMBOL_250_32 = 250,                                /* $@32  */
  YYSYMBOL_parameter_list = 251,                        /* parameter_list  */
  YYSYMBOL_252_33 = 252,                                /* $@33  */
  YYSYMBOL_parameter_declaration = 253,                 /* parameter_declaration  */
  YYSYMBOL_254_34 = 254,                                /* $@34  */
  YYSYMBOL_255_35 = 255,                                /* $@35  */
  YYSYMBOL_opt_initializer = 256,                       /* opt_initializer  */
  YYSYMBOL_initializer = 257,                           /* initializer  */
  YYSYMBOL_258_36 = 258,                                /* $@36  */
  YYSYMBOL_259_37 = 259,                                /* $@37  */
  YYSYMBOL_260_38 = 260,                                /* $@38  */
  YYSYMBOL_constructor_args = 261,                      /* constructor_args  */
  YYSYMBOL_262_39 = 262,                                /* $@39  */
  YYSYMBOL_variable_declaration = 263,                  /* variable_declaration  */
  YYSYMBOL_init_declarator_id = 264,                    /* init_declarator_id  */
  YYSYMBOL_opt_declarator_list = 265,                   /* opt_declarator_list  */
  YYSYMBOL_declarator_list_cont = 266,                  /* declarator_list_cont  */
  YYSYMBOL_267_40 = 267,                                /* $@40  */
  YYSYMBOL_init_declarator = 268,                       /* init_declarator  */
  YYSYMBOL_opt_ptr_operator_seq = 269,                  /* opt_ptr_operator_seq  */
  YYSYMBOL_direct_abstract_declarator = 270,            /* direct_abstract_declarator  */
  YYSYMBOL_271_41 = 271,                                /* $@41  */
  YYSYMBOL_direct_declarator = 272,                     /* direct_declarator  */
  YYSYMBOL_273_42 = 273,                                /* $@42  */
  YYSYMBOL_lp_or_la = 274,                              /* lp_or_la  */
  YYSYMBOL_275_43 = 275,                                /* $@43  */
  YYSYMBOL_opt_array_or_parameters = 276,               /* opt_array_or_parameters  */
  YYSYMBOL_277_44 = 277,                                /* $@44  */
  YYSYMBOL_278_45 = 278,                                /* $@45  */
  YYSYMBOL_function_qualifiers = 279,                   /* function_qualifiers  */
  YYSYMBOL_abstract_declarator = 280,                   /* abstract_declarator  */
  YYSYMBOL_declarator = 281,                            /* declarator  */
  YYSYMBOL_opt_declarator_id = 282,                     /* opt_declarator_id  */
  YYSYMBOL_declarator_id = 283,                         /* declarator_id  */
  YYSYMBOL_bitfield_size = 284,                         /* bitfield_size  */
  YYSYMBOL_opt_array_decorator_seq = 285,               /* opt_array_decorator_seq  */
  YYSYMBOL_array_decorator_seq = 286,                   /* array_decorator_seq  */
  YYSYMBOL_287_46 = 287,                                /* $@46  */
  YYSYMBOL_array_decorator_seq_impl = 288,              /* array_decorator_seq_impl  */
  YYSYMBOL_array_decorator = 289,                       /* array_decorator  */
  YYSYMBOL_290_47 = 290,                                /* $@47  */
  YYSYMBOL_array_size_specifier = 291,                  /* array_size_specifier  */
  YYSYMBOL_292_48 = 292,                                /* $@48  */
  YYSYMBOL_id_expression = 293,                         /* id_expression  */
  YYSYMBOL_unqualified_id = 294,                        /* unqualified_id  */
  YYSYMBOL_qualified_id = 295,                          /* qualified_id  */
  YYSYMBOL_nested_name_specifier = 296,                 /* nested_name_specifier  */
  YYSYMBOL_297_49 = 297,                                /* $@49  */
  YYSYMBOL_tilde_sig = 298,                             /* tilde_sig  */
  YYSYMBOL_identifier_sig = 299,                        /* identifier_sig  */
  YYSYMBOL_scope_operator_sig = 300,                    /* scope_operator_sig  */
  YYSYMBOL_template_id = 301,                           /* template_id  */
  YYSYMBOL_302_50 = 302,                                /* $@50  */
  YYSYMBOL_decltype_specifier = 303,                    /* decltype_specifier  */
  YYSYMBOL_304_51 = 304,                                /* $@51  */
  YYSYMBOL_simple_id = 305,                             /* simple_id  */
  YYSYMBOL_identifier = 306,                            /* identifier  */
  YYSYMBOL_opt_decl_specifier_seq = 307,                /* opt_decl_specifier_seq  */
  YYSYMBOL_decl_specifier2 = 308,                       /* decl_specifier2  */
  YYSYMBOL_decl_specifier_seq = 309,                    /* decl_specifier_seq  */
  YYSYMBOL_decl_specifier = 310,                        /* decl_specifier  */
  YYSYMBOL_storage_class_specifier = 311,               /* storage_class_specifier  */
  YYSYMBOL_function_specifier = 312,                    /* function_specifier  */
  YYSYMBOL_cv_qualifier = 313,                          /* cv_qualifier  */
  YYSYMBOL_cv_qualifier_seq = 314,                      /* cv_qualifier_seq  */
  YYSYMBOL_store_type = 315,                            /* store_type  */
  YYSYMBOL_store_type_specifier = 316,                  /* store_type_specifier  */
  YYSYMBOL_317_52 = 317,                                /* $@52  */
  YYSYMBOL_318_53 = 318,                                /* $@53  */
  YYSYMBOL_type_specifier = 319,                        /* type_specifier  */
  YYSYMBOL_trailing_type_specifier = 320,               /* trailing_type_specifier  */
  YYSYMBOL_321_54 = 321,                                /* $@54  */
  YYSYMBOL_trailing_type_specifier_seq = 322,           /* trailing_type_specifier_seq  */
  YYSYMBOL_trailing_type_specifier_seq2 = 323,          /* trailing_type_specifier_seq2  */
  YYSYMBOL_324_55 = 324,                                /* $@55  */
  YYSYMBOL_325_56 = 325,                                /* $@56  */
  YYSYMBOL_tparam_type = 326,                           /* tparam_type  */
  YYSYMBOL_tparam_type_specifier2 = 327,                /* tparam_type_specifier2  */
  YYSYMBOL_328_57 = 328,                                /* $@57  */
  YYSYMBOL_329_58 = 329,                                /* $@58  */
  YYSYMBOL_tparam_type_specifier = 330,                 /* tparam_type_specifier  */
  YYSYMBOL_simple_type_specifier = 331,                 /* simple_type_specifier  */
  YYSYMBOL_type_name = 332,                             /* type_name  */
  YYSYMBOL_primitive_type = 333,                        /* primitive_type  */
  YYSYMBOL_ptr_operator_seq = 334,                      /* ptr_operator_seq  */
  YYSYMBOL_reference = 335,                             /* reference  */
  YYSYMBOL_rvalue_reference = 336,                      /* rvalue_reference  */
  YYSYMBOL_pointer = 337,                               /* pointer  */
  YYSYMBOL_338_59 = 338,                                /* $@59  */
  YYSYMBOL_ptr_cv_qualifier_seq = 339,                  /* ptr_cv_qualifier_seq  */
  YYSYMBOL_pointer_seq = 340,                           /* pointer_seq  */
  YYSYMBOL_decl_attribute_specifier_seq = 341,          /* decl_attribute_specifier_seq  */
  YYSYMBOL_342_60 = 342,                                /* $@60  */
  YYSYMBOL_id_attribute_specifier_seq = 343,            /* id_attribute_specifier_seq  */
  YYSYMBOL_344_61 = 344,                                /* $@61  */
  YYSYMBOL_ref_attribute_specifier_seq = 345,           /* ref_attribute_specifier_seq  */
  YYSYMBOL_346_62 = 346,                                /* $@62  */
  YYSYMBOL_func_attribute_specifier_seq = 347,          /* func_attribute_specifier_seq  */
  YYSYMBOL_348_63 = 348,                                /* $@63  */
  YYSYMBOL_array_attribute_specifier_seq = 349,         /* array_attribute_specifier_seq  */
  YYSYMBOL_350_64 = 350,                                /* $@64  */
  YYSYMBOL_class_attribute_specifier_seq = 351,         /* class_attribute_specifier_seq  */
  YYSYMBOL_352_65 = 352,                                /* $@65  */
  YYSYMBOL_attribute_specifier_seq = 353,               /* attribute_specifier_seq  */
  YYSYMBOL_attribute_specifier = 354,                   /* attribute_specifier  */
  YYSYMBOL_attribute_specifier_contents = 355,          /* attribute_specifier_contents  */
  YYSYMBOL_attribute_using_prefix = 356,                /* attribute_using_prefix  */
  YYSYMBOL_attribute_list = 357,                        /* attribute_list  */
  YYSYMBOL_attribute = 358,                             /* attribute  */
  YYSYMBOL_359_66 = 359,                                /* $@66  */
  YYSYMBOL_attribute_pack = 360,                        /* attribute_pack  */
  YYSYMBOL_attribute_sig = 361,                         /* attribute_sig  */
  YYSYMBOL_attribute_token = 362,                       /* attribute_token  */
  YYSYMBOL_operator_id = 363,                           /* operator_id  */
  YYSYMBOL_operator_id_no_delim = 364,                  /* operator_id_no_delim  */
  YYSYMBOL_keyword = 365,                               /* keyword  */
  YYSYMBOL_literal = 366,                               /* literal  */
  YYSYMBOL_constant_expression = 367,                   /* constant_expression  */
  YYSYMBOL_constant_expression_item = 368,              /* constant_expression_item  */
  YYSYMBOL_369_67 = 369,                                /* $@67  */
  YYSYMBOL_common_bracket_item = 370,                   /* common_bracket_item  */
  YYSYMBOL_common_bracket_item_no_scope_operator = 371, /* common_bracket_item_no_scope_operator  */
  YYSYMBOL_any_bracket_contents = 372,                  /* any_bracket_contents  */
  YYSYMBOL_bracket_pitem = 373,                         /* bracket_pitem  */
  YYSYMBOL_any_bracket_item = 374,                      /* any_bracket_item  */
  YYSYMBOL_braces_item = 375,                           /* braces_item  */
  YYSYMBOL_angle_bracket_contents = 376,                /* angle_bracket_contents  */
  YYSYMBOL_braces_contents = 377,                       /* braces_contents  */
  YYSYMBOL_angle_bracket_pitem = 378,                   /* angle_bracket_pitem  */
  YYSYMBOL_angle_bracket_item = 379,                    /* angle_bracket_item  */
  YYSYMBOL_angle_brackets_sig = 380,                    /* angle_brackets_sig  */
  YYSYMBOL_381_68 = 381,                                /* $@68  */
  YYSYMBOL_right_angle_bracket = 382,                   /* right_angle_bracket  */
  YYSYMBOL_brackets_sig = 383,                          /* brackets_sig  */
  YYSYMBOL_384_69 = 384,                                /* $@69  */
  YYSYMBOL_385_70 = 385,                                /* $@70  */
  YYSYMBOL_parentheses_sig = 386,                       /* parentheses_sig  */
  YYSYMBOL_387_71 = 387,                                /* $@71  */
  YYSYMBOL_388_72 = 388,                                /* $@72  */
  YYSYMBOL_389_73 = 389,                                /* $@73  */
  YYSYMBOL_braces_sig = 390,                            /* braces_sig  */
  YYSYMBOL_391_74 = 391,                                /* $@74  */
  YYSYMBOL_ignored_items = 392,                         /* ignored_items  */
  YYSYMBOL_ignored_expression = 393,                    /* ignored_expression  */
  YYSYMBOL_ignored_item = 394,                          /* ignored_item  */
  YYSYMBOL_ignored_item_no_semi = 395,                  /* ignored_item_no_semi  */
  YYSYMBOL_ignored_item_no_angle = 396,                 /* ignored_item_no_angle  */
  YYSYMBOL_ignored_braces = 397,                        /* ignored_braces  */
  YYSYMBOL_ignored_brackets = 398,                      /* ignored_brackets  */
  YYSYMBOL_ignored_parentheses = 399,                   /* ignored_parentheses  */
  YYSYMBOL_ignored_left_parenthesis = 400               /* ignored_left_parenthesis  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;

/* Default (constant) value used for initialization for null
   right-hand sides.  Unlike the standard yacc.c template, here we set
   the default value of $$ to a zeroed-out value.  Since the default
   value is undefined, this behavior is technically correct.  */
static YYSTYPE yyval_default;

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef short
#undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
#include <limits.h> /* INFRINGES ON USER NAME SPACE */
#if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#define YY_STDINT_H
#endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
#undef UINT_LEAST8_MAX
#undef UINT_LEAST16_MAX
#define UINT_LEAST8_MAX 255
#define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif
#ifndef YYPTRDIFF_T
#if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#define YYPTRDIFF_T __PTRDIFF_TYPE__
#define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
#elif defined PTRDIFF_MAX
#ifndef ptrdiff_t
#include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#endif
#define YYPTRDIFF_T ptrdiff_t
#define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
#else
#define YYPTRDIFF_T long
#define YYPTRDIFF_MAXIMUM LONG_MAX
#endif
#endif

#ifndef YYSIZE_T
#ifdef __SIZE_TYPE__
#define YYSIZE_T __SIZE_TYPE__
#elif defined size_t
#define YYSIZE_T size_t
#elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#define YYSIZE_T size_t
#else
#define YYSIZE_T unsigned
#endif
#endif

#define YYSIZE_MAXIMUM                                                                             \
  YY_CAST(YYPTRDIFF_T,                                                                             \
    (YYPTRDIFF_MAXIMUM < YY_CAST(YYSIZE_T, -1) ? YYPTRDIFF_MAXIMUM : YY_CAST(YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST(YYPTRDIFF_T, sizeof(X))

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

#ifdef __cplusplus
typedef bool yybool;
#define yytrue true
#define yyfalse false
#else
/* When we move to stdbool, get rid of the various casts to yybool.  */
typedef signed char yybool;
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
    YY_ASSERT(0);                                                                                  \
  } while (yyfalse)
#endif

#ifndef YY_ATTRIBUTE_PURE
#if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#define YY_ATTRIBUTE_PURE __attribute__((__pure__))
#else
#define YY_ATTRIBUTE_PURE
#endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
#if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#define YY_ATTRIBUTE_UNUSED __attribute__((__unused__))
#else
#define YY_ATTRIBUTE_UNUSED
#endif
#endif

/* The _Noreturn keyword of C11.  */
#ifndef _Noreturn
#if (defined __cplusplus &&                                                                        \
  ((201103 <= __cplusplus && !(__GNUC__ == 4 && __GNUC_MINOR__ == 7)) ||                           \
    (defined _MSC_VER && 1900 <= _MSC_VER)))
#define _Noreturn [[noreturn]]
#elif ((!defined __cplusplus || defined __clang__) &&                                              \
  (201112 <= (defined __STDC_VERSION__ ? __STDC_VERSION__ : 0) ||                                  \
    (!defined __STRICT_ANSI__ &&                                                                   \
      (4 < __GNUC__ + (7 <= __GNUC_MINOR__) ||                                                     \
        (defined __apple_build_version__ ? 6000000 <= __apple_build_version__                      \
                                         : 3 < __clang_major__ + (5 <= __clang_minor__))))))
/* _Noreturn works as-is.  */
#elif (2 < __GNUC__ + (8 <= __GNUC_MINOR__) || defined __clang__ || 0x5110 <= __SUNPRO_C)
#define _Noreturn __attribute__((__noreturn__))
#elif 1200 <= (defined _MSC_VER ? _MSC_VER : 0)
#define _Noreturn __declspec(noreturn)
#else
#define _Noreturn
#endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if 1
#define YY_USE(E) ((void)(E))
#else
#define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && !defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
#if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                                                        \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")
#else
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                                                        \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")             \
    _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#endif
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

#if defined __cplusplus && defined __GNUC__ && !defined __ICC && 6 <= __GNUC__
#define YY_IGNORE_USELESS_CAST_BEGIN                                                               \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuseless-cast\"")
#define YY_IGNORE_USELESS_CAST_END _Pragma("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
#define YY_IGNORE_USELESS_CAST_BEGIN
#define YY_IGNORE_USELESS_CAST_END
#endif

#define YY_ASSERT(E) ((void)(0 && (E)))

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL 3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST 6511

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 124
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 277
/* YYNRULES -- Number of rules.  */
#define YYNRULES 674
/* YYNSTATES -- Number of states.  */
#define YYNSTATES 1052
/* YYMAXRHS -- Maximum number of symbols on right-hand side of rule.  */
#define YYMAXRHS 8
/* YYMAXLEFT -- Maximum number of symbols to the left of a handle
   accessed by $0, $-1, etc., in any rule.  */
#define YYMAXLEFT 0

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK 355

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                                           \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? YY_CAST(yysymbol_kind_t, yytranslate[YYX]) : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] = { 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 120, 2, 2, 2, 116, 110, 2, 107, 108, 114, 119, 106, 118,
  123, 117, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 105, 101, 109, 104, 115, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 111, 2, 112, 122, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 102, 121, 103, 113, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
  35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
  59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
  83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100 };

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] = { 0, 1780, 1780, 1782, 1784, 1783, 1794, 1795, 1796, 1797,
  1798, 1799, 1800, 1801, 1802, 1803, 1804, 1805, 1806, 1807, 1808, 1809, 1810, 1813, 1814, 1815,
  1816, 1817, 1818, 1821, 1822, 1829, 1836, 1837, 1837, 1839, 1842, 1849, 1850, 1853, 1854, 1855,
  1858, 1859, 1862, 1862, 1877, 1876, 1882, 1888, 1887, 1892, 1898, 1899, 1900, 1903, 1905, 1907,
  1910, 1911, 1914, 1915, 1917, 1919, 1918, 1927, 1931, 1932, 1933, 1936, 1937, 1938, 1939, 1940,
  1941, 1942, 1943, 1944, 1945, 1946, 1947, 1948, 1949, 1952, 1953, 1954, 1955, 1956, 1957, 1960,
  1961, 1962, 1963, 1967, 1968, 1971, 1973, 1976, 1981, 1982, 1985, 1986, 1989, 1990, 1991, 2002,
  2003, 2004, 2008, 2009, 2013, 2013, 2026, 2033, 2042, 2043, 2044, 2047, 2048, 2048, 2052, 2053,
  2055, 2056, 2057, 2057, 2065, 2069, 2070, 2073, 2075, 2077, 2078, 2081, 2082, 2090, 2091, 2094,
  2095, 2097, 2099, 2101, 2105, 2107, 2108, 2111, 2114, 2115, 2118, 2119, 2118, 2123, 2165, 2168,
  2169, 2170, 2172, 2174, 2176, 2180, 2183, 2183, 2216, 2215, 2219, 2227, 2218, 2237, 2239, 2238,
  2243, 2245, 2243, 2247, 2249, 2247, 2251, 2254, 2251, 2265, 2266, 2269, 2270, 2272, 2273, 2276,
  2276, 2286, 2287, 2295, 2296, 2297, 2298, 2301, 2304, 2305, 2306, 2309, 2310, 2311, 2314, 2315,
  2316, 2320, 2321, 2322, 2323, 2326, 2327, 2328, 2332, 2337, 2331, 2349, 2353, 2364, 2363, 2372,
  2376, 2379, 2389, 2393, 2394, 2397, 2398, 2400, 2401, 2402, 2405, 2406, 2408, 2409, 2410, 2412,
  2413, 2416, 2429, 2430, 2431, 2432, 2439, 2440, 2443, 2443, 2451, 2452, 2453, 2456, 2458, 2459,
  2463, 2462, 2479, 2503, 2475, 2514, 2514, 2517, 2518, 2521, 2522, 2525, 2526, 2532, 2533, 2533,
  2536, 2537, 2537, 2539, 2541, 2545, 2547, 2545, 2571, 2572, 2575, 2575, 2577, 2577, 2579, 2579,
  2584, 2585, 2585, 2593, 2596, 2666, 2667, 2669, 2670, 2670, 2673, 2676, 2677, 2681, 2693, 2692,
  2714, 2716, 2716, 2737, 2737, 2739, 2743, 2744, 2745, 2744, 2750, 2752, 2753, 2754, 2755, 2756,
  2757, 2760, 2761, 2765, 2766, 2770, 2771, 2774, 2775, 2779, 2780, 2781, 2782, 2785, 2786, 2789,
  2789, 2792, 2793, 2796, 2796, 2800, 2801, 2801, 2808, 2809, 2812, 2813, 2814, 2815, 2816, 2819,
  2821, 2823, 2827, 2829, 2831, 2833, 2835, 2837, 2839, 2839, 2844, 2847, 2850, 2853, 2853, 2861,
  2861, 2870, 2871, 2872, 2873, 2874, 2875, 2876, 2877, 2878, 2885, 2886, 2887, 2888, 2889, 2890,
  2896, 2897, 2900, 2901, 2903, 2904, 2907, 2908, 2911, 2912, 2913, 2914, 2917, 2918, 2919, 2920,
  2921, 2925, 2926, 2927, 2930, 2931, 2934, 2935, 2943, 2946, 2946, 2948, 2948, 2952, 2953, 2955,
  2959, 2960, 2962, 2962, 2965, 2967, 2971, 2974, 2974, 2976, 2976, 2980, 2983, 2983, 2985, 2985,
  2989, 2990, 2992, 2994, 2996, 2998, 3000, 3004, 3005, 3008, 3009, 3010, 3011, 3012, 3013, 3014,
  3015, 3016, 3019, 3020, 3021, 3022, 3023, 3024, 3025, 3026, 3027, 3028, 3029, 3030, 3031, 3032,
  3052, 3053, 3054, 3055, 3058, 3062, 3066, 3066, 3070, 3071, 3086, 3087, 3112, 3112, 3116, 3116,
  3120, 3120, 3124, 3124, 3128, 3128, 3132, 3132, 3135, 3136, 3139, 3143, 3144, 3147, 3150, 3151,
  3152, 3153, 3156, 3156, 3160, 3161, 3164, 3165, 3168, 3169, 3176, 3177, 3178, 3179, 3180, 3181,
  3182, 3183, 3184, 3185, 3186, 3187, 3188, 3191, 3192, 3193, 3194, 3195, 3196, 3197, 3198, 3199,
  3200, 3201, 3202, 3203, 3204, 3205, 3206, 3207, 3208, 3209, 3210, 3211, 3212, 3213, 3214, 3215,
  3216, 3217, 3218, 3219, 3220, 3221, 3222, 3223, 3224, 3227, 3228, 3229, 3230, 3231, 3232, 3233,
  3234, 3235, 3236, 3237, 3238, 3239, 3240, 3241, 3242, 3243, 3244, 3245, 3246, 3247, 3248, 3249,
  3250, 3251, 3252, 3253, 3254, 3255, 3256, 3259, 3260, 3261, 3262, 3263, 3264, 3265, 3266, 3267,
  3268, 3275, 3276, 3279, 3280, 3281, 3282, 3282, 3283, 3286, 3287, 3290, 3291, 3292, 3293, 3329,
  3329, 3330, 3331, 3332, 3333, 3335, 3336, 3339, 3340, 3341, 3342, 3345, 3346, 3347, 3350, 3351,
  3353, 3354, 3356, 3357, 3360, 3361, 3364, 3365, 3366, 3370, 3369, 3383, 3384, 3387, 3387, 3389,
  3389, 3393, 3393, 3395, 3395, 3397, 3397, 3401, 3401, 3406, 3407, 3409, 3410, 3413, 3414, 3417,
  3418, 3421, 3422, 3423, 3424, 3425, 3426, 3427, 3428, 3428, 3428, 3428, 3428, 3429, 3430, 3431,
  3432, 3433, 3436, 3439, 3440, 3443, 3446, 3446, 3446 };
#endif

#define YYPACT_NINF (-867)
#define YYTABLE_NINF (-628)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] = { -867, 75, 93, -867, -867, 5294, -867, 346, 400, 404, 431,
  509, 519, 161, 207, 257, -867, -867, -867, 367, -867, -867, -867, -867, -867, -867, -867, 103,
  -867, 83, -867, 3480, -867, -867, 6178, 286, 1306, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, -867, -867, -867, -867, 9, -867, -867, -867, -867, -867, -867, 5884, -867,
  99, 99, 99, 99, -867, 36, 6178, -867, 73, -867, 113, 707, 1157, 120, 1587, 246, 282, -867, 117,
  5982, -867, -867, -867, -867, 72, 256, -867, -867, -867, -867, -867, 141, -867, -867, 472, 133,
  3843, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, -867, -867, 48, -867, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, 104, 1587, 74,
  139, 155, 169, 192, 194, -867, 232, -867, -867, -867, -867, -867, 1636, 120, 120, 6178, 72, -867,
  -867, -867, -867, -867, -867, -867, 184, 74, 139, 155, 169, 192, 194, -867, -867, -867, 1587,
  1587, 206, 260, -867, 707, 1587, 120, 120, 6397, 195, 5353, -867, 6397, -867, 1392, 263, 1587,
  -867, -867, -867, -867, -867, -867, 5884, -867, -867, 6080, 72, 306, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, 6178, -867, -867, -867, -867, -867, -867, 212, 343, 120, 120, 120, -867,
  -867, -867, -867, 117, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, 707, -867, -867, -867, -867, -867, -867, 1645, -867, 356, 68, -867, -867, -867, -867, -867,
  -867, -867, -867, 240, -867, -867, -867, 65, -867, 338, -867, -867, 1907, 2028, -867, -867, 237,
  -867, 2149, 2996, 2270, -867, -867, -867, -867, -867, -867, 1787, 5746, 1787, 1579, -867, -867,
  -867, -867, -867, -867, 568, -867, 2391, 574, 379, -867, 390, -867, 389, -867, -867, -867, 5174,
  707, -867, -867, 402, -867, 72, -867, -867, -867, -867, -867, -867, 107, -867, 6088, 769, 120,
  120, 141, 416, 1095, -867, -867, -867, 19, -867, 1587, 6080, 1645, 1587, 423, 2512, 429, 5942,
  472, -867, -867, -867, 104, -867, -867, -867, -867, -867, 6397, 5746, 6397, 1579, -867, -867,
  -867, -867, 439, -867, 491, -867, 1458, -867, 491, 432, -867, 707, 311, -867, -867, -867, 442,
  443, 568, -867, 441, 72, -867, -867, -867, -867, -867, -867, 5960, 1432, 440, 259, 447, -867, 472,
  -867, 451, 3117, -867, -867, 450, -867, -867, -867, -867, 135, -867, 6276, 270, 520, -867, -867,
  -867, -867, -867, -867, -867, -867, -867, 468, -867, 72, 67, 483, 280, 1787, 1787, 333, 292, -867,
  -867, -867, -867, 484, 120, -867, -867, -867, 141, 585, -867, 485, 492, 143, -867, -867, 487,
  -867, 493, -867, -867, -867, -867, -867, -867, 507, -867, -867, 59, 1344, -867, -867, 510, -867,
  -867, 120, 120, 6088, -867, -867, -867, -867, -867, -867, -867, 320, -867, -867, 6178, 513, -867,
  -867, 707, 518, -867, 95, -867, -867, 524, 543, -867, 120, -867, -867, -867, 429, 4569, 537, 126,
  546, 19, 5960, -867, 439, -867, -867, -867, -867, 38, -867, 545, -867, -867, -867, 533, 210, -867,
  -867, -867, -867, -867, 4811, -867, -867, 1472, -867, -867, 141, 439, 549, -867, 538, 447, 262,
  120, -867, 572, 104, 555, -867, -867, -867, -867, -867, 1587, 1587, 1587, -867, 120, 120, 6178,
  72, 256, -867, -867, -867, -867, 72, 270, -867, 3964, 4085, 4206, -867, 556, -867, -867, -867,
  562, 563, -867, 256, -867, 560, -867, 564, 6178, -867, 558, 561, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, 575, -867, -867, -867, 371, 576, -867, 646, 600, -867, -867, -867, -867,
  1095, 582, -867, -867, 347, 1587, 600, 600, 2633, -867, -867, 583, -867, -867, -867, 685, 141,
  584, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, 589, -867, -867, -867, 212, -867,
  -867, 533, -867, 627, -867, 587, 256, -867, 4690, -867, 4811, -867, -867, -867, -867, 233, -867,
  335, -867, -867, -867, -867, 472, -867, -867, -867, -867, 237, -867, -867, -867, -867, -867, 568,
  -867, -867, -867, -867, -867, 72, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, -867, -867, -867, 429, -867, 72, -867, -867, 5393, -867, 1587, -867, -867, -867, 1587, -867,
  1344, -867, -867, -867, -867, 595, -867, -867, -867, -867, -867, 491, 613, 6178, -867, -867, 306,
  -867, -867, -867, -867, -867, -867, 429, 592, -867, -867, -867, -867, -867, -867, 429, -867, 5053,
  -867, 3601, -867, -867, -867, -867, -867, -867, -867, -867, -867, 391, -867, 602, 68, 5960, 602,
  -867, 591, 604, -867, 208, 1432, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867, -867,
  -867, 5492, -867, 99, -867, -867, -867, 608, 343, 707, 5590, 72, 600, 1344, 600, 576, 4811, 3722,
  -867, 669, -867, -867, -867, 72, -867, 4327, 4569, 4448, 652, 612, 609, 4811, 614, -867, -867,
  -867, -867, -867, 4811, 429, 5960, -867, -867, -867, -867, -867, 615, 72, -867, 602, -867, -867,
  5688, -867, -867, -867, -867, 5492, -867, -867, 343, 5786, -867, -867, -867, -867, 707, 1645,
  -867, -867, -867, 4811, 125, -867, -867, 621, 619, -867, -867, -867, -867, -867, -867, -867, 4811,
  -867, 4811, 624, 4932, -867, -867, -867, -867, -867, -867, -867, 1727, 99, 5786, 600, 5786, 633,
  -867, -867, 634, 356, 76, -867, -867, 6374, 62, -867, -867, -867, 4932, -867, 399, 601, 1768,
  -867, -867, 1727, -867, -867, 1645, -867, 638, -867, -867, -867, -867, -867, 6374, -867, -867,
  256, -867, 141, -867, -867, -867, -867, -867, 125, 140, -867, -867, 222, -867, 1768, -867, 5550,
  -867, 2754, -867, -867, -867, 601, -867, -867, 2875, 3238, 359, 80, 5550, 235, -867, -867, -867,
  5960, -867, -867, -867, -867, 105, 359, 5960, 3359, -867, -867 };

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] = { 3, 0, 4, 1, 470, 0, 482, 438, 439, 440, 435, 436, 437, 442,
  443, 441, 53, 52, 54, 114, 398, 399, 390, 393, 394, 396, 397, 395, 389, 391, 218, 0, 361, 412, 0,
  0, 0, 358, 444, 445, 446, 447, 448, 453, 454, 455, 449, 450, 451, 452, 456, 457, 22, 356, 5, 19,
  20, 13, 11, 12, 9, 37, 17, 378, 44, 480, 10, 16, 378, 0, 480, 14, 135, 7, 6, 8, 0, 18, 0, 0, 0, 0,
  207, 0, 0, 15, 0, 338, 470, 0, 0, 0, 0, 470, 411, 340, 357, 0, 470, 386, 387, 388, 179, 293, 403,
  407, 410, 470, 470, 471, 116, 115, 0, 392, 0, 438, 439, 440, 435, 436, 437, 673, 674, 583, 584,
  578, 579, 580, 577, 581, 582, 585, 586, 442, 443, 441, 643, 550, 549, 551, 570, 553, 555, 554,
  556, 557, 558, 559, 562, 563, 561, 560, 566, 569, 552, 571, 572, 564, 548, 547, 568, 567, 523,
  524, 565, 575, 574, 573, 576, 525, 526, 527, 657, 528, 529, 530, 536, 537, 531, 532, 533, 534,
  535, 538, 539, 540, 541, 542, 543, 544, 545, 546, 655, 654, 667, 643, 661, 658, 662, 672, 164,
  520, 643, 519, 514, 660, 513, 515, 516, 517, 518, 521, 522, 659, 666, 665, 656, 663, 664, 645,
  651, 653, 652, 643, 0, 0, 438, 439, 440, 435, 436, 437, 395, 391, 378, 480, 378, 480, 470, 0, 470,
  411, 0, 179, 372, 374, 373, 377, 376, 375, 643, 33, 365, 363, 364, 368, 367, 366, 371, 370, 369,
  0, 0, 0, 472, 339, 0, 0, 341, 342, 293, 0, 51, 482, 293, 110, 117, 0, 0, 26, 38, 23, 480, 25, 27,
  0, 24, 28, 0, 179, 257, 246, 643, 189, 245, 191, 192, 190, 210, 480, 0, 213, 21, 415, 354, 197,
  195, 225, 345, 0, 341, 342, 343, 59, 344, 58, 0, 348, 346, 347, 349, 414, 350, 359, 378, 480, 378,
  480, 136, 208, 0, 470, 405, 384, 301, 303, 180, 0, 289, 274, 179, 474, 474, 474, 402, 294, 458,
  459, 468, 460, 378, 434, 433, 492, 483, 0, 3, 645, 0, 0, 630, 629, 170, 162, 0, 0, 0, 637, 639,
  635, 362, 470, 392, 293, 51, 293, 117, 345, 378, 378, 151, 147, 143, 0, 146, 0, 0, 0, 154, 0, 152,
  0, 482, 156, 155, 0, 0, 383, 382, 0, 289, 179, 470, 380, 381, 62, 40, 49, 408, 470, 0, 0, 59, 0,
  481, 0, 122, 106, 118, 113, 470, 472, 0, 0, 0, 0, 0, 0, 264, 0, 0, 229, 228, 476, 227, 255, 351,
  352, 353, 618, 293, 51, 293, 117, 198, 196, 385, 378, 466, 209, 221, 472, 0, 193, 221, 327, 472,
  0, 0, 276, 286, 275, 0, 0, 0, 317, 0, 179, 463, 482, 462, 464, 461, 469, 404, 0, 0, 492, 486, 489,
  0, 3, 4, 0, 648, 650, 0, 644, 647, 649, 668, 0, 167, 0, 0, 0, 470, 669, 30, 646, 671, 607, 607,
  607, 413, 0, 143, 179, 408, 0, 470, 293, 293, 0, 327, 472, 341, 342, 32, 0, 0, 3, 159, 160, 473,
  0, 511, 523, 524, 0, 507, 506, 0, 504, 0, 505, 217, 512, 158, 157, 42, 288, 292, 379, 63, 0, 61,
  39, 48, 57, 470, 59, 0, 0, 108, 365, 363, 364, 368, 367, 366, 0, 120, 472, 0, 112, 409, 470, 0,
  258, 259, 0, 643, 244, 0, 470, 408, 0, 233, 482, 226, 264, 0, 0, 408, 0, 470, 406, 400, 467, 302,
  223, 224, 214, 230, 222, 0, 219, 298, 328, 0, 321, 199, 194, 472, 285, 290, 0, 641, 279, 0, 299,
  318, 475, 466, 0, 153, 0, 485, 492, 498, 357, 494, 496, 4, 31, 29, 670, 168, 165, 0, 0, 0, 429,
  428, 427, 0, 179, 293, 422, 426, 181, 182, 179, 0, 163, 0, 0, 0, 138, 142, 145, 140, 112, 0, 0,
  137, 293, 148, 321, 36, 4, 0, 510, 0, 0, 509, 508, 500, 501, 66, 67, 68, 45, 470, 0, 102, 103,
  104, 100, 50, 93, 98, 179, 46, 55, 470, 111, 122, 123, 119, 105, 340, 0, 179, 179, 0, 211, 270,
  265, 266, 271, 355, 252, 477, 0, 633, 596, 625, 601, 626, 627, 631, 602, 606, 605, 600, 603, 604,
  623, 595, 624, 619, 622, 360, 597, 598, 599, 43, 41, 109, 112, 401, 232, 231, 225, 215, 333, 330,
  331, 0, 250, 0, 293, 594, 591, 592, 277, 587, 589, 590, 620, 0, 282, 304, 465, 487, 484, 491, 0,
  495, 493, 497, 35, 170, 470, 430, 431, 432, 424, 319, 171, 474, 421, 378, 174, 179, 612, 614, 615,
  638, 610, 611, 609, 613, 608, 640, 636, 139, 141, 144, 264, 34, 179, 502, 503, 0, 65, 0, 101, 470,
  99, 0, 95, 0, 56, 121, 124, 645, 0, 128, 260, 262, 261, 248, 221, 267, 0, 235, 234, 257, 256, 607,
  618, 607, 107, 476, 264, 336, 332, 324, 325, 326, 323, 322, 264, 291, 0, 588, 0, 283, 281, 305,
  300, 308, 499, 169, 166, 378, 304, 320, 183, 179, 423, 183, 177, 0, 0, 470, 391, 0, 82, 80, 71,
  77, 64, 79, 73, 72, 76, 74, 69, 70, 0, 78, 0, 204, 205, 75, 0, 338, 0, 0, 179, 179, 0, 179, 47, 0,
  127, 126, 247, 212, 269, 470, 179, 253, 0, 0, 0, 240, 0, 0, 0, 0, 593, 617, 642, 616, 621, 0, 264,
  425, 295, 185, 172, 184, 315, 0, 179, 175, 183, 149, 161, 0, 83, 85, 88, 86, 0, 84, 87, 0, 0, 200,
  81, 472, 206, 0, 0, 96, 94, 97, 125, 0, 268, 272, 236, 0, 628, 632, 242, 233, 241, 216, 478, 337,
  251, 284, 0, 0, 296, 316, 178, 309, 91, 480, 89, 0, 0, 0, 179, 0, 0, 472, 203, 0, 274, 0, 254,
  634, 0, 236, 334, 482, 306, 186, 187, 304, 150, 0, 480, 90, 0, 92, 480, 0, 201, 0, 643, 273, 239,
  237, 238, 0, 417, 243, 293, 220, 479, 309, 188, 297, 311, 310, 0, 314, 643, 645, 408, 131, 0, 480,
  0, 202, 0, 419, 378, 416, 476, 312, 313, 0, 0, 0, 60, 0, 408, 132, 249, 378, 418, 307, 645, 134,
  129, 60, 0, 420, 0, 130, 133 };

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] = { -867, -867, -305, -867, -867, 736, -54, -867, -867, -867,
  -867, -733, -72, 10, -29, -867, -867, -867, -867, 64, -368, -80, -662, -867, -867, -867, -867,
  -53, -52, -57, -131, -867, -867, 78, -41, -38, -25, -867, -867, 3, -371, -867, -867, 79, -867,
  -867, -867, -209, -505, -35, -77, -322, 264, 106, -867, -867, -867, -867, 255, -30, 293, -867, 27,
  -867, -3, -867, -867, -867, -867, -867, 5, -867, -867, -867, -867, -867, -867, 144, 134, -768,
  -867, -867, -867, 775, -867, -867, -867, -17, -143, 61, 47, -867, -867, -211, -395, -867, -867,
  -245, -236, -440, -419, -867, -867, 52, -867, -867, -170, -867, -194, -867, -867, -867, -68, -867,
  -867, -867, -867, -45, -867, -867, -867, -867, -31, -867, 97, -525, -867, -867, -867, -97, -867,
  -867, -169, -867, -867, -867, -867, -867, -867, 29, 408, -222, 410, -867, 70, -102, -597, -867,
  -198, -867, -540, -867, -795, -867, -867, -199, -867, -867, -867, -333, -867, -867, -356, -867,
  -867, 77, -867, -867, -867, 984, 774, 1003, 71, -867, -867, 291, 701, -5, -867, 33, -867, 196,
  -32, -50, -867, 26, 360, -867, -867, -391, -867, 91, 250, -867, -867, -78, -866, -867, -867, -867,
  -867, -867, -867, -867, -867, -867, -867, 324, 313, 234, -324, 470, -867, 475, -867, 214, -867,
  1047, -867, -409, -867, -318, -867, -778, -867, -867, -867, -43, -867, -266, -867, -867, -867,
  349, 211, -867, -867, -867, -867, -867, 156, 202, 56, -584, -714, -867, -113, -6, -462, -867, -7,
  -867, 12, -867, -756, -867, -556, -867, -463, -867, -867, -867, -185, -867, -867, -867, 374, -867,
  -159, -343, -867, -342, 43, -512, -867, -549, -867 };

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] = { 0, 1, 2, 4, 54, 279, 56, 57, 58, 387, 59, 60, 61, 281, 63,
  271, 64, 804, 545, 299, 408, 409, 548, 544, 674, 675, 865, 926, 927, 680, 681, 802, 798, 682, 66,
  67, 68, 416, 69, 282, 419, 564, 561, 562, 888, 283, 809, 967, 1020, 71, 72, 505, 513, 506, 380,
  381, 791, 964, 382, 73, 263, 74, 284, 662, 285, 496, 362, 765, 491, 764, 492, 493, 851, 494, 854,
  495, 921, 770, 643, 915, 916, 960, 986, 286, 78, 79, 80, 930, 875, 876, 82, 428, 815, 83, 449,
  450, 827, 451, 84, 453, 593, 594, 595, 433, 434, 735, 703, 819, 979, 952, 953, 981, 293, 294, 891,
  454, 835, 877, 820, 947, 307, 581, 426, 569, 570, 574, 575, 699, 894, 700, 817, 977, 460, 461,
  607, 462, 463, 752, 910, 287, 338, 399, 458, 743, 400, 401, 771, 988, 339, 754, 340, 448, 843,
  911, 1010, 989, 918, 466, 849, 455, 834, 598, 844, 600, 738, 739, 828, 902, 903, 683, 87, 238,
  239, 430, 90, 91, 92, 268, 439, 269, 224, 95, 96, 270, 402, 300, 98, 99, 100, 101, 589, 883, 103,
  350, 447, 104, 105, 225, 1006, 1007, 1027, 1040, 637, 638, 774, 848, 639, 106, 107, 108, 345, 346,
  347, 348, 614, 590, 349, 566, 6, 391, 392, 468, 469, 578, 579, 983, 984, 272, 273, 109, 354, 476,
  477, 478, 479, 480, 761, 622, 623, 536, 716, 717, 718, 747, 748, 837, 749, 720, 646, 784, 785,
  909, 582, 839, 721, 722, 750, 823, 363, 725, 824, 822, 726, 503, 501, 502, 727, 751, 358, 365,
  487, 488, 489, 220, 221, 222, 223 };

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] = { 93, 344, 76, 251, 280, 234, 508, 414, 70, 236, 312, 295,
  296, 297, 483, 62, 467, 603, 274, 332, 328, 394, 695, 499, 470, 471, 723, 276, 629, 240, 395, 97,
  75, 838, 85, 597, 359, 237, 94, 370, 647, 648, 596, 364, 306, 384, 775, 601, 900, 565, 514, 482,
  415, 509, 913, 393, 705, 588, 316, 694, 242, 576, 602, 863, 366, 818, 81, 241, 324, 65, 546, 240,
  326, 584, 219, 3, 89, 334, 335, 240, 355, 334, 335, 546, 310, 313, 920, 218, 670, 671, 672, 385,
  240, -2, 113, 1001, 102, 772, 235, 599, 327, 452, 288, 772, 121, 122, 659, 266, 546, 241, 546,
  275, 360, 367, 368, 1005, 733, 241, 475, 724, -117, -117, 311, 314, 418, 243, 524, 114, 603, 546,
  241, 1002, 1003, 427, 121, 122, 305, 81, 1026, 653, 65, 499, 465, 298, 329, 290, 341, 89, 734,
  121, 122, 539, 336, 963, 689, 112, 336, 357, 599, -372, 395, 325, 673, 361, 332, 353, 978, 289,
  330, -60, 218, -488, -60, 602, 302, 301, 624, -488, 342, 855, 810, 547, 343, -372, 373, 538, 375,
  216, 452, 654, 655, 965, 374, 1012, 376, 742, 857, 195, 732, 360, 291, 292, 199, 613, 987, 37,
  729, 666, 547, -60, 332, 369, -60, 378, -339, 731, 661, 379, 583, 372, 585, 452, 332, 588, -374,
  546, 323, 729, -60, 838, 1011, -60, 199, 217, 310, 356, 772, 240, 546, 422, -373, 628, 838, 372,
  838, 237, 337, 199, -374, 580, 361, 1042, 114, 772, -377, 917, 429, 431, 667, 324, 432, 310, -371,
  -173, -373, 215, 856, 412, -371, 612, 216, 507, 311, 507, 440, 241, 442, -376, -377, -375, 240,
  -176, 441, 240, 443, 445, 315, -173, 386, 244, 245, 246, 247, 248, 249, 240, 641, 405, 946, 311,
  474, -376, 901, -375, 943, 413, 235, 390, -370, 384, 904, 423, 772, 704, -370, 740, 217, 741, 241,
  956, 642, 241, 962, 547, -60, 310, 958, -60, 511, 512, 550, 37, 452, 241, 341, 329, 729, -60, 396,
  840, -60, 841, 723, 410, 214, 332, 81, -341, 215, 235, 342, 316, 325, -341, 343, 772, 89, 1034,
  -369, 330, 897, -153, 899, 311, -369, -488, 342, 37, -490, 412, 343, -488, 200, 1034, -490, 1047,
  289, 444, 516, 772, 309, 520, -117, -342, 1047, 418, 959, 383, 250, -342, 240, 310, 110, 111, 324,
  976, 892, 587, 316, 658, 676, 677, 678, -329, 723, 551, 520, 413, 457, 507, 507, 425, 605, 696,
  218, 218, 517, 606, 240, 413, 218, 218, 218, 687, 412, 577, 688, 214, 241, 311, 723, -372, 316,
  424, 656, 949, 412, 850, 762, 657, 481, 218, 842, 552, 413, 410, -329, -365, 621, 195, 435, 808,
  310, -365, 852, -372, 241, 331, -278, 516, 459, 1017, 413, -280, 1018, 889, 1030, 1031, 315, 719,
  329, 20, 21, 686, 413, 244, 245, 246, 247, 248, 249, 521, 553, 218, 464, -374, 301, 235, 634,
  -373, 311, 522, 644, 523, 330, 568, 632, 517, 842, 975, 410, -363, -329, 540, 404, -364, 842,
  -363, 404, -374, -329, -364, 410, -373, 216, 216, -377, 554, 1038, 636, 216, 216, 216, 591, 592,
  572, 635, 973, 919, 1045, 309, 1038, -368, 783, 783, 783, 773, -263, -368, 218, -377, 216, 1050,
  -329, 608, 464, 266, 499, 551, 611, 610, 537, 617, 619, 625, 507, 975, 309, 769, 240, 217, 217,
  627, 310, 277, 998, 217, 217, 217, 649, 315, 252, 253, 254, 255, 256, 257, 244, 245, 246, 247,
  248, 249, 216, 652, 660, 552, 217, 663, 258, 259, 260, 215, 215, -376, 668, 664, 241, 215, 215,
  215, 311, 331, 665, -375, 669, 315, 404, 537, 404, -367, 464, 563, 606, 691, 684, -367, 32, -376,
  215, -366, 309, 1019, 32, 693, 315, -366, 698, -375, 217, 403, 240, 697, 1013, 403, 1014, 936,
  315, 728, 216, 830, 831, 832, 833, 737, 452, 1015, 730, 331, 1016, 757, 383, 736, 1035, 756, 1037,
  760, 240, 763, 37, 331, 215, 657, 788, 789, 740, 753, 792, 1046, 241, 794, 214, 214, 795, 404,
  1033, 404, 214, 214, 214, 797, 53, 800, 801, 336, 217, 807, 309, 818, 816, 825, 499, 821, 974,
  741, 241, 890, 893, 214, 922, 1049, 309, 452, 452, -335, 923, 914, 499, 404, 935, 252, 253, 254,
  255, 256, 257, 944, 215, 1009, 951, 954, 955, 957, 961, 853, 978, 783, 621, 258, 259, 260, 980,
  985, 403, 997, 403, 999, 267, 316, 1024, 214, 55, 862, 866, 867, 404, 404, 887, 309, 315, 30, 304,
  218, 793, 941, 868, 32, 799, 869, 992, 452, 870, 651, 790, 692, 516, 871, 806, 615, 846, 650, 620,
  244, 245, 246, 247, 248, 249, 776, 874, 77, 969, 982, 331, 783, 719, 783, 826, 1008, 896, 811, 93,
  317, 873, 320, 322, 214, 945, 912, 70, 403, 280, 403, 517, 332, 328, 864, 934, 1000, 542, 541,
  1029, 240, 836, 690, 829, 715, 640, 32, 472, 53, 404, 882, 872, 473, 878, 618, 803, 755, 94, 758,
  905, 908, 0, 403, 898, 609, 937, 812, 813, 1025, 715, 0, 0, 309, 0, 0, 719, 0, 0, 241, 0, 216,
  324, 0, 332, 0, 326, 1032, 309, 65, 0, 0, 308, 0, 0, 318, 881, 93, 0, 929, 403, 403, 719, 0, 0,
  310, 93, 0, 0, 715, 715, 715, 925, 563, 327, 0, 0, 937, 0, 0, 0, 332, 0, 332, 714, 280, 0, 217,
  933, 928, 994, 931, 0, 324, 1028, 94, 316, 895, 0, 0, 0, 0, 0, 311, 94, 0, 0, 0, 93, 714, 968, 0,
  990, 93, 937, 929, 937, 93, 329, 215, 266, 0, 310, 516, 0, 0, 65, 0, 925, 0, 320, 322, 324, 881,
  218, 325, 403, 1021, 0, 970, 0, 1023, 938, 0, 972, 928, 94, 931, 714, 714, 714, 94, 0, 316, 93,
  94, 93, 0, 320, 322, 311, 517, 411, 0, 0, 240, 1041, 0, 1036, 329, 715, 0, 0, 0, 412, 0, 0, 966,
  86, 1048, 516, 0, 235, 993, 0, 331, 325, 0, 240, 881, 0, 0, 94, 938, 94, 0, 1004, 88, 214, 436,
  437, 438, 377, 241, 971, 412, 329, 412, 329, 264, 0, 0, 413, 0, 0, 0, 939, 940, 517, 942, 412,
  991, 0, 995, 0, 235, 241, 0, 265, 377, 318, 0, 938, 0, 216, 0, 0, 0, 0, 845, 5, 0, 0, 413, 0, 413,
  1022, 0, 0, 714, 410, 0, 0, 0, 0, 0, 0, 0, 413, 0, 715, 0, 715, 0, 411, 0, 0, 0, 0, 0, 218, 0, 0,
  0, 0, 0, 404, 218, 218, 0, 217, 410, 0, 410, 319, 0, 0, 555, 556, 557, 558, 559, 560, 308, 218, 0,
  410, 0, 0, 0, 456, 0, 320, 322, 996, 258, 259, 260, 0, 0, 0, 715, 215, 0, 0, 0, 0, 0, 0, 411, 715,
  715, 715, 0, 303, 0, 715, 0, 0, 321, 0, 411, 0, 715, 333, 404, 0, 0, 0, 714, 0, 714, 0, 351, 352,
  0, 515, 0, 0, 252, 253, 254, 255, 256, 257, 0, 0, 0, 0, 377, 0, 309, 0, 0, 0, 267, 715, 258, 259,
  260, 216, 0, 0, 0, 0, 315, 0, 216, 216, 715, 0, 715, 0, 715, 0, 0, 0, 456, 0, 0, 714, 214, 0, 0,
  216, 32, 0, 0, 371, 714, 714, 714, 403, 0, 0, 714, 315, 0, 315, 715, 320, 0, 714, 0, 0, 0, 217,
  265, 309, 0, 604, 315, 0, 217, 217, 0, 0, 515, 0, 0, 0, 331, 0, 0, 388, 389, 0, 0, 0, 0, 217, 437,
  438, 0, 0, 714, 0, 0, 215, 420, 0, 421, 0, 265, 265, 215, 215, 0, 714, 319, 714, 403, 714, 0, 404,
  0, 0, 702, 265, 0, 265, 404, 215, 0, 303, 0, 321, 0, 0, 0, 0, 0, 331, 0, 0, 0, 0, 0, 714, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 252, 253, 254, 255, 256, 257, 0, 0, 0, 0, 0, 0, 759, 0, 0, 0, 0, 0, 258, 259,
  260, 331, 0, 331, 0, 0, 320, 322, 0, 214, 0, 0, 0, 604, 0, 0, 214, 214, 252, 253, 254, 255, 256,
  257, 331, 0, 32, 261, 0, 262, 0, 510, 0, 214, 0, 331, 258, 259, 260, 0, 0, 0, 0, 0, 676, 677, 678,
  0, 0, 446, 265, 0, 679, 0, 0, 0, 0, 0, 0, 0, 519, 0, 37, 0, 32, 0, 252, 253, 254, 255, 256, 257,
  403, 0, 0, 0, 0, 567, 0, 403, 571, 0, 0, 0, 258, 259, 260, 0, 0, 504, 53, 0, 0, 0, 0, 0, 265, 0,
  586, 265, 37, 0, 0, 0, 0, 0, 252, 253, 254, 255, 256, 257, 32, 0, 0, 0, 0, 265, 0, 0, 543, 0, 0,
  0, 258, 259, 260, 549, 53, 0, 616, 0, 0, 0, 0, 0, 0, 0, 0, 0, 525, 526, 0, 0, 0, 0, 0, 0, 37, 265,
  0, 0, 32, 261, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 417, 0, 0, 633, 418, 0, 0, 0, 0,
  0, 0, 0, 53, 0, 0, 0, 0, 0, 0, 527, 528, 0, 0, 0, 37, 0, 169, 170, 171, 529, 173, 174, 175, 176,
  177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 645, 515, 53, 0, 0,
  265, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 267, 530, 0, 531, 532, 0, 533, 201, 534, 880, 203, 204,
  535, 206, 207, 208, 209, 210, 211, 212, 0, 252, 253, 254, 255, 256, 257, 0, 0, 252, 253, 254, 255,
  256, 257, 0, 685, 0, 0, 258, 259, 260, 0, 0, 0, 0, 0, 258, 259, 260, 0, 0, 0, 766, 767, 768, 0, 0,
  0, 0, 0, 701, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 265, 265, 265, 32, 0, 0, 252, 253, 254, 255, 256,
  257, 0, 0, 932, 252, 253, 254, 255, 256, 257, 0, 377, 932, 258, 259, 260, 0, 0, 0, 0, 37, 0, 258,
  259, 260, 0, 0, 0, 37, 0, 0, 0, 0, 571, 0, 0, 304, 0, 0, 0, 418, 32, 0, 0, 30, 0, 0, 411, 53, 0,
  32, 0, 265, 0, 932, 0, 53, 0, 0, 932, 0, 0, 0, 932, 0, 0, 0, 0, 377, 456, 0, 0, 0, 0, 0, 0, 0,
  796, 411, 0, 411, 0, 0, 0, 0, 0, 0, 0, 0, 805, 0, 0, 0, 411, 0, 0, 0, 0, 0, 0, 932, 0, 932, 0, 0,
  53, 0, 0, 16, 17, 18, 0, 0, 0, 53, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 604, 252, 253,
  254, 255, 256, 257, 0, 0, 0, 879, 0, 884, 0, 0, 0, 886, 0, 0, 258, 259, 260, 0, 0, 0, 0, 0, 0, 0,
  88, 0, 265, 0, 0, 0, 265, 0, 265, 0, 0, 0, 0, 847, 0, 0, 0, 0, 32, 0, 20, 21, 22, 23, 24, 25, 26,
  232, 28, 397, 233, 0, 0, 0, 0, 0, 0, 0, 0, 398, 0, 0, 0, 0, 0, 264, 0, 0, 885, 0, 0, 0, 0, 0, 37,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 265, 0, 341, 0, 0, 0, 406, 1017, 0, 0, 1018, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 265, 0, 0, 0, 0, 0, 0, 0, 0, 342, 0, 0, 0, 343, 0, 0, 0, 924, 0,
  0, 0, 0, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
  133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 701, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 484,
  195, 0, 196, 197, 198, 199, 0, 485, 201, 202, 486, 203, 204, 205, 206, 207, 208, 209, 210, 211,
  212, 213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 484,
  195, 490, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211,
  212, 213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 484,
  195, 0, 196, 197, 198, 199, 0, 485, 201, 202, 497, 203, 204, 205, 206, 207, 208, 209, 210, 211,
  212, 213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 484,
  195, 0, 196, 197, 198, 199, 500, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211,
  212, 213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 484,
  195, 518, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211,
  212, 213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 484,
  195, 573, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211,
  212, 213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 484,
  195, 814, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211,
  212, 213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 484,
  195, 1039, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211,
  212, 213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 484,
  195, 1043, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211,
  212, 213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 498,
  195, 0, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
  213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
  133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151,
  0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 626, 195, 0,
  196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 1044, 195,
  0, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 1051, 195,
  0, 196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 195, 0,
  196, 197, 198, 199, 0, 200, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 367, 368, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 706, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 777, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 0, 707, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 906, 608, 907,
  778, 709, 779, 369, 0, 781, 201, 712, 0, 203, 204, 782, 206, 207, 208, 209, 210, 211, 212, 713,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 195, 0,
  196, 197, 198, 199, 0, 485, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 195, 0,
  196, 197, 198, 199, 0, 0, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 115,
  116, 117, 118, 119, 120, 367, 368, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
  135, 706, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152,
  153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 777, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
  190, 191, 0, 707, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 608, 0, 778, 709,
  779, 369, 780, 781, 201, 712, 0, 203, 204, 782, 206, 207, 208, 209, 210, 211, 212, 713, 115, 116,
  117, 118, 119, 120, 367, 368, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
  706, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153,
  154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171,
  777, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
  191, 0, 707, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 608, 0, 778, 709, 779,
  369, 786, 781, 201, 712, 0, 203, 204, 782, 206, 207, 208, 209, 210, 211, 212, 713, 115, 116, 117,
  118, 119, 120, 367, 368, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 706,
  137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154,
  155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 777,
  173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0,
  707, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 608, 0, 778, 709, 779, 369,
  787, 781, 201, 712, 0, 203, 204, 782, 206, 207, 208, 209, 210, 211, 212, 713, 115, 116, 117, 118,
  119, 120, 367, 368, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 706, 137,
  138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0,
  0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 777, 173, 174,
  175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 707, 0,
  38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 608, 0, 778, 709, 779, 369, 0, 781,
  201, 712, 948, 203, 204, 782, 206, 207, 208, 209, 210, 211, 212, 713, 115, 116, 117, 118, 119,
  120, 367, 368, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 706, 137, 138,
  139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0,
  156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 777, 173, 174,
  175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 707, 0,
  38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 608, 0, 778, 709, 779, 369, 0, 781,
  201, 712, 950, 203, 204, 782, 206, 207, 208, 209, 210, 211, 212, 713, 115, 116, 117, 118, 119,
  120, 367, 368, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 706, 137, 138,
  139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0,
  156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 360, 173, 174,
  175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 707, 0,
  38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 608, 0, 708, 709, 710, 369, 0, 711,
  201, 712, 0, 203, 204, 361, 206, 207, 208, 209, 210, 211, 212, 713, -591, -591, -591, -591, -591,
  -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591,
  -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, 0, -591, -591, -591, -591, -591, -591,
  0, -591, -591, -591, -591, 0, 0, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591,
  -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591,
  -591, -591, -591, -591, -591, -591, -591, -591, -591, 0, -591, 0, -591, -591, -591, -591, -591,
  -591, -591, -591, -591, -591, -591, -591, -591, -591, 0, -591, 0, -627, -591, -591, -591, 0, -591,
  -591, -591, 0, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, -591, 115, 116, 117,
  118, 119, 120, 367, 368, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 706,
  137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154,
  155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 744,
  173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0,
  707, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 608, 0, 0, 709, 0, 369, 0, 745,
  201, 712, 0, 203, 204, 746, 206, 207, 208, 209, 210, 211, 212, 713, 115, 116, 117, 118, 119, 120,
  367, 368, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 706, 137, 138, 139,
  140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156,
  157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 0, 173, 174, 175, 176,
  177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 707, 0, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 608, 0, 0, 709, 0, 369, 0, 711, 201, 712, 0, 203,
  204, 0, 206, 207, 208, 209, 210, 211, 212, 713, 115, 116, 117, 118, 119, 120, 367, 368, 123, 124,
  125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 706, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 0, 173, 174, 175, 176, 177, 178, 179, 180,
  181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 0, 0, 38, 39, 40, 41, 42, 43, 44, 45,
  46, 47, 48, 49, 50, 51, 0, 608, 0, 0, 709, 0, 369, 0, 0, 201, 712, 0, 203, 204, 0, 206, 207, 208,
  209, 210, 211, 212, 713, 226, 227, 228, 229, 230, 231, 0, 0, 525, 526, 0, 0, 0, 0, 0, 0, 0, 0,
  133, 134, 135, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 0, 0, 0,
  0, 0, 0, 32, 33, 0, 0, 0, 527, 528, 0, 0, 0, 0, 0, 169, 170, 171, 529, 173, 174, 175, 176, 177,
  178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 0, 37, 0, 38, 39, 40, 41,
  42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 530, 0, 531, 532, 0, 533, 201, 534, 0, 203, 204,
  535, 206, 207, 208, 209, 210, 211, 212, 7, 8, 9, 10, 11, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 27, 28, 0, 29, 30, 31, 0, 0,
  0, 0, 32, 33, 34, 35, 36, 0, 0, 0, 0, 0, 0, 0, 0, 252, 253, 254, 255, 256, 257, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 258, 259, 260, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
  51, 52, 7, 8, 9, 10, 11, 12, 32, 0, 0, 0, 0, 53, 0, 0, 0, 0, 0, 0, 13, 14, 15, 0, 16, 17, 18, 19,
  0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 858, 859, 30, 31, 37, 0, 0, 0, 32, 33, 34, 0, 860,
  0, 0, 0, 0, 0, 0, 0, 406, 0, 0, 0, 407, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 861, 7, 8, 9, 10, 11, 12, 0, 0, 0,
  0, 0, 53, 0, 0, 0, 0, 0, 0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26,
  232, 28, 858, 233, 30, 277, 0, 0, 0, 0, 32, 33, 0, 0, 278, 0, 0, 0, 0, 0, 0, 0, 252, 253, 254,
  255, 256, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 258, 259, 260, 0, 0, 0, 37, 0, 38, 39, 40, 41,
  42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 7, 8, 9, 10, 11, 12, 32, 0, 0, 0, 0, 0, 53, 0, 0, 0, 0, 0,
  13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 0, 37, 0,
  0, 0, 32, 33, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1017, 0, 0, 1018, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 7, 8, 9,
  10, 11, 12, 0, 0, 0, 0, 0, 0, 53, 0, 0, 0, 0, 0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21,
  22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 277, 0, 0, 0, 0, 32, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  252, 253, 254, 255, 256, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 258, 259, 260, 0, 0, 0, 37, 0,
  38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 7, 8, 9, 10, 11, 12, 32, 0, 0, 0, 0, 0,
  53, 0, 0, 0, 0, 0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0,
  233, 30, 0, 37, 0, 0, 0, 32, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 407, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
  51, 226, 227, 228, 229, 230, 231, 0, 0, 0, 0, 0, 0, 53, 0, 0, 0, 0, 0, 133, 134, 135, 0, 16, 17,
  18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 277, 0, 0, 0, 0, 32, 33, 0, 0,
  278, 0, 0, 0, 0, 0, 0, 0, 252, 253, 254, 255, 256, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 258,
  259, 260, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 226, 227, 228,
  229, 230, 231, 32, 20, 21, 22, 23, 24, 25, 26, 232, 28, 397, 233, 133, 134, 135, 0, 16, 17, 18,
  19, 398, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 0, 37, 0, 0, 0, 32, 33, 34, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0,
  0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 226, 227, 228, 229, 230,
  231, 0, 0, 252, 253, 254, 255, 256, 257, 0, 0, 0, 0, 133, 134, 135, 0, 16, 17, 18, 19, 258, 259,
  260, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 0, 0, 0, 0, 0, 32, 33, 0, 304, 0, 0, 0, 0,
  32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38,
  39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 226, 227, 228, 229, 230, 231, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 133, 134, 135, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232,
  28, 0, 233, 0, 0, 0, 0, 0, 0, 32, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
  50, 51, 226, 227, 228, 229, 230, 231, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 133, 134, 135, 0, 630,
  0, 631, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 226, 227, 228, 229, 230, 231, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 133, 134, 135, 0, 0, 0, 0, 0, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26,
  232, 28, 0, 233, 0, 0, 0, 0, 0, 0, 32, 33, 0, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 397,
  233, 0, 0, 0, 0, 0, 0, 0, 0, 398, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44,
  45, 46, 47, 48, 49, 50, 51, 341, 0, 0, 0, 0, 0, 0, 0, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, -287, 0, 0, 0, 0, 0, 0, 0, 0, 342, 0, 0, 0, 343 };

static const yytype_int16 yycheck[] = { 5, 103, 5, 35, 76, 34, 374, 273, 5, 34, 90, 79, 80, 81, 357,
  5, 340, 457, 68, 97, 97, 266, 571, 365, 342, 343, 582, 70, 491, 34, 266, 5, 5, 747, 5, 454, 195,
  34, 5, 224, 502, 503, 451, 202, 89, 243, 643, 456, 826, 420, 383, 356, 274, 375, 849, 266, 581,
  448, 90, 571, 34, 429, 457, 796, 223, 3, 5, 34, 97, 5, 3, 76, 97, 441, 31, 0, 5, 9, 10, 84, 112,
  9, 10, 3, 89, 90, 854, 31, 29, 30, 31, 250, 97, 0, 11, 19, 5, 637, 34, 455, 97, 337, 76, 643, 9,
  10, 515, 36, 3, 76, 3, 102, 64, 9, 10, 981, 78, 84, 53, 582, 101, 102, 89, 90, 105, 34, 392, 44,
  568, 3, 97, 55, 56, 292, 9, 10, 89, 76, 1004, 510, 76, 483, 340, 107, 97, 46, 78, 76, 110, 9, 10,
  396, 84, 921, 563, 52, 84, 114, 514, 85, 396, 97, 103, 115, 242, 24, 104, 76, 97, 102, 114, 106,
  105, 568, 101, 84, 481, 112, 110, 776, 692, 101, 114, 109, 234, 396, 236, 31, 424, 511, 512, 924,
  235, 988, 237, 604, 793, 102, 589, 64, 101, 102, 107, 469, 960, 85, 101, 64, 101, 102, 288, 107,
  105, 242, 101, 586, 521, 242, 440, 11, 442, 457, 300, 614, 85, 3, 109, 101, 102, 943, 986, 105,
  107, 31, 239, 102, 776, 242, 3, 282, 85, 106, 956, 11, 958, 242, 102, 107, 109, 434, 115, 1029,
  44, 793, 85, 852, 299, 45, 115, 288, 48, 266, 101, 26, 109, 31, 791, 272, 107, 467, 114, 373, 239,
  375, 324, 242, 326, 85, 109, 85, 285, 44, 325, 288, 327, 330, 90, 50, 104, 3, 4, 5, 6, 7, 8, 300,
  26, 102, 895, 266, 350, 109, 827, 109, 888, 272, 242, 101, 101, 507, 835, 285, 852, 579, 107, 105,
  114, 107, 285, 903, 50, 288, 919, 101, 102, 330, 910, 105, 378, 379, 410, 85, 568, 300, 78, 288,
  101, 102, 267, 106, 105, 108, 898, 272, 31, 423, 285, 101, 114, 285, 110, 383, 288, 107, 114, 895,
  285, 1019, 101, 288, 822, 101, 824, 330, 107, 106, 110, 85, 106, 374, 114, 112, 109, 1035, 112,
  1037, 285, 330, 383, 919, 89, 386, 102, 101, 1046, 105, 911, 243, 102, 107, 395, 396, 25, 26, 423,
  944, 815, 447, 430, 107, 29, 30, 31, 111, 960, 410, 411, 374, 337, 511, 512, 105, 101, 572, 358,
  359, 383, 106, 423, 386, 364, 365, 366, 103, 429, 430, 106, 114, 395, 396, 986, 85, 464, 289, 101,
  898, 441, 770, 623, 106, 102, 385, 107, 410, 411, 374, 111, 101, 480, 102, 107, 104, 457, 107,
  772, 109, 423, 97, 102, 464, 104, 102, 429, 107, 105, 808, 1015, 1016, 272, 582, 423, 32, 33, 553,
  441, 3, 4, 5, 6, 7, 8, 102, 411, 427, 340, 85, 395, 423, 493, 85, 457, 101, 495, 104, 423, 424,
  493, 464, 107, 939, 429, 101, 111, 101, 270, 101, 107, 107, 274, 109, 111, 107, 441, 109, 358,
  359, 85, 101, 1023, 493, 364, 365, 366, 32, 33, 102, 493, 936, 852, 1034, 239, 1036, 101, 646,
  647, 648, 638, 108, 107, 483, 109, 385, 1047, 111, 102, 401, 475, 889, 553, 108, 107, 395, 112,
  106, 103, 657, 996, 266, 636, 564, 358, 359, 112, 568, 44, 974, 364, 365, 366, 101, 374, 3, 4, 5,
  6, 7, 8, 3, 4, 5, 6, 7, 8, 427, 101, 101, 553, 385, 3, 21, 22, 23, 358, 359, 85, 108, 111, 564,
  364, 365, 366, 568, 242, 111, 85, 112, 410, 373, 452, 375, 101, 467, 416, 106, 101, 105, 107, 49,
  109, 385, 101, 330, 990, 49, 106, 429, 107, 84, 109, 427, 270, 636, 108, 32, 274, 34, 881, 441,
  101, 483, 13, 14, 15, 16, 111, 881, 45, 101, 288, 48, 112, 507, 107, 1021, 105, 1023, 84, 662,
  103, 85, 300, 427, 106, 101, 101, 105, 610, 103, 1036, 636, 112, 358, 359, 112, 440, 1018, 442,
  364, 365, 366, 105, 113, 106, 37, 84, 483, 104, 396, 3, 106, 101, 1033, 108, 938, 107, 662, 101,
  84, 385, 108, 1043, 410, 938, 939, 112, 101, 104, 1049, 474, 101, 3, 4, 5, 6, 7, 8, 47, 483, 984,
  67, 108, 112, 108, 108, 774, 104, 839, 759, 21, 22, 23, 112, 108, 373, 101, 375, 102, 36, 770,
  101, 427, 5, 796, 796, 796, 511, 512, 804, 457, 553, 43, 44, 696, 662, 885, 796, 49, 679, 796,
  968, 996, 796, 507, 657, 568, 770, 796, 688, 475, 764, 506, 480, 3, 4, 5, 6, 7, 8, 644, 796, 5,
  924, 952, 423, 897, 898, 899, 735, 982, 820, 693, 796, 91, 796, 93, 94, 483, 894, 848, 796, 440,
  873, 442, 770, 882, 882, 796, 875, 977, 401, 400, 1010, 817, 743, 564, 738, 582, 493, 49, 349,
  113, 587, 796, 796, 349, 796, 477, 683, 614, 796, 619, 837, 839, -1, 474, 823, 462, 882, 694, 695,
  999, 607, -1, -1, 553, -1, -1, 960, -1, -1, 817, -1, 696, 882, -1, 933, -1, 882, 1017, 568, 796,
  -1, -1, 89, -1, -1, 92, 796, 873, -1, 873, 511, 512, 986, -1, -1, 881, 882, -1, -1, 646, 647, 648,
  873, 688, 882, -1, -1, 933, -1, -1, -1, 970, -1, 972, 582, 968, -1, 696, 873, 873, 969, 873, -1,
  933, 1007, 873, 939, 817, -1, -1, -1, -1, -1, 881, 882, -1, -1, -1, 924, 607, 924, -1, 966, 929,
  970, 929, 972, 933, 882, 696, 860, -1, 938, 939, -1, -1, 873, -1, 929, -1, 240, 241, 972, 873,
  889, 882, 587, 991, -1, 924, -1, 995, 882, -1, 929, 929, 924, 929, 646, 647, 648, 929, -1, 996,
  970, 933, 972, -1, 268, 269, 938, 939, 272, -1, -1, 981, 1027, -1, 1022, 933, 747, -1, -1, -1,
  990, -1, -1, 924, 5, 1040, 996, -1, 929, 968, -1, 636, 933, -1, 1004, 929, -1, -1, 970, 933, 972,
  -1, 981, 5, 696, 309, 310, 311, 239, 981, 924, 1021, 970, 1023, 972, 36, -1, -1, 990, -1, -1, -1,
  883, 884, 996, 886, 1036, 968, -1, 970, -1, 972, 1004, -1, 36, 266, 267, -1, 972, -1, 889, -1, -1,
  -1, -1, 759, 4, -1, -1, 1021, -1, 1023, 993, -1, -1, 747, 990, -1, -1, -1, -1, -1, -1, -1, 1036,
  -1, 837, -1, 839, -1, 374, -1, -1, -1, -1, -1, 1025, -1, -1, -1, -1, -1, 853, 1032, 1033, -1, 889,
  1021, -1, 1023, 92, -1, -1, 3, 4, 5, 6, 7, 8, 330, 1049, -1, 1036, -1, -1, -1, 337, -1, 412, 413,
  971, 21, 22, 23, -1, -1, -1, 888, 889, -1, -1, -1, -1, -1, -1, 429, 897, 898, 899, -1, 88, -1,
  903, -1, -1, 93, -1, 441, -1, 910, 98, 912, -1, -1, -1, 837, -1, 839, -1, 107, 108, -1, 383, -1,
  -1, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, 396, -1, 881, -1, -1, -1, 475, 943, 21, 22, 23, 1025, -1,
  -1, -1, -1, 990, -1, 1032, 1033, 956, -1, 958, -1, 960, -1, -1, -1, 424, -1, -1, 888, 889, -1, -1,
  1049, 49, -1, -1, 225, 897, 898, 899, 853, -1, -1, 903, 1021, -1, 1023, 986, 520, -1, 910, -1, -1,
  -1, 1025, 225, 938, -1, 457, 1036, -1, 1032, 1033, -1, -1, 464, -1, -1, -1, 882, -1, -1, 261, 262,
  -1, -1, -1, -1, 1049, 551, 552, -1, -1, 943, -1, -1, 1025, 276, -1, 278, -1, 261, 262, 1032, 1033,
  -1, 956, 267, 958, 912, 960, -1, 1041, -1, -1, 577, 276, -1, 278, 1048, 1049, -1, 238, -1, 240,
  -1, -1, -1, -1, -1, 933, -1, -1, -1, -1, -1, 986, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 4, 5, 6,
  7, 8, -1, -1, -1, -1, -1, -1, 620, -1, -1, -1, -1, -1, 21, 22, 23, 970, -1, 972, -1, -1, 634, 635,
  -1, 1025, -1, -1, -1, 568, -1, -1, 1032, 1033, 3, 4, 5, 6, 7, 8, 993, -1, 49, 50, -1, 52, -1, 376,
  -1, 1049, -1, 1004, 21, 22, 23, -1, -1, -1, -1, -1, 29, 30, 31, -1, -1, 331, 376, -1, 37, -1, -1,
  -1, -1, -1, -1, -1, 386, -1, 85, -1, 49, -1, 3, 4, 5, 6, 7, 8, 1041, -1, -1, -1, -1, 422, -1,
  1048, 425, -1, -1, -1, 21, 22, 23, -1, -1, 371, 113, -1, -1, -1, -1, -1, 422, -1, 443, 425, 85,
  -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, 49, -1, -1, -1, -1, 443, -1, -1, 402, -1, -1, -1, 21, 22,
  23, 409, 113, -1, 475, -1, -1, -1, -1, -1, -1, -1, -1, -1, 11, 12, -1, -1, -1, -1, -1, -1, 85,
  475, -1, -1, 49, 50, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 101, -1, -1, 493, 105, -1, -1, -1,
  -1, -1, -1, -1, 113, -1, -1, -1, -1, -1, -1, 54, 55, -1, -1, -1, 85, -1, 61, 62, 63, 64, 65, 66,
  67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, 496, 770, 113, -1, -1,
  545, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 860, 104, -1, 106, 107, -1, 109, 110, 111,
  796, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, -1, 3, 4, 5, 6, 7, 8, -1, -1, 3, 4, 5, 6,
  7, 8, -1, 550, -1, -1, 21, 22, 23, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, 630, 631, 632, -1,
  -1, -1, -1, -1, 575, -1, -1, -1, -1, -1, 49, -1, -1, -1, -1, 630, 631, 632, 49, -1, -1, 3, 4, 5,
  6, 7, 8, -1, -1, 873, 3, 4, 5, 6, 7, 8, -1, 881, 882, 21, 22, 23, -1, -1, -1, -1, 85, -1, 21, 22,
  23, -1, -1, -1, 85, -1, -1, -1, -1, 693, -1, -1, 44, -1, -1, -1, 105, 49, -1, -1, 43, -1, -1, 990,
  113, -1, 49, -1, 693, -1, 924, -1, 113, -1, -1, 929, -1, -1, -1, 933, -1, -1, -1, -1, 938, 939,
  -1, -1, -1, -1, -1, -1, -1, 674, 1021, -1, 1023, -1, -1, -1, -1, -1, -1, -1, -1, 686, -1, -1, -1,
  1036, -1, -1, -1, -1, -1, -1, 970, -1, 972, -1, -1, 113, -1, -1, 25, 26, 27, -1, -1, -1, 113, 32,
  33, 34, 35, 36, 37, 38, 39, 40, -1, 42, 996, 3, 4, 5, 6, 7, 8, -1, -1, -1, 796, -1, 798, -1, -1,
  -1, 802, -1, -1, 21, 22, 23, -1, -1, -1, -1, -1, -1, -1, 796, -1, 798, -1, -1, -1, 802, -1, 804,
  -1, -1, -1, -1, 765, -1, -1, -1, -1, 49, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, -1, -1,
  -1, -1, -1, -1, -1, -1, 51, -1, -1, -1, -1, -1, 860, -1, -1, 800, -1, -1, -1, -1, -1, 85, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, 860, -1, 78, -1, -1, -1, 101, 102, -1, -1, 105, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99, 100, 885, -1, -1, -1, -1, -1, -1, -1, -1, 110, -1, -1, -1, 114,
  -1, -1, -1, 858, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 894, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44,
  45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
  69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92,
  93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, 112, 113,
  114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40,
  -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
  65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110,
  111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36,
  37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
  85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104, 105, 106, 107,
  -1, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8,
  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
  -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
  82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104,
  105, 106, 107, 108, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3,
  4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
  30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53,
  54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77,
  78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101,
  102, 103, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121,
  122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50,
  51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
  75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
  99, 100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118,
  119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1,
  -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
  71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
  95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115,
  116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42,
  43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
  67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
  91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110, 111, -1,
  113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38,
  39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
  63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
  87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, -1,
  109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1,
  35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
  59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
  83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104, 105,
  106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5,
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102,
  -1, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
  123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
  76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
  100, 101, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
  72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, 101, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116,
  117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43,
  44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
  68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113,
  114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40,
  -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
  65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110,
  111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36,
  37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
  85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107,
  -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
  -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
  82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104,
  105, 106, 107, -1, -1, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4,
  5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102,
  -1, 104, 105, 106, 107, 108, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
  123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
  76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
  100, -1, 102, -1, 104, 105, 106, 107, 108, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
  72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, 108, 109, 110, 111, -1, 113, 114, 115, 116,
  117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43,
  44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
  68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, 112, 113,
  114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40,
  -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
  65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, -1, 109, 110,
  111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36,
  37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1,
  85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107,
  -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
  -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
  82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104,
  105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4,
  5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102,
  -1, -1, 105, -1, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
  123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, -1, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
  76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
  100, -1, 102, -1, -1, 105, -1, 107, -1, 109, 110, 111, -1, 113, 114, -1, 116, 117, 118, 119, 120,
  121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48,
  49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, -1, 65, 66, 67, 68, 69, 70, 71, 72,
  73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, -1, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
  97, 98, 99, 100, -1, 102, -1, -1, 105, -1, 107, -1, -1, 110, 111, -1, 113, 114, -1, 116, 117, 118,
  119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, -1, -1, 11, 12, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22,
  23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1, -1,
  -1, -1, 49, 50, -1, -1, -1, 54, 55, -1, -1, -1, -1, -1, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
  71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94,
  95, 96, 97, 98, 99, 100, -1, -1, -1, 104, -1, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116,
  117, 118, 119, 120, 121, 122, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44,
  -1, -1, -1, -1, 49, 50, 51, 52, 53, -1, -1, -1, -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94,
  95, 96, 97, 98, 99, 100, 101, 3, 4, 5, 6, 7, 8, 49, -1, -1, -1, -1, 113, -1, -1, -1, -1, -1, -1,
  21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
  85, -1, -1, -1, 49, 50, 51, -1, 53, -1, -1, -1, -1, -1, -1, -1, 101, -1, -1, -1, 105, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92,
  93, 94, 95, 96, 97, 98, 99, 100, 101, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, 113, -1, -1, -1, -1,
  -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
  43, 44, -1, -1, -1, -1, 49, 50, -1, -1, 53, -1, -1, -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92,
  93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, 49, -1, -1, -1, -1, -1, 113, -1, -1, -1, -1,
  -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, 43,
  -1, 85, -1, -1, -1, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 102, -1, -1, 105, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, 113, -1, -1, -1,
  -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42,
  43, 44, -1, -1, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92,
  93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, 49, -1, -1, -1, -1, -1, 113, -1, -1, -1, -1,
  -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, 43,
  -1, 85, -1, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 105, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, 113, -1, -1, -1,
  -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42,
  43, 44, -1, -1, -1, -1, 49, 50, -1, -1, 53, -1, -1, -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92,
  93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, 49, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
  21, 22, 23, -1, 25, 26, 27, 28, 51, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, 43, -1,
  85, -1, -1, -1, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 87, 88, 89, 90,
  91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, -1, -1, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92,
  93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, 21,
  22, 23, -1, 25, 26, 27, 28, 21, 22, 23, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, 43, -1, -1,
  -1, -1, -1, 49, 50, -1, 44, -1, -1, -1, -1, 49, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93,
  94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21,
  22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1,
  -1, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93,
  94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21,
  22, 23, -1, 25, -1, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1,
  -1, -1, -1, 49, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93,
  94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21,
  22, 23, -1, -1, -1, -1, -1, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1,
  -1, -1, -1, 49, 50, -1, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, -1, -1, -1, -1,
  -1, -1, -1, -1, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93,
  94, 95, 96, 97, 98, 99, 100, 78, -1, -1, -1, -1, -1, -1, -1, -1, 87, 88, 89, 90, 91, 92, 93, 94,
  95, 96, 97, 98, 99, 100, 101, -1, -1, -1, -1, -1, -1, -1, -1, 110, -1, -1, -1, 114 };

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] = { 0, 125, 126, 0, 127, 341, 342, 3, 4, 5, 6, 7, 8, 21, 22, 23,
  25, 26, 27, 28, 32, 33, 34, 35, 36, 37, 38, 39, 40, 42, 43, 44, 49, 50, 51, 52, 53, 85, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 113, 128, 129, 130, 131, 132, 134, 135, 136,
  137, 138, 140, 143, 158, 159, 160, 162, 163, 173, 174, 183, 185, 186, 188, 207, 208, 209, 210,
  213, 214, 217, 222, 263, 293, 294, 295, 296, 298, 299, 300, 301, 303, 305, 306, 309, 310, 311,
  312, 313, 315, 316, 319, 320, 331, 332, 333, 353, 25, 26, 52, 11, 44, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 35, 36,
  37, 38, 39, 40, 42, 43, 44, 45, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 102,
  104, 105, 106, 107, 109, 110, 111, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 332,
  333, 364, 365, 366, 396, 397, 398, 399, 400, 304, 321, 3, 4, 5, 6, 7, 8, 39, 42, 138, 143, 160,
  163, 295, 296, 301, 303, 309, 315, 3, 4, 5, 6, 7, 8, 102, 306, 3, 4, 5, 6, 7, 8, 21, 22, 23, 50,
  52, 184, 293, 295, 296, 300, 301, 303, 307, 139, 351, 352, 307, 102, 351, 44, 53, 129, 136, 137,
  163, 169, 186, 188, 207, 263, 309, 315, 46, 101, 102, 236, 237, 236, 236, 236, 107, 143, 309, 315,
  101, 341, 44, 214, 241, 244, 294, 299, 301, 303, 145, 301, 303, 305, 306, 300, 294, 295, 300, 341,
  300, 109, 138, 143, 160, 163, 174, 214, 296, 310, 319, 341, 9, 10, 84, 201, 264, 272, 274, 78,
  110, 114, 269, 334, 335, 336, 337, 340, 317, 341, 341, 24, 354, 306, 102, 396, 392, 392, 64, 115,
  190, 382, 392, 393, 392, 9, 10, 107, 386, 293, 11, 307, 351, 307, 351, 294, 138, 160, 178, 179,
  182, 201, 272, 392, 104, 133, 293, 293, 101, 343, 344, 217, 221, 222, 296, 41, 51, 265, 268, 269,
  308, 310, 333, 102, 101, 105, 144, 145, 296, 300, 301, 303, 353, 265, 161, 101, 105, 164, 293,
  293, 351, 309, 201, 105, 246, 392, 215, 351, 297, 45, 48, 227, 228, 107, 300, 300, 300, 302, 307,
  351, 307, 351, 214, 241, 341, 318, 275, 218, 219, 221, 222, 223, 239, 283, 294, 296, 266, 104,
  256, 257, 259, 260, 201, 272, 281, 334, 345, 346, 345, 345, 335, 337, 307, 53, 355, 356, 357, 358,
  359, 102, 126, 393, 101, 109, 112, 394, 395, 396, 103, 192, 194, 195, 197, 199, 189, 112, 101,
  395, 108, 388, 389, 387, 341, 175, 177, 269, 144, 175, 293, 307, 307, 176, 283, 294, 301, 303,
  103, 295, 301, 102, 101, 104, 353, 11, 12, 54, 55, 64, 104, 106, 107, 109, 111, 115, 363, 364,
  217, 221, 101, 266, 264, 341, 147, 142, 3, 101, 146, 341, 145, 301, 303, 296, 101, 3, 4, 5, 6, 7,
  8, 166, 167, 305, 165, 164, 341, 293, 296, 247, 248, 293, 102, 103, 249, 250, 144, 301, 347, 348,
  386, 245, 376, 265, 144, 265, 293, 307, 313, 314, 339, 32, 33, 224, 225, 226, 343, 224, 285, 286,
  287, 343, 218, 223, 294, 101, 106, 258, 102, 390, 107, 108, 272, 353, 338, 184, 293, 112, 357,
  106, 299, 306, 361, 362, 126, 103, 101, 112, 106, 382, 25, 27, 163, 295, 301, 303, 309, 326, 327,
  330, 331, 26, 50, 202, 188, 341, 372, 372, 372, 101, 176, 182, 101, 164, 175, 175, 101, 106, 107,
  343, 101, 126, 187, 3, 111, 111, 64, 115, 108, 112, 29, 30, 31, 103, 148, 149, 29, 30, 31, 37,
  153, 154, 157, 293, 105, 341, 145, 103, 106, 343, 316, 101, 305, 106, 397, 399, 392, 108, 84, 251,
  253, 341, 300, 230, 353, 249, 24, 85, 104, 105, 106, 109, 111, 123, 332, 333, 364, 365, 366, 370,
  371, 378, 379, 380, 382, 383, 386, 390, 101, 101, 101, 164, 313, 78, 110, 229, 107, 111, 288, 289,
  105, 107, 343, 267, 64, 109, 115, 367, 368, 370, 380, 391, 261, 366, 273, 339, 105, 112, 358, 300,
  84, 360, 386, 103, 193, 191, 293, 293, 293, 319, 201, 270, 274, 269, 328, 270, 202, 64, 104, 106,
  108, 109, 115, 370, 373, 374, 108, 108, 101, 101, 177, 180, 103, 315, 112, 112, 341, 105, 156,
  157, 106, 37, 155, 201, 141, 341, 167, 104, 104, 170, 397, 248, 201, 201, 103, 216, 106, 254, 3,
  231, 242, 108, 385, 381, 384, 101, 227, 220, 290, 289, 13, 14, 15, 16, 284, 240, 268, 369, 368,
  377, 106, 108, 107, 276, 286, 299, 194, 341, 329, 282, 283, 196, 345, 307, 198, 270, 249, 270, 41,
  42, 53, 101, 130, 135, 137, 150, 151, 152, 158, 159, 173, 183, 186, 188, 211, 212, 213, 241, 263,
  293, 294, 296, 309, 315, 293, 341, 293, 153, 168, 393, 101, 238, 224, 84, 252, 315, 246, 372, 376,
  372, 347, 249, 291, 292, 249, 371, 101, 103, 374, 375, 262, 277, 307, 276, 104, 203, 204, 270,
  280, 334, 203, 200, 108, 101, 341, 137, 151, 152, 186, 188, 211, 263, 294, 309, 236, 101, 221,
  241, 296, 201, 201, 154, 201, 367, 47, 253, 270, 243, 112, 382, 112, 67, 233, 234, 108, 112, 367,
  108, 367, 249, 205, 108, 270, 203, 181, 135, 143, 171, 188, 212, 309, 315, 309, 343, 221, 223,
  399, 255, 104, 232, 112, 235, 230, 349, 350, 108, 206, 378, 271, 279, 351, 143, 171, 309, 236,
  143, 201, 101, 343, 102, 256, 19, 55, 56, 309, 320, 322, 323, 232, 353, 278, 378, 276, 32, 34, 45,
  48, 102, 105, 144, 172, 351, 143, 351, 101, 392, 320, 324, 269, 279, 399, 399, 392, 393, 146, 144,
  351, 144, 172, 103, 325, 307, 347, 103, 101, 172, 144, 146, 307, 393, 172, 101 };

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] = { 0, 124, 125, 126, 127, 126, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 129, 129, 129, 129, 129, 129, 130, 130, 131,
  132, 133, 132, 132, 134, 135, 135, 136, 136, 136, 137, 137, 139, 138, 141, 140, 140, 142, 140,
  140, 143, 143, 143, 144, 144, 144, 145, 145, 146, 146, 147, 148, 147, 147, 149, 149, 149, 150,
  150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 151, 151, 151, 151, 151, 151,
  152, 152, 152, 152, 153, 153, 154, 154, 154, 155, 155, 156, 156, 157, 157, 157, 158, 158, 158,
  159, 159, 161, 160, 162, 162, 163, 163, 163, 164, 165, 164, 166, 166, 167, 167, 168, 167, 169,
  170, 170, 171, 171, 171, 171, 172, 172, 173, 173, 174, 174, 174, 174, 174, 175, 176, 176, 177,
  178, 178, 180, 181, 179, 182, 183, 184, 184, 184, 184, 184, 184, 185, 187, 186, 189, 188, 190,
  191, 188, 192, 193, 192, 195, 196, 194, 197, 198, 194, 199, 200, 194, 201, 201, 202, 202, 203,
  203, 205, 204, 206, 206, 207, 207, 207, 207, 208, 209, 209, 209, 210, 210, 210, 211, 211, 211,
  212, 212, 212, 212, 213, 213, 213, 215, 216, 214, 217, 218, 220, 219, 221, 222, 223, 224, 225,
  225, 226, 226, 227, 227, 227, 228, 228, 229, 229, 229, 230, 230, 231, 232, 232, 232, 232, 233,
  233, 235, 234, 236, 236, 236, 237, 238, 238, 240, 239, 242, 243, 241, 245, 244, 246, 246, 247,
  247, 248, 248, 249, 250, 249, 251, 252, 251, 251, 251, 254, 255, 253, 256, 256, 258, 257, 259,
  257, 260, 257, 261, 262, 261, 263, 264, 265, 265, 266, 267, 266, 268, 269, 269, 270, 271, 270,
  272, 273, 272, 275, 274, 274, 276, 277, 278, 276, 276, 279, 279, 279, 279, 279, 279, 280, 280,
  281, 281, 282, 282, 283, 283, 284, 284, 284, 284, 285, 285, 287, 286, 288, 288, 290, 289, 291,
  292, 291, 293, 293, 294, 294, 294, 294, 294, 295, 295, 295, 296, 296, 296, 296, 296, 296, 297,
  296, 298, 299, 300, 302, 301, 304, 303, 305, 305, 305, 305, 305, 305, 305, 305, 305, 306, 306,
  306, 306, 306, 306, 307, 307, 308, 308, 308, 308, 309, 309, 310, 310, 310, 310, 311, 311, 311,
  311, 311, 312, 312, 312, 313, 313, 314, 314, 315, 317, 316, 318, 316, 319, 319, 319, 320, 320,
  321, 320, 320, 320, 322, 324, 323, 325, 323, 326, 328, 327, 329, 327, 330, 330, 330, 330, 330,
  330, 330, 331, 331, 332, 332, 332, 332, 332, 332, 332, 332, 332, 333, 333, 333, 333, 333, 333,
  333, 333, 333, 333, 333, 333, 333, 333, 334, 334, 334, 334, 335, 336, 338, 337, 339, 339, 340,
  340, 342, 341, 344, 343, 346, 345, 348, 347, 350, 349, 352, 351, 353, 353, 354, 355, 355, 356,
  357, 357, 357, 357, 359, 358, 360, 360, 361, 361, 362, 362, 363, 363, 363, 363, 363, 363, 363,
  363, 363, 363, 363, 363, 363, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364,
  364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364, 364,
  364, 364, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365,
  365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 365, 366, 366, 366, 366, 366, 366,
  366, 366, 366, 366, 367, 367, 368, 368, 368, 369, 368, 368, 370, 370, 371, 371, 371, 371, 371,
  371, 371, 371, 371, 371, 372, 372, 373, 373, 373, 373, 374, 374, 374, 375, 375, 376, 376, 377,
  377, 378, 378, 379, 379, 379, 381, 380, 382, 382, 384, 383, 385, 383, 387, 386, 388, 386, 389,
  386, 391, 390, 392, 392, 393, 393, 394, 394, 395, 395, 396, 396, 396, 396, 396, 396, 396, 396,
  396, 396, 396, 396, 396, 396, 396, 396, 396, 397, 398, 398, 399, 400, 400, 400 };

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] = { 0, 2, 1, 0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
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
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 0, 4, 0, 1, 1, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0,
  2, 4, 2, 1, 3, 0, 1, 2, 3, 0, 3, 0, 1, 1, 2, 1, 3, 2, 2, 3, 3, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 0, 2, 0, 2, 1, 1, 1, 1, 1, 0, 4, 1, 1, 0, 4, 0, 5, 0, 4, 0, 4, 0, 4, 0, 4, 0, 2, 0, 2, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 4, 3, 1, 1, 1 };

/* YYDPREC[RULE-NUM] -- Dynamic precedence of rule #RULE-NUM (0 if none).  */
static const yytype_int8 yydprec[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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

/* YYMERGER[RULE-NUM] -- Index of merging function for rule #RULE-NUM.  */
static const yytype_int8 yymerger[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* YYCONFLP[YYPACT[STATE-NUM]] -- Pointer into YYCONFL of start of
   list of conflicting reductions corresponding to action entry for
   state STATE-NUM in yytable.  0 means no conflicts.  The list in
   yyconfl is terminated by a rule number of 0.  */
static const yytype_uint8 yyconflp[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 237, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 239, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 229, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 231, 0, 0, 0, 0, 0, 0, 0, 233, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 11, 13, 15, 17, 19,
  21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67,
  69, 0, 71, 73, 75, 77, 79, 81, 0, 83, 85, 87, 89, 0, 0, 91, 93, 95, 97, 99, 101, 103, 105, 107,
  109, 111, 113, 115, 117, 119, 121, 123, 125, 127, 129, 131, 133, 135, 137, 139, 141, 143, 145,
  147, 149, 151, 153, 155, 157, 159, 161, 0, 163, 0, 165, 167, 169, 171, 173, 175, 177, 179, 181,
  183, 185, 187, 189, 191, 0, 193, 0, 0, 195, 197, 199, 0, 201, 203, 205, 0, 207, 209, 211, 213,
  215, 217, 219, 221, 223, 225, 227, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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

/* YYCONFL[I] -- lists of conflicting rule numbers, each terminated by
   0, pointed into by YYCONFLP.  */
static const short yyconfl[] = { 0, 408, 0, 408, 0, 408, 0, 321, 0, 627, 0, 627, 0, 627, 0, 627, 0,
  627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0,
  627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0,
  627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0,
  627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0,
  627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0,
  627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0,
  627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0,
  627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0,
  627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 627, 0, 304, 0, 304, 0,
  304, 0, 314, 0, 408, 0, 408, 0 };

YYSTYPE yylval;

int yynerrs;
int yychar;

enum
{
  YYENOMEM = -2
};

typedef enum
{
  yyok,
  yyaccept,
  yyabort,
  yyerr,
  yynomem
} YYRESULTTAG;

#define YYCHK(YYE)                                                                                 \
  do                                                                                               \
  {                                                                                                \
    YYRESULTTAG yychk_flag = YYE;                                                                  \
    if (yychk_flag != yyok)                                                                        \
      return yychk_flag;                                                                           \
  } while (0)

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

/** State numbers. */
typedef int yy_state_t;

/** Rule numbers. */
typedef int yyRuleNum;

/** Item references. */
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
  /** Type tag for yysemantics.  If true, yyval applies, otherwise
   *  yyfirstVal applies.  */
  yybool yyresolved;
  /** Number of corresponding LALR(1) machine state.  */
  yy_state_t yylrState;
  /** Preceding state in this stack */
  yyGLRState* yypred;
  /** Source position of the last token produced by my symbol */
  YYPTRDIFF_T yyposn;
  union
  {
    /** First in a chain of alternative reductions producing the
     *  nonterminal corresponding to this state, threaded through
     *  yynext.  */
    yySemanticOption* yyfirstVal;
    /** Semantic value for this state.  */
    YYSTYPE yyval;
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
  YYPTRDIFF_T yysize;
  YYPTRDIFF_T yycapacity;
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
union yyGLRStackItem
{
  yyGLRState yystate;
  yySemanticOption yyoption;
};

struct yyGLRStack
{
  int yyerrState;

  YYJMP_BUF yyexception_buffer;
  yyGLRStackItem* yyitems;
  yyGLRStackItem* yynextFree;
  YYPTRDIFF_T yyspaceLeft;
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

/** Accessing symbol of state YYSTATE.  */
static yysymbol_kind_t yy_accessing_symbol(yy_state_t yystate)
{
  return YY_CAST(yysymbol_kind_t, yystos[yystate]);
}

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char* yysymbol_name(yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char* const yytname[] = { "\"end of file\"", "error", "\"invalid token\"", "ID",
  "VTK_ID", "QT_ID", "StdString", "OSTREAM", "ISTREAM", "LP", "LA", "STRING_LITERAL",
  "STRING_LITERAL_UD", "INT_LITERAL", "HEX_LITERAL", "BIN_LITERAL", "OCT_LITERAL", "FLOAT_LITERAL",
  "CHAR_LITERAL", "ZERO", "NULLPTR", "SSIZE_T", "SIZE_T", "NULLPTR_T", "BEGIN_ATTRIB", "STRUCT",
  "CLASS", "UNION", "ENUM", "PUBLIC", "PRIVATE", "PROTECTED", "CONST", "VOLATILE", "MUTABLE",
  "STATIC", "THREAD_LOCAL", "VIRTUAL", "EXPLICIT", "INLINE", "CONSTEXPR", "FRIEND", "EXTERN",
  "OPERATOR", "TEMPLATE", "THROW", "TRY", "CATCH", "NOEXCEPT", "DECLTYPE", "TYPENAME", "TYPEDEF",
  "NAMESPACE", "USING", "NEW", "DELETE", "DEFAULT", "STATIC_CAST", "DYNAMIC_CAST", "CONST_CAST",
  "REINTERPRET_CAST", "OP_LSHIFT_EQ", "OP_RSHIFT_EQ", "OP_LSHIFT", "OP_RSHIFT_A", "OP_DOT_POINTER",
  "OP_ARROW_POINTER", "OP_ARROW", "OP_INCR", "OP_DECR", "OP_PLUS_EQ", "OP_MINUS_EQ", "OP_TIMES_EQ",
  "OP_DIVIDE_EQ", "OP_REMAINDER_EQ", "OP_AND_EQ", "OP_OR_EQ", "OP_XOR_EQ", "OP_LOGIC_AND",
  "OP_LOGIC_OR", "OP_LOGIC_EQ", "OP_LOGIC_NEQ", "OP_LOGIC_LEQ", "OP_LOGIC_GEQ", "ELLIPSIS",
  "DOUBLE_COLON", "OTHER", "AUTO", "VOID", "BOOL", "FLOAT", "DOUBLE", "INT", "SHORT", "LONG",
  "CHAR", "CHAR16_T", "CHAR32_T", "WCHAR_T", "SIGNED", "UNSIGNED", "';'", "'{'", "'}'", "'='",
  "':'", "','", "'('", "')'", "'<'", "'&'", "'['", "']'", "'~'", "'*'", "'>'", "'%'", "'/'", "'-'",
  "'+'", "'!'", "'|'", "'^'", "'.'", "$accept", "translation_unit", "opt_declaration_seq", "$@1",
  "declaration", "template_declaration", "explicit_instantiation", "linkage_specification",
  "namespace_definition", "$@2", "namespace_alias_definition", "forward_declaration",
  "simple_forward_declaration", "class_definition", "class_specifier", "$@3", "class_head", "$@4",
  "$@5", "class_key", "class_head_name", "class_name", "opt_final", "member_specification", "$@6",
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

static const char* yysymbol_name(yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

/** Left-hand-side symbol for rule #YYRULE.  */
static yysymbol_kind_t yylhsNonterm(yyRuleNum yyrule)
{
  return YY_CAST(yysymbol_kind_t, yyr1[yyrule]);
}

#if YYDEBUG

#ifndef YYFPRINTF
#define YYFPRINTF fprintf
#endif

#define YY_FPRINTF YY_IGNORE_USELESS_CAST_BEGIN YY_FPRINTF_

#define YY_FPRINTF_(Args)                                                                          \
  do                                                                                               \
  {                                                                                                \
    YYFPRINTF Args;                                                                                \
    YY_IGNORE_USELESS_CAST_END                                                                     \
  } while (0)

#define YY_DPRINTF YY_IGNORE_USELESS_CAST_BEGIN YY_DPRINTF_

#define YY_DPRINTF_(Args)                                                                          \
  do                                                                                               \
  {                                                                                                \
    if (yydebug)                                                                                   \
      YYFPRINTF Args;                                                                              \
    YY_IGNORE_USELESS_CAST_END                                                                     \
  } while (0)

/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void yy_symbol_value_print(FILE* yyo, yysymbol_kind_t yykind, YYSTYPE const* const yyvaluep)
{
  FILE* yyoutput = yyo;
  YY_USE(yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE(yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}

/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void yy_symbol_print(FILE* yyo, yysymbol_kind_t yykind, YYSTYPE const* const yyvaluep)
{
  YYFPRINTF(yyo, "%s %s (", yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name(yykind));

  yy_symbol_value_print(yyo, yykind, yyvaluep);
  YYFPRINTF(yyo, ")");
}

#define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                                              \
  do                                                                                               \
  {                                                                                                \
    if (yydebug)                                                                                   \
    {                                                                                              \
      YY_FPRINTF((stderr, "%s ", Title));                                                          \
      yy_symbol_print(stderr, Kind, Value);                                                        \
      YY_FPRINTF((stderr, "\n"));                                                                  \
    }                                                                                              \
  } while (0)

static void yy_reduce_print(
  yybool yynormal, yyGLRStackItem* yyvsp, YYPTRDIFF_T yyk, yyRuleNum yyrule);

#define YY_REDUCE_PRINT(Args)                                                                      \
  do                                                                                               \
  {                                                                                                \
    if (yydebug)                                                                                   \
      yy_reduce_print Args;                                                                        \
  } while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;

static void yypstack(yyGLRStack* yystackp, YYPTRDIFF_T yyk) YY_ATTRIBUTE_UNUSED;
static void yypdumpstack(yyGLRStack* yystackp) YY_ATTRIBUTE_UNUSED;

#else /* !YYDEBUG */

#define YY_DPRINTF(Args)                                                                           \
  do                                                                                               \
  {                                                                                                \
  } while (yyfalse)
#define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
#define YY_REDUCE_PRINT(Args)

#endif /* !YYDEBUG */

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
      yyvsp[i].yystate.yysemantics.yyval = s->yysemantics.yyval;
    else
      /* The effect of using yyval or yyloc (in an immediate rule) is
       * undefined.  */
      yyvsp[i].yystate.yysemantics.yyfirstVal = YY_NULLPTR;
    s = yyvsp[i].yystate.yypred = s->yypred;
  }
}

/** If yychar is empty, fetch the next token.  */
static yysymbol_kind_t yygetToken(int* yycharp)
{
  yysymbol_kind_t yytoken;
  if (*yycharp == YYEMPTY)
  {
    YY_DPRINTF((stderr, "Reading a token\n"));
    *yycharp = yylex();
  }
  if (*yycharp <= YYEOF)
  {
    *yycharp = YYEOF;
    yytoken = YYSYMBOL_YYEOF;
    YY_DPRINTF((stderr, "Now at end of input.\n"));
  }
  else
  {
    yytoken = YYTRANSLATE(*yycharp);
    YY_SYMBOL_PRINT("Next token is", yytoken, &yylval, &yylloc);
  }
  return yytoken;
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
 *  yyerr for YYERROR, yyabort for YYABORT, yynomem for YYNOMEM.  */
static YYRESULTTAG yyuserAction(yyRuleNum yyrule, int yyrhslen, yyGLRStackItem* yyvsp,
  yyGLRStack* yystackp, YYPTRDIFF_T yyk, YYSTYPE* yyvalp)
{
  const yybool yynormal = yystackp->yysplitPoint == YY_NULLPTR;
  int yylow = 1;
  YY_USE(yyvalp);
  YY_USE(yyk);
  YY_USE(yyrhslen);
#undef yyerrok
#define yyerrok (yystackp->yyerrState = 0)
#undef YYACCEPT
#define YYACCEPT return yyaccept
#undef YYABORT
#define YYABORT return yyabort
#undef YYNOMEM
#define YYNOMEM return yynomem
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

  if (yyrhslen == 0)
    *yyvalp = yyval_default;
  else
    *yyvalp = yyvsp[YYFILL(1 - (int)yyrhslen)].yystate.yysemantics.yyval;
  /* If yyk == -1, we are running a deferred action on a temporary
     stack.  In that case, YY_REDUCE_PRINT must not play with YYFILL,
     so pretend the stack is "normal". */
  YY_REDUCE_PRINT((yynormal || yyk == -1, yyvsp, yyk, yyrule));
  switch (yyrule)
  {
    case 4: /* $@1: %empty  */
    {
      startSig();
      clearType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }
    break;

    case 33: /* $@2: %empty  */
    {
      pushNamespace(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 34: /* namespace_definition: NAMESPACE identifier $@2 '{' opt_declaration_seq '}'  */
    {
      popNamespace();
    }
    break;

    case 44: /* $@3: %empty  */
    {
      pushType();
    }
    break;

    case 45: /* class_specifier: class_head $@3 '{' member_specification '}'  */
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

    case 46: /* $@4: %empty  */
    {
      start_class((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-4)].yystate.yysemantics.yyval.integer));
      currentClass->IsFinal =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer);
    }
    break;

    case 48: /* class_head: class_key class_attribute_specifier_seq class_head_name opt_final  */
    {
      start_class((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-3)].yystate.yysemantics.yyval.integer));
      currentClass->IsFinal =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer);
    }
    break;

    case 49: /* $@5: %empty  */
    {
      start_class(NULL,
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 51: /* class_head: class_key class_attribute_specifier_seq  */
    {
      start_class(NULL,
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 52: /* class_key: CLASS  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 53: /* class_key: STRUCT  */
    {
      ((*yyvalp).integer) = 1;
    }
    break;

    case 54: /* class_key: UNION  */
    {
      ((*yyvalp).integer) = 2;
    }
    break;

    case 55: /* class_head_name: nested_name_specifier class_name decl_attribute_specifier_seq  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 56: /* class_head_name: scope_operator_sig nested_name_specifier class_name
                decl_attribute_specifier_seq  */
    {
      ((*yyvalp).str) = vtkstrcat3(
        "::", (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 60: /* opt_final: %empty  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 61: /* opt_final: ID  */
    {
      ((*yyvalp).integer) =
        (strcmp((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str),
           "final") == 0);
    }
    break;

    case 63: /* $@6: %empty  */
    {
      startSig();
      clearType();
      clearTypeId();
      clearTemplate();
      closeComment();
    }
    break;

    case 66: /* member_access_specifier: PUBLIC  */
    {
      access_level = VTK_ACCESS_PUBLIC;
    }
    break;

    case 67: /* member_access_specifier: PRIVATE  */
    {
      access_level = VTK_ACCESS_PRIVATE;
    }
    break;

    case 68: /* member_access_specifier: PROTECTED  */
    {
      access_level = VTK_ACCESS_PROTECTED;
    }
    break;

    case 92: /* friend_declaration: FRIEND decl_attribute_specifier_seq method_declaration
                function_body  */
    {
      output_friend_function();
    }
    break;

    case 95: /* base_specifier: id_expression opt_ellipsis  */
    {
      add_base_class(currentClass,
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        access_level,
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 96: /* base_specifier: VIRTUAL opt_access_specifier id_expression opt_ellipsis  */
    {
      add_base_class(currentClass,
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.integer),
        (VTK_PARSE_VIRTUAL |
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer)));
    }
    break;

    case 97: /* base_specifier: access_specifier opt_virtual id_expression opt_ellipsis  */
    {
      add_base_class(currentClass,
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-3)].yystate.yysemantics.yyval.integer),
        ((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.integer) |
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer)));
    }
    break;

    case 98: /* opt_virtual: %empty  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 99: /* opt_virtual: VIRTUAL  */
    {
      ((*yyvalp).integer) = VTK_PARSE_VIRTUAL;
    }
    break;

    case 100: /* opt_access_specifier: %empty  */
    {
      ((*yyvalp).integer) = access_level;
    }
    break;

    case 102: /* access_specifier: PUBLIC  */
    {
      ((*yyvalp).integer) = VTK_ACCESS_PUBLIC;
    }
    break;

    case 103: /* access_specifier: PRIVATE  */
    {
      ((*yyvalp).integer) = VTK_ACCESS_PRIVATE;
    }
    break;

    case 104: /* access_specifier: PROTECTED  */
    {
      ((*yyvalp).integer) = VTK_ACCESS_PROTECTED;
    }
    break;

    case 110: /* $@7: %empty  */
    {
      pushType();
    }
    break;

    case 111: /* enum_specifier: enum_head '{' $@7 enumerator_list '}'  */
    {
      popType();
      clearTypeId();
      if ((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-4)].yystate.yysemantics.yyval.str) != NULL)
      {
        setTypeId(
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-4)].yystate.yysemantics.yyval.str));
        setTypeBase(guess_id_type(
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-4)].yystate.yysemantics.yyval.str)));
      }
      end_enum();
    }
    break;

    case 112: /* enum_head: enum_key class_attribute_specifier_seq id_expression opt_enum_base  */
    {
      start_enum((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-3)].yystate.yysemantics.yyval.integer),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer),
        getTypeId());
      clearType();
      clearTypeId();
      ((*yyvalp).str) =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str);
    }
    break;

    case 113: /* enum_head: enum_key class_attribute_specifier_seq opt_enum_base  */
    {
      start_enum(NULL,
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.integer),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer),
        getTypeId());
      clearType();
      clearTypeId();
      ((*yyvalp).str) = NULL;
    }
    break;

    case 114: /* enum_key: ENUM  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 115: /* enum_key: ENUM CLASS  */
    {
      ((*yyvalp).integer) = 1;
    }
    break;

    case 116: /* enum_key: ENUM STRUCT  */
    {
      ((*yyvalp).integer) = 1;
    }
    break;

    case 117: /* opt_enum_base: %empty  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 118: /* $@8: %empty  */
    {
      pushType();
    }
    break;

    case 119: /* opt_enum_base: ':' $@8 store_type_specifier  */
    {
      ((*yyvalp).integer) = getType();
      popType();
    }
    break;

    case 123: /* enumerator_definition: simple_id id_attribute_specifier_seq  */
    {
      closeComment();
      add_enum(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str), NULL);
      clearType();
    }
    break;

    case 124: /* $@9: %empty  */
    {
      postSig("=");
      markSig();
      closeComment();
    }
    break;

    case 125: /* enumerator_definition: simple_id id_attribute_specifier_seq '=' $@9
                 constant_expression  */
    {
      chopSig();
      add_enum((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-4)].yystate.yysemantics.yyval.str),
        copySig());
      clearType();
    }
    break;

    case 148: /* $@10: %empty  */
    {
      pushFunction();
      postSig("(");
    }
    break;

    case 149: /* $@11: %empty  */
    {
      postSig(")");
    }
    break;

    case 150: /* function_direct_declarator: opt_ellipsis declarator_id '(' $@10
                 parameter_declaration_clause ')' $@11 function_qualifiers  */
    {
      ((*yyvalp).integer) = (VTK_PARSE_FUNCTION |
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-7)].yystate.yysemantics.yyval.integer));
      popFunction();
    }
    break;

    case 151: /* typedef_declarator_id: typedef_direct_declarator  */
    {
      ValueInfo* item = (ValueInfo*)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(item);
      item->ItemType = VTK_TYPEDEF_INFO;
      item->Access = access_level;

      handle_complex_type(item, getAttributes(), getType(),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer),
        getSig());

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

    case 152: /* using_declaration: USING using_id ';'  */
    {
      add_using(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str), 0);
    }
    break;

    case 154: /* using_id: TYPENAME id_expression  */
    {
      ((*yyvalp).str) =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str);
    }
    break;

    case 155: /* using_id: nested_name_specifier operator_function_id  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 156: /* using_id: nested_name_specifier conversion_function_id  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 157: /* using_id: scope_operator_sig nested_name_specifier operator_function_id  */
    {
      ((*yyvalp).str) = vtkstrcat3(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 158: /* using_id: scope_operator_sig nested_name_specifier conversion_function_id  */
    {
      ((*yyvalp).str) = vtkstrcat3(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 159: /* using_directive: USING NAMESPACE id_expression ';'  */
    {
      add_using(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str), 1);
    }
    break;

    case 160: /* $@12: %empty  */
    {
      markSig();
    }
    break;

    case 161: /* alias_declaration: USING id_expression id_attribute_specifier_seq '=' $@12
                 store_type direct_abstract_declarator ';'  */
    {
      ValueInfo* item = (ValueInfo*)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(item);
      item->ItemType = VTK_TYPEDEF_INFO;
      item->Access = access_level;

      handle_complex_type(item, getAttributes(), getType(),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer),
        copySig());

      item->Name =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-6)].yystate.yysemantics.yyval.str);
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

    case 162: /* $@13: %empty  */
    {
      postSig("template<> ");
      clearTypeId();
    }
    break;

    case 164: /* $@14: %empty  */
    {
      postSig("template<");
      pushType();
      clearType();
      clearTypeId();
      startTemplate();
    }
    break;

    case 165: /* $@15: %empty  */
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

    case 168: /* $@16: %empty  */
    {
      chopSig();
      postSig(", ");
      clearType();
      clearTypeId();
    }
    break;

    case 170: /* $@17: %empty  */
    {
      markSig();
    }
    break;

    case 171: /* $@18: %empty  */
    {
      add_template_parameter(getType(),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer),
        copySig());
    }
    break;

    case 173: /* $@19: %empty  */
    {
      markSig();
    }
    break;

    case 174: /* $@20: %empty  */
    {
      add_template_parameter(0,
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer),
        copySig());
    }
    break;

    case 176: /* $@21: %empty  */
    {
      pushTemplate();
      markSig();
    }
    break;

    case 177: /* $@22: %empty  */
    {
      int i;
      TemplateInfo* newTemplate = currentTemplate;
      popTemplate();
      add_template_parameter(0,
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer),
        copySig());
      i = currentTemplate->NumberOfParameters - 1;
      currentTemplate->Parameters[i]->Template = newTemplate;
    }
    break;

    case 179: /* opt_ellipsis: %empty  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 180: /* opt_ellipsis: ELLIPSIS  */
    {
      postSig("...");
      ((*yyvalp).integer) = VTK_PARSE_PACK;
    }
    break;

    case 181: /* class_or_typename: CLASS  */
    {
      postSig("class ");
    }
    break;

    case 182: /* class_or_typename: TYPENAME  */
    {
      postSig("typename ");
    }
    break;

    case 185: /* $@23: %empty  */
    {
      postSig("=");
      markSig();
    }
    break;

    case 186: /* template_parameter_initializer: '=' $@23 template_parameter_value  */
    {
      int i = currentTemplate->NumberOfParameters - 1;
      ValueInfo* param = currentTemplate->Parameters[i];
      chopSig();
      param->Value = copySig();
    }
    break;

    case 189: /* function_definition: function_declaration function_body  */
    {
      output_function();
    }
    break;

    case 190: /* function_definition: operator_declaration function_body  */
    {
      output_function();
    }
    break;

    case 191: /* function_definition: nested_method_declaration function_body  */
    {
      reject_function();
    }
    break;

    case 192: /* function_definition: nested_operator_declaration function_body  */
    {
      reject_function();
    }
    break;

    case 200: /* method_definition: method_declaration function_body  */
    {
      output_function();
    }
    break;

    case 210: /* $@24: %empty  */
    {
      postSig("(");
      currentFunction->IsExplicit = ((getType() & VTK_PARSE_EXPLICIT) != 0);
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }
    break;

    case 211: /* $@25: %empty  */
    {
      postSig(")");
    }
    break;

    case 212: /* conversion_function: conversion_function_id '(' $@24 parameter_declaration_clause
                 ')' $@25 function_trailer_clause  */
    {
      postSig(";");
      closeSig();
      currentFunction->IsOperator = 1;
      currentFunction->Name = "operator typecast";
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", "operator typecast");
    }
    break;

    case 213: /* conversion_function_id: operator_sig store_type  */
    {
      ((*yyvalp).str) = copySig();
    }
    break;

    case 214: /* operator_function_nr: operator_function_sig function_trailer_clause  */
    {
      postSig(";");
      closeSig();
      currentFunction->Name =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }
    break;

    case 215: /* $@26: %empty  */
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }
    break;

    case 216: /* operator_function_sig: operator_function_id id_attribute_specifier_seq '(' $@26
                 parameter_declaration_clause ')'  */
    {
      postSig(")");
    }
    break;

    case 217: /* operator_function_id: operator_sig operator_id  */
    {
      chopSig();
      ((*yyvalp).str) = vtkstrcat(copySig(),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 218: /* operator_sig: OPERATOR  */
    {
      markSig();
      postSig("operator ");
    }
    break;

    case 219: /* function_nr: function_sig function_trailer_clause  */
    {
      postSig(";");
      closeSig();
      currentFunction->Name =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

    case 223: /* func_cv_qualifier: CONST  */
    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }
    break;

    case 224: /* func_cv_qualifier: VOLATILE  */
    {
      postSig(" volatile");
    }
    break;

    case 226: /* opt_noexcept_specifier: noexcept_sig parentheses_sig  */
    {
      chopSig();
    }
    break;

    case 228: /* noexcept_sig: NOEXCEPT  */
    {
      postSig(" noexcept");
    }
    break;

    case 229: /* noexcept_sig: THROW  */
    {
      postSig(" throw");
    }
    break;

    case 231: /* opt_ref_qualifier: '&'  */
    {
      postSig("&");
    }
    break;

    case 232: /* opt_ref_qualifier: OP_LOGIC_AND  */
    {
      postSig("&&");
    }
    break;

    case 235: /* virt_specifier: ID  */
    {
      postSig(" ");
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      if (strcmp((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str),
            "final") == 0)
      {
        currentFunction->IsFinal = 1;
      }
      else if (strcmp(
                 (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str),
                 "override") == 0)
      {
        currentFunction->IsOverride = 1;
      }
    }
    break;

    case 237: /* opt_body_as_trailer: '=' DELETE  */
    {
      currentFunction->IsDeleted = 1;
    }
    break;

    case 239: /* opt_body_as_trailer: '=' ZERO  */
    {
      postSig(" = 0");
      currentFunction->IsPureVirtual = 1;
      if (currentClass)
      {
        currentClass->IsAbstract = 1;
      }
    }
    break;

    case 242: /* $@27: %empty  */
    {
      postSig(" -> ");
      clearType();
      clearTypeId();
    }
    break;

    case 243: /* trailing_return_type: OP_ARROW $@27 trailing_type_specifier_seq  */
    {
      chopSig();
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }
    break;

    case 250: /* $@28: %empty  */
    {
      postSig("(");
      set_return(currentFunction, getAttributes(), getType(), getTypeId(), 0);
    }
    break;

    case 251: /* function_sig: unqualified_id id_attribute_specifier_seq '(' $@28
                 parameter_declaration_clause ')'  */
    {
      postSig(")");
    }
    break;

    case 252: /* $@29: %empty  */
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
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-3)].yystate.yysemantics.yyval.str);
      currentFunction->Comment = vtkstrdup(getComment());
    }
    break;

    case 253: /* $@30: %empty  */
    {
      openSig();
    }
    break;

    case 254: /* structor_declaration: structor_sig opt_noexcept_specifier
                 func_attribute_specifier_seq virt_specifier_seq $@29 opt_ctor_initializer $@30
                 opt_body_as_trailer  */
    {
      postSig(";");
      closeSig();
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

    case 255: /* $@31: %empty  */
    {
      pushType();
      postSig("(");
    }
    break;

    case 256: /* structor_sig: unqualified_id '(' $@31 parameter_declaration_clause ')'  */
    {
      postSig(")");
      popType();
    }
    break;

    case 264: /* $@32: %empty  */
    {
      clearType();
      clearTypeId();
    }
    break;

    case 266: /* parameter_list: parameter_declaration  */
    {
      clearType();
      clearTypeId();
    }
    break;

    case 267: /* $@33: %empty  */
    {
      clearType();
      clearTypeId();
      postSig(", ");
    }
    break;

    case 269: /* parameter_list: parameter_list ',' ELLIPSIS  */
    {
      currentFunction->IsVariadic = 1;
      postSig(", ...");
    }
    break;

    case 270: /* parameter_list: ELLIPSIS  */
    {
      currentFunction->IsVariadic = 1;
      postSig("...");
    }
    break;

    case 271: /* $@34: %empty  */
    {
      markSig();
    }
    break;

    case 272: /* $@35: %empty  */
    {
      ValueInfo* param = (ValueInfo*)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(param);

      handle_complex_type(param, getAttributes(), getType(),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer),
        copySig());
      add_legacy_parameter(currentFunction, param);

      if (getVarName())
      {
        param->Name = getVarName();
      }

      vtkParse_AddParameterToFunction(currentFunction, param);
    }
    break;

    case 273: /* parameter_declaration: decl_attribute_specifier_seq $@34 store_type
                 direct_abstract_declarator $@35 opt_initializer  */
    {
      int i = currentFunction->NumberOfParameters - 1;
      if (getVarValue())
      {
        currentFunction->Parameters[i]->Value = getVarValue();
      }
    }
    break;

    case 274: /* opt_initializer: %empty  */
    {
      clearVarValue();
    }
    break;

    case 276: /* $@36: %empty  */
    {
      postSig("=");
      clearVarValue();
      markSig();
    }
    break;

    case 277: /* initializer: '=' $@36 constant_expression  */
    {
      chopSig();
      setVarValue(copySig());
    }
    break;

    case 278: /* $@37: %empty  */
    {
      clearVarValue();
      markSig();
    }
    break;

    case 279: /* initializer: $@37 braces_sig  */
    {
      chopSig();
      setVarValue(copySig());
    }
    break;

    case 280: /* $@38: %empty  */
    {
      clearVarValue();
      markSig();
      postSig("(");
    }
    break;

    case 281: /* initializer: $@38 '(' constructor_args ')'  */
    {
      chopSig();
      postSig(")");
      setVarValue(copySig());
    }
    break;

    case 282: /* constructor_args: literal  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 283: /* $@39: %empty  */
    {
      postSig(", ");
    }
    break;

    case 286: /* init_declarator_id: direct_declarator opt_initializer  */
    {
      unsigned int attributes = getAttributes();
      unsigned int type = getType();
      ValueInfo* var = (ValueInfo*)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(var);
      var->ItemType = VTK_VARIABLE_INFO;
      var->Access = access_level;

      handle_complex_type(var, attributes, type,
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer),
        getSig());

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

    case 290: /* $@40: %empty  */
    {
      postSig(", ");
    }
    break;

    case 293: /* opt_ptr_operator_seq: %empty  */
    {
      setTypePtr(0);
    }
    break;

    case 294: /* opt_ptr_operator_seq: ptr_operator_seq  */
    {
      setTypePtr(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 295: /* direct_abstract_declarator: opt_ellipsis opt_declarator_id opt_array_or_parameters
               */
    {
      if ((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer) ==
        VTK_PARSE_FUNCTION)
      {
        ((*yyvalp).integer) = (VTK_PARSE_FUNCTION_PTR |
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.integer));
      }
      else
      {
        ((*yyvalp).integer) =
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.integer);
      }
    }
    break;

    case 296: /* $@41: %empty  */
    {
      postSig(")");
    }
    break;

    case 297: /* direct_abstract_declarator: lp_or_la ref_attribute_specifier_seq
                 abstract_declarator ')' $@41 opt_array_or_parameters  */
    {
      const char* scope = getScope();
      unsigned int parens = add_indirection(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-5)].yystate.yysemantics.yyval.integer),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-3)].yystate.yysemantics.yyval.integer));
      if ((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer) ==
        VTK_PARSE_FUNCTION)
      {
        if (scope)
        {
          scope = vtkstrndup(scope, strlen(scope) - 2);
        }
        getFunction()->Class = scope;
        ((*yyvalp).integer) = (parens | VTK_PARSE_FUNCTION);
      }
      else if ((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)]
                   .yystate.yysemantics.yyval.integer) == VTK_PARSE_ARRAY)
      {
        ((*yyvalp).integer) = add_indirection_to_array(parens);
      }
    }
    break;

    case 298: /* direct_declarator: opt_ellipsis declarator_id opt_array_decorator_seq  */
    {
      ((*yyvalp).integer) =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.integer);
    }
    break;

    case 299: /* $@42: %empty  */
    {
      postSig(")");
    }
    break;

    case 300: /* direct_declarator: lp_or_la declarator ')' $@42 opt_array_or_parameters  */
    {
      const char* scope = getScope();
      unsigned int parens = add_indirection(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-4)].yystate.yysemantics.yyval.integer),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-3)].yystate.yysemantics.yyval.integer));
      if ((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer) ==
        VTK_PARSE_FUNCTION)
      {
        if (scope)
        {
          scope = vtkstrndup(scope, strlen(scope) - 2);
        }
        getFunction()->Class = scope;
        ((*yyvalp).integer) = (parens | VTK_PARSE_FUNCTION);
      }
      else if ((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)]
                   .yystate.yysemantics.yyval.integer) == VTK_PARSE_ARRAY)
      {
        ((*yyvalp).integer) = add_indirection_to_array(parens);
      }
    }
    break;

    case 301: /* $@43: %empty  */
    {
      postSig("(");
      scopeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig("*");
    }
    break;

    case 302: /* lp_or_la: LP $@43 ptr_cv_qualifier_seq  */
    {
      ((*yyvalp).integer) =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer);
    }
    break;

    case 303: /* lp_or_la: LA  */
    {
      postSig("(");
      scopeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig("&");
      ((*yyvalp).integer) = VTK_PARSE_REF;
    }
    break;

    case 304: /* opt_array_or_parameters: %empty  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 305: /* $@44: %empty  */
    {
      pushFunction();
      postSig("(");
    }
    break;

    case 306: /* $@45: %empty  */
    {
      postSig(")");
    }
    break;

    case 307: /* opt_array_or_parameters: '(' $@44 parameter_declaration_clause ')' $@45
                 function_qualifiers func_attribute_specifier_seq  */
    {
      ((*yyvalp).integer) = VTK_PARSE_FUNCTION;
      popFunction();
    }
    break;

    case 308: /* opt_array_or_parameters: array_decorator_seq  */
    {
      ((*yyvalp).integer) = VTK_PARSE_ARRAY;
    }
    break;

    case 311: /* function_qualifiers: function_qualifiers CONST  */
    {
      currentFunction->IsConst = 1;
    }
    break;

    case 316: /* abstract_declarator: ptr_operator_seq direct_abstract_declarator  */
    {
      ((*yyvalp).integer) = add_indirection(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 318: /* declarator: ptr_operator_seq direct_declarator  */
    {
      ((*yyvalp).integer) = add_indirection(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 319: /* opt_declarator_id: %empty  */
    {
      clearVarName();
      chopSig();
    }
    break;

    case 321: /* declarator_id: unqualified_id id_attribute_specifier_seq  */
    {
      setVarName((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 322: /* declarator_id: unqualified_id id_attribute_specifier_seq ':' bitfield_size  */
    {
      setVarName((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-3)].yystate.yysemantics.yyval.str));
    }
    break;

    case 327: /* opt_array_decorator_seq: %empty  */
    {
      clearArray();
    }
    break;

    case 329: /* $@46: %empty  */
    {
      clearArray();
    }
    break;

    case 333: /* $@47: %empty  */
    {
      postSig("[");
    }
    break;

    case 334: /* array_decorator: '[' $@47 array_size_specifier ']' array_attribute_specifier_seq */
    {
      postSig("]");
    }
    break;

    case 335: /* array_size_specifier: %empty  */
    {
      pushArraySize("");
    }
    break;

    case 336: /* $@48: %empty  */
    {
      markSig();
    }
    break;

    case 337: /* array_size_specifier: $@48 constant_expression  */
    {
      chopSig();
      pushArraySize(copySig());
    }
    break;

    case 343: /* unqualified_id: tilde_sig class_name  */
    {
      ((*yyvalp).str) = vtkstrcat(
        "~", (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 344: /* unqualified_id: tilde_sig decltype_specifier  */
    {
      ((*yyvalp).str) = vtkstrcat(
        "~", (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 345: /* qualified_id: nested_name_specifier unqualified_id  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 346: /* qualified_id: scope_operator_sig unqualified_id  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 347: /* qualified_id: scope_operator_sig qualified_id  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 348: /* nested_name_specifier: identifier_sig scope_operator_sig  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 349: /* nested_name_specifier: template_id scope_operator_sig  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 350: /* nested_name_specifier: decltype_specifier scope_operator_sig  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 351: /* nested_name_specifier: nested_name_specifier identifier_sig scope_operator_sig  */
    {
      ((*yyvalp).str) = vtkstrcat3(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 352: /* nested_name_specifier: nested_name_specifier template_id scope_operator_sig  */
    {
      ((*yyvalp).str) = vtkstrcat3(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 353: /* nested_name_specifier: nested_name_specifier decltype_specifier scope_operator_sig
               */
    {
      ((*yyvalp).str) = vtkstrcat3(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 354: /* $@49: %empty  */
    {
      postSig("template ");
    }
    break;

    case 355: /* nested_name_specifier: nested_name_specifier TEMPLATE $@49 template_id
                 scope_operator_sig  */
    {
      ((*yyvalp).str) = vtkstrcat4(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-4)].yystate.yysemantics.yyval.str),
        "template ",
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 356: /* tilde_sig: '~'  */
    {
      postSig("~");
    }
    break;

    case 357: /* identifier_sig: identifier  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 358: /* scope_operator_sig: DOUBLE_COLON  */
    {
      ((*yyvalp).str) = "::";
      postSig(((*yyvalp).str));
    }
    break;

    case 359: /* $@50: %empty  */
    {
      markSig();
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
      postSig("<");
    }
    break;

    case 360: /* template_id: identifier '<' $@50 angle_bracket_contents right_angle_bracket  */
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

    case 361: /* $@51: %empty  */
    {
      markSig();
      postSig("decltype");
    }
    break;

    case 362: /* decltype_specifier: DECLTYPE $@51 parentheses_sig  */
    {
      chopSig();
      ((*yyvalp).str) = copySig();
      clearTypeId();
    }
    break;

    case 363: /* simple_id: VTK_ID  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 364: /* simple_id: QT_ID  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 365: /* simple_id: ID  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 366: /* simple_id: ISTREAM  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 367: /* simple_id: OSTREAM  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 368: /* simple_id: StdString  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 369: /* simple_id: NULLPTR_T  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 370: /* simple_id: SIZE_T  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 371: /* simple_id: SSIZE_T  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 381: /* decl_specifier2: primitive_type  */
    {
      setTypeBase(buildTypeBase(getType(),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer)));
    }
    break;

    case 382: /* decl_specifier2: TYPEDEF  */
    {
      setTypeMod(VTK_PARSE_TYPEDEF);
    }
    break;

    case 383: /* decl_specifier2: FRIEND  */
    {
      setTypeMod(VTK_PARSE_FRIEND);
    }
    break;

    case 386: /* decl_specifier: storage_class_specifier  */
    {
      setTypeMod(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 387: /* decl_specifier: function_specifier  */
    {
      setTypeMod(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 388: /* decl_specifier: cv_qualifier  */
    {
      setTypeMod(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 389: /* decl_specifier: CONSTEXPR  */
    {
      postSig("constexpr ");
      ((*yyvalp).integer) = 0;
    }
    break;

    case 390: /* storage_class_specifier: MUTABLE  */
    {
      postSig("mutable ");
      ((*yyvalp).integer) = VTK_PARSE_MUTABLE;
    }
    break;

    case 391: /* storage_class_specifier: EXTERN  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 392: /* storage_class_specifier: EXTERN STRING_LITERAL  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 393: /* storage_class_specifier: STATIC  */
    {
      postSig("static ");
      ((*yyvalp).integer) = VTK_PARSE_STATIC;
    }
    break;

    case 394: /* storage_class_specifier: THREAD_LOCAL  */
    {
      postSig("thread_local ");
      ((*yyvalp).integer) = VTK_PARSE_THREAD_LOCAL;
    }
    break;

    case 395: /* function_specifier: INLINE  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 396: /* function_specifier: VIRTUAL  */
    {
      postSig("virtual ");
      ((*yyvalp).integer) = VTK_PARSE_VIRTUAL;
    }
    break;

    case 397: /* function_specifier: EXPLICIT  */
    {
      postSig("explicit ");
      ((*yyvalp).integer) = VTK_PARSE_EXPLICIT;
    }
    break;

    case 398: /* cv_qualifier: CONST  */
    {
      postSig("const ");
      ((*yyvalp).integer) = VTK_PARSE_CONST;
    }
    break;

    case 399: /* cv_qualifier: VOLATILE  */
    {
      postSig("volatile ");
      ((*yyvalp).integer) = VTK_PARSE_VOLATILE;
    }
    break;

    case 401: /* cv_qualifier_seq: cv_qualifier_seq cv_qualifier  */
    {
      ((*yyvalp).integer) =
        ((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer) |
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 403: /* $@52: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 405: /* $@53: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 408: /* type_specifier: class_key class_attribute_specifier_seq class_head_name  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 409: /* type_specifier: enum_key class_attribute_specifier_seq id_expression
                 decl_attribute_specifier_seq  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 411: /* trailing_type_specifier: decltype_specifier  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = 0;
    }
    break;

    case 412: /* $@54: %empty  */
    {
      postSig("typename ");
    }
    break;

    case 413: /* trailing_type_specifier: TYPENAME $@54 id_expression decl_attribute_specifier_seq
               */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 414: /* trailing_type_specifier: template_id decl_attribute_specifier_seq  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 415: /* trailing_type_specifier: qualified_id decl_attribute_specifier_seq  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 417: /* $@55: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 419: /* $@56: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 422: /* $@57: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 424: /* $@58: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 427: /* tparam_type_specifier: decltype_specifier  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = 0;
    }
    break;

    case 428: /* tparam_type_specifier: template_id  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 429: /* tparam_type_specifier: qualified_id  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 430: /* tparam_type_specifier: STRUCT id_expression  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 431: /* tparam_type_specifier: UNION id_expression  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 432: /* tparam_type_specifier: enum_key id_expression  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 433: /* simple_type_specifier: primitive_type decl_attribute_specifier_seq  */
    {
      setTypeId("");
    }
    break;

    case 435: /* type_name: StdString  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_STRING;
    }
    break;

    case 436: /* type_name: OSTREAM  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_OSTREAM;
    }
    break;

    case 437: /* type_name: ISTREAM  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_ISTREAM;
    }
    break;

    case 438: /* type_name: ID  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_UNKNOWN;
    }
    break;

    case 439: /* type_name: VTK_ID  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_OBJECT;
    }
    break;

    case 440: /* type_name: QT_ID  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_QOBJECT;
    }
    break;

    case 441: /* type_name: NULLPTR_T  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_NULLPTR_T;
    }
    break;

    case 442: /* type_name: SSIZE_T  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_SSIZE_T;
    }
    break;

    case 443: /* type_name: SIZE_T  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_SIZE_T;
    }
    break;

    case 444: /* primitive_type: AUTO  */
    {
      postSig("auto ");
      ((*yyvalp).integer) = 0;
    }
    break;

    case 445: /* primitive_type: VOID  */
    {
      postSig("void ");
      ((*yyvalp).integer) = VTK_PARSE_VOID;
    }
    break;

    case 446: /* primitive_type: BOOL  */
    {
      postSig("bool ");
      ((*yyvalp).integer) = VTK_PARSE_BOOL;
    }
    break;

    case 447: /* primitive_type: FLOAT  */
    {
      postSig("float ");
      ((*yyvalp).integer) = VTK_PARSE_FLOAT;
    }
    break;

    case 448: /* primitive_type: DOUBLE  */
    {
      postSig("double ");
      ((*yyvalp).integer) = VTK_PARSE_DOUBLE;
    }
    break;

    case 449: /* primitive_type: CHAR  */
    {
      postSig("char ");
      ((*yyvalp).integer) = VTK_PARSE_CHAR;
    }
    break;

    case 450: /* primitive_type: CHAR16_T  */
    {
      postSig("char16_t ");
      ((*yyvalp).integer) = VTK_PARSE_CHAR16_T;
    }
    break;

    case 451: /* primitive_type: CHAR32_T  */
    {
      postSig("char32_t ");
      ((*yyvalp).integer) = VTK_PARSE_CHAR32_T;
    }
    break;

    case 452: /* primitive_type: WCHAR_T  */
    {
      postSig("wchar_t ");
      ((*yyvalp).integer) = VTK_PARSE_WCHAR_T;
    }
    break;

    case 453: /* primitive_type: INT  */
    {
      postSig("int ");
      ((*yyvalp).integer) = VTK_PARSE_INT;
    }
    break;

    case 454: /* primitive_type: SHORT  */
    {
      postSig("short ");
      ((*yyvalp).integer) = VTK_PARSE_SHORT;
    }
    break;

    case 455: /* primitive_type: LONG  */
    {
      postSig("long ");
      ((*yyvalp).integer) = VTK_PARSE_LONG;
    }
    break;

    case 456: /* primitive_type: SIGNED  */
    {
      postSig("signed ");
      ((*yyvalp).integer) = VTK_PARSE_INT;
    }
    break;

    case 457: /* primitive_type: UNSIGNED  */
    {
      postSig("unsigned ");
      ((*yyvalp).integer) = VTK_PARSE_UNSIGNED_INT;
    }
    break;

    case 461: /* ptr_operator_seq: pointer_seq reference  */
    {
      ((*yyvalp).integer) =
        ((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer) |
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 462: /* reference: '&' ref_attribute_specifier_seq  */
    {
      postSig("&");
      ((*yyvalp).integer) = VTK_PARSE_REF;
    }
    break;

    case 463: /* rvalue_reference: OP_LOGIC_AND ref_attribute_specifier_seq  */
    {
      postSig("&&");
      ((*yyvalp).integer) = (VTK_PARSE_RVALUE | VTK_PARSE_REF);
    }
    break;

    case 464: /* $@59: %empty  */
    {
      postSig("*");
    }
    break;

    case 465: /* pointer: '*' ref_attribute_specifier_seq $@59 ptr_cv_qualifier_seq  */
    {
      ((*yyvalp).integer) =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer);
    }
    break;

    case 466: /* ptr_cv_qualifier_seq: %empty  */
    {
      ((*yyvalp).integer) = VTK_PARSE_POINTER;
    }
    break;

    case 467: /* ptr_cv_qualifier_seq: cv_qualifier_seq  */
    {
      if (((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer) &
            VTK_PARSE_CONST) != 0)
      {
        ((*yyvalp).integer) = VTK_PARSE_CONST_POINTER;
      }
      if (((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer) &
            VTK_PARSE_VOLATILE) != 0)
      {
        ((*yyvalp).integer) = VTK_PARSE_BAD_INDIRECT;
      }
    }
    break;

    case 469: /* pointer_seq: pointer_seq pointer  */
    {
      unsigned int n;
      n = (((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer)
             << 2) |
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
      if ((n & VTK_PARSE_INDIRECT) != n)
      {
        n = VTK_PARSE_BAD_INDIRECT;
      }
      ((*yyvalp).integer) = n;
    }
    break;

    case 470: /* $@60: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_DECL);
    }
    break;

    case 471: /* decl_attribute_specifier_seq: $@60 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 472: /* $@61: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_ID);
    }
    break;

    case 473: /* id_attribute_specifier_seq: $@61 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 474: /* $@62: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_REF);
    }
    break;

    case 475: /* ref_attribute_specifier_seq: $@62 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 476: /* $@63: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_FUNC);
    }
    break;

    case 477: /* func_attribute_specifier_seq: $@63 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 478: /* $@64: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_ARRAY);
    }
    break;

    case 479: /* array_attribute_specifier_seq: $@64 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 480: /* $@65: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_CLASS);
    }
    break;

    case 481: /* class_attribute_specifier_seq: $@65 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 484: /* attribute_specifier: BEGIN_ATTRIB attribute_specifier_contents ']' ']'  */
    {
      setAttributePrefix(NULL);
    }
    break;

    case 487: /* attribute_using_prefix: USING using_id ':'  */
    {
      setAttributePrefix(vtkstrcat(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str), "::"));
    }
    break;

    case 492: /* $@66: %empty  */
    {
      markSig();
    }
    break;

    case 493: /* attribute: $@66 attribute_sig attribute_pack  */
    {
      handle_attribute(cutSig(),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 494: /* attribute_pack: %empty  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 495: /* attribute_pack: ELLIPSIS  */
    {
      ((*yyvalp).integer) = VTK_PARSE_PACK;
    }
    break;

    case 500: /* operator_id: '(' ')'  */
    {
      ((*yyvalp).str) = "()";
    }
    break;

    case 501: /* operator_id: '[' ']'  */
    {
      ((*yyvalp).str) = "[]";
    }
    break;

    case 502: /* operator_id: NEW '[' ']'  */
    {
      ((*yyvalp).str) = " new[]";
    }
    break;

    case 503: /* operator_id: DELETE '[' ']'  */
    {
      ((*yyvalp).str) = " delete[]";
    }
    break;

    case 504: /* operator_id: '<'  */
    {
      ((*yyvalp).str) = "<";
    }
    break;

    case 505: /* operator_id: '>'  */
    {
      ((*yyvalp).str) = ">";
    }
    break;

    case 506: /* operator_id: ','  */
    {
      ((*yyvalp).str) = ",";
    }
    break;

    case 507: /* operator_id: '='  */
    {
      ((*yyvalp).str) = "=";
    }
    break;

    case 508: /* operator_id: OP_RSHIFT_A '>'  */
    {
      ((*yyvalp).str) = ">>";
    }
    break;

    case 509: /* operator_id: OP_RSHIFT_A OP_RSHIFT_A  */
    {
      ((*yyvalp).str) = ">>";
    }
    break;

    case 510: /* operator_id: STRING_LITERAL ID  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 513: /* operator_id_no_delim: '%'  */
    {
      ((*yyvalp).str) = "%";
    }
    break;

    case 514: /* operator_id_no_delim: '*'  */
    {
      ((*yyvalp).str) = "*";
    }
    break;

    case 515: /* operator_id_no_delim: '/'  */
    {
      ((*yyvalp).str) = "/";
    }
    break;

    case 516: /* operator_id_no_delim: '-'  */
    {
      ((*yyvalp).str) = "-";
    }
    break;

    case 517: /* operator_id_no_delim: '+'  */
    {
      ((*yyvalp).str) = "+";
    }
    break;

    case 518: /* operator_id_no_delim: '!'  */
    {
      ((*yyvalp).str) = "!";
    }
    break;

    case 519: /* operator_id_no_delim: '~'  */
    {
      ((*yyvalp).str) = "~";
    }
    break;

    case 520: /* operator_id_no_delim: '&'  */
    {
      ((*yyvalp).str) = "&";
    }
    break;

    case 521: /* operator_id_no_delim: '|'  */
    {
      ((*yyvalp).str) = "|";
    }
    break;

    case 522: /* operator_id_no_delim: '^'  */
    {
      ((*yyvalp).str) = "^";
    }
    break;

    case 523: /* operator_id_no_delim: NEW  */
    {
      ((*yyvalp).str) = " new";
    }
    break;

    case 524: /* operator_id_no_delim: DELETE  */
    {
      ((*yyvalp).str) = " delete";
    }
    break;

    case 525: /* operator_id_no_delim: OP_LSHIFT_EQ  */
    {
      ((*yyvalp).str) = "<<=";
    }
    break;

    case 526: /* operator_id_no_delim: OP_RSHIFT_EQ  */
    {
      ((*yyvalp).str) = ">>=";
    }
    break;

    case 527: /* operator_id_no_delim: OP_LSHIFT  */
    {
      ((*yyvalp).str) = "<<";
    }
    break;

    case 528: /* operator_id_no_delim: OP_DOT_POINTER  */
    {
      ((*yyvalp).str) = ".*";
    }
    break;

    case 529: /* operator_id_no_delim: OP_ARROW_POINTER  */
    {
      ((*yyvalp).str) = "->*";
    }
    break;

    case 530: /* operator_id_no_delim: OP_ARROW  */
    {
      ((*yyvalp).str) = "->";
    }
    break;

    case 531: /* operator_id_no_delim: OP_PLUS_EQ  */
    {
      ((*yyvalp).str) = "+=";
    }
    break;

    case 532: /* operator_id_no_delim: OP_MINUS_EQ  */
    {
      ((*yyvalp).str) = "-=";
    }
    break;

    case 533: /* operator_id_no_delim: OP_TIMES_EQ  */
    {
      ((*yyvalp).str) = "*=";
    }
    break;

    case 534: /* operator_id_no_delim: OP_DIVIDE_EQ  */
    {
      ((*yyvalp).str) = "/=";
    }
    break;

    case 535: /* operator_id_no_delim: OP_REMAINDER_EQ  */
    {
      ((*yyvalp).str) = "%=";
    }
    break;

    case 536: /* operator_id_no_delim: OP_INCR  */
    {
      ((*yyvalp).str) = "++";
    }
    break;

    case 537: /* operator_id_no_delim: OP_DECR  */
    {
      ((*yyvalp).str) = "--";
    }
    break;

    case 538: /* operator_id_no_delim: OP_AND_EQ  */
    {
      ((*yyvalp).str) = "&=";
    }
    break;

    case 539: /* operator_id_no_delim: OP_OR_EQ  */
    {
      ((*yyvalp).str) = "|=";
    }
    break;

    case 540: /* operator_id_no_delim: OP_XOR_EQ  */
    {
      ((*yyvalp).str) = "^=";
    }
    break;

    case 541: /* operator_id_no_delim: OP_LOGIC_AND  */
    {
      ((*yyvalp).str) = "&&";
    }
    break;

    case 542: /* operator_id_no_delim: OP_LOGIC_OR  */
    {
      ((*yyvalp).str) = "||";
    }
    break;

    case 543: /* operator_id_no_delim: OP_LOGIC_EQ  */
    {
      ((*yyvalp).str) = "==";
    }
    break;

    case 544: /* operator_id_no_delim: OP_LOGIC_NEQ  */
    {
      ((*yyvalp).str) = "!=";
    }
    break;

    case 545: /* operator_id_no_delim: OP_LOGIC_LEQ  */
    {
      ((*yyvalp).str) = "<=";
    }
    break;

    case 546: /* operator_id_no_delim: OP_LOGIC_GEQ  */
    {
      ((*yyvalp).str) = ">=";
    }
    break;

    case 547: /* keyword: TYPEDEF  */
    {
      ((*yyvalp).str) = "typedef";
    }
    break;

    case 548: /* keyword: TYPENAME  */
    {
      ((*yyvalp).str) = "typename";
    }
    break;

    case 549: /* keyword: CLASS  */
    {
      ((*yyvalp).str) = "class";
    }
    break;

    case 550: /* keyword: STRUCT  */
    {
      ((*yyvalp).str) = "struct";
    }
    break;

    case 551: /* keyword: UNION  */
    {
      ((*yyvalp).str) = "union";
    }
    break;

    case 552: /* keyword: TEMPLATE  */
    {
      ((*yyvalp).str) = "template";
    }
    break;

    case 553: /* keyword: PUBLIC  */
    {
      ((*yyvalp).str) = "public";
    }
    break;

    case 554: /* keyword: PROTECTED  */
    {
      ((*yyvalp).str) = "protected";
    }
    break;

    case 555: /* keyword: PRIVATE  */
    {
      ((*yyvalp).str) = "private";
    }
    break;

    case 556: /* keyword: CONST  */
    {
      ((*yyvalp).str) = "const";
    }
    break;

    case 557: /* keyword: VOLATILE  */
    {
      ((*yyvalp).str) = "volatile";
    }
    break;

    case 558: /* keyword: STATIC  */
    {
      ((*yyvalp).str) = "static";
    }
    break;

    case 559: /* keyword: THREAD_LOCAL  */
    {
      ((*yyvalp).str) = "thread_local";
    }
    break;

    case 560: /* keyword: CONSTEXPR  */
    {
      ((*yyvalp).str) = "constexpr";
    }
    break;

    case 561: /* keyword: INLINE  */
    {
      ((*yyvalp).str) = "inline";
    }
    break;

    case 562: /* keyword: VIRTUAL  */
    {
      ((*yyvalp).str) = "virtual";
    }
    break;

    case 563: /* keyword: EXPLICIT  */
    {
      ((*yyvalp).str) = "explicit";
    }
    break;

    case 564: /* keyword: DECLTYPE  */
    {
      ((*yyvalp).str) = "decltype";
    }
    break;

    case 565: /* keyword: DEFAULT  */
    {
      ((*yyvalp).str) = "default";
    }
    break;

    case 566: /* keyword: EXTERN  */
    {
      ((*yyvalp).str) = "extern";
    }
    break;

    case 567: /* keyword: USING  */
    {
      ((*yyvalp).str) = "using";
    }
    break;

    case 568: /* keyword: NAMESPACE  */
    {
      ((*yyvalp).str) = "namespace";
    }
    break;

    case 569: /* keyword: OPERATOR  */
    {
      ((*yyvalp).str) = "operator";
    }
    break;

    case 570: /* keyword: ENUM  */
    {
      ((*yyvalp).str) = "enum";
    }
    break;

    case 571: /* keyword: THROW  */
    {
      ((*yyvalp).str) = "throw";
    }
    break;

    case 572: /* keyword: NOEXCEPT  */
    {
      ((*yyvalp).str) = "noexcept";
    }
    break;

    case 573: /* keyword: CONST_CAST  */
    {
      ((*yyvalp).str) = "const_cast";
    }
    break;

    case 574: /* keyword: DYNAMIC_CAST  */
    {
      ((*yyvalp).str) = "dynamic_cast";
    }
    break;

    case 575: /* keyword: STATIC_CAST  */
    {
      ((*yyvalp).str) = "static_cast";
    }
    break;

    case 576: /* keyword: REINTERPRET_CAST  */
    {
      ((*yyvalp).str) = "reinterpret_cast";
    }
    break;

    case 591: /* constant_expression_item: '<'  */
    {
      postSig("< ");
    }
    break;

    case 592: /* $@67: %empty  */
    {
      postSig("> ");
    }
    break;

    case 594: /* constant_expression_item: OP_RSHIFT_A  */
    {
      postSig(">");
    }
    break;

    case 596: /* common_bracket_item: DOUBLE_COLON  */
    {
      chopSig();
      postSig("::");
    }
    break;

    case 600: /* common_bracket_item_no_scope_operator: operator_id_no_delim  */
    {
      const char* op =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str);
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

    case 601: /* common_bracket_item_no_scope_operator: ':'  */
    {
      postSig(":");
      postSig(" ");
    }
    break;

    case 602: /* common_bracket_item_no_scope_operator: '.'  */
    {
      postSig(".");
    }
    break;

    case 603: /* common_bracket_item_no_scope_operator: keyword  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig(" ");
    }
    break;

    case 604: /* common_bracket_item_no_scope_operator: literal  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig(" ");
    }
    break;

    case 606: /* common_bracket_item_no_scope_operator: type_name  */
    {
      chopSig();
      postSig(" ");
    }
    break;

    case 610: /* bracket_pitem: '<'  */
    {
      postSig("< ");
    }
    break;

    case 611: /* bracket_pitem: '>'  */
    {
      postSig("> ");
    }
    break;

    case 612: /* bracket_pitem: OP_RSHIFT_A  */
    {
      postSig(">");
    }
    break;

    case 614: /* any_bracket_item: '='  */
    {
      postSig("= ");
    }
    break;

    case 615: /* any_bracket_item: ','  */
    {
      chopSig();
      postSig(", ");
    }
    break;

    case 617: /* braces_item: ';'  */
    {
      chopSig();
      postSig(";");
    }
    break;

    case 625: /* angle_bracket_item: '='  */
    {
      postSig("= ");
    }
    break;

    case 626: /* angle_bracket_item: ','  */
    {
      chopSig();
      postSig(", ");
    }
    break;

    case 627: /* $@68: %empty  */
    {
      chopSig();
      if (getSig()[getSigLength() - 1] == '<')
      {
        postSig(" ");
      }
      postSig("<");
    }
    break;

    case 628: /* angle_brackets_sig: '<' $@68 angle_bracket_contents right_angle_bracket  */
    {
      chopSig();
      if (getSig()[getSigLength() - 1] == '>')
      {
        postSig(" ");
      }
      postSig("> ");
    }
    break;

    case 631: /* $@69: %empty  */
    {
      postSigLeftBracket("[");
    }
    break;

    case 632: /* brackets_sig: '[' $@69 any_bracket_contents ']'  */
    {
      postSigRightBracket("] ");
    }
    break;

    case 633: /* $@70: %empty  */
    {
      postSig("[[");
    }
    break;

    case 634: /* brackets_sig: BEGIN_ATTRIB $@70 any_bracket_contents ']' ']'  */
    {
      chopSig();
      postSig("]] ");
    }
    break;

    case 635: /* $@71: %empty  */
    {
      postSigLeftBracket("(");
    }
    break;

    case 636: /* parentheses_sig: '(' $@71 any_bracket_contents ')'  */
    {
      postSigRightBracket(") ");
    }
    break;

    case 637: /* $@72: %empty  */
    {
      postSigLeftBracket("(");
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig("*");
    }
    break;

    case 638: /* parentheses_sig: LP $@72 any_bracket_contents ')'  */
    {
      postSigRightBracket(") ");
    }
    break;

    case 639: /* $@73: %empty  */
    {
      postSigLeftBracket("(");
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig("&");
    }
    break;

    case 640: /* parentheses_sig: LA $@73 any_bracket_contents ')'  */
    {
      postSigRightBracket(") ");
    }
    break;

    case 641: /* $@74: %empty  */
    {
      postSig("{ ");
    }
    break;

    case 642: /* braces_sig: '{' $@74 braces_contents '}'  */
    {
      postSig("} ");
    }
    break;

    default:
      break;
  }
  YY_SYMBOL_PRINT("-> $$ =", yylhsNonterm(yyrule), yyvalp, yylocp);

  return yyok;
#undef yyerrok
#undef YYABORT
#undef YYACCEPT
#undef YYNOMEM
#undef YYERROR
#undef YYBACKUP
#undef yyclearin
#undef YYRECOVERING
}

static void yyuserMerge(int yyn, YYSTYPE* yy0, YYSTYPE* yy1)
{
  YY_USE(yy0);
  YY_USE(yy1);

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

static void yydestruct(const char* yymsg, yysymbol_kind_t yykind, YYSTYPE* yyvaluep)
{
  YY_USE(yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT(yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE(yykind);
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
    yydestruct(yymsg, yy_accessing_symbol(yys->yylrState), &yys->yysemantics.yyval);
  else
  {
#if YYDEBUG
    if (yydebug)
    {
      if (yys->yysemantics.yyfirstVal)
        YY_FPRINTF((stderr, "%s unresolved", yymsg));
      else
        YY_FPRINTF((stderr, "%s incomplete", yymsg));
      YY_SYMBOL_PRINT("", yy_accessing_symbol(yys->yylrState), YY_NULLPTR, &yys->yyloc);
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

#define yypact_value_is_default(Yyn) ((Yyn) == YYPACT_NINF)

/** True iff LR state YYSTATE has only a default reduction (regardless
 *  of token).  */
static yybool yyisDefaultedState(yy_state_t yystate)
{
  return yypact_value_is_default(yypact[yystate]);
}

/** The default reduction for YYSTATE, assuming it has one.  */
static yyRuleNum yydefaultAction(yy_state_t yystate)
{
  return yydefact[yystate];
}

#define yytable_value_is_error(Yyn) 0

/** The action to take in YYSTATE on seeing YYTOKEN.
 *  Result R means
 *    R < 0:  Reduce on rule -R.
 *    R = 0:  Error.
 *    R > 0:  Shift to state R.
 *  Set *YYCONFLICTS to a pointer into yyconfl to a 0-terminated list
 *  of conflicting reductions.
 */
static int yygetLRActions(yy_state_t yystate, yysymbol_kind_t yytoken, const short** yyconflicts)
{
  int yyindex = yypact[yystate] + yytoken;
  if (yytoken == YYSYMBOL_YYerror)
  {
    // This is the error token.
    *yyconflicts = yyconfl;
    return 0;
  }
  else if (yyisDefaultedState(yystate) || yyindex < 0 || YYLAST < yyindex ||
    yycheck[yyindex] != yytoken)
  {
    *yyconflicts = yyconfl;
    return -yydefact[yystate];
  }
  else if (!yytable_value_is_error(yytable[yyindex]))
  {
    *yyconflicts = yyconfl + yyconflp[yyindex];
    return yytable[yyindex];
  }
  else
  {
    *yyconflicts = yyconfl + yyconflp[yyindex];
    return 0;
  }
}

/** Compute post-reduction state.
 * \param yystate   the current state
 * \param yysym     the nonterminal to push on the stack
 */
static yy_state_t yyLRgotoState(yy_state_t yystate, yysymbol_kind_t yysym)
{
  int yyr = yypgoto[yysym - YYNTOKENS] + yystate;
  if (0 <= yyr && yyr <= YYLAST && yycheck[yyr] == yystate)
    return yytable[yyr];
  else
    return yydefgoto[yysym - YYNTOKENS];
}

static yybool yyisShiftAction(int yyaction)
{
  return 0 < yyaction;
}

static yybool yyisErrorAction(int yyaction)
{
  return yyaction == 0;
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
  yyGLRStack* yystackp, YYPTRDIFF_T yyk, yyGLRState* yystate, yyGLRState* yyrhs, yyRuleNum yyrule)
{
  yySemanticOption* yynewOption = &yynewGLRStackItem(yystackp, yyfalse)->yyoption;
  YY_ASSERT(!yynewOption->yyisState);
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
  yyset->yystates =
    YY_CAST(yyGLRState**, YYMALLOC(YY_CAST(YYSIZE_T, yyset->yycapacity) * sizeof(yyGLRState*)));
  if (!yyset->yystates)
    return yyfalse;
  yyset->yystates[0] = YY_NULLPTR;
  yyset->yylookaheadNeeds =
    YY_CAST(yybool*, YYMALLOC(YY_CAST(YYSIZE_T, yyset->yycapacity) * sizeof(yybool)));
  if (!yyset->yylookaheadNeeds)
  {
    YYFREE(yyset->yystates);
    return yyfalse;
  }
  memset(yyset->yylookaheadNeeds, 0,
    YY_CAST(YYSIZE_T, yyset->yycapacity) * sizeof yyset->yylookaheadNeeds[0]);
  return yytrue;
}

static void yyfreeStateSet(yyGLRStateSet* yyset)
{
  YYFREE(yyset->yystates);
  YYFREE(yyset->yylookaheadNeeds);
}

/** Initialize *YYSTACKP to a single empty stack, with total maximum
 *  capacity for all stacks of YYSIZE.  */
static yybool yyinitGLRStack(yyGLRStack* yystackp, YYPTRDIFF_T yysize)
{
  yystackp->yyerrState = 0;
  yynerrs = 0;
  yystackp->yyspaceLeft = yysize;
  yystackp->yyitems =
    YY_CAST(yyGLRStackItem*, YYMALLOC(YY_CAST(YYSIZE_T, yysize) * sizeof(yyGLRStackItem)));
  if (!yystackp->yyitems)
    return yyfalse;
  yystackp->yynextFree = yystackp->yyitems;
  yystackp->yysplitPoint = YY_NULLPTR;
  yystackp->yylastDeleted = YY_NULLPTR;
  return yyinitStateSet(&yystackp->yytops);
}

#if YYSTACKEXPANDABLE
#define YYRELOC(YYFROMITEMS, YYTOITEMS, YYX, YYTYPE)                                               \
  &((YYTOITEMS) - ((YYFROMITEMS)-YY_REINTERPRET_CAST(yyGLRStackItem*, (YYX))))->YYTYPE

/** If *YYSTACKP is expandable, extend it.  WARNING: Pointers into the
    stack from outside should be considered invalid after this call.
    We always expand when there are 1 or fewer items left AFTER an
    allocation, so that we can avoid having external pointers exist
    across an allocation.  */
static void yyexpandGLRStack(yyGLRStack* yystackp)
{
  yyGLRStackItem* yynewItems;
  yyGLRStackItem *yyp0, *yyp1;
  YYPTRDIFF_T yynewSize;
  YYPTRDIFF_T yyn;
  YYPTRDIFF_T yysize = yystackp->yynextFree - yystackp->yyitems;
  if (YYMAXDEPTH - YYHEADROOM < yysize)
    yyMemoryExhausted(yystackp);
  yynewSize = 2 * yysize;
  if (YYMAXDEPTH < yynewSize)
    yynewSize = YYMAXDEPTH;
  yynewItems =
    YY_CAST(yyGLRStackItem*, YYMALLOC(YY_CAST(YYSIZE_T, yynewSize) * sizeof(yyGLRStackItem)));
  if (!yynewItems)
    yyMemoryExhausted(yystackp);
  for (yyp0 = yystackp->yyitems, yyp1 = yynewItems, yyn = yysize; 0 < yyn;
       yyn -= 1, yyp0 += 1, yyp1 += 1)
  {
    *yyp1 = *yyp0;
    if (*YY_REINTERPRET_CAST(yybool*, yyp0))
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
static void yymarkStackDeleted(yyGLRStack* yystackp, YYPTRDIFF_T yyk)
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
  YY_DPRINTF((stderr, "Restoring last deleted stack as stack #0.\n"));
  yystackp->yylastDeleted = YY_NULLPTR;
}

static void yyremoveDeletes(yyGLRStack* yystackp)
{
  YYPTRDIFF_T yyi, yyj;
  yyi = yyj = 0;
  while (yyj < yystackp->yytops.yysize)
  {
    if (yystackp->yytops.yystates[yyi] == YY_NULLPTR)
    {
      if (yyi == yyj)
        YY_DPRINTF((stderr, "Removing dead stacks.\n"));
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
        YY_DPRINTF((stderr, "Rename stack %ld -> %ld.\n", YY_CAST(long, yyi), YY_CAST(long, yyj)));
      yyj += 1;
    }
    yyi += 1;
  }
}

/** Shift to a new state on stack #YYK of *YYSTACKP, corresponding to LR
 * state YYLRSTATE, at input position YYPOSN, with (resolved) semantic
 * value *YYVALP and source location *YYLOCP.  */
static void yyglrShift(
  yyGLRStack* yystackp, YYPTRDIFF_T yyk, yy_state_t yylrState, YYPTRDIFF_T yyposn, YYSTYPE* yyvalp)
{
  yyGLRState* yynewState = &yynewGLRStackItem(yystackp, yytrue)->yystate;

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yytrue;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yyval = *yyvalp;
  yystackp->yytops.yystates[yyk] = yynewState;

  YY_RESERVE_GLRSTACK(yystackp);
}

/** Shift stack #YYK of *YYSTACKP, to a new state corresponding to LR
 *  state YYLRSTATE, at input position YYPOSN, with the (unresolved)
 *  semantic value of YYRHS under the action for YYRULE.  */
static void yyglrShiftDefer(yyGLRStack* yystackp, YYPTRDIFF_T yyk, yy_state_t yylrState,
  YYPTRDIFF_T yyposn, yyGLRState* yyrhs, yyRuleNum yyrule)
{
  yyGLRState* yynewState = &yynewGLRStackItem(yystackp, yytrue)->yystate;
  YY_ASSERT(yynewState->yyisState);

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yyfalse;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yyfirstVal = YY_NULLPTR;
  yystackp->yytops.yystates[yyk] = yynewState;

  /* Invokes YY_RESERVE_GLRSTACK.  */
  yyaddDeferredAction(yystackp, yyk, yynewState, yyrhs, yyrule);
}

#if YYDEBUG

/*----------------------------------------------------------------------.
| Report that stack #YYK of *YYSTACKP is going to be reduced by YYRULE. |
`----------------------------------------------------------------------*/

static void yy_reduce_print(
  yybool yynormal, yyGLRStackItem* yyvsp, YYPTRDIFF_T yyk, yyRuleNum yyrule)
{
  int yynrhs = yyrhsLength(yyrule);
  int yyi;
  YY_FPRINTF((stderr, "Reducing stack %ld by rule %d (line %d):\n", YY_CAST(long, yyk), yyrule - 1,
    yyrline[yyrule]));
  if (!yynormal)
    yyfillin(yyvsp, 1, -yynrhs);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
  {
    YY_FPRINTF((stderr, "   $%d = ", yyi + 1));
    yy_symbol_print(stderr, yy_accessing_symbol(yyvsp[yyi - yynrhs + 1].yystate.yylrState),
      &yyvsp[yyi - yynrhs + 1].yystate.yysemantics.yyval);
    if (!yyvsp[yyi - yynrhs + 1].yystate.yyresolved)
      YY_FPRINTF((stderr, " (unresolved)"));
    YY_FPRINTF((stderr, "\n"));
  }
}
#endif

/** Pop the symbols consumed by reduction #YYRULE from the top of stack
 *  #YYK of *YYSTACKP, and perform the appropriate semantic action on their
 *  semantic values.  Assumes that all ambiguities in semantic values
 *  have been previously resolved.  Set *YYVALP to the resulting value,
 *  and *YYLOCP to the computed location (if any).  Return value is as
 *  for userAction.  */
static YYRESULTTAG yydoAction(
  yyGLRStack* yystackp, YYPTRDIFF_T yyk, yyRuleNum yyrule, YYSTYPE* yyvalp)
{
  int yynrhs = yyrhsLength(yyrule);

  if (yystackp->yysplitPoint == YY_NULLPTR)
  {
    /* Standard special case: single stack.  */
    yyGLRStackItem* yyrhs = YY_REINTERPRET_CAST(yyGLRStackItem*, yystackp->yytops.yystates[yyk]);
    YY_ASSERT(yyk == 0);
    yystackp->yynextFree -= yynrhs;
    yystackp->yyspaceLeft += yynrhs;
    yystackp->yytops.yystates[0] = &yystackp->yynextFree[-1].yystate;
    return yyuserAction(yyrule, yynrhs, yyrhs, yystackp, yyk, yyvalp);
  }
  else
  {
    yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
    yyGLRState* yys = yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred =
      yystackp->yytops.yystates[yyk];
    int yyi;
    for (yyi = 0; yyi < yynrhs; yyi += 1)
    {
      yys = yys->yypred;
      YY_ASSERT(yys);
    }
    yyupdateSplit(yystackp, yys);
    yystackp->yytops.yystates[yyk] = yys;
    return yyuserAction(
      yyrule, yynrhs, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1, yystackp, yyk, yyvalp);
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
  yyGLRStack* yystackp, YYPTRDIFF_T yyk, yyRuleNum yyrule, yybool yyforceEval)
{
  YYPTRDIFF_T yyposn = yystackp->yytops.yystates[yyk]->yyposn;

  if (yyforceEval || yystackp->yysplitPoint == YY_NULLPTR)
  {
    YYSTYPE yyval;

    YYRESULTTAG yyflag = yydoAction(yystackp, yyk, yyrule, &yyval);
    if (yyflag == yyerr && yystackp->yysplitPoint != YY_NULLPTR)
      YY_DPRINTF((stderr, "Parse on stack %ld rejected by rule %d (line %d).\n", YY_CAST(long, yyk),
        yyrule - 1, yyrline[yyrule]));
    if (yyflag != yyok)
      return yyflag;
    yyglrShift(yystackp, yyk,
      yyLRgotoState(yystackp->yytops.yystates[yyk]->yylrState, yylhsNonterm(yyrule)), yyposn,
      &yyval);
  }
  else
  {
    YYPTRDIFF_T yyi;
    int yyn;
    yyGLRState *yys, *yys0 = yystackp->yytops.yystates[yyk];
    yy_state_t yynewLRState;

    for (yys = yystackp->yytops.yystates[yyk], yyn = yyrhsLength(yyrule); 0 < yyn; yyn -= 1)
    {
      yys = yys->yypred;
      YY_ASSERT(yys);
    }
    yyupdateSplit(yystackp, yys);
    yynewLRState = yyLRgotoState(yys->yylrState, yylhsNonterm(yyrule));
    YY_DPRINTF((stderr,
      "Reduced stack %ld by rule %d (line %d); action deferred.  "
      "Now in state %d.\n",
      YY_CAST(long, yyk), yyrule - 1, yyrline[yyrule], yynewLRState));
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
            YY_DPRINTF((stderr, "Merging stack %ld into stack %ld.\n", YY_CAST(long, yyk),
              YY_CAST(long, yyi)));
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

static YYPTRDIFF_T yysplitStack(yyGLRStack* yystackp, YYPTRDIFF_T yyk)
{
  if (yystackp->yysplitPoint == YY_NULLPTR)
  {
    YY_ASSERT(yyk == 0);
    yystackp->yysplitPoint = yystackp->yytops.yystates[yyk];
  }
  if (yystackp->yytops.yycapacity <= yystackp->yytops.yysize)
  {
    YYPTRDIFF_T state_size = YYSIZEOF(yystackp->yytops.yystates[0]);
    YYPTRDIFF_T half_max_capacity = YYSIZE_MAXIMUM / 2 / state_size;
    if (half_max_capacity < yystackp->yytops.yycapacity)
      yyMemoryExhausted(yystackp);
    yystackp->yytops.yycapacity *= 2;

    {
      yyGLRState** yynewStates = YY_CAST(yyGLRState**,
        YYREALLOC(yystackp->yytops.yystates,
          (YY_CAST(YYSIZE_T, yystackp->yytops.yycapacity) * sizeof(yyGLRState*))));
      if (yynewStates == YY_NULLPTR)
        yyMemoryExhausted(yystackp);
      yystackp->yytops.yystates = yynewStates;
    }

    {
      yybool* yynewLookaheadNeeds = YY_CAST(yybool*,
        YYREALLOC(yystackp->yytops.yylookaheadNeeds,
          (YY_CAST(YYSIZE_T, yystackp->yytops.yycapacity) * sizeof(yybool))));
      if (yynewLookaheadNeeds == YY_NULLPTR)
        yyMemoryExhausted(yystackp);
      yystackp->yytops.yylookaheadNeeds = yynewLookaheadNeeds;
    }
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
  for (yys0 = yyy0->yystate, yys1 = yyy1->yystate, yyn = yyrhsLength(yyy0->yyrule); 0 < yyn;
       yys0 = yys0->yypred, yys1 = yys1->yypred, yyn -= 1)
  {
    if (yys0 == yys1)
      break;
    else if (yys0->yyresolved)
    {
      yys1->yyresolved = yytrue;
      yys1->yysemantics.yyval = yys0->yysemantics.yyval;
    }
    else if (yys1->yyresolved)
    {
      yys0->yyresolved = yytrue;
      yys0->yysemantics.yyval = yys1->yysemantics.yyval;
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
    YY_ASSERT(yys->yypred);
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
    yyflag = yyuserAction(
      yyopt->yyrule, yynrhs, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1, yystackp, -1, yyvalp);
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
    YY_FPRINTF((stderr, "%*s%s -> <Rule %d, empty>\n", yyindent, "",
      yysymbol_name(yylhsNonterm(yyx->yyrule)), yyx->yyrule - 1));
  else
    YY_FPRINTF((stderr, "%*s%s -> <Rule %d, tokens %ld .. %ld>\n", yyindent, "",
      yysymbol_name(yylhsNonterm(yyx->yyrule)), yyx->yyrule - 1, YY_CAST(long, yys->yyposn + 1),
      YY_CAST(long, yyx->yystate->yyposn)));
  for (yyi = 1; yyi <= yynrhs; yyi += 1)
  {
    if (yystates[yyi]->yyresolved)
    {
      if (yystates[yyi - 1]->yyposn + 1 > yystates[yyi]->yyposn)
        YY_FPRINTF((stderr, "%*s%s <empty>\n", yyindent + 2, "",
          yysymbol_name(yy_accessing_symbol(yystates[yyi]->yylrState))));
      else
        YY_FPRINTF((stderr, "%*s%s <tokens %ld .. %ld>\n", yyindent + 2, "",
          yysymbol_name(yy_accessing_symbol(yystates[yyi]->yylrState)),
          YY_CAST(long, yystates[yyi - 1]->yyposn + 1), YY_CAST(long, yystates[yyi]->yyposn)));
    }
    else
      yyreportTree(yystates[yyi]->yysemantics.yyfirstVal, yyindent + 2);
  }
}
#endif

static YYRESULTTAG yyreportAmbiguity(yySemanticOption* yyx0, yySemanticOption* yyx1)
{
  YY_USE(yyx0);
  YY_USE(yyx1);

#if YYDEBUG
  YY_FPRINTF((stderr, "Ambiguity detected.\n"));
  YY_FPRINTF((stderr, "Option 1,\n"));
  yyreportTree(yyx0, 2);
  YY_FPRINTF((stderr, "\nOption 2,\n"));
  yyreportTree(yyx1, 2);
  YY_FPRINTF((stderr, "\n"));
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
  YYSTYPE yyval;
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
          /* This cannot happen so it is not worth a YY_ASSERT (yyfalse),
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
    yyflag = yyresolveAction(yybest, yystackp, &yyval);
    if (yyflag == yyok)
      for (yyp = yybest->yynext; yyp != YY_NULLPTR; yyp = yyp->yynext)
      {
        if (yyprec == yydprec[yyp->yyrule])
        {
          YYSTYPE yyval_other;
          yyflag = yyresolveAction(yyp, yystackp, &yyval_other);
          if (yyflag != yyok)
          {
            yydestruct("Cleanup: discarding incompletely merged value for",
              yy_accessing_symbol(yys->yylrState), &yyval);
            break;
          }
          yyuserMerge(yymerger[yyp->yyrule], &yyval, &yyval_other);
        }
      }
  }
  else
    yyflag = yyresolveAction(yybest, yystackp, &yyval);

  if (yyflag == yyok)
  {
    yys->yyresolved = yytrue;
    yys->yysemantics.yyval = yyval;
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

/** Called when returning to deterministic operation to clean up the extra
 * stacks. */
static void yycompressStack(yyGLRStack* yystackp)
{
  /* yyr is the state after the split point.  */
  yyGLRState* yyr;

  if (yystackp->yytops.yysize != 1 || yystackp->yysplitPoint == YY_NULLPTR)
    return;

  {
    yyGLRState *yyp, *yyq;
    for (yyp = yystackp->yytops.yystates[0], yyq = yyp->yypred, yyr = YY_NULLPTR;
         yyp != yystackp->yysplitPoint; yyr = yyp, yyp = yyq, yyq = yyp->yypred)
      yyp->yypred = yyr;
  }

  yystackp->yyspaceLeft += yystackp->yynextFree - yystackp->yyitems;
  yystackp->yynextFree = YY_REINTERPRET_CAST(yyGLRStackItem*, yystackp->yysplitPoint) + 1;
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

static YYRESULTTAG yyprocessOneStack(yyGLRStack* yystackp, YYPTRDIFF_T yyk, YYPTRDIFF_T yyposn)
{
  while (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
  {
    yy_state_t yystate = yystackp->yytops.yystates[yyk]->yylrState;
    YY_DPRINTF((stderr, "Stack %ld Entering state %d\n", YY_CAST(long, yyk), yystate));

    YY_ASSERT(yystate != YYFINAL);

    if (yyisDefaultedState(yystate))
    {
      YYRESULTTAG yyflag;
      yyRuleNum yyrule = yydefaultAction(yystate);
      if (yyrule == 0)
      {
        YY_DPRINTF((stderr, "Stack %ld dies.\n", YY_CAST(long, yyk)));
        yymarkStackDeleted(yystackp, yyk);
        return yyok;
      }
      yyflag = yyglrReduce(yystackp, yyk, yyrule, yyimmediate[yyrule]);
      if (yyflag == yyerr)
      {
        YY_DPRINTF((stderr,
          "Stack %ld dies "
          "(predicate failure or explicit user error).\n",
          YY_CAST(long, yyk)));
        yymarkStackDeleted(yystackp, yyk);
        return yyok;
      }
      if (yyflag != yyok)
        return yyflag;
    }
    else
    {
      yysymbol_kind_t yytoken = yygetToken(&yychar);
      const short* yyconflicts;
      const int yyaction = yygetLRActions(yystate, yytoken, &yyconflicts);
      yystackp->yytops.yylookaheadNeeds[yyk] = yytrue;

      for (/* nothing */; *yyconflicts; yyconflicts += 1)
      {
        YYRESULTTAG yyflag;
        YYPTRDIFF_T yynewStack = yysplitStack(yystackp, yyk);
        YY_DPRINTF((stderr, "Splitting off stack %ld from %ld.\n", YY_CAST(long, yynewStack),
          YY_CAST(long, yyk)));
        yyflag = yyglrReduce(yystackp, yynewStack, *yyconflicts, yyimmediate[*yyconflicts]);
        if (yyflag == yyok)
          YYCHK(yyprocessOneStack(yystackp, yynewStack, yyposn));
        else if (yyflag == yyerr)
        {
          YY_DPRINTF((stderr, "Stack %ld dies.\n", YY_CAST(long, yynewStack)));
          yymarkStackDeleted(yystackp, yynewStack);
        }
        else
          return yyflag;
      }

      if (yyisShiftAction(yyaction))
        break;
      else if (yyisErrorAction(yyaction))
      {
        YY_DPRINTF((stderr, "Stack %ld dies.\n", YY_CAST(long, yyk)));
        yymarkStackDeleted(yystackp, yyk);
        break;
      }
      else
      {
        YYRESULTTAG yyflag = yyglrReduce(yystackp, yyk, -yyaction, yyimmediate[-yyaction]);
        if (yyflag == yyerr)
        {
          YY_DPRINTF((stderr,
            "Stack %ld dies "
            "(predicate failure or explicit user error).\n",
            YY_CAST(long, yyk)));
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
  yyerror(YY_("syntax error"));
  yynerrs += 1;
}

/* Recover from a syntax error on *YYSTACKP, assuming that *YYSTACKP->YYTOKENP,
   yylval, and yylloc are the syntactic category, semantic value, and location
   of the lookahead.  */
static void yyrecoverSyntaxError(yyGLRStack* yystackp)
{
  if (yystackp->yyerrState == 3)
    /* We just shifted the error token and (perhaps) took some
       reductions.  Skip tokens until we can proceed.  */
    while (yytrue)
    {
      yysymbol_kind_t yytoken;
      int yyj;
      if (yychar == YYEOF)
        yyFail(yystackp, YY_NULLPTR);
      if (yychar != YYEMPTY)
      {
        yytoken = YYTRANSLATE(yychar);
        yydestruct("Error: discarding", yytoken, &yylval);
        yychar = YYEMPTY;
      }
      yytoken = yygetToken(&yychar);
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
  {
    YYPTRDIFF_T yyk;
    for (yyk = 0; yyk < yystackp->yytops.yysize; yyk += 1)
      if (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
        break;
    if (yyk >= yystackp->yytops.yysize)
      yyFail(yystackp, YY_NULLPTR);
    for (yyk += 1; yyk < yystackp->yytops.yysize; yyk += 1)
      yymarkStackDeleted(yystackp, yyk);
    yyremoveDeletes(yystackp);
    yycompressStack(yystackp);
  }

  /* Pop stack until we find a state that shifts the error token.  */
  yystackp->yyerrState = 3;
  while (yystackp->yytops.yystates[0] != YY_NULLPTR)
  {
    yyGLRState* yys = yystackp->yytops.yystates[0];
    int yyj = yypact[yys->yylrState];
    if (!yypact_value_is_default(yyj))
    {
      yyj += YYSYMBOL_YYerror;
      if (0 <= yyj && yyj <= YYLAST && yycheck[yyj] == YYSYMBOL_YYerror &&
        yyisShiftAction(yytable[yyj]))
      {
        /* Shift the error token.  */
        int yyaction = yytable[yyj];
        YY_SYMBOL_PRINT("Shifting", yy_accessing_symbol(yyaction), &yylval, &yyerrloc);
        yyglrShift(yystackp, 0, yyaction, yys->yyposn, &yylval);
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
      case yynomem:                                                                                \
        goto yyexhaustedlab;                                                                       \
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
  YYPTRDIFF_T yyposn;

  YY_DPRINTF((stderr, "Starting parse\n"));

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
    /* Standard mode. */
    while (yytrue)
    {
      yy_state_t yystate = yystack.yytops.yystates[0]->yylrState;
      YY_DPRINTF((stderr, "Entering state %d\n", yystate));
      if (yystate == YYFINAL)
        goto yyacceptlab;
      if (yyisDefaultedState(yystate))
      {
        yyRuleNum yyrule = yydefaultAction(yystate);
        if (yyrule == 0)
        {
          yyreportSyntaxError(&yystack);
          goto yyuser_error;
        }
        YYCHK1(yyglrReduce(&yystack, 0, yyrule, yytrue));
      }
      else
      {
        yysymbol_kind_t yytoken = yygetToken(&yychar);
        const short* yyconflicts;
        int yyaction = yygetLRActions(yystate, yytoken, &yyconflicts);
        if (*yyconflicts)
          /* Enter nondeterministic mode.  */
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
          /* Issue an error message unless the scanner already
             did. */
          if (yychar != YYerror)
            yyreportSyntaxError(&yystack);
          goto yyuser_error;
        }
        else
          YYCHK1(yyglrReduce(&yystack, 0, -yyaction, yytrue));
      }
    }

    /* Nondeterministic mode. */
    while (yytrue)
    {
      yysymbol_kind_t yytoken_to_shift;
      YYPTRDIFF_T yys;

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
        YYCHK1(yyprocessOneStack(&yystack, yys, yyposn));
      yyremoveDeletes(&yystack);
      if (yystack.yytops.yysize == 0)
      {
        yyundeleteLastStack(&yystack);
        if (yystack.yytops.yysize == 0)
          yyFail(&yystack, YY_("syntax error"));
        YYCHK1(yyresolveStack(&yystack));
        YY_DPRINTF((stderr, "Returning to deterministic operation.\n"));
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
        yy_state_t yystate = yystack.yytops.yystates[yys]->yylrState;
        const short* yyconflicts;
        int yyaction = yygetLRActions(yystate, yytoken_to_shift, &yyconflicts);
        /* Note that yyconflicts were handled by yyprocessOneStack.  */
        YY_DPRINTF((stderr, "On stack %ld, ", YY_CAST(long, yys)));
        YY_SYMBOL_PRINT("shifting", yytoken_to_shift, &yylval, &yylloc);
        yyglrShift(&yystack, yys, yyaction, yyposn, &yylval);
        YY_DPRINTF((stderr, "Stack %ld now in state %d\n", YY_CAST(long, yys),
          yystack.yytops.yystates[yys]->yylrState));
      }

      if (yystack.yytops.yysize == 1)
      {
        YYCHK1(yyresolveStack(&yystack));
        YY_DPRINTF((stderr, "Returning to deterministic operation.\n"));
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
  goto yyreturnlab;

yybuglab:
  YY_ASSERT(yyfalse);
  goto yyabortlab;

yyabortlab:
  yyresult = 1;
  goto yyreturnlab;

yyexhaustedlab:
  yyerror(YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;

yyreturnlab:
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
      YYPTRDIFF_T yysize = yystack.yytops.yysize;
      YYPTRDIFF_T yyk;
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
/* Print *YYS and its predecessors. */
static void yy_yypstack(yyGLRState* yys)
{
  if (yys->yypred)
  {
    yy_yypstack(yys->yypred);
    YY_FPRINTF((stderr, " -> "));
  }
  YY_FPRINTF((stderr, "%d@%ld", yys->yylrState, YY_CAST(long, yys->yyposn)));
}

/* Print YYS (possibly NULL) and its predecessors. */
static void yypstates(yyGLRState* yys)
{
  if (yys == YY_NULLPTR)
    YY_FPRINTF((stderr, "<null>"));
  else
    yy_yypstack(yys);
  YY_FPRINTF((stderr, "\n"));
}

/* Print the stack #YYK.  */
static void yypstack(yyGLRStack* yystackp, YYPTRDIFF_T yyk)
{
  yypstates(yystackp->yytops.yystates[yyk]);
}

/* Print all the stacks.  */
static void yypdumpstack(yyGLRStack* yystackp)
{
#define YYINDEX(YYX)                                                                               \
  YY_CAST(long, ((YYX) ? YY_REINTERPRET_CAST(yyGLRStackItem*, (YYX)) - yystackp->yyitems : -1))

  yyGLRStackItem* yyp;
  for (yyp = yystackp->yyitems; yyp < yystackp->yynextFree; yyp += 1)
  {
    YY_FPRINTF((stderr, "%3ld. ", YY_CAST(long, yyp - yystackp->yyitems)));
    if (*YY_REINTERPRET_CAST(yybool*, yyp))
    {
      YY_ASSERT(yyp->yystate.yyisState);
      YY_ASSERT(yyp->yyoption.yyisState);
      YY_FPRINTF((stderr, "Res: %d, LR State: %d, posn: %ld, pred: %ld", yyp->yystate.yyresolved,
        yyp->yystate.yylrState, YY_CAST(long, yyp->yystate.yyposn), YYINDEX(yyp->yystate.yypred)));
      if (!yyp->yystate.yyresolved)
        YY_FPRINTF((stderr, ", firstVal: %ld", YYINDEX(yyp->yystate.yysemantics.yyfirstVal)));
    }
    else
    {
      YY_ASSERT(!yyp->yystate.yyisState);
      YY_ASSERT(!yyp->yyoption.yyisState);
      YY_FPRINTF((stderr, "Option. rule: %d, state: %ld, next: %ld", yyp->yyoption.yyrule - 1,
        YYINDEX(yyp->yyoption.yystate), YYINDEX(yyp->yyoption.yynext)));
    }
    YY_FPRINTF((stderr, "\n"));
  }

  YY_FPRINTF((stderr, "Tops:"));
  {
    YYPTRDIFF_T yyi;
    for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
      YY_FPRINTF(
        (stderr, "%ld: %ld; ", YY_CAST(long, yyi), YYINDEX(yystackp->yytops.yystates[yyi])));
    YY_FPRINTF((stderr, "\n"));
  }
#undef YYINDEX
}
#endif

#undef yylval
#undef yychar
#undef yynerrs

// NOLINTNEXTLINE(bugprone-suspicious-include)
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

  if (getAttributes() & VTK_PARSE_MARSHALAUTO)
  {
    currentClass->MarshalType = VTK_MARSHAL_AUTO_MODE;
  }
  else if (getAttributes() & VTK_PARSE_MARSHALMANUAL)
  {
    currentClass->MarshalType = VTK_MARSHAL_MANUAL_MODE;
  }
  else
  {
    currentClass->MarshalType = VTK_MARSHAL_NONE;
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
    else
    {
      /* mark class to be deleted at end of its definition */
      currentClass->Name = NULL;
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
  if (currentClass->Name && currentClass->Name[0] != '\0')
  {
    /* add default constructors */
    vtkParse_AddDefaultConstructors(currentClass, data->Strings);
  }
  else
  {
    /* classes we had to parse but don't want to record */
    vtkParse_FreeClass(currentClass);
  }

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

    if (type && basename)
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
    const MacroInfo* macro = vtkParsePreprocess_GetMacro(preprocessor, valstring);

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

  if (func->ReturnValue)
  {
    vtkParse_FreeValue(func->ReturnValue);
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
    else if (l == 16 && strncmp(att, "vtk::propexclude", l) == 0 && !args &&
      role == VTK_PARSE_ATTRIB_DECL)
    {
      addAttribute(VTK_PARSE_PROPEXCLUDE);
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
    else if (l == 19 && strncmp(att, "vtk::unblockthreads", l) == 0 && !args &&
      role == VTK_PARSE_ATTRIB_DECL)
    {
      addAttribute(VTK_PARSE_UNBLOCKTHREADS);
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
          /* underscore by itself signifies the return value */
          if (l == 1 && args[0] == '_')
          {
            arg = currentFunction->ReturnValue;
          }
          else
          {
            print_parser_error("unrecognized parameter name", args, l);
            exit(1);
          }
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
    else if (l == 16 && strncmp(att, "vtk::marshalauto", l) == 0 && !args &&
      (role == VTK_PARSE_ATTRIB_DECL || role == VTK_PARSE_ATTRIB_CLASS))
    {
      addAttribute(VTK_PARSE_MARSHALAUTO);
    }
    else if (l == 18 && strncmp(att, "vtk::marshalmanual", l) == 0 && !args &&
      (role == VTK_PARSE_ATTRIB_DECL || role == VTK_PARSE_ATTRIB_CLASS))
    {
      addAttribute(VTK_PARSE_MARSHALMANUAL);
    }
    else if (l == 19 && strncmp(att, "vtk::marshalexclude", l) == 0 && args &&
      role == VTK_PARSE_ATTRIB_DECL)
    {
      currentFunction->IsMarshalExcluded = 1;
      currentFunction->MarshalExcludeReason = vtkstrndup(args, la);
    }
    else if (l == 18 && (strncmp(att, "vtk::marshalgetter", l) == 0 ||
      strncmp(att, "vtk::marshalsetter", l) == 0) && args &&
      role == VTK_PARSE_ATTRIB_DECL)
    {
      if (args[0] != '"')
      {
        print_parser_error("args were not quoted here! Check macro definition in vtkWrappingHints.h", att, l);
        exit(1);
      }
      /* advance args to next attribute arg */
      while ((args[0] == '"') || (args[0] == ' '))
      {
        args += 1;
        la -= 1;
      }
      size_t n = vtkParse_SkipId(args);
      currentFunction->MarshalPropertyName = vtkstrndup(args, n);
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

    if (currentFunction->ReturnValue->Attributes & VTK_PARSE_PROPEXCLUDE)
    {
      /* remove "propexclude" attrib from ReturnValue, attach it to function */
      currentFunction->ReturnValue->Attributes ^= VTK_PARSE_PROPEXCLUDE;
      currentFunction->IsPropExcluded = 1;
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
    }
    else
    {
      vtkParse_FreeFunction(currentFunction);
    }

    currentFunction = (FunctionInfo*)malloc(sizeof(FunctionInfo));
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
  clearTemplate();

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

  vtkParse_FreeFunction(currentFunction);
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

  free(IncludeDirectories);
  IncludeDirectories = NULL;
  NumberOfIncludeDirectories = 0;

  free(Definitions);
  Definitions = NULL;
  NumberOfDefinitions = 0;

  free(MacroIncludes);
  MacroIncludes = NULL;
  NumberOfMacroIncludes = 0;
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
  cp = vtkParse_NewString(&system_strings, l);
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

  cp = vtkParse_NewString(&system_strings, n + 1);
  cp[0] = 'U';
  strncpy(&cp[1], name, n);
  cp[n + 1] = '\0';

  vtkParse_AddStringToArray(&Definitions, &NumberOfDefinitions, cp);
}

/** Do not define any platform-specific macros.  */
void vtkParse_UndefinePlatformMacros(void)
{
  PredefinePlatformMacros = 0;
}

/** Add an include file to read macros from, for use with -imacro. */
void vtkParse_IncludeMacros(const char* filename)
{
  const char* cp;

  cp = vtkParse_CacheString(&system_strings, filename, strlen(filename));
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
  const char* cp;
  int i;

  for (i = 0; i < NumberOfIncludeDirectories; i++)
  {
    if (strncmp(IncludeDirectories[i], dirname, n) == 0 && IncludeDirectories[i][n] == '\0')
    {
      return;
    }
  }

  cp = vtkParse_CacheString(&system_strings, dirname, n);
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
