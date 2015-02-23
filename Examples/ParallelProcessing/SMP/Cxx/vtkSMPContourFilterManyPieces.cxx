/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkSMPContourFilterManyPieces.h"
#include "vtkSMPContourGridManyPieces.h"
#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSimpleScalarTree.h"
#include "vtkGenericCell.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkImageData.h"
#include "vtkUniformGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkContourHelper.h"
#include "vtkCutter.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPMinMaxTree.h"

#include "vtkSMPTools2.h"

#include <math.h>

vtkStandardNewMacro(vtkSMPContourFilterManyPieces);

// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkSMPContourFilterManyPieces::vtkSMPContourFilterManyPieces()
{
}

vtkSMPContourFilterManyPieces::~vtkSMPContourFilterManyPieces()
{
}

namespace
{
  class vtkContourFilterManyPiecesTraversalFunctor
  {
    vtkSMPContourFilterManyPieces* Filter;
  public:
    vtkContourFilterManyPiecesTraversalFunctor(vtkSMPContourFilterManyPieces* f)
      : Filter(f)
    {
    }

    void operator()(vtkIdType vtkNotUsed(cellId)){}

  };

// This functor creates a new vtkPolyData piece each time it runs.
// This is less efficient that the previous version but can be used
// to generate more piece to exploit coarse-grained parallelism
// downstream.
  class vtkContourFilterManyPiecesFunctor
  {
    vtkSMPContourFilterManyPieces* Filter;
    vtkDataSet* Input;
    vtkPointSet* InputPointSet;
    vtkDataArray* InScalars;
    vtkMultiBlockDataSet* Output;
    vtkInformation* Info;

    int NumValues;
    double* Values;
    unsigned char cellTypeDimensions[VTK_NUMBER_OF_CELL_TYPES];

    vtkSMPThreadLocal<std::vector<vtkPolyData*> > Outputs;

  public:

    vtkContourFilterManyPiecesFunctor(vtkSMPContourFilterManyPieces* filter,
                                      vtkDataSet* input,
                                      vtkDataArray* inScalars,
                                      int numValues,
                                      double* values,
                                      vtkMultiBlockDataSet* output,
                                      vtkInformation* info)
      : Filter(filter), Input(input), InScalars(inScalars), Output(output),
        Info(info), NumValues(numValues), Values(values)
    {
      InputPointSet = vtkPointSet::SafeDownCast(input);
      vtkCutter::GetCellTypeDimensions(cellTypeDimensions);
    }

    ~vtkContourFilterManyPiecesFunctor()
    {
    }

    void Initialize()
    {
    }

    void operator()(vtkIdType begin, vtkIdType end)
    {
      vtkIdList *cellPts;
      vtkCellArray *newVerts, *newLines, *newPolys;
      vtkIdType numCells, estimatedSize;
      vtkDataArray *cellScalars;

      vtkNew<vtkPolyData> output;

      vtkNew<vtkPoints> newPts;

      vtkPointData *inPd=Input->GetPointData(), *outPd=output->GetPointData();
      vtkCellData *inCd=Input->GetCellData(), *outCd=output->GetCellData();

      // set precision for the points in the output
      if(this->Filter->GetOutputPointsPrecision() == vtkAlgorithm::DEFAULT_PRECISION)
        {
        if(InputPointSet)
          {
          newPts->SetDataType(InputPointSet->GetPoints()->GetDataType());
          }
        else
          {
          newPts->SetDataType(VTK_FLOAT);
          }
        }
      else if(this->Filter->GetOutputPointsPrecision() == vtkAlgorithm::SINGLE_PRECISION)
        {
        newPts->SetDataType(VTK_FLOAT);
        }
      else if(this->Filter->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION)
        {
        newPts->SetDataType(VTK_DOUBLE);
        }

      output->SetPoints(newPts.GetPointer());

      numCells = end - begin;
      estimatedSize = static_cast<vtkIdType>(pow(static_cast<double>(numCells),.75));
      estimatedSize *= NumValues;
      estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
      if (estimatedSize < 1024)
        {
        estimatedSize = 1024;
        }

      newPts->Allocate(estimatedSize,estimatedSize);
      newVerts = vtkCellArray::New();
      newVerts->Allocate(estimatedSize,estimatedSize);
      newLines = vtkCellArray::New();
      newLines->Allocate(estimatedSize,estimatedSize);
      newPolys = vtkCellArray::New();
      newPolys->Allocate(estimatedSize,estimatedSize);
      cellScalars = InScalars->NewInstance();
      cellScalars->SetNumberOfComponents(InScalars->GetNumberOfComponents());
      cellScalars->Allocate(cellScalars->GetNumberOfComponents()*VTK_CELL_SIZE);

      // locator used to merge potentially duplicate points
      vtkIncrementalPointLocator* locator = this->Filter->GetLocator()->NewInstance();
      locator->InitPointInsertion (newPts.GetPointer(),
                                   Input->GetBounds(),
                                   Input->GetNumberOfPoints());

      // interpolate data along edge
      // if we did not ask for scalars to be computed, don't copy them
      if (!this->Filter->GetComputeScalars())
        {
        outPd->CopyScalarsOff();
        }
      outPd->InterpolateAllocate(inPd,estimatedSize,estimatedSize);
      outCd->CopyAllocate(inCd,estimatedSize,estimatedSize);

      vtkContourHelper helper(locator, newVerts, newLines, newPolys,inPd, inCd, outPd,outCd, estimatedSize, this->Filter->GetGenerateTriangles()!=0);

      vtkGenericCell *cell = vtkGenericCell::New();
      // Three passes over the cells to process lower dimensional cells first.
      // For poly data output cells need to be added in the order:
      // verts, lines and then polys, or cell data gets mixed up.
      // A better solution is to have an unstructured grid output.
      // I create a table that maps cell type to cell dimensionality,
      // because I need a fast way to get cell dimensionality.
      // This assumes GetCell is slow and GetCellType is fast.
      // I do not like hard coding a list of cell types here,
      // but I do not want to add GetCellDimension(vtkIdType cellId)
      // to the vtkDataSet API.  Since I anticipate that the output
      // will change to vtkUnstructuredGrid.  This temporary solution
      // is acceptable.
      //
      int cellType;
      int dimensionality;
      int i;
      vtkIdType cellId;
      // We skip 0d cells (points), because they cannot be cut (generate no data).
      for (dimensionality = 1; dimensionality <= 3; ++dimensionality)
        {
        // Loop over all cells; get scalar values for all cell points
        // and process each cell.
        //
        for (cellId=begin; cellId < end; cellId++)
          {
          // I assume that "GetCellType" is fast.
          cellType = Input->GetCellType(cellId);
          if (cellType >= VTK_NUMBER_OF_CELL_TYPES)
            { // Protect against new cell types added.
            //vtkErrorMacro("Unknown cell type " << cellType);
            continue;
            }
          if (cellTypeDimensions[cellType] != dimensionality)
            {
            continue;
            }
          Input->GetCell(cellId,cell);
          cellPts = cell->GetPointIds();
          if (cellScalars->GetSize()/cellScalars->GetNumberOfComponents() <
              cellPts->GetNumberOfIds())
            {
            cellScalars->Allocate(
              cellScalars->GetNumberOfComponents()*cellPts->GetNumberOfIds());
            }
          InScalars->GetTuples(cellPts,cellScalars);

          //if (dimensionality == 3 &&  ! (cellId % 5000) )
          //{
          //vtkDebugMacro(<<"Contouring #" << cellId);
          //this->UpdateProgress (static_cast<double>(cellId)/numCells);
          //abortExecute = this->GetAbortExecute();
          //}

          for (i=0; i < NumValues; i++)
            {
            helper.Contour(cell,Values[i],cellScalars,cellId);
            } // for all contour values
          } // for all cells
        } // for all dimensions
      cell->Delete();

      cellScalars->Delete();

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

      // -1 == uninitialized. This setting used to be ignored, and we preserve the
      // old behavior for backward compatibility. Normals will be computed here
      // if and only if the user has explicitly set the option.
      if (this->Filter->GetComputeNormals() > 0)
        {
        vtkNew<vtkPolyDataNormals> normalsFilter;
        normalsFilter->SetOutputPointsPrecision(this->Filter->GetOutputPointsPrecision());
        vtkNew<vtkPolyData> tempInput;
        tempInput->ShallowCopy(output.GetPointer());
        normalsFilter->SetInputData(tempInput.GetPointer());
        normalsFilter->SetFeatureAngle(180.);
        normalsFilter->SetUpdateExtent(
          0,
          Info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()),
          Info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()),
          Info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
        normalsFilter->Update();
        output->ShallowCopy(normalsFilter->GetOutput());
        }

      locator->Initialize();//releases leftover memory
      output->Squeeze();

      output->Register(0);
      this->Outputs.Local().push_back(output.GetPointer());
    }

    void Reduce()
    {
      int count = -1;

      vtkSMPThreadLocal<std::vector<vtkPolyData*> >::iterator outIter =
        this->Outputs.begin();
      while(outIter != this->Outputs.end())
        {
        std::vector<vtkPolyData*>& outs = *outIter;
        std::vector<vtkPolyData*>::iterator iter = outs.begin();
        while(iter != outs.end())
          {
          this->Output->SetBlock(++count, *iter);
          (*iter)->Delete();
          ++iter;
          }
        ++outIter;
        }
    }

  };

}

// General contouring filter.  Handles arbitrary input.
//
int vtkSMPContourFilterManyPieces::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    vtkErrorMacro(<< "No output");
    return 0;
    }

  // get the contours
  int numContours=this->ContourValues->GetNumberOfContours();
  double *values=this->ContourValues->GetValues();
  if (numContours < 1)
    {
    vtkDebugMacro(<<"No contour to generate");
    return 0;
    }

  // is there data to process?
  if (!this->GetInputArrayToProcess(0, inputVector))
    {
    return 1;
    }

  int sType = this->GetInputArrayToProcess(0, inputVector)->GetDataType();

  // handle 2D images
  if (vtkImageData::SafeDownCast(input) && sType != VTK_BIT &&
      !vtkUniformGrid::SafeDownCast(input))
    {
    vtkErrorMacro(<< "Many pieces unsupported for "
                  << input->GetClassName());
    return 0;
    } //if image data

  // handle 3D RGrids
  if (vtkRectilinearGrid::SafeDownCast(input) && sType != VTK_BIT)
    {
    vtkErrorMacro(<< "Many pieces unsupported for "
                  << input->GetClassName());
    return 0;
    } // if 3D Rgrid

  // handle 3D SGrids
  if (vtkStructuredGrid::SafeDownCast(input) && sType != VTK_BIT)
    {
    vtkErrorMacro(<< "Many pieces unsupported for "
                  << input->GetClassName());
    return 0;
    } //if 3D SGrid

  vtkDataArray *inScalars;

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) {return 0;}

  vtkDebugMacro(<< "Executing contour filter");
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  if (!this->UseScalarTree)
    {
    inScalars = this->GetInputArrayToProcess(0,inputVector);
    vtkIdType numCells = input->GetNumberOfCells();
    if ( ! inScalars || numCells < 1 )
      {
      vtkDebugMacro(<<"No data to contour");
      return 1;
      }

    input->GetBounds(); //Not thread safe, so compute it here
    input->GetCellType(0); // same

    vtkContourFilterManyPiecesFunctor functor(this, input, inScalars, numContours, values, output, info);
    vtkSMPTools::For(0, numCells, functor);
    }
  else
    {
    if (this->ScalarTree && !vtkSMPMinMaxTree::SafeDownCast(this->ScalarTree))
      {
      this->ScalarTree->Delete();
      this->ScalarTree = NULL;
      }
    if (this->ScalarTree == NULL)
      {
      this->ScalarTree = vtkSMPMinMaxTree::New();
      }
    vtkSMPMinMaxTree* tree = vtkSMPMinMaxTree::SafeDownCast(this->ScalarTree);
    tree->SetDataSet(input);
    tree->InitTraversal(200);
    int level;
    vtkIdType bf;
    tree->GetTreeSize(level, bf);
    inScalars = this->GetInputArrayToProcess(0,inputVector);
    vtkContourFilterManyPiecesFunctor functor(this, input, inScalars, numContours, values, output, info);
    vtkSMPTools2::Traverse(level, bf, tree, functor);
    } //using scalar tree

  return 1;
}

int vtkSMPContourFilterManyPieces::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

void vtkSMPContourFilterManyPieces::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
