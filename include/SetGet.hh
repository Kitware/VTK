//
// Macros for setting and getting instance variables.  Macros are available 
// for  built-in types; for character strings; vector arrays of built-in 
// types size 2,3,4; and for setting objects (i.e., Registering objects).
// Macros enforce proper use of Debug, Modified time, and Registering objects.
//

#ifndef __vlSetGet_hh
#define __vlSetGet_hh

#include <string.h>
// 
// For super speedy execution define __vlNoDebug
//
//#define __vlNoDebug

//
// Set built-in type.  Creates member Set"name"() (e.g., SetVisibility());
//
#define vlSetMacro(name,type) \
void Set##name (type _arg) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to " << _arg << "\n"; \
  if (name != _arg) \
    { \
    name = _arg; \
    Modified(); \
    } \
  } 

//
// Get built-in type.  Creates member Get"name"() (e.g., GetVisibility());
//
#define vlGetMacro(name,type) \
type Get##name () { \
  if (Debug) cerr << GetClassName() << " " << this << ", returning " << #name " of " << name << "\n"; \
  return name; \
  } 

//
// Set character string.  Creates member Set"name"() 
// (e.g., SetFilename(char *));
//
#define vlSetStringMacro(name) \
void Set##name (char* _arg) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to " << _arg << "\n"; \
  if ( name && _arg ) \
    if ( !strcmp(name,_arg) ) return; \
  if (name) delete [] name; \
  if (_arg) \
    { \
    name = new char[strlen(_arg)+1]; \
    strcpy(name,_arg); \
    } \
   else \
    { \
    name = 0; \
    } \
  Modified(); \
  } 

//
// Get character string.  Creates member Get"name"() 
// (e.g., char *GetFilename());
//
#define vlGetStringMacro(name) \
char* Get##name () { \
  if (Debug) cerr << GetClassName() << " " << this << ", returning " << #name " of " << name << "\n"; \
  return name; \
  } 

//
// Set built-in type where value is constrained between min/max limits.
// Create member Set"name"() (e.q., SetRadius()). #defines are 
// convienience for clamping open-ended values.
//
#define LARGE_FLOAT 1.0e29
#define LARGE_INTEGER 2147483646 /* 2**31 - 1 */
#define vlSetClampMacro(name,type,min,max) \
void Set##name (type _arg) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to " << _arg << "\n"; \
  if (name != _arg) \
    { \
    name = (_arg<min?min:(_arg>max?max:_arg)); \
    Modified(); \
    } \
  } 

//
// Set pointer to object; uses vlObject reference counting methodology.
// Creates method Set"name"() (e.g., SetPoints()).
//
#define vlSetObjectMacro(name,type) \
void Set##name (type* _arg) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to " << &_arg << "\n"; \
  if (name != _arg) \
    { \
    if (name != 0) name->UnRegister((void *)this); \
    name = _arg; \
    name->Register((void *)this); \
    Modified(); \
    } \
  } 

//
// Get pointer to object.  Creates member Get"name" (e.g., GetPoints()).
//
#define vlGetObjectMacro(name,type) \
type *Get##name () \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", returning " << #name " address " << name << "\n"; \
  return name; \
  } 

//
// Create members "name"On() and "name"Off() (e.g., DebugOn() DebugOff()).
// Set method must be defined to use this macro.
//
#define vlBooleanMacro(name,type) \
void name##On () { Set##name((type)1);}; \
void name##Off () { Set##name((type)0);}

//
// Following set macros for vectors define two members for each macro.  The first 
// allows setting of individual components (e.g, SetColor(float,float,float)), 
// the second allows setting from an array (e.g., SetColor(float* rgb[3])).
// The macros vary in the size of the vector they deal with.
//
#define vlSetVector2Macro(name,type) \
void Set##name (type _arg1, type _arg2) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg1 << "," << _arg2 << ")\n"; \
  if ((name[0] != _arg1)||(name[1] != _arg2)) \
    { \
    Modified(); \
    } \
  name[0] = _arg1; \
  name[1] = _arg2; \
  }; \
void Set##name (type _arg[2]) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg[0] << "," << _arg[1] << ")\n"; \
  if ((name[0] != _arg[0])||(name[1] != _arg[1])) \
    { \
    Modified(); \
    } \
  name[0] = _arg[0]; \
  name[1] = _arg[1]; \
  } 

#define vlSetVector3Macro(name,type) \
void Set##name (type _arg1, type _arg2, type _arg3) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << ")\n"; \
  if ((name[0] != _arg1)||(name[1] != _arg2)||(name[2] != _arg3)) \
    { \
    Modified(); \
    } \
  name[0] = _arg1; \
  name[1] = _arg2; \
  name[2] = _arg3; \
  }; \
void Set##name (type _arg[3]) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg[0] << "," << _arg[1] << "," << _arg[2] << ")\n"; \
  if ((name[0] != _arg[0])||(name[1] != _arg[1])||(name[2] != _arg[2])) \
    { \
    Modified(); \
    } \
  name[0] = _arg[0]; \
  name[1] = _arg[1]; \
  name[2] = _arg[2]; \
  } 

#define vlSetVector4Macro(name,type) \
void Set##name (type _arg1, type _arg2, type _arg3, type _arg4) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << ")\n"; \
  if ((name[0] != _arg1)||(name[1] != _arg2)||(name[2] != _arg3)||(name[3] != _arg4)) \
    { \
    Modified(); \
    } \
  name[0] = _arg1; \
  name[1] = _arg2; \
  name[2] = _arg3; \
  name[3] = _arg4; \
  }; \
void Set##name (type _arg[4]) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg[0] << "," << _arg[1] << "," << _arg[2] << "," << _arg[3] << ")\n"; \
  if ((name[0] != _arg[0])||(name[1] != _arg[1])||(name[2] != _arg[2])||(name[3] != _arg[3])) \
    { \
    Modified(); \
    } \
  name[0] = _arg[0]; \
  name[1] = _arg[1]; \
  name[2] = _arg[2]; \
  name[3] = _arg[3]; \
  } 

//
// Get vector macro returns pointer to type (i.e., array of type). 
// Example: float *GetColor()
//
#define vlGetVectorMacro(name,type) \
type *Get##name () { \
  if (Debug) cerr << GetClassName() << " " << this << ", returning " << #name " pointer " << name << "\n"; \
  return name; \
} 

//
// This macro is used for  debug statements in instance methods
// vlDebugMacro(<< "this is debug info" << this->SomeVariable << "\n");
//
#define vlDebugMacro(x) \
  if (Debug) cerr << "In " __FILE__ << ", line " << __LINE__ << "\n   : " x

#endif
