/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFortran.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * The whole pupose of this header file is to allow us to 'wrap' a c library
 * into Fortram. Here are the basic goal we should reach:
 *
 * 1. The symbol mangling in fortan is different than is C. For example if 
 * you do this fortan:
 *     CALL Foo()
 * you either need to define a symbol:
 *    - void FOO()
 *    - void foo()
 *    - void FOO_()
 *    - void foo_()
 *  
 *  2. Passing array, Fortran start at 1 instead of 0.
 *
 *  3. Passing STRING array from fortran to C is a bit more tricky.
 *
 *  For more info refer to:
 *
 */
#ifndef __vtkFortran_h
#define __vtkFortran_h

#define VTK_FORTRAN_NAME(name, NAME) \
  name##__

#define VTK_FORTRAN_ARG_STRING(__arg) \
  const char *__arg_string, unsigned int __arg_lenght

#define VTK_FORTRAN_REF_STRING_POINTER(__arg) \
  __arg_string

#define VTK_FORTRAN_REF_STRING_LENGTH(__arg) \
  __arg_lenght

#define VTK_FORTRAN_ARG_INTEGER4(data) \
  int data
  
#define VTK_FORTRAN_ARG_REAL4_ARRAY_1D(array) \
  float *array
  
#define VTK_FORTRAN_ARG_INTEGER8(size) \
  unsigned int size

#define VTK_FORTRAN_REF_REAL4_ARRAY_1D(array) \
  array

#define VTK_FORTRAN_REF_INTEGER4(data) \
  data

#define VTK_FORTRAN_REF_INTEGER8(data) \
  data

#define VTK_FORTRAN_ARG_INTEGER4_ARRAY_1D(array) \
  int *array

#define VTK_FORTRAN_ARG_REAL8(t) \
  double t

#define VTK_FORTRAN_REF_REAL8(t) \
  t

#define VTK_FORTRAN_ARG_INT4(n) \
  int n

#define VTK_FORTRAN_REF_INT4(n) \
  n

#if 0
//#define F77_FUNC(vtkXML_Initialize, vtkxml_initialize, VTKXML_INITIALIZE)
//#define F77_FUNC(Name, name, NAME)
//#ifdef HAVE_NO_FORTRAN_UNDERSCORE
//#else
//#endif

struct descriptor                       /* VMS fixed length string    */
{                                  /* descriptor used in FORTRAN */
  unsigned short  length;
  unsigned char   data_type,         /* = 14      */
                  dsc_class;         /* = 1       */
  char            *string_ptr;
}; 
//
//void vtkxml_setfilename__(const char *filename, unsigned int length_arg)
//{
//  vtkstd::string s(filename, length_arg);
//  return vtkXML_SetFileName(s.c_str());
//}

#endif

#endif

