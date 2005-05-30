/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLCInterface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkDataArray.h"

#include "vtkXMLCInterface.h"
#include "vtkFortran.h"
#include <vtkstd/string>

static vtkXMLUnstructuredGridWriter *writer;
static vtkUnstructuredGrid *ug;

//----------------------------------------------------------------------------

extern "C" {
void VTK_FORTRAN_NAME(vtkxml_initialize, VTKXML_INTIALIZE)()
{
  return vtkXML_Initialize();
}

//----------------------------------------------------------------------------
void vtkXML_Initialize()
{
  if( !ug && !writer)
    {
    ug = vtkUnstructuredGrid::New();
    ug->Allocate(1,1);
    vtkPoints *pts = vtkPoints::New();
    ug->SetPoints( pts );
    pts->Delete();
    writer = vtkXMLUnstructuredGridWriter::New();
    writer->SetInput( ug );
    ug->Delete();
    }
  else
    {
    vtkGenericWarningMacro( "Don't need to call vtkXML_Initialize twice" );
    }
}

//----------------------------------------------------------------------------
void VTK_FORTRAN_NAME(vtkxml_setfilename, VTKXML_SETFILENAME)
  (VTK_FORTRAN_ARG_STRING(filename))
{
  vtkstd::string s( VTK_FORTRAN_REF_STRING_POINTER(filename),
    VTK_FORTRAN_REF_STRING_LENGTH(filename));
  return vtkXML_SetFileName(s.c_str());
}

//----------------------------------------------------------------------------
void vtkXML_SetFileName(const char* filename)
{
  if( !writer)
    {
    vtkGenericWarningMacro( "You need to call vtkXML_Initialize first");
    return;
    }
  writer->SetFileName( filename );
}

//----------------------------------------------------------------------------
void VTK_FORTRAN_NAME(vtkxml_setpoints, VTKXML_SETPOINTS)
  (VTK_FORTRAN_ARG_INTEGER4(data), 
   VTK_FORTRAN_ARG_REAL4_ARRAY_1D(array), 
   VTK_FORTRAN_ARG_INTEGER8(size))
{
  return vtkXML_SetPoints(VTK_FORTRAN_REF_INTEGER4(data),
                          VTK_FORTRAN_REF_REAL4_ARRAY_1D(array),
                          VTK_FORTRAN_REF_INTEGER8(size));
}

//----------------------------------------------------------------------------
void vtkXML_SetPoints(int datatype, void* array, size_t size)
{
  if( !ug )
    {
    vtkGenericWarningMacro( "You need to call vtkXML_Initialize first");
    return;
    }
  vtkDataArray *dataarray = vtkDataArray::CreateDataArray( datatype );
  dataarray->SetNumberOfComponents(3);
  dataarray->SetVoidArray(array, 3*size, 1);

  vtkPoints *pts = ug->GetPoints();
  pts->SetNumberOfPoints( size );
  pts->SetData( dataarray );
  dataarray->Delete();
}

//----------------------------------------------------------------------------
void VTK_FORTRAN_NAME(vtkxml_setpointdata, VTKXML_SETPOINTDATA) (
   VTK_FORTRAN_ARG_INTEGER4(data), 
   VTK_FORTRAN_ARG_REAL4_ARRAY_1D(array), 
   VTK_FORTRAN_ARG_INTEGER8(size),
   VTK_FORTRAN_ARG_INTEGER4(numComp)
   )
{
  return vtkXML_SetPointData(VTK_FORTRAN_REF_INTEGER4(data),
                             VTK_FORTRAN_REF_REAL4_ARRAY_1D(array),
                             VTK_FORTRAN_REF_INTEGER8(size),
                             VTK_FORTRAN_REF_INTEGER4(numComp));
}

//----------------------------------------------------------------------------
void vtkXML_SetPointData(int datatype, void* array, size_t size, int numComp)
{
  if( !ug )
    {
    vtkGenericWarningMacro( "You need to call vtkXML_Initialize first");
    return;
    }
  vtkDataArray *dataarray = vtkDataArray::CreateDataArray( datatype );
  dataarray->SetNumberOfComponents (numComp);
  dataarray->SetVoidArray(array, size*numComp, 1); //do not save
  ug->GetPointData()->SetScalars(dataarray);
  dataarray->Delete();
}

//----------------------------------------------------------------------------
void VTK_FORTRAN_NAME(vtkxml_setcelldata, VTKXML_SETPOINTDATA)
  (VTK_FORTRAN_ARG_INTEGER4(data), 
   VTK_FORTRAN_ARG_REAL4_ARRAY_1D(array), 
   VTK_FORTRAN_ARG_INTEGER8(size),
   VTK_FORTRAN_ARG_INTEGER4(numComp))
{
  return vtkXML_SetCellData(VTK_FORTRAN_REF_INTEGER4(data),
                            VTK_FORTRAN_REF_REAL4_ARRAY_1D(array),
                            VTK_FORTRAN_REF_INTEGER8(size),
                            VTK_FORTRAN_REF_INTEGER4(numComp));
}
//----------------------------------------------------------------------------
void vtkXML_SetCellData(int datatype, void* array, size_t size, int numComp)
{
  if( !ug )
    {
    vtkGenericWarningMacro( "You need to call vtkXML_Initialize first");
    return;
    }
  vtkDataArray *dataarray = vtkDataArray::CreateDataArray( datatype );
  dataarray->SetNumberOfComponents (numComp);
  dataarray->SetVoidArray(array, size*numComp, 1); //do not save
  ug->GetCellData()->SetScalars(dataarray);
  dataarray->Delete();
}

//----------------------------------------------------------------------------
void VTK_FORTRAN_NAME(vtkxml_setcellarray, VTKXML_SETCELLARRAY)
  (VTK_FORTRAN_ARG_INTEGER4_ARRAY_1D(array), 
   VTK_FORTRAN_ARG_INTEGER4(ncells), 
   VTK_FORTRAN_ARG_INTEGER8(size))
{
  return vtkXML_SetCellArray( VTK_FORTRAN_REF_REAL4_ARRAY_1D(array),
                             VTK_FORTRAN_REF_INTEGER4(ncells),
                             VTK_FORTRAN_REF_INTEGER8(size));
}

//----------------------------------------------------------------------------
void vtkXML_SetCellArray(int* array, int ncells, size_t size)
{
  if( !ug )
    {
    vtkGenericWarningMacro( "You need to call vtkXML_Initialize first");
    return;
    }
  vtkIdTypeArray *cells = vtkIdTypeArray::New();
  cells->SetArray( array, size, 1);

  vtkCellArray *cellArray = ug->GetCells();
  cellArray->SetCells( ncells, cells);
  cells->Delete();

//  vtkUnsignedCharArray *ua = ug->GetCellTypesArray();
//  ua->InsertNextValue( 12 );
  ug->BuildLinks();
}

//----------------------------------------------------------------------------
void VTK_FORTRAN_NAME(vtkxml_writenexttime, VTKXML_WRITENEXTTIME)
  (VTK_FORTRAN_ARG_REAL8(t))
{
  return vtkXML_WriteNextTime(VTK_FORTRAN_REF_REAL8(t));
}

//----------------------------------------------------------------------------
void vtkXML_WriteNextTime(double t)
{
  if(!writer )
    {
    vtkGenericWarningMacro( "You need to call vtkXML_Initialize first");
    }
  else
    {
    writer->WriteNextTime(t);
    }
}

//----------------------------------------------------------------------------
void VTK_FORTRAN_NAME(vtkxml_setnumberoftimesteps, VTKXML_SETNUMBEROFTIMESTEPS)
  (VTK_FORTRAN_ARG_INT4(n))
{
  return vtkXML_SetNumberOfTimeSteps(VTK_FORTRAN_REF_INT4(n));
}

//----------------------------------------------------------------------------
void vtkXML_SetNumberOfTimeSteps(int n)
{
  if( !writer )
    {
    vtkGenericWarningMacro( "You need to call vtkXML_Initialize first");
    }
  else
    {
    writer->SetNumberOfTimeSteps(n);
    }
}

//----------------------------------------------------------------------------
void VTK_FORTRAN_NAME(vtkxml_start, VTKXML_START)()
{
  vtkXML_Start();
}

//----------------------------------------------------------------------------
void vtkXML_Start()
{
  if( writer->GetNumberOfTimeSteps() == 0 )
    {
    vtkGenericWarningMacro( "You need to call vtkXML_SetNumberOfTimeSteps first");
    }
  else if( !writer->GetFileName() )
    {
    vtkGenericWarningMacro( "You need to call vtkXML_SetFileName first");
    }
  else
    {
    writer->Start();
    }
}

//----------------------------------------------------------------------------
void VTK_FORTRAN_NAME(vtkxml_stop, VTKXML_STOP)()
{
  return vtkXML_Stop();
}

//----------------------------------------------------------------------------
void vtkXML_Stop()
{
  if( !writer )
    {
    vtkGenericWarningMacro(" You need to call vtkXML_Initialize first" );
    }
  else
    {
    writer->Stop();
    writer->Delete();
    writer = NULL;
    }
}
} //end of extern "C"
