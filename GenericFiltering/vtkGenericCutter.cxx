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

#include <math.h>

vtkCxxRevisionMacro(vtkGenericCutter, "1.1");
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

  vtkGenericDataSet *input = this->GetInput();

  if (!input)
    {
    vtkErrorMacro("No input specified");
    return;
    }
  
  if (!this->CutFunction)
    {
    vtkErrorMacro("No cut function specified");
    return;
    }

  if ( input->GetNumberOfPoints() < 1 )
    {
    vtkErrorMacro("Input data set is empty");
    return;
    }
  
//  if (input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
    { 
    vtkDebugMacro(<< "Executing Unstructured Grid Cutter");   
    this->UnstructuredGridCutter();
    }
//  else
//    {
//    vtkDebugMacro(<< "Executing DataSet Cutter");
//    this->DataSetCutter();
//    }
}

//----------------------------------------------------------------------------
void vtkGenericCutter::UnstructuredGridCutter()
{
  vtkIdType i;
  int iter;
  vtkDoubleArray *cellScalars;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkPoints *newPoints;
  vtkDoubleArray *cutScalars;
  double *values, s;
  vtkPolyData *output = this->GetOutput();
  vtkGenericDataSet *input = this->GetInput();
  vtkIdType estimatedSize, numCells=input->GetNumberOfCells();
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkIdType cellArrayIt = 0;
  int numCellPts;
  vtkPointData *inPD = NULL, *outPD;
  //vtkCellData *inCD; //=input->GetCellData(), 
  vtkCellData *outCD=output->GetCellData();
  //vtkIdList *cellIds;
  vtkIdType cellIds[4];
  int numContours = this->ContourValues->GetNumberOfContours();
  int abortExecute = 0;

  double range[2];

  // Create objects to hold output of contour operation
  //
  estimatedSize = (vtkIdType) pow ((double) numCells, .75) * numContours;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
      estimatedSize = 1024;
    }

  newPoints = vtkPoints::New();
  newPoints->Allocate(estimatedSize,estimatedSize/2);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize,estimatedSize/2);
  newLines = vtkCellArray::New();
  newLines->Allocate(estimatedSize,estimatedSize/2);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(estimatedSize,estimatedSize/2);
  cutScalars = vtkDoubleArray::New();
  cutScalars->SetNumberOfTuples(numPts);

  // Interpolate data along edge. If generating cut scalars, do necessary setup
  if ( this->GenerateCutScalars )
    {
    inPD = vtkPointData::New();
//    inPD->ShallowCopy(input->GetPointData());//copies original attributes
    inPD->SetScalars(cutScalars);
    }
  else 
    {
//    inPD = input->GetPointData();
    }
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD,estimatedSize,estimatedSize/2);
  //outCD->CopyAllocate(inCD,estimatedSize,estimatedSize/2);
    
  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPoints, input->GetBounds());

  // Loop over all points evaluating scalar function at each point
  //
//  vtkGenericCellIterator *pointIt = input->NewVertexIterator();
  vtkGenericPointIterator *pointIt = input->NewPointIterator();
  double position[3];
  for( i = 0, pointIt->Begin(); !pointIt->IsAtEnd(); pointIt->Next() )
    {
    pointIt->GetPosition( position );
    s = this->CutFunction->FunctionValue( position );
    cutScalars->SetComponent(i++,0,s);
    }
  pointIt->Delete();

  vtkGenericCellIterator *cellIt = input->NewCellIterator();
  // Compute some information for progress methods
  //
  vtkIdType numCuts = numContours*numCells;
  vtkIdType progressInterval = numCuts/20 + 1;
  int cut=0;

//  vtkUnstructuredGrid *grid = (vtkUnstructuredGrid *)input;
//  vtkIdType *cellArrayPtr = grid->GetCells()->GetPointer();
  double *scalarArrayPtr = cutScalars->GetPointer(0);
  double tempScalar;
  cellScalars = cutScalars->NewInstance();
  cellScalars->SetNumberOfComponents(cutScalars->GetNumberOfComponents());
  cellScalars->Allocate(VTK_CELL_SIZE*cutScalars->GetNumberOfComponents());

  (void)values;
  (void)cellArrayIt;
  (void)range;
  (void)scalarArrayPtr;
  (void)numCellPts;
  (void)tempScalar;

  //input->ResetCell();
  
  // SORT_BY_VALUE:
    {
    // Loop over all cells; get scalar values for all cell points
    // and process each cell.
    //
    //for (cellId=0; cellId < numCells && !abortExecute; cellId++)
    //for (cellIt->Begin(); !cellIt->IsAtEnd() && !abortExecute; cellIt++)
    for (cellIt->Begin(); !cellIt->IsAtEnd() && !abortExecute; cellIt->Next())
      {
/*      numCellPts = cellArrayPtr[cellArrayIt];
      cellArrayIt++;
          
      //find min and max values in scalar data
      range[0] = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
      range[1] = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
      cellArrayIt++;
          
      for (i = 1; i < numCellPts; i++)
        {
        tempScalar = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
        cellArrayIt++;
        if (tempScalar <= range[0])
          {
          range[0] = tempScalar;
          } //if tempScalar <= min range value
        if (tempScalar >= range[1])
          {
          range[1] = tempScalar;
          } //if tempScalar >= max range value
        } // for all points in this cell
  */        
//      int needCell = 0;
//      for (int cont = 0; cont < numContours; ++cont) 
//        {
//        double val = this->ContourValues->GetValue(cont);
//        if (val >= range[0] && val <= range[1]) 
//          {
//          needCell = 1;
//          break;
//          }
//        }
          
//      if (needCell) 
        {
        vtkGenericAdaptorCell *cell = cellIt->GetCell();
        
        //cell->NewVertexIterator();  //FIXME
        //cellIds = cell->GetPointIds();
        cell->GetPointIds( cellIds );
        //cutScalars->GetTuples(cellIds,cellScalars);
        
        for(int j=0; j<cell->GetNumberOfPoints();j++)
          {
          //da->SetTuple(i,this->GetTuple(ptIds->GetId(i)));
          cellScalars->SetTuple(j, cutScalars->GetTuple(cellIds[j]));
          }
        
        (void)iter;
        // Loop over all contour values.
//        for (iter=0; iter < numContours && !abortExecute; iter++)
          {
          if ( !(++cut % progressInterval) )
            {
            vtkDebugMacro(<<"Cutting #" << cut);
            this->UpdateProgress ((double)cut/numCuts);
            abortExecute = this->GetAbortExecute();
            }

          cell->Contour(this->ContourValues, this->CutFunction,
                        input->GetAttributes(), input->GetTessellator(),this->Locator, 
                        newVerts, newLines, newPolys, outPD, outCD);
          } // for all contour values
            
        } // if need cell
      } // for all cells
    } // sort by value
  cellIt->Delete();

  // Update ourselves.  Because we don't know upfront how many verts, lines,
  // polys we've created, take care to reclaim memory. 
  //
  cellScalars->Delete();
  cutScalars->Delete();

  if ( this->GenerateCutScalars )
    {
    inPD->Delete();
    }

  output->SetPoints(newPoints);
  newPoints->Delete();

  if (newVerts->GetNumberOfCells())
    {
    output->SetVerts(newVerts);
    }
  newVerts->Delete();

  if (newLines->GetNumberOfCells())
    {
    output->SetLines(newLines);
    }
  newLines->Delete();

  if (newPolys->GetNumberOfCells())
    {
    output->SetPolys(newPolys);
    }
  newPolys->Delete();

  this->Locator->Initialize();//release any extra memory
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

  this->ContourValues->PrintSelf(os,indent);

  os << indent << "Generate Cut Scalars: " 
     << (this->GenerateCutScalars ? "On\n" : "Off\n");
}
