/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJavaScriptMethod.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkParseData.h"
#include "vtkParseExtras.h"
#include "vtkParseHierarchy.h"
#include "vtkParseType.h"
#include "vtkWrap.h"
#include "vtkWrapJavaScriptClass.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(bugprone-unsafe-functions)
// NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)

#ifdef NDEBUG
#define DLOG(...)
#else
#define DLOG(...) printf(__VA_ARGS__);
#endif

/* -------------------------------------------------------------------- */
/* prototypes for utility methods */

/* convenient function for const char* tests */
static int vtkWrapJavaScript_IsConstCharPointer(ValueInfo* val);

/* print out any custom methods */
static void vtkWrapJavaScript_CustomMethods(const char* classname, ClassInfo* data);

/* modify vtkObjectBase methods as needed for javascript */
static void vtkWrapJavaScript_HideObjectBaseMethods(const char* classname, ClassInfo* data);

/* check for wrappability, flags may be VTK_WRAP_ARG or VTK_WRAP_RETURN */
static int vtkWrapJavaScript_IsValueWrappable(
  ClassInfo* data, ValueInfo* val, HierarchyInfo* hinfo, int flags);

/* whether a std::string overload needs to be created instead of a function with const char*
 * parameters or return types */
static int vtkWrapJavaScript_NeedsCppStringOverload(FunctionInfo* functionInfo);

/* write the return value type for the emscripten::select_overload statement */
static void vtkWrapJavaScript_WriteOverloadReturnType(FILE* fp, FunctionInfo* functionInfo);

/* Write function signature. this method will enumerate arguments like arg_0, arg_1 if enumerateArgs
 * == 1*/
static void vtkWrapJavaScript_WriteSignature(FILE* fp, FunctionInfo* functionInfo,
  const char* argBaseName, int enumerateArgs, int withSelfParameter,
  int treatPointersAsIntegerAddresses);

/* Writes a return statement which calls a class member function with enumerated arguments like
 * return self.Func(arg_0, arg_1) */
static void vtkWrapJavaScript_WriteMemberFunctionCall(FILE* fp, FunctionInfo* functionInfo);

/* Whether the function parameters could accept memory addresses */
static int vtkWrapJavaScript_AcceptsMemoryAddress(FunctionInfo* functionInfo);

/* Whether embind should be explcitly told to allow raw pointers to pass through */
static int vtkWrapJavaScript_NeedsToAllowRawPointers(FunctionInfo* functionInfo);

/* Whether any function parameters has raw pointer arguments if the return type is a raw pointer */
static int vtkWrapJavaScript_NeedsToReturnMemoryView(FunctionInfo* functionInfo);

/* Whether a memory address must be returned */
static int vtkWrapJavaScript_NeedsToReturnMemoryAddress(FunctionInfo* functionInfo);

/* Whether arguments have non-const lvalue refs */
static int vtkWrapJavaScript_HasNonConstRefParameters(FunctionInfo* functionInfo);

/* Whether arguments have non-const lvalue refs */
static int vtkWrapJavaScript_HasFunctionPointerParameters(FunctionInfo* functionInfo);

/* -------------------------------------------------------------------- */
/* Convenient method tests whether a value is a const char* */
static int vtkWrapJavaScript_IsConstCharPointer(ValueInfo* val)
{
  return vtkWrap_IsConst(val) && vtkWrap_IsCharPointer(val);
}

/* -------------------------------------------------------------------- */
/* generate code for custom methods for some classes */
/* classname is the class name, for if data->Name is a templated id */
static void vtkWrapJavaScript_CustomMethods(const char* classname, ClassInfo* data)
{
  /* Remove methods which do not make sense to have in javascript environment */
  vtkWrapJavaScript_HideObjectBaseMethods(classname, data);
}

/* -------------------------------------------------------------------- */
/* modify vtkObjectBase methods as needed for javascript */
static void vtkWrapJavaScript_HideObjectBaseMethods(const char* classname, ClassInfo* data)
{
  int i;
  FunctionInfo* theFunc;

  /* the python vtkObjectBase needs a couple extra functions */
  if (strcmp("vtkObjectBase", classname) == 0)
  {
    /* remove the original methods, if they exist */
    for (i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];

      if ((strcmp(theFunc->Name, "Register") == 0) || (strcmp(theFunc->Name, "UnRegister") == 0) ||
        (strcmp(theFunc->Name, "FastDelete") == 0) ||
        (strcmp(theFunc->Name, "SetMemkindDirectory") == 0) ||
        (strcmp(theFunc->Name, "GetUsingMemkind") == 0) ||
        (strcmp(theFunc->Name, "GetIsInMemkind") == 0))
      {
        theFunc->Name = NULL;
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* Check an arg to see if it is wrappable */
static int vtkWrapJavaScript_IsValueWrappable(
  ClassInfo* data, ValueInfo* val, HierarchyInfo* hinfo, int flags)
{
  static const unsigned int wrappableTypes[] = { VTK_PARSE_VOID, VTK_PARSE_BOOL, VTK_PARSE_FLOAT,
    VTK_PARSE_DOUBLE, VTK_PARSE_CHAR, VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR, VTK_PARSE_INT,
    VTK_PARSE_UNSIGNED_INT, VTK_PARSE_SHORT, VTK_PARSE_UNSIGNED_SHORT, VTK_PARSE_LONG,
    VTK_PARSE_UNSIGNED_LONG, VTK_PARSE_SSIZE_T, VTK_PARSE_SIZE_T, VTK_PARSE_UNKNOWN,
    VTK_PARSE_LONG_LONG, VTK_PARSE_UNSIGNED_LONG_LONG, VTK_PARSE_OBJECT, VTK_PARSE_QOBJECT,
    VTK_PARSE_STRING, VTK_PARSE_FUNCTION, 0 };

  const char* aClass;
  unsigned int baseType;
  int j;

  if ((flags & VTK_WRAP_RETURN) != 0)
  {
    if (vtkWrap_IsVoid(val))
    {
      return 1;
    }

    // Return type is double* with size hint
    if ((val->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_DOUBLE_PTR && val->Count > 0)
    {
      return 1;
    }

    // Return type is int* with size hint
    if ((val->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_INT_PTR && val->Count > 0)
    {
      return 1;
    }

    if (vtkWrap_IsNArray(val))
    {
      return 0;
    }
  }

  /* wrap std::vector<T> (IsScalar means "not pointer or array") */
  if (vtkWrap_IsStdVector(val) && vtkWrap_IsScalar(val))
  {
    int wrappable = 0;
    char* arg = vtkWrap_TemplateArg(val->Class);
    size_t n;
    size_t l = vtkParse_BasicTypeFromString(arg, &baseType, &aClass, &n);

    /* check that type has no following '*' or '[]' decorators */
    if (arg[l] == '\0')
    {
      if (baseType != VTK_PARSE_UNKNOWN && baseType != VTK_PARSE_OBJECT &&
        baseType != VTK_PARSE_QOBJECT && baseType != VTK_PARSE_CHAR)
      {
        for (j = 0; wrappableTypes[j] != 0; j++)
        {
          if (baseType == wrappableTypes[j])
          {
            wrappable = 1;
            break;
          }
        }
      }
      else if (strncmp(arg, "vtkSmartPointer<", 16) == 0)
      {
        if (arg[strlen(arg) - 1] == '>')
        {
          wrappable = 1;
        }
      }
    }

    free(arg);
    return wrappable;
  }

  aClass = val->Class;
  baseType = (val->Type & VTK_PARSE_BASE_TYPE);

  /* go through all types that are indicated as wrappable */
  for (j = 0; wrappableTypes[j] != 0; j++)
  {
    if (baseType == wrappableTypes[j])
    {
      break;
    }
  }
  if (wrappableTypes[j] == 0)
  {
    return 0;
  }

  if (vtkWrap_IsRef(val) && !vtkWrap_IsScalar(val) && !vtkWrap_IsArray(val) &&
    !vtkWrap_IsPODPointer(val))
  {
    return 0;
  }

  if (vtkWrap_IsScalar(val))
  {
    if (vtkWrap_IsNonConstRef(val))
    {
      return 0;
    }
    if (vtkWrap_IsNumeric(val) || vtkWrap_IsEnumMember(data, val) || vtkWrap_IsString(val))
    {
      return 1;
    }
    /* enum types were marked in vtkWrapJavaScript_MarkAllEnums() */
    if (val->IsEnum)
    {
      return 1;
    }
    if (vtkWrap_IsVTKSmartPointer(val))
    {
      return 1;
    }
    else if (vtkWrap_IsObject(val) && vtkWrap_IsClassWrapped(hinfo, aClass))
    {
      return 1;
    }
  }
  else if (vtkWrap_IsArray(val) || vtkWrap_IsNArray(val))
  {
    if (vtkWrap_IsNonConstRef(val))
    {
      return 0;
    }
    if (vtkWrap_IsNumeric(val))
    {
      return 0;
    }
  }
  else if (vtkWrap_IsPointer(val))
  {
    if (vtkWrap_IsNonConstRef(val))
    {
      return 0;
    }
    if (vtkWrap_IsCharPointer(val) || vtkWrap_IsVoidPointer(val) ||
      vtkWrap_IsZeroCopyPointer(val) || vtkWrap_IsPODPointer(val))
    {
      return 1;
    }
    else if (vtkWrap_IsObject(val))
    {
      if (vtkWrap_IsVTKObjectBaseType(hinfo, aClass))
      {
        return 1;
      }
    }
    else if (vtkWrap_IsFunction(val))
    {
      int i;
      int n = vtkWrap_CountWrappedParameters(val->Function);

      /* check to see if we can handle all the args */
      for (i = 0; i < n; i++)
      {
        if (!vtkWrapJavaScript_IsValueWrappable(
              data, val->Function->Parameters[i], hinfo, VTK_WRAP_ARG))
        {
          return 0;
        }
      }

      /* check the return value */
      if (!vtkWrapJavaScript_IsValueWrappable(
            data, val->Function->ReturnValue, hinfo, VTK_WRAP_RETURN))
      {
        return 0;
      }
      return 1;
    }
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* whether an overload needs to be selected */
static int vtkWrapJavaScript_NeedsCppStringOverload(FunctionInfo* functionInfo)
{
  int hasCharPtrParameter = 0;
  for (int i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    ValueInfo* parameter = functionInfo->Parameters[i];
    hasCharPtrParameter |= (vtkWrapJavaScript_IsConstCharPointer(parameter));
  }
  // Do not check only for const char* return type. Some VTK code returns char* strings!
  int returnsCharPtr = vtkWrap_IsCharPointer(functionInfo->ReturnValue);
  if (!(returnsCharPtr || hasCharPtrParameter))
  {
    return 0;
  }
  return 1;
}

/* -------------------------------------------------------------------- */
/* find the return value type to use in the select_overload statement */
static void vtkWrapJavaScript_WriteOverloadReturnType(FILE* fp, FunctionInfo* functionInfo)
{
  // Do not check only for const char* return type. Some VTK code returns char* strings!
  if (vtkWrap_IsCharPointer(functionInfo->ReturnValue))
  {
    fprintf(fp, "std::string");
  }
  else if (vtkWrapJavaScript_NeedsToReturnMemoryView(functionInfo))
  {
    fprintf(fp, "emscripten::val");
  }
  else if (vtkWrapJavaScript_NeedsToReturnMemoryAddress(functionInfo))
  {
    fprintf(fp, "std::uintptr_t");
  }
  else
  {
    fprintf(fp, "%s", functionInfo->ReturnValue->Class);
    if (vtkWrap_IsPointer(functionInfo->ReturnValue))
    {
      fprintf(fp, "*");
    }
  }
}

/* -------------------------------------------------------------------- */
/* Write function signature. this method will enumerate arguments like arg_0, arg_1 if enumerateArgs
 * == 1*/
static void vtkWrapJavaScript_WriteSignature(FILE* fp, FunctionInfo* functionInfo,
  const char* argBaseName, int enumerateArgs, int withSelfParameter,
  int treatPointersAsIntegerAddresses)
{
  fprintf(fp, "(");
  if (!functionInfo->IsStatic && withSelfParameter)
  {
    if (enumerateArgs)
    {
      fprintf(fp, "%s& self", functionInfo->Class);
    }
    else
    {
      fprintf(fp, "%s&", functionInfo->Class);
    }
    if (functionInfo->NumberOfParameters)
    {
      fprintf(fp, ",");
    }
  }
  for (int i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    ValueInfo* parameter = functionInfo->Parameters[i];
    const char* tpname = "";
    const char* prefix = "";
    const char* ref_suffix = "";
    const char* ptr_suffix = "";
    if (vtkWrapJavaScript_IsConstCharPointer(parameter))
    {
      prefix = "const ";
      tpname = "std::string";
      ref_suffix = " &";
    }
    else
    {
      if (vtkWrap_IsConstRef(parameter))
      {
        prefix = "const ";
        ref_suffix = "&";
      }
      else if (vtkWrap_IsRef(parameter))
      {
        ref_suffix = "&";
      }
      if (vtkWrap_IsPointer(parameter))
      {
        ptr_suffix = "*";
      }
      // all kinds of pointers are treated as integer addresses into wasm memory which is literally
      // a javascript Uint8Array
      if (vtkWrap_IsVoidPointer(parameter) && treatPointersAsIntegerAddresses)
      {
        tpname = "std::uintptr_t";
        ptr_suffix = "";
      }
      else if (vtkWrap_IsPointer(parameter) && vtkWrap_IsNumeric(parameter) &&
        treatPointersAsIntegerAddresses)
      {
        tpname = "std::uintptr_t";
        ptr_suffix = "";
      }
      else if (vtkWrap_IsFunction(parameter))
      {
        tpname = "emscripten::val";
        ptr_suffix = "";
      }
      else if (vtkWrap_IsArray(parameter) && treatPointersAsIntegerAddresses)
      {
        tpname = "std::uintptr_t";
        ptr_suffix = "";
      }
      else if (vtkWrap_IsNArray(parameter) && treatPointersAsIntegerAddresses)
      {
        tpname = "std::uintptr_t";
        ptr_suffix = "";
      }
      else
      {
        tpname = parameter->Class;
      }
    }
    if (enumerateArgs)
    {
      fprintf(fp, " %s%s%s%s %s_%d", prefix, tpname, ptr_suffix, ref_suffix, argBaseName, i);
    }
    else
    {
      fprintf(fp, " %s%s%s%s", prefix, tpname, ptr_suffix, ref_suffix);
    }
    if (i < functionInfo->NumberOfParameters - 1)
    {
      fprintf(fp, ",");
    }
  }
  fprintf(fp, ")");
}

/* -------------------------------------------------------------------- */
/* Writes a return statement which calls a class member function with enumerated arguments like
 * return self.Func(arg_0, arg_1) */
static void vtkWrapJavaScript_WriteMemberFunctionCall(FILE* fp, FunctionInfo* functionInfo)
{
  if (functionInfo->IsStatic)
  {
    fprintf(fp, "%s::%s(", functionInfo->Class, functionInfo->Name);
  }
  else
  {
    fprintf(fp, "self.%s(", functionInfo->Name);
  }
  for (int i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    ValueInfo* parameter = functionInfo->Parameters[i];
    if (vtkWrapJavaScript_IsConstCharPointer(parameter))
    {
      fprintf(fp, " arg_%d.c_str()", i);
    }
    else if (vtkWrap_IsFunction(parameter))
    {
      fprintf(fp, " reinterpret_cast<");
      fprintf(fp, "%s", parameter->Function->ReturnValue->Class);
      if (vtkWrap_IsPointer(parameter->Function->ReturnValue))
      {
        fprintf(fp, "*");
      }
      fprintf(fp, "(*)");
      FunctionInfo* cb = parameter->Function;
      vtkWrapJavaScript_WriteSignature(fp, cb, "", 0, 0, 0);
      char* sig = malloc(cb->NumberOfParameters + 2);
      ValueInfo* cbReturnValue = cb->ReturnValue;
      int j = 0;
      if (vtkWrap_IsVoidPointer(cbReturnValue))
      {
        sig[j] = 'i';
      }
      else if (vtkWrap_IsVoid(cbReturnValue))
      {
        sig[j] = 'v';
      }
      else if (vtkWrap_IsPointer(cbReturnValue) || vtkWrap_IsVTKObject(cbReturnValue))
      {
        sig[j] = 'i'; // TODO: 64 bit memory needs 'j'
      }
      else if (vtkWrap_IsInteger(cbReturnValue) || vtkWrap_IsBool(cbReturnValue))
      {
        sig[j] = 'i'; // TODO: 64 bit integers need 'j'
      }
      else if ((cbReturnValue->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FLOAT)
      {
        sig[j] = 'f';
      }
      else if ((cbReturnValue->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_DOUBLE)
      {
        sig[j] = 'd';
      }
      for (j = 1; j <= cb->NumberOfParameters; ++j)
      {
        ValueInfo* cbParam = cb->Parameters[j - 1];
        if (vtkWrap_IsVoidPointer(cbParam))
        {
          sig[j] = 'i';
        }
        else if (vtkWrap_IsVoid(cbParam))
        {
          sig[j] = 'v';
        }
        else if (vtkWrap_IsPointer(cbParam) || vtkWrap_IsVTKObject(cbParam))
        {
          sig[j] = 'i'; // TODO: 64 bit memory needs 'j'
        }
        else if (vtkWrap_IsInteger(cbParam) || vtkWrap_IsBool(cbParam))
        {
          sig[j] = 'i'; // TODO: 64 bit integers need 'j'
        }
        else if ((cbParam->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FLOAT)
        {
          sig[j] = 'f';
        }
        else if ((cbParam->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_DOUBLE)
        {
          sig[j] = 'd';
        }
      }
      sig[j] = '\0';
      fprintf(fp,
        ">(emscripten::val::module_property(\"addFunction\")(arg_%d, "
        "std::string(\"%s\")).as<int>())",
        i, sig);
      free(sig);
    }
    else
    {
      // all kinds of pointers are treated as integer addresses into wasm memory which is literally
      // a javascript Uint8Array
      if (vtkWrap_IsVoidPointer(parameter))
      {
        fprintf(fp, "reinterpret_cast<void*>(arg_%d)", i);
      }
      else if (vtkWrap_IsPointer(parameter) && vtkWrap_IsNumeric(parameter))
      {
        fprintf(fp, "reinterpret_cast<%s*>(arg_%d)", parameter->Class, i);
      }
      else if (vtkWrap_IsFunction(parameter))
      {
        // TODO:
        fprintf(fp, "&arg_%d", i);
      }
      else if (vtkWrap_IsArray(parameter))
      {
        fprintf(fp, "reinterpret_cast<%s*>(arg_%d)", parameter->Class, i);
      }
      else if (vtkWrap_IsNArray(parameter))
      {
        fprintf(fp, "reinterpret_cast<%s(*)", parameter->Class);
        for (int j = 1; j < parameter->NumberOfDimensions; ++j)
        {
          fprintf(fp, "[%s]", parameter->Dimensions[j]);
        }
        fprintf(fp, ">(arg_%d)", i);
      }
      else
      {
        fprintf(fp, " arg_%d", i);
      }
    }
    if (i < functionInfo->NumberOfParameters - 1)
    {
      fprintf(fp, ",");
    }
  }
  fprintf(fp, ")");
}

/* Whether the function parameters could accept memory addresses */
static int vtkWrapJavaScript_AcceptsMemoryAddress(FunctionInfo* functionInfo)
{
  for (int i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    ValueInfo* parameter = functionInfo->Parameters[i];
    if (vtkWrap_IsVoidPointer(parameter))
    {
      return 1;
    }
    else if (vtkWrap_IsPointer(parameter) && vtkWrap_IsNumeric(parameter))
    {
      return 1;
    }
    else if (vtkWrap_IsArray(parameter))
    {
      return 1;
    }
    else if (vtkWrap_IsNArray(parameter))
    {
      return 1;
    }
  }
  return 0;
}

/* -------------------------------------------------------------------- */
/* Whether embind should be explcitly told to allow raw pointers to pass through */
static int vtkWrapJavaScript_NeedsToAllowRawPointers(FunctionInfo* functionInfo)
{
  if (vtkWrap_IsPointer(functionInfo->ReturnValue) &&
    !vtkWrap_IsPODPointer(functionInfo->ReturnValue) &&
    !vtkWrap_IsChar(functionInfo->ReturnValue) && !vtkWrap_IsFunction(functionInfo->ReturnValue) &&
    !vtkWrap_IsVoidPointer(functionInfo->ReturnValue))
  {
    return 1;
  }
  for (int i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    ValueInfo* parameter = functionInfo->Parameters[i];
    if (vtkWrap_IsPointer(parameter) && !vtkWrap_IsPODPointer(parameter) &&
      !vtkWrap_IsChar(parameter) && !vtkWrap_IsFunction(parameter) &&
      !vtkWrap_IsVoidPointer(parameter))
    {
      return 1;
    }
  }
  return 0;
}

/* -------------------------------------------------------------------- */
/* Whether a memory view must be returned */
static int vtkWrapJavaScript_NeedsToReturnMemoryView(FunctionInfo* functionInfo)
{
  if (vtkWrap_IsPODPointer(functionInfo->ReturnValue) || vtkWrap_IsArray(functionInfo->ReturnValue))
  {
    return (functionInfo->ReturnValue->Count > 0) || (functionInfo->ReturnValue->CountHint != NULL);
  }
  return 0;
}

/* -------------------------------------------------------------------- */
/* Whether a memory address must be returned */
static int vtkWrapJavaScript_NeedsToReturnMemoryAddress(FunctionInfo* functionInfo)
{
  if (vtkWrap_IsPODPointer(functionInfo->ReturnValue) || vtkWrap_IsArray(functionInfo->ReturnValue))
  {
    return (functionInfo->ReturnValue->Count == 0) &&
      (functionInfo->ReturnValue->CountHint == NULL);
  }
  return vtkWrap_IsVoidPointer(functionInfo->ReturnValue);
}

/* -------------------------------------------------------------------- */
/* Whether arguments have non-const lvalue refs */
static int vtkWrapJavaScript_HasNonConstRefParameters(FunctionInfo* functionInfo)
{
  for (int i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    if (vtkWrap_IsNonConstRef(functionInfo->Parameters[i]))
    {
      return 1;
    }
  }
  return 0;
}

/* -------------------------------------------------------------------- */
/* Whether an argument is a function pointer */
static int vtkWrapJavaScript_HasFunctionPointerParameters(FunctionInfo* functionInfo)
{
  for (int i = 0; i < functionInfo->NumberOfParameters; ++i)
  {
    if (vtkWrap_IsFunction(functionInfo->Parameters[i]))
    {
      return 1;
    }
  }
  return 0;
}

/* -------------------------------------------------------------------- */
/* print out the code for one method, including all of its overloads */
void vtkWrapJavaScript_GenerateOneMethod(FILE* fp, const char* classname,
  FunctionInfo* wrappedFunctions[], int numberOfWrappedFunctions,
  FunctionInfo* unwrappableFunctions[], int numberOfUnwrappableFunctions, int fnum,
  const char* indent)
{
  FunctionInfo* theFunc = wrappedFunctions[fnum];
  const char* name = theFunc->Name;

  // Functions with a bucketId == NO_BUCKET will not be processed.
  int* buckets = calloc(numberOfWrappedFunctions, sizeof(int));
  const int NO_BUCKET = 0;
  // build buckets of functions with similar name AND same number of parameters.
  for (int occ1 = fnum, bucketId = NO_BUCKET; occ1 < numberOfWrappedFunctions; occ1++)
  {
    FunctionInfo* func1 = wrappedFunctions[occ1];
    int nargs1 = vtkWrap_CountWrappedParameters(func1);
    if (func1->Name && !strcmp(func1->Name, name))
    {
      if (buckets[occ1] == NO_BUCKET)
      {
        // start a new bucket for this occurrence if not already bucketed.
        buckets[occ1] = ++bucketId;
        DLOG("\n\n=> New bucketId:%d", bucketId);
        DLOG("\n=> Bucketed %s in bucketId:%d", func1->Signature, bucketId);
      }
      else if (buckets[occ1] > 0)
      {
        // already bucketed.
        DLOG("\n=> Already bucketed %s in bucketId:%d", func1->Signature, bucketId);
        continue;
      }
      // attempt to bucket other overloads.
      for (int occ2 = occ1 + 1; occ2 < numberOfWrappedFunctions; occ2++)
      {
        FunctionInfo* func2 = wrappedFunctions[occ2];
        int nargs2 = vtkWrap_CountWrappedParameters(func2);
        if ((func2->Name && !strcmp(func2->Name, name)) && (nargs2 == nargs1))
        {
          buckets[occ2] = bucketId;
          DLOG("\n=> Bucketed %s in bucketId:%d", func2->Signature, bucketId);
        }
      }
    }
  }
  // count the number of buckets.
  int numBuckets = 0;
  for (int i = fnum; i < numberOfWrappedFunctions; ++i)
  {
    numBuckets = (buckets[i] > numBuckets) ? buckets[i] : numBuckets;
  }
  DLOG("\n=> numBuckets=%d", numBuckets);

  // count the number of functions in each bucket.
  typedef struct
  {
    // number of functions in the bucket
    int ItemCount;
    // number of parameters for the functions in the bucket. Remember, all functions in a bucket
    // have same number of parameters
    int ParameterCount;
    // number of occurrences of these functions which are unwrappable.
    int UnwrappableOccurrenceCount;
  } BucketData;
  BucketData* bucketData = calloc(numBuckets + 1, sizeof(BucketData));

  // Populate bucketData with no. of functions, parameter counts and number of unwrappable
  // occurrences
  for (int i = fnum; i < numberOfWrappedFunctions; ++i)
  {
    const int bucketId = buckets[i];
    if (bucketId == NO_BUCKET)
    {
      continue;
    }
    bucketData[bucketId].ItemCount++;
    bucketData[bucketId].ParameterCount = wrappedFunctions[i]->NumberOfParameters;
    // mark buckets that have unwrappable Occurrences
    for (int j = 0; j < numberOfUnwrappableFunctions; ++j)
    {
      if (wrappedFunctions[i]->Name &&
        !strcmp(wrappedFunctions[i]->Name, unwrappableFunctions[j]->Name))
      {
        bucketData[bucketId].UnwrappableOccurrenceCount += 1;
      }
    }
  }

  // prints the number of functions in bucketIds: 1,....,numBuckets
  for (int i = 1; i <= numBuckets; ++i)
  {
    DLOG("\n=> %d function(s) in bucketId:%d", bucketData[i].ItemCount, i);
    DLOG("\n=> %d occurrences in bucketId:%d cannot be wrapped",
      bucketData[i].UnwrappableOccurrenceCount, i);
  }

  // process functions from the buckets one at a time.
  for (int bucketId = 1; bucketId <= numBuckets; ++bucketId)
  {
    // an overload will be selected with
    // clang-format off
    // emscripten::select_overload<ReturnType(Arg...)>([](Arg...) -> ReturnType {return self.Func(...arg); })
    // if this bucket has more than one items 
    // or
    // if there is only one item in this bucket and there are more buckets 
    // or
    // if there are non-zero number of occurrences which cannot be wrapped.
    // clang-format on
    const int needsOverloadSelection = (bucketData[bucketId].ItemCount > 1) ||
      (bucketData[bucketId].ItemCount == 1 && numBuckets > 1) ||
      (bucketData[bucketId].UnwrappableOccurrenceCount > 0);
    if (needsOverloadSelection)
    {
      DLOG("\n=> Generate select_overload code");
      for (int i = fnum; i < numberOfWrappedFunctions; ++i)
      {
        if (buckets[i] != bucketId)
        {
          continue;
        }
        DLOG("\n- %s", wrappedFunctions[i]->Signature);

        if (wrappedFunctions[i]->IsStatic)
        {
          fprintf(fp, "\n%s%s.class_function(\"%s\"", indent, indent, name);
        }
        else
        {
          fprintf(fp, "\n%s%s.function(\"%s\"", indent, indent, name);
        }

        fprintf(fp, ", emscripten::select_overload<");
        vtkWrapJavaScript_WriteOverloadReturnType(fp, wrappedFunctions[i]);
        vtkWrapJavaScript_WriteSignature(fp, wrappedFunctions[i], "arg", 0, 1, 1);
        fprintf(fp, ">([]");
        vtkWrapJavaScript_WriteSignature(fp, wrappedFunctions[i], "arg", 1, 1, 1);
        // write return type
        fprintf(fp, " -> ");
        vtkWrapJavaScript_WriteOverloadReturnType(fp, wrappedFunctions[i]);
        fprintf(fp, " ");
        // lambda body start
        fprintf(fp, "{");
        // declare args

        fprintf(fp, " return ");
        ValueInfo* returnValue = wrappedFunctions[i]->ReturnValue;
        if (vtkWrapJavaScript_NeedsToReturnMemoryView(wrappedFunctions[i]))
        {
          if (returnValue->CountHint)
          {
            if (wrappedFunctions[i]->IsStatic)
            {
              fprintf(fp, " emscripten::val(emscripten::typed_memory_view(%s::%s(),", classname,
                returnValue->CountHint);
            }
            else
            {
              fprintf(fp, " emscripten::val(emscripten::typed_memory_view(self.%s(),",
                returnValue->CountHint);
            }
          }
          else if (returnValue->Count > 0)
          {
            fprintf(fp, " emscripten::val(emscripten::typed_memory_view(%d,", returnValue->Count);
          }
        }
        else if (vtkWrapJavaScript_NeedsToReturnMemoryAddress(wrappedFunctions[i]))
        {
          fprintf(fp, "reinterpret_cast<std::uintptr_t>(");
        }
        vtkWrapJavaScript_WriteMemberFunctionCall(fp, wrappedFunctions[i]);
        if (vtkWrapJavaScript_NeedsToReturnMemoryView(wrappedFunctions[i]))
        {
          fprintf(fp, "))");
        }
        else if (vtkWrapJavaScript_NeedsToReturnMemoryAddress(wrappedFunctions[i]))
        {
          fprintf(fp, ")");
        }
        fprintf(fp, "; ");
        fprintf(fp, "}");
        // lambda body end
        fprintf(fp, ")");
        if (vtkWrapJavaScript_NeedsToAllowRawPointers(wrappedFunctions[i]))
        {
          fprintf(fp, ", emscripten::allow_raw_pointers()");
        }
        fprintf(fp, ")");
      }
      DLOG("\n");
    }
    else
    {
      DLOG("\n=> Generate direct code (may have optional_override)");
      for (int i = fnum; i < numberOfWrappedFunctions; ++i)
      {
        if (buckets[i] != bucketId)
        {
          continue;
        }
        DLOG("\n- %s", wrappedFunctions[i]->Signature);

        if (wrappedFunctions[i]->IsStatic)
        {
          fprintf(fp, "\n%s%s.class_function(\"%s\",", indent, indent, name);
        }
        else
        {
          fprintf(fp, "\n%s%s.function(\"%s\",", indent, indent, name);
        }
        int needsOptionalOverride = vtkWrapJavaScript_NeedsCppStringOverload(wrappedFunctions[i]) ||
          vtkWrapJavaScript_NeedsToReturnMemoryView(wrappedFunctions[i]) ||
          vtkWrapJavaScript_NeedsToReturnMemoryAddress(wrappedFunctions[i]) ||
          vtkWrapJavaScript_AcceptsMemoryAddress(wrappedFunctions[i]) ||
          vtkWrapJavaScript_HasNonConstRefParameters(wrappedFunctions[i]) ||
          vtkWrapJavaScript_HasFunctionPointerParameters(wrappedFunctions[i]);
        if (needsOptionalOverride)
        {
          DLOG(" [optional_override]");
          fprintf(fp, " emscripten::optional_override");
          // Construct a lambda.
          fprintf(fp, "([]");
          // lambda arguments.
          vtkWrapJavaScript_WriteSignature(fp, wrappedFunctions[i], "arg", 1, 1, 1);
          // write return type
          fprintf(fp, " -> ");
          vtkWrapJavaScript_WriteOverloadReturnType(fp, wrappedFunctions[i]);
          fprintf(fp, " ");
          // lambda body start
          fprintf(fp, "{ ");
          fprintf(fp, " return ");
          ValueInfo* returnValue = wrappedFunctions[i]->ReturnValue;
          if (vtkWrapJavaScript_NeedsToReturnMemoryView(wrappedFunctions[i]))
          {
            if (returnValue->CountHint)
            {
              if (wrappedFunctions[i]->IsStatic)
              {
                fprintf(fp, " emscripten::val(emscripten::typed_memory_view(%s::%s(),", classname,
                  returnValue->CountHint);
              }
              else
              {
                fprintf(fp, " emscripten::val(emscripten::typed_memory_view(self.%s(),",
                  returnValue->CountHint);
              }
            }
            else if (returnValue->Count > 0)
            {
              fprintf(fp, " emscripten::val(emscripten::typed_memory_view(%d,", returnValue->Count);
            }
          }
          else if (vtkWrapJavaScript_NeedsToReturnMemoryAddress(wrappedFunctions[i]))
          {
            fprintf(fp, "reinterpret_cast<std::uintptr_t>(");
          }
          // call the member function
          vtkWrapJavaScript_WriteMemberFunctionCall(fp, wrappedFunctions[i]);
          if (vtkWrapJavaScript_NeedsToReturnMemoryView(wrappedFunctions[i]))
          {
            fprintf(fp, "))");
          }
          else if (vtkWrapJavaScript_NeedsToReturnMemoryAddress(wrappedFunctions[i]))
          {
            fprintf(fp, ")");
          }
          fprintf(fp, ";");
          fprintf(fp, "}");
          // lambda body end
          fprintf(fp, ")");
          if (vtkWrapJavaScript_NeedsToAllowRawPointers(wrappedFunctions[i]))
          {
            fprintf(fp, ", emscripten::allow_raw_pointers()");
          }
          fprintf(fp, ")");
        }
        else
        {
          fprintf(fp, " &%s::%s", classname, wrappedFunctions[i]->Name);
          if (vtkWrapJavaScript_NeedsToAllowRawPointers(wrappedFunctions[i]))
          {
            fprintf(fp, ", emscripten::allow_raw_pointers()");
          }
          fprintf(fp, ")");
        }
      }
      DLOG("\n");
    }
  }
  free(bucketData);
  free(buckets);

  /* clear all occurrences of this method from further consideration */
  for (int occ = fnum + 1; occ < numberOfWrappedFunctions; occ++)
  {
    FunctionInfo* theOccurrence = wrappedFunctions[occ];

    if ((theOccurrence->Name && strcmp(theOccurrence->Name, name) == 0))
    {
      theOccurrence->Name = NULL;
    }
  }
}

/* -------------------------------------------------------------------- */
/* check whether a method is wrappable */
int vtkWrapJavaScript_MethodCheck(
  ClassInfo* data, FunctionInfo* currentFunction, HierarchyInfo* hinfo)
{
  int i, n;

  /* some functions will not get wrapped no matter what */
  if (currentFunction->IsExcluded || currentFunction->IsDeleted ||
    currentFunction->Access != VTK_ACCESS_PUBLIC ||
    vtkWrap_IsInheritedMethod(data, currentFunction))
  {
    return 0;
  }

  /* new and delete are meaningless in wrapped languages */
  if (currentFunction->Name == 0 || strcmp("Register", currentFunction->Name) == 0 ||
    strcmp("UnRegister", currentFunction->Name) == 0 ||
    strcmp("Delete", currentFunction->Name) == 0 || strcmp("New", currentFunction->Name) == 0)
  {
    return 0;
  }

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfParameters == 2 &&
    vtkWrap_IsVoidFunction(currentFunction->Parameters[0]) &&
    vtkWrap_IsVoidPointer(currentFunction->Parameters[1]) &&
    !vtkWrap_IsConst(currentFunction->Parameters[1]) &&
    vtkWrap_IsVoid(currentFunction->ReturnValue))
  {
    return 1;
  }

  n = vtkWrap_CountWrappedParameters(currentFunction);

  /* check to see if we can handle all the args */
  for (i = 0; i < n; i++)
  {
    if (!vtkWrapJavaScript_IsValueWrappable(
          data, currentFunction->Parameters[i], hinfo, VTK_WRAP_ARG))
    {
      return 0;
    }
  }

  /* check the return value */
  if (!vtkWrapJavaScript_IsValueWrappable(
        data, currentFunction->ReturnValue, hinfo, VTK_WRAP_RETURN))
  {
    return 0;
  }

  return 1;
}

/* -------------------------------------------------------------------- */
/* print out all methods and the method table */
void vtkWrapJavaScript_GenerateMethods(FILE* fp, const char* classname, ClassInfo* data,
  FileInfo* finfo, HierarchyInfo* hinfo, const char* indent)
{
  int i;
  int fnum;
  int numberOfWrappedFunctions = 0;
  int numberOfUnwrappableFunctions = 0;
  FunctionInfo** wrappedFunctions;
  FunctionInfo** unwrappableFunctions;
  FunctionInfo* theFunc;

  wrappedFunctions = (FunctionInfo**)malloc(data->NumberOfFunctions * sizeof(FunctionInfo*));
  unwrappableFunctions = (FunctionInfo**)malloc(data->NumberOfFunctions * sizeof(FunctionInfo*));

  /* TODO: Custom methods like Register, FastDelete, Memkind */
  vtkWrapJavaScript_CustomMethods(classname, data);

  /* size hints for GetTuple(), ... */
  vtkWrap_FindCountHints(data, finfo, hinfo);

  /* TODO: identify methods that create new instances of objects */

  /* TODO: identify methods that need file paths ? */

  /* go through all functions and see which are wrappable */
  DLOG("\n Check %s methods for wrappability", classname)
  for (i = 0; i < data->NumberOfFunctions; ++i)
  {
    theFunc = data->Functions[i];
    /* check for wrappability */
    if (!vtkWrapJavaScript_MethodCheck(data, theFunc, hinfo))
    {
      unwrappableFunctions[numberOfUnwrappableFunctions++] = theFunc;
      DLOG("\n- %s cannot be wrapped because MethodCheck failed", theFunc->Signature);
    }
    else if (theFunc->IsOperator)
    {
      unwrappableFunctions[numberOfUnwrappableFunctions++] = theFunc;
      DLOG("\n- %s cannot be wrapped because its an operator", theFunc->Signature);
    }
    else if (theFunc->Template)
    {
      unwrappableFunctions[numberOfUnwrappableFunctions++] = theFunc;
      DLOG("\n- %s cannot be wrapped because its a template", theFunc->Signature);
    }
    else if (vtkWrap_IsDestructor(data, theFunc))
    {
      unwrappableFunctions[numberOfUnwrappableFunctions++] = theFunc;
      DLOG("\n- %s cannot be wrapped because its a destructor", theFunc->Signature);
    }
    else if (vtkWrap_IsConstructor(data, theFunc))
    {
      unwrappableFunctions[numberOfUnwrappableFunctions++] = theFunc;
      DLOG("\n- %s cannot be wrapped because its a constructor", theFunc->Signature);
    }
    else if (theFunc->IsPureVirtual)
    {
      unwrappableFunctions[numberOfUnwrappableFunctions++] = theFunc;
      DLOG("\n- %s cannot be wrapped because its a pure virtual", theFunc->Signature);
    }
    else if (theFunc->IsDeprecated)
    {
      unwrappableFunctions[numberOfUnwrappableFunctions++] = theFunc;
      DLOG("\n- %s cannot be wrapped because its deprecated", theFunc->Signature);
    }
    else
    {
      wrappedFunctions[numberOfWrappedFunctions++] = theFunc;
    }
  }

  /* write out the wrapper for each function in the array */
  for (fnum = 0; fnum < numberOfWrappedFunctions; ++fnum)
  {
    theFunc = wrappedFunctions[fnum];

    /* if theFunc wasn't removed, process all its signatures */
    if (theFunc->Name)
    {
      vtkWrapJavaScript_GenerateOneMethod(fp, classname, wrappedFunctions, numberOfWrappedFunctions,
        unwrappableFunctions, numberOfUnwrappableFunctions, fnum, indent);
    }
  }
  free(wrappedFunctions);
  free(unwrappableFunctions);
}

// NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)
// NOLINTEND(bugprone-unsafe-functions)
