/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericGeometryFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkHexahedron.h"
#include "vtkMergePoints.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPyramid.h"
#include "vtkStructuredGrid.h"
#include "vtkTetra.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCellIterator.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericDataSet.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericCellTessellator.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkGenericGeometryFilter);

vtkCxxSetObjectMacro(vtkGenericGeometryFilter,Locator,vtkIncrementalPointLocator);
//----------------------------------------------------------------------------
// Construct with all types of clipping turned off.
vtkGenericGeometryFilter::vtkGenericGeometryFilter()
{
  this->PointMinimum = 0;
  this->PointMaximum = VTK_LARGE_ID;

  this->CellMinimum = 0;
  this->CellMaximum = VTK_LARGE_ID;

  this->Extent[0] = VTK_DOUBLE_MIN;
  this->Extent[1] = VTK_DOUBLE_MAX;
  this->Extent[2] = VTK_DOUBLE_MIN;
  this->Extent[3] = VTK_DOUBLE_MAX;
  this->Extent[4] = VTK_DOUBLE_MIN;
  this->Extent[5] = VTK_DOUBLE_MAX;

  this->PointClipping = 0;
  this->CellClipping = 0;
  this->ExtentClipping = 0;

  this->Merging = 1;
  this->Locator = NULL;
  this->InternalPD = vtkPointData::New();

  this->PassThroughCellIds = 0;
}
//----------------------------------------------------------------------------
vtkGenericGeometryFilter::~vtkGenericGeometryFilter()
{
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  this->InternalPD->Delete();
}

//----------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkGenericGeometryFilter::SetExtent(double xMin, double xMax, double yMin,
                                         double yMax, double zMin, double zMax)
{
  double extent[6];

  extent[0] = xMin;
  extent[1] = xMax;
  extent[2] = yMin;
  extent[3] = yMax;
  extent[4] = zMin;
  extent[5] = zMax;

  this->SetExtent(extent);
}

//----------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkGenericGeometryFilter::SetExtent(double extent[6])
{
  int i;

  if ( extent[0] != this->Extent[0] || extent[1] != this->Extent[1] ||
       extent[2] != this->Extent[2] || extent[3] != this->Extent[3] ||
       extent[4] != this->Extent[4] || extent[5] != this->Extent[5] )
    {
    this->Modified();
    for (i=0; i<3; i++)
      {
      if ( extent[2*i+1] < extent[2*i] )
        {
        extent[2*i+1] = extent[2*i];
        }
      this->Extent[2*i] = extent[2*i];
      this->Extent[2*i+1] = extent[2*i+1];
      }
    }
}

//----------------------------------------------------------------------------
int vtkGenericGeometryFilter::RequestData(
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

  vtkIdType cellId;
  int i, j;
//  vtkGenericDataSet *input= this->GetInput();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  char *cellVis;
  vtkGenericAdaptorCell *cell;
  double x[3]={0,0,0};
  //vtkIdList *ptIds;
  vtkIdType ptIds[4];

  vtkIdType ptId;
  //int npts;
  //vtkIdType pt=0;
  int allVisible;
//  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  
  if (numCells == 0)
    {
    vtkErrorMacro(<<"No data to clip");
    return 1;
    }

  vtkDebugMacro(<<"Executing geometry filter");

  if ( (!this->CellClipping) && (!this->PointClipping) && 
  (!this->ExtentClipping) )
    {
    allVisible = 1;
    cellVis = NULL;
    }
  else
    {
    allVisible = 0;
    cellVis = new char[numCells];
    }

  vtkGenericCellIterator *cellIt = input->NewCellIterator();
  // Mark cells as being visible or not
  //
  if ( ! allVisible )
    {
    for (cellIt->Begin(); !cellIt->IsAtEnd(); cellIt->Next())
      {
      cell = cellIt->GetCell();
      cellId = cell->GetId();
      if ( this->CellClipping && (cellId < this->CellMinimum ||
                                  cellId > this->CellMaximum) )
        {
        cellVis[cellId] = 0;
        }
      else
        {
        //ptIds = cell->GetPointIds();
        cell->GetPointIds( ptIds );
        for (i=0; i < cell->GetNumberOfPoints(); i++) 
          {
          ptId = ptIds[i];
          //input->GetPoint(ptId, x);

          if ( (this->PointClipping && (ptId < this->PointMinimum ||
               ptId > this->PointMaximum) ) ||
               (this->ExtentClipping && 
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] ||
              x[1] < this->Extent[2] || x[1] > this->Extent[3] ||
              x[2] < this->Extent[4] || x[2] > this->Extent[5] )) )
            {
            cellVis[cellId] = 0;
            break;
            }
          }
        if ( i >= cell->GetNumberOfPoints() )
          {
          cellVis[cellId] = 1;
          }
        }
      }
    }

  // Allocate
  //
  vtkIdType estimatedSize = input->GetEstimatedSize();
  estimatedSize = (estimatedSize / 1024  + 1 )* 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }
  output->Allocate(numCells);

  vtkPoints *newPts = vtkPoints::New();
//FB  vtkDoubleArray *newScalars = vtkDoubleArray::New();
  vtkCellArray *cellArray = vtkCellArray::New();

  newPts->Allocate(estimatedSize,numPts);
//FB  newScalars->Allocate(estimatedSize, 5*numPts);
  cellArray->Allocate(numCells);

  
  // prepare the output attributes
  vtkGenericAttributeCollection *attributes=input->GetAttributes();
  vtkGenericAttribute *attribute;
  vtkDataArray *attributeArray;
  
  int c = attributes->GetNumberOfAttributes();
  vtkDataSetAttributes *dsAttributes;

  int attributeType;
  
  this->InternalPD->Initialize();
  for(i = 0; i<c; ++i)
    {
    attribute=attributes->GetAttribute(i);
    attributeType=attribute->GetType();
    int centering = attribute->GetCentering();
    if ( centering != vtkPointCentered && centering != vtkCellCentered )
      { // skip boundary-centered attributes.
      continue;
      }

    if ( centering == vtkPointCentered )
      {
      dsAttributes = outputPD;
      
      attributeArray = vtkDataArray::CreateDataArray(attribute->GetComponentType());
      attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
      attributeArray->SetName(attribute->GetName());
      this->InternalPD->AddArray(attributeArray);
      attributeArray->Delete();
      if(this->InternalPD->GetAttribute(attributeType)==0)
        {
        this->InternalPD->SetActiveAttribute(this->InternalPD->GetNumberOfArrays()-1,attributeType);
        }
      }
    else // vtkCellCentered
      {
      dsAttributes=outputCD;
      }
    attributeArray=vtkDataArray::CreateDataArray(attribute->GetComponentType());
    attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
    attributeArray->SetName(attribute->GetName());
    dsAttributes->AddArray(attributeArray);
    attributeArray->Delete();
    
    if(dsAttributes->GetAttribute(attributeType)==0)
      {
      dsAttributes->SetActiveAttribute(dsAttributes->GetNumberOfArrays()-1,attributeType);
      }
    }
  
  vtkIncrementalPointLocator *locator = 0;
  if ( this->Merging )
    {
    if ( this->Locator == NULL )
      {
      this->CreateDefaultLocator();
      }
    this->Locator->InitPointInsertion (newPts, input->GetBounds());
    locator = this->Locator;
    }

  // Traverse cells to extract geometry
  //
  int abort=0;
  vtkIdType progressInterval = numCells/20 + 1;

  vtkIdList *faceList = vtkIdList::New();
  faceList->SetNumberOfIds(3);

  input->GetTessellator()->InitErrorMetrics(input);
  
  vtkIdTypeArray *originalCellIds = NULL;
  if (this->PassThroughCellIds)
    {
    originalCellIds = vtkIdTypeArray::New();
    originalCellIds->SetName("vtkOriginalCellIds");
    originalCellIds->SetNumberOfComponents(1);
    }        
        
  for (cellId = 0, cellIt->Begin(); !cellIt->IsAtEnd() && !abort; 
    cellIt->Next(), cellId++)
    {
    cell = cellIt->GetCell();
    //Progress and abort method support
    if ( !(cellId % progressInterval) )
      {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId)/numCells);
      abort = this->GetAbortExecute();
      }

    vtkIdType BeginTopOutCId = outputCD->GetNumberOfTuples();
    if ( allVisible || cellVis[cellId] )
      {
      switch ( cell->GetDimension() )
        {
        // create new points and then cell
        case 0: case 1: 
            vtkErrorMacro( "Cell not handled yet" );
            break;
        case 2:
            if ( cell->IsOnBoundary() )
              {
              cell->Tessellate(input->GetAttributes(), input->GetTessellator(),
                               newPts, locator,cellArray, this->InternalPD,
                               outputPD, outputCD,0); //newScalars );

              }
          break;
        case 3:
//          int numFaces = cell->GetNumberOfFaces();
          int numFaces = cell->GetNumberOfBoundaries(2);
          for (j=0; j < numFaces; j++)
            {
            if ( cell->IsFaceOnBoundary(j) )
              {
              cell->TriangulateFace(input->GetAttributes(),
                                    input->GetTessellator(),
                                    j, newPts, locator, cellArray,
                                    this->InternalPD,outputPD, outputCD );
              }
            }
          break;

        } //switch
      } //if visible

    vtkIdType EndTopOutCId = outputCD->GetNumberOfTuples();
    if (this->PassThroughCellIds)
      {
      for (vtkIdType cId = BeginTopOutCId; cId < EndTopOutCId; cId++)
        {
        originalCellIds->InsertNextValue(cellId);
        }
      }

    } //for all cells

  if (this->PassThroughCellIds)
    {
    outputCD->AddArray(originalCellIds);
    originalCellIds->Delete();
    originalCellIds = NULL;
    }

  cellIt->Delete();
  vtkDebugMacro(<<"Extracted " << newPts->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  output->SetPolys(cellArray);
//FB  newScalars->SetNumberOfTuples( newPts->GetNumberOfPoints() );
//FB  outputPD->SetScalars(newScalars);

  cellArray->Delete();
  newPts->Delete();
//FB  newScalars->Delete();
  faceList->Delete();

  //free storage
  if (!this->Merging && this->Locator)
    {
    this->Locator->Initialize(); 
    }
  output->Squeeze();

  if ( cellVis )
    {
    delete [] cellVis;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkGenericGeometryFilter::FillInputPortInformation(int port,
                                                       vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkGenericGeometryFilter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

//----------------------------------------------------------------------------
void vtkGenericGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Point Minimum : " << this->PointMinimum << "\n";
  os << indent << "Point Maximum : " << this->PointMaximum << "\n";

  os << indent << "Cell Minimum : " << this->CellMinimum << "\n";
  os << indent << "Cell Maximum : " << this->CellMaximum << "\n";

  os << indent << "Extent: \n";
  os << indent << "  Xmin,Xmax: (" << this->Extent[0] << ", " << this->Extent[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Extent[2] << ", " << this->Extent[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Extent[4] << ", " << this->Extent[5] << ")\n";

  os << indent << "PointClipping: " << (this->PointClipping ? "On\n" : "Off\n");
  os << indent << "CellClipping: " << (this->CellClipping ? "On\n" : "Off\n");
  os << indent << "ExtentClipping: " << (this->ExtentClipping ? "On\n" : "Off\n");

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");
  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }

  os << indent << "PassThroughCellIds: " << (this->PassThroughCellIds ? "On\n" : "Off\n");

}

//----------------------------------------------------------------------------
unsigned long int vtkGenericGeometryFilter::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long time;

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}

//----------------------------------------------------------------------------
int vtkGenericGeometryFilter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  piece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1)
    {
    ++ghostLevels;
    }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
              numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              ghostLevels);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkGenericGeometryFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info objects
//  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
//  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("No Input");
    return 1;
    }
  return 1;
}
