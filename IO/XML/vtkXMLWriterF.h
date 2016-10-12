/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLWriterF.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkXMLWriterF_h
#define vtkXMLWriterF_h
/*
 * vtkXMLWriterF.h helps fortran programs call the C interface for
 * writing VTK XML files.  A program can use this by writing one
 * vtkXMLWriterF.c file that includes this header.  DO NOT INCLUDE
 * THIS HEADER ELSEWHERE.  The fortran program then compiles
 * vtkXMLWriterF.c using a C compiler and links to the resulting
 * object file.
 */

#if defined(__cplusplus)
# error "This should be included only by a .c file."
#endif

/* Calls will be forwarded to the C interface.  */
#include "vtkXMLWriterC.h"

#include <stdio.h>  /* fprintf */
#include <stdlib.h> /* malloc, free */
#include <string.h> /* memcpy */

/* Define a static-storage default-zero-initialized table to store
   writer objects for the fortran program.  */
#define VTK_XMLWRITERF_MAX 256
static vtkXMLWriterC* vtkXMLWriterF_Table[VTK_XMLWRITERF_MAX+1];

/* Fortran compilers expect certain symbol names for their calls to C
   code.  These macros help build the C symbols so that the fortran
   program can link to them properly.  The definitions here are
   reasonable defaults but the source file that includes this can
   define them appropriately for a particular compiler and override
   these.  */
#if !defined(VTK_FORTRAN_NAME)
# define VTK_FORTRAN_NAME(name, NAME) name##__
#endif
#if !defined(VTK_FORTRAN_ARG_STRING_POINTER)
# define VTK_FORTRAN_ARG_STRING_POINTER(name) const char* name##_ptr_arg
#endif
#if !defined(VTK_FORTRAN_ARG_STRING_LENGTH)
# define VTK_FORTRAN_ARG_STRING_LENGTH(name) , const long int name##_len_arg
#endif
#if !defined(VTK_FORTRAN_REF_STRING_POINTER)
# define VTK_FORTRAN_REF_STRING_POINTER(name) name##_ptr_arg
#endif
#if !defined(VTK_FORTRAN_REF_STRING_LENGTH)
# define VTK_FORTRAN_REF_STRING_LENGTH(name) ((int)name##_len_arg)
#endif

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_New */
void VTK_FORTRAN_NAME(vtkxmlwriterf_new, VTKXMLWRITERF_NEW)(
  int* self
  )
{
  int i;

  /* Initialize result to failure.  */
  *self = 0;

  /* Search for a table entry to use for this object.  */
  for(i=1;i <= VTK_XMLWRITERF_MAX; ++i)
  {
    if(!vtkXMLWriterF_Table[i])
    {
      vtkXMLWriterF_Table[i] = vtkXMLWriterC_New();
      if(vtkXMLWriterF_Table[i])
      {
        *self = i;
      }
      return;
    }
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_Delete */
void VTK_FORTRAN_NAME(vtkxmlwriterf_delete, VTKXMLWRITERF_DELETE)(
  int* self
  )
{
  /* Check if the writer object exists.  */
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    /* Delete this writer object.  */
    vtkXMLWriterC_Delete(vtkXMLWriterF_Table[*self]);

    /* Erase the table entry.  */
    vtkXMLWriterF_Table[*self] = 0;
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_Delete called with invalid id %d.\n",
            *self);
  }

  /* The writer object no longer exists.  Destroy the id.  */
  *self = 0;
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetDataModeType */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setdatamodetype, VTKXMLWRITERF_SETDATAMODETYPE)(
  const int* self, const int* objType
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_SetDataModeType(vtkXMLWriterF_Table[*self], *objType);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetDataModeType called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetDataObjectType */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setdataobjecttype, VTKXMLWRITERF_SETDATAOBJECTTYPE)(
  const int* self, const int* objType
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_SetDataObjectType(vtkXMLWriterF_Table[*self], *objType);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetDataObjectType called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetExtent */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setextent, VTKXMLWRITERF_SETEXTENT)(
  const int* self, int extent[6]
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_SetExtent(vtkXMLWriterF_Table[*self], extent);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetExtent called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetPoints */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setpoints, VTKXMLWRITERF_SETPOINTS)(
  const int* self, const int* dataType,
  void* data, const vtkIdType* numPoints
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_SetPoints(vtkXMLWriterF_Table[*self], *dataType,
                            data, *numPoints);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetPoints called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetOrigin */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setorigin, VTKXMLWRITERF_SETORIGIN)(
  const int* self, double origin[3]
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_SetOrigin(vtkXMLWriterF_Table[*self], origin);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetOrigin called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetSpacing */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setspacing, VTKXMLWRITERF_SETSPACING)(
  const int* self, double spacing[3]
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_SetSpacing(vtkXMLWriterF_Table[*self], spacing);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetSpacing called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetCoordinates */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setcoordinates, VTKXMLWRITERF_SETCOORDINATES)(
  const int* self, const int* axis, const int* dataType, void* data,
  const vtkIdType* numCoordinates
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_SetCoordinates(vtkXMLWriterF_Table[*self], *axis,
                                 *dataType, data, *numCoordinates);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetCoordinates called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetCellsWithType */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setcellswithtype, VTKXMLWRITERF_SETCELLSWITHTYPE)(
  const int* self, const int* cellType, const vtkIdType* ncells,
  vtkIdType* cells, const vtkIdType* cellsSize
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_SetCellsWithType(vtkXMLWriterF_Table[*self], *cellType,
                                   *ncells, cells, *cellsSize);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetCellsWithType called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetCellsWithTypes */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setcellswithtypes, VTKXMLWRITERF_SETCELLSWITHTYPES)(
  const int* self, int* cellTypes, const vtkIdType* ncells,
  vtkIdType* cells, const vtkIdType* cellsSize
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_SetCellsWithTypes(vtkXMLWriterF_Table[*self], cellTypes,
                                    *ncells, cells, *cellsSize);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetCellsWithTypes called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetPointData */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setpointdata, VTKXMLWRITERF_SETPOINTDATA)(
  const int* self, VTK_FORTRAN_ARG_STRING_POINTER(name),
  const int* dataType, void* data, const vtkIdType* numTuples,
  const int* numComponents, VTK_FORTRAN_ARG_STRING_POINTER(role)
  VTK_FORTRAN_ARG_STRING_LENGTH(name)
  VTK_FORTRAN_ARG_STRING_LENGTH(role)
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    /* Prepare NULL-terminated strings.  */
    const char* name_ptr = VTK_FORTRAN_REF_STRING_POINTER(name);
    int name_length = VTK_FORTRAN_REF_STRING_LENGTH(name);
    char* name_buffer = malloc(name_length+1);
    const char* role_ptr = VTK_FORTRAN_REF_STRING_POINTER(role);
    int role_length = VTK_FORTRAN_REF_STRING_LENGTH(role);
    char* role_buffer = malloc(role_length+1);
    if(!name_buffer || !role_buffer)
    {
      fprintf(stderr,
              "vtkXMLWriterF_SetPointData failed to allocate name or role.\n");
      if(name_buffer) { free(name_buffer); }
      if(role_buffer) { free(role_buffer); }
      return;
    }
    memcpy(name_buffer, name_ptr, name_length);
    name_buffer[name_length] = 0;
    memcpy(role_buffer, role_ptr, role_length);
    role_buffer[role_length] = 0;

    /* Forward the call.  */
    vtkXMLWriterC_SetPointData(vtkXMLWriterF_Table[*self], name_buffer,
                               *dataType, data, *numTuples, *numComponents,
                               role_buffer);

    /* Free the NULL-terminated strings.  */
    free(name_buffer);
    free(role_buffer);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetPointData called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetCellData */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setcelldata, VTKXMLWRITERF_SETCELLDATA)(
  const int* self, VTK_FORTRAN_ARG_STRING_POINTER(name),
  const int* dataType, void* data, const vtkIdType* numTuples,
  const int* numComponents, VTK_FORTRAN_ARG_STRING_POINTER(role)
  VTK_FORTRAN_ARG_STRING_LENGTH(name)
  VTK_FORTRAN_ARG_STRING_LENGTH(role)
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    /* Prepare NULL-terminated strings.  */
    const char* name_ptr = VTK_FORTRAN_REF_STRING_POINTER(name);
    int name_length = VTK_FORTRAN_REF_STRING_LENGTH(name);
    char* name_buffer = malloc(name_length+1);
    const char* role_ptr = VTK_FORTRAN_REF_STRING_POINTER(role);
    int role_length = VTK_FORTRAN_REF_STRING_LENGTH(role);
    char* role_buffer = malloc(role_length+1);
    if(!name_buffer || !role_buffer)
    {
      fprintf(stderr,
              "vtkXMLWriterF_SetCellData failed to allocate name or role.\n");
      if(name_buffer) { free(name_buffer); }
      if(role_buffer) { free(role_buffer); }
      return;
    }
    memcpy(name_buffer, name_ptr, name_length);
    name_buffer[name_length] = 0;
    memcpy(role_buffer, role_ptr, role_length);
    role_buffer[role_length] = 0;

    /* Forward the call.  */
    vtkXMLWriterC_SetCellData(vtkXMLWriterF_Table[*self], name_buffer,
                              *dataType, data, *numTuples, *numComponents,
                              role_buffer);

    /* Free the NULL-terminated strings.  */
    free(name_buffer);
    free(role_buffer);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetCellData called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetFileName */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setfilename, VTKXMLWRITERF_SETFILENAME)(
  const int* self, VTK_FORTRAN_ARG_STRING_POINTER(name)
  VTK_FORTRAN_ARG_STRING_LENGTH(name)
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    /* Prepare NULL-terminated string.  */
    const char* name_ptr = VTK_FORTRAN_REF_STRING_POINTER(name);
    int name_length = VTK_FORTRAN_REF_STRING_LENGTH(name);
    char* name_buffer = malloc(name_length+1);
    if(!name_buffer)
    {
      fprintf(stderr,
              "vtkXMLWriterF_SetFileName failed to allocate name.\n");
      return;
    }
    memcpy(name_buffer, name_ptr, name_length);
    name_buffer[name_length] = 0;

    /* Forward the call.  */
    vtkXMLWriterC_SetFileName(vtkXMLWriterF_Table[*self], name_buffer);

    /* Free the NULL-terminated string.  */
    free(name_buffer);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetFileName called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_Write */
void VTK_FORTRAN_NAME(vtkxmlwriterf_write, VTKXMLWRITERF_WRITE)(
  const int* self, int* success
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    *success = vtkXMLWriterC_Write(vtkXMLWriterF_Table[*self]);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_Write called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_SetNumberOfTimeSteps */
void VTK_FORTRAN_NAME(vtkxmlwriterf_setnumberoftimesteps, VTKXMLWRITERF_SETNUMBEROFTIMESTEPS)(
  const int* self, const int* numTimeSteps
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_SetNumberOfTimeSteps(vtkXMLWriterF_Table[*self],
                                       *numTimeSteps);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_SetNumberOfTimeSteps called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_Start */
void VTK_FORTRAN_NAME(vtkxmlwriterf_start, VTKXMLWRITERF_START)(
  const int* self
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_Start(vtkXMLWriterF_Table[*self]);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_Start called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_WriteNextTimeStep */
void VTK_FORTRAN_NAME(vtkxmlwriterf_writenexttimestep, VTKXMLWRITERF_WRITENEXTTIMESTEP)(
  const int* self, const double* timeValue
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_WriteNextTimeStep(vtkXMLWriterF_Table[*self], *timeValue);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_WriteNextTimeStep called with invalid id %d.\n",
            *self);
  }
}

/*--------------------------------------------------------------------------*/
/* vtkXMLWriterF_Stop */
void VTK_FORTRAN_NAME(vtkxmlwriterf_stop, VTKXMLWRITERF_STOP)(
  const int* self
  )
{
  if(*self > 0 && *self <= VTK_XMLWRITERF_MAX && vtkXMLWriterF_Table[*self])
  {
    vtkXMLWriterC_Stop(vtkXMLWriterF_Table[*self]);
  }
  else
  {
    fprintf(stderr,
            "vtkXMLWriterF_Stop called with invalid id %d.\n",
            *self);
  }
}
#endif
// VTK-HeaderTest-Exclude: vtkXMLWriterF.h
