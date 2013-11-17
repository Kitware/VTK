/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThreshold.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMath.h"

#include <algorithm>

vtkStandardNewMacro(vtkThreshold);

// Construct with lower threshold=0, upper threshold=1, and threshold
// function=upper AllScalars=1.
vtkThreshold::vtkThreshold()
{
  this->LowerThreshold         = 0.0;
  this->UpperThreshold         = 1.0;
  this->AllScalars             = 1;
  this->AttributeMode          = -1;
  this->ThresholdFunction      = &vtkThreshold::Upper;
  this->ComponentMode          = VTK_COMPONENT_MODE_USE_SELECTED;
  this->SelectedComponent      = 0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
                               vtkDataSetAttributes::SCALARS);

  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_RANGES(), 1);
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_BOUNDS(), 1);
  this->UseContinuousCellRange = 0;
}

vtkThreshold::~vtkThreshold()
{
}

// Criterion is cells whose scalars are less or equal to lower threshold.
void vtkThreshold::ThresholdByLower(double lower)
{
  if ( this->LowerThreshold != lower ||
       this->ThresholdFunction != &vtkThreshold::Lower)
    {
    this->LowerThreshold = lower;
    this->ThresholdFunction = &vtkThreshold::Lower;
    this->Modified();
    }
}

// Criterion is cells whose scalars are greater or equal to upper threshold.
void vtkThreshold::ThresholdByUpper(double upper)
{
  if ( this->UpperThreshold != upper ||
       this->ThresholdFunction != &vtkThreshold::Upper)
    {
    this->UpperThreshold = upper;
    this->ThresholdFunction = &vtkThreshold::Upper;
    this->Modified();
    }
}

// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkThreshold::ThresholdBetween(double lower, double upper)
{
  if ( this->LowerThreshold != lower || this->UpperThreshold != upper ||
       this->ThresholdFunction != &vtkThreshold::Between)
    {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->ThresholdFunction = &vtkThreshold::Between;
    this->Modified();
    }
}

int vtkThreshold::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType cellId, newCellId;
  vtkIdList *cellPts, *pointMap;
  vtkIdList *newCellPts;
  vtkCell *cell;
  vtkPoints *newPoints;
  int i, ptId, newId, numPts;
  int numCellPts;
  double x[3];
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();
  int keepCell, usePointScalars;

  vtkDebugMacro(<< "Executing threshold filter");

  if (this->AttributeMode != -1)
    {
    vtkErrorMacro(<<"You have set the attribute mode on vtkThreshold. This method is deprecated, please use SetInputArrayToProcess instead.");
    return 1;
    }

  vtkDataArray *inScalars = this->GetInputArrayToProcess(0,inputVector);

  if (!inScalars)
    {
    vtkDebugMacro(<<"No scalar data to threshold");
    return 1;
    }

  outPD->CopyGlobalIdsOn();
  outPD->CopyAllocate(pd);
  outCD->CopyGlobalIdsOn();
  outCD->CopyAllocate(cd);

  numPts = input->GetNumberOfPoints();
  output->Allocate(input->GetNumberOfCells());

  newPoints = vtkPoints::New();

  // set precision for the points in the output
  if(this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
    {
    vtkPointSet *inputPointSet = vtkPointSet::SafeDownCast(input);
    if(inputPointSet && inputPointSet->GetPoints())
      {
      newPoints->SetDataType(inputPointSet->GetPoints()->GetDataType());
      }
    else
      {
      newPoints->SetDataType(VTK_FLOAT);
      }
    }
  else if(this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
    {
    newPoints->SetDataType(VTK_FLOAT);
    }
  else if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
    {
    newPoints->SetDataType(VTK_DOUBLE);
    }

  newPoints->Allocate(numPts);

  pointMap = vtkIdList::New(); //maps old point ids into new
  pointMap->SetNumberOfIds(numPts);
  for (i=0; i < numPts; i++)
    {
    pointMap->SetId(i,-1);
    }

  newCellPts = vtkIdList::New();

  // are we using pointScalars?
  usePointScalars = (inScalars->GetNumberOfTuples() == numPts);

  // Check that the scalars of each cell satisfy the threshold criterion
  for (cellId=0; cellId < input->GetNumberOfCells(); cellId++)
    {
    cell = input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    if ( usePointScalars )
      {
      if (this->AllScalars)
        {
        keepCell = 1;
        for ( i=0; keepCell && (i < numCellPts); i++)
          {
          ptId = cellPts->GetId(i);
          keepCell = this->EvaluateComponents( inScalars, ptId );
          }
        }
      else
        {
        if(!this->UseContinuousCellRange)
          {
          keepCell = 0;
          for ( i=0; (!keepCell) && (i < numCellPts); i++)
            {
            ptId = cellPts->GetId(i);
            keepCell = this->EvaluateComponents( inScalars, ptId );
            }
          }
        else
          {
          keepCell = this->EvaluateCell(inScalars, cellPts, numCellPts);
          }
        }
      }
    else //use cell scalars
      {
      keepCell = this->EvaluateComponents( inScalars, cellId );
      }

    if (  numCellPts > 0 && keepCell )
      {
      // satisfied thresholding (also non-empty cell, i.e. not VTK_EMPTY_CELL)
      for (i=0; i < numCellPts; i++)
        {
        ptId = cellPts->GetId(i);
        if ( (newId = pointMap->GetId(ptId)) < 0 )
          {
          input->GetPoint(ptId, x);
          newId = newPoints->InsertNextPoint(x);
          pointMap->SetId(ptId,newId);
          outPD->CopyData(pd,ptId,newId);
          }
        newCellPts->InsertId(i,newId);
        }
      // special handling for polyhedron cells
      if (vtkUnstructuredGrid::SafeDownCast(input) &&
          input->GetCellType(cellId) == VTK_POLYHEDRON)
        {
        newCellPts->Reset();
        vtkUnstructuredGrid::SafeDownCast(input)->
          GetFaceStream(cellId, newCellPts);
        vtkUnstructuredGrid::ConvertFaceStreamPointIds(
          newCellPts, pointMap->GetPointer(0));
        }
      newCellId = output->InsertNextCell(cell->GetCellType(),newCellPts);
      outCD->CopyData(cd,cellId,newCellId);
      newCellPts->Reset();
      } // satisfied thresholding
    } // for all cells

  vtkDebugMacro(<< "Extracted " << output->GetNumberOfCells()
                << " number of cells.");

  // now clean up / update ourselves
  pointMap->Delete();
  newCellPts->Delete();

  output->SetPoints(newPoints);
  newPoints->Delete();

  output->Squeeze();

  return 1;
}

int vtkThreshold::EvaluateCell( vtkDataArray *scalars,vtkIdList* cellPts, int numCellPts )
{
  int c(0);
  int numComp = scalars->GetNumberOfComponents();
  int keepCell(0);
  switch (this->ComponentMode)
    {
    case VTK_COMPONENT_MODE_USE_SELECTED:
      c  =   (this->SelectedComponent < numComp)?(this->SelectedComponent):(0);
      keepCell = EvaluateCell(scalars,c,cellPts,numCellPts);
      break;
    case VTK_COMPONENT_MODE_USE_ANY:
      keepCell = 0;
      for ( c = 0; (!keepCell) && (c < numComp); c++ )
        {
        keepCell =EvaluateCell(scalars,c,cellPts,numCellPts);
        }
      break;
    case VTK_COMPONENT_MODE_USE_ALL:
      keepCell = 1;
      for ( c = 0; keepCell && (c < numComp); c++ )
        {
        keepCell =EvaluateCell(scalars,c,cellPts,numCellPts);
        }
      break;
    }
  return keepCell;
}

int vtkThreshold::EvaluateCell( vtkDataArray *scalars, int c, vtkIdList* cellPts, int numCellPts )
{
  double minScalar=DBL_MAX, maxScalar=DBL_MIN;
  for (int i=0; i < numCellPts; i++)
    {
    int ptId = cellPts->GetId(i);
    double s = scalars->GetComponent(ptId,c);
    minScalar = std::min(s,minScalar);
    maxScalar = std::max(s,maxScalar);
    }

  int keepCell =  !(this->LowerThreshold > maxScalar || this->UpperThreshold < minScalar);
  return keepCell;
}

int vtkThreshold::EvaluateComponents( vtkDataArray *scalars, vtkIdType id )
{
  int keepCell = 0;
  int numComp = scalars->GetNumberOfComponents();
  int c;

  switch ( this->ComponentMode )
    {
    case VTK_COMPONENT_MODE_USE_SELECTED:
      c = (this->SelectedComponent < numComp)?(this->SelectedComponent):(0);
      keepCell =
        (this->*(this->ThresholdFunction))(scalars->GetComponent(id,c));
      break;
    case VTK_COMPONENT_MODE_USE_ANY:
      keepCell = 0;
      for ( c = 0; (!keepCell) && (c < numComp); c++ )
        {
        keepCell =
          (this->*(this->ThresholdFunction))(scalars->GetComponent(id,c));
        }
      break;
    case VTK_COMPONENT_MODE_USE_ALL:
      keepCell = 1;
      for ( c = 0; keepCell && (c < numComp); c++ )
        {
        keepCell =
          (this->*(this->ThresholdFunction))(scalars->GetComponent(id,c));
        }
      break;
    }
  return keepCell;
}


// Return the method for manipulating scalar data as a string.
const char *vtkThreshold::GetAttributeModeAsString(void)
{
  if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT )
    {
    return "Default";
    }
  else if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA )
    {
    return "UsePointData";
    }
  else
    {
    return "UseCellData";
    }
}

// Return a string representation of the component mode
const char *vtkThreshold::GetComponentModeAsString(void)
{
  if ( this->ComponentMode == VTK_COMPONENT_MODE_USE_SELECTED )
    {
    return "UseSelected";
    }
  else if ( this->ComponentMode == VTK_COMPONENT_MODE_USE_ANY )
    {
    return "UseAny";
    }
  else
    {
    return "UseAll";
    }
}

void vtkThreshold::SetPointsDataType(int type)
{
  if(type == VTK_FLOAT)
    {
    this->SetOutputPointsPrecision(SINGLE_PRECISION);
    }
  else if(type == VTK_DOUBLE)
    {
    this->SetOutputPointsPrecision(DOUBLE_PRECISION);
    }
}

int vtkThreshold::GetPointsDataType()
{
  if(this->OutputPointsPrecision == SINGLE_PRECISION)
    {
    return VTK_FLOAT;
    }
  else if(this->OutputPointsPrecision == DOUBLE_PRECISION)
    {
    return VTK_DOUBLE;
    }

  return 0;
}

void vtkThreshold::SetOutputPointsPrecision(int precision)
{
  this->OutputPointsPrecision = precision;
  this->Modified();
}

int vtkThreshold::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

int vtkThreshold::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Attribute Mode: " << this->GetAttributeModeAsString() << endl;
  os << indent << "Component Mode: " << this->GetComponentModeAsString() << endl;
  os << indent << "Selected Component: " << this->SelectedComponent << endl;

  os << indent << "All Scalars: " << this->AllScalars << "\n";
  if ( this->ThresholdFunction == &vtkThreshold::Upper )
    {
    os << indent << "Threshold By Upper\n";
    }

  else if ( this->ThresholdFunction == &vtkThreshold::Lower )
    {
    os << indent << "Threshold By Lower\n";
    }

  else if ( this->ThresholdFunction == &vtkThreshold::Between )
    {
    os << indent << "Threshold Between\n";
    }

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";
  os << indent << "Precision of the output points: "
     << this->OutputPointsPrecision << "\n";
  os << indent << "Use Continuous Cell Range: "<<this->UseContinuousCellRange<<endl;
}

//----------------------------------------------------------------------------
int vtkThreshold::ProcessRequest(vtkInformation* request,
                                     vtkInformationVector** inputVector,
                                     vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT_INFORMATION()))
    {
    // compute the priority for this UE
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    // get the range of the input if available
    vtkInformation *fInfo = NULL;
    vtkDataArray *inscalars = this->GetInputArrayToProcess(0, inputVector);
    if (inscalars)
      {
      vtkInformationVector *miv = inInfo->Get(vtkDataObject::POINT_DATA_VECTOR());
      for (int index = 0; index < miv->GetNumberOfInformationObjects(); index++)
        {
        vtkInformation *mInfo = miv->GetInformationObject(index);
        const char *minfo_arrayname =
          mInfo->Get(vtkDataObject::FIELD_ARRAY_NAME());
        if (minfo_arrayname && !strcmp(minfo_arrayname, inscalars->GetName()))
          {
          fInfo = mInfo;
          break;
          }
        }
      }
    else
      {
      fInfo = vtkDataObject::GetActiveFieldInformation
        (inInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS,
         vtkDataSetAttributes::SCALARS);
      }
    if (!fInfo)
      {
      return 1;
      }

    double *range = fInfo->Get(vtkDataObject::PIECE_FIELD_RANGE());
    if (range)
      {
      // compute the priority
      // get the incoming priority if any
      double inPriority = 1;
      if (inInfo->Has(vtkStreamingDemandDrivenPipeline::PRIORITY()))
        {
        inPriority = inInfo->Get(vtkStreamingDemandDrivenPipeline::PRIORITY());
        }
      outInfo->Set(vtkStreamingDemandDrivenPipeline::PRIORITY(),inPriority);
      if (!inPriority)
        {
        return 1;
        }

      // do any contours intersect the range?
      if (this->ThresholdFunction == &vtkThreshold::Upper)
        {
        if ((this->*(this->ThresholdFunction))(range[0]))
          {
          return 1;
          }
        }
      if (this->ThresholdFunction == &vtkThreshold::Between)
        {
        if (
            (this->*(this->ThresholdFunction))(range[0]) ||
            (this->*(this->ThresholdFunction))(range[1]) ||
            (range[0] < this->LowerThreshold
             &&
             range[1] > this->UpperThreshold))
          {
          return 1;
          };
        }
      if (this->ThresholdFunction == &vtkThreshold::Lower)
        {
        if ((this->*(this->ThresholdFunction))(range[1]))
          {
          return 1;
          }
        }

      double inRes = 1.0;
      if (inInfo->Has(
                      vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
        {
        inRes = inInfo->Get(
                            vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
        }
      if (inRes == 1.0)
        {
        outInfo->Set(vtkStreamingDemandDrivenPipeline::PRIORITY(),0.0);
        }
      else
        {
        outInfo->Set(vtkStreamingDemandDrivenPipeline::PRIORITY(),inPriority*0.1);
        }
      }
    return 1;
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
