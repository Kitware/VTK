/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFacetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFacetWriter.h"

#include "vtkUnstructuredGrid.h"
#include "vtkObjectFactory.h"
#include "vtkErrorCode.h"
#include "vtkCellType.h"
#include "vtkCellArray.h"
#include "vtkGarbageCollector.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkUnsignedIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkSmartPointer.h"

#include <sys/stat.h>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkFacetWriter);

//----------------------------------------------------------------------------
vtkFacetWriter::vtkFacetWriter()
{
  this->FileName  = NULL;
  this->OutputStream = 0;
}

//----------------------------------------------------------------------------
vtkFacetWriter::~vtkFacetWriter()
{
  this->SetFileName(0);
}

//----------------------------------------------------------------------------
void vtkFacetWriter::Write()
{
  this->WriteToStream(0);
}

//----------------------------------------------------------------------------
int vtkFacetWriter::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed( outputVector) )
{
  this->SetErrorCode(vtkErrorCode::NoError);

  int cleanStream = 0;
  if ( !this->OutputStream )
  {
    if ( !this->FileName )
    {
      vtkErrorMacro("File name not specified");
      return 0;
    }

    this->OutputStream = new ofstream(this->FileName);
    if ( !this->OutputStream )
    {
      vtkErrorMacro("Error opening file: " << this->FileName << " for writing");
      return 0;
    }
    cleanStream = 1;
  }

  if ( !this->OutputStream )
  {
    vtkErrorMacro("No output stream");
    return 0;
  }

  int cc;
  int len = inputVector[0]->GetNumberOfInformationObjects();
  *this->OutputStream << "FACET FILE FROM VTK" << endl
    << len << endl;

  for ( cc =0; cc < len; cc ++ )
  {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(cc);
    vtkPolyData *input =
      vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if ( !this->WriteDataToStream(this->OutputStream, input) )
    {
      if ( cleanStream )
      {
        delete this->OutputStream;
        this->OutputStream = 0;
      }
      return 0;
    }
  }
  if ( cleanStream )
  {
    delete this->OutputStream;
    this->OutputStream = 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkFacetWriter::WriteToStream(ostream* ost)
{
  this->OutputStream = ost;
  // we always write, even if nothing has changed, so send a modified
  this->Modified();
  this->UpdateInformation();
  vtkInformation* inInfo = this->GetInputInformation(0, 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
              inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
              6);
  this->Update();
  this->OutputStream = 0;
}

//----------------------------------------------------------------------------
int vtkFacetWriter::WriteDataToStream(ostream* ost, vtkPolyData* data)
{
  *ost << "Element" << data << endl
    << "0" << endl
    << data->GetNumberOfPoints() << " 0 0" << endl;
  vtkIdType point;
  for ( point = 0; point < data->GetNumberOfPoints(); point ++ )
  {
    double xyz[3];
    data->GetPoint(point, xyz);
    *ost << xyz[0] << " " << xyz[1] << " " << xyz[2] << endl;
  }
  *ost << "1" << endl
    << "Element" << data << endl;
  int written = 0;
  vtkCellArray* ca;
  vtkIdType numCells;
  vtkIdType totalCells = 0;
  int material = 0;
  int part = 0;
  vtkIdType cc;

  if ( data->GetVerts()->GetNumberOfCells() )
  {
    // This test is needed if another cell type is written above this
    // block.  We must remove it here because it produces an
    // unreachable code warning.
    //
    //if ( written )
    //  {
    //  vtkErrorMacro("Multiple different cells in the poly data");
    //  return 0;
    //  }
    ca = data->GetVerts();
    numCells = ca->GetNumberOfCells();
    vtkIdType numPts = 0;
    vtkIdType *pts = NULL;
    ca->InitTraversal();
    while ( ca->GetNextCell( numPts, pts ) )
    {
      // Each vertex is one cell
      for ( cc = 0; cc < numPts; cc ++ )
      {
        totalCells ++;
      }
    }
    *ost << totalCells << " 1" << endl;
    ca->InitTraversal();
    while ( ca->GetNextCell( numPts, pts ) )
    {
      for ( cc = 0; cc < numPts; cc ++ )
      {
        // Indices of point starts with 1
        vtkIdType pointIndex = pts[cc] + 1;
        *ost << pointIndex << " " << material << " " << part << endl;
      }
    }
    written = 1;
  }

  if ( data->GetLines()->GetNumberOfCells() )
  {
    if ( written )
    {
      vtkErrorMacro("Multiple different cells in the poly data");
      return 0;
    }
    ca = data->GetLines();
    numCells = ca->GetNumberOfCells();
    vtkIdType numPts = 0;
    vtkIdType *pts = NULL;
    ca->InitTraversal();
    while ( ca->GetNextCell( numPts, pts ) )
    {
      // One line per cell
      for ( cc = 1; cc < numPts; cc ++ )
      {
        totalCells ++;
      }
    }
    *ost << totalCells << " 2" << endl;
    ca->InitTraversal();
    while ( ca->GetNextCell( numPts, pts ) )
    {
      for ( cc = 1; cc < numPts; cc ++ )
      {
        vtkIdType point1 = pts[cc-1] + 1;
        vtkIdType point2 = pts[cc] + 1;
        *ost << point1 << " " << point2 << " " << material << " " << part << endl;
      }
    }
    written = 1;
  }

  if ( data->GetPolys()->GetNumberOfCells() )
  {
    if ( written )
    {
      vtkErrorMacro("Multiple different cells in the poly data");
      return 0;
    }
    ca = data->GetPolys();
    numCells = ca->GetNumberOfCells();
    vtkIdType numPts = 0;
    vtkIdType *pts = NULL;
    ca->InitTraversal();
    ca->GetNextCell(numPts, pts);
    totalCells ++;
    vtkIdType numPoints = numPts;
    while ( ca->GetNextCell( numPts, pts ) )
    {
      if ( numPts != numPoints )
      {
        vtkErrorMacro("Found polygons with different order");
        return 0;
      }
      totalCells ++;
    }
    *ost << totalCells << " " << numPoints << endl;
    ca->InitTraversal();
    int cnt = 0;
    while ( ca->GetNextCell( numPts, pts ) )
    {
      for ( cc = 0; cc < numPts; cc ++ )
      {
        vtkIdType pointindex = pts[cc] + 1;
        *ost << pointindex << " ";
      }
      *ost << material << " " << part << endl;
      cnt ++;
    }
    cout << "Written: " << cnt  << " / " << numCells << " / " << totalCells << endl;
    written = 1;
  }

  if ( data->GetStrips()->GetNumberOfCells() )
  {
    if ( written )
    {
      vtkErrorMacro("Multiple different cells in the poly data");
      return 0;
    }
    ca = data->GetStrips();
    numCells = ca->GetNumberOfCells();
    vtkIdType numPts = 0;
    vtkIdType *pts = NULL;
    ca->InitTraversal();
    while ( ca->GetNextCell( numPts, pts ) )
    {
      // One triangle per cell
      for ( cc = 2; cc < numPts; cc ++ )
      {
        totalCells ++;
      }
    }
    *ost << totalCells << " 3" << endl;
    ca->InitTraversal();
    while ( ca->GetNextCell( numPts, pts ) )
    {
      for ( cc = 2; cc < numPts; cc ++ )
      {
        vtkIdType point1 = pts[cc-2] + 1;
        vtkIdType point2 = pts[cc-1] + 1;
        vtkIdType point3 = pts[cc] + 1;
        *ost << point1 << " " << point2 << " " << point3 << " " << material << " " << part << endl;
      }
    }
    written = 1;
  }

 return 1;
}

//----------------------------------------------------------------------------
int vtkFacetWriter::FillInputPortInformation(int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}
//----------------------------------------------------------------------------
void vtkFacetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
    << (this->FileName ? this->FileName : "(none)") << "\n";
}

