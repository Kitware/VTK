/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSetGet.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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

#include "vtkSystemIncludes.h"
#include <math.h>

// Some constants used throughout the code
#define VTK_LARGE_FLOAT 1.0e+38F
#ifdef VTK_USE_64BIT_IDS
#ifdef _WIN32
#define VTK_LARGE_ID 9223372036854775807i64 // 2^63 - 1
#else
#define VTK_LARGE_ID 9223372036854775807LL // 2^63 - 1
#endif
#else
#define VTK_LARGE_ID 2147483647 // 2^31 - 1
#endif

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
#define VTK_ID_TYPE        12

// These types are not currently supported by GetDataType, but are 
// for completeness.
#define VTK_STRING         13
#define VTK_OPAQUE         14

// Some constant required for correct template performance
#define VTK_BIT_MIN            0
#define VTK_BIT_MAX            1
#define VTK_CHAR_MIN          -128
#define VTK_CHAR_MAX           127
#define VTK_UNSIGNED_CHAR_MIN  0
#define VTK_UNSIGNED_CHAR_MAX  255
#define VTK_SHORT_MIN         -32768
#define VTK_SHORT_MAX          32767
#define VTK_UNSIGNED_SHORT_MIN 0
#define VTK_UNSIGNED_SHORT_MAX 65535
#define VTK_INT_MIN          (-VTK_LARGE_INTEGER-1)
#define VTK_INT_MAX            VTK_LARGE_INTEGER
#define VTK_UNSIGNED_INT_MIN   0
#define VTK_UNSIGNED_INT_MAX   4294967295UL
#define VTK_LONG_MIN         (-VTK_LARGE_INTEGER-1)
#define VTK_LONG_MAX           VTK_LARGE_INTEGER
#define VTK_UNSIGNED_LONG_MIN  0
#define VTK_UNSIGNED_LONG_MAX  4294967295UL
#define VTK_FLOAT_MIN         -VTK_LARGE_FLOAT
#define VTK_FLOAT_MAX          VTK_LARGE_FLOAT
#define VTK_DOUBLE_MIN        -1.0e+99L
#define VTK_DOUBLE_MAX         1.0e+99L

// These types are returned to distinguish data object types
#define VTK_POLY_DATA          0
#define VTK_STRUCTURED_POINTS  1
#define VTK_STRUCTURED_GRID    2
#define VTK_RECTILINEAR_GRID   3
#define VTK_UNSTRUCTURED_GRID  4
#define VTK_PIECEWISE_FUNCTION 5
#define VTK_IMAGE_DATA         6
#define VTK_DATA_OBJECT        7
#define VTK_DATA_SET           8

// These types define error codes for vtk functions
#define VTK_OK                 1
#define VTK_ERROR              2

// These types define different text properties
#define VTK_ARIAL     0
#define VTK_COURIER   1
#define VTK_TIMES     2

#define VTK_TEXT_LEFT     0
#define VTK_TEXT_CENTERED 1
#define VTK_TEXT_RIGHT    2

#define VTK_TEXT_BOTTOM 0
#define VTK_TEXT_TOP    2

// 

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
virtual void Set##name (type _arg) \
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
virtual type Get##name () { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " of " << this->name ); \
  return this->name; \
  } 


//
// Set character string.  Creates member Set"name"() 
// (e.g., SetFilename(char *));
//
#define vtkSetStringMacro(name) \
virtual void Set##name (const char* _arg) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to " << _arg ); \
  if ( this->name == NULL && _arg == NULL) { return;} \
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
virtual char* Get##name () { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " of " << this->name); \
  return this->name; \
  } 

//
// Set built-in type where value is constrained between min/max limits.
// Create member Set"name"() (eg., SetRadius()). #defines are 
// convenience for clamping open-ended values.
// The Get"name"MinValue() and Get"name"MaxValue() members return the
// min and max limits.
//
#define vtkSetClampMacro(name,type,min,max) \
virtual void Set##name (type _arg) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to " << _arg ); \
  if (this->name != (_arg<min?min:(_arg>max?max:_arg))) \
    { \
    this->name = (_arg<min?min:(_arg>max?max:_arg)); \
    this->Modified(); \
    } \
  } \
virtual type Get##name##MinValue () \
  { \
  return min; \
  } \
virtual type Get##name##MaxValue () \
  { \
  return max; \
  }

//
// This macro defines a body of set object macro. It can be used either in 
// the header file vtkSetObjectMacro or in the implementation one
// vtkSetObjectMacro. It sets the pointer to object; uses vtkObject 
// reference counting methodology. Creates method 
// Set"name"() (e.g., SetPoints()).
//
#define vtkSetObjectBodyMacro(name,type,args)                   \
  {                                                             \
  vtkDebugMacro(<< this->GetClassName() << " (" << this         \
                << "): setting " << #name " to " << args );     \
  if (this->name != args)                                       \
    {                                                           \
    if (this->name != NULL) { this->name->UnRegister(this); }   \
    this->name = args;                                          \
    if (this->name != NULL) { this->name->Register(this); }     \
    this->Modified();                                           \
    }                                                           \
  }

//
// Set pointer to object; uses vtkObject reference counting methodology.
// Creates method Set"name"() (e.g., SetPoints()). This macro should
// be used in the header file.
//
#define vtkSetObjectMacro(name,type)            \
virtual void Set##name (type* _arg)             \
  {                                             \
  vtkSetObjectBodyMacro(name,type,_arg);        \
  }

//
// Set pointer to object; uses vtkObject reference counting methodology.
// Creates method Set"name"() (e.g., SetPoints()). This macro should
// be used in the implementation file. You will also have to write
// prototype in the header file. The prototype should look like this:
// virtual void Set"name"("type" *);
//
// Please use vtkCxxSetObjectMacro not vtkSetObjectImplementationMacro.
// The first one is just for people who already used it.
#define vtkSetObjectImplementationMacro(class,name,type)        \
  vtkCxxSetObjectMacro(class,name,type)

#define vtkCxxSetObjectMacro(class,name,type)   \
void class::Set##name (type* _arg)              \
  {                                             \
  vtkSetObjectBodyMacro(name,type,_arg);        \
  }

//
// Get pointer to object.  Creates member Get"name" (e.g., GetPoints()).
// This macro should be used in the header file.
//
#define vtkGetObjectMacro(name,type)                                    \
virtual type *Get##name ()                                              \
  {                                                                     \
  vtkDebugMacro(<< this->GetClassName() << " (" << this                 \
                << "): returning " #name " address " << this->name );   \
  return this->name;                                                    \
  }

//
// Create members "name"On() and "name"Off() (e.g., DebugOn() DebugOff()).
// Set method must be defined to use this macro.
//
#define vtkBooleanMacro(name,type) \
virtual void name##On () { this->Set##name((type)1);}; \
virtual void name##Off () { this->Set##name((type)0);}

//
// Following set macros for vectors define two members for each macro.  The first 
// allows setting of individual components (e.g, SetColor(float,float,float)), 
// the second allows setting from an array (e.g., SetColor(float* rgb[3])).
// The macros vary in the size of the vector they deal with.
//
#define vtkSetVector2Macro(name,type) \
virtual void Set##name (type _arg1, type _arg2) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to (" << _arg1 << "," << _arg2 << ")"); \
  if ((this->name[0] != _arg1)||(this->name[1] != _arg2)) \
    { \
    this->name[0] = _arg1; \
    this->name[1] = _arg2; \
    this->Modified(); \
    } \
  }; \
void Set##name (type _arg[2]) \
  { \
  this->Set##name (_arg[0], _arg[1]); \
  } 

#define vtkGetVector2Macro(name,type) \
virtual type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << this->name); \
  return this->name; \
} \
virtual void Get##name (type &_arg1, type &_arg2) \
  { \
    _arg1 = this->name[0]; \
    _arg2 = this->name[1]; \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " = (" << _arg1 << "," << _arg2 << ")"); \
  }; \
virtual void Get##name (type _arg[2]) \
  { \
  this->Get##name (_arg[0], _arg[1]);\
  } 

#define vtkSetVector3Macro(name,type) \
virtual void Set##name (type _arg1, type _arg2, type _arg3) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << ")"); \
  if ((this->name[0] != _arg1)||(this->name[1] != _arg2)||(this->name[2] != _arg3)) \
    { \
    this->name[0] = _arg1; \
    this->name[1] = _arg2; \
    this->name[2] = _arg3; \
    this->Modified(); \
    } \
  }; \
virtual void Set##name (type _arg[3]) \
  { \
  this->Set##name (_arg[0], _arg[1], _arg[2]);\
  } 

#define vtkGetVector3Macro(name,type) \
virtual type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << this->name); \
  return this->name; \
} \
virtual void Get##name (type &_arg1, type &_arg2, type &_arg3) \
  { \
    _arg1 = this->name[0]; \
    _arg2 = this->name[1]; \
    _arg3 = this->name[2]; \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " = (" << _arg1 << "," << _arg2 << "," << _arg3 << ")"); \
  }; \
virtual void Get##name (type _arg[3]) \
  { \
  this->Get##name (_arg[0], _arg[1], _arg[2]);\
  } 

#define vtkSetVector4Macro(name,type) \
virtual void Set##name (type _arg1, type _arg2, type _arg3, type _arg4) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << ")"); \
  if ((this->name[0] != _arg1)||(this->name[1] != _arg2)||(this->name[2] != _arg3)||(this->name[3] != _arg4)) \
    { \
    this->name[0] = _arg1; \
    this->name[1] = _arg2; \
    this->name[2] = _arg3; \
    this->name[3] = _arg4; \
    this->Modified(); \
    } \
  }; \
virtual void Set##name (type _arg[4]) \
  { \
  this->Set##name (_arg[0], _arg[1], _arg[2], _arg[3]);\
  } 


#define vtkGetVector4Macro(name,type) \
virtual type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << this->name); \
  return this->name; \
} \
virtual void Get##name (type &_arg1, type &_arg2, type &_arg3, type &_arg4) \
  { \
    _arg1 = this->name[0]; \
    _arg2 = this->name[1]; \
    _arg3 = this->name[2]; \
    _arg4 = this->name[3]; \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " = (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << ")"); \
  }; \
virtual void Get##name (type _arg[4]) \
  { \
  this->Get##name (_arg[0], _arg[1], _arg[2], _arg[3]);\
  } 

#define vtkSetVector6Macro(name,type) \
virtual void Set##name (type _arg1, type _arg2, type _arg3, type _arg4, type _arg5, type _arg6) \
  { \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << "," << _arg5 << "," << _arg6 << ")"); \
  if ((this->name[0] != _arg1)||(this->name[1] != _arg2)||(this->name[2] != _arg3)||(this->name[3] != _arg4)||(this->name[4] != _arg5)||(this->name[5] != _arg6)) \
    { \
    this->name[0] = _arg1; \
    this->name[1] = _arg2; \
    this->name[2] = _arg3; \
    this->name[3] = _arg4; \
    this->name[4] = _arg5; \
    this->name[5] = _arg6; \
    this->Modified(); \
    } \
  }; \
virtual void Set##name (type _arg[6]) \
  { \
  this->Set##name (_arg[0], _arg[1], _arg[2], _arg[3], _arg[4], _arg[5]);\
  } 

#define vtkGetVector6Macro(name,type) \
virtual type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << this->name); \
  return this->name; \
} \
virtual void Get##name (type &_arg1, type &_arg2, type &_arg3, type &_arg4, type &_arg5, type &_arg6) \
  { \
    _arg1 = this->name[0]; \
    _arg2 = this->name[1]; \
    _arg3 = this->name[2]; \
    _arg4 = this->name[3]; \
    _arg5 = this->name[4]; \
    _arg6 = this->name[5]; \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " = (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << "," << _arg5 <<"," << _arg6 << ")"); \
  }; \
virtual void Get##name (type _arg[6]) \
  { \
  this->Get##name (_arg[0], _arg[1], _arg[2], _arg[3], _arg[4], _arg[5]);\
  } 

//
// General set vector macro creates a single method that copies specified
// number of values into object.
// Examples: void SetColor(c,3)
//
#define vtkSetVectorMacro(name,type,count) \
virtual void Set##name(type data[]) \
{ \
  int i; \
  for (i=0; i<count; i++) { if ( data[i] != this->name[i] ) { break; }} \
  if ( i < count ) \
    { \
    for (i=0; i<count; i++) { this->name[i] = data[i]; }\
    this->Modified(); \
    } \
}

//
// Get vector macro defines two methods. One returns pointer to type 
// (i.e., array of type). This is for efficiency. The second copies data
// into user provided array. This is more object-oriented.
// Examples: float *GetColor() and void GetColor(float c[count]).
//
#define vtkGetVectorMacro(name,type,count) \
virtual type *Get##name () \
{ \
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " << #name " pointer " << this->name); \
  return this->name; \
} \
virtual void Get##name (type data[count]) \
{ \
  for (int i=0; i<count; i++) { data[i] = this->name[i]; }\
}

// Use a global function which actually calls:
//  vtkOutputWindow::GetInstance()->DisplayText();
// This is to avoid vtkObject #include of vtkOutputWindow
// while vtkOutputWindow #includes vtkObject

extern VTK_COMMON_EXPORT void vtkOutputWindowDisplayText(const char*);
extern VTK_COMMON_EXPORT void vtkOutputWindowDisplayErrorText(const char*);
extern VTK_COMMON_EXPORT void vtkOutputWindowDisplayWarningText(const char*);
extern VTK_COMMON_EXPORT void vtkOutputWindowDisplayGenericWarningText(const char*);
extern VTK_COMMON_EXPORT void vtkOutputWindowDisplayDebugText(const char*);

//
// This macro is used for any output that may not be in an instance method
// vtkGenericWarningMacro(<< "this is debug info" << this->SomeVariable);
//
#define vtkGenericWarningMacro(x) \
{ if (vtkObject::GetGlobalWarningDisplay()) { \
      char *vtkmsgbuff; ostrstream vtkmsg; \
      vtkmsg << "Generic Warning: In " __FILE__ ", line " << __LINE__ << "\n" x \
      << "\n\n" << ends; \
      vtkmsgbuff = vtkmsg.str(); \
      vtkOutputWindowDisplayGenericWarningText(vtkmsgbuff);\
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
    { char *vtkmsgbuff; \
      ostrstream vtkmsg; \
      vtkmsg << "Debug: In " __FILE__ ", line " << __LINE__ << "\n" << this->GetClassName() << " (" << this << "): " x  << "\n\n" << ends; \
      vtkmsgbuff = vtkmsg.str(); \
      vtkOutputWindowDisplayDebugText(vtkmsgbuff);\
      vtkmsg.rdbuf()->freeze(0);}}
#endif
//
// This macro is used to print out warning messages.
// vtkWarningMacro(<< "Warning message" << variable);
//
#define vtkWarningMacro(x)                      \
   vtkWarningWithObjectMacro(this,x)

//
// This macro is used to print out errors
// vtkErrorMacro(<< "Error message" << variable);
//
#define vtkErrorMacro(x)                        \
   vtkErrorWithObjectMacro(this,x)

//
// This macro is used to print out errors
// vtkErrorWithObjectMacro(self, << "Error message" << variable);
//
#define vtkErrorWithObjectMacro(self, x)                        \
   {                                                            \
   if (vtkObject::GetGlobalWarningDisplay())                    \
     {                                                          \
     char *vtkmsgbuff;                                          \
     ostrstream vtkmsg;                                         \
     vtkmsg << "ERROR: In " __FILE__ ", line " << __LINE__      \
            << "\n" << self->GetClassName() << " (" << self     \
            << "): " x << "\n\n" << ends;                       \
     vtkmsgbuff = vtkmsg.str();                                 \
     if ( self->HasObserver("ErrorEvent") )                     \
       {                                                        \
       self->InvokeEvent("ErrorEvent", vtkmsg.str());           \
       }                                                        \
     else                                                       \
       {                                                        \
       vtkOutputWindowDisplayErrorText(vtkmsgbuff);             \
       }                                                        \
     vtkmsg.rdbuf()->freeze(0); vtkObject::BreakOnError();      \
     }                                                          \
   }

//
// This macro is used to print out warnings
// vtkWarningWithObjectMacro(self, "Warning message" << variable);
//
#define vtkWarningWithObjectMacro(self, x)                      \
   {                                                            \
   if (vtkObject::GetGlobalWarningDisplay())                    \
     {                                                          \
     char *vtkmsgbuff;                                          \
     ostrstream vtkmsg;                                         \
     vtkmsg << "Warning: In " __FILE__ ", line " << __LINE__    \
            << "\n" << self->GetClassName() << " (" << self     \
            << "): " x << "\n\n" << ends;                       \
     vtkmsgbuff = vtkmsg.str();                                 \
     if ( self->HasObserver("WarningEvent") )                   \
       {                                                        \
       self->InvokeEvent("WarningEvent", vtkmsg.str());         \
       }                                                        \
     else                                                       \
       {                                                        \
       vtkOutputWindowDisplayWarningText(vtkmsgbuff);           \
       }                                                        \
     vtkmsg.rdbuf()->freeze(0);                                 \
     }                                                          \
   }



//
// This macro is used to quiet compiler warnings about unused parameters
// to methods. Only use it when the parameter really shouldn't be used.
// Don't use it as a way to shut up the compiler while you take your
// sweet time getting around to implementing the method.
//
#define vtkNotUsed(x)

#define vtkWorldCoordinateMacro(name) \
virtual vtkCoordinate *Get##name##Coordinate () \
{ \
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " #name "Coordinate address " << this->name##Coordinate ); \
    return this->name##Coordinate; \
} \
virtual void Set##name(float x[3]) {this->Set##name(x[0],x[1],x[2]);}; \
virtual void Set##name(float x, float y, float z) \
{ \
    this->name##Coordinate->SetValue(x,y,z); \
} \
virtual float *Get##name() \
{ \
    return this->name##Coordinate->GetValue(); \
}

#define vtkViewportCoordinateMacro(name) \
virtual vtkCoordinate *Get##name##Coordinate () \
{ \
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning " #name "Coordinate address " << this->name##Coordinate ); \
    return this->name##Coordinate; \
} \
virtual void Set##name(float x[2]) {this->Set##name(x[0],x[1]);}; \
virtual void Set##name(float x, float y) \
{ \
    this->name##Coordinate->SetValue(x,y); \
} \
virtual float *Get##name() \
{ \
    return this->name##Coordinate->GetValue(); \
}

// Macro used to determine whether a class is the same class or
// a subclass of the named class.
//
#define vtkTypeMacro(thisClass,superclass) \
  typedef superclass Superclass; \
  virtual const char *GetClassName() const {return #thisClass;} \
  static int IsTypeOf(const char *type) \
  { \
    if ( !strcmp(#thisClass,type) ) \
      { \
      return 1; \
      } \
    return superclass::IsTypeOf(type); \
  } \
  virtual int IsA(const char *type) \
  { \
    return this->thisClass::IsTypeOf(type); \
  } \
  static thisClass* SafeDownCast(vtkObject *o) \
  { \
    if ( o && o->IsA(#thisClass) ) \
      { \
      return static_cast<thisClass *>(o); \
      } \
    return NULL;\
  } \
  protected: \
  virtual vtkObject *NewInstanceInternal() const \
  { \
    return thisClass::New(); \
  } \
  public: \
  thisClass *NewInstance() const \
  { \
    return thisClass::SafeDownCast(this->NewInstanceInternal()); \
  }

// Version of vtkTypeMacro that adds the CollectRevisions method.
#define vtkTypeRevisionMacro(thisClass,superclass) \
  protected: \
  void CollectRevisions(ostream& os); \
  public: \
  vtkTypeMacro(thisClass,superclass)

// Macro to implement the standard CollectRevisions method.
#define vtkCxxRevisionMacro(thisClass, revision) \
  void thisClass::CollectRevisions(ostream& os) \
  { \
    this->Superclass::CollectRevisions(os); \
    os << #thisClass " " revision "\n"; \
  }

// Macro to implement the standard form of the New() method.
#define vtkStandardNewMacro(thisClass) \
  thisClass* thisClass::New() \
  { \
    vtkObject* ret = vtkObjectFactory::CreateInstance(#thisClass); \
    if(ret) \
      { \
      return static_cast<thisClass*>(ret); \
      } \
    return new thisClass; \
  }

// The following macros are all just there to centralize the template 
// switch code so that every execute method doesn't have to have the same
// long list if case statements

#define vtkTemplateMacro3(func, arg1, arg2, arg3) \
      case VTK_DOUBLE: \
        { typedef double VTK_TT; \
        func(arg1, arg2, arg3); } \
        break; \
      case VTK_FLOAT: \
        { typedef float VTK_TT; \
        func(arg1, arg2, arg3); } \
        break; \
      case VTK_LONG: \
        { typedef long VTK_TT; \
        func(arg1, arg2, arg3); } \
        break; \
      case VTK_UNSIGNED_LONG: \
        { typedef unsigned long VTK_TT; \
        func(arg1, arg2, arg3); } \
        break; \
      case VTK_INT: \
        { typedef int VTK_TT; \
        func(arg1, arg2, arg3); } \
        break; \
      case VTK_UNSIGNED_INT: \
        { typedef unsigned int VTK_TT; \
        func(arg1, arg2, arg3); } \
        break; \
      case VTK_SHORT: \
        { typedef short VTK_TT; \
        func(arg1, arg2, arg3); } \
        break; \
      case VTK_UNSIGNED_SHORT: \
        { typedef unsigned short VTK_TT; \
        func(arg1, arg2, arg3); } \
        break; \
      case VTK_CHAR: \
        { typedef char VTK_TT; \
        func(arg1, arg2, arg3); } \
        break; \
      case VTK_UNSIGNED_CHAR: \
        { typedef unsigned char VTK_TT; \
        func(arg1, arg2, arg3); } \
        break

#define vtkTemplateMacro4(func, arg1, arg2, arg3, arg4) \
      case VTK_DOUBLE: \
        { typedef double VTK_TT; \
        func(arg1, arg2, arg3, arg4); } \
        break; \
      case VTK_FLOAT: \
        { typedef float VTK_TT; \
        func(arg1, arg2, arg3, arg4); } \
        break; \
      case VTK_LONG: \
        { typedef long VTK_TT; \
        func(arg1, arg2, arg3, arg4); } \
        break; \
      case VTK_UNSIGNED_LONG: \
        { typedef unsigned long VTK_TT; \
        func(arg1, arg2, arg3, arg4); } \
        break; \
      case VTK_INT: \
        { typedef int VTK_TT; \
        func(arg1, arg2, arg3, arg4); } \
        break; \
      case VTK_UNSIGNED_INT: \
        { typedef unsigned int VTK_TT; \
        func(arg1, arg2, arg3, arg4); } \
        break; \
      case VTK_SHORT: \
        { typedef short VTK_TT; \
        func(arg1, arg2, arg3, arg4); } \
        break; \
      case VTK_UNSIGNED_SHORT: \
        { typedef unsigned short VTK_TT; \
        func(arg1, arg2, arg3, arg4); } \
        break; \
      case VTK_CHAR: \
        { typedef char VTK_TT; \
        func(arg1, arg2, arg3, arg4); } \
        break; \
      case VTK_UNSIGNED_CHAR: \
        { typedef unsigned char VTK_TT; \
        func(arg1, arg2, arg3, arg4); } \
        break

#define vtkTemplateMacro5(func, arg1, arg2, arg3, arg4, arg5) \
      case VTK_DOUBLE: \
        { typedef double VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5); } \
        break; \
      case VTK_FLOAT: \
        { typedef float VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5); } \
        break; \
      case VTK_LONG: \
        { typedef long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5); } \
        break; \
      case VTK_UNSIGNED_LONG: \
        { typedef unsigned long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5); } \
        break; \
      case VTK_INT: \
        { typedef int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5); } \
        break; \
      case VTK_UNSIGNED_INT: \
        { typedef unsigned int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5); } \
        break; \
      case VTK_SHORT: \
        { typedef short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5); } \
        break; \
      case VTK_UNSIGNED_SHORT: \
        { typedef unsigned short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5); } \
        break; \
      case VTK_CHAR: \
        { typedef char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5); } \
        break; \
      case VTK_UNSIGNED_CHAR: \
        { typedef unsigned char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5); } \
        break

#define vtkTemplateMacro6(func, arg1, arg2, arg3, arg4, arg5, arg6) \
      case VTK_DOUBLE: \
        { typedef double VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6); } \
        break; \
      case VTK_FLOAT: \
        { typedef float VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6); } \
        break; \
      case VTK_LONG: \
        { typedef long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6); } \
        break; \
      case VTK_UNSIGNED_LONG: \
        { typedef unsigned long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6); } \
        break; \
      case VTK_INT: \
        { typedef int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6); } \
        break; \
      case VTK_UNSIGNED_INT: \
        { typedef unsigned int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6); } \
        break; \
      case VTK_SHORT: \
        { typedef short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6); } \
        break; \
      case VTK_UNSIGNED_SHORT: \
        { typedef unsigned short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6); } \
        break; \
      case VTK_CHAR: \
        { typedef char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6); } \
        break; \
      case VTK_UNSIGNED_CHAR: \
        { typedef unsigned char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6); } \
        break
          
#define vtkTemplateMacro7(func, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
      case VTK_DOUBLE: \
        { typedef double VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7); } \
        break; \
      case VTK_FLOAT: \
        { typedef float VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7); } \
        break; \
      case VTK_LONG: \
        { typedef long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7); } \
        break; \
      case VTK_UNSIGNED_LONG: \
        { typedef unsigned long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7); } \
        break; \
      case VTK_INT: \
        { typedef int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7); } \
        break; \
      case VTK_UNSIGNED_INT: \
        { typedef unsigned int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7); } \
        break; \
      case VTK_SHORT: \
        { typedef short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7); } \
        break; \
      case VTK_UNSIGNED_SHORT: \
        { typedef unsigned short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7); } \
        break; \
      case VTK_CHAR: \
        { typedef char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7); } \
        break; \
      case VTK_UNSIGNED_CHAR: \
        { typedef unsigned char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7); } \
        break
          
#define vtkTemplateMacro8(func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
      case VTK_DOUBLE: \
        { typedef double VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); } \
        break; \
      case VTK_FLOAT: \
        { typedef float VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); } \
        break; \
      case VTK_LONG: \
        { typedef long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); } \
        break; \
      case VTK_UNSIGNED_LONG: \
        { typedef unsigned long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); } \
        break; \
      case VTK_INT: \
        { typedef int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); } \
        break; \
      case VTK_UNSIGNED_INT: \
        { typedef unsigned int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); } \
        break; \
      case VTK_SHORT: \
        { typedef short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); } \
        break; \
      case VTK_UNSIGNED_SHORT: \
        { typedef unsigned short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); } \
        break; \
      case VTK_CHAR: \
        { typedef char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); } \
        break; \
      case VTK_UNSIGNED_CHAR: \
        { typedef unsigned char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); } \
        break

#define vtkTemplateMacro9(func,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9) \
      case VTK_DOUBLE: \
        { typedef double VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } \
        break; \
      case VTK_FLOAT: \
        { typedef float VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } \
        break; \
      case VTK_LONG: \
        { typedef long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } \
        break; \
      case VTK_UNSIGNED_LONG: \
        { typedef unsigned long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } \
        break; \
      case VTK_INT: \
        { typedef int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } \
        break; \
      case VTK_UNSIGNED_INT: \
        { typedef unsigned int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } \
        break; \
      case VTK_SHORT: \
        { typedef short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } \
        break; \
      case VTK_UNSIGNED_SHORT: \
        { typedef unsigned short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } \
        break; \
      case VTK_CHAR: \
        { typedef char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } \
        break; \
      case VTK_UNSIGNED_CHAR: \
        { typedef unsigned char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } \
        break

#define vtkTemplateMacro10(func,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10) \
      case VTK_DOUBLE: \
        { typedef double VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); } \
        break; \
      case VTK_FLOAT: \
        { typedef float VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); } \
        break; \
      case VTK_LONG: \
        { typedef long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); } \
        break; \
      case VTK_UNSIGNED_LONG: \
        { typedef unsigned long VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); } \
        break; \
      case VTK_INT: \
        { typedef int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); } \
        break; \
      case VTK_UNSIGNED_INT: \
        { typedef unsigned int VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); } \
        break; \
      case VTK_SHORT: \
        { typedef short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); } \
        break; \
      case VTK_UNSIGNED_SHORT: \
        { typedef unsigned short VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); } \
        break; \
      case VTK_CHAR: \
        { typedef char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); } \
        break; \
      case VTK_UNSIGNED_CHAR: \
        { typedef unsigned char VTK_TT; \
        func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); } \
        break

// Use to mark methods legacy. Make sure the correct date is used to
// keep track of when a method was made legacy, and so that it can be
// eliminated at the right time.
#ifndef VTK_LEAN_AND_MEAN
#define VTK_LEGACY_METHOD(oldMethod,versionStringMadeLegacy) \
  vtkErrorMacro(<< #oldMethod \
                << " was obsoleted for version " << #versionStringMadeLegacy \
                << " and will be removed in a future version");
#else
#define VTK_LEGACY_METHOD(oldMethod,versionStringMadeLegacy)
#endif


#endif

