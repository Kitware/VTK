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

// NOLINTBEGIN(bugprone-unsafe-functions)
// NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)
/*

The file 'vtkParse.tab.c' is generated from 'vtkParse.y'.

See the adjacent README.md for instructions.

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
#include "vtkParseAttributes.h"
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
  FunctionInfo* func, unsigned int type, const char* typeclass, int n, const char** attrs);
static void add_template_parameter(unsigned int datatype, unsigned int extra, const char* funcSig);
static void add_using(const char* name, int is_namespace);
static void start_enum(const char* name, int is_scoped, unsigned int type, const char* basename);
static void add_enum(const char* name, const char* value);
static void end_enum(void);
static unsigned int guess_constant_type(const char* valstring);
static void add_constant(const char* name, const char* value, unsigned int type,
  const char* typeclass, int n, const char** attrs, int flag);
static unsigned int guess_id_type(const char* cp);
static unsigned int add_indirection(unsigned int type1, unsigned int type2);
static unsigned int add_indirection_to_array(unsigned int type);
static void handle_complex_type(ValueInfo* val, int n, const char** attrs, unsigned int datatype,
  unsigned int extra, const char* funcSig);
static void handle_attribute(const char* attr, int pack);
static void handle_attribute_error(const char* attr, parse_attribute_return_t rcode);
static void handle_decl_attributes(FunctionInfo* func, int n, const char** attrs);
static void handle_value_attributes(ValueInfo* func, int n, const char** attrs);
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
static int declAttrCount = 0;
static int idAttrCount = 0;
static int classAttrCount = 0;
static const char** declAttrs = NULL;
static const char** idAttrs = NULL;
static const char** classAttrs = NULL;
static struct
{
  const char** attrs;
  int n;
} declAttrStack[10];
static int typeDepth = 0;

/* save the type on the stack */
static void pushType(void)
{
  declAttrStack[typeDepth].attrs = declAttrs;
  declAttrStack[typeDepth].n = declAttrCount;
  typeStack[typeDepth++] = storedType;
  declAttrs = NULL;
  declAttrCount = 0;
  storedType = 0;
}

/* pop the type stack */
static void popType(void)
{
  storedType = typeStack[--typeDepth];
  free(declAttrs);
  declAttrs = declAttrStack[typeDepth].attrs;
  declAttrCount = declAttrStack[typeDepth].n;
}

/* clear the storage type */
static void clearType(void)
{
  storedType = 0;
  free(declAttrs);
  declAttrs = NULL;
  declAttrCount = 0;
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
static void addDeclAttr(const char* att)
{
  vtkParse_AddStringToArray(&declAttrs, &declAttrCount, att);
}

/* get the number of attributes for the current declaration */
static int getDeclAttrCount(void)
{
  return declAttrCount;
}

/* get the attributes for the current declaration */
static const char** getDeclAttrs(void)
{
  return declAttrs;
}

/* add an attribute specifier for the preceding identifier */
static void addIdAttr(const char* att)
{
  vtkParse_AddStringToArray(&idAttrs, &idAttrCount, att);
}

/* get the number of attributes for the preceding identifier */
static int getIdAttrCount(void)
{
  return idAttrCount;
}

/* get the attributes for the preceding identifier */
static const char** getIdAttrs(void)
{
  return idAttrs;
}

/* clear the current set of identifier attributes */
static void clearIdAttrs(void)
{
  free(idAttrs);
  idAttrs = NULL;
  idAttrCount = 0;
}

/* add an attribute specifier for the current class */
static void addClassAttr(const char* att)
{
  vtkParse_AddStringToArray(&classAttrs, &classAttrCount, att);
}

/* get the number of attributes for the current class */
static int getClassAttrCount(void)
{
  return classAttrCount;
}

/* get the attributes for the current class */
static const char** getClassAttrs(void)
{
  return classAttrs;
}

/* clear the current set of class attributes */
static void clearClassAttrs(void)
{
  free(classAttrs);
  classAttrs = NULL;
  classAttrCount = 0;
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
  YYSYMBOL_using_declarator_list = 184,                 /* using_declarator_list  */
  YYSYMBOL_using_declarator_list_cont = 185,            /* using_declarator_list_cont  */
  YYSYMBOL_using_declarator = 186,                      /* using_declarator  */
  YYSYMBOL_using_id = 187,                              /* using_id  */
  YYSYMBOL_using_directive = 188,                       /* using_directive  */
  YYSYMBOL_alias_declaration = 189,                     /* alias_declaration  */
  YYSYMBOL_190_12 = 190,                                /* $@12  */
  YYSYMBOL_template_head = 191,                         /* template_head  */
  YYSYMBOL_192_13 = 192,                                /* $@13  */
  YYSYMBOL_193_14 = 193,                                /* $@14  */
  YYSYMBOL_194_15 = 194,                                /* $@15  */
  YYSYMBOL_template_parameter_list = 195,               /* template_parameter_list  */
  YYSYMBOL_196_16 = 196,                                /* $@16  */
  YYSYMBOL_template_parameter = 197,                    /* template_parameter  */
  YYSYMBOL_198_17 = 198,                                /* $@17  */
  YYSYMBOL_199_18 = 199,                                /* $@18  */
  YYSYMBOL_200_19 = 200,                                /* $@19  */
  YYSYMBOL_201_20 = 201,                                /* $@20  */
  YYSYMBOL_202_21 = 202,                                /* $@21  */
  YYSYMBOL_203_22 = 203,                                /* $@22  */
  YYSYMBOL_opt_ellipsis = 204,                          /* opt_ellipsis  */
  YYSYMBOL_class_or_typename = 205,                     /* class_or_typename  */
  YYSYMBOL_opt_template_parameter_initializer = 206,    /* opt_template_parameter_initializer  */
  YYSYMBOL_template_parameter_initializer = 207,        /* template_parameter_initializer  */
  YYSYMBOL_208_23 = 208,                                /* $@23  */
  YYSYMBOL_template_parameter_value = 209,              /* template_parameter_value  */
  YYSYMBOL_function_definition = 210,                   /* function_definition  */
  YYSYMBOL_function_declaration = 211,                  /* function_declaration  */
  YYSYMBOL_nested_method_declaration = 212,             /* nested_method_declaration  */
  YYSYMBOL_nested_operator_declaration = 213,           /* nested_operator_declaration  */
  YYSYMBOL_method_definition = 214,                     /* method_definition  */
  YYSYMBOL_method_declaration = 215,                    /* method_declaration  */
  YYSYMBOL_operator_declaration = 216,                  /* operator_declaration  */
  YYSYMBOL_conversion_function = 217,                   /* conversion_function  */
  YYSYMBOL_218_24 = 218,                                /* $@24  */
  YYSYMBOL_219_25 = 219,                                /* $@25  */
  YYSYMBOL_conversion_function_id = 220,                /* conversion_function_id  */
  YYSYMBOL_operator_function_nr = 221,                  /* operator_function_nr  */
  YYSYMBOL_operator_function_sig = 222,                 /* operator_function_sig  */
  YYSYMBOL_223_26 = 223,                                /* $@26  */
  YYSYMBOL_operator_function_id = 224,                  /* operator_function_id  */
  YYSYMBOL_operator_sig = 225,                          /* operator_sig  */
  YYSYMBOL_function_nr = 226,                           /* function_nr  */
  YYSYMBOL_function_trailer_clause = 227,               /* function_trailer_clause  */
  YYSYMBOL_func_cv_qualifier_seq = 228,                 /* func_cv_qualifier_seq  */
  YYSYMBOL_func_cv_qualifier = 229,                     /* func_cv_qualifier  */
  YYSYMBOL_opt_noexcept_specifier = 230,                /* opt_noexcept_specifier  */
  YYSYMBOL_noexcept_sig = 231,                          /* noexcept_sig  */
  YYSYMBOL_opt_ref_qualifier = 232,                     /* opt_ref_qualifier  */
  YYSYMBOL_virt_specifier_seq = 233,                    /* virt_specifier_seq  */
  YYSYMBOL_virt_specifier = 234,                        /* virt_specifier  */
  YYSYMBOL_opt_body_as_trailer = 235,                   /* opt_body_as_trailer  */
  YYSYMBOL_opt_trailing_return_type = 236,              /* opt_trailing_return_type  */
  YYSYMBOL_trailing_return_type = 237,                  /* trailing_return_type  */
  YYSYMBOL_238_27 = 238,                                /* $@27  */
  YYSYMBOL_function_body = 239,                         /* function_body  */
  YYSYMBOL_function_try_block = 240,                    /* function_try_block  */
  YYSYMBOL_handler_seq = 241,                           /* handler_seq  */
  YYSYMBOL_function_sig = 242,                          /* function_sig  */
  YYSYMBOL_243_28 = 243,                                /* $@28  */
  YYSYMBOL_structor_declaration = 244,                  /* structor_declaration  */
  YYSYMBOL_245_29 = 245,                                /* $@29  */
  YYSYMBOL_246_30 = 246,                                /* $@30  */
  YYSYMBOL_structor_sig = 247,                          /* structor_sig  */
  YYSYMBOL_248_31 = 248,                                /* $@31  */
  YYSYMBOL_opt_ctor_initializer = 249,                  /* opt_ctor_initializer  */
  YYSYMBOL_mem_initializer_list = 250,                  /* mem_initializer_list  */
  YYSYMBOL_mem_initializer = 251,                       /* mem_initializer  */
  YYSYMBOL_parameter_declaration_clause = 252,          /* parameter_declaration_clause  */
  YYSYMBOL_253_32 = 253,                                /* $@32  */
  YYSYMBOL_parameter_list = 254,                        /* parameter_list  */
  YYSYMBOL_255_33 = 255,                                /* $@33  */
  YYSYMBOL_parameter_declaration = 256,                 /* parameter_declaration  */
  YYSYMBOL_257_34 = 257,                                /* $@34  */
  YYSYMBOL_258_35 = 258,                                /* $@35  */
  YYSYMBOL_opt_initializer = 259,                       /* opt_initializer  */
  YYSYMBOL_initializer = 260,                           /* initializer  */
  YYSYMBOL_261_36 = 261,                                /* $@36  */
  YYSYMBOL_262_37 = 262,                                /* $@37  */
  YYSYMBOL_263_38 = 263,                                /* $@38  */
  YYSYMBOL_constructor_args = 264,                      /* constructor_args  */
  YYSYMBOL_265_39 = 265,                                /* $@39  */
  YYSYMBOL_variable_declaration = 266,                  /* variable_declaration  */
  YYSYMBOL_init_declarator_id = 267,                    /* init_declarator_id  */
  YYSYMBOL_opt_declarator_list = 268,                   /* opt_declarator_list  */
  YYSYMBOL_declarator_list_cont = 269,                  /* declarator_list_cont  */
  YYSYMBOL_270_40 = 270,                                /* $@40  */
  YYSYMBOL_init_declarator = 271,                       /* init_declarator  */
  YYSYMBOL_opt_ptr_operator_seq = 272,                  /* opt_ptr_operator_seq  */
  YYSYMBOL_direct_abstract_declarator = 273,            /* direct_abstract_declarator  */
  YYSYMBOL_274_41 = 274,                                /* $@41  */
  YYSYMBOL_direct_declarator = 275,                     /* direct_declarator  */
  YYSYMBOL_276_42 = 276,                                /* $@42  */
  YYSYMBOL_lp_or_la = 277,                              /* lp_or_la  */
  YYSYMBOL_278_43 = 278,                                /* $@43  */
  YYSYMBOL_opt_array_or_parameters = 279,               /* opt_array_or_parameters  */
  YYSYMBOL_280_44 = 280,                                /* $@44  */
  YYSYMBOL_281_45 = 281,                                /* $@45  */
  YYSYMBOL_function_qualifiers = 282,                   /* function_qualifiers  */
  YYSYMBOL_abstract_declarator = 283,                   /* abstract_declarator  */
  YYSYMBOL_declarator = 284,                            /* declarator  */
  YYSYMBOL_opt_declarator_id = 285,                     /* opt_declarator_id  */
  YYSYMBOL_declarator_id = 286,                         /* declarator_id  */
  YYSYMBOL_bitfield_size = 287,                         /* bitfield_size  */
  YYSYMBOL_opt_array_decorator_seq = 288,               /* opt_array_decorator_seq  */
  YYSYMBOL_array_decorator_seq = 289,                   /* array_decorator_seq  */
  YYSYMBOL_290_46 = 290,                                /* $@46  */
  YYSYMBOL_array_decorator_seq_impl = 291,              /* array_decorator_seq_impl  */
  YYSYMBOL_array_decorator = 292,                       /* array_decorator  */
  YYSYMBOL_293_47 = 293,                                /* $@47  */
  YYSYMBOL_array_size_specifier = 294,                  /* array_size_specifier  */
  YYSYMBOL_295_48 = 295,                                /* $@48  */
  YYSYMBOL_id_expression = 296,                         /* id_expression  */
  YYSYMBOL_unqualified_id = 297,                        /* unqualified_id  */
  YYSYMBOL_qualified_id = 298,                          /* qualified_id  */
  YYSYMBOL_nested_name_specifier = 299,                 /* nested_name_specifier  */
  YYSYMBOL_300_49 = 300,                                /* $@49  */
  YYSYMBOL_tilde_sig = 301,                             /* tilde_sig  */
  YYSYMBOL_identifier_sig = 302,                        /* identifier_sig  */
  YYSYMBOL_scope_operator_sig = 303,                    /* scope_operator_sig  */
  YYSYMBOL_template_id = 304,                           /* template_id  */
  YYSYMBOL_305_50 = 305,                                /* $@50  */
  YYSYMBOL_decltype_specifier = 306,                    /* decltype_specifier  */
  YYSYMBOL_307_51 = 307,                                /* $@51  */
  YYSYMBOL_simple_id = 308,                             /* simple_id  */
  YYSYMBOL_identifier = 309,                            /* identifier  */
  YYSYMBOL_opt_decl_specifier_seq = 310,                /* opt_decl_specifier_seq  */
  YYSYMBOL_decl_specifier2 = 311,                       /* decl_specifier2  */
  YYSYMBOL_decl_specifier_seq = 312,                    /* decl_specifier_seq  */
  YYSYMBOL_decl_specifier = 313,                        /* decl_specifier  */
  YYSYMBOL_storage_class_specifier = 314,               /* storage_class_specifier  */
  YYSYMBOL_function_specifier = 315,                    /* function_specifier  */
  YYSYMBOL_cv_qualifier = 316,                          /* cv_qualifier  */
  YYSYMBOL_cv_qualifier_seq = 317,                      /* cv_qualifier_seq  */
  YYSYMBOL_store_type = 318,                            /* store_type  */
  YYSYMBOL_store_type_specifier = 319,                  /* store_type_specifier  */
  YYSYMBOL_320_52 = 320,                                /* $@52  */
  YYSYMBOL_321_53 = 321,                                /* $@53  */
  YYSYMBOL_type_specifier = 322,                        /* type_specifier  */
  YYSYMBOL_trailing_type_specifier = 323,               /* trailing_type_specifier  */
  YYSYMBOL_324_54 = 324,                                /* $@54  */
  YYSYMBOL_trailing_type_specifier_seq = 325,           /* trailing_type_specifier_seq  */
  YYSYMBOL_trailing_type_specifier_seq2 = 326,          /* trailing_type_specifier_seq2  */
  YYSYMBOL_327_55 = 327,                                /* $@55  */
  YYSYMBOL_328_56 = 328,                                /* $@56  */
  YYSYMBOL_tparam_type = 329,                           /* tparam_type  */
  YYSYMBOL_tparam_type_specifier2 = 330,                /* tparam_type_specifier2  */
  YYSYMBOL_331_57 = 331,                                /* $@57  */
  YYSYMBOL_332_58 = 332,                                /* $@58  */
  YYSYMBOL_tparam_type_specifier = 333,                 /* tparam_type_specifier  */
  YYSYMBOL_simple_type_specifier = 334,                 /* simple_type_specifier  */
  YYSYMBOL_type_name = 335,                             /* type_name  */
  YYSYMBOL_primitive_type = 336,                        /* primitive_type  */
  YYSYMBOL_ptr_operator_seq = 337,                      /* ptr_operator_seq  */
  YYSYMBOL_reference = 338,                             /* reference  */
  YYSYMBOL_rvalue_reference = 339,                      /* rvalue_reference  */
  YYSYMBOL_pointer = 340,                               /* pointer  */
  YYSYMBOL_341_59 = 341,                                /* $@59  */
  YYSYMBOL_ptr_cv_qualifier_seq = 342,                  /* ptr_cv_qualifier_seq  */
  YYSYMBOL_pointer_seq = 343,                           /* pointer_seq  */
  YYSYMBOL_decl_attribute_specifier_seq = 344,          /* decl_attribute_specifier_seq  */
  YYSYMBOL_345_60 = 345,                                /* $@60  */
  YYSYMBOL_id_attribute_specifier_seq = 346,            /* id_attribute_specifier_seq  */
  YYSYMBOL_347_61 = 347,                                /* $@61  */
  YYSYMBOL_ref_attribute_specifier_seq = 348,           /* ref_attribute_specifier_seq  */
  YYSYMBOL_349_62 = 349,                                /* $@62  */
  YYSYMBOL_func_attribute_specifier_seq = 350,          /* func_attribute_specifier_seq  */
  YYSYMBOL_351_63 = 351,                                /* $@63  */
  YYSYMBOL_array_attribute_specifier_seq = 352,         /* array_attribute_specifier_seq  */
  YYSYMBOL_353_64 = 353,                                /* $@64  */
  YYSYMBOL_class_attribute_specifier_seq = 354,         /* class_attribute_specifier_seq  */
  YYSYMBOL_355_65 = 355,                                /* $@65  */
  YYSYMBOL_attribute_specifier_seq = 356,               /* attribute_specifier_seq  */
  YYSYMBOL_attribute_specifier = 357,                   /* attribute_specifier  */
  YYSYMBOL_attribute_specifier_contents = 358,          /* attribute_specifier_contents  */
  YYSYMBOL_attribute_using_prefix = 359,                /* attribute_using_prefix  */
  YYSYMBOL_attribute_list = 360,                        /* attribute_list  */
  YYSYMBOL_attribute = 361,                             /* attribute  */
  YYSYMBOL_362_66 = 362,                                /* $@66  */
  YYSYMBOL_attribute_pack = 363,                        /* attribute_pack  */
  YYSYMBOL_attribute_sig = 364,                         /* attribute_sig  */
  YYSYMBOL_attribute_token = 365,                       /* attribute_token  */
  YYSYMBOL_operator_id = 366,                           /* operator_id  */
  YYSYMBOL_operator_id_no_delim = 367,                  /* operator_id_no_delim  */
  YYSYMBOL_keyword = 368,                               /* keyword  */
  YYSYMBOL_literal = 369,                               /* literal  */
  YYSYMBOL_constant_expression = 370,                   /* constant_expression  */
  YYSYMBOL_constant_expression_item = 371,              /* constant_expression_item  */
  YYSYMBOL_372_67 = 372,                                /* $@67  */
  YYSYMBOL_common_bracket_item = 373,                   /* common_bracket_item  */
  YYSYMBOL_common_bracket_item_no_scope_operator = 374, /* common_bracket_item_no_scope_operator  */
  YYSYMBOL_any_bracket_contents = 375,                  /* any_bracket_contents  */
  YYSYMBOL_bracket_pitem = 376,                         /* bracket_pitem  */
  YYSYMBOL_any_bracket_item = 377,                      /* any_bracket_item  */
  YYSYMBOL_braces_item = 378,                           /* braces_item  */
  YYSYMBOL_angle_bracket_contents = 379,                /* angle_bracket_contents  */
  YYSYMBOL_braces_contents = 380,                       /* braces_contents  */
  YYSYMBOL_angle_bracket_pitem = 381,                   /* angle_bracket_pitem  */
  YYSYMBOL_angle_bracket_item = 382,                    /* angle_bracket_item  */
  YYSYMBOL_angle_brackets_sig = 383,                    /* angle_brackets_sig  */
  YYSYMBOL_384_68 = 384,                                /* $@68  */
  YYSYMBOL_right_angle_bracket = 385,                   /* right_angle_bracket  */
  YYSYMBOL_brackets_sig = 386,                          /* brackets_sig  */
  YYSYMBOL_387_69 = 387,                                /* $@69  */
  YYSYMBOL_388_70 = 388,                                /* $@70  */
  YYSYMBOL_parentheses_sig = 389,                       /* parentheses_sig  */
  YYSYMBOL_390_71 = 390,                                /* $@71  */
  YYSYMBOL_391_72 = 391,                                /* $@72  */
  YYSYMBOL_392_73 = 392,                                /* $@73  */
  YYSYMBOL_braces_sig = 393,                            /* braces_sig  */
  YYSYMBOL_394_74 = 394,                                /* $@74  */
  YYSYMBOL_ignored_items = 395,                         /* ignored_items  */
  YYSYMBOL_ignored_expression = 396,                    /* ignored_expression  */
  YYSYMBOL_ignored_item = 397,                          /* ignored_item  */
  YYSYMBOL_ignored_item_no_semi = 398,                  /* ignored_item_no_semi  */
  YYSYMBOL_ignored_item_no_angle = 399,                 /* ignored_item_no_angle  */
  YYSYMBOL_ignored_braces = 400,                        /* ignored_braces  */
  YYSYMBOL_ignored_brackets = 401,                      /* ignored_brackets  */
  YYSYMBOL_ignored_parentheses = 402,                   /* ignored_parentheses  */
  YYSYMBOL_ignored_left_parenthesis = 403               /* ignored_left_parenthesis  */
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
#define YYLAST 6502

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 124
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 280
/* YYNRULES -- Number of rules.  */
#define YYNRULES 679
/* YYNSTATES -- Number of states.  */
#define YYNSTATES 1058
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
static const yytype_int16 yyrline[] = { 0, 1848, 1848, 1858, 1860, 1859, 1870, 1871, 1872, 1873,
  1874, 1875, 1876, 1877, 1878, 1879, 1880, 1881, 1882, 1883, 1884, 1885, 1886, 1889, 1890, 1891,
  1892, 1893, 1894, 1897, 1898, 1905, 1912, 1913, 1913, 1915, 1918, 1925, 1926, 1929, 1930, 1931,
  1934, 1935, 1938, 1938, 1953, 1952, 1958, 1964, 1963, 1968, 1974, 1975, 1976, 1979, 1981, 1983,
  1986, 1987, 1990, 1991, 1993, 1995, 1994, 2003, 2007, 2008, 2009, 2012, 2013, 2014, 2015, 2016,
  2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2028, 2029, 2030, 2031, 2032, 2033, 2036,
  2037, 2038, 2039, 2043, 2044, 2047, 2049, 2052, 2057, 2058, 2061, 2062, 2065, 2066, 2067, 2078,
  2079, 2080, 2084, 2085, 2089, 2089, 2102, 2109, 2118, 2119, 2120, 2123, 2124, 2124, 2128, 2129,
  2131, 2132, 2133, 2133, 2141, 2145, 2146, 2149, 2151, 2153, 2154, 2157, 2158, 2166, 2167, 2170,
  2171, 2173, 2175, 2177, 2181, 2183, 2184, 2187, 2190, 2191, 2194, 2195, 2194, 2199, 2240, 2243,
  2245, 2246, 2249, 2252, 2253, 2255, 2257, 2259, 2261, 2265, 2268, 2268, 2301, 2300, 2304, 2312,
  2303, 2322, 2324, 2323, 2328, 2330, 2328, 2332, 2334, 2332, 2336, 2339, 2336, 2350, 2351, 2354,
  2355, 2357, 2358, 2361, 2361, 2371, 2372, 2380, 2381, 2382, 2383, 2386, 2389, 2390, 2391, 2394,
  2395, 2396, 2399, 2400, 2401, 2405, 2406, 2407, 2408, 2411, 2412, 2413, 2417, 2422, 2416, 2434,
  2438, 2449, 2448, 2457, 2461, 2464, 2474, 2478, 2479, 2482, 2483, 2485, 2486, 2487, 2490, 2491,
  2493, 2494, 2495, 2497, 2498, 2501, 2514, 2515, 2516, 2517, 2524, 2525, 2528, 2528, 2536, 2537,
  2538, 2541, 2543, 2544, 2548, 2547, 2564, 2581, 2560, 2592, 2592, 2595, 2596, 2599, 2600, 2603,
  2604, 2610, 2611, 2611, 2614, 2615, 2615, 2617, 2619, 2623, 2625, 2623, 2649, 2650, 2653, 2653,
  2655, 2655, 2657, 2657, 2662, 2663, 2663, 2671, 2674, 2745, 2746, 2748, 2749, 2749, 2752, 2755,
  2756, 2760, 2772, 2771, 2793, 2795, 2795, 2816, 2816, 2818, 2822, 2823, 2824, 2823, 2829, 2831,
  2832, 2833, 2834, 2835, 2836, 2839, 2840, 2844, 2845, 2849, 2850, 2853, 2854, 2858, 2859, 2860,
  2861, 2864, 2865, 2868, 2868, 2871, 2872, 2875, 2875, 2879, 2880, 2880, 2887, 2888, 2891, 2892,
  2893, 2894, 2895, 2898, 2900, 2902, 2906, 2908, 2910, 2912, 2914, 2916, 2918, 2918, 2923, 2926,
  2929, 2932, 2932, 2940, 2940, 2949, 2950, 2951, 2952, 2953, 2954, 2955, 2956, 2957, 2964, 2965,
  2966, 2967, 2968, 2969, 2975, 2976, 2979, 2980, 2982, 2983, 2986, 2987, 2990, 2991, 2992, 2993,
  2996, 2997, 2998, 2999, 3000, 3004, 3005, 3006, 3009, 3010, 3013, 3014, 3022, 3025, 3025, 3027,
  3027, 3031, 3032, 3034, 3038, 3039, 3041, 3041, 3044, 3046, 3050, 3053, 3053, 3055, 3055, 3059,
  3062, 3062, 3064, 3064, 3068, 3069, 3071, 3073, 3075, 3077, 3079, 3083, 3084, 3087, 3088, 3089,
  3090, 3091, 3092, 3093, 3094, 3095, 3098, 3099, 3100, 3101, 3102, 3103, 3104, 3105, 3106, 3107,
  3108, 3109, 3110, 3111, 3131, 3132, 3133, 3134, 3137, 3141, 3145, 3145, 3149, 3150, 3165, 3166,
  3191, 3191, 3195, 3195, 3199, 3199, 3203, 3203, 3207, 3207, 3211, 3211, 3214, 3215, 3218, 3222,
  3223, 3226, 3229, 3230, 3231, 3232, 3235, 3235, 3239, 3240, 3243, 3244, 3247, 3248, 3255, 3256,
  3257, 3258, 3259, 3260, 3261, 3262, 3263, 3264, 3265, 3266, 3267, 3270, 3271, 3272, 3273, 3274,
  3275, 3276, 3277, 3278, 3279, 3280, 3281, 3282, 3283, 3284, 3285, 3286, 3287, 3288, 3289, 3290,
  3291, 3292, 3293, 3294, 3295, 3296, 3297, 3298, 3299, 3300, 3301, 3302, 3303, 3306, 3307, 3308,
  3309, 3310, 3311, 3312, 3313, 3314, 3315, 3316, 3317, 3318, 3319, 3320, 3321, 3322, 3323, 3324,
  3325, 3326, 3327, 3328, 3329, 3330, 3331, 3332, 3333, 3334, 3335, 3338, 3339, 3340, 3341, 3342,
  3343, 3344, 3345, 3346, 3347, 3354, 3355, 3358, 3359, 3360, 3361, 3361, 3362, 3365, 3366, 3369,
  3370, 3371, 3372, 3408, 3408, 3409, 3410, 3411, 3412, 3413, 3415, 3416, 3419, 3420, 3421, 3422,
  3425, 3426, 3427, 3430, 3431, 3433, 3434, 3436, 3437, 3440, 3441, 3444, 3445, 3446, 3450, 3449,
  3463, 3464, 3467, 3467, 3469, 3469, 3473, 3473, 3475, 3475, 3477, 3477, 3481, 3481, 3486, 3487,
  3489, 3490, 3493, 3494, 3497, 3498, 3501, 3502, 3503, 3504, 3505, 3506, 3507, 3508, 3508, 3508,
  3508, 3508, 3509, 3510, 3511, 3512, 3513, 3516, 3519, 3520, 3523, 3526, 3526, 3526 };
#endif

#define YYPACT_NINF (-792)
#define YYTABLE_NINF (-633)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] = { -792, 101, 114, -792, -792, 1664, -792, 202, 252, 253, 404,
  407, 449, 128, 197, 224, -792, -792, -792, 264, -792, -792, -792, -792, -792, -792, -792, 116,
  -792, 118, -792, 3608, -792, -792, 6086, 616, 1308, -792, -792, -792, -792, -792, -792, -792,
  -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792,
  -792, -792, -792, -792, -792, -792, -792, -792, -792, -8, -792, -792, -792, -792, -792, -792,
  5792, -792, 78, 78, 78, 78, -792, 15, 6086, -792, 31, -792, 52, 1438, 1848, 131, 1271, 267, 271,
  -792, 133, 5890, -792, -792, -792, -792, 107, 237, -792, -792, -792, -792, -792, 225, -792, -792,
  1068, 158, 4455, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792,
  -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792,
  -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792,
  -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792,
  -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792,
  -792, -792, -792, -792, -792, -792, -792, -792, 40, -792, -792, -792, -792, -792, -792, -792,
  -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792,
  97, 1271, 54, 148, 168, 182, 209, 223, -792, 257, -792, -792, -792, -792, -792, 742, 131, 131,
  6086, 107, -792, -792, -792, -792, -792, -792, -792, 174, 54, 148, 168, 182, 209, 223, -792, -792,
  -792, 1271, 1271, 222, -792, -792, 50, -792, 1438, 1271, 131, 131, 6305, 228, 5360, -792, 6305,
  -792, 1403, 230, 1271, -792, -792, -792, -792, -792, -792, 5792, -792, -792, 5988, 107, 293, -792,
  -792, -792, -792, -792, -792, -792, -792, -792, 6086, -792, -792, -792, -792, -792, -792, 448,
  299, 131, 131, 131, -792, -792, -792, -792, 133, -792, -792, -792, -792, -792, -792, -792, -792,
  -792, -792, -792, -792, -792, 1438, -792, -792, -792, -792, -792, -792, 1782, -792, 383, 303,
  -792, -792, -792, -792, -792, -792, -792, -792, 259, -792, -792, -792, 70, -792, 324, -792, -792,
  1914, 2035, -792, -792, 341, -792, 2156, 3003, 2277, -792, -792, -792, -792, -792, -792, 6388,
  1469, 6388, 1722, -792, -792, -792, -792, -792, -792, 733, -792, 2398, 1082, 334, -792, 314, -792,
  331, 353, -792, -792, -792, 5181, 1438, -792, -792, 374, -792, 107, -792, -792, -792, -792, -792,
  -792, 72, -792, 1842, 565, 131, 131, 225, 387, 1412, -792, -792, -792, 442, -792, 1271, 5988,
  1782, 1271, 397, 2519, 395, 1656, 1068, -792, -792, -792, 97, -792, -792, -792, -792, -792, 6305,
  1469, 6305, 1722, -792, -792, -792, -792, 413, -792, 436, -792, 1516, -792, 436, 411, -792, 1438,
  60, -792, -792, -792, 425, 426, 733, -792, 440, 107, -792, -792, -792, -792, -792, -792, 5868,
  1491, 475, 313, 455, -792, 1068, -792, 456, 3124, -792, -792, 486, -792, -792, -792, -792, 144,
  -792, 6184, 294, 521, -792, -792, -792, -792, -792, -792, -792, -792, -792, 501, -792, 107, 73,
  503, 424, 6388, 6388, 278, 300, -792, -792, -792, -792, 506, 131, -792, -792, 1491, -792, 225,
  605, -792, 498, 500, 71, -792, -792, 505, -792, 504, -792, -792, -792, -792, -792, -792, 509,
  -792, -792, 90, 646, -792, -792, 512, -792, -792, 131, 131, 1842, -792, -792, -792, -792, -792,
  -792, -792, 443, -792, -792, 6086, 525, -792, -792, 1438, 522, -792, 137, -792, -792, 519, 548,
  -792, 131, -792, -792, -792, 395, 4576, 532, 94, 533, 442, 5868, -792, 413, -792, -792, -792,
  -792, 30, -792, 530, -792, -792, -792, 527, 281, -792, -792, -792, -792, -792, 4818, -792, -792,
  5299, -792, -792, 225, 413, 534, -792, 529, 455, 317, 131, -792, 561, 97, 539, -792, -792, -792,
  -792, -792, 1271, 1271, 1271, -792, 131, 131, 6086, 107, 237, -792, -792, -792, -792, 107, 294,
  -792, 3729, 3850, 3971, -792, 540, -792, -792, -792, 547, 556, -792, 237, -792, 550, -792, 555,
  -792, 6086, -792, 549, 552, -792, -792, -792, -792, -792, -792, -792, -792, -792, 566, -792, -792,
  -792, 554, 553, -792, 629, 586, -792, -792, -792, -792, 1412, 568, -792, -792, 416, 1271, 586,
  586, 2640, -792, -792, 567, -792, -792, -792, 671, 225, 570, -792, -792, -792, -792, -792, -792,
  -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792,
  -792, -792, -792, -792, 581, -792, -792, -792, 448, -792, -792, 527, -792, 580, -792, 574, 237,
  -792, 4697, -792, 4818, -792, -792, -792, -792, 447, -792, 307, -792, -792, -792, -792, 1068,
  -792, -792, -792, -792, 341, -792, -792, -792, -792, -792, 733, -792, -792, -792, -792, -792, 107,
  -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, 395, -792,
  107, -792, -792, 5301, -792, 1271, -792, -792, -792, 1271, -792, 646, -792, -792, -792, -792, 583,
  -792, -792, -792, -792, -792, 436, 604, 6086, -792, -792, 293, -792, -792, -792, -792, -792, -792,
  395, 577, -792, -792, -792, -792, -792, -792, 395, -792, 5060, -792, 3245, -792, -792, -792, -792,
  -792, -792, -792, -792, -792, 347, -792, 587, 303, 5868, 587, -792, 582, 592, -792, 127, 1491,
  -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, 5400, -792, 78, -792,
  -792, -792, 596, 299, 1438, 5498, 107, 586, 646, 586, 553, 4818, 4092, -792, 652, -792, -792,
  -792, 107, -792, 4213, 4576, 4334, 634, 594, 591, 4818, 598, -792, -792, -792, -792, -792, 4818,
  395, 5868, -792, -792, -792, -792, -792, 599, 107, -792, 587, -792, -792, 5596, -792, -792, -792,
  -792, 5400, -792, -792, 299, 5694, -792, -792, -792, -792, 1438, 1782, -792, -792, -792, 4818,
  103, -792, -792, 606, 600, -792, -792, -792, -792, -792, -792, -792, 4818, -792, 4818, 603, 4939,
  -792, -792, -792, -792, -792, -792, -792, 1310, 78, 5694, 586, 5694, 612, -792, -792, 613, 383,
  81, -792, -792, 6282, 80, -792, -792, -792, 4939, -792, 369, 558, 1788, -792, -792, 1310, -792,
  -792, 1782, -792, 615, -792, -792, -792, -792, -792, 6282, -792, -792, 237, -792, 225, -792, -792,
  -792, -792, -792, 103, 117, -792, -792, 125, -792, 1788, -792, 5556, -792, 2761, -792, -792, -792,
  558, -792, -792, 2882, 3366, 474, 84, 5556, 146, -792, -792, -792, 5868, -792, -792, -792, -792,
  130, 474, 5868, 3487, -792, -792 };

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] = { 3, 0, 4, 1, 474, 0, 486, 442, 443, 444, 439, 440, 441, 446,
  447, 445, 53, 52, 54, 114, 402, 403, 394, 397, 398, 400, 401, 399, 393, 395, 222, 0, 365, 416, 0,
  0, 0, 362, 448, 449, 450, 451, 452, 457, 458, 459, 453, 454, 455, 456, 460, 461, 22, 360, 5, 19,
  20, 13, 11, 12, 9, 37, 17, 382, 44, 484, 10, 16, 382, 0, 484, 14, 135, 7, 6, 8, 0, 18, 0, 0, 0, 0,
  211, 0, 0, 15, 0, 342, 474, 0, 0, 0, 0, 474, 415, 344, 361, 0, 474, 390, 391, 392, 183, 297, 407,
  411, 414, 474, 474, 475, 116, 115, 0, 396, 0, 442, 443, 444, 439, 440, 441, 678, 679, 587, 588,
  582, 583, 584, 581, 585, 586, 589, 590, 446, 447, 445, 648, 554, 553, 555, 574, 557, 559, 558,
  560, 561, 562, 563, 566, 567, 565, 564, 570, 573, 556, 575, 576, 568, 552, 551, 572, 571, 527,
  528, 569, 579, 578, 577, 580, 529, 530, 531, 662, 532, 533, 534, 540, 541, 535, 536, 537, 538,
  539, 542, 543, 544, 545, 546, 547, 548, 549, 550, 660, 659, 672, 648, 666, 663, 667, 677, 168,
  524, 648, 523, 518, 665, 517, 519, 520, 521, 522, 525, 526, 664, 671, 670, 661, 668, 669, 650,
  656, 658, 657, 648, 0, 0, 442, 443, 444, 439, 440, 441, 399, 395, 382, 484, 382, 484, 474, 0, 474,
  415, 0, 183, 376, 378, 377, 381, 380, 379, 648, 33, 369, 367, 368, 372, 371, 370, 375, 374, 373,
  0, 0, 0, 154, 156, 157, 343, 0, 0, 345, 346, 297, 0, 51, 486, 297, 110, 117, 0, 0, 26, 38, 23,
  484, 25, 27, 0, 24, 28, 0, 183, 261, 250, 648, 193, 249, 195, 196, 194, 214, 484, 0, 217, 21, 419,
  358, 201, 199, 229, 349, 0, 345, 346, 347, 59, 348, 58, 0, 352, 350, 351, 353, 418, 354, 363, 382,
  484, 382, 484, 136, 212, 0, 474, 409, 388, 305, 307, 184, 0, 293, 278, 183, 478, 478, 478, 406,
  298, 462, 463, 472, 464, 382, 438, 437, 496, 487, 0, 3, 650, 0, 0, 635, 634, 174, 166, 0, 0, 0,
  642, 644, 640, 366, 474, 396, 297, 51, 297, 117, 349, 382, 382, 151, 147, 143, 0, 146, 0, 0, 0,
  158, 0, 152, 153, 0, 486, 160, 159, 0, 0, 387, 386, 0, 293, 183, 474, 384, 385, 62, 40, 49, 412,
  474, 0, 0, 59, 0, 485, 0, 122, 106, 118, 113, 474, 476, 0, 0, 0, 0, 0, 0, 268, 0, 0, 233, 232,
  480, 231, 259, 355, 356, 357, 623, 297, 51, 297, 117, 202, 200, 389, 382, 470, 213, 225, 476, 0,
  197, 225, 331, 476, 0, 0, 280, 290, 279, 0, 0, 0, 321, 0, 183, 467, 486, 466, 468, 465, 473, 408,
  0, 0, 496, 490, 493, 0, 3, 4, 0, 653, 655, 0, 649, 652, 654, 673, 0, 171, 0, 0, 0, 474, 674, 30,
  651, 676, 612, 612, 612, 417, 0, 143, 183, 412, 0, 474, 297, 297, 0, 331, 476, 345, 346, 32, 0, 0,
  3, 163, 0, 164, 477, 0, 515, 527, 528, 0, 511, 510, 0, 508, 0, 509, 221, 516, 162, 161, 42, 292,
  296, 383, 63, 0, 61, 39, 48, 57, 474, 59, 0, 0, 108, 369, 367, 368, 372, 371, 370, 0, 120, 476, 0,
  112, 413, 474, 0, 262, 263, 0, 648, 248, 0, 474, 412, 0, 237, 486, 230, 268, 0, 0, 412, 0, 474,
  410, 404, 471, 306, 227, 228, 218, 234, 226, 0, 223, 302, 332, 0, 325, 203, 198, 476, 289, 294, 0,
  646, 283, 0, 303, 322, 479, 470, 0, 157, 0, 489, 496, 502, 361, 498, 500, 4, 31, 29, 675, 172,
  169, 0, 0, 0, 433, 432, 431, 0, 183, 297, 426, 430, 185, 186, 183, 0, 167, 0, 0, 0, 138, 142, 145,
  140, 112, 0, 0, 137, 297, 148, 325, 36, 4, 155, 0, 514, 0, 0, 513, 512, 504, 505, 66, 67, 68, 45,
  474, 0, 102, 103, 104, 100, 50, 93, 98, 183, 46, 55, 474, 111, 122, 123, 119, 105, 344, 0, 183,
  183, 0, 215, 274, 269, 270, 275, 359, 256, 481, 0, 638, 611, 600, 630, 605, 631, 632, 636, 606,
  610, 609, 604, 607, 608, 628, 599, 629, 624, 627, 364, 601, 602, 603, 43, 41, 109, 112, 405, 236,
  235, 229, 219, 337, 334, 335, 0, 254, 0, 297, 598, 595, 596, 281, 591, 593, 594, 625, 0, 286, 308,
  469, 491, 488, 495, 0, 499, 497, 501, 35, 174, 474, 434, 435, 436, 428, 323, 175, 478, 425, 382,
  178, 183, 617, 619, 620, 643, 615, 616, 614, 618, 613, 645, 641, 139, 141, 144, 268, 34, 183, 506,
  507, 0, 65, 0, 101, 474, 99, 0, 95, 0, 56, 121, 124, 650, 0, 128, 264, 266, 265, 252, 225, 271, 0,
  239, 238, 261, 260, 612, 623, 612, 107, 480, 268, 340, 336, 328, 329, 330, 327, 326, 268, 295, 0,
  592, 0, 287, 285, 309, 304, 312, 503, 173, 170, 382, 308, 324, 187, 183, 427, 187, 181, 0, 0, 474,
  395, 0, 82, 80, 71, 77, 64, 79, 73, 72, 76, 74, 69, 70, 0, 78, 0, 208, 209, 75, 0, 342, 0, 0, 183,
  183, 0, 183, 47, 0, 127, 126, 251, 216, 273, 474, 183, 257, 0, 0, 0, 244, 0, 0, 0, 0, 597, 622,
  647, 621, 626, 0, 268, 429, 299, 189, 176, 188, 319, 0, 183, 179, 187, 149, 165, 0, 83, 85, 88,
  86, 0, 84, 87, 0, 0, 204, 81, 476, 210, 0, 0, 96, 94, 97, 125, 0, 272, 276, 240, 0, 633, 637, 246,
  237, 245, 220, 482, 341, 255, 288, 0, 0, 300, 320, 182, 313, 91, 484, 89, 0, 0, 0, 183, 0, 0, 476,
  207, 0, 278, 0, 258, 639, 0, 240, 338, 486, 310, 190, 191, 308, 150, 0, 484, 90, 0, 92, 484, 0,
  205, 0, 648, 277, 243, 241, 242, 0, 421, 247, 297, 224, 483, 313, 192, 301, 315, 314, 0, 318, 648,
  650, 412, 131, 0, 484, 0, 206, 0, 423, 382, 420, 480, 316, 317, 0, 0, 0, 60, 0, 412, 132, 253,
  382, 422, 311, 650, 134, 129, 60, 0, 424, 0, 130, 133 };

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] = { -792, -792, -303, -792, -792, 712, -83, -792, -792, -792,
  -792, -736, -74, 11, -2, -792, -792, -792, -792, -4, -333, -56, -677, -792, -792, -792, -792, -82,
  -79, -78, -163, -792, -792, 43, -68, -60, -30, -792, -792, 1, -362, -792, -792, 58, -792, -792,
  -792, -222, -788, -45, -94, -297, 251, 100, -792, -792, -792, -792, 256, -40, -792, -792, 241,
  290, -792, 13, -792, 8, -792, -792, -792, -792, -792, -1, -792, -792, -792, -792, -792, -792, 675,
  122, -771, -792, -792, -792, 767, -792, -792, -792, -29, -156, 42, -77, -792, -792, -199, -382,
  -792, -792, -240, -258, -424, -420, -792, -792, 37, -792, -792, -178, -792, -209, -792, -792,
  -792, -72, -792, -792, -792, -792, -66, -792, -792, -792, -792, -43, -792, 83, -541, -792, -792,
  -792, -113, -792, -792, -195, -792, -792, -792, -792, -792, -792, 17, 385, -223, 390, -792, 41,
  -51, -585, -792, -175, -792, -584, -792, -791, -792, -792, -220, -792, -792, -792, -346, -792,
  -792, -367, -792, -792, 55, -792, -792, -792, 1022, 453, 1011, 6, -792, -792, 954, 740, -5, -792,
  68, -792, 186, 19, -47, -792, 14, 862, -792, -792, -386, -792, 12, 233, -792, -792, 205, -764,
  -792, -792, -792, -792, -792, -792, -792, -792, -792, -792, 306, 214, 74, -318, 463, -792, 466,
  -792, 188, -792, 749, -792, -403, -792, -295, -792, -774, -792, -792, -792, -65, -792, -261, -792,
  -792, -792, 340, 181, -792, -792, -792, -792, -792, 111, 170, 29, -517, -643, -792, -380, -22,
  -465, -792, -21, -792, -6, -792, -783, -792, -561, -792, -461, -792, -792, -792, -179, -792, -792,
  -792, 361, -792, -164, -344, -792, -341, 46, -505, -792, -548, -792 };

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] = { 0, 1, 2, 4, 54, 281, 56, 57, 58, 389, 59, 60, 61, 283, 63,
  273, 64, 810, 549, 301, 411, 412, 552, 548, 679, 680, 871, 932, 933, 685, 686, 808, 804, 687, 66,
  67, 68, 419, 69, 284, 422, 568, 565, 566, 894, 285, 815, 973, 1026, 71, 72, 508, 516, 509, 382,
  383, 797, 970, 384, 73, 263, 393, 264, 265, 74, 286, 667, 287, 499, 364, 771, 494, 770, 495, 496,
  857, 497, 860, 498, 927, 776, 647, 921, 922, 966, 992, 288, 78, 79, 80, 936, 881, 882, 82, 431,
  821, 83, 452, 453, 833, 454, 84, 456, 597, 598, 599, 436, 437, 741, 708, 825, 985, 958, 959, 987,
  295, 296, 897, 457, 841, 883, 826, 953, 309, 585, 429, 573, 574, 578, 579, 704, 900, 705, 823,
  983, 463, 464, 611, 465, 466, 758, 916, 289, 340, 402, 461, 749, 403, 404, 777, 994, 341, 760,
  342, 451, 849, 917, 1016, 995, 924, 469, 855, 458, 840, 602, 850, 604, 744, 745, 834, 908, 909,
  688, 87, 238, 239, 433, 90, 91, 92, 270, 442, 271, 224, 95, 96, 272, 405, 302, 98, 99, 100, 101,
  593, 889, 103, 352, 450, 104, 105, 225, 1012, 1013, 1033, 1046, 641, 642, 780, 854, 643, 106, 107,
  108, 347, 348, 349, 350, 618, 594, 351, 570, 6, 394, 395, 471, 472, 582, 583, 989, 990, 274, 275,
  109, 356, 479, 480, 481, 482, 483, 767, 626, 627, 540, 722, 723, 724, 753, 754, 843, 755, 726,
  650, 790, 791, 915, 586, 845, 727, 728, 756, 829, 365, 731, 830, 828, 732, 506, 504, 505, 733,
  757, 360, 367, 490, 491, 492, 220, 221, 222, 223 };

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] = { 93, 65, 282, 330, 236, 278, 70, 297, 298, 299, 398, 89, 307,
  76, 417, 486, 62, 102, 75, 97, 331, 276, 85, 308, 470, 729, 502, 700, 397, 240, 235, 361, 234,
  633, 314, 237, 607, 601, 366, 517, 651, 652, 268, 511, 710, 372, 243, 81, 242, 473, 474, 600, 346,
  418, 251, 485, 605, 778, 906, 368, 218, 569, 781, 778, 919, 592, 869, 328, 386, 396, 699, 240, 65,
  94, -476, 550, 550, 219, 606, 240, 512, 455, 89, 824, 312, 315, 387, 550, 291, 926, 290, 603, 240,
  327, 277, 326, 303, 550, 329, 580, 1007, 3, 241, 332, 362, 215, 369, 370, 739, 318, 844, 588, 121,
  122, -2, 663, 336, 337, 81, 675, 676, 677, 300, 478, 292, 730, 121, 122, 550, 113, 430, 357, 304,
  550, 528, 671, 1008, 1009, 374, -376, 740, 398, 216, 218, 241, 502, 121, 122, 607, 550, 603, 657,
  241, -343, -476, 363, 969, 313, 316, 543, 359, 609, 114, -376, 694, 241, 610, 468, 112, 455, 376,
  114, 378, 551, -60, -60, -492, -60, -60, 293, 294, 628, -492, 993, 984, 551, 672, 375, 215, 377,
  606, 338, 816, 678, 971, 735, -60, 861, 778, -60, 542, 217, 455, 1018, 371, 748, 725, 738, 362,
  1017, 199, 617, 381, 331, 863, 778, 37, 658, 659, 425, 587, 665, 589, 1011, 199, 216, 551, -60,
  737, -375, -60, 735, 592, -378, 312, -375, 432, 240, 235, 195, 380, 1044, 325, 237, 199, 214,
  1032, 735, -60, 355, 632, -60, 1051, -377, 1044, 447, 862, -378, 584, 363, 358, 1048, 444, 312,
  446, 1056, 448, -381, 374, 415, 789, 789, 789, 923, 778, 399, 317, -377, 388, 443, 413, 445, 240,
  235, 217, 240, 327, -376, 326, 110, 111, -381, 907, 89, -380, 616, 332, 240, -374, 291, 910, 426,
  334, -369, -374, 477, 844, 313, -379, -369, 241, -376, 336, 337, 455, 343, 952, 778, -380, 844,
  645, 844, 709, 392, 510, -373, 510, 312, 214, 81, 408, -373, -379, 514, 515, 386, 313, -378, -377,
  200, 968, 778, 416, 729, 646, 460, 407, 344, 1040, 331, 407, 345, 37, -367, -368, 241, 37, 554,
  241, -367, -368, -378, -377, 903, 1040, 905, 1053, -177, -345, 344, 241, 415, -346, 345, -345,
  1053, 965, 949, -346, 660, 519, 343, 413, 523, 661, -180, 746, 338, 747, 218, 218, -177, 962, 240,
  312, 218, 218, 218, 428, 964, 313, 898, 982, 591, 318, 729, 438, 662, 555, 523, 303, -333, 701,
  344, 848, 525, 218, 345, -333, -492, 557, 240, 235, -494, 326, -492, 484, 415, 581, -494, 856,
  729, 332, 572, 215, 215, 524, 526, 413, 415, 215, 215, 215, 955, 416, 20, 21, 334, 768, 407, 413,
  407, 318, 520, 848, 312, 416, 527, -333, 218, 317, 215, 519, 510, 510, 789, 241, 313, 595, 596,
  895, 216, 216, 1036, 1037, 544, 848, 216, 216, 216, -333, 556, 416, 858, 268, -282, 318, 462, 558,
  -381, -284, 638, -380, 434, 241, 334, 435, 636, 216, 576, 416, 691, 625, -267, 215, -372, 648,
  334, -371, 541, 640, -372, 416, -381, -371, 218, -380, 407, 195, 407, 814, 981, -333, 789, 725,
  789, -117, 612, 313, 421, 217, 217, 268, 614, -379, 520, 217, 217, 217, 979, 925, 216, 310, -117,
  -117, 320, 692, 421, 615, 693, -370, 407, 555, 846, 502, 847, -370, 217, -379, 629, 215, 623, 317,
  240, 639, 279, 541, 312, 244, 245, 246, 247, 248, 249, 214, 214, 1023, 1004, 981, 1024, 214, 214,
  214, 681, 682, 683, 725, 621, 407, 407, 1019, 779, 1020, 836, 837, 838, 839, 216, 631, 317, 217,
  214, 653, 1021, 656, 567, 1022, 664, 668, 669, 510, 670, 725, 673, 32, 610, 674, 689, 317, 244,
  245, 246, 247, 248, 249, 556, 696, 702, 698, 455, 317, 334, 703, 734, 736, 240, 241, 742, 743,
  762, 313, 763, 769, 759, 214, 766, 661, 942, 794, 252, 253, 254, 255, 256, 257, 746, 217, 795,
  798, 806, 721, 800, 240, 1025, 801, 407, 807, 258, 259, 260, 338, 803, 813, 822, 824, 681, 682,
  683, 827, 799, 1039, 747, 831, 684, 896, 721, 455, 455, 899, -339, 928, 920, 379, 929, 1041, 32,
  1043, 941, 502, 950, 214, 957, 960, 961, 980, 1055, 963, 967, 241, 1052, 984, 991, 986, 1003, 502,
  1005, 1030, 55, 250, 868, 872, 379, 320, 873, 721, 721, 721, 805, 947, 1015, 218, 37, 893, 859,
  874, 241, 252, 253, 254, 255, 256, 257, 875, 317, 455, 252, 253, 254, 255, 256, 257, 812, 998, 5,
  258, 259, 260, 876, 697, 53, 654, 796, 877, 258, 259, 260, 655, 666, 619, 852, 782, 519, 77, 880,
  975, 215, 269, 339, 832, 1014, 988, 817, 32, 902, 625, 310, 306, 951, 1006, 546, 842, 32, 459,
  545, 330, 318, 1035, 93, 65, 835, 720, 695, 644, 70, 764, 282, 761, 918, 887, 940, 879, 331, 216,
  870, 475, 878, 888, 476, 240, 884, 622, 911, 943, 904, 914, 720, 613, 721, 0, 0, 0, 319, 0, 322,
  324, 901, 0, 305, 518, 0, 0, 1031, 323, 0, 520, 775, 53, 335, 0, 0, 0, 0, 379, 0, 0, 53, 353, 354,
  328, 1038, 0, 0, 331, 0, 720, 720, 720, 0, 0, 0, 94, 217, 268, 943, 93, 65, 0, 0, 0, 567, 459, 0,
  312, 93, 327, 887, 326, 935, 0, 329, 931, 241, 934, 939, 944, 0, 937, 0, 0, 331, 282, 331, 0,
  1000, 0, 0, 0, 996, 0, 0, 943, 0, 943, 608, 0, 214, 0, 721, 385, 721, 518, 0, 0, 0, 218, 93, 972,
  0, 0, 0, 93, 235, 1027, 407, 93, 327, 1029, 326, 974, 312, 519, 887, 977, 935, 976, 944, 931, 94,
  934, 978, 0, 0, 937, 0, 0, 313, 94, 0, 0, 333, 0, 0, 1034, 1042, 318, 0, 427, 720, 721, 215, 997,
  93, 1001, 93, 235, 0, 326, 721, 721, 721, 322, 324, 240, 721, 944, 0, 1047, 305, 999, 323, 721,
  415, 407, 0, 0, 1028, 0, 519, 94, 1054, 0, 1010, 413, 94, 0, 240, 216, 94, 0, 0, 322, 324, 313,
  520, 414, 0, 88, 467, 0, 0, 0, 318, 415, 721, 415, 608, 0, 86, 0, 0, 0, 0, 0, 413, 0, 413, 721,
  415, 721, 0, 721, 0, 0, 311, 94, 0, 94, 267, 413, 0, 0, 439, 440, 441, 0, 241, 0, 720, 266, 720,
  218, 0, 0, 0, 416, 217, 721, 218, 218, 0, 520, 244, 245, 246, 247, 248, 249, 0, 241, 467, 0, 0,
  449, 0, 218, 244, 245, 246, 247, 248, 249, 0, 0, 334, 0, 416, 0, 416, 0, 0, 0, 0, 0, 321, 333,
  215, 0, 0, 720, 214, 416, 0, 215, 215, 0, 0, 414, 720, 720, 720, 0, 407, 507, 720, 0, 0, 0, 0,
  407, 215, 720, 32, 0, 0, 406, 0, 0, 0, 406, 0, 0, 0, 216, 0, 334, 467, 0, 0, 0, 216, 216, 0, 333,
  0, 547, 322, 324, 0, 0, 0, 0, 553, 0, 720, 333, 0, 216, 37, 0, 0, 0, 0, 414, 0, 0, 0, 720, 0, 720,
  0, 720, 334, 317, 334, 414, 385, 0, 0, 0, 0, 0, 0, 0, 311, 0, 0, 0, 0, 0, 0, 0, 217, 0, 0, 0, 0,
  720, 0, 217, 217, 0, 0, 0, 317, 0, 317, 0, 0, 269, 0, 0, 0, 311, 0, 0, 217, 0, 0, 317, 518, 0, 0,
  0, 0, 0, 0, 267, 406, 0, 406, 0, 0, 0, 0, 0, 214, 0, 373, 649, 0, 0, 0, 214, 214, 0, 886, 0, 0, 0,
  0, 0, 0, 0, 322, 0, 0, 269, 0, 0, 214, 0, 0, 267, 267, 252, 253, 254, 255, 256, 257, 321, 0, 0,
  390, 391, 0, 311, 0, 333, 267, 0, 267, 258, 259, 260, 440, 441, 0, 0, 0, 423, 0, 424, 690, 0, 406,
  0, 406, 0, 0, 0, 252, 253, 254, 255, 256, 257, 0, 0, 0, 32, 707, 0, 0, 0, 0, 0, 0, 706, 258, 259,
  260, 938, 0, 0, 16, 17, 18, 0, 406, 379, 938, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 311, 0,
  0, 37, 32, 261, 0, 262, 0, 0, 809, 765, 0, 0, 311, 0, 0, 0, 0, 0, 0, 818, 819, 406, 406, 322, 324,
  0, 0, 0, 938, 53, 0, 0, 0, 938, 267, 0, 0, 938, 37, 0, 0, 0, 379, 459, 522, 513, 0, 0, 0, 0, 0,
  252, 253, 254, 255, 256, 257, 0, 0, 311, 559, 560, 561, 562, 563, 564, 53, 0, 0, 258, 259, 260, 0,
  802, 938, 0, 938, 0, 258, 259, 260, 267, 624, 0, 267, 811, 252, 253, 254, 255, 256, 257, 571, 0,
  0, 575, 0, 32, 406, 0, 608, 0, 267, 0, 258, 259, 260, 0, 0, 0, 0, 0, 0, 590, 0, 0, 0, 252, 253,
  254, 255, 256, 257, 0, 0, 0, 30, 306, 0, 0, 0, 0, 32, 37, 267, 258, 259, 260, 0, 252, 253, 254,
  255, 256, 257, 620, 0, 333, 0, 420, 0, 0, 637, 421, 0, 0, 311, 258, 259, 260, 0, 53, 0, 32, 0,
  853, 0, 0, 0, 0, 0, 311, 529, 530, 0, 0, 0, 0, 0, 0, 0, 0, 267, 0, 0, 32, 261, 0, 0, 0, 0, 0, 0,
  620, 0, 0, 53, 0, 0, 37, 891, 0, 0, 0, 0, 267, 0, 0, 0, 945, 946, 0, 948, 0, 0, 531, 532, 0, 0,
  410, 0, 37, 169, 170, 171, 533, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185,
  186, 187, 188, 189, 190, 191, 0, 0, 0, 0, 53, 0, 269, 0, 0, 0, 0, 0, 0, 930, 0, 0, 0, 0, 0, 0,
  534, 0, 535, 536, 0, 537, 201, 538, 0, 203, 204, 539, 206, 207, 208, 209, 210, 211, 212, 0, 0, 0,
  0, 0, 0, 267, 267, 267, 0, 706, 0, 0, 1002, 0, 0, 0, 772, 773, 774, 252, 253, 254, 255, 256, 257,
  0, 0, 7, 8, 9, 10, 11, 12, 0, 0, 0, 0, 258, 259, 260, 0, 0, 0, 0, 0, 13, 14, 15, 0, 16, 17, 18,
  19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 27, 28, 32, 29, 30, 31, 267, 0, 0, 0, 32, 33, 34, 35, 36,
  0, 851, 575, 406, 0, 0, 0, 252, 253, 254, 255, 256, 257, 0, 0, 0, 0, 0, 414, 0, 0, 0, 0, 37, 0,
  258, 259, 260, 0, 0, 0, 37, 333, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0,
  414, 0, 414, 0, 32, 0, 0, 0, 0, 0, 53, 0, 0, 406, 0, 414, 0, 0, 252, 253, 254, 255, 256, 257, 252,
  253, 254, 255, 256, 257, 0, 0, 0, 0, 333, 0, 258, 259, 260, 0, 37, 0, 258, 259, 260, 0, 88, 0,
  267, 0, 0, 0, 267, 0, 267, 0, 0, 885, 30, 890, 421, 0, 0, 892, 32, 0, 0, 0, 53, 0, 32, 333, 0,
  333, 311, 0, 0, 0, 252, 253, 254, 255, 256, 257, 252, 253, 254, 255, 256, 257, 0, 0, 0, 0, 333, 0,
  258, 259, 260, 0, 0, 0, 258, 259, 260, 333, 37, 0, 0, 0, 267, 0, 0, 0, 0, 0, 0, 0, 0, 306, 0, 266,
  409, 1023, 32, 0, 1024, 0, 53, 0, 32, 311, 0, 0, 0, 267, 0, 0, 0, 0, 0, 0, 406, 0, 0, 0, 0, 0, 0,
  406, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
  133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151,
  0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 487, 195, 0,
  196, 197, 198, 199, 0, 488, 201, 202, 489, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 487, 195,
  493, 196, 197, 198, 199, 0, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
  213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
  133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151,
  0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 487, 195, 0,
  196, 197, 198, 199, 0, 488, 201, 202, 500, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 487, 195, 0,
  196, 197, 198, 199, 503, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 487, 195,
  521, 196, 197, 198, 199, 0, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
  213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
  133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151,
  0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 487, 195,
  577, 196, 197, 198, 199, 0, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
  213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
  133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151,
  0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 487, 195,
  820, 196, 197, 198, 199, 0, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
  213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
  133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151,
  0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 487, 195,
  1045, 196, 197, 198, 199, 0, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
  213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
  133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151,
  0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 487, 195,
  1049, 196, 197, 198, 199, 0, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
  213, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
  133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151,
  0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 501, 195, 0,
  196, 197, 198, 199, 0, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 630, 195, 0,
  196, 197, 198, 199, 0, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 369, 370, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 711, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 783, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 712, 713, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 912, 612, 913,
  784, 715, 785, 371, 0, 787, 201, 718, 0, 203, 204, 788, 206, 207, 208, 209, 210, 211, 212, 719,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 1050, 195,
  0, 196, 197, 198, 199, 0, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 1057, 195,
  0, 196, 197, 198, 199, 0, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 195, 0,
  196, 197, 198, 199, 0, 200, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 369, 370, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 711, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 783, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 712, 713, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 612, 0,
  784, 715, 785, 371, 786, 787, 201, 718, 0, 203, 204, 788, 206, 207, 208, 209, 210, 211, 212, 719,
  115, 116, 117, 118, 119, 120, 369, 370, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 711, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 783, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 712, 713, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 612, 0,
  784, 715, 785, 371, 792, 787, 201, 718, 0, 203, 204, 788, 206, 207, 208, 209, 210, 211, 212, 719,
  115, 116, 117, 118, 119, 120, 369, 370, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 711, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 783, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 712, 713, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 612, 0,
  784, 715, 785, 371, 793, 787, 201, 718, 0, 203, 204, 788, 206, 207, 208, 209, 210, 211, 212, 719,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 195, 0,
  196, 197, 198, 199, 0, 488, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
  115, 116, 117, 118, 119, 120, 369, 370, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 711, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 783, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 712, 713, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 612, 0,
  784, 715, 785, 371, 0, 787, 201, 718, 954, 203, 204, 788, 206, 207, 208, 209, 210, 211, 212, 719,
  115, 116, 117, 118, 119, 120, 369, 370, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 711, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 783, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 712, 713, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 612, 0,
  784, 715, 785, 371, 0, 787, 201, 718, 956, 203, 204, 788, 206, 207, 208, 209, 210, 211, 212, 719,
  115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
  189, 190, 191, 192, 193, 194, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 195, 0,
  196, 197, 198, 199, 0, 0, 201, 202, 0, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 115,
  116, 117, 118, 119, 120, 369, 370, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
  135, 711, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152,
  153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 362, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
  190, 191, 712, 713, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 612, 0, 714,
  715, 716, 371, 0, 717, 201, 718, 0, 203, 204, 363, 206, 207, 208, 209, 210, 211, 212, 719, -595,
  -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595,
  -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, 0, -595, -595,
  -595, -595, -595, -595, 0, -595, -595, -595, -595, 0, 0, -595, -595, -595, -595, -595, -595, -595,
  -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595,
  -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, 0, -595,
  -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, -595, 0, -595, 0, -632,
  -595, -595, -595, 0, -595, -595, -595, 0, -595, -595, -595, -595, -595, -595, -595, -595, -595,
  -595, -595, 115, 116, 117, 118, 119, 120, 369, 370, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 711, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150,
  151, 0, 152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 750, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 712, 713, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 612,
  0, 0, 715, 0, 371, 0, 751, 201, 718, 0, 203, 204, 752, 206, 207, 208, 209, 210, 211, 212, 719,
  115, 116, 117, 118, 119, 120, 369, 370, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
  134, 135, 711, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0,
  152, 153, 154, 155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
  170, 171, 0, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
  190, 191, 712, 713, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 612, 0, 0, 715,
  0, 371, 0, 717, 201, 718, 0, 203, 204, 0, 206, 207, 208, 209, 210, 211, 212, 719, 115, 116, 117,
  118, 119, 120, 369, 370, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 711,
  137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154,
  155, 0, 0, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 0, 173,
  174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 712, 0,
  0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 612, 0, 0, 715, 0, 371, 0, 0, 201,
  718, 0, 203, 204, 0, 206, 207, 208, 209, 210, 211, 212, 719, 226, 227, 228, 229, 230, 231, 0, 0,
  529, 530, 0, 0, 0, 0, 0, 0, 0, 0, 133, 134, 135, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24,
  25, 26, 232, 28, 0, 233, 0, 0, 0, 0, 0, 0, 32, 33, 0, 0, 0, 531, 532, 0, 0, 0, 0, 0, 169, 170,
  171, 533, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
  190, 191, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 534, 0, 535,
  536, 0, 537, 201, 538, 0, 203, 204, 539, 206, 207, 208, 209, 210, 211, 212, 7, 8, 9, 10, 11, 12,
  123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 0, 0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0,
  20, 21, 22, 23, 24, 25, 26, 232, 28, 864, 865, 30, 31, 0, 0, 0, 0, 32, 33, 34, 0, 866, 0, 0, 0, 0,
  0, 0, 0, 0, 252, 253, 254, 255, 256, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 258, 259, 260, 0, 0,
  37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 867, 7, 8, 9, 10, 11, 12, 32, 0, 0,
  0, 0, 53, 0, 0, 0, 0, 0, 0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26,
  232, 28, 864, 233, 30, 279, 37, 0, 0, 0, 32, 33, 0, 0, 280, 0, 0, 0, 0, 0, 0, 0, 409, 0, 0, 0,
  410, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44,
  45, 46, 47, 48, 49, 50, 51, 7, 8, 9, 10, 11, 12, 0, 0, 0, 0, 0, 0, 53, 0, 0, 0, 0, 0, 13, 14, 15,
  0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 0, 0, 0, 0, 0, 32,
  33, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 252, 253, 254, 255, 256, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 258, 259, 260, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 7, 8, 9,
  10, 11, 12, 32, 0, 0, 0, 0, 0, 53, 0, 0, 0, 0, 0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21,
  22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 279, 37, 0, 0, 0, 32, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1023, 0, 0, 1024, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 7, 8, 9, 10, 11, 12, 0, 0, 0, 0, 0, 0, 53, 0, 0, 0, 0,
  0, 13, 14, 15, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 0, 0,
  0, 0, 0, 32, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 226, 227, 228,
  229, 230, 231, 0, 0, 0, 0, 0, 0, 53, 0, 0, 0, 0, 0, 133, 134, 135, 0, 16, 17, 18, 19, 0, 0, 0, 20,
  21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 279, 0, 0, 0, 0, 32, 33, 0, 0, 280, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41,
  42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 226, 227, 228, 229, 230, 231, 0, 20, 21, 22, 23, 24, 25,
  26, 232, 28, 400, 233, 133, 134, 135, 0, 16, 17, 18, 19, 401, 0, 0, 20, 21, 22, 23, 24, 25, 26,
  232, 28, 0, 233, 30, 0, 0, 0, 0, 0, 32, 33, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44,
  45, 46, 47, 48, 49, 50, 51, 226, 227, 228, 229, 230, 231, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 133,
  134, 135, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 30, 0, 0, 0, 0,
  0, 32, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 226, 227, 228, 229,
  230, 231, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 133, 134, 135, 0, 16, 17, 18, 19, 0, 0, 0, 20, 21,
  22, 23, 24, 25, 26, 232, 28, 0, 233, 0, 0, 0, 0, 0, 0, 32, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 226, 227, 228, 229, 230, 231, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  133, 134, 135, 0, 634, 0, 635, 19, 0, 0, 0, 20, 21, 22, 23, 24, 25, 26, 232, 28, 0, 233, 0, 0, 0,
  0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 226, 227, 228,
  229, 230, 231, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 133, 134, 135, 0, 0, 0, 0, 0, 0, 0, 0, 20, 21,
  22, 23, 24, 25, 26, 232, 28, 0, 233, 0, 0, 0, 0, 0, 0, 32, 33, 0, 0, 0, 0, 20, 21, 22, 23, 24, 25,
  26, 232, 28, 400, 233, 0, 0, 0, 0, 0, 0, 0, 0, 401, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 0, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 343, 0, 0, 0, 0, 0, 0, 0, 0, 38, 39, 40, 41, 42,
  43, 44, 45, 46, 47, 48, 49, 50, 51, -291, 0, 0, 0, 0, 0, 0, 0, 0, 344, 0, 0, 0, 345, 20, 21, 22,
  23, 24, 25, 26, 232, 28, 400, 233, 0, 0, 0, 0, 0, 0, 0, 0, 401, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 343, 0, 0, 0, 0, 0, 0, 0, 0, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0, 0, 0, 0, 0, 344, 0, 0, 0, 345 };

static const yytype_int16 yycheck[] = { 5, 5, 76, 97, 34, 70, 5, 79, 80, 81, 268, 5, 89, 5, 275,
  359, 5, 5, 5, 5, 97, 68, 5, 89, 342, 586, 367, 575, 268, 34, 34, 195, 34, 494, 90, 34, 460, 457,
  202, 385, 505, 506, 36, 376, 585, 224, 34, 5, 34, 344, 345, 454, 103, 276, 35, 358, 459, 641, 832,
  223, 31, 423, 647, 647, 855, 451, 802, 97, 243, 268, 575, 76, 76, 5, 24, 3, 3, 31, 460, 84, 377,
  339, 76, 3, 89, 90, 250, 3, 76, 860, 76, 458, 97, 97, 102, 97, 84, 3, 97, 432, 19, 0, 34, 97, 64,
  31, 9, 10, 78, 90, 753, 444, 9, 10, 0, 518, 9, 10, 76, 29, 30, 31, 107, 53, 46, 586, 9, 10, 3, 11,
  294, 112, 101, 3, 395, 64, 55, 56, 11, 85, 110, 399, 31, 114, 76, 486, 9, 10, 572, 3, 517, 513,
  84, 101, 104, 115, 927, 89, 90, 399, 114, 101, 44, 109, 567, 97, 106, 342, 52, 427, 235, 44, 237,
  101, 102, 102, 106, 105, 105, 101, 102, 484, 112, 966, 104, 101, 115, 234, 114, 236, 572, 84, 697,
  103, 930, 101, 102, 782, 782, 105, 399, 31, 460, 994, 107, 608, 586, 593, 64, 992, 107, 472, 242,
  290, 799, 799, 85, 514, 515, 284, 443, 524, 445, 987, 107, 114, 101, 102, 590, 101, 105, 101, 618,
  85, 239, 107, 301, 242, 242, 102, 242, 1029, 109, 242, 107, 31, 1010, 101, 102, 24, 106, 105,
  1040, 85, 1042, 332, 797, 109, 437, 115, 102, 1035, 327, 268, 329, 1053, 332, 85, 11, 274, 650,
  651, 652, 858, 858, 269, 90, 109, 104, 326, 274, 328, 287, 287, 114, 290, 290, 85, 290, 25, 26,
  109, 833, 287, 85, 470, 290, 302, 101, 287, 841, 287, 97, 101, 107, 352, 949, 239, 85, 107, 242,
  109, 9, 10, 572, 78, 901, 901, 109, 962, 26, 964, 583, 101, 375, 101, 377, 332, 114, 287, 102,
  107, 109, 380, 381, 510, 268, 85, 85, 109, 925, 925, 274, 904, 50, 339, 272, 110, 1025, 426, 276,
  114, 85, 101, 101, 287, 85, 413, 290, 107, 107, 109, 109, 828, 1041, 830, 1043, 26, 101, 110, 302,
  376, 101, 114, 107, 1052, 917, 894, 107, 101, 385, 78, 376, 388, 106, 44, 105, 84, 107, 360, 361,
  50, 909, 398, 399, 366, 367, 368, 105, 916, 332, 821, 950, 450, 385, 966, 107, 107, 413, 414, 398,
  111, 576, 110, 107, 101, 387, 114, 111, 106, 414, 426, 426, 106, 426, 112, 102, 432, 433, 112,
  776, 992, 426, 427, 360, 361, 102, 106, 432, 444, 366, 367, 368, 904, 376, 32, 33, 242, 627, 375,
  444, 377, 433, 385, 107, 460, 388, 104, 111, 430, 274, 387, 467, 514, 515, 845, 398, 399, 32, 33,
  814, 360, 361, 1021, 1022, 101, 107, 366, 367, 368, 111, 413, 414, 778, 478, 102, 467, 104, 101,
  85, 107, 496, 85, 45, 426, 290, 48, 496, 387, 102, 432, 557, 483, 108, 430, 101, 498, 302, 101,
  398, 496, 107, 444, 109, 107, 486, 109, 443, 102, 445, 104, 945, 111, 903, 904, 905, 102, 102,
  460, 105, 360, 361, 526, 107, 85, 467, 366, 367, 368, 942, 858, 430, 89, 101, 102, 92, 103, 105,
  108, 106, 101, 477, 557, 106, 895, 108, 107, 387, 109, 103, 486, 106, 376, 568, 496, 44, 455, 572,
  3, 4, 5, 6, 7, 8, 360, 361, 102, 980, 1002, 105, 366, 367, 368, 29, 30, 31, 966, 112, 514, 515,
  32, 642, 34, 13, 14, 15, 16, 486, 112, 413, 430, 387, 101, 45, 101, 419, 48, 101, 3, 111, 661,
  111, 992, 108, 49, 106, 112, 105, 432, 3, 4, 5, 6, 7, 8, 557, 101, 108, 106, 887, 444, 426, 84,
  101, 101, 640, 568, 107, 111, 105, 572, 112, 103, 614, 430, 84, 106, 887, 101, 3, 4, 5, 6, 7, 8,
  105, 486, 101, 103, 106, 586, 112, 667, 996, 112, 591, 37, 21, 22, 23, 84, 105, 104, 106, 3, 29,
  30, 31, 108, 667, 1024, 107, 101, 37, 101, 611, 944, 945, 84, 112, 108, 104, 239, 101, 1027, 49,
  1029, 101, 1039, 47, 486, 67, 108, 112, 944, 1049, 108, 108, 640, 1042, 104, 108, 112, 101, 1055,
  102, 101, 5, 102, 802, 802, 268, 269, 802, 650, 651, 652, 684, 891, 990, 701, 85, 810, 780, 802,
  667, 3, 4, 5, 6, 7, 8, 802, 557, 1002, 3, 4, 5, 6, 7, 8, 693, 974, 4, 21, 22, 23, 802, 572, 113,
  509, 661, 802, 21, 22, 23, 510, 526, 478, 770, 648, 776, 5, 802, 930, 701, 36, 102, 741, 988, 958,
  698, 49, 826, 765, 332, 44, 900, 983, 404, 749, 49, 339, 403, 888, 776, 1016, 802, 802, 744, 586,
  568, 496, 802, 623, 879, 618, 854, 802, 881, 802, 888, 701, 802, 351, 802, 802, 351, 823, 802,
  480, 843, 888, 829, 845, 611, 465, 753, -1, -1, -1, 91, -1, 93, 94, 823, -1, 88, 385, -1, -1,
  1005, 93, -1, 776, 640, 113, 98, -1, -1, -1, -1, 399, -1, -1, 113, 107, 108, 888, 1023, -1, -1,
  939, -1, 650, 651, 652, -1, -1, -1, 802, 701, 866, 939, 879, 879, -1, -1, -1, 693, 427, -1, 887,
  888, 888, 879, 888, 879, -1, 888, 879, 823, 879, 879, 888, -1, 879, -1, -1, 976, 974, 978, -1,
  975, -1, -1, -1, 972, -1, -1, 976, -1, 978, 460, -1, 701, -1, 843, 243, 845, 467, -1, -1, -1, 895,
  930, 930, -1, -1, -1, 935, 935, 997, 859, 939, 939, 1001, 939, 930, 944, 945, 935, 930, 935, 930,
  939, 935, 879, 935, 935, -1, -1, 935, -1, -1, 887, 888, -1, -1, 97, -1, -1, 1013, 1028, 945, -1,
  291, 753, 894, 895, 974, 976, 976, 978, 978, -1, 978, 903, 904, 905, 240, 241, 987, 909, 978, -1,
  1033, 238, 974, 240, 916, 996, 918, -1, -1, 999, -1, 1002, 930, 1046, -1, 987, 996, 935, -1, 1010,
  895, 939, -1, -1, 270, 271, 944, 945, 274, -1, 5, 342, -1, -1, -1, 1002, 1027, 949, 1029, 572, -1,
  5, -1, -1, -1, -1, -1, 1027, -1, 1029, 962, 1042, 964, -1, 966, -1, -1, 89, 976, -1, 978, 36,
  1042, -1, -1, 311, 312, 313, -1, 987, -1, 843, 36, 845, 1031, -1, -1, -1, 996, 895, 992, 1038,
  1039, -1, 1002, 3, 4, 5, 6, 7, 8, -1, 1010, 404, -1, -1, 333, -1, 1055, 3, 4, 5, 6, 7, 8, -1, -1,
  888, -1, 1027, -1, 1029, -1, -1, -1, -1, -1, 92, 242, 1031, -1, -1, 894, 895, 1042, -1, 1038,
  1039, -1, -1, 376, 903, 904, 905, -1, 1047, 373, 909, -1, -1, -1, -1, 1054, 1055, 916, 49, -1, -1,
  272, -1, -1, -1, 276, -1, -1, -1, 1031, -1, 939, 470, -1, -1, -1, 1038, 1039, -1, 290, -1, 405,
  415, 416, -1, -1, -1, -1, 412, -1, 949, 302, -1, 1055, 85, -1, -1, -1, -1, 432, -1, -1, -1, 962,
  -1, 964, -1, 966, 976, 996, 978, 444, 510, -1, -1, -1, -1, -1, -1, -1, 239, -1, -1, -1, -1, -1,
  -1, -1, 1031, -1, -1, -1, -1, 992, -1, 1038, 1039, -1, -1, -1, 1027, -1, 1029, -1, -1, 478, -1,
  -1, -1, 268, -1, -1, 1055, -1, -1, 1042, 776, -1, -1, -1, -1, -1, -1, 225, 375, -1, 377, -1, -1,
  -1, -1, -1, 1031, -1, 225, 499, -1, -1, -1, 1038, 1039, -1, 802, -1, -1, -1, -1, -1, -1, -1, 523,
  -1, -1, 526, -1, -1, 1055, -1, -1, 261, 262, 3, 4, 5, 6, 7, 8, 269, -1, -1, 261, 262, -1, 332, -1,
  426, 278, -1, 280, 21, 22, 23, 555, 556, -1, -1, -1, 278, -1, 280, 554, -1, 443, -1, 445, -1, -1,
  -1, 3, 4, 5, 6, 7, 8, -1, -1, -1, 49, 581, -1, -1, -1, -1, -1, -1, 579, 21, 22, 23, 879, -1, -1,
  25, 26, 27, -1, 477, 887, 888, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, 399, -1, -1, 85, 49,
  50, -1, 52, -1, -1, 688, 624, -1, -1, 413, -1, -1, -1, -1, -1, -1, 699, 700, 514, 515, 638, 639,
  -1, -1, -1, 930, 113, -1, -1, -1, 935, 378, -1, -1, 939, 85, -1, -1, -1, 944, 945, 388, 378, -1,
  -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, 460, 3, 4, 5, 6, 7, 8, 113, -1, -1, 21, 22, 23, -1, 679,
  976, -1, 978, -1, 21, 22, 23, 425, 483, -1, 428, 691, 3, 4, 5, 6, 7, 8, 425, -1, -1, 428, -1, 49,
  591, -1, 1002, -1, 446, -1, 21, 22, 23, -1, -1, -1, -1, -1, -1, 446, -1, -1, -1, 3, 4, 5, 6, 7, 8,
  -1, -1, -1, 43, 44, -1, -1, -1, -1, 49, 85, 478, 21, 22, 23, -1, 3, 4, 5, 6, 7, 8, 478, -1, 640,
  -1, 101, -1, -1, 496, 105, -1, -1, 557, 21, 22, 23, -1, 113, -1, 49, -1, 771, -1, -1, -1, -1, -1,
  572, 11, 12, -1, -1, -1, -1, -1, -1, -1, -1, 526, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, 526, -1,
  -1, 113, -1, -1, 85, 806, -1, -1, -1, -1, 549, -1, -1, -1, 889, 890, -1, 892, -1, -1, 54, 55, -1,
  -1, 105, -1, 85, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
  81, 82, 83, -1, -1, -1, -1, 113, -1, 866, -1, -1, -1, -1, -1, -1, 864, -1, -1, -1, -1, -1, -1,
  104, -1, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, -1,
  -1, -1, -1, -1, -1, 634, 635, 636, -1, 900, -1, -1, 977, -1, -1, -1, 634, 635, 636, 3, 4, 5, 6, 7,
  8, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25,
  26, 27, 28, -1, -1, -1, 32, 33, 34, 35, 36, 37, 38, 39, 40, 49, 42, 43, 44, 698, -1, -1, -1, 49,
  50, 51, 52, 53, -1, 765, 698, 859, -1, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, 996, -1, -1,
  -1, -1, 85, -1, 21, 22, 23, -1, -1, -1, 85, 888, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
  99, 100, 101, -1, 1027, -1, 1029, -1, 49, -1, -1, -1, -1, -1, 113, -1, -1, 918, -1, 1042, -1, -1,
  3, 4, 5, 6, 7, 8, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, 939, -1, 21, 22, 23, -1, 85, -1, 21, 22, 23,
  -1, 802, -1, 804, -1, -1, -1, 808, -1, 810, -1, -1, 802, 43, 804, 105, -1, -1, 808, 49, -1, -1,
  -1, 113, -1, 49, 976, -1, 978, 887, -1, -1, -1, 3, 4, 5, 6, 7, 8, 3, 4, 5, 6, 7, 8, -1, -1, -1,
  -1, 999, -1, 21, 22, 23, -1, -1, -1, 21, 22, 23, 1010, 85, -1, -1, -1, 866, -1, -1, -1, -1, -1,
  -1, -1, -1, 44, -1, 866, 101, 102, 49, -1, 105, -1, 113, -1, 49, 944, -1, -1, -1, 891, -1, -1, -1,
  -1, -1, -1, 1047, -1, -1, -1, -1, -1, -1, 1054, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40,
  -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
  65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104, 105, 106, 107, -1, 109, 110,
  111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36,
  37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
  85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106,
  107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
  33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56,
  57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
  81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1,
  104, 105, 106, 107, -1, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
  3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
  29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52,
  53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
  77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
  101, 102, -1, 104, 105, 106, 107, 108, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120,
  121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48,
  49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
  73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
  97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116,
  117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43,
  44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
  68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110, 111, -1,
  113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38,
  39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
  63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
  87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, -1,
  109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1,
  35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
  59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
  83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105,
  106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5,
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102,
  103, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
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
  68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, -1, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, -1, 109, 110, 111, -1,
  113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38,
  39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
  63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
  87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104, 105, 106, 107, -1,
  109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1,
  35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
  59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
  83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, -1, 104, 105,
  106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5,
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54,
  55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
  79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102,
  -1, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
  123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
  76, 77, 78, 79, 80, 81, 82, 83, 84, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
  100, -1, 102, -1, 104, 105, 106, 107, 108, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
  72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, 108, 109, 110, 111, -1, 113, 114, 115, 116,
  117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43,
  44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
  68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, -1, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, 108, 109, 110, 111, -1, 113,
  114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40,
  -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
  65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, -1, 109, 110,
  111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36,
  37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
  85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107,
  -1, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8,
  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
  -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
  82, 83, 84, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, 104,
  105, 106, 107, -1, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3,
  4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
  30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53,
  54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77,
  78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1,
  102, -1, 104, 105, 106, 107, -1, -1, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121,
  122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50,
  51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
  75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
  99, 100, -1, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118,
  119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1,
  -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
  71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94,
  95, 96, 97, 98, 99, 100, -1, 102, -1, 104, 105, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115,
  116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38, 39, 40, -1, 42,
  43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
  67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, -1, 87, 88, 89, 90,
  91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, -1, 105, -1, 107, -1, 109, 110, 111, -1,
  113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35, 36, 37, 38,
  39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
  63, -1, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, -1,
  87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, -1, 105, -1, 107, -1, 109,
  110, 111, -1, 113, 114, -1, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, 35,
  36, 37, 38, 39, 40, -1, 42, 43, 44, 45, -1, -1, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
  60, 61, 62, 63, -1, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
  84, -1, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, 102, -1, -1, 105, -1,
  107, -1, -1, 110, 111, -1, 113, 114, -1, 116, 117, 118, 119, 120, 121, 122, 123, 3, 4, 5, 6, 7, 8,
  -1, -1, 11, 12, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32,
  33, 34, 35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1, -1, -1, -1, 49, 50, -1, -1, -1, 54, 55, -1,
  -1, -1, -1, -1, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
  81, 82, 83, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, -1, -1, 104,
  -1, 106, 107, -1, 109, 110, 111, -1, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 3, 4, 5, 6,
  7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, -1, -1, -1, -1, 49, 50, 51, -1, 53, -1, -1,
  -1, -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22,
  23, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 3, 4, 5, 6, 7,
  8, 49, -1, -1, -1, -1, 113, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 85, -1, -1, -1, 49, 50, -1, -1, 53, -1, -1,
  -1, -1, -1, -1, -1, 101, -1, -1, -1, 105, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6,
  7, 8, -1, -1, -1, -1, -1, -1, 113, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1,
  32, 33, 34, 35, 36, 37, 38, 39, 40, -1, 42, 43, -1, -1, -1, -1, -1, 49, 50, 51, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23,
  -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, 49,
  -1, -1, -1, -1, -1, 113, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33,
  34, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, 85, -1, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, 102, -1, -1, 105, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1,
  -1, -1, -1, -1, -1, 113, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33,
  34, 35, 36, 37, 38, 39, 40, -1, 42, 43, -1, -1, -1, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1,
  -1, -1, -1, -1, -1, 113, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33,
  34, 35, 36, 37, 38, 39, 40, -1, 42, 43, 44, -1, -1, -1, -1, 49, 50, -1, -1, 53, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 21, 22, 23, -1, 25, 26, 27, 28, 51, -1, -1, 32, 33,
  34, 35, 36, 37, 38, 39, 40, -1, 42, 43, -1, -1, -1, -1, -1, 49, 50, 51, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1, -1, -1,
  -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33,
  34, 35, 36, 37, 38, 39, 40, -1, 42, 43, -1, -1, -1, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, 26, 27, 28, -1, -1, -1, 32, 33,
  34, 35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1, -1, -1, -1, 49, 50, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, 25, -1, 27, 28, -1, -1, -1, 32, 33,
  34, 35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1, -1, -1, -1, 49, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 3, 4, 5, 6, 7, 8, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, 23, -1, -1, -1, -1, -1, -1, -1, -1, 32, 33,
  34, 35, 36, 37, 38, 39, 40, -1, 42, -1, -1, -1, -1, -1, -1, 49, 50, -1, -1, -1, -1, 32, 33, 34,
  35, 36, 37, 38, 39, 40, 41, 42, -1, -1, -1, -1, -1, -1, -1, -1, 51, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 85, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 78, -1, -1, -1, -1,
  -1, -1, -1, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, -1, -1, -1, -1, -1,
  -1, -1, -1, 110, -1, -1, -1, 114, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, -1, -1, -1, -1, -1,
  -1, -1, -1, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, 78, -1, -1, -1, -1, -1, -1, -1, -1, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, -1, -1, -1, -1, -1, -1, -1, -1, -1, 110, -1, -1, -1, 114 };

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] = { 0, 125, 126, 0, 127, 344, 345, 3, 4, 5, 6, 7, 8, 21, 22, 23,
  25, 26, 27, 28, 32, 33, 34, 35, 36, 37, 38, 39, 40, 42, 43, 44, 49, 50, 51, 52, 53, 85, 87, 88,
  89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 113, 128, 129, 130, 131, 132, 134, 135, 136,
  137, 138, 140, 143, 158, 159, 160, 162, 163, 173, 174, 183, 188, 189, 191, 210, 211, 212, 213,
  216, 217, 220, 225, 266, 296, 297, 298, 299, 301, 302, 303, 304, 306, 308, 309, 312, 313, 314,
  315, 316, 318, 319, 322, 323, 334, 335, 336, 356, 25, 26, 52, 11, 44, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 35, 36,
  37, 38, 39, 40, 42, 43, 44, 45, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 102,
  104, 105, 106, 107, 109, 110, 111, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 335,
  336, 367, 368, 369, 399, 400, 401, 402, 403, 307, 324, 3, 4, 5, 6, 7, 8, 39, 42, 138, 143, 160,
  163, 298, 299, 304, 306, 312, 318, 3, 4, 5, 6, 7, 8, 102, 309, 3, 4, 5, 6, 7, 8, 21, 22, 23, 50,
  52, 184, 186, 187, 296, 298, 299, 303, 304, 306, 310, 139, 354, 355, 310, 102, 354, 44, 53, 129,
  136, 137, 163, 169, 189, 191, 210, 266, 312, 318, 46, 101, 102, 239, 240, 239, 239, 239, 107, 143,
  312, 318, 101, 344, 44, 217, 244, 247, 297, 302, 304, 306, 145, 304, 306, 308, 309, 303, 297, 298,
  303, 344, 303, 109, 138, 143, 160, 163, 174, 217, 299, 313, 322, 344, 9, 10, 84, 204, 267, 275,
  277, 78, 110, 114, 272, 337, 338, 339, 340, 343, 320, 344, 344, 24, 357, 309, 102, 399, 395, 395,
  64, 115, 193, 385, 395, 396, 395, 9, 10, 107, 389, 296, 11, 310, 354, 310, 354, 297, 138, 160,
  178, 179, 182, 204, 275, 395, 104, 133, 296, 296, 101, 185, 346, 347, 220, 224, 225, 299, 41, 51,
  268, 271, 272, 311, 313, 336, 102, 101, 105, 144, 145, 299, 303, 304, 306, 356, 268, 161, 101,
  105, 164, 296, 296, 354, 312, 204, 105, 249, 395, 218, 354, 300, 45, 48, 230, 231, 107, 303, 303,
  303, 305, 310, 354, 310, 354, 217, 244, 344, 321, 278, 221, 222, 224, 225, 226, 242, 286, 297,
  299, 269, 104, 259, 260, 262, 263, 204, 275, 284, 337, 348, 349, 348, 348, 338, 340, 310, 53, 358,
  359, 360, 361, 362, 102, 126, 396, 101, 109, 112, 397, 398, 399, 103, 195, 197, 198, 200, 202,
  192, 112, 101, 398, 108, 391, 392, 390, 344, 175, 177, 272, 144, 175, 296, 310, 310, 176, 286,
  297, 304, 306, 103, 298, 304, 102, 101, 106, 104, 356, 11, 12, 54, 55, 64, 104, 106, 107, 109,
  111, 115, 366, 367, 220, 224, 101, 269, 267, 344, 147, 142, 3, 101, 146, 344, 145, 304, 306, 299,
  101, 3, 4, 5, 6, 7, 8, 166, 167, 308, 165, 164, 344, 296, 299, 250, 251, 296, 102, 103, 252, 253,
  144, 304, 350, 351, 389, 248, 379, 268, 144, 268, 296, 310, 316, 317, 342, 32, 33, 227, 228, 229,
  346, 227, 288, 289, 290, 346, 221, 226, 297, 101, 106, 261, 102, 393, 107, 108, 275, 356, 341,
  187, 296, 112, 360, 106, 302, 309, 364, 365, 126, 103, 101, 112, 106, 385, 25, 27, 163, 298, 304,
  306, 312, 329, 330, 333, 334, 26, 50, 205, 191, 344, 375, 375, 375, 101, 176, 182, 101, 164, 175,
  175, 101, 106, 107, 346, 101, 126, 186, 190, 3, 111, 111, 64, 115, 108, 112, 29, 30, 31, 103, 148,
  149, 29, 30, 31, 37, 153, 154, 157, 296, 105, 344, 145, 103, 106, 346, 319, 101, 308, 106, 400,
  402, 395, 108, 84, 254, 256, 344, 303, 233, 356, 252, 24, 84, 85, 104, 105, 106, 109, 111, 123,
  335, 336, 367, 368, 369, 373, 374, 381, 382, 383, 385, 386, 389, 393, 101, 101, 101, 164, 316, 78,
  110, 232, 107, 111, 291, 292, 105, 107, 346, 270, 64, 109, 115, 370, 371, 373, 383, 394, 264, 369,
  276, 342, 105, 112, 361, 303, 84, 363, 389, 103, 196, 194, 296, 296, 296, 322, 204, 273, 277, 272,
  331, 273, 205, 64, 104, 106, 108, 109, 115, 373, 376, 377, 108, 108, 101, 101, 177, 180, 103, 318,
  112, 112, 344, 105, 156, 157, 106, 37, 155, 204, 141, 344, 167, 104, 104, 170, 400, 251, 204, 204,
  103, 219, 106, 257, 3, 234, 245, 108, 388, 384, 387, 101, 230, 223, 293, 292, 13, 14, 15, 16, 287,
  243, 271, 372, 371, 380, 106, 108, 107, 279, 289, 302, 197, 344, 332, 285, 286, 199, 348, 310,
  201, 273, 252, 273, 41, 42, 53, 101, 130, 135, 137, 150, 151, 152, 158, 159, 173, 183, 189, 191,
  214, 215, 216, 244, 266, 296, 297, 299, 312, 318, 296, 344, 296, 153, 168, 396, 101, 241, 227, 84,
  255, 318, 249, 375, 379, 375, 350, 252, 294, 295, 252, 374, 101, 103, 377, 378, 265, 280, 310,
  279, 104, 206, 207, 273, 283, 337, 206, 203, 108, 101, 344, 137, 151, 152, 189, 191, 214, 266,
  297, 312, 239, 101, 224, 244, 299, 204, 204, 154, 204, 370, 47, 256, 273, 246, 112, 385, 112, 67,
  236, 237, 108, 112, 370, 108, 370, 252, 208, 108, 273, 206, 181, 135, 143, 171, 191, 215, 312,
  318, 312, 346, 224, 226, 402, 258, 104, 235, 112, 238, 233, 352, 353, 108, 209, 381, 274, 282,
  354, 143, 171, 312, 239, 143, 204, 101, 346, 102, 259, 19, 55, 56, 312, 323, 325, 326, 235, 356,
  281, 381, 279, 32, 34, 45, 48, 102, 105, 144, 172, 354, 143, 354, 101, 395, 323, 327, 272, 282,
  402, 402, 395, 396, 146, 144, 354, 144, 172, 103, 328, 310, 350, 103, 101, 172, 144, 146, 310,
  396, 172, 101 };

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] = { 0, 124, 125, 126, 127, 126, 128, 128, 128, 128, 128, 128, 128,
  128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 129, 129, 129, 129, 129, 129, 130, 130, 131,
  132, 133, 132, 132, 134, 135, 135, 136, 136, 136, 137, 137, 139, 138, 141, 140, 140, 142, 140,
  140, 143, 143, 143, 144, 144, 144, 145, 145, 146, 146, 147, 148, 147, 147, 149, 149, 149, 150,
  150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 151, 151, 151, 151, 151, 151,
  152, 152, 152, 152, 153, 153, 154, 154, 154, 155, 155, 156, 156, 157, 157, 157, 158, 158, 158,
  159, 159, 161, 160, 162, 162, 163, 163, 163, 164, 165, 164, 166, 166, 167, 167, 168, 167, 169,
  170, 170, 171, 171, 171, 171, 172, 172, 173, 173, 174, 174, 174, 174, 174, 175, 176, 176, 177,
  178, 178, 180, 181, 179, 182, 183, 184, 185, 185, 186, 187, 187, 187, 187, 187, 187, 188, 190,
  189, 192, 191, 193, 194, 191, 195, 196, 195, 198, 199, 197, 200, 201, 197, 202, 203, 197, 204,
  204, 205, 205, 206, 206, 208, 207, 209, 209, 210, 210, 210, 210, 211, 212, 212, 212, 213, 213,
  213, 214, 214, 214, 215, 215, 215, 215, 216, 216, 216, 218, 219, 217, 220, 221, 223, 222, 224,
  225, 226, 227, 228, 228, 229, 229, 230, 230, 230, 231, 231, 232, 232, 232, 233, 233, 234, 235,
  235, 235, 235, 236, 236, 238, 237, 239, 239, 239, 240, 241, 241, 243, 242, 245, 246, 244, 248,
  247, 249, 249, 250, 250, 251, 251, 252, 253, 252, 254, 255, 254, 254, 254, 257, 258, 256, 259,
  259, 261, 260, 262, 260, 263, 260, 264, 265, 264, 266, 267, 268, 268, 269, 270, 269, 271, 272,
  272, 273, 274, 273, 275, 276, 275, 278, 277, 277, 279, 280, 281, 279, 279, 282, 282, 282, 282,
  282, 282, 283, 283, 284, 284, 285, 285, 286, 286, 287, 287, 287, 287, 288, 288, 290, 289, 291,
  291, 293, 292, 294, 295, 294, 296, 296, 297, 297, 297, 297, 297, 298, 298, 298, 299, 299, 299,
  299, 299, 299, 300, 299, 301, 302, 303, 305, 304, 307, 306, 308, 308, 308, 308, 308, 308, 308,
  308, 308, 309, 309, 309, 309, 309, 309, 310, 310, 311, 311, 311, 311, 312, 312, 313, 313, 313,
  313, 314, 314, 314, 314, 314, 315, 315, 315, 316, 316, 317, 317, 318, 320, 319, 321, 319, 322,
  322, 322, 323, 323, 324, 323, 323, 323, 325, 327, 326, 328, 326, 329, 331, 330, 332, 330, 333,
  333, 333, 333, 333, 333, 333, 334, 334, 335, 335, 335, 335, 335, 335, 335, 335, 335, 336, 336,
  336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 337, 337, 337, 337, 338, 339, 341,
  340, 342, 342, 343, 343, 345, 344, 347, 346, 349, 348, 351, 350, 353, 352, 355, 354, 356, 356,
  357, 358, 358, 359, 360, 360, 360, 360, 362, 361, 363, 363, 364, 364, 365, 365, 366, 366, 366,
  366, 366, 366, 366, 366, 366, 366, 366, 366, 366, 367, 367, 367, 367, 367, 367, 367, 367, 367,
  367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367,
  367, 367, 367, 367, 367, 367, 368, 368, 368, 368, 368, 368, 368, 368, 368, 368, 368, 368, 368,
  368, 368, 368, 368, 368, 368, 368, 368, 368, 368, 368, 368, 368, 368, 368, 368, 368, 369, 369,
  369, 369, 369, 369, 369, 369, 369, 369, 370, 370, 371, 371, 371, 372, 371, 371, 373, 373, 374,
  374, 374, 374, 374, 374, 374, 374, 374, 374, 374, 375, 375, 376, 376, 376, 376, 377, 377, 377,
  378, 378, 379, 379, 380, 380, 381, 381, 382, 382, 382, 384, 383, 385, 385, 387, 386, 388, 386,
  390, 389, 391, 389, 392, 389, 394, 393, 395, 395, 396, 396, 397, 397, 398, 398, 399, 399, 399,
  399, 399, 399, 399, 399, 399, 399, 399, 399, 399, 399, 399, 399, 399, 400, 401, 401, 402, 403,
  403, 403 };

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] = { 0, 2, 1, 0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 1, 2, 2, 2, 2, 2, 2, 5, 4, 5, 4, 0, 6, 6, 5, 1, 2, 4, 3, 5, 4, 5, 0, 5, 0, 7, 4, 0, 5, 2, 1, 1,
  1, 3, 4, 2, 1, 1, 0, 1, 0, 0, 4, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 2,
  2, 2, 3, 4, 3, 4, 1, 4, 2, 4, 4, 0, 1, 0, 1, 1, 1, 1, 5, 3, 6, 4, 5, 0, 5, 4, 3, 1, 2, 2, 0, 0, 3,
  1, 3, 0, 2, 0, 5, 6, 2, 1, 5, 6, 3, 4, 5, 3, 1, 2, 5, 5, 6, 5, 6, 2, 0, 3, 2, 1, 1, 0, 0, 8, 1, 3,
  2, 0, 3, 1, 1, 2, 2, 2, 3, 3, 4, 0, 8, 0, 5, 0, 0, 7, 1, 0, 4, 0, 0, 5, 0, 0, 5, 0, 0, 6, 0, 1, 1,
  1, 0, 1, 0, 3, 1, 2, 2, 2, 2, 2, 3, 4, 2, 3, 2, 3, 4, 2, 4, 5, 3, 1, 1, 2, 1, 2, 3, 0, 0, 7, 2, 2,
  0, 6, 2, 1, 2, 7, 0, 1, 1, 1, 0, 2, 1, 1, 1, 0, 1, 1, 0, 2, 1, 0, 2, 2, 2, 0, 1, 0, 3, 3, 1, 1, 6,
  0, 6, 0, 6, 0, 0, 8, 0, 5, 0, 2, 1, 3, 3, 3, 0, 0, 2, 1, 0, 4, 3, 1, 0, 0, 6, 0, 1, 0, 3, 0, 2, 0,
  4, 1, 0, 4, 4, 2, 0, 2, 0, 0, 4, 2, 0, 1, 3, 0, 6, 3, 0, 5, 0, 3, 1, 0, 0, 0, 7, 1, 0, 2, 2, 3, 3,
  2, 1, 2, 1, 2, 0, 1, 2, 4, 1, 1, 1, 1, 0, 1, 0, 2, 1, 2, 0, 5, 0, 0, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2,
  2, 2, 2, 2, 3, 3, 3, 0, 5, 1, 1, 1, 0, 5, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 3,
  1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 0, 3, 0, 4, 1, 3, 4, 1, 1, 0,
  4, 2, 2, 2, 0, 3, 0, 4, 2, 0, 3, 0, 4, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 0, 4, 0, 1, 1, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0,
  2, 0, 2, 0, 2, 4, 2, 1, 3, 0, 1, 2, 3, 0, 3, 0, 1, 1, 2, 1, 3, 2, 2, 3, 3, 1, 1, 1, 1, 2, 2, 2, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 0, 2, 1, 1, 1, 1, 1, 0, 4, 1, 1, 0, 4, 0, 5, 0, 4, 0, 4, 0, 4, 0, 4,
  0, 2, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 4, 3, 1, 1, 1 };

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
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0 };

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
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0 };

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
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* YYCONFLP[YYPACT[STATE-NUM]] -- Pointer into YYCONFL of start of
   list of conflicting reductions corresponding to action entry for
   state STATE-NUM in yytable.  0 means no conflicts.  The list in
   yyconfl is terminated by a rule number of 0.  */
static const yytype_uint8 yyconflp[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 239, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 241,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 237, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 231, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 233, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55,
  57, 59, 61, 63, 65, 67, 69, 0, 71, 73, 75, 77, 79, 81, 0, 83, 85, 87, 89, 0, 0, 91, 93, 95, 97,
  99, 101, 103, 105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125, 127, 129, 131, 133, 135, 137,
  139, 141, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 0, 167, 169, 171, 173, 175,
  177, 179, 181, 183, 185, 187, 189, 191, 193, 0, 195, 0, 0, 197, 199, 201, 0, 203, 205, 207, 0,
  209, 211, 213, 215, 217, 219, 221, 223, 225, 227, 229, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
static const short yyconfl[] = { 0, 412, 0, 412, 0, 412, 0, 325, 0, 632, 0, 632, 0, 632, 0, 632, 0,
  632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0,
  632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0,
  632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0,
  632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0,
  632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0,
  632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0,
  632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0,
  632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0,
  632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 632, 0, 308, 0,
  308, 0, 308, 0, 318, 0, 412, 0, 412, 0 };

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
  "typedef_declarator_id", "using_declaration", "using_declarator_list",
  "using_declarator_list_cont", "using_declarator", "using_id", "using_directive",
  "alias_declaration", "$@12", "template_head", "$@13", "$@14", "$@15", "template_parameter_list",
  "$@16", "template_parameter", "$@17", "$@18", "$@19", "$@20", "$@21", "$@22", "opt_ellipsis",
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
    case 2: /* translation_unit: opt_declaration_seq  */
    {
      /* release various resources when parse is done */
      clearType();
      clearTypeId();
      clearTemplate();
      clearIdAttrs();
      clearClassAttrs();
    }
    break;

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

      handle_complex_type(item, getDeclAttrCount(), getDeclAttrs(), getType(),
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

    case 156: /* using_declarator: using_id  */
    {
      add_using(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str), 0);
    }
    break;

    case 158: /* using_id: TYPENAME id_expression  */
    {
      ((*yyvalp).str) = vtkstrcat("typename ",
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 159: /* using_id: nested_name_specifier operator_function_id  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 160: /* using_id: nested_name_specifier conversion_function_id  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 161: /* using_id: scope_operator_sig nested_name_specifier operator_function_id  */
    {
      ((*yyvalp).str) = vtkstrcat3(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 162: /* using_id: scope_operator_sig nested_name_specifier conversion_function_id  */
    {
      ((*yyvalp).str) = vtkstrcat3(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 163: /* using_directive: USING NAMESPACE id_expression ';'  */
    {
      add_using(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str), 1);
    }
    break;

    case 164: /* $@12: %empty  */
    {
      markSig();
    }
    break;

    case 165: /* alias_declaration: USING id_expression id_attribute_specifier_seq '=' $@12
                 store_type direct_abstract_declarator ';'  */
    {
      ValueInfo* item = (ValueInfo*)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(item);
      item->ItemType = VTK_TYPEDEF_INFO;
      item->Access = access_level;

      handle_complex_type(item, getDeclAttrCount(), getDeclAttrs(), getType(),
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

    case 166: /* $@13: %empty  */
    {
      postSig("template<> ");
      clearTypeId();
    }
    break;

    case 168: /* $@14: %empty  */
    {
      postSig("template<");
      pushType();
      clearType();
      clearTypeId();
      startTemplate();
    }
    break;

    case 169: /* $@15: %empty  */
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

    case 172: /* $@16: %empty  */
    {
      chopSig();
      postSig(", ");
      clearType();
      clearTypeId();
    }
    break;

    case 174: /* $@17: %empty  */
    {
      markSig();
    }
    break;

    case 175: /* $@18: %empty  */
    {
      add_template_parameter(getType(),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer),
        copySig());
    }
    break;

    case 177: /* $@19: %empty  */
    {
      markSig();
    }
    break;

    case 178: /* $@20: %empty  */
    {
      add_template_parameter(0,
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer),
        copySig());
    }
    break;

    case 180: /* $@21: %empty  */
    {
      pushTemplate();
      markSig();
    }
    break;

    case 181: /* $@22: %empty  */
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

    case 183: /* opt_ellipsis: %empty  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 184: /* opt_ellipsis: ELLIPSIS  */
    {
      postSig("...");
      ((*yyvalp).integer) = VTK_PARSE_PACK;
    }
    break;

    case 185: /* class_or_typename: CLASS  */
    {
      postSig("class ");
    }
    break;

    case 186: /* class_or_typename: TYPENAME  */
    {
      postSig("typename ");
    }
    break;

    case 189: /* $@23: %empty  */
    {
      postSig("=");
      markSig();
    }
    break;

    case 190: /* template_parameter_initializer: '=' $@23 template_parameter_value  */
    {
      int i = currentTemplate->NumberOfParameters - 1;
      ValueInfo* param = currentTemplate->Parameters[i];
      chopSig();
      param->Value = copySig();
    }
    break;

    case 193: /* function_definition: function_declaration function_body  */
    {
      output_function();
    }
    break;

    case 194: /* function_definition: operator_declaration function_body  */
    {
      output_function();
    }
    break;

    case 195: /* function_definition: nested_method_declaration function_body  */
    {
      reject_function();
    }
    break;

    case 196: /* function_definition: nested_operator_declaration function_body  */
    {
      reject_function();
    }
    break;

    case 204: /* method_definition: method_declaration function_body  */
    {
      output_function();
    }
    break;

    case 214: /* $@24: %empty  */
    {
      postSig("(");
      currentFunction->IsExplicit = ((getType() & VTK_PARSE_EXPLICIT) != 0);
      set_return(currentFunction, getType(), getTypeId(), getDeclAttrCount(), getDeclAttrs());
    }
    break;

    case 215: /* $@25: %empty  */
    {
      postSig(")");
    }
    break;

    case 216: /* conversion_function: conversion_function_id '(' $@24 parameter_declaration_clause
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

    case 217: /* conversion_function_id: operator_sig store_type  */
    {
      ((*yyvalp).str) = copySig();
    }
    break;

    case 218: /* operator_function_nr: operator_function_sig function_trailer_clause  */
    {
      postSig(";");
      closeSig();
      currentFunction->Name =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed operator", currentFunction->Name);
    }
    break;

    case 219: /* $@26: %empty  */
    {
      postSig("(");
      currentFunction->IsOperator = 1;
      set_return(currentFunction, getType(), getTypeId(), getDeclAttrCount(), getDeclAttrs());
    }
    break;

    case 220: /* operator_function_sig: operator_function_id id_attribute_specifier_seq '(' $@26
                 parameter_declaration_clause ')'  */
    {
      postSig(")");
    }
    break;

    case 221: /* operator_function_id: operator_sig operator_id  */
    {
      chopSig();
      ((*yyvalp).str) = vtkstrcat(copySig(),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 222: /* operator_sig: OPERATOR  */
    {
      markSig();
      postSig("operator ");
    }
    break;

    case 223: /* function_nr: function_sig function_trailer_clause  */
    {
      postSig(";");
      closeSig();
      currentFunction->Name =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str);
      currentFunction->Comment = vtkstrdup(getComment());
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

    case 227: /* func_cv_qualifier: CONST  */
    {
      postSig(" const");
      currentFunction->IsConst = 1;
    }
    break;

    case 228: /* func_cv_qualifier: VOLATILE  */
    {
      postSig(" volatile");
    }
    break;

    case 230: /* opt_noexcept_specifier: noexcept_sig parentheses_sig  */
    {
      chopSig();
    }
    break;

    case 232: /* noexcept_sig: NOEXCEPT  */
    {
      postSig(" noexcept");
    }
    break;

    case 233: /* noexcept_sig: THROW  */
    {
      postSig(" throw");
    }
    break;

    case 235: /* opt_ref_qualifier: '&'  */
    {
      postSig("&");
    }
    break;

    case 236: /* opt_ref_qualifier: OP_LOGIC_AND  */
    {
      postSig("&&");
    }
    break;

    case 239: /* virt_specifier: ID  */
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

    case 241: /* opt_body_as_trailer: '=' DELETE  */
    {
      currentFunction->IsDeleted = 1;
    }
    break;

    case 243: /* opt_body_as_trailer: '=' ZERO  */
    {
      postSig(" = 0");
      currentFunction->IsPureVirtual = 1;
      if (currentClass)
      {
        currentClass->IsAbstract = 1;
      }
    }
    break;

    case 246: /* $@27: %empty  */
    {
      postSig(" -> ");
      clearType();
      clearTypeId();
    }
    break;

    case 247: /* trailing_return_type: OP_ARROW $@27 trailing_type_specifier_seq  */
    {
      chopSig();
      set_return(currentFunction, getType(), getTypeId(), getDeclAttrCount(), getDeclAttrs());
    }
    break;

    case 254: /* $@28: %empty  */
    {
      postSig("(");
      set_return(currentFunction, getType(), getTypeId(), getDeclAttrCount(), getDeclAttrs());
    }
    break;

    case 255: /* function_sig: unqualified_id id_attribute_specifier_seq '(' $@28
                 parameter_declaration_clause ')'  */
    {
      postSig(")");
    }
    break;

    case 256: /* $@29: %empty  */
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
      currentFunction->Name =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-3)].yystate.yysemantics.yyval.str);
      currentFunction->Comment = vtkstrdup(getComment());

      /* handle any attributes present in the declaration */
      handle_decl_attributes(currentFunction, getDeclAttrCount(), getDeclAttrs());
    }
    break;

    case 257: /* $@30: %empty  */
    {
      openSig();
    }
    break;

    case 258: /* structor_declaration: structor_sig opt_noexcept_specifier
                 func_attribute_specifier_seq virt_specifier_seq $@29 opt_ctor_initializer $@30
                 opt_body_as_trailer  */
    {
      postSig(";");
      closeSig();
      vtkParseDebug("Parsed func", currentFunction->Name);
    }
    break;

    case 259: /* $@31: %empty  */
    {
      pushType();
      postSig("(");
    }
    break;

    case 260: /* structor_sig: unqualified_id '(' $@31 parameter_declaration_clause ')'  */
    {
      postSig(")");
      popType();
    }
    break;

    case 268: /* $@32: %empty  */
    {
      clearType();
      clearTypeId();
    }
    break;

    case 270: /* parameter_list: parameter_declaration  */
    {
      clearType();
      clearTypeId();
    }
    break;

    case 271: /* $@33: %empty  */
    {
      clearType();
      clearTypeId();
      postSig(", ");
    }
    break;

    case 273: /* parameter_list: parameter_list ',' ELLIPSIS  */
    {
      currentFunction->IsVariadic = 1;
      postSig(", ...");
    }
    break;

    case 274: /* parameter_list: ELLIPSIS  */
    {
      currentFunction->IsVariadic = 1;
      postSig("...");
    }
    break;

    case 275: /* $@34: %empty  */
    {
      markSig();
    }
    break;

    case 276: /* $@35: %empty  */
    {
      ValueInfo* param = (ValueInfo*)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(param);

      handle_complex_type(param, getDeclAttrCount(), getDeclAttrs(), getType(),
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

    case 277: /* parameter_declaration: decl_attribute_specifier_seq $@34 store_type
                 direct_abstract_declarator $@35 opt_initializer  */
    {
      int i = currentFunction->NumberOfParameters - 1;
      if (getVarValue())
      {
        currentFunction->Parameters[i]->Value = getVarValue();
      }
    }
    break;

    case 278: /* opt_initializer: %empty  */
    {
      clearVarValue();
    }
    break;

    case 280: /* $@36: %empty  */
    {
      postSig("=");
      clearVarValue();
      markSig();
    }
    break;

    case 281: /* initializer: '=' $@36 constant_expression  */
    {
      chopSig();
      setVarValue(copySig());
    }
    break;

    case 282: /* $@37: %empty  */
    {
      clearVarValue();
      markSig();
    }
    break;

    case 283: /* initializer: $@37 braces_sig  */
    {
      chopSig();
      setVarValue(copySig());
    }
    break;

    case 284: /* $@38: %empty  */
    {
      clearVarValue();
      markSig();
      postSig("(");
    }
    break;

    case 285: /* initializer: $@38 '(' constructor_args ')'  */
    {
      chopSig();
      postSig(")");
      setVarValue(copySig());
    }
    break;

    case 286: /* constructor_args: literal  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 287: /* $@39: %empty  */
    {
      postSig(", ");
    }
    break;

    case 290: /* init_declarator_id: direct_declarator opt_initializer  */
    {
      const char** attrs = getDeclAttrs();
      int n = getDeclAttrCount();
      unsigned int type = getType();
      ValueInfo* var = (ValueInfo*)malloc(sizeof(ValueInfo));
      vtkParse_InitValue(var);
      var->ItemType = VTK_VARIABLE_INFO;
      var->Access = access_level;

      handle_complex_type(var, n, attrs, type,
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

    case 294: /* $@40: %empty  */
    {
      postSig(", ");
    }
    break;

    case 297: /* opt_ptr_operator_seq: %empty  */
    {
      setTypePtr(0);
    }
    break;

    case 298: /* opt_ptr_operator_seq: ptr_operator_seq  */
    {
      setTypePtr(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 299: /* direct_abstract_declarator: opt_ellipsis opt_declarator_id opt_array_or_parameters
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

    case 300: /* $@41: %empty  */
    {
      postSig(")");
    }
    break;

    case 301: /* direct_abstract_declarator: lp_or_la ref_attribute_specifier_seq
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

    case 302: /* direct_declarator: opt_ellipsis declarator_id opt_array_decorator_seq  */
    {
      ((*yyvalp).integer) =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.integer);
    }
    break;

    case 303: /* $@42: %empty  */
    {
      postSig(")");
    }
    break;

    case 304: /* direct_declarator: lp_or_la declarator ')' $@42 opt_array_or_parameters  */
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

    case 305: /* $@43: %empty  */
    {
      postSig("(");
      scopeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig("*");
    }
    break;

    case 306: /* lp_or_la: LP $@43 ptr_cv_qualifier_seq  */
    {
      ((*yyvalp).integer) =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer);
    }
    break;

    case 307: /* lp_or_la: LA  */
    {
      postSig("(");
      scopeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig("&");
      ((*yyvalp).integer) = VTK_PARSE_REF;
    }
    break;

    case 308: /* opt_array_or_parameters: %empty  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 309: /* $@44: %empty  */
    {
      pushFunction();
      postSig("(");
    }
    break;

    case 310: /* $@45: %empty  */
    {
      postSig(")");
    }
    break;

    case 311: /* opt_array_or_parameters: '(' $@44 parameter_declaration_clause ')' $@45
                 function_qualifiers func_attribute_specifier_seq  */
    {
      ((*yyvalp).integer) = VTK_PARSE_FUNCTION;
      popFunction();
    }
    break;

    case 312: /* opt_array_or_parameters: array_decorator_seq  */
    {
      ((*yyvalp).integer) = VTK_PARSE_ARRAY;
    }
    break;

    case 315: /* function_qualifiers: function_qualifiers CONST  */
    {
      currentFunction->IsConst = 1;
    }
    break;

    case 320: /* abstract_declarator: ptr_operator_seq direct_abstract_declarator  */
    {
      ((*yyvalp).integer) = add_indirection(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 322: /* declarator: ptr_operator_seq direct_declarator  */
    {
      ((*yyvalp).integer) = add_indirection(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 323: /* opt_declarator_id: %empty  */
    {
      clearVarName();
      chopSig();
    }
    break;

    case 325: /* declarator_id: unqualified_id id_attribute_specifier_seq  */
    {
      setVarName((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 326: /* declarator_id: unqualified_id id_attribute_specifier_seq ':' bitfield_size  */
    {
      setVarName((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-3)].yystate.yysemantics.yyval.str));
    }
    break;

    case 331: /* opt_array_decorator_seq: %empty  */
    {
      clearArray();
    }
    break;

    case 333: /* $@46: %empty  */
    {
      clearArray();
    }
    break;

    case 337: /* $@47: %empty  */
    {
      postSig("[");
    }
    break;

    case 338: /* array_decorator: '[' $@47 array_size_specifier ']' array_attribute_specifier_seq */
    {
      postSig("]");
    }
    break;

    case 339: /* array_size_specifier: %empty  */
    {
      pushArraySize("");
    }
    break;

    case 340: /* $@48: %empty  */
    {
      markSig();
    }
    break;

    case 341: /* array_size_specifier: $@48 constant_expression  */
    {
      chopSig();
      pushArraySize(copySig());
    }
    break;

    case 347: /* unqualified_id: tilde_sig class_name  */
    {
      ((*yyvalp).str) = vtkstrcat(
        "~", (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 348: /* unqualified_id: tilde_sig decltype_specifier  */
    {
      ((*yyvalp).str) = vtkstrcat(
        "~", (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 349: /* qualified_id: nested_name_specifier unqualified_id  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 350: /* qualified_id: scope_operator_sig unqualified_id  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 351: /* qualified_id: scope_operator_sig qualified_id  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 352: /* nested_name_specifier: identifier_sig scope_operator_sig  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 353: /* nested_name_specifier: template_id scope_operator_sig  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 354: /* nested_name_specifier: decltype_specifier scope_operator_sig  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 355: /* nested_name_specifier: nested_name_specifier identifier_sig scope_operator_sig  */
    {
      ((*yyvalp).str) = vtkstrcat3(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 356: /* nested_name_specifier: nested_name_specifier template_id scope_operator_sig  */
    {
      ((*yyvalp).str) = vtkstrcat3(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 357: /* nested_name_specifier: nested_name_specifier decltype_specifier scope_operator_sig
               */
    {
      ((*yyvalp).str) = vtkstrcat3(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-2)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 358: /* $@49: %empty  */
    {
      postSig("template ");
    }
    break;

    case 359: /* nested_name_specifier: nested_name_specifier TEMPLATE $@49 template_id
                 scope_operator_sig  */
    {
      ((*yyvalp).str) = vtkstrcat4(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-4)].yystate.yysemantics.yyval.str),
        "template ",
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 360: /* tilde_sig: '~'  */
    {
      postSig("~");
    }
    break;

    case 361: /* identifier_sig: identifier  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 362: /* scope_operator_sig: DOUBLE_COLON  */
    {
      ((*yyvalp).str) = "::";
      postSig(((*yyvalp).str));
    }
    break;

    case 363: /* $@50: %empty  */
    {
      markSig();
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
      postSig("<");
    }
    break;

    case 364: /* template_id: identifier '<' $@50 angle_bracket_contents right_angle_bracket  */
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

    case 365: /* $@51: %empty  */
    {
      markSig();
      postSig("decltype");
    }
    break;

    case 366: /* decltype_specifier: DECLTYPE $@51 parentheses_sig  */
    {
      chopSig();
      ((*yyvalp).str) = copySig();
      clearTypeId();
    }
    break;

    case 367: /* simple_id: VTK_ID  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 368: /* simple_id: QT_ID  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 369: /* simple_id: ID  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 370: /* simple_id: ISTREAM  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 371: /* simple_id: OSTREAM  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 372: /* simple_id: StdString  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 373: /* simple_id: NULLPTR_T  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 374: /* simple_id: SIZE_T  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 375: /* simple_id: SSIZE_T  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 385: /* decl_specifier2: primitive_type  */
    {
      setTypeBase(buildTypeBase(getType(),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer)));
    }
    break;

    case 386: /* decl_specifier2: TYPEDEF  */
    {
      setTypeMod(VTK_PARSE_TYPEDEF);
    }
    break;

    case 387: /* decl_specifier2: FRIEND  */
    {
      setTypeMod(VTK_PARSE_FRIEND);
    }
    break;

    case 390: /* decl_specifier: storage_class_specifier  */
    {
      setTypeMod(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 391: /* decl_specifier: function_specifier  */
    {
      setTypeMod(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 392: /* decl_specifier: cv_qualifier  */
    {
      setTypeMod(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 393: /* decl_specifier: CONSTEXPR  */
    {
      postSig("constexpr ");
      ((*yyvalp).integer) = 0;
    }
    break;

    case 394: /* storage_class_specifier: MUTABLE  */
    {
      postSig("mutable ");
      ((*yyvalp).integer) = VTK_PARSE_MUTABLE;
    }
    break;

    case 395: /* storage_class_specifier: EXTERN  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 396: /* storage_class_specifier: EXTERN STRING_LITERAL  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 397: /* storage_class_specifier: STATIC  */
    {
      postSig("static ");
      ((*yyvalp).integer) = VTK_PARSE_STATIC;
    }
    break;

    case 398: /* storage_class_specifier: THREAD_LOCAL  */
    {
      postSig("thread_local ");
      ((*yyvalp).integer) = VTK_PARSE_THREAD_LOCAL;
    }
    break;

    case 399: /* function_specifier: INLINE  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 400: /* function_specifier: VIRTUAL  */
    {
      postSig("virtual ");
      ((*yyvalp).integer) = VTK_PARSE_VIRTUAL;
    }
    break;

    case 401: /* function_specifier: EXPLICIT  */
    {
      postSig("explicit ");
      ((*yyvalp).integer) = VTK_PARSE_EXPLICIT;
    }
    break;

    case 402: /* cv_qualifier: CONST  */
    {
      postSig("const ");
      ((*yyvalp).integer) = VTK_PARSE_CONST;
    }
    break;

    case 403: /* cv_qualifier: VOLATILE  */
    {
      postSig("volatile ");
      ((*yyvalp).integer) = VTK_PARSE_VOLATILE;
    }
    break;

    case 405: /* cv_qualifier_seq: cv_qualifier_seq cv_qualifier  */
    {
      ((*yyvalp).integer) =
        ((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer) |
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 407: /* $@52: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 409: /* $@53: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 412: /* type_specifier: class_key class_attribute_specifier_seq class_head_name  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 413: /* type_specifier: enum_key class_attribute_specifier_seq id_expression
                 decl_attribute_specifier_seq  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 415: /* trailing_type_specifier: decltype_specifier  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = 0;
    }
    break;

    case 416: /* $@54: %empty  */
    {
      postSig("typename ");
    }
    break;

    case 417: /* trailing_type_specifier: TYPENAME $@54 id_expression decl_attribute_specifier_seq
               */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 418: /* trailing_type_specifier: template_id decl_attribute_specifier_seq  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 419: /* trailing_type_specifier: qualified_id decl_attribute_specifier_seq  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str));
    }
    break;

    case 421: /* $@55: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 423: /* $@56: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 426: /* $@57: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 428: /* $@58: %empty  */
    {
      setTypeBase(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 431: /* tparam_type_specifier: decltype_specifier  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = 0;
    }
    break;

    case 432: /* tparam_type_specifier: template_id  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 433: /* tparam_type_specifier: qualified_id  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 434: /* tparam_type_specifier: STRUCT id_expression  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 435: /* tparam_type_specifier: UNION id_expression  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 436: /* tparam_type_specifier: enum_key id_expression  */
    {
      postSig(" ");
      setTypeId((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = guess_id_type(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 437: /* simple_type_specifier: primitive_type decl_attribute_specifier_seq  */
    {
      setTypeId("");
    }
    break;

    case 439: /* type_name: StdString  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_STRING;
    }
    break;

    case 440: /* type_name: OSTREAM  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_OSTREAM;
    }
    break;

    case 441: /* type_name: ISTREAM  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_ISTREAM;
    }
    break;

    case 442: /* type_name: ID  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_UNKNOWN;
    }
    break;

    case 443: /* type_name: VTK_ID  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_OBJECT;
    }
    break;

    case 444: /* type_name: QT_ID  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_QOBJECT;
    }
    break;

    case 445: /* type_name: NULLPTR_T  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_NULLPTR_T;
    }
    break;

    case 446: /* type_name: SSIZE_T  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_SSIZE_T;
    }
    break;

    case 447: /* type_name: SIZE_T  */
    {
      typeSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      ((*yyvalp).integer) = VTK_PARSE_SIZE_T;
    }
    break;

    case 448: /* primitive_type: AUTO  */
    {
      postSig("auto ");
      ((*yyvalp).integer) = 0;
    }
    break;

    case 449: /* primitive_type: VOID  */
    {
      postSig("void ");
      ((*yyvalp).integer) = VTK_PARSE_VOID;
    }
    break;

    case 450: /* primitive_type: BOOL  */
    {
      postSig("bool ");
      ((*yyvalp).integer) = VTK_PARSE_BOOL;
    }
    break;

    case 451: /* primitive_type: FLOAT  */
    {
      postSig("float ");
      ((*yyvalp).integer) = VTK_PARSE_FLOAT;
    }
    break;

    case 452: /* primitive_type: DOUBLE  */
    {
      postSig("double ");
      ((*yyvalp).integer) = VTK_PARSE_DOUBLE;
    }
    break;

    case 453: /* primitive_type: CHAR  */
    {
      postSig("char ");
      ((*yyvalp).integer) = VTK_PARSE_CHAR;
    }
    break;

    case 454: /* primitive_type: CHAR16_T  */
    {
      postSig("char16_t ");
      ((*yyvalp).integer) = VTK_PARSE_CHAR16_T;
    }
    break;

    case 455: /* primitive_type: CHAR32_T  */
    {
      postSig("char32_t ");
      ((*yyvalp).integer) = VTK_PARSE_CHAR32_T;
    }
    break;

    case 456: /* primitive_type: WCHAR_T  */
    {
      postSig("wchar_t ");
      ((*yyvalp).integer) = VTK_PARSE_WCHAR_T;
    }
    break;

    case 457: /* primitive_type: INT  */
    {
      postSig("int ");
      ((*yyvalp).integer) = VTK_PARSE_INT;
    }
    break;

    case 458: /* primitive_type: SHORT  */
    {
      postSig("short ");
      ((*yyvalp).integer) = VTK_PARSE_SHORT;
    }
    break;

    case 459: /* primitive_type: LONG  */
    {
      postSig("long ");
      ((*yyvalp).integer) = VTK_PARSE_LONG;
    }
    break;

    case 460: /* primitive_type: SIGNED  */
    {
      postSig("signed ");
      ((*yyvalp).integer) = VTK_PARSE_INT;
    }
    break;

    case 461: /* primitive_type: UNSIGNED  */
    {
      postSig("unsigned ");
      ((*yyvalp).integer) = VTK_PARSE_UNSIGNED_INT;
    }
    break;

    case 465: /* ptr_operator_seq: pointer_seq reference  */
    {
      ((*yyvalp).integer) =
        ((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.integer) |
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 466: /* reference: '&' ref_attribute_specifier_seq  */
    {
      postSig("&");
      ((*yyvalp).integer) = VTK_PARSE_REF;
    }
    break;

    case 467: /* rvalue_reference: OP_LOGIC_AND ref_attribute_specifier_seq  */
    {
      postSig("&&");
      ((*yyvalp).integer) = (VTK_PARSE_RVALUE | VTK_PARSE_REF);
    }
    break;

    case 468: /* $@59: %empty  */
    {
      postSig("*");
    }
    break;

    case 469: /* pointer: '*' ref_attribute_specifier_seq $@59 ptr_cv_qualifier_seq  */
    {
      ((*yyvalp).integer) =
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer);
    }
    break;

    case 470: /* ptr_cv_qualifier_seq: %empty  */
    {
      ((*yyvalp).integer) = VTK_PARSE_POINTER;
    }
    break;

    case 471: /* ptr_cv_qualifier_seq: cv_qualifier_seq  */
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

    case 473: /* pointer_seq: pointer_seq pointer  */
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

    case 474: /* $@60: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_DECL);
    }
    break;

    case 475: /* decl_attribute_specifier_seq: $@60 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 476: /* $@61: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_ID);
      clearIdAttrs();
    }
    break;

    case 477: /* id_attribute_specifier_seq: $@61 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 478: /* $@62: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_REF);
    }
    break;

    case 479: /* ref_attribute_specifier_seq: $@62 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 480: /* $@63: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_FUNC);
    }
    break;

    case 481: /* func_attribute_specifier_seq: $@63 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 482: /* $@64: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_ARRAY);
    }
    break;

    case 483: /* array_attribute_specifier_seq: $@64 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 484: /* $@65: %empty  */
    {
      setAttributeRole(VTK_PARSE_ATTRIB_CLASS);
      clearClassAttrs();
    }
    break;

    case 485: /* class_attribute_specifier_seq: $@65 attribute_specifier_seq  */
    {
      clearAttributeRole();
    }
    break;

    case 488: /* attribute_specifier: BEGIN_ATTRIB attribute_specifier_contents ']' ']'  */
    {
      setAttributePrefix(NULL);
    }
    break;

    case 491: /* attribute_using_prefix: USING using_id ':'  */
    {
      setAttributePrefix(vtkstrcat(
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str), "::"));
    }
    break;

    case 496: /* $@66: %empty  */
    {
      markSig();
    }
    break;

    case 497: /* attribute: $@66 attribute_sig attribute_pack  */
    {
      chopSig();
      handle_attribute(cutSig(),
        (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.integer));
    }
    break;

    case 498: /* attribute_pack: %empty  */
    {
      ((*yyvalp).integer) = 0;
    }
    break;

    case 499: /* attribute_pack: ELLIPSIS  */
    {
      ((*yyvalp).integer) = VTK_PARSE_PACK;
    }
    break;

    case 504: /* operator_id: '(' ')'  */
    {
      ((*yyvalp).str) = "()";
    }
    break;

    case 505: /* operator_id: '[' ']'  */
    {
      ((*yyvalp).str) = "[]";
    }
    break;

    case 506: /* operator_id: NEW '[' ']'  */
    {
      ((*yyvalp).str) = " new[]";
    }
    break;

    case 507: /* operator_id: DELETE '[' ']'  */
    {
      ((*yyvalp).str) = " delete[]";
    }
    break;

    case 508: /* operator_id: '<'  */
    {
      ((*yyvalp).str) = "<";
    }
    break;

    case 509: /* operator_id: '>'  */
    {
      ((*yyvalp).str) = ">";
    }
    break;

    case 510: /* operator_id: ','  */
    {
      ((*yyvalp).str) = ",";
    }
    break;

    case 511: /* operator_id: '='  */
    {
      ((*yyvalp).str) = "=";
    }
    break;

    case 512: /* operator_id: OP_RSHIFT_A '>'  */
    {
      ((*yyvalp).str) = ">>";
    }
    break;

    case 513: /* operator_id: OP_RSHIFT_A OP_RSHIFT_A  */
    {
      ((*yyvalp).str) = ">>";
    }
    break;

    case 514: /* operator_id: STRING_LITERAL ID  */
    {
      ((*yyvalp).str) =
        vtkstrcat((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(-1)].yystate.yysemantics.yyval.str),
          (YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
    }
    break;

    case 517: /* operator_id_no_delim: '%'  */
    {
      ((*yyvalp).str) = "%";
    }
    break;

    case 518: /* operator_id_no_delim: '*'  */
    {
      ((*yyvalp).str) = "*";
    }
    break;

    case 519: /* operator_id_no_delim: '/'  */
    {
      ((*yyvalp).str) = "/";
    }
    break;

    case 520: /* operator_id_no_delim: '-'  */
    {
      ((*yyvalp).str) = "-";
    }
    break;

    case 521: /* operator_id_no_delim: '+'  */
    {
      ((*yyvalp).str) = "+";
    }
    break;

    case 522: /* operator_id_no_delim: '!'  */
    {
      ((*yyvalp).str) = "!";
    }
    break;

    case 523: /* operator_id_no_delim: '~'  */
    {
      ((*yyvalp).str) = "~";
    }
    break;

    case 524: /* operator_id_no_delim: '&'  */
    {
      ((*yyvalp).str) = "&";
    }
    break;

    case 525: /* operator_id_no_delim: '|'  */
    {
      ((*yyvalp).str) = "|";
    }
    break;

    case 526: /* operator_id_no_delim: '^'  */
    {
      ((*yyvalp).str) = "^";
    }
    break;

    case 527: /* operator_id_no_delim: NEW  */
    {
      ((*yyvalp).str) = " new";
    }
    break;

    case 528: /* operator_id_no_delim: DELETE  */
    {
      ((*yyvalp).str) = " delete";
    }
    break;

    case 529: /* operator_id_no_delim: OP_LSHIFT_EQ  */
    {
      ((*yyvalp).str) = "<<=";
    }
    break;

    case 530: /* operator_id_no_delim: OP_RSHIFT_EQ  */
    {
      ((*yyvalp).str) = ">>=";
    }
    break;

    case 531: /* operator_id_no_delim: OP_LSHIFT  */
    {
      ((*yyvalp).str) = "<<";
    }
    break;

    case 532: /* operator_id_no_delim: OP_DOT_POINTER  */
    {
      ((*yyvalp).str) = ".*";
    }
    break;

    case 533: /* operator_id_no_delim: OP_ARROW_POINTER  */
    {
      ((*yyvalp).str) = "->*";
    }
    break;

    case 534: /* operator_id_no_delim: OP_ARROW  */
    {
      ((*yyvalp).str) = "->";
    }
    break;

    case 535: /* operator_id_no_delim: OP_PLUS_EQ  */
    {
      ((*yyvalp).str) = "+=";
    }
    break;

    case 536: /* operator_id_no_delim: OP_MINUS_EQ  */
    {
      ((*yyvalp).str) = "-=";
    }
    break;

    case 537: /* operator_id_no_delim: OP_TIMES_EQ  */
    {
      ((*yyvalp).str) = "*=";
    }
    break;

    case 538: /* operator_id_no_delim: OP_DIVIDE_EQ  */
    {
      ((*yyvalp).str) = "/=";
    }
    break;

    case 539: /* operator_id_no_delim: OP_REMAINDER_EQ  */
    {
      ((*yyvalp).str) = "%=";
    }
    break;

    case 540: /* operator_id_no_delim: OP_INCR  */
    {
      ((*yyvalp).str) = "++";
    }
    break;

    case 541: /* operator_id_no_delim: OP_DECR  */
    {
      ((*yyvalp).str) = "--";
    }
    break;

    case 542: /* operator_id_no_delim: OP_AND_EQ  */
    {
      ((*yyvalp).str) = "&=";
    }
    break;

    case 543: /* operator_id_no_delim: OP_OR_EQ  */
    {
      ((*yyvalp).str) = "|=";
    }
    break;

    case 544: /* operator_id_no_delim: OP_XOR_EQ  */
    {
      ((*yyvalp).str) = "^=";
    }
    break;

    case 545: /* operator_id_no_delim: OP_LOGIC_AND  */
    {
      ((*yyvalp).str) = "&&";
    }
    break;

    case 546: /* operator_id_no_delim: OP_LOGIC_OR  */
    {
      ((*yyvalp).str) = "||";
    }
    break;

    case 547: /* operator_id_no_delim: OP_LOGIC_EQ  */
    {
      ((*yyvalp).str) = "==";
    }
    break;

    case 548: /* operator_id_no_delim: OP_LOGIC_NEQ  */
    {
      ((*yyvalp).str) = "!=";
    }
    break;

    case 549: /* operator_id_no_delim: OP_LOGIC_LEQ  */
    {
      ((*yyvalp).str) = "<=";
    }
    break;

    case 550: /* operator_id_no_delim: OP_LOGIC_GEQ  */
    {
      ((*yyvalp).str) = ">=";
    }
    break;

    case 551: /* keyword: TYPEDEF  */
    {
      ((*yyvalp).str) = "typedef";
    }
    break;

    case 552: /* keyword: TYPENAME  */
    {
      ((*yyvalp).str) = "typename";
    }
    break;

    case 553: /* keyword: CLASS  */
    {
      ((*yyvalp).str) = "class";
    }
    break;

    case 554: /* keyword: STRUCT  */
    {
      ((*yyvalp).str) = "struct";
    }
    break;

    case 555: /* keyword: UNION  */
    {
      ((*yyvalp).str) = "union";
    }
    break;

    case 556: /* keyword: TEMPLATE  */
    {
      ((*yyvalp).str) = "template";
    }
    break;

    case 557: /* keyword: PUBLIC  */
    {
      ((*yyvalp).str) = "public";
    }
    break;

    case 558: /* keyword: PROTECTED  */
    {
      ((*yyvalp).str) = "protected";
    }
    break;

    case 559: /* keyword: PRIVATE  */
    {
      ((*yyvalp).str) = "private";
    }
    break;

    case 560: /* keyword: CONST  */
    {
      ((*yyvalp).str) = "const";
    }
    break;

    case 561: /* keyword: VOLATILE  */
    {
      ((*yyvalp).str) = "volatile";
    }
    break;

    case 562: /* keyword: STATIC  */
    {
      ((*yyvalp).str) = "static";
    }
    break;

    case 563: /* keyword: THREAD_LOCAL  */
    {
      ((*yyvalp).str) = "thread_local";
    }
    break;

    case 564: /* keyword: CONSTEXPR  */
    {
      ((*yyvalp).str) = "constexpr";
    }
    break;

    case 565: /* keyword: INLINE  */
    {
      ((*yyvalp).str) = "inline";
    }
    break;

    case 566: /* keyword: VIRTUAL  */
    {
      ((*yyvalp).str) = "virtual";
    }
    break;

    case 567: /* keyword: EXPLICIT  */
    {
      ((*yyvalp).str) = "explicit";
    }
    break;

    case 568: /* keyword: DECLTYPE  */
    {
      ((*yyvalp).str) = "decltype";
    }
    break;

    case 569: /* keyword: DEFAULT  */
    {
      ((*yyvalp).str) = "default";
    }
    break;

    case 570: /* keyword: EXTERN  */
    {
      ((*yyvalp).str) = "extern";
    }
    break;

    case 571: /* keyword: USING  */
    {
      ((*yyvalp).str) = "using";
    }
    break;

    case 572: /* keyword: NAMESPACE  */
    {
      ((*yyvalp).str) = "namespace";
    }
    break;

    case 573: /* keyword: OPERATOR  */
    {
      ((*yyvalp).str) = "operator";
    }
    break;

    case 574: /* keyword: ENUM  */
    {
      ((*yyvalp).str) = "enum";
    }
    break;

    case 575: /* keyword: THROW  */
    {
      ((*yyvalp).str) = "throw";
    }
    break;

    case 576: /* keyword: NOEXCEPT  */
    {
      ((*yyvalp).str) = "noexcept";
    }
    break;

    case 577: /* keyword: CONST_CAST  */
    {
      ((*yyvalp).str) = "const_cast";
    }
    break;

    case 578: /* keyword: DYNAMIC_CAST  */
    {
      ((*yyvalp).str) = "dynamic_cast";
    }
    break;

    case 579: /* keyword: STATIC_CAST  */
    {
      ((*yyvalp).str) = "static_cast";
    }
    break;

    case 580: /* keyword: REINTERPRET_CAST  */
    {
      ((*yyvalp).str) = "reinterpret_cast";
    }
    break;

    case 595: /* constant_expression_item: '<'  */
    {
      postSig("< ");
    }
    break;

    case 596: /* $@67: %empty  */
    {
      postSig("> ");
    }
    break;

    case 598: /* constant_expression_item: OP_RSHIFT_A  */
    {
      postSig(">");
    }
    break;

    case 600: /* common_bracket_item: DOUBLE_COLON  */
    {
      chopSig();
      postSig("::");
    }
    break;

    case 604: /* common_bracket_item_no_scope_operator: operator_id_no_delim  */
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

    case 605: /* common_bracket_item_no_scope_operator: ':'  */
    {
      postSig(":");
      postSig(" ");
    }
    break;

    case 606: /* common_bracket_item_no_scope_operator: '.'  */
    {
      postSig(".");
    }
    break;

    case 607: /* common_bracket_item_no_scope_operator: keyword  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig(" ");
    }
    break;

    case 608: /* common_bracket_item_no_scope_operator: literal  */
    {
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig(" ");
    }
    break;

    case 610: /* common_bracket_item_no_scope_operator: type_name  */
    {
      chopSig();
      postSig(" ");
    }
    break;

    case 611: /* common_bracket_item_no_scope_operator: ELLIPSIS  */
    {
      postSig("...");
    }
    break;

    case 615: /* bracket_pitem: '<'  */
    {
      postSig("< ");
    }
    break;

    case 616: /* bracket_pitem: '>'  */
    {
      postSig("> ");
    }
    break;

    case 617: /* bracket_pitem: OP_RSHIFT_A  */
    {
      postSig(">");
    }
    break;

    case 619: /* any_bracket_item: '='  */
    {
      postSig("= ");
    }
    break;

    case 620: /* any_bracket_item: ','  */
    {
      chopSig();
      postSig(", ");
    }
    break;

    case 622: /* braces_item: ';'  */
    {
      chopSig();
      postSig(";");
    }
    break;

    case 630: /* angle_bracket_item: '='  */
    {
      postSig("= ");
    }
    break;

    case 631: /* angle_bracket_item: ','  */
    {
      chopSig();
      postSig(", ");
    }
    break;

    case 632: /* $@68: %empty  */
    {
      chopSig();
      if (getSig()[getSigLength() - 1] == '<')
      {
        postSig(" ");
      }
      postSig("<");
    }
    break;

    case 633: /* angle_brackets_sig: '<' $@68 angle_bracket_contents right_angle_bracket  */
    {
      chopSig();
      if (getSig()[getSigLength() - 1] == '>')
      {
        postSig(" ");
      }
      postSig("> ");
    }
    break;

    case 636: /* $@69: %empty  */
    {
      postSigLeftBracket("[");
    }
    break;

    case 637: /* brackets_sig: '[' $@69 any_bracket_contents ']'  */
    {
      postSigRightBracket("] ");
    }
    break;

    case 638: /* $@70: %empty  */
    {
      postSig("[[");
    }
    break;

    case 639: /* brackets_sig: BEGIN_ATTRIB $@70 any_bracket_contents ']' ']'  */
    {
      chopSig();
      postSig("]] ");
    }
    break;

    case 640: /* $@71: %empty  */
    {
      postSigLeftBracket("(");
    }
    break;

    case 641: /* parentheses_sig: '(' $@71 any_bracket_contents ')'  */
    {
      postSigRightBracket(") ");
    }
    break;

    case 642: /* $@72: %empty  */
    {
      postSigLeftBracket("(");
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig("*");
    }
    break;

    case 643: /* parentheses_sig: LP $@72 any_bracket_contents ')'  */
    {
      postSigRightBracket(") ");
    }
    break;

    case 644: /* $@73: %empty  */
    {
      postSigLeftBracket("(");
      postSig((YY_CAST(yyGLRStackItem const*, yyvsp)[YYFILL(0)].yystate.yysemantics.yyval.str));
      postSig("&");
    }
    break;

    case 645: /* parentheses_sig: LA $@73 any_bracket_contents ')'  */
    {
      postSigRightBracket(") ");
    }
    break;

    case 646: /* $@74: %empty  */
    {
      postSig("{ ");
    }
    break;

    case 647: /* braces_sig: '{' $@74 braces_contents '}'  */
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

/* NOLINTNEXTLINE(bugprone-suspicious-include) */
#include "lex.yy.c"

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
  int i;
  int n = getClassAttrCount();
  const char** attrs = getClassAttrs();
  parse_attribute_return_t attrResult;

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

  for (i = 0; i < n; i++)
  {
    attrResult = vtkParse_ClassAttribute(currentClass, attrs[i], preprocessor);
    handle_attribute_error(attrs[i], attrResult);
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
    if (strncmp(name, "typename ", 9) == 0)
    {
      item->IsType = 1;
      name += 9;
    }
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
  int i;
  int n = getClassAttrCount();
  const char** attrs = getClassAttrs();
  parse_attribute_return_t attrResult;

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

    for (i = 0; i < n; i++)
    {
      attrResult = vtkParse_ClassAttribute(currentClass, attrs[i], preprocessor);
      handle_attribute_error(attrs[i], attrResult);
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
      snprintf(&text[i], sizeof(text) - i, "%li", j + 1);
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

  add_constant(
    name, currentEnumValue, currentEnumType, currentEnumName, getIdAttrCount(), getIdAttrs(), 2);
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
static void add_constant(const char* name, const char* value, unsigned int type,
  const char* typeclass, int nattrs, const char** attrs, int flag)
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

  /* handle any attributes that were present in the declaration */
  handle_value_attributes(con, nattrs, attrs);

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
  handle_complex_type(param, 0, NULL, datatype, extra, funcSig);
  param->Name = getVarName();
  vtkParse_AddParameterToTemplate(currentTemplate, param);
}

/* attach attributes to a value (variable, parameter, etc.) */
static void handle_value_attributes(ValueInfo* val, int n, const char** attrs)
{
  int i;
  parse_attribute_return_t attrResult;

  for (i = 0; i < n; ++i)
  {
    attrResult = vtkParse_ValueAttribute(val, attrs[i], preprocessor);
    handle_attribute_error(attrs[i], attrResult);
  }
}

/* attach attributes to either the function or its return value */
static void handle_decl_attributes(FunctionInfo* func, int n, const char** attrs)
{
  int i;
  parse_attribute_return_t attrResult;

  for (i = 0; i < n; ++i)
  {
    attrResult = vtkParse_FunctionAttribute(func, attrs[i], preprocessor);
    if (func->ReturnValue && attrResult == VTK_ATTRIB_HANDLER_SKIPPED)
    {
      attrResult = vtkParse_ValueAttribute(func->ReturnValue, attrs[i], preprocessor);
    }
    handle_attribute_error(attrs[i], attrResult);
  }
}

/* set the return type for the current function */
static void set_return(
  FunctionInfo* func, unsigned int type, const char* typeclass, int n, const char** attrs)
{
  ValueInfo* val = (ValueInfo*)malloc(sizeof(ValueInfo));

  vtkParse_InitValue(val);
  val->Type = type;
  val->Class = type_class(type, typeclass);

  if (func->ReturnValue)
  {
    /* there will already be a placeholder if we are setting a trailing
     * return return value, so we free the placeholder first */
    vtkParse_FreeValue(func->ReturnValue);
  }
  func->ReturnValue = val;

  /* attach attributes to either the function or its return value */
  handle_decl_attributes(func, n, attrs);

#ifndef VTK_PARSE_LEGACY_REMOVE
  func->ReturnType = val->Type;
  func->ReturnClass = val->Class;
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
static void handle_complex_type(ValueInfo* val, int n, const char** attrs, unsigned int datatype,
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
    func->ReturnValue->Type = datatype;
    func->ReturnValue->Class = type_class(datatype, getTypeId());
    if (funcSig)
    {
      func->Signature = vtkstrdup(funcSig);
    }
    val->Function = func;

    /* attach attributes to either the function or its return value */
    handle_decl_attributes(func, n, attrs);

    /* set n=0 since all attributes have been consumed */
    n = 0;

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

  /* handle any attributes that were present in the declaration */
  handle_value_attributes(val, n, attrs);
}

/* handle [[attributes]] */
static void handle_attribute(const char* attr, int pack)
{
  /* the role means "this is what the attribute applies to" */
  int role = getAttributeRole();

  if (!attr)
  {
    return;
  }

  /* append the prefix from the "using" statement */
  if (getAttributePrefix())
  {
    attr = vtkstrcat(getAttributePrefix(), attr);
  }

  if (pack && strncmp(attr, "vtk::", 5) == 0)
  {
    /* pack attributes are not handled (yet) */
    const char* errtext = "\"...\" not allowed with VTK attributes.";
    print_parser_error(attr, errtext, strlen(errtext));
    exit(1);
  }

  if (role == VTK_PARSE_ATTRIB_DECL)
  {
    /* store the attribute until declaration is complete */
    addDeclAttr(attr);
  }
  else if (role == VTK_PARSE_ATTRIB_ID)
  {
    /* store the attribute until the identifier is processed */
    addIdAttr(attr);
  }
  else if (role == VTK_PARSE_ATTRIB_CLASS)
  {
    /* store the attribute until the class declaration begins */
    addClassAttr(attr);
  }
  else if (role == VTK_PARSE_ATTRIB_FUNC)
  {
    /* this is for attributes that follow the parameter list */
    parse_attribute_return_t attrResult;
    attrResult = vtkParse_AfterFunctionAttribute(currentFunction, attr, preprocessor);
    handle_attribute_error(attr, attrResult);
  }
  else if (role == VTK_PARSE_ATTRIB_REF)
  {
    /* no handling for attributes that modify *, &, etc. */
    handle_attribute_error(attr, VTK_ATTRIB_HANDLER_SKIPPED);
  }
  else if (role == VTK_PARSE_ATTRIB_ARRAY)
  {
    /* no handling for attributes that modify arrays */
    handle_attribute_error(attr, VTK_ATTRIB_HANDLER_SKIPPED);
  }
}

/* check attribute handling return code for error information */
static void handle_attribute_error(const char* attr, parse_attribute_return_t rcode)
{
  if (rcode == VTK_ATTRIB_HANDLER_ERRORED)
  {
    /* get error text and print it */
    size_t n = 0;
    const char* errtext = vtkParse_GetAttributeError();
    if (errtext)
    {
      n = strlen(errtext);
    }
    print_parser_error(attr, errtext, n);
    exit(1); /* fatal error */
  }
  else if (rcode == VTK_ATTRIB_HANDLER_NO_ARGS)
  {
    size_t l = vtkParse_SkipId(attr);
    while (attr[l] == ':' && attr[l + 1] == ':')
    {
      l += 2;
      l += vtkParse_SkipId(&attr[l]);
    }
    if (attr[l] == '(')
    {
      const char* errtext = "attribute takes no arguments.";
      print_parser_error(attr, errtext, strlen(errtext));
      exit(1); /* fatal error */
    }
  }
  else if (rcode == VTK_ATTRIB_HANDLER_SKIPPED)
  {
    /* is this an unrecognized VTK attribute? */
    if (strncmp(attr, "vtk::", 5) == 0)
    {
      const char* errtext = "attribute is not recognized in this context.";
      print_parser_error(attr, errtext, strlen(errtext));
      exit(1); /* fatal error */
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

// NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)
// NOLINTEND(bugprone-unsafe-functions)
