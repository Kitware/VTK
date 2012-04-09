/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericCutter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkContourValues.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkImplicitFunction.h"
#include "vtkMergePoints.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPoints.h"
#include "vtkGenericCellIterator.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericPointIterator.h"
#include "vtkGenericDataSet.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericCellTessellator.h"
#include "vtkIncrementalPointLocator.h"

#include <math.h>

vtkStandardNewMacro(vtkGenericCutter);
vtkCxxSetObjectMacro(vtkGenericCutter,CutFunction,vtkImplicitFunction);
vtkCxxSetObjectMacro(vtkGenericCutter,Locator,vtkIncrementalPointLocator);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; initial value of 0.0; and
// generating cut scalars turned off.
//
vtkGenericCutter::vtkGenericCutter(vtkImplicitFunction *cf)
{
  this->ContourValues = vtkContourValues::New();
  this->CutFunction = cf;
  this->GenerateCutScalars = 0;
  this->Locator = NULL;
  
  this->InternalPD = vtkPointData::New();
  this->SecondaryPD = vtkPointData::New();
  this->SecondaryCD = vtkCellData::New();
}

//----------------------------------------------------------------------------
vtkGenericCutter::~vtkGenericCutter()
{
  this->ContourValues->Delete();
  this->SetCutFunction(NULL);
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  this->InternalPD->Delete();
  this->SecondaryPD->Delete();
  this->SecondaryCD->Delete();
}

//----------------------------------------------------------------------------
// Description:
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
void vtkGenericCutter::SetValue(int i, double value) 
{
  this->ContourValues->SetValue(i,value);
}

//-----------------------------------------------------------------------------
// Description:
// Get the ith contour value.
double vtkGenericCutter::GetValue(int i) 
{
  return this->ContourValues->GetValue(i);
}

//-----------------------------------------------------------------------------
// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
double *vtkGenericCutter::GetValues() 
{
  return this->ContourValues->GetValues();
}

//-----------------------------------------------------------------------------
// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
void vtkGenericCutter::GetValues(double *contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

//-----------------------------------------------------------------------------
// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
void vtkGenericCutter::SetNumberOfContours(int number) 
{
  this->ContourValues->SetNumberOfContours(number);
}

//-----------------------------------------------------------------------------
// Description:
// Get the number of contours in the list of contour values.
int vtkGenericCutter::GetNumberOfContours() 
{
  return this->ContourValues->GetNumberOfContours();
}

//-----------------------------------------------------------------------------
// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void vtkGenericCutter::GenerateValues(int numContours, double range[2]) 
{
  this->ContourValues->GenerateValues(numContours, range);
}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void vtkGenericCutter::GenerateValues(int numContours, double rangeStart,
                                      double rangeEnd) 
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If cut functions is modified,
// or contour values modified, then this object is modified as well.
//
unsigned long vtkGenericCutter::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long contourValuesMTime = this->ContourValues->GetMTime();
  unsigned long time;
 
  mTime = ( contourValuesMTime > mTime ? contourValuesMTime : mTime );

  if ( this->CutFunction != NULL )
    {
    time = this->CutFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
// Cut through data generating surface.
//
int vtkGenericCutter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGenericDataSet *input = vtkGenericDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Executing cutter");
  
  if (input==0)
    {
    vtkErrorMacro("No input specified");
    return 1;
    }
  
  if (this->CutFunction==0)
    {
    vtkErrorMacro("No cut function specified");
    return 1;
    }

  if ( input->GetNumberOfPoints()<1 )
    {
    vtkErrorMacro("Input data set is empty");
    return 1;
    }
  
  vtkPointData *outPd = output->GetPointData();
  vtkCellData *outCd = output->GetCellData();

  // Create objects to hold output of contour operation
  //
  vtkIdType numCells=input->GetNumberOfCells();
  int numContours = this->ContourValues->GetNumberOfContours();
  
  vtkIdType estimatedSize = static_cast<vtkIdType>(
    pow(static_cast<double>(numCells), .75)) * numContours;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
      estimatedSize = 1024;
    }

  vtkPoints *newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize,estimatedSize);
  vtkCellArray *newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize,estimatedSize);
  vtkCellArray *newLines = vtkCellArray::New();
  newLines->Allocate(estimatedSize,estimatedSize);
  vtkCellArray *newPolys = vtkCellArray::New();
  newPolys->Allocate(estimatedSize,estimatedSize);
  
  output->Allocate(numCells);
  
  // locator used to merge potentially duplicate points
  if(this->Locator==0)
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion(newPts,input->GetBounds(),estimatedSize);
  
  // prepare the output attributes
  vtkGenericAttributeCollection *attributes=input->GetAttributes();
  vtkGenericAttribute *attribute;
  vtkDataArray *attributeArray;
  
  int c=attributes->GetNumberOfAttributes();
  vtkDataSetAttributes *secondaryAttributes;

  int attributeType;
  
  for(vtkIdType i = 0; i<c; ++i)
    {
    attribute = attributes->GetAttribute(i);
    attributeType = attribute->GetType();
    if(attribute->GetCentering() == vtkPointCentered)
      {
      secondaryAttributes = this->SecondaryPD;
      
      attributeArray=vtkDataArray::CreateDataArray(attribute->GetComponentType());
      attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
      attributeArray->SetName(attribute->GetName());
      this->InternalPD->AddArray(attributeArray);
      attributeArray->Delete();
      if(this->InternalPD->GetAttribute(attributeType)==0)
        {
        this->InternalPD->SetActiveAttribute(
          this->InternalPD->GetNumberOfArrays()-1,attributeType);
        }
      }
    else // vtkCellCentered
      {
      secondaryAttributes = this->SecondaryCD;
      }
    
    attributeArray = vtkDataArray::CreateDataArray(attribute->GetComponentType());
    attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
    attributeArray->SetName(attribute->GetName());
    secondaryAttributes->AddArray(attributeArray);
    attributeArray->Delete();
    
    if(secondaryAttributes->GetAttribute(attributeType)==0)
      {
      secondaryAttributes->SetActiveAttribute(secondaryAttributes->GetNumberOfArrays()-1,
                                              attributeType);
      }
    }
  
  outPd->InterpolateAllocate(this->SecondaryPD,estimatedSize,estimatedSize);
  outCd->CopyAllocate(this->SecondaryCD,estimatedSize,estimatedSize);
  
  
  vtkGenericAdaptorCell *cell;

  //----------- Begin of contouring algorithm --------------------//
  vtkGenericCellIterator *cellIt = input->NewCellIterator();
    
  
  vtkIdType updateCount = numCells/20 + 1;  // update roughly every 5%
  vtkIdType count = 0;
  int abortExecute=0;
  
  input->GetTessellator()->InitErrorMetrics(input);
  
  for(cellIt->Begin(); !cellIt->IsAtEnd() && !abortExecute; cellIt->Next())
    {
    if ( !(count % updateCount) )
      {
      this->UpdateProgress(static_cast<double>(count) / numCells);
      abortExecute = this->GetAbortExecute();
      }
    
    cell = cellIt->GetCell();
    cell->Contour(this->ContourValues, this->CutFunction, input->GetAttributes(),
                  input->GetTessellator(),
                  this->Locator, newVerts, newLines, newPolys, outPd, outCd,
                  this->InternalPD,this->SecondaryPD,this->SecondaryCD);
    ++count;
    } // for each cell
  cellIt->Delete();

  vtkDebugMacro(<<"Created: " 
                << newPts->GetNumberOfPoints() << " points, " 
                << newVerts->GetNumberOfCells() << " verts, " 
                << newLines->GetNumberOfCells() << " lines, " 
                << newPolys->GetNumberOfCells() << " triangles");

  //----------- End of contouring algorithm ----------------------//

  // Update ourselves.  Because we don't know up front how many verts, lines,
  // polys we've created, take care to reclaim memory. 
  //
  output->SetPoints(newPts);
  newPts->Delete();

  if (newVerts->GetNumberOfCells()>0)
    {
    output->SetVerts(newVerts);
    }
  newVerts->Delete();
  
  if (newLines->GetNumberOfCells()>0)
    {
    output->SetLines(newLines);
    }
  newLines->Delete();

  if (newPolys->GetNumberOfCells()>0)
    {
    output->SetPolys(newPolys);
    }
  newPolys->Delete();

  this->Locator->Initialize();//releases leftover memory
  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkGenericCutter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkGenericCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Cut Function: " << this->CutFunction << "\n";

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Generate Cut Scalars: " 
     << (this->GenerateCutScalars ? "On\n" : "Off\n");
}
//----------------------------------------------------------------------------
int vtkGenericCutter::FillInputPortInformation(int port,
                                               vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
  return 1;
}
