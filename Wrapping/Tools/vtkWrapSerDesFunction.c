// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWrapSerDesFunction.h"
#include "vtkParseExtras.h"
#include "vtkParseHierarchy.h"
#include "vtkParseString.h"
#include "vtkWrap.h"
#include "vtkWrapText.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(bugprone-unsafe-functions)

/* A zero copy pointer refers to memory that outlives the call and that neither side copies. Its
 * length is not part of the signature, so its contents cannot be copied in or out. It travels as
 * the literal numeric address (uintptr_t) of its first element instead. The type of the elements
 * behind that address must be inferred from the type manifest.
 *
 * Which side owns the memory depends on the direction, and neither direction transfers it:
 * - a parameter, as in SetArray(VTK_ZEROCOPY T* array, ...), points at memory of the client's
 *   that the object borrows for as long as it holds on to it.
 * - a return value, as in VTK_ZEROCOPY T* GetPointer(vtkIdType), points into memory of the
 *   object's that the client borrows until the object reallocates or goes away.
 */
int vtkWrapSerDes_CanMarshalZeroCopyPointer(const ValueInfo* valInfo)
{
  return (
    vtkWrap_IsZeroCopyPointer(valInfo) && vtkWrap_IsNumeric(valInfo) && !vtkWrap_IsBool(valInfo));
}

static int vtkWrapSerDes_IsIdentifierChar(char c)
{
  return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
}

/* Report whether a size hint refers to one of the parameters of the method, as in
 * VTK_SIZEHINT(values, numValues). Such a hint cannot be evaluated before the arguments have been
 * read out of the json. */
static int vtkWrapSerDes_HintNamesParameter(const FunctionInfo* functionInfo, const char* hint)
{
  int i = 0;
  for (i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    const char* name = functionInfo->Parameters[i]->Name;
    if (name == NULL || name[0] == '\0')
    {
      continue;
    }
    const size_t length = strlen(name);
    const char* at = hint;
    while ((at = strstr(at, name)) != NULL)
    {
      const char before = (at == hint) ? '\0' : at[-1];
      const char after = at[length];
      if (!vtkWrapSerDes_IsIdentifierChar(before) && !vtkWrapSerDes_IsIdentifierChar(after))
      {
        return 1;
      }
      at += length;
    }
  }
  return 0;
}

/* Report whether the number of elements of an array value can be written out here.
 * 'Trivial' means the expression is not relying on any function parameter. This means
 * we can write the expression without deserializing any function parameter.
 */
static int vtkWrapSerDes_HasTrivialCountExpression(
  const FunctionInfo* functionInfo, const ValueInfo* valInfo)
{
  // If we know the exact count, use it.
  if (valInfo->Count > 0)
  {
    return 1;
  }
  return (valInfo->CountHint != NULL &&
    !vtkWrapSerDes_HintNamesParameter(functionInfo, valInfo->CountHint));
}

/* Write the expression that gives the number of elements of an array value. A size hint is an
 * expression in terms of the members of the class, as in VTK_SIZEHINT(GetNumberOfComponents()). */
static void vtkWrapSerDes_WriteCountExpression(
  FILE* fp, const FunctionInfo* functionInfo, const ValueInfo* valInfo)
{
  assert(vtkWrapSerDes_HasTrivialCountExpression(functionInfo, valInfo));
  (void)functionInfo;
  if (valInfo->Count > 0)
  {
    fprintf(fp, "%d", valInfo->Count);
    return;
  }
  const char* hint = valInfo->CountHint;
  // we do not have 'this' here, replace with 'object'
  if (!strncmp(hint, "this->", 6))
  {
    hint += 6;
  }
  fprintf(fp, "object->%s", hint);
}

/* -------------------------------------------------------------------- */
/* Write out an expression that a header states about a method, as in
 * VTK_EXPECTS(0 <= i && i < GetNumberOfTuples()), in terms of the locals of the invoker. An
 * unqualified name is the object under the call, a parameter or a member of the class. */
static void vtkWrapSerDes_SubstituteCode(
  FILE* fp, const ClassInfo* classInfo, const FunctionInfo* functionInfo, const char* code)
{
  StringTokenizer tokenizer;
  int qualified = 0;
  int j = 0;
  (void)classInfo;

  vtkParse_InitTokenizer(&tokenizer, code, WS_DEFAULT);
  do
  {
    int matched = 0;
    /* TOK_ID == "any C++ identifier" */
    if (tokenizer.tok == TOK_ID && !qualified)
    {
      /* 'this' transforms to 'object' */
      if (tokenizer.len == 4 && !strncmp(tokenizer.text, "this", 4))
      {
        fprintf(fp, "object");
        matched = 1;
      }
      /* match function arguments to 'arg_j' */
      for (j = 0; !matched && j < functionInfo->NumberOfParameters; ++j)
      {
        const char* name = functionInfo->Parameters[j]->Name;
        if (name && strlen(name) == tokenizer.len && !strncmp(name, tokenizer.text, tokenizer.len))
        {
          fprintf(fp, "arg_%d", j);
          matched = 1;
        }
      }
      /* a name that opens a call is a method of the class. The superclasses of the class are not
       * merged into it here, so the name is not looked up any further than that. */
      if (!matched && tokenizer.text[tokenizer.len] == '(')
      {
        fprintf(fp, "object->%*.*s", (int)tokenizer.len, (int)tokenizer.len, tokenizer.text);
        matched = 1;
      }
    }
    if (!matched)
    {
      /* anything else like '(', ')' lands here */
      fprintf(fp, "%*.*s", (int)tokenizer.len, (int)tokenizer.len, tokenizer.text);
    }
    /* a name that follows one of these is looked up in a scope of its own */
    qualified = (tokenizer.tok == '.' || tokenizer.tok == TOK_ARROW || tokenizer.tok == TOK_SCOPE ||
      tokenizer.tok == TOK_ARROW_STAR || tokenizer.tok == TOK_DOT_STAR);
    fprintf(fp, " ");
  } while (vtkParse_NextToken(&tokenizer));
}

/* Write the checks that run the preconditions of a method. They guard against arguments that
 * would make the method read or write out of bounds. */
static void vtkWrapSerDes_WritePreconditionCheck(
  FILE* fp, const ClassInfo* classInfo, const FunctionInfo* functionInfo)
{
  int i = 0;
  fprintf(fp, "    if (");
  for (i = 0; i < functionInfo->NumberOfPreconds; ++i)
  {
    fprintf(fp, "%s(", (i == 0 ? "" : "\n      && "));
    vtkWrapSerDes_SubstituteCode(fp, classInfo, functionInfo, functionInfo->Preconds[i]);
    fprintf(fp, ")");
  }
  fprintf(fp, ")\n    {\n");
}

/* An out parameter is a numeric array that the method fills in rather than reads. The client does
 * not supply it. The invoker sizes it, hands it to the method and returns its contents as the
 * value of the call.
 */
static int vtkWrapSerDes_IsOutParameter(const FunctionInfo* functionInfo, const ValueInfo* valInfo)
{
  return (vtkWrap_IsArray(valInfo) && vtkWrap_IsNumeric(valInfo) && !vtkWrap_IsConst(valInfo) &&
    !vtkWrap_IsRef(valInfo) && vtkWrapSerDes_HasTrivialCountExpression(functionInfo, valInfo));
}

/* Return the index of the single out parameter of a method, or -1 when it has none. A result
 * carries one value, so a method that returns something, or that fills in more than one array,
 * keeps the plain treatment where every parameter is read from the arguments.
 */
int vtkWrapSerDes_FindOutParameterPosition(const FunctionInfo* functionInfo)
{
  int i = 0;
  int found = -1;
  if (!vtkWrap_IsVoid(functionInfo->ReturnValue))
  {
    return -1;
  }
  if (strncmp(functionInfo->Name, "Get", 3) != 0)
  {
    return -1;
  }
  for (i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    if (vtkWrapSerDes_IsOutParameter(functionInfo, functionInfo->Parameters[i]))
    {
      if (found >= 0)
      {
        return -1;
      }
      found = i;
    }
  }
  return found;
}

static int vtkWrapSerDes_CanMarshalValue(
  ValueInfo* valInfo, const ClassInfo* classInfo, const HierarchyInfo* hinfo, int isReturnValue)
{
  if (isReturnValue == 1 && vtkWrap_IsVoid(valInfo))
  {
    return 1;
  }

  const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, valInfo->Class);
  const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(valInfo);
  const int isPointer = vtkWrap_IsPointer(valInfo);
  const int isScalar = vtkWrap_IsScalar(valInfo);
  const int isNumeric = vtkWrap_IsNumeric(valInfo);
  const int isString = vtkWrap_IsString(valInfo);
  const int isCharPointer = vtkWrap_IsCharPointer(valInfo);
  const int isEnumMember = vtkWrap_IsEnumMember(classInfo, valInfo);
  const int isEnum = valInfo->IsEnum;
  const int isArray = vtkWrap_IsArray(valInfo);
  const int isStdVector = vtkWrap_IsStdVector(valInfo);

  int isAllowed = -1;
  // vtkAbstractBuffer is not exposed.
  if (strstr(valInfo->Class, "vtkAbstractBuffer") != NULL ||
    strstr(valInfo->Class, "vtkBuffer") != NULL)
  {
    isAllowed = 0;
  }
  // Array classes do not get recognized as a template class through valInfo->Template.
  else if (strstr(valInfo->Class, "vtkAOSDataArrayTemplate") != NULL ||
    strstr(valInfo->Class, "vtkScaledSOADataArrayTemplate") != NULL || // VTK_DEPRECATED_IN_9_7_0
    strstr(valInfo->Class, "vtkSOADataArrayTemplate") != NULL ||
    strstr(valInfo->Class, "vtkAffineArray") != NULL ||
    strstr(valInfo->Class, "vtkCompositeArray") != NULL ||
    strstr(valInfo->Class, "vtkConstantArray") != NULL ||
    strstr(valInfo->Class, "vtkIndexedArray") != NULL ||
    strstr(valInfo->Class, "vtkStdFunctionArray") != NULL || // VTK_DEPRECATED_IN_9_7_0
    strstr(valInfo->Class, "vtkStridedArray") != NULL ||
    strstr(valInfo->Class, "vtkStructuredPointArray") != NULL)
  {
    isAllowed = 0;
  }
  else if (valInfo->Template != NULL)
  {
    isAllowed = 0;
  }
  else if (vtkWrap_IsNonConstRef(valInfo))
  {
    isAllowed = 0;
  }
  else if (!strcmp(valInfo->Class, "vtkStdString") && isPointer)
  {
    isAllowed = 0;
  }
  else if ((isVTKObject && (isPointer || isVTKSmartPointer)) || isVTKSmartPointer)
  {
    isAllowed = 1;
  }
  else if (isNumeric)
  {
    if (vtkWrapSerDes_CanMarshalZeroCopyPointer(valInfo))
    {
      isAllowed = 1;
    }
    else if (isScalar)
    {
      isAllowed = 1;
    }
    else if (isArray)
    {
      isAllowed = 1;
    }
    else if (isCharPointer)
    {
      isAllowed = 1;
    }
    else
    {
      isAllowed = 0;
    }
  }
  else if (isString)
  {
    isAllowed = 1;
  }
  else if (isEnumMember)
  {
    isAllowed = 1;
  }
  else if (isEnum)
  {
    isAllowed = 1;
  }
  else if (!strncmp(valInfo->Class, "vtkVector", 9) || !strncmp(valInfo->Class, "vtkTuple", 8) ||
    !strncmp(valInfo->Class, "vtkColor", 8) || !strncmp(valInfo->Class, "vtkRect", 7))
  {
    isAllowed = 1;
  }
  else if (!strcmp(valInfo->Class, "vtkBoundingBox"))
  {
    isAllowed = 1;
  }
  else if (isStdVector)
  {
    const char* arg = vtkWrap_TemplateArg(valInfo->Class);
    size_t n;
    ValueInfo* element = (ValueInfo*)calloc(1, sizeof(ValueInfo));
    size_t l = vtkParse_BasicTypeFromString(arg, &(element->Type), &(element->Class), &n);
    (void)l;
    /* check that type is a string or real or integer */
    if (vtkWrap_IsString(element) || vtkWrap_IsRealNumber(element) || vtkWrap_IsInteger(element))
    {
      isAllowed = 1;
    }
    else if (vtkWrap_IsVTKObjectBaseType(hinfo, element->Class))
    {
      isAllowed = 0;
    }
    else
    {
      isAllowed = 0;
    }
    free(element);
  }
  else
  {
    isAllowed = 0;
  }
  return isAllowed;
}

int vtkWrapSerDes_IsFunctionAllowed(FunctionInfo* functionInfo, const ClassInfo* classInfo,
  const HierarchyInfo* hinfo, const char** rejectReason, int* rejectedParameterId)
{
  *rejectedParameterId = -1;
  /* Ignore static methods */
  if (functionInfo->IsStatic)
  {
    *rejectReason = "static";
    return 0;
  }
  /* Ignore inaccessible methods */
  if (!functionInfo->IsPublic)
  {
    *rejectReason = "not-public";
    return 0;
  }
  /* Ignore template methods */
  if (functionInfo->Template)
  {
    *rejectReason = "templated";
    return 0;
  }
  /* Ignore NewInstance */
  if (!strcmp(functionInfo->Name, "NewInstance"))
  {
    *rejectReason = "NewInstance";
    return 0;
  }
  /* Return value must be allowable */
  if (!vtkWrapSerDes_CanMarshalValue(
        functionInfo->ReturnValue, classInfo, hinfo, /*isReturnValue=*/1))
  {
    *rejectReason = "rejected-return-type";
    return 0;
  }
  /* A returned array is copied out element by element, so its length has to be known. */
  if (functionInfo->ReturnValue != NULL && vtkWrap_IsArray(functionInfo->ReturnValue) &&
    !vtkWrapSerDes_HasTrivialCountExpression(functionInfo, functionInfo->ReturnValue))
  {
    *rejectReason = "unsized-return-array";
    return 0;
  }
  /* Inherited methods and overridden methods are handled by superclasses */
  if (vtkWrap_IsInheritedMethod(classInfo, functionInfo) || functionInfo->IsOverride)
  {
    *rejectReason = "inherited";
    return 0;
  }
  /* Ignore constructors and destructors. */
  if (vtkWrap_IsConstructor(classInfo, functionInfo) ||
    vtkWrap_IsDestructor(classInfo, functionInfo))
  {
    *rejectReason = "constructor-or-destructor";
    return 0;
  }
  /* All parameters must be allowable */
  int parameterId = 0;
  for (parameterId = 0; parameterId < functionInfo->NumberOfParameters; ++parameterId)
  {
    ValueInfo* parameterInfo = functionInfo->Parameters[parameterId];
    if (!vtkWrapSerDes_CanMarshalValue(parameterInfo, classInfo, hinfo, /*isReturnValue=*/0))
    {
      *rejectReason = "rejected-parameter-type";
      *rejectedParameterId = parameterId;
      return 0;
    }
  }
  return 1;
}

static char* vtkWrapSerDes_SmartPointerTypeTemplateArg(const char* name)
{
  const char* defaults[1] = { NULL };
  const char** args;
  char* arg;

  vtkParse_DecomposeTemplatedType(name, NULL, 1, &args, defaults);
  arg = strdup(args[0]);
  vtkParse_FreeTemplateDecomposition(NULL, 1, args);

  return arg;
}

static int vtkWrapSerDes_DecomposeTemplatedTuple(
  const ValueInfo* valInfo, char** elementType, const HierarchyInfo* hinfo)
{
  const HierarchyEntry* entry;
  const char* classname = NULL;
  int tupleSize = 0;

  entry = vtkParseHierarchy_FindEntry(hinfo, valInfo->Class);
  assert(entry != NULL);
  if (entry &&
    vtkParseHierarchy_IsTypeOfTemplated(hinfo, entry, valInfo->Class, "vtkTuple", &classname))
  {
    const char* defaults[2] = { NULL, NULL };
    const char** args;

    vtkParse_DecomposeTemplatedType(classname, NULL, 2, &args, defaults);
    *elementType = strdup(args[0]);
    tupleSize = atoi(args[1]);
    vtkParse_FreeTemplateDecomposition(NULL, 2, args);
  }
  assert(tupleSize > 0);
  assert(*elementType != NULL);
  return tupleSize;
}

static void vtkWrapSerDes_FreeTemplatedTupleDecomposition(char** elementType)
{
  free(*elementType);
}

static void vtkWrapSerDes_WriteArgumentDeserializer(FILE* fp, int paramId, int argId,
  FunctionInfo* functionInfo, ValueInfo* valInfo, const ClassInfo* classInfo,
  const HierarchyInfo* hinfo)
{
  /* An out parameter is not read from the arguments. It is sized here, filled in by the method
   * and returned as the value of the call. */
  if (argId < 0)
  {
    fprintf(fp, "    std::vector<%s> elements_%d(", valInfo->Class, paramId);
    vtkWrapSerDes_WriteCountExpression(fp, functionInfo, valInfo);
    fprintf(fp, ");\n");
    fprintf(fp, "    auto* arg_%d = elements_%d.data();\n", paramId, paramId);
    return;
  }
  /* The argument is the address of memory of the client's. The object borrows it as it stands,
   * so nothing is read out of the json beyond the address itself. */
  if (vtkWrapSerDes_CanMarshalZeroCopyPointer(valInfo))
  {
    fprintf(fp,
      "    // NOLINTNEXTLINE(performance-no-int-to-ptr)\n"
      "    auto* arg_%d = reinterpret_cast<%s*>(args[%d].get<std::uintptr_t>());\n",
      paramId, valInfo->Class, argId);
    return;
  }
  const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, valInfo->Class);
  const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(valInfo);
  const int isPointer = vtkWrap_IsPointer(valInfo);
  const int isScalar = vtkWrap_IsScalar(valInfo);
  const int isNumeric = vtkWrap_IsNumeric(valInfo);
  const int isString = vtkWrap_IsString(valInfo);
  const int isCharPointer = vtkWrap_IsCharPointer(valInfo);
  const int isEnumMember = vtkWrap_IsEnumMember(classInfo, valInfo);
  const int isEnum = valInfo->IsEnum;
  const int isArray = vtkWrap_IsArray(valInfo);
  const int isStdVector = vtkWrap_IsStdVector(valInfo);

  // vtkObject and vtkSmartPointer<T> are handled in the same way
  // vtkSmartPointer<T> is a template class, so we need to get the type of the
  // template argument.
  // vtkObject is a class, so we can use the class name directly.
  char* className = NULL;
  if (isVTKSmartPointer)
  {
    className = vtkWrapSerDes_SmartPointerTypeTemplateArg(valInfo->Class);
  }
  else if (isVTKObject && isPointer)
  {
    className = strdup(valInfo->Class);
  }
  if (isVTKSmartPointer || (isVTKObject && isPointer))
  {
    fprintf(fp,
      "    auto* arg_%d = "
      "reinterpret_cast<%s*>(args[%d].is_null() ? nullptr : objectFromContext%d.GetPointer());\n",
      paramId, className, argId, paramId);
    free(className);
    return;
  }
  if (isNumeric)
  {
    if (isScalar)
    {
      fprintf(fp, "    auto arg_%d = args[%d].get<%s>();\n", paramId, argId, valInfo->Class);
      return;
    }
    if (isArray)
    {
      fprintf(fp, "    auto elements_%d = args[%d].get<std::vector<%s>>();\n", paramId, argId,
        valInfo->Class);
      fprintf(fp, "    auto* arg_%d = elements_%d.data();\n", paramId, paramId);
      return;
    }
    if (isCharPointer)
    {
      fprintf(fp, "    auto elements_%d = args[%d].get<std::string>();\n", paramId, argId);
      fprintf(fp, "    auto* arg_%d = elements_%d.data();\n", paramId, paramId);
      return;
    }
    const int declarationLength = vtkParse_ValueInfoToString(valInfo, NULL, VTK_PARSE_EVERYTHING);
    char* declaration = (char*)malloc((declarationLength + 1) * sizeof(char));
    vtkParse_ValueInfoToString(valInfo, declaration, VTK_PARSE_EVERYTHING);
    fprintf(stderr, "%s:%d Unexpected parameter \'%s\' in %s!\n", __FILE__, __LINE__, declaration,
      __func__);
    free(declaration);
    abort();
  }
  if (isString)
  {
    fprintf(fp, "    auto arg_%d = args[%d].get<std::string>();\n", paramId, argId);
    return;
  }
  if (isEnumMember)
  {
    fprintf(fp,
      "    auto arg_%d = "
      "static_cast<%s::%s>(args[%d].get<std::underlying_type<%s::%s>::type>());\n",
      paramId, classInfo->Name, valInfo->Class, argId, classInfo->Name, valInfo->Class);
    return;
  }
  if (isEnum)
  {
    const char* cp = valInfo->Class;
    size_t l;
    /* search for scope operator */
    for (l = 0; cp[l] != '\0'; l++)
    {
      if (cp[l] == ':')
      {
        break;
      }
    }
    if (cp[l] == ':' && cp[l + 1] == ':')
    {
      fprintf(fp,
        "    auto arg_%d = "
        "static_cast<%*.*s::%s>(args[%d].get<std::underlying_type<%*.*s::%s>::type>());\n",
        paramId, (int)l, (int)l, cp, &cp[l + 2], argId, (int)l, (int)l, cp, &cp[l + 2]);
    }
    else
    {
      fprintf(fp,
        "    auto arg_%d = static_cast<%s>(args[%d].get<std::underlying_type<%s>::type>());\n",
        paramId, cp, argId, cp);
    }
    return;
  }
  if (!strncmp(valInfo->Class, "vtkVector", 9) || !strncmp(valInfo->Class, "vtkTuple", 8) ||
    !strncmp(valInfo->Class, "vtkColor", 8) || !strncmp(valInfo->Class, "vtkRect", 7))
  {
    char* elementType = NULL;
    const int elementCount = vtkWrapSerDes_DecomposeTemplatedTuple(valInfo, &elementType, hinfo);
    fprintf(fp, "    auto elements_%d = args[%d].get<std::array<%s, %d>>();\n", paramId, argId,
      elementType, elementCount);
    fprintf(fp, "    %s arg_%d{elements_%d.data()};\n", valInfo->Class, paramId, paramId);
    vtkWrapSerDes_FreeTemplatedTupleDecomposition(&elementType);
    return;
  }
  if (!strcmp(valInfo->Class, "vtkBoundingBox"))
  {
    fprintf(fp, "    auto elements_%d = args[%d].get<std::array<double, 6>>();\n", paramId, argId);
    fprintf(fp, "    vtkBoundingBox arg_%d{elements_%d.data()};\n", paramId, paramId);
    return;
  }
  if (isStdVector)
  {
    const char* arg = vtkWrap_TemplateArg(valInfo->Class);
    size_t n;
    ValueInfo* element = (ValueInfo*)calloc(1, sizeof(ValueInfo));
    vtkParse_BasicTypeFromString(arg, &(element->Type), &(element->Class), &n);
    /* check that type is a string or real or integer */
    if (vtkWrap_IsString(element) || vtkWrap_IsRealNumber(element) || vtkWrap_IsInteger(element))
    {
      fprintf(
        fp, "    auto arg_%d = args[%d].get<std::vector<%s>>();\n", paramId, argId, element->Class);
      free(element);
      return;
    }
    free(element);
  }
  const int declarationLength = vtkParse_ValueInfoToString(valInfo, NULL, VTK_PARSE_EVERYTHING);
  char* declaration = (char*)malloc((declarationLength + 1) * sizeof(char));
  vtkParse_ValueInfoToString(valInfo, declaration, VTK_PARSE_EVERYTHING);
  fprintf(
    stderr, "%s:%d Unexpected parameter %s in %s!\n", __FILE__, __LINE__, declaration, __func__);
  free(declaration);
  abort();
}

/* -------------------------------------------------------------------- */
static void vtkWrapSerDes_WriteReturnValueSerializer(FILE* fp, const ClassInfo* classInfo,
  const HierarchyInfo* hinfo, FunctionInfo* functionInfo, ValueInfo* valInfo)
{
  const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, valInfo->Class);
  const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(valInfo);
  const int isPointer = vtkWrap_IsPointer(valInfo);
  const int isScalar = vtkWrap_IsScalar(valInfo);
  const int isNumeric = vtkWrap_IsNumeric(valInfo);
  const int isString = vtkWrap_IsString(valInfo);
  const int isCharPointer = vtkWrap_IsCharPointer(valInfo);
  const int isEnumMember = vtkWrap_IsEnumMember(classInfo, valInfo);
  const int isEnum = valInfo->IsEnum;
  const int isArray = vtkWrap_IsArray(valInfo);
  const int isStdVector = vtkWrap_IsStdVector(valInfo);

  if (isVTKObject && isPointer)
  {
    fprintf(fp,
      "    // NOLINTNEXTLINE(readability-redundant-casting)\n"
      "    vtkTypeUInt32 identifier = "
      "context->GetId(reinterpret_cast<vtkObjectBase*>(methodReturnValue));\n");
    fprintf(fp,
      "    if (identifier == 0 && methodReturnValue != nullptr)\n"
      "    {\n"
      "      // NOLINTNEXTLINE(readability-redundant-casting)\n"
      "      context->RegisterObject(reinterpret_cast<vtkObjectBase*>(methodReturnValue), "
      "identifier);\n"
      "    }\n");
    if (vtkWrap_IsNewInstance(valInfo))
    {
      // Manage the new instance in invoker context.
      fprintf(fp,
        "    context->KeepAlive(invoker->GetObjectDescription(), "
        "reinterpret_cast<vtkObjectBase*>(methodReturnValue));\n");
    }
    fprintf(fp, "    result[\"Id\"] = identifier;\n");
    return;
  }
  if (isVTKSmartPointer)
  {
    fprintf(fp,
      "    // NOLINTNEXTLINE(readability-redundant-casting)\n"
      "    vtkTypeUInt32 identifier = "
      "context->GetId(reinterpret_cast<vtkObjectBase*>(methodReturnValue.GetPointer()"
      "));\n");
    fprintf(fp,
      "    if (identifier == 0) { "
      "// NOLINTNEXTLINE(readability-redundant-casting)\n"
      "context->RegisterObject(reinterpret_cast<vtkObjectBase*>(methodReturnValue."
      "GetPointer()), identifier); }");
    if (vtkWrap_IsNewInstance(valInfo))
    {
      // Manage the new instance in invoker context.
      fprintf(fp,
        "    context->KeepAlive(invoker->GetObjectDescription(), "
        "reinterpret_cast<vtkObjectBase*>(methodReturnValue.GetPointer()));\n");
    }
    fprintf(fp, "    result[\"Id\"] = identifier;\n");
    return;
  }
  if (isNumeric)
  {
    /* The memory stays with the object. The client receives its address and reads what is behind
     * it in place, for as long as the object keeps that memory. */
    if (vtkWrapSerDes_CanMarshalZeroCopyPointer(valInfo))
    {
      fprintf(fp, "    result[\"Value\"] = reinterpret_cast<std::uintptr_t>(methodReturnValue);\n");
      return;
    }
    if (isScalar)
    {
      fprintf(fp, "    result[\"Value\"] = methodReturnValue;\n");
      return;
    }
    if (isArray)
    {
      fprintf(fp, "    if(methodReturnValue != nullptr)\n");
      fprintf(fp, "    {\n");
      fprintf(fp, "      auto& dst = result[\"Value\"] = nlohmann::json::array();\n");
      fprintf(fp, "      const auto count = ");
      vtkWrapSerDes_WriteCountExpression(fp, functionInfo, valInfo);
      fprintf(fp, ";\n");
      fprintf(fp,
        "      for (int i = 0; i < static_cast<int>(count); ++i) "
        "{ dst.push_back(methodReturnValue[i]); }\n");
      fprintf(fp, "    }\n");
      return;
    }
    if (isCharPointer)
    {
      fprintf(
        fp, "    if (methodReturnValue != nullptr) { result[\"Value\"] = methodReturnValue; }\n");
      return;
    }
    const int declarationLength = vtkParse_ValueInfoToString(valInfo, NULL, VTK_PARSE_EVERYTHING);
    char* declaration = (char*)malloc((declarationLength + 1) * sizeof(char));
    vtkParse_ValueInfoToString(valInfo, declaration, VTK_PARSE_EVERYTHING);
    fprintf(stderr, "%s:%d Unexpected parameter \'%s\' in %s!\n", __FILE__, __LINE__, declaration,
      __func__);
    free(declaration);
    abort();
  }
  if (isString)
  {
    if (!strcmp(valInfo->Class, "vtkStdString"))
    {
      // workaround error: call to '__is_path_src' is ambiguous on el8.
      fprintf(fp, "    result[\"Value\"] = std::string(methodReturnValue);\n");
    }
    else
    {
      fprintf(fp, "    result[\"Value\"] = methodReturnValue;\n");
    }
    return;
  }
  if (isEnumMember)
  {
    fprintf(fp,
      "    result[\"Value\"] = "
      "static_cast<std::underlying_type<%s::%s>::type>(methodReturnValue);\n",
      classInfo->Name, valInfo->Class);
    return;
  }
  if (isEnum)
  {
    const char* cp = valInfo->Class;
    size_t l;
    /* search for scope operator */
    for (l = 0; cp[l] != '\0'; l++)
    {
      if (cp[l] == ':')
      {
        break;
      }
    }
    if (cp[l] == ':' && cp[l + 1] == ':')
    {
      fprintf(fp,
        "    // NOLINTNEXTLINE(readability-redundant-casting)\n"
        "    result[\"Value\"] = static_cast<%*.*s::%s>(methodReturnValue);\n",
        (int)l, (int)l, cp, &cp[l + 2]);
    }
    else
    {
      fprintf(fp,
        "    // NOLINTNEXTLINE(readability-redundant-casting)\n"
        "    result[\"Value\"] = static_cast<%s>(methodReturnValue);\n",
        cp);
    }
    return;
  }
  if (!strncmp(valInfo->Class, "vtkVector", 9) || !strncmp(valInfo->Class, "vtkTuple", 8) ||
    !strncmp(valInfo->Class, "vtkColor", 8) || !strncmp(valInfo->Class, "vtkRect", 7))
  {
    fprintf(fp, "    {\n");
    fprintf(fp, "      auto& dst = result[\"Value\"] = nlohmann::json::array();\n");
    fprintf(fp,
      "      for (int i = 0; i < methodReturnValue.GetSize(); ++i) { "
      "dst.push_back(methodReturnValue[i]); }\n");
    fprintf(fp, "    }\n");
    return;
  }
  if (!strcmp(valInfo->Class, "vtkBoundingBox"))
  {
    fprintf(fp, "    {\n");
    fprintf(fp, "      auto& dst = result[\"Value\"] = nlohmann::json::array();\n");
    fprintf(fp, "      double bounds[6] = {};\n");
    fprintf(fp, "      methodReturnValue.GetBounds(bounds);\n");
    fprintf(fp, "      for (int i = 0; i < 6; ++i) { dst.push_back(bounds[i]); }\n");
    fprintf(fp, "    }\n");
    return;
  }
  if (isStdVector)
  {
    const char* arg = vtkWrap_TemplateArg(valInfo->Class);
    size_t n;
    ValueInfo* element = (ValueInfo*)calloc(1, sizeof(ValueInfo));
    size_t l = vtkParse_BasicTypeFromString(arg, &(element->Type), &(element->Class), &n);
    (void)l;
    /* check that type is a string or real or integer */
    if (vtkWrap_IsString(element) || vtkWrap_IsRealNumber(element) || vtkWrap_IsInteger(element))
    {
      if (isPointer)
      {
        fprintf(fp, "    result[\"Value\"] = *methodReturnValue;\n");
      }
      else
      {
        fprintf(fp, "    result[\"Value\"] = methodReturnValue;\n");
      }
      free(element);
      return;
    }
    free(element);
  }
  const int declarationLength = vtkParse_ValueInfoToString(valInfo, NULL, VTK_PARSE_EVERYTHING);
  char* declaration = (char*)malloc((declarationLength + 1) * sizeof(char));
  vtkParse_ValueInfoToString(valInfo, declaration, VTK_PARSE_EVERYTHING);
  fprintf(
    stderr, "%s:%d Unexpected parameter %s in %s!\n", __FILE__, __LINE__, declaration, __func__);
  free(declaration);
  abort();
}

static void vtkWrapSerDes_WriteArgumentCheck(FILE* fp, FunctionInfo* functionInfo,
  const ClassInfo* classInfo, const HierarchyInfo* hinfo, int outParameterId)
{
  int i = 0;
  int argId = 0; // incremented per item used from the input array 'args'.
  const int numberOfArguments = functionInfo->NumberOfParameters - (outParameterId >= 0 ? 1 : 0);
  for (i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    ValueInfo* valInfo = functionInfo->Parameters[i];
    const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, valInfo->Class);
    const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(valInfo);
    const int isPointer = vtkWrap_IsPointer(valInfo);
    if (isVTKSmartPointer || (isVTKObject && isPointer))
    {
      fprintf(fp, "  nlohmann::json::const_iterator idIter%d;\n", i);
      fprintf(fp, "  vtkSmartPointer<vtkObjectBase> objectFromContext%d;\n", i);
    }
  }
  if (numberOfArguments == 0)
  {
    fprintf(fp, "  if (args.empty())\n");
  }
  else
  {
    fprintf(fp, "  if (args.size() == %d", numberOfArguments);
  }
  for (i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    /* the out parameter does not come from the arguments, so there is nothing to check. */
    if (i == outParameterId)
    {
      continue;
    }
    ValueInfo* valInfo = functionInfo->Parameters[i];
    const int argIndex = argId++;
    const int isVTKObject = vtkWrap_IsVTKObjectBaseType(hinfo, valInfo->Class);
    const int isVTKSmartPointer = vtkWrap_IsVTKSmartPointer(valInfo);
    const int isPointer = vtkWrap_IsPointer(valInfo);
    const int isScalar = vtkWrap_IsScalar(valInfo);
    const int isNumeric = vtkWrap_IsNumeric(valInfo);
    const int isString = vtkWrap_IsString(valInfo);
    const int isCharPointer = vtkWrap_IsCharPointer(valInfo);
    const int isEnumMember = vtkWrap_IsEnumMember(classInfo, valInfo);
    const int isEnum = valInfo->IsEnum;
    const int isArray = vtkWrap_IsArray(valInfo);
    const int isStdVector = vtkWrap_IsStdVector(valInfo);

    // vtkObject and vtkSmartPointer<T> are handled in the same way
    // vtkSmartPointer<T> is a template class, so we need to get the type of the
    // template argument.
    // vtkObject is a class, so we can use the class name directly.
    char* className = NULL;
    if (isVTKSmartPointer)
    {
      className = vtkWrapSerDes_SmartPointerTypeTemplateArg(valInfo->Class);
    }
    else if (isVTKObject && isPointer)
    {
      className = strdup(valInfo->Class);
    }
    if (isVTKSmartPointer || (isVTKObject && isPointer))
    {
      // Verify that the parameter is a JSON object and contains the "Id" key
      // This is needed for vtkSmartPointer and vtkObjectBase types.
      fprintf(fp,
        "\n   && ((args[%d].is_object()"
        "\n   && (static_cast<void>(idIter%d = args[%d].find(\"Id\")), idIter%d != "
        "args[%d].end())" // uses the comma to initialize idIter inline. This cannot be done outside
                          // because args[i] might throw out of range error. The first expression is
                          // casted to void in order to silence the `-Wcomma` warning
        "\n   && idIter%d->is_number_unsigned()"
        "\n   && (objectFromContext%d = context->GetObjectAtId(*idIter%d))"
        "\n   && context->GetObjectAtId(*idIter%d)->IsA(\"%s\"))"
        "\n   || args[%d].is_null())",
        argIndex, i, argIndex, i, argIndex, i, i, i, i, className, argIndex);
      free(className);
    }
    else if (isNumeric)
    {
      /* The client hands over the address of its own memory for the object to borrow. Zero is
       * never the address of such a buffer, so it is rejected along with the non numeric
       * arguments. */
      if (vtkWrapSerDes_CanMarshalZeroCopyPointer(valInfo))
      {
        fprintf(fp,
          "\n   && args[%d].is_number_unsigned() && (args[%d].get<std::uintptr_t>() != 0)",
          argIndex, argIndex);
      }
      else if (isScalar)
      {
        if (vtkWrap_IsBool(valInfo))
        {
          fprintf(fp, "\n   && args[%d].is_boolean()", argIndex);
        }
        else if (vtkWrap_IsInteger(valInfo))
        {
          fprintf(fp, "\n   && args[%d].is_number_integer()", argIndex);
        }
        else
        {
          fprintf(fp, "\n   && args[%d].is_number()", argIndex);
        }
      }
      else if (isArray)
      {
        fprintf(fp, "\n   && args[%d].is_array()", argIndex);
        /* the method reads a fixed number of elements out of the array, so a shorter one would
         * make it read past the end. */
        if (vtkWrapSerDes_HasTrivialCountExpression(functionInfo, valInfo))
        {
          fprintf(fp, "\n   && (args[%d].size() == static_cast<std::size_t>(", argIndex);
          vtkWrapSerDes_WriteCountExpression(fp, functionInfo, valInfo);
          fprintf(fp, "))");
        }
      }
      else if (isCharPointer)
      {
        fprintf(fp, "\n   && args[%d].is_string()", argIndex);
      }
    }
    else if (isString)
    {
      fprintf(fp, "\n   && args[%d].is_string()", argIndex);
    }
    else if (isEnumMember)
    {
      fprintf(fp, "\n   && args[%d].is_number_integer()", argIndex);
    }
    else if (isEnum)
    {
      fprintf(fp, "\n   && args[%d].is_number_integer()", argIndex);
    }
    else if (!strncmp(valInfo->Class, "vtkVector", 9) || !strncmp(valInfo->Class, "vtkTuple", 8) ||
      !strncmp(valInfo->Class, "vtkColor", 8) || !strncmp(valInfo->Class, "vtkRect", 7))
    {
      char* elementType = NULL;
      const int elementCount = vtkWrapSerDes_DecomposeTemplatedTuple(valInfo, &elementType, hinfo);
      fprintf(fp, "\n   && args[%d].is_array() && (args[%d].size() == %d)", argIndex, argIndex,
        elementCount);
      vtkWrapSerDes_FreeTemplatedTupleDecomposition(&elementType);
    }
    else if (!strcmp(valInfo->Class, "vtkBoundingBox"))
    {
      fprintf(fp, "\n   && args[%d].is_array() && (args[%d].size() == 6)", argIndex, argIndex);
    }
    else if (isStdVector)
    {
      fprintf(fp, "\n   && args[%d].is_array()", argIndex);
    }
  }
  if (numberOfArguments > 0)
  {
    fprintf(fp, "\n     )\n");
  }
}

static int vtkWrapSerDes_WriteMemberFunctionCall(
  FILE* fp, const ClassInfo* classInfo, FunctionInfo* functionInfo, const HierarchyInfo* hinfo)
{
  int i = 0;
  int argId = 0;
  const int outParameterId = vtkWrapSerDes_FindOutParameterPosition(functionInfo);
  fprintf(fp, "  {\n"); // some arguments need locals, so scope them.
  vtkWrapSerDes_WriteArgumentCheck(fp, functionInfo, classInfo, hinfo, outParameterId);
  fprintf(fp, "  {\n");
  for (i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    // Retrieve individual arguments to stack and call method on object with arguments.
    // Then pack the result in json and return the json object. Handle errors by storing message
    // in the result json and set Success = false
    // Return json: {"Value" (or) "Id": value, "Success": false/true, "Message": "Failed to parse
    // args ... etc."}

    ValueInfo* paramInfo = functionInfo->Parameters[i];
    vtkWrapSerDes_WriteArgumentDeserializer(
      fp, i, (i == outParameterId ? -1 : argId++), functionInfo, paramInfo, classInfo, hinfo);
  }
  const char* argStart = "";
  const char* argEnd = "";
  if (functionInfo->NumberOfParameters > 0)
  {
    argStart = "\n";
    argEnd = "    ";
  }
  /* the arguments are valid to use with the method only when its preconditions are met. */
  if (functionInfo->NumberOfPreconds > 0)
  {
    vtkWrapSerDes_WritePreconditionCheck(fp, classInfo, functionInfo);
  }
  fprintf(fp,
    "    vtkVLog(invoker->GetInvokerLogVerbosity(), \"Calling %s::%s with args\" << "
    "args.dump());\n",
    classInfo->Name, functionInfo->Name);
  if (vtkWrap_IsVoid(functionInfo->ReturnValue))
  {
    fprintf(fp, "    object->%s(%s", functionInfo->Name, argStart);
    for (i = 0; i < functionInfo->NumberOfParameters; ++i)
    {
      ValueInfo* paramInfo = functionInfo->Parameters[i];
      const char* parameterName = paramInfo->Name ? paramInfo->Name : "noname";
      fprintf(fp, "      /*%s=*/arg_%d", parameterName, i);
      if (functionInfo->NumberOfParameters > 1 && i != functionInfo->NumberOfParameters - 1)
      {
        fprintf(fp, ",");
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "%s);\n", argEnd);
    /* the out parameter holds what the method produced, so it is the value of the call. */
    if (outParameterId >= 0)
    {
      fprintf(fp, "    {\n");
      fprintf(fp, "      auto& dst = result[\"Value\"] = nlohmann::json::array();\n");
      fprintf(fp, "      for (const auto& element : elements_%d) { dst.push_back(element); }\n",
        outParameterId);
      fprintf(fp, "    }\n");
    }
  }
  else
  {
    fprintf(fp,
      "    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)\n"
      "    auto%s methodReturnValue = object->%s(%s",
      (vtkWrap_IsPointer(functionInfo->ReturnValue) ? "*" : ""), functionInfo->Name, argStart);
    for (i = 0; i < functionInfo->NumberOfParameters; ++i)
    {
      ValueInfo* paramInfo = functionInfo->Parameters[i];
      const char* parameterName = paramInfo->Name ? paramInfo->Name : "noname";
      fprintf(fp, "      /*%s=*/arg_%d", parameterName, i);
      if (functionInfo->NumberOfParameters > 1 && i != functionInfo->NumberOfParameters - 1)
      {
        fprintf(fp, ",\n");
      }
    }
    fprintf(fp, "%s);\n", argEnd);
    vtkWrapSerDes_WriteReturnValueSerializer(
      fp, classInfo, hinfo, functionInfo, functionInfo->ReturnValue);
  }
  fprintf(fp,
    "    result[\"Message\"] = std::string(\"Call to \") + object->GetClassName() + "
    "std::string(\"::\") + \"%s\" + std::string(\" is successful.\");\n",
    functionInfo->Name);
  fprintf(fp, "    result[\"Success\"] = true;\n");
  /* an out parameter takes the place of an argument, so two overloads can now read the same
   * number of arguments. Stop at the one that ran instead of calling the other one too. */
  fprintf(fp, "    return;\n");
  if (functionInfo->NumberOfPreconds > 0)
  {
    fprintf(fp, "    }\n");
    fprintf(fp, "    else\n    {\n");
    fprintf(fp,
      "      result[\"Message\"] = std::string(\"Call to \") + object->GetClassName() + "
      "std::string(\"::\") + \"%s\" + std::string(\" expects \") + \"",
      functionInfo->Name);
    for (i = 0; i < functionInfo->NumberOfPreconds; ++i)
    {
      fprintf(fp, "%s%s", (i == 0 ? "" : " and "),
        vtkWrapText_QuoteString(functionInfo->Preconds[i], 200));
    }
    fprintf(fp, "\" + std::string(\", got \") + args.dump();\n");
    fprintf(fp, "    }\n");
  }
  fprintf(fp, "  }\n"); // end if arguments check
  fprintf(fp, "  }\n");
  return 1;
}

void vtkWrapSerDes_DefineFunctions(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo)
{
  // Ignore the invoker for vtkObjectBase, as its methods are sensitive to object lifetime.
  if (!strcmp(classInfo->Name, "vtkObjectBase"))
  {
    return;
  }
  int functionId = 0, functionId2 = 0;
  FunctionInfo *theFunc = NULL, *overloadedFunc = NULL;
  int* generatedFunctionCalls = (int*)calloc(classInfo->NumberOfFunctions, sizeof(int));
  /* Loop through all functions in the class */
  for (functionId = 0; functionId < classInfo->NumberOfFunctions; ++functionId)
  {
    theFunc = classInfo->Functions[functionId];
    fprintf(fp, "//Method:\'%s\'\n", theFunc->Signature);
    const char* rejectReason = NULL;
    int rejectedParameterId = -1;
    /* Skip unallowable function */
    if (!vtkWrapSerDes_IsFunctionAllowed(
          theFunc, classInfo, hinfo, &rejectReason, &rejectedParameterId))
    {
      fprintf(fp, "//- not allowed: %s", rejectReason);
      if (rejectedParameterId >= 0)
      {
        fprintf(fp, ", parameter at index:%d cannot be marshalled.", rejectedParameterId);
      }
      fprintf(fp, "\n");
      continue;
    }
    /* Skip function whose calling code was already generated */
    if (generatedFunctionCalls[functionId])
    {
      fprintf(fp, "//Overload already handled\n");
      continue;
    }
    fprintf(fp,
      "static void Invoke_%s_%s(vtkInvoker* invoker, "
      "%s* object, const nlohmann::json& args, nlohmann::json& result)\n"
      "{\n",
      classInfo->Name, theFunc->Name, classInfo->Name);
    fprintf(fp, "  vtkVLogScopeFunction(invoker->GetInvokerLogVerbosity());\n");
    fprintf(fp,
      "  result[\"Message\"] = std::string(\"No suitable overload of "
      "\'%s::%s\' takes the specified arguments.\") + args.dump();\n",
      classInfo->Name, theFunc->Name);
    fprintf(fp, "  result[\"Success\"] = false;\n");
    fprintf(fp, "  auto context = invoker->GetContext();\n");
    fprintf(fp, "  (void)context;\n");
    fprintf(fp, "  (void)object;\n");
    fprintf(fp, "  (void)args;\n");
    generatedFunctionCalls[functionId] =
      vtkWrapSerDes_WriteMemberFunctionCall(fp, classInfo, theFunc, hinfo);
    for (functionId2 = 0; functionId2 < classInfo->NumberOfFunctions; ++functionId2)
    {
      /* Skip function same as outer function */
      if (functionId == functionId2)
      {
        continue;
      }
      /* Skip different function */
      if (strcmp(theFunc->Name, classInfo->Functions[functionId2]->Name) != 0)
      {
        continue;
      }
      // found an overload.
      overloadedFunc = classInfo->Functions[functionId2];
      fprintf(fp, "//Overload:\'%s\'\n", overloadedFunc->Signature);
      /* Skip unallowable function */
      if (!vtkWrapSerDes_IsFunctionAllowed(
            overloadedFunc, classInfo, hinfo, &rejectReason, &rejectedParameterId))
      {
        continue;
      }
      generatedFunctionCalls[functionId2] =
        vtkWrapSerDes_WriteMemberFunctionCall(fp, classInfo, overloadedFunc, hinfo);
    }
    fprintf(fp, "}\n");
  }
  free(generatedFunctionCalls);
}

void vtkWrapSerDes_CallFunctions(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo)
{
  // Ignore the invoker for vtkObjectBase, as its methods are sensitive to object lifetime.
  if (!strcmp(classInfo->Name, "vtkObjectBase"))
  {
    fprintf(fp,
      "  result[\"Message\"] = std::string(\"Call to %s\") + "
      "std::string(\"::\") + methodName + "
      "std::string(\" is "
      "not permitted.\");\n",
      classInfo->Name);
    return;
  }
  // try the superclass
  fprintf(fp,
    "  if (auto f = invoker->GetHandler(typeid(%s::Superclass)))\n"
    "  {\n"
    "    result = f(invoker, objectBase, methodName, args);\n"
    "  }\n",
    classInfo->Name);
  fprintf(fp, "  if (result[\"Success\"]) { return result; }\n");
  // if the superclass handler did not return a successful result, we need to
  // handle the method ourselves.
  int functionId = 0, functionId2 = 0;
  FunctionInfo* theFunc = NULL;
  int* generatedFunctionCalls = (int*)calloc(classInfo->NumberOfFunctions, sizeof(int));
  int generateDefaultBlock = 0;
  int switchCaseStarted = 0;
  /* Loop through all functions in the class */
  for (functionId = 0; functionId < classInfo->NumberOfFunctions; ++functionId)
  {
    theFunc = classInfo->Functions[functionId];
    const char* rejectReason = NULL;
    int rejectedParameterId = -1;
    /* Skip unallowable function */
    if (!vtkWrapSerDes_IsFunctionAllowed(
          theFunc, classInfo, hinfo, &rejectReason, &rejectedParameterId))
    {
      continue;
    }
    /* Skip function whose calling code was already generated */
    if (generatedFunctionCalls[functionId])
    {
      continue;
    }

    if (!switchCaseStarted)
    {
      fprintf(fp, "  using namespace vtk::literals;\n");
      fprintf(fp, "  const vtkStringToken methodToken(methodName);\n");
      fprintf(fp, "  switch(methodToken.GetId())\n");
      fprintf(fp, "  {\n");
      switchCaseStarted = 1;
      generateDefaultBlock = 1;
    }
    fprintf(fp, "    case \"%s\"_hash:\n", theFunc->Name);
    fprintf(
      fp, "      Invoke_%s_%s(invoker, object, args, result);\n", classInfo->Name, theFunc->Name);
    fprintf(fp, "      break;\n");
    generatedFunctionCalls[functionId] = 1;
    // mark all overloads as generated
    for (functionId2 = 0; functionId2 < classInfo->NumberOfFunctions; ++functionId2)
    {
      /* Skip function same as outer function */
      if (functionId == functionId2)
      {
        continue;
      }
      if (!strcmp(theFunc->Name, classInfo->Functions[functionId2]->Name))
      {
        generatedFunctionCalls[functionId2] = 1;
      }
    }
  }
  free(generatedFunctionCalls);
  if (generateDefaultBlock)
  {
    fprintf(fp,
      "    default:\n"
      "    {\n"
      "      if (result[\"Message\"].get<std::string>().empty())\n"
      "      {\n"
      "        result[\"Message\"] = std::string(\"No such method exists %s::\") + "
      "(methodName ? std::string(methodName) : \"null\");\n"
      "      }\n"
      "      result[\"Success\"] = false;\n"
      "      break;\n"
      "    }\n"
      "  }\n",
      classInfo->Name);
  }
  else
  {
    fprintf(fp, "  (void)object;\n");
  }
}

// NOLINTEND(bugprone-unsafe-functions)
