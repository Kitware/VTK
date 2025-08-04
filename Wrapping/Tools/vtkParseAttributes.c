// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
  This file manages custom wrapping attributes.

  VTK wrapping attributes are allowed in the following locations:
  1. before a function: `[[vtk::attribute]] int function()` *
  2. after a function:  `int function() [[vtk::attribute]]` **
  3. before a class:    `class [[vtk::attribute]] classname {...}`
  4. before a value:    `[[vtk::attribute]] int x`
  5. after an enum id:  `enum { ID [[vtk::attribute]] = 0 }

  * If declaration attributes aren't handled by the function itself,
    then they apply to the return value of the function.

  ** If attributes refer to the function parameters, then they are
    placed after the parameter list.

  New attributes must be added to the `parse_attribute_t` enumeration,
  to the `AttributeEnumTable`, and to the case list of one or more of
  the `handle_<item>_attribute()` functions below.

  If attributes are to be inherited by subclasses, then it might be
  necessary to edit vtkParseMerge.c so that it copies the associated
  members of FunctionInfo and ClassInfo to derived classes.
*/

#include "vtkParseAttributes.h"
#include "vtkParseData.h"
#include "vtkParsePreprocess.h"
#include "vtkParseString.h"

#include <string.h>

/* Each attribute has an associated enum constant */
typedef enum parse_attribute_t_
{
  VTK_ATTRIB_NONE,           /* nonexistent attribute   */
  VTK_ATTRIB_NEWINSTANCE,    /* [[vtk::newinstance]]    */
  VTK_ATTRIB_ZEROCOPY,       /* [[vtk::zerocopy]]       */
  VTK_ATTRIB_WRAPEXCLUDE,    /* [[vtk::filepath]]       */
  VTK_ATTRIB_FILEPATH,       /* [[vtk::wrapexclude]]    */
  VTK_ATTRIB_SIZEHINT,       /* [[vtk::sizehint()]]     */
  VTK_ATTRIB_EXPECTS,        /* [[vtk::expects()]]      */
  VTK_ATTRIB_UNBLOCKTHREADS, /* [[vtk::unblockthreads]] */
  VTK_ATTRIB_DEPRECATED,     /* [[vtk::deprecated()]]   */
  VTK_ATTRIB_MARSHALAUTO,    /* [[vtk::marshalauto]]    */
  VTK_ATTRIB_MARSHALMANUAL,  /* [[vtk::marshalmanual]]  */
  VTK_ATTRIB_MARSHALEXCLUDE, /* [[vtk::marshalexclude]] */
  VTK_ATTRIB_MARSHALGETTER,  /* [[vtk::marshalgetter]]  */
  VTK_ATTRIB_MARSHALSETTER,  /* [[vtk::marshalsetter]]  */
  VTK_ATTRIB_PROPEXCLUDE,    /* [[vtk::propexclude]]    */
} parse_attribute_t;

/* Map attribute names to attribute enum constants */
static const struct
{
  const char* name;
  parse_attribute_t value;
} AttributeEnumTable[] = {
  { "vtk::newinstance", VTK_ATTRIB_NEWINSTANCE },
  { "vtk::zerocopy", VTK_ATTRIB_ZEROCOPY },
  { "vtk::wrapexclude", VTK_ATTRIB_WRAPEXCLUDE },
  { "vtk::filepath", VTK_ATTRIB_FILEPATH },
  { "vtk::sizehint", VTK_ATTRIB_SIZEHINT },
  { "vtk::expects", VTK_ATTRIB_EXPECTS },
  { "vtk::unblockthreads", VTK_ATTRIB_UNBLOCKTHREADS },
  { "vtk::deprecated", VTK_ATTRIB_DEPRECATED },
  { "vtk::marshalauto", VTK_ATTRIB_MARSHALAUTO },
  { "vtk::marshalmanual", VTK_ATTRIB_MARSHALMANUAL },
  { "vtk::marshalexclude", VTK_ATTRIB_MARSHALEXCLUDE },
  { "vtk::marshalgetter", VTK_ATTRIB_MARSHALGETTER },
  { "vtk::marshalsetter", VTK_ATTRIB_MARSHALSETTER },
  { "vtk::propexclude", VTK_ATTRIB_PROPEXCLUDE },
  { NULL, VTK_ATTRIB_NONE },
};

/* -------------------------------------------------------------------- */

/* for error reporting */
static const char* attributeErrorText = NULL;

/* Set the error text for the caller to use if VTK_HANDLER_ERRORED is returned.
 * Example output: "[[{attribute}]]: {errtext}: {detail}"
 * If `detail` is NULL: "[[{attribute}]]: {errtext}."
 * The {attribute} will include the attribute argument list. */
static void set_attribute_error(
  const char* errtext, const char* detail, size_t n, PreprocessInfo* preprocessor)
{
  if (errtext)
  {
    char* cp;
    size_t l = strlen(errtext);

    if (detail)
    {
      size_t m = l + n + 2;
      cp = vtkParse_NewString(preprocessor->Strings, m);
      snprintf(cp, m + 1, "%s: %*.*s", errtext, (int)n, (int)n, detail);
    }
    else
    {
      size_t m = l + 1;
      cp = vtkParse_NewString(preprocessor->Strings, m);
      snprintf(cp, m + 1, "%s.", errtext);
    }
    attributeErrorText = cp;
  }
  else
  {
    attributeErrorText = NULL;
  }
}

/* -------------------------------------------------------------------- */

/* Get the attribute enum value, given the name and length */
static parse_attribute_t get_attribute_id(const char* name, size_t l)
{
  int i;
  for (i = 0; AttributeEnumTable[i].name; ++i)
  {
    if (strlen(AttributeEnumTable[i].name) == l &&
      strncmp(AttributeEnumTable[i].name, name, l) == 0)
    {
      return AttributeEnumTable[i].value;
    }
  }
  return VTK_ATTRIB_NONE;
}

/* Parse the arguments for a single quoted string, e.g. "hello world" */
static parse_attribute_return_t parse_quoted_arg(
  const char* args, const char** arg, PreprocessInfo* preprocessor)
{
  size_t lr;
  if (!args || args[0] != '\"')
  {
    set_attribute_error("requires a argument in double quotes", NULL, 0, preprocessor);
    return VTK_ATTRIB_HANDLER_ERRORED;
  }

  lr = vtkParse_SkipQuotes(args);
  if (lr != strlen(args))
  {
    set_attribute_error("requires a single argument", NULL, 0, preprocessor);
    return VTK_ATTRIB_HANDLER_ERRORED;
  }

  if (arg)
  {
    *arg = vtkParse_CacheString(preprocessor->Strings, args, strlen(args));
  }

  return VTK_ATTRIB_HANDLER_HANDLED;
}

/* Parse arguments for a single quoted id, which is returned without quotes */
static parse_attribute_return_t parse_quoted_identifier_arg(
  const char* args, const char** identifier, PreprocessInfo* preprocessor)
{
  parse_attribute_return_t rcode = parse_quoted_arg(args, NULL, preprocessor);

  if (rcode == VTK_ATTRIB_HANDLER_HANDLED)
  {
    size_t l = vtkParse_SkipId(&args[1]);
    if (args[l + 1] != '\"')
    {
      set_attribute_error("requires a single identifier in double quotes", NULL, 0, preprocessor);
      return VTK_ATTRIB_HANDLER_ERRORED;
    }

    if (identifier)
    {
      *identifier = vtkParse_CacheString(preprocessor->Strings, &args[1], l);
    }
  }

  return rcode;
}

/* Parse arguments for deprecation information ("reason", "version") */
static parse_attribute_return_t parse_deprecation_args(
  const char* args, const char** reason, const char** version, PreprocessInfo* preprocessor)
{
  if (args)
  {
    if (args[0] != '\"')
    {
      set_attribute_error("arguments must be in double quotes", NULL, 0, preprocessor);
      return VTK_ATTRIB_HANDLER_ERRORED;
    }
    size_t lr = vtkParse_SkipQuotes(args);
    *reason = vtkParse_CacheString(preprocessor->Strings, args, lr);
    if (args[lr] == ',')
    {
      /* skip spaces and get the next argument */
      do
      {
        ++lr;
      } while (args[lr] == ' ');

      if (args[lr] != '\"')
      {
        set_attribute_error("arguments must be in double quotes", NULL, 0, preprocessor);
        return VTK_ATTRIB_HANDLER_ERRORED;
      }
      *version =
        vtkParse_CacheString(preprocessor->Strings, &args[lr], vtkParse_SkipQuotes(&args[lr]));
    }
  }
  return VTK_ATTRIB_HANDLER_HANDLED;
}

/* -------------------------------------------------------------------- */

/* Handle class, struct, and enum type attributes:
 *     class [[attribute]] classname { ... }    */
static parse_attribute_return_t handle_class_attribute(
  ClassInfo* cls, parse_attribute_t attribute, const char* args, PreprocessInfo* preprocessor)
{
  switch (attribute)
  {
    case VTK_ATTRIB_WRAPEXCLUDE:
    {
      cls->IsExcluded = 1;
      return VTK_ATTRIB_HANDLER_NO_ARGS;
    }
    case VTK_ATTRIB_DEPRECATED:
    {
      cls->IsDeprecated = 1;
      return parse_deprecation_args(
        args, &cls->DeprecatedReason, &cls->DeprecatedVersion, preprocessor);
    }
    case VTK_ATTRIB_MARSHALAUTO:
    {
      cls->MarshalType = VTK_MARSHAL_AUTO_MODE;
      return VTK_ATTRIB_HANDLER_NO_ARGS;
    }
    case VTK_ATTRIB_MARSHALMANUAL:
    {
      cls->MarshalType = VTK_MARSHAL_MANUAL_MODE;
      return VTK_ATTRIB_HANDLER_NO_ARGS;
    }
    default:
    {
      return VTK_ATTRIB_HANDLER_SKIPPED;
    }
  }
}

/* Handle function attributes that appear before the function:
 *     [[attribute]] int function()
 * Any attributes not handled here will be applied to the return value
 * rather than to the function itself. */
static parse_attribute_return_t handle_function_attribute(
  FunctionInfo* func, parse_attribute_t attribute, const char* args, PreprocessInfo* preprocessor)
{
  switch (attribute)
  {
    case VTK_ATTRIB_WRAPEXCLUDE:
    {
      func->IsExcluded = 1;
      return VTK_ATTRIB_HANDLER_NO_ARGS;
    }
    case VTK_ATTRIB_PROPEXCLUDE:
    {
      func->IsPropExcluded = 1;
      return VTK_ATTRIB_HANDLER_NO_ARGS;
    }
    case VTK_ATTRIB_DEPRECATED:
    {
      func->IsDeprecated = 1;
      return parse_deprecation_args(
        args, &func->DeprecatedReason, &func->DeprecatedVersion, preprocessor);
    }
    case VTK_ATTRIB_MARSHALEXCLUDE:
    {
      func->IsMarshalExcluded = 1;
      return parse_quoted_arg(args, &func->MarshalExcludeReason, preprocessor);
    }
    case VTK_ATTRIB_MARSHALGETTER:
    {
      return parse_quoted_identifier_arg(args, &func->MarshalPropertyName, preprocessor);
    }
    case VTK_ATTRIB_MARSHALSETTER:
    {
      return parse_quoted_identifier_arg(args, &func->MarshalPropertyName, preprocessor);
    }
    default:
    {
      return VTK_ATTRIB_HANDLER_SKIPPED;
    }
  }
}

/* Handle attributes that appear after the function parameter list:
 *     int function(int x) [[attribute]]
 * This style should only be used for attributes that refer to the parameters
 * or that might need access to the parameters. */
static parse_attribute_return_t handle_after_function_attribute(
  FunctionInfo* func, parse_attribute_t attribute, const char* args, PreprocessInfo* preprocessor)
{
  switch (attribute)
  {
    case VTK_ATTRIB_EXPECTS:
    {
      /* add to the preconditions */
      vtkParse_AddStringToArray(&func->Preconds, &func->NumberOfPreconds, args);
      return VTK_ATTRIB_HANDLER_HANDLED;
    }
    case VTK_ATTRIB_SIZEHINT:
    {
      /* first arg is parameter name, unless return value hint */
      ValueInfo* arg = func->ReturnValue;
      size_t n = vtkParse_SkipId(args);
      size_t l = n;
      preproc_int_t count;
      int is_unsigned;
      int i;

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
        for (i = 0; i < func->NumberOfParameters; i++)
        {
          arg = func->Parameters[i];
          if (arg->Name && strlen(arg->Name) == l && strncmp(arg->Name, args, l) == 0)
          {
            break;
          }
        }
        if (i == func->NumberOfParameters)
        {
          /* underscore by itself signifies the return value */
          if (l == 1 && args[0] == '_')
          {
            arg = func->ReturnValue;
          }
          else
          {
            set_attribute_error("unrecognized parameter name", args, l, preprocessor);
            return VTK_ATTRIB_HANDLER_ERRORED;
          }
        }
        /* advance args to second attribute arg */
        args += n;
      }
      /* set the size hint */
      arg->CountHint = args;
      /* see if hint is an integer */
      if (VTK_PARSE_OK ==
        vtkParsePreprocess_EvaluateExpression(preprocessor, arg->CountHint, &count, &is_unsigned))
      {
        if (count > 0 && count < 127)
        {
          arg->CountHint = NULL;
          arg->Count = (int)count;
#ifndef VTK_PARSE_LEGACY_REMOVE
          if (arg == func->ReturnValue)
          {
            func->HaveHint = 1;
            func->HintSize = arg->Count;
          }
#endif
        }
      }
      return VTK_ATTRIB_HANDLER_HANDLED;
    }
    default:
    {
      return VTK_ATTRIB_HANDLER_SKIPPED;
    }
  }
}

/* Handle attributes for parameters, return values, variables, etc:
 *     1. [[attribute]] int function()
 *     2. int function([[attribute]] int x)
 *     3. [[attribute]] int variable        */
static parse_attribute_return_t handle_value_attribute(
  ValueInfo* val, parse_attribute_t attribute, const char* args, PreprocessInfo* preprocessor)
{
  switch (attribute)
  {
    case VTK_ATTRIB_NEWINSTANCE:
    {
      val->Attributes |= VTK_PARSE_NEWINSTANCE;
      return VTK_ATTRIB_HANDLER_NO_ARGS;
    }
    case VTK_ATTRIB_ZEROCOPY:
    {
      val->Attributes |= VTK_PARSE_ZEROCOPY;
      return VTK_ATTRIB_HANDLER_NO_ARGS;
    }
    case VTK_ATTRIB_FILEPATH:
    {
      val->Attributes |= VTK_PARSE_FILEPATH;
      return VTK_ATTRIB_HANDLER_NO_ARGS;
    }
    case VTK_ATTRIB_WRAPEXCLUDE:
    {
      val->Attributes |= VTK_PARSE_WRAPEXCLUDE;
      return VTK_ATTRIB_HANDLER_NO_ARGS;
    }
    case VTK_ATTRIB_DEPRECATED:
    {
      val->Attributes |= VTK_PARSE_DEPRECATED;
      return parse_deprecation_args(
        args, &val->DeprecatedReason, &val->DeprecatedVersion, preprocessor);
    }
    case VTK_ATTRIB_UNBLOCKTHREADS:
    {
      val->Attributes |= VTK_PARSE_UNBLOCKTHREADS;
      return VTK_ATTRIB_HANDLER_NO_ARGS;
    }
    default:
    {
      return VTK_ATTRIB_HANDLER_SKIPPED;
    }
  }
}

/* Handle attributes that occur after enum constant name:
 *     1. enum { name [[attribute]] = value }
 *     2. enum { name [[attribute]] }         */
static parse_attribute_return_t handle_after_value_attribute(
  ValueInfo* val, parse_attribute_t attribute, const char* args, PreprocessInfo* preprocessor)
{
  switch (attribute)
  {
    case VTK_ATTRIB_WRAPEXCLUDE:
    {
      val->Attributes |= VTK_PARSE_WRAPEXCLUDE;
      return VTK_ATTRIB_HANDLER_NO_ARGS;
    }
    case VTK_ATTRIB_DEPRECATED:
    {
      val->Attributes |= VTK_PARSE_DEPRECATED;
      return parse_deprecation_args(
        args, &val->DeprecatedReason, &val->DeprecatedVersion, preprocessor);
    }
    default:
    {
      return VTK_ATTRIB_HANDLER_SKIPPED;
    }
  }
}

/* -------------------------------------------------------------------- */

/* Split an attribute into its identifier and its argument list.
 * If there are no arguments, then *argp will be set to NULL.
 * The parentheses are stripped from the args before they are returned. */
static parse_attribute_t split_attribute(
  const char* attr, const char** argp, PreprocessInfo* preprocessor)
{
  parse_attribute_t attrId;
  size_t l = 0;
  size_t la = 0;
  const char* args = NULL;

  /* get the attribute's enum value */
  l = vtkParse_SkipId(attr);
  while (attr[l] == ':' && attr[l + 1] == ':')
  {
    l += 2;
    l += vtkParse_SkipId(&attr[l]);
  }
  attrId = get_attribute_id(attr, l);

  /* search for arguments */
  if (attr[l] == '(')
  {
    /* strip the parentheses and whitespace from the args */
    args = &attr[l + 1];
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
    args = vtkParse_CacheString(preprocessor->Strings, args, la);
  }

  *argp = args;
  return attrId;
}

/* -------------------------------------------------------------------- */

/* Handle class and struct attributes */
parse_attribute_return_t vtkParse_ClassAttribute(
  ClassInfo* cls, const char* attr, PreprocessInfo* preprocessor)
{
  parse_attribute_t attrId;
  const char* args;
  attrId = split_attribute(attr, &args, preprocessor);
  return handle_class_attribute(cls, attrId, args, preprocessor);
}

/* Handle function attributes that appear before the function */
parse_attribute_return_t vtkParse_FunctionAttribute(
  FunctionInfo* func, const char* attr, PreprocessInfo* preprocessor)
{
  parse_attribute_t attrId;
  const char* args;
  attrId = split_attribute(attr, &args, preprocessor);
  return handle_function_attribute(func, attrId, args, preprocessor);
}

/* Handle attributes that appear after the function */
parse_attribute_return_t vtkParse_AfterFunctionAttribute(
  FunctionInfo* func, const char* attr, PreprocessInfo* preprocessor)
{
  parse_attribute_t attrId;
  const char* args;
  attrId = split_attribute(attr, &args, preprocessor);
  return handle_after_function_attribute(func, attrId, args, preprocessor);
}

/* Handle attributes for parameters, return values, variables, etc. */
parse_attribute_return_t vtkParse_ValueAttribute(
  ValueInfo* val, const char* attr, PreprocessInfo* preprocessor)
{
  parse_attribute_t attrId;
  const char* args;
  attrId = split_attribute(attr, &args, preprocessor);
  return handle_value_attribute(val, attrId, args, preprocessor);
}

/* Handle attributes for parameters, return values, variables, etc. */
parse_attribute_return_t vtkParse_AfterValueAttribute(
  ValueInfo* val, const char* attr, PreprocessInfo* preprocessor)
{
  parse_attribute_t attrId;
  const char* args;
  attrId = split_attribute(attr, &args, preprocessor);
  return handle_after_value_attribute(val, attrId, args, preprocessor);
}

/* Get the last attribute processing error */
const char* vtkParse_GetAttributeError(void)
{
  return attributeErrorText;
}
