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

#include <math.h>

vtkCxxRevisionMacro(vtkGenericCutter, "1.5");
vtkStandardNewMacro(vtkGenericCutter);
vtkCxxSetObjectMacro(vtkGenericCutter,CutFunction,vtkImplicitFunction);

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
  
  this->internalPD=vtkPointData::New();
  this->secondaryPD=vtkPointData::New();
  this->secondaryCD=vtkCellData::New();
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
  this->internalPD->Delete();
  this->secondaryPD->Delete();
  this->secondaryCD->Delete();
}

//----------------------------------------------------------------------------
// Description:
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
void vtkGenericCutter::SetValue(int i, double value) 
{
  this->ContourValues->SetValue(i,value);
}

// Description:
// Get the ith contour value.
double vtkGenericCutter::GetValue(int i) 
{
  return this->ContourValues->GetValue(i);
}

// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
double *vtkGenericCutter::GetValues() 
{
  return this->ContourValues->GetValues();
}

// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
void vtkGenericCutter::GetValues(double *contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
void vtkGenericCutter::SetNumberOfContours(int number) 
{
  this->ContourValues->SetNumberOfContours(number);
}

// Description:
// Get the number of contours in the list of contour values.
int vtkGenericCutter::GetNumberOfContours() 
{
  return this->ContourValues->GetNumberOfContours();
}

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
  unsigned long mTime=this->vtkGenericDataSetToPolyDataFilter::GetMTime();
  unsigned long contourValuesMTime=this->ContourValues->GetMTime();
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
void vtkGenericCutter::Execute()
{
  vtkDebugMacro(<< "Executing cutter");

  vtkGenericDataSet *input=this->GetInput();

  if (input==0)
    {
    vtkErrorMacro("No input specified");
    return;
    }
  
  if (this->CutFunction==0)
    {
    vtkErrorMacro("No cut function specified");
    return;
    }

  if ( input->GetNumberOfPoints()<1 )
    {
    vtkErrorMacro("Input data set is empty");
    return;
    }
  
  vtkPolyData *output=this->GetOutput();
  vtkPointData *outPd = output->GetPointData();
  vtkCellData *outCd = output->GetCellData();

  // Create objects to hold output of contour operation
  //
  vtkIdType numCells=input->GetNumberOfCells();
  int numContours = this->ContourValues->GetNumberOfContours();
  
  vtkIdType estimatedSize = (vtkIdType) pow ((double) numCells, .75) * numContours;
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
  
  vtkIdType i=0;
  while(i<c)
    {
    attribute=attributes->GetAttribute(i);
    attributeType=attribute->GetType();
    if(attribute->GetCentering()==vtkPointCentered)
      {
      secondaryAttributes=this->secondaryPD;
      
      attributeArray=vtkDataArray::CreateDataArray(attribute->GetComponentType());
      attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
      attributeArray->SetName(attribute->GetName());
      this->internalPD->AddArray(attributeArray);
      attributeArray->Delete();
      if(this->internalPD->GetAttribute(attributeType)==0)
        {
        this->internalPD->SetActiveAttribute(this->internalPD->GetNumberOfArrays()-1,attributeType);
        }
      }
    else // vtkCellCentered
      {
      secondaryAttributes=this->secondaryCD;
      }
    
    attributeArray=vtkDataArray::CreateDataArray(attribute->GetComponentType());
    attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
    attributeArray->SetName(attribute->GetName());
    secondaryAttributes->AddArray(attributeArray);
    attributeArray->Delete();
    
    if(secondaryAttributes->GetAttribute(attributeType)==0)
      {
      secondaryAttributes->SetActiveAttribute(secondaryAttributes->GetNumberOfArrays()-1,
                                              attributeType);
      }
    ++i;
    }
  
  outPd->InterpolateAllocate(this->secondaryPD,estimatedSize,estimatedSize);
  outCd->CopyAllocate(this->secondaryCD,estimatedSize,estimatedSize);
  
  
  vtkGenericAdaptorCell *cell;

  //----------- Begin of contouring algorithm --------------------//
  vtkGenericCellIterator *cellIt = input->NewCellIterator();
    
  
  vtkIdType updateCount = numCells/20 + 1;  // update roughly every 5%
  vtkIdType count = 0;
  int abortExecute=0;
  for(cellIt->Begin(); !cellIt->IsAtEnd() && !abortExecute; cellIt->Next())
    {
    if ( !(count % updateCount) )
      {
      this->UpdateProgress((double)count / numCells);
      abortExecute = this->GetAbortExecute();
      }
    
    cell = cellIt->GetCell();
    cell->Contour(this->ContourValues, this->CutFunction, input->GetAttributes(),
                  input->GetTessellator(),
                  this->Locator, newVerts, newLines, newPolys, outPd, outCd,
                  this->internalPD,this->secondaryPD,this->secondaryCD);
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
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkGenericCutter::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator ) 
    {
    return;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  if ( locator )
    {
    locator->Register(this);
    }
  this->Locator = locator;
  this->Modified();
}

//----------------------------------------------------------------------------
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
