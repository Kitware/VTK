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
#define VTK_LARGE_FLOAT 1.0e+38F
#define VTK_LARGE_INTEGER 2147483647 // 2^31 - 1

// These types are returned by GetDataType to indicate pixel type.
#define VTK_VOID            0
#define VTK_BIT             1 
#define VTK_CHAR            2
#define VTK_UNSIGNED_CHAR   3
#define VTK_SHORT           4
#define VTK_UNSIGNED_SHORT  5
#define VTK_INT             6
#define VTK_UNSIGNED_INT    7
#define VTK_LONG            8
#define VTK_UNSIGNED_LONG   9
#define VTK_FLOAT          10
#define VTK_DOUBLE         11 

// Some constant required for correct template performance
#define VTK_BIT_MIN 0
#define VTK_BIT_MAX 1
#define VTK_CHAR_MIN -128
#define VTK_CHAR_MAX 127
#define VTK_UNSIGNED_CHAR_MIN 0
#define VTK_UNSIGNED_CHAR_MAX 255
#define VTK_SHORT_MIN -32768
#define VTK_SHORT_MAX 32767
#define VTK_UNSIGNED_SHORT_MIN 0
#define VTK_UNSIGNED_SHORT_MAX 65535
#define VTK_INT_MIN (-VTK_LARGE_INTEGER-1)
#define VTK_INT_MAX VTK_LARGE_INTEGER
#define VTK_UNSIGNED_INT_MIN 0
#define VTK_UNSIGNED_INT_MAX 4294967295
#define VTK_LONG_MIN (-VTK_LARGE_INTEGER-1)
#define VTK_LONG_MAX VTK_LARGE_INTEGER
#define VTK_UNSIGNED_LONG_MIN 0
#define VTK_UNSIGNED_LONG_MAX 4294967295
#define VTK_FLOAT_MIN -VTK_LARGE_FLOAT
#define VTK_FLOAT_MAX VTK_LARGE_FLOAT
#define VTK_DOUBLE_MIN -1.0e+99L
#define VTK_DOUBLE_MAX  1.0e+99L

// These types are returned to distinguish dataset types
#define VTK_POLY_DATA          0
#define VTK_STRUCTURED_POINTS  1
#define VTK_STRUCTURED_GRID    2
#define VTK_RECTILINEAR_GRID   3
#define VTK_UNSTRUCTURED_GRID  4

// A macro to get the name of a type
#define vtkImageScalarTypeNameMacro(type) \
(((type) == VTK_VOID) ? "void" : \
(((type) == VTK_FLOAT) ? "float" : \
(((type) == VTK_INT) ? "int" : \
(((type) == VTK_SHORT) ? "short" : \
(((type) == VTK_UNSIGNED_SHORT) ? "unsigned short" : \
(((type) == VTK_UNSIGNED_CHAR) ? "unsigned char" : \
"Undefined"))))))
  
  
//
// Set built-in type.  Creates member Set"name"() (e.g., SetVisibility());
//
#define vtkSetMacro(name,type) \
void Set##name (type _arg) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " #name " to " << _arg); \
  if (this->name != _arg) \
    { \
    this->name = _arg; \
    this->Modified(); \
    } \
  } 

//
// Get built-in type.  Creates member Get"name"() (e.g., GetVisibility());
//
#define vtkGetMacro(name,type) \
type Get##name () { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " of " << this->name ); \
  return this->name; \
  } 

//
// Set character string.  Creates member Set"name"() 
// (e.g., SetFilename(char *));
//
#define vtkSetStringMacro(name) \
void Set##name (char* _arg) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to " << _arg ); \
  if ( this->name && _arg && (!strcmp(this->name,_arg))) { return;} \
  if (this->name) { delete [] this->name; } \
  if (_arg) \
    { \
    this->name = new char[strlen(_arg)+1]; \
    strcpy(this->name,_arg); \
    } \
   else \
    { \
    this->name = NULL; \
    } \
  this->Modified(); \
  } 

//
// Get character string.  Creates member Get"name"() 
// (e.g., char *GetFilename());
//
#define vtkGetStringMacro(name) \
char* Get##name () { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " of " << this->name); \
  return this->name; \
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
  if (this->name != (_arg<min?min:(_arg>max?max:_arg))) \
    { \
    this->name = (_arg<min?min:(_arg>max?max:_arg)); \
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
  if (this->name != _arg) \
    { \
    this->name = _arg; \
    this->Modified(); \
    } \
  } \
void Set##name (type& _arg) \
  { \
  this->Set##name (&_arg);\
  } 

//
// Set pointer to object; uses vtkObject reference counting methodology.
// Creates method Set"name"() (e.g., SetPoints()).
//
#define vtkSetReferenceCountedObjectMacro(name,type) \
void Set##name (type* _arg) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to " << _arg ); \
  if (this->name != _arg) \
    { \
    if (this->name != NULL) { this->name->UnRegister(this); }\
    this->name = _arg; \
    if (this->name != NULL) { this->name->Register(this); } \
    this->Modified(); \
    } \
  } 

//
// Get pointer to object.  Creates member Get"name" (e.g., GetPoints()).
//
#define vtkGetObjectMacro(name,type) \
type *Get##name () \
  { \
      vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " #name " address " << this->name ); \
  return this->name; \
  } 

//
// Create members "name"On() and "name"Off() (e.g., DebugOn() DebugOff()).
// Set method must be defined to use this macro.
//
#define vtkBooleanMacro(name,type) \
void name##On () { this->Set##name((type)1);}; \
void name##Off () { this->Set##name((type)0);}

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
  if ((this->name[0] != _arg1)||(this->name[1] != _arg2)) \
    { \
    this->Modified(); \
    this->name[0] = _arg1; \
    this->name[1] = _arg2; \
    } \
  }; \
void Set##name (type _arg[2]) \
  { \
  this->Set##name (_arg[0], _arg[1]); \
  } 

#define vtkGetVector2Macro(name,type) \
type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << this->name); \
  return this->name; \
} \
void Get##name (type &_arg1, type &_arg2) \
  { \
    _arg1 = this->name[0]; \
    _arg2 = this->name[1]; \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " = (" << _arg1 << "," << _arg2 << ")"); \
  }; \
void Get##name (type _arg[2]) \
  { \
  this->Get##name (_arg[0], _arg[1]);\
  } 

#define vtkSetVector3Macro(name,type) \
void Set##name (type _arg1, type _arg2, type _arg3) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << ")"); \
  if ((this->name[0] != _arg1)||(this->name[1] != _arg2)||(this->name[2] != _arg3)) \
    { \
    this->Modified(); \
    this->name[0] = _arg1; \
    this->name[1] = _arg2; \
    this->name[2] = _arg3; \
    } \
  }; \
void Set##name (type _arg[3]) \
  { \
  this->Set##name (_arg[0], _arg[1], _arg[2]);\
  } 

#define vtkGetVector3Macro(name,type) \
type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << this->name); \
  return this->name; \
} \
void Get##name (type &_arg1, type &_arg2, type &_arg3) \
  { \
    _arg1 = this->name[0]; \
    _arg2 = this->name[1]; \
    _arg3 = this->name[2]; \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " = (" << _arg1 << "," << _arg2 << "," << _arg3 << ")"); \
  }; \
void Get##name (type _arg[3]) \
  { \
  this->Get##name (_arg[0], _arg[1], _arg[2]);\
  } 

#define vtkSetVector4Macro(name,type) \
void Set##name (type _arg1, type _arg2, type _arg3, type _arg4) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << ")"); \
  if ((this->name[0] != _arg1)||(this->name[1] != _arg2)||(this->name[2] != _arg3)||(this->name[3] != _arg4)) \
    { \
    this->Modified(); \
    this->name[0] = _arg1; \
    this->name[1] = _arg2; \
    this->name[2] = _arg3; \
    this->name[3] = _arg4; \
    } \
  }; \
void Set##name (type _arg[4]) \
  { \
  this->Set##name (_arg[0], _arg[1], _arg[2], _arg[3]);\
  } 


#define vtkGetVector4Macro(name,type) \
type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << this->name); \
  return this->name; \
} \
void Get##name (type &_arg1, type &_arg2, type &_arg3, type &_arg4) \
  { \
    _arg1 = this->name[0]; \
    _arg2 = this->name[1]; \
    _arg3 = this->name[2]; \
    _arg4 = this->name[3]; \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " = (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << ")"); \
  }; \
void Get##name (type _arg[4]) \
  { \
  this->Get##name (_arg[0], _arg[1], _arg[2], _arg[3]);\
  } 

#define vtkSetVector6Macro(name,type) \
void Set##name (type _arg1, type _arg2, type _arg3, type _arg4, type _arg5, type _arg6) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << "," << _arg5 << "," << _arg6 << ")"); \
  if ((this->name[0] != _arg1)||(this->name[1] != _arg2)||(this->name[2] != _arg3)||(this->name[3] != _arg4)||(this->name[4] != _arg5)||(this->name[5] != _arg6)) \
    { \
    this->Modified(); \
    this->name[0] = _arg1; \
    this->name[1] = _arg2; \
    this->name[2] = _arg3; \
    this->name[3] = _arg4; \
    this->name[4] = _arg5; \
    this->name[5] = _arg6; \
    } \
  }; \
void Set##name (type _arg[6]) \
  { \
  this->Set##name (_arg[0], _arg[1], _arg[2], _arg[3], _arg[4], _arg[5]);\
  } 

#define vtkGetVector6Macro(name,type) \
type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << this->name); \
  return this->name; \
} \
void Get##name (type &_arg1, type &_arg2, type &_arg3, type &_arg4, type &_arg5, type &_arg6) \
  { \
    _arg1 = this->name[0]; \
    _arg2 = this->name[1]; \
    _arg3 = this->name[2]; \
    _arg4 = this->name[3]; \
    _arg5 = this->name[4]; \
    _arg6 = this->name[5]; \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " = (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << "," << _arg5 <<"," << _arg6 << ")"); \
  }; \
void Get##name (type _arg[6]) \
  { \
  this->Get##name (_arg[0], _arg[1], _arg[2], _arg[3], _arg[4], _arg[5]);\
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
  for (i=0; i<count; i++) { if ( data[i] != this->name[i] ) { break; }} \
  if ( i < count ) \
    { \
    this->Modified(); \
    for (i=0; i<count; i++) { this->name[i] = data[i]; }\
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
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << this->name); \
  return this->name; \
} \
void Get##name (type data[count]) \
{ \
  for (int i=0; i<count; i++) { data[i] = this->name[i]; }\
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
      if (MessageBox(NULL,vtkmsgbuff,"Debug Info",MB_ICONINFORMATION | MB_OKCANCEL) == IDCANCEL) { vtkObject::GlobalWarningDisplayOff(); }\
      vtkmsg.rdbuf()->freeze(0);}}

//
// This macro is used for  debug statements in instance methods
// vtkDebugMacro(<< "this is debug info" << this->SomeVariable);
//
#ifdef VTK_LEAN_AND_MEAN
#define vtkDebugMacro(x)
#else
#define vtkDebugMacro(x) \
{ if (this->Debug && vtkObject::GetGlobalWarningDisplay()) \
    { char *vtkmsgbuff; ostrstream vtkmsg; \
      vtkmsg << "Debug: In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x << "\nPress Cancel to supress any further messages." << ends; \
      vtkmsgbuff = vtkmsg.str(); \
      if (MessageBox(NULL,vtkmsgbuff,"Debug Info",MB_ICONINFORMATION | MB_OKCANCEL) == IDCANCEL) { vtkObject::GlobalWarningDisplayOff(); } \
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
      if (MessageBox(NULL,vtkmsgbuff,"Warning",MB_ICONWARNING | MB_OKCANCEL) == IDCANCEL) { vtkObject::GlobalWarningDisplayOff(); } \
      vtkmsg.rdbuf()->freeze(0);}}

//
// This macro is used to print out errors
// vtkErrorMacro(<< "Error message" << variable);
//
#define vtkErrorMacro(x) \
{ if (vtkObject::GetGlobalWarningDisplay()) {char *vtkmsgbuff; ostrstream vtkmsg; \
      vtkmsg << "ERROR: In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x << "\nPress Cancel to supress any further messages." << ends; \
      vtkmsgbuff = vtkmsg.str(); \
      if (MessageBox(NULL,vtkmsgbuff,"Error",MB_ICONERROR | MB_OKCANCEL) == IDCANCEL) { vtkObject::GlobalWarningDisplayOff(); } \
      vtkmsg.rdbuf()->freeze(0); vtkObject::BreakOnError();}}

#else
//
// This macro is used for any output that may not be in an instance method
// vtkGenericWarningMacro(<< "this is debug info" << this->SomeVariable);
//
#define vtkGenericWarningMacro(x) \
if (vtkObject::GetGlobalWarningDisplay()) { cerr << "Generic Warning: In " __FILE__ ", line " << __LINE__ << "\n  " x << "\n\n";}



//
// This macro is used for  debug statements in instance methods
// vtkDebugMacro(<< "this is debug info" << this->SomeVariable);
//
#ifdef VTK_LEAN_AND_MEAN
#define vtkDebugMacro(x)
#else
#define vtkDebugMacro(x) \
if (this->Debug && vtkObject::GetGlobalWarningDisplay()) { cerr << "Debug: In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x <<  "\n\n";};


#endif
//
// This macro is used to print out warning messages.
// vtkWarningMacro(<< "Warning message" << variable);
//
#define vtkWarningMacro(x) \
if (vtkObject::GetGlobalWarningDisplay()) { cerr << "Warning: In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x << "\n\n";};


//
// This macro is used to print out errors
// vtkErrorMacro(<< "Error message" << variable);
//
#define vtkErrorMacro(x) \
{ if (vtkObject::GetGlobalWarningDisplay()) { cerr << "ERROR In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x << "\n\n";} vtkObject::BreakOnError();}
#endif

//
// This macro is used to quiet compiler warnings about unused parameters
// to methods. Only use it when the parameter really shouldn't be used.
// Don't use it as a way to shut up the compiler while you take your
// sweet time getting around to implementing the method.
//
#define vtkNotUsed(x)

#define vtkWorldCoordinateMacro(name) \
vtkCoordinate *Get##name##Coordinate () \
{ \
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " #name "Coordinate address " << this->name##Coordinate ); \
    return this->name##Coordinate; \
} \
void Set##name(float x[3]) {this->Set##name(x[0],x[1],x[2]);}; \
void Set##name(float x, float y, float z) \
{ \
    this->name##Coordinate->SetCoordinateSystem(VTK_WORLD); \
    this->name##Coordinate->SetValue(x,y,z); \
} \
float *Get##name() \
{ \
    return this->name##Coordinate->GetValue(); \
}

#define vtkViewportCoordinateMacro(name) \
vtkCoordinate *Get##name##Coordinate () \
{ \
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " #name "Coordinate address " << this->name##Coordinate ); \
    return this->name##Coordinate; \
} \
void Set##name(float x[2]) {this->Set##name(x[0],x[1]);}; \
void Set##name(float x, float y) \
{ \
    this->name##Coordinate->SetCoordinateSystem(VTK_VIEWPORT); \
    this->name##Coordinate->SetValue(x,y); \
} \
float *Get##name() \
{ \
    return this->name##Coordinate->GetValue(); \
}

#endif
