/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSetGet.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME SetGet Macros - standard macros for setting/getting instance variables
// .SECTION Description
// The SetGet macros are used to interface to instance variables
// in a standard fashion. This includes properly treating modified time
// and printing out debug information.
//
// Macros are available for built-in types; for character strings; 
// vector arrays of built-in types size 2,3,4; for setting objects; and
// debug, warning, and error printout information.

#ifndef __vtkSetGet_h
#define __vtkSetGet_h
#include <string.h>

//
// Some constants used throughout code
//
#define VTK_LARGE_FLOAT 1.0e29
#define VTK_LARGE_INTEGER 2147483647 // 2^31 - 1

//
// Set built-in type.  Creates member Set"name"() (e.g., SetVisibility());
//
#define vtkSetMacro(name,type) \
void Set##name (type _arg) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to " << _arg); \
  if (name != _arg) \
    { \
    name = _arg; \
    this->Modified(); \
    } \
  } 

//
// Get built-in type.  Creates member Get"name"() (e.g., GetVisibility());
//
#define vtkGetMacro(name,type) \
type Get##name () { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " of " << name ); \
  return name; \
  } 

//
// Set character string.  Creates member Set"name"() 
// (e.g., SetFilename(char *));
//
#define vtkSetStringMacro(name) \
void Set##name (char* _arg) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to " << _arg ); \
  if ( name && _arg && (!strcmp(name,_arg))) return; \
  if (name) delete [] name; \
  if (_arg) \
    { \
    name = new char[strlen(_arg)+1]; \
    strcpy(name,_arg); \
    } \
   else \
    { \
    name = NULL; \
    } \
  this->Modified(); \
  } 

//
// Get character string.  Creates member Get"name"() 
// (e.g., char *GetFilename());
//
#define vtkGetStringMacro(name) \
char* Get##name () { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " of " << name); \
  return name; \
  } 

//
// Set built-in type where value is constrained between min/max limits.
// Create member Set"name"() (e.q., SetRadius()). #defines are 
// convienience for clamping open-ended values.
//
#define vtkSetClampMacro(name,type,min,max) \
void Set##name (type _arg) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to " << _arg ); \
  if (name != _arg) \
    { \
    name = (_arg<min?min:(_arg>max?max:_arg)); \
    this->Modified(); \
    } \
  } 

//
// Set pointer to object. Creates method Set"name"() (e.g., SetPoints()).
//
#define vtkSetObjectMacro(name,type) \
void Set##name (type* _arg) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to " << _arg); \
  if (name != _arg) \
    { \
    name = _arg; \
    this->Modified(); \
    } \
  } \
void Set##name (type& _arg) \
  { \
  Set##name (&_arg);\
  } 

//
// Set pointer to object; uses vtkReferenceCount reference counting methodology.
// Creates method Set"name"() (e.g., SetPoints()).
//
#define vtkSetReferenceCountedObjectMacro(name,type) \
void Set##name (type* _arg) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to " << _arg ); \
  if (name != _arg) \
    { \
    if (name != NULL) name->UnRegister(this); \
    name = _arg; \
    if (name != NULL) name->Register(this); \
    this->Modified(); \
    } \
  } 

//
// Get pointer to object.  Creates member Get"name" (e.g., GetPoints()).
//
#define vtkGetObjectMacro(name,type) \
type *Get##name () \
  { \
      vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " #name " address " << name ); \
  return name; \
  } 

//
// Create members "name"On() and "name"Off() (e.g., DebugOn() DebugOff()).
// Set method must be defined to use this macro.
//
#define vtkBooleanMacro(name,type) \
void name##On () { Set##name((type)1);}; \
void name##Off () { Set##name((type)0);}

//
// Following set macros for vectors define two members for each macro.  The first 
// allows setting of individual components (e.g, SetColor(float,float,float)), 
// the second allows setting from an array (e.g., SetColor(float* rgb[3])).
// The macros vary in the size of the vector they deal with.
//
#define vtkSetVector2Macro(name,type) \
void Set##name (type _arg1, type _arg2) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to (" << _arg1 << "," << _arg2 << ")"); \
  if ((name[0] != _arg1)||(name[1] != _arg2)) \
    { \
    this->Modified(); \
    name[0] = _arg1; \
    name[1] = _arg2; \
    } \
  }; \
void Set##name (type _arg[2]) \
  { \
  Set##name (_arg[0], _arg[1]); \
  } 

#define vtkGetVector2Macro(name,type) \
type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << name); \
  return name; \
} \
void Get##name (type &_arg1, type &_arg2) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " = (" << _arg1 << "," << _arg2 << ")"); \
    _arg1 = name[0]; \
    _arg2 = name[1]; \
  }; \
void Get##name (type _arg[2]) \
  { \
  Get##name (_arg[0], _arg[1]);\
  } 

#define vtkSetVector3Macro(name,type) \
void Set##name (type _arg1, type _arg2, type _arg3) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << ")"); \
  if ((name[0] != _arg1)||(name[1] != _arg2)||(name[2] != _arg3)) \
    { \
    this->Modified(); \
    name[0] = _arg1; \
    name[1] = _arg2; \
    name[2] = _arg3; \
    } \
  }; \
void Set##name (type _arg[3]) \
  { \
  Set##name (_arg[0], _arg[1], _arg[2]);\
  } 

#define vtkGetVector3Macro(name,type) \
type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << name); \
  return name; \
} \
void Get##name (type &_arg1, type &_arg2, type &_arg3) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " = (" << _arg1 << "," << _arg2 << "," << _arg3 << ")"); \
    _arg1 = name[0]; \
    _arg2 = name[1]; \
    _arg3 = name[2]; \
  }; \
void Get##name (type _arg[3]) \
  { \
  Get##name (_arg[0], _arg[1], _arg[2]);\
  } 

#define vtkSetVector4Macro(name,type) \
void Set##name (type _arg1, type _arg2, type _arg3, type _arg4) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << ")"); \
  if ((name[0] != _arg1)||(name[1] != _arg2)||(name[2] != _arg3)||(name[3] != _arg4)) \
    { \
    this->Modified(); \
    name[0] = _arg1; \
    name[1] = _arg2; \
    name[2] = _arg3; \
    name[3] = _arg4; \
    } \
  }; \
void Set##name (type _arg[4]) \
  { \
  Set##name (_arg[0], _arg[1], _arg[2], _arg[3]);\
  } 

#define vtkGetVector4Macro(name,type) \
type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << name); \
  return name; \
} \
void Get##name (type &_arg1, type &_arg2, type &_arg3, type &_arg4) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " = (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << ")"); \
    _arg1 = name[0]; \
    _arg2 = name[1]; \
    _arg3 = name[2]; \
    _arg4 = name[3]; \
  }; \
void Get##name (type _arg[4]) \
  { \
  Get##name (_arg[0], _arg[1], _arg[2], _arg[3]);\
  } 

//
// General set vector macro creates a single method that copies specified
// number of values into object.
// Examples: void SetColor(c,3)
//
#define vtkSetVectorMacro(name,type,count) \
void Set##name(type data[]) \
{ \
  int i; \
  for (i=0; i<count; i++) if ( data[i] != name[i] ) break; \
  if ( i < count ) \
    { \
    this->Modified(); \
    for (i=0; i<count; i++) name[i] = data[i]; \
    } \
}

//
// Get vector macro defines two methods. One returns pointer to type 
// (i.e., array of type). This is for efficiency. The second copies data
// into user provided array. This is more object-oriented.
// Examples: float *GetColor() and void GetColor(float c[count]).
//
#define vtkGetVectorMacro(name,type,count) \
type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << name); \
  return name; \
} \
void Get##name (type data[count]) \
{ \
  for (int i=0; i<count; i++) data[i] = name[i]; \
}

#ifdef _WIN32
//
// This macro is used for any output that may not be in an instance method
// vtkGenericWarningMacro(<< "this is debug info" << this->SomeVariable);
//
#define vtkGenericWarningMacro(x) \
{ if (vtkObject::GetGlobalWarningDisplay()) {char *vtkmsgbuff; ostrstream vtkmsg; \
      vtkmsg << "Generic Warning: In " __FILE__ ", line " << __LINE__ << "\n" x << "\nPress Cancel to supress any further messages." << ends; \
      vtkmsgbuff = vtkmsg.str(); \
      if (MessageBox(NULL,vtkmsgbuff,"Debug Info",MB_ICONINFORMATION | MB_OKCANCEL) == IDCANCEL) vtkObject::GlobalWarningDisplayOff(); \
      vtkmsg.rdbuf()->freeze(0);}}

//
// This macro is used for  debug statements in instance methods
// vtkDebugMacro(<< "this is debug info" << this->SomeVariable);
//
#ifdef VTK_LEAN_AND_MEAN
#define vtkDebugMacro(x)
#else
#define vtkDebugMacro(x) \
{ if (Debug && vtkObject::GetGlobalWarningDisplay()) \
    { char *vtkmsgbuff; ostrstream vtkmsg; \
      vtkmsg << "Debug: In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x << "\nPress Cancel to supress any further messages." << ends; \
      vtkmsgbuff = vtkmsg.str(); \
      if (MessageBox(NULL,vtkmsgbuff,"Debug Info",MB_ICONINFORMATION | MB_OKCANCEL) == IDCANCEL) vtkObject::GlobalWarningDisplayOff();  \
      vtkmsg.rdbuf()->freeze(0);}}
#endif
//
// This macro is used to print out warning messages.
// vtkWarningMacro(<< "Warning message" << variable);
//
#define vtkWarningMacro(x) \
{ if (vtkObject::GetGlobalWarningDisplay()) {char *vtkmsgbuff; ostrstream vtkmsg; \
      vtkmsg << "Warning: In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x << "\nPress Cancel to supress any further messages." << ends; \
      vtkmsgbuff = vtkmsg.str(); \
      if (MessageBox(NULL,vtkmsgbuff,"Warning",MB_ICONWARNING | MB_OKCANCEL) == IDCANCEL) vtkObject::GlobalWarningDisplayOff(); \
      vtkmsg.rdbuf()->freeze(0);}}

//
// This macro is used to print out errors
// vtkErrorMacro(<< "Error message" << variable);
//
#define vtkErrorMacro(x) \
{ if (vtkObject::GetGlobalWarningDisplay()) {char *vtkmsgbuff; ostrstream vtkmsg; \
      vtkmsg << "ERROR: In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x << "\nPress Cancel to supress any further messages." << ends; \
      vtkmsgbuff = vtkmsg.str(); \
      if (MessageBox(NULL,vtkmsgbuff,"Error",MB_ICONERROR | MB_OKCANCEL) == IDCANCEL) vtkObject::GlobalWarningDisplayOff(); \
      vtkmsg.rdbuf()->freeze(0); vtkObject::BreakOnError();}}

#else
//
// This macro is used for any output that may not be in an instance method
// vtkGenericWarningMacro(<< "this is debug info" << this->SomeVariable);
//
#define vtkGenericWarningMacro(x) \
if (vtkObject::GetGlobalWarningDisplay()) cerr << "Generic Warning: In " __FILE__ ", line " << __LINE__ << "\n  " x << "\n\n"

//
// This macro is used for  debug statements in instance methods
// vtkDebugMacro(<< "this is debug info" << this->SomeVariable);
//
#ifdef VTK_LEAN_AND_MEAN
#define vtkDebugMacro(x)
#else
#define vtkDebugMacro(x) \
if (Debug && vtkObject::GetGlobalWarningDisplay()) cerr << "Debug: In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x <<  "\n\n"
#endif
//
// This macro is used to print out warning messages.
// vtkWarningMacro(<< "Warning message" << variable);
//
#define vtkWarningMacro(x) \
if (vtkObject::GetGlobalWarningDisplay()) cerr << "Warning: In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x << "\n\n"

//
// This macro is used to print out errors
// vtkErrorMacro(<< "Error message" << variable);
//
#define vtkErrorMacro(x) \
{ if (vtkObject::GetGlobalWarningDisplay()) cerr << "ERROR In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x << "\n\n"; vtkObject::BreakOnError();}
#endif

//
// This macro is used to quiet compiler warnings about unused parameters
// to methods. Only use it when the parameter really shouldn't be used.
// Don't use it as a way to shut up the compiler while you take your
// sweet time getting around to implementing the method.
//
#define vtkNotUsed(x)

#endif
