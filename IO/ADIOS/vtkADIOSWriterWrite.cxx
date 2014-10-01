/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkADIOSWriterWrite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "ADIOSWriter.h"
#include "vtkADIOSWriter.h"
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkFieldData.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkLookupTable.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#define BlockStepIndexEntry() \
  (static_cast<vtkTypeInt64>(this->CurrentStep) << 32) | \
  this->Controller->GetLocalProcessId()

//----------------------------------------------------------------------------
bool vtkADIOSWriter::UpdateMTimeTable(const std::string path,
  const vtkObject* value)
{
  unsigned long &mtimeCurrent = this->LastUpdated[path];
  unsigned long mtimeNew = const_cast<vtkObject*>(value)->GetMTime();
  unsigned long mtimePrev = mtimeCurrent;

  mtimeCurrent = mtimeNew;
  return this->WriteMode == vtkADIOSWriter::Always || mtimeNew != mtimePrev;
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkAbstractArray* v)
{
  if(!this->UpdateMTimeTable(path, v))
    {
    return;
    }

  vtkAbstractArray* valueTmp = const_cast<vtkAbstractArray*>(v);

  // String arryas not currently supported
  if(valueTmp->GetDataType() == VTK_STRING)
    {
    return;
    }

  // Ignore empty arrays
  if(valueTmp->GetNumberOfTuples() == 0 ||
    valueTmp->GetNumberOfComponents() == 0)
    {
    /*
    vtkWarningMacro("Skipping " << path << " because it is empty");
    */
    return;
    }

  this->Writer->WriteArray(path, valueTmp->GetVoidPointer(0));

  if(this->WriteMode == vtkADIOSWriter::OnChange)
    {
    this->BlockStepIndex[this->BlockStepIndexIdMap[path]] =
       BlockStepIndexEntry();
    }
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkDataArray* v)
{
  vtkDataArray* valueTmp = const_cast<vtkDataArray*>(v);
  vtkLookupTable *lut = valueTmp->GetLookupTable();

  if(lut)
    {
    // Only heck the mtime here if a LUT is present.  Otherwise it will be
    // handled apropriately by the abstract array writer
    if(!this->UpdateMTimeTable(path, v))
      {
      return;
      }

    this->Write(path+"/LookupTable", static_cast<vtkAbstractArray*>(lut->GetTable()));
    this->Write(path+"/Values", static_cast<vtkAbstractArray*>(valueTmp));
    }
  else
    {
    this->Write(path, static_cast<vtkAbstractArray*>(valueTmp));
    }
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkCellArray* v)
{
  if(!this->UpdateMTimeTable(path, v))
    {
    return;
    }

  vtkCellArray* valueTmp = const_cast<vtkCellArray*>(v);

  this->Writer->WriteScalar<vtkIdType>(path+"/NumberOfCells",
    valueTmp->GetNumberOfCells());
  this->Write(path+"/IndexArray", valueTmp->GetData());
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkFieldData* v)
{
  if(!this->UpdateMTimeTable(path, v))
    {
    return;
    }

  vtkFieldData* valueTmp = const_cast<vtkFieldData*>(v);
  for(int i = 0; i < valueTmp->GetNumberOfArrays(); ++i)
    {
    vtkDataArray *da = valueTmp->GetArray(i);
    vtkAbstractArray *aa = da ? da : valueTmp->GetAbstractArray(i);

    std::string name = aa->GetName();
    if(name.empty()) // skip unnamed arrays
      {
      continue;
      }
    this->Write(path+"/"+name, da ? da : aa);
    }
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkDataSet* v)
{
  if(!this->UpdateMTimeTable(path, v))
    {
    return;
    }

  vtkDataSet* valueTmp = const_cast<vtkDataSet*>(v);

  this->Write(path+"/FieldData", valueTmp->GetFieldData());
  this->Write(path+"/CellData", valueTmp->GetCellData());
  this->Write(path+"/PointData", valueTmp->GetPointData());
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkImageData* v)
{
  if(!this->UpdateMTimeTable(path, v))
    {
    return;
    }

  this->Write(path+"/DataSet", static_cast<const vtkDataSet*>(v));

  vtkImageData* valueTmp = const_cast<vtkImageData*>(v);
  this->Writer->WriteScalar<vtkTypeUInt8>(path+"/DataObjectType", VTK_IMAGE_DATA);

  double *origin = valueTmp->GetOrigin();
  this->Writer->WriteScalar<double>(path+"/OriginX", origin[0]);
  this->Writer->WriteScalar<double>(path+"/OriginY", origin[1]);
  this->Writer->WriteScalar<double>(path+"/OriginZ", origin[2]);

  double *spacing = valueTmp->GetSpacing();
  this->Writer->WriteScalar<double>(path+"/SpacingX", spacing[0]);
  this->Writer->WriteScalar<double>(path+"/SpacingY", spacing[1]);
  this->Writer->WriteScalar<double>(path+"/SpacingZ", spacing[2]);

  int *extent = valueTmp->GetExtent();
  this->Writer->WriteScalar<int>(path+"/ExtentXMin", extent[0]);
  this->Writer->WriteScalar<int>(path+"/ExtentXMax", extent[1]);
  this->Writer->WriteScalar<int>(path+"/ExtentYMin", extent[2]);
  this->Writer->WriteScalar<int>(path+"/ExtentYMax", extent[3]);
  this->Writer->WriteScalar<int>(path+"/ExtentZMin", extent[4]);
  this->Writer->WriteScalar<int>(path+"/ExtentZMax", extent[5]);
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkPolyData* v)
{
  if(!this->UpdateMTimeTable(path, v))
    {
    return;
    }

  this->Write(path+"/DataSet", static_cast<const vtkDataSet*>(v));

  vtkPolyData* valueTmp = const_cast<vtkPolyData*>(v);
  this->Writer->WriteScalar<vtkTypeUInt8>(path+"/DataObjectType",
    VTK_POLY_DATA);

  vtkPoints *p;
  if((p = valueTmp->GetPoints()))
    {
    this->Write(path+"/Points", p->GetData());
    }

  this->Write(path+"/Verticies", valueTmp->GetVerts());
  this->Write(path+"/Lines", valueTmp->GetLines());
  this->Write(path+"/Polygons", valueTmp->GetPolys());
  this->Write(path+"/Strips", valueTmp->GetStrips());
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path,
  const vtkUnstructuredGrid* v)
{
  if(!this->UpdateMTimeTable(path, v))
    {
    return;
    }

  this->Write(path+"/DataSet", static_cast<const vtkDataSet*>(v));

  vtkUnstructuredGrid *valueTmp = const_cast<vtkUnstructuredGrid*>(v);
  this->Writer->WriteScalar<vtkTypeUInt8>(path+"/DataObjectType",
    VTK_UNSTRUCTURED_GRID);

  vtkPoints *p;
  if((p = valueTmp->GetPoints()))
    {
    this->Write(path+"/Points", p->GetData());
    }

  vtkUnsignedCharArray *cta = valueTmp->GetCellTypesArray();
  vtkIdTypeArray *cla = valueTmp->GetCellLocationsArray();
  vtkCellArray *ca = valueTmp->GetCells();
  if(cta && cla && ca)
    {
    this->Write(path+"/CellTypes", cta);
    this->Write(path+"/CellLocations", cla);
    this->Write(path+"/Cells", ca);
    }
}
