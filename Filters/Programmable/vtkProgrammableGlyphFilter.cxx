/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableGlyphFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProgrammableGlyphFilter.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkProgrammableGlyphFilter);

// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkProgrammableGlyphFilter::vtkProgrammableGlyphFilter()
{
  this->GlyphMethod = NULL;
  this->GlyphMethodArgDelete = NULL;
  this->GlyphMethodArg = NULL;

  this->Point[0] = this->Point[1] = this->Point[2] = 0.0;
  this->PointId = -1;
  this->PointData = NULL;

  this->ColorMode = VTK_COLOR_BY_INPUT;

  this->SetNumberOfInputPorts(2);
}

vtkProgrammableGlyphFilter::~vtkProgrammableGlyphFilter()
{
  if ((this->GlyphMethodArg)&&(this->GlyphMethodArgDelete))
  {
    (*this->GlyphMethodArgDelete)(this->GlyphMethodArg);
  }
}

void vtkProgrammableGlyphFilter::SetSourceConnection(vtkAlgorithmOutput* output)
{
  this->SetInputConnection(1, output);
}

void vtkProgrammableGlyphFilter::SetSourceData(vtkPolyData *pd)
{
  this->SetInputData(1, pd);
}

// Get a pointer to a source object at a specified table location.
vtkPolyData *vtkProgrammableGlyphFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return NULL;
  }

  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}

int vtkProgrammableGlyphFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *source = vtkPolyData::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData *inputPD = input->GetPointData();
  vtkCellData *inputCD = input->GetCellData();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkPoints *newPts, *sourcePts;
  vtkFloatArray *ptScalars=NULL, *cellScalars=NULL;
  vtkDataArray *inPtScalars = NULL, *inCellScalars = NULL;
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkPointData *sourcePD;
  vtkCellData *sourceCD;
  vtkIdType numSourcePts, numSourceCells, ptOffset=0, cellId, ptId, id, idx;
  int i, npts;
  vtkIdList *pts;
  vtkIdList *cellPts;
  vtkCell *cell;

  // Initialize
  vtkDebugMacro(<<"Generating programmable glyphs!");

  if ( numPts < 1 )
  {
    vtkErrorMacro(<<"No input points to glyph");
  }

  pts=vtkIdList::New();
  pts->Allocate(VTK_CELL_SIZE);
  sourcePD = source->GetPointData();
  sourceCD = source->GetCellData();
  numSourcePts = source->GetNumberOfPoints();
  numSourceCells = source->GetNumberOfCells();

  outputPD->CopyScalarsOff(); //'cause we control the coloring process
  outputCD->CopyScalarsOff();

  output->Allocate(numSourceCells*numPts,numSourceCells*numPts);
  outputPD->CopyAllocate(sourcePD, numSourcePts*numPts, numSourcePts*numPts);
  outputCD->CopyAllocate(sourceCD, numSourceCells*numPts, numSourceCells*numPts);
  newPts = vtkPoints::New();
  newPts->Allocate(numSourcePts*numPts);

  // figure out how to color the data and setup
  if ( this->ColorMode == VTK_COLOR_BY_INPUT )
  {
    if ( (inPtScalars = inputPD->GetScalars()) )
    {
      ptScalars = vtkFloatArray::New();
      ptScalars->Allocate(numSourcePts*numPts);
    }
    if ( (inCellScalars = inputCD->GetScalars()) )
    {
      cellScalars = vtkFloatArray::New();
      cellScalars->Allocate(numSourcePts*numPts);
    }
  }

  else
  {
    if ( sourcePD->GetScalars() )
    {
      ptScalars = vtkFloatArray::New();
      ptScalars->Allocate(numSourcePts*numPts);
    }
    if ( sourceCD->GetScalars() )
    {
      cellScalars = vtkFloatArray::New();
      cellScalars->Allocate(numSourcePts*numPts);
    }
  }

  // Loop over all points, invoking glyph method and Update(),
  // then append output of source to output of this filter.
  //
//  this->Updating = 1; // to prevent infinite recursion
  this->PointData = input->GetPointData();
  for (this->PointId=0; this->PointId < numPts; this->PointId++)
  {
    if ( ! (this->PointId % 10000) )
    {
      this->UpdateProgress ((double)this->PointId/numPts);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    input->GetPoint(this->PointId, this->Point);

    if ( this->GlyphMethod )
    {
      (*this->GlyphMethod)(this->GlyphMethodArg);

      // The GlyphMethod may have set the source connection to NULL
      if (this->GetNumberOfInputConnections(1) == 0)
      {
        source = NULL;
      }
      else
      {
        // Update the source connection in case the GlyphMethod changed
        // its parameters.
        this->GetInputAlgorithm(1, 0)->Update();
        // The GlyphMethod may also have changed the source.
        sourceInfo = inputVector[1]->GetInformationObject(0);
        source = vtkPolyData::SafeDownCast(
          sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
      }
    }
    if (source)
    {
      sourcePts = source->GetPoints();
      numSourcePts = source->GetNumberOfPoints();
      numSourceCells = source->GetNumberOfCells();
      sourcePD = source->GetPointData();
      sourceCD = source->GetCellData();

      if ( this->ColorMode == VTK_COLOR_BY_SOURCE )
      {
        inPtScalars = sourcePD->GetScalars();
        inCellScalars = sourceCD->GetScalars();
      }

      // Copy all data from source to output.
      for (ptId=0; ptId < numSourcePts; ptId++)
      {
        id = newPts->InsertNextPoint(sourcePts->GetPoint(ptId));
        outputPD->CopyData(sourcePD, ptId, id);
      }

      for (cellId=0; cellId < numSourceCells; cellId++)
      {
        cell = source->GetCell(cellId);
        cellPts = cell->GetPointIds();
        npts = cellPts->GetNumberOfIds();
        for (pts->Reset(), i=0; i < npts; i++)
        {
          pts->InsertId(i,cellPts->GetId(i) + ptOffset);
        }
        id = output->InsertNextCell(cell->GetCellType(),pts);
        outputCD->CopyData(sourceCD, cellId, id);
      }

      // If we're coloring the output with scalars, do that now
      if ( ptScalars )
      {
        for (ptId=0; ptId < numSourcePts; ptId++)
        {
          idx = (this->ColorMode == VTK_COLOR_BY_INPUT ? this->PointId : ptId);
          ptScalars->InsertNextValue(inPtScalars->GetComponent(idx,0));
        }
      }
      else if ( cellScalars )
      {
        for (cellId=0; cellId < numSourceCells; cellId++)
        {
          idx = (this->ColorMode == VTK_COLOR_BY_INPUT ? this->PointId : cellId);
          cellScalars->InsertNextValue(inCellScalars->GetComponent(idx,0));
        }
      }

      ptOffset += numSourcePts;

    }//if a source is available
  } //for all input points

//  this->Updating = 0;

  pts->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  if ( ptScalars )
  {
    idx = outputPD->AddArray(ptScalars);
    outputPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    ptScalars->Delete();
  }

  if ( cellScalars )
  {
    idx = outputCD->AddArray(cellScalars);
    outputCD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    cellScalars->Delete();
  }

  output->Squeeze();

  return 1;
}


// Specify function to be called before object executes.
void vtkProgrammableGlyphFilter::SetGlyphMethod(
  void (*f)(void *), void *arg)
{
  if ( f != this->GlyphMethod || arg != this->GlyphMethodArg )
  {
    // delete the current arg if there is one and a delete meth
    if ((this->GlyphMethodArg)&&(this->GlyphMethodArgDelete))
    {
      (*this->GlyphMethodArgDelete)(this->GlyphMethodArg);
    }
    this->GlyphMethod = f;
    this->GlyphMethodArg = arg;
    this->Modified();
  }
}

// Set the arg delete method. This is used to free user memory.
void vtkProgrammableGlyphFilter::SetGlyphMethodArgDelete(
  void (*f)(void *))
{
  if ( f != this->GlyphMethodArgDelete)
  {
    this->GlyphMethodArgDelete = f;
    this->Modified();
  }
}

// Description:
// Return the method of coloring as a descriptive character string.
const char *vtkProgrammableGlyphFilter::GetColorModeAsString(void)
{
  if ( this->ColorMode == VTK_COLOR_BY_INPUT )
  {
    return "ColorByInput";
  }
  else
  {
    return "ColorBySource";
  }
}

int vtkProgrammableGlyphFilter::FillInputPortInformation(int port,
                                                         vtkInformation *info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    return 1;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

void vtkProgrammableGlyphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;
  os << indent << "Point Id: " << this->PointId << "\n";
  os << indent << "Point: " << this->Point[0]
                    << ", " << this->Point[1]
                    << ", " << this->Point[2] << "\n";
  if (this->PointData)
  {
    os << indent << "PointData: " << this->PointData << "\n";
  }
  else
  {
    os << indent << "PointData: (not defined)\n";
  }

  if ( this->GlyphMethod )
  {
    os << indent << "Glyph Method defined\n";
  }
  else
  {
    os << indent << "No Glyph Method\n";
  }
}
