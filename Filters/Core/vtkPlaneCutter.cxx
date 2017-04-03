/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPlaneCutter.h"

#include "vtkMath.h"
#include "vtkIdList.h"
#include "vtkCell.h"
#include "vtkGenericCell.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkFlyingEdgesPlaneCutter.h"
#include "vtkNonMergingPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"
#include "vtkPlane.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkSphereTree.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkNew.h"

#include <cmath>
#include <unordered_map>

vtkStandardNewMacro(vtkPlaneCutter);
vtkCxxSetObjectMacro(vtkPlaneCutter,Plane,vtkPlane);

//----------------------------------------------------------------------------
namespace {

  struct vtkLocalDataType
  {
    vtkPolyData* Output;
    vtkNonMergingPointLocator* Locator;

    vtkLocalDataType() : Output(0), Locator(0)
      {
      }
  };

  // This functor uses thread local storage to create one vtkPolyData per
  // thread. Each execution of the functor adds to the vtkPolyData that is
  // local to the thread it is running on.
  struct CuttingFunctor
  {
    vtkDataSet* Input;
    vtkPoints* InPoints;
    vtkDataObject* Output;
    vtkPlane* Plane;
    vtkSphereTree* SphereTree;
    const unsigned char *Selected;

    vtkSMPThreadLocal<vtkDoubleArray*> CellScalars;
    vtkSMPThreadLocalObject<vtkGenericCell> Cell;
    vtkSMPThreadLocalObject<vtkPoints> NewPts;
    vtkSMPThreadLocalObject<vtkCellArray> NewVerts;
    vtkSMPThreadLocalObject<vtkCellArray> NewLines;
    vtkSMPThreadLocalObject<vtkCellArray> NewPolys;

    vtkSMPThreadLocal<vtkLocalDataType> LocalData;

    double* Origin;
    double* Normal;
    vtkIdType NumSelected;

    CuttingFunctor(vtkDataSet* input, vtkDataObject* output, vtkPlane* plane,
                   vtkSphereTree* tree, double *origin, double* normal) :
      Input(input), InPoints(0), Output(output), Plane(plane), SphereTree(tree),
      Origin(origin), Normal(normal)
    {
      this->Selected = this->SphereTree->SelectPlane(this->Origin,this->Normal,
                                                     this->NumSelected);
    }

    virtual ~CuttingFunctor()
    {
      // Cleanup all allocated temporaries
      vtkSMPThreadLocal<vtkDoubleArray*>::iterator cellScalarsIter =
        this->CellScalars.begin();
      while(cellScalarsIter != this->CellScalars.end())
      {
        (*cellScalarsIter)->Delete();
        ++cellScalarsIter;
      }

      vtkSMPThreadLocal<vtkLocalDataType>::iterator dataIter =
        this->LocalData.begin();
      while(dataIter != this->LocalData.end())
      {
        (*dataIter).Output->Delete();
        (*dataIter).Locator->Delete();
        ++dataIter;
      }
    }

    void Initialize()
    {
      // Initialize thread local object before any processing happens.
      // This gets called once per thread.
      vtkLocalDataType& localData = this->LocalData.Local();

      localData.Output = vtkPolyData::New();
      vtkPolyData *output = localData.Output;

      localData.Locator = vtkNonMergingPointLocator::New();
      vtkPointLocator* locator = localData.Locator;

      vtkIdType numCells = this->Input->GetNumberOfCells();

      vtkPoints*& newPts = this->NewPts.Local();
      newPts->SetDataType(VTK_FLOAT);
      output->SetPoints(newPts);

      vtkIdType estimatedSize =
        static_cast<vtkIdType>(sqrt(static_cast<double>(numCells)));
      estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
      estimatedSize = (estimatedSize < 1024 ? 1024 : estimatedSize);

      newPts->Allocate(estimatedSize, estimatedSize);

      // bounds are not important for non-merging locator
      double bounds[6];
      bounds[0]=bounds[2]=bounds[4]=(VTK_FLOAT_MIN);
      bounds[1]=bounds[3]=bounds[5]=(VTK_FLOAT_MAX);
      locator->InitPointInsertion (newPts, bounds,
                                   this->Input->GetNumberOfPoints());

      vtkCellArray*& newVerts = this->NewVerts.Local();
      newVerts->Allocate(estimatedSize,estimatedSize);
      output->SetVerts(newVerts);

      vtkCellArray*& newLines = this->NewLines.Local();
      newLines->Allocate(estimatedSize,estimatedSize);
      output->SetLines(newLines);

      vtkCellArray*& newPolys = this->NewPolys.Local();
      newPolys->Allocate(estimatedSize,estimatedSize);
      output->SetPolys(newPolys);

      vtkDoubleArray*& cellScalars = this->CellScalars.Local();
      cellScalars = vtkDoubleArray::New();
      cellScalars->SetNumberOfComponents(1);
      cellScalars->Allocate(VTK_CELL_SIZE);

      vtkPointData* outPd = output->GetPointData();
      vtkCellData* outCd = output->GetCellData();
      vtkPointData* inPd = this->Input->GetPointData();
      vtkCellData* inCd = this->Input->GetCellData();
      outPd->InterpolateAllocate(inPd, estimatedSize, estimatedSize);
      outCd->CopyAllocate(inCd, estimatedSize, estimatedSize);
    }

    void Reduce()
    {
      // Create the final multi-block dataset
      vtkNew<vtkMultiPieceDataSet> mp;
      int count = 0;

      vtkSMPThreadLocal<vtkLocalDataType>::iterator outIter =
        this->LocalData.begin();
      while(outIter != this->LocalData.end())
      {
        vtkPolyData* output = (*outIter).Output;
        mp->SetPiece(count++, output);
        ++outIter;
      }

      vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::SafeDownCast(this->Output);
      if (output)
      {
        output->SetBlock(0, mp.GetPointer());
      }
    }
  };

  // Process unstructured grids
  struct UnstructuredFunctor : public CuttingFunctor
  {
    UnstructuredFunctor(vtkDataSet* input, vtkDataObject* output, vtkPlane* plane,
                        vtkSphereTree* tree, double *origin, double* normal) :
      CuttingFunctor(input, output, plane, tree, origin, normal)
    {
      vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(input);
      this->InPoints = ugrid->GetPoints();
    }

    void Initialize()
    {
      CuttingFunctor::Initialize();
    }

    void operator()(vtkIdType cellId, vtkIdType endCellId)
    {
      // Actual computation.
      // Note the usage of thread local objects. These objects
      // persist for each thread across multiple execution of the
      // functor.
      vtkLocalDataType& localData = this->LocalData.Local();
      vtkPointLocator* loc = localData.Locator;

      vtkGenericCell* cell = this->Cell.Local();
      vtkDoubleArray* cellScalars = this->CellScalars.Local();
      vtkPointData* inPD = this->Input->GetPointData();
      vtkCellData* inCD = this->Input->GetCellData();

      vtkPolyData* output = localData.Output;
      vtkPointData* outPD = output->GetPointData();
      vtkCellData* outCD = output->GetCellData();

      vtkCellArray* newVerts = this->NewVerts.Local();
      vtkCellArray* newLines = this->NewLines.Local();
      vtkCellArray* newPolys = this->NewPolys.Local();

      double *s;
      int i, numPts;
      vtkPoints *cellPoints;
      const unsigned char *selected = this->Selected + cellId;

      // Loop over the cell spheres, processing those cells whose
      // bounding sphere intersect with the plane.
      for ( ; cellId < endCellId; ++cellId )
      {
        if ( *selected++ )
        {
          this->Input->GetCell(cellId, cell);
          numPts = cell->GetNumberOfPoints();
          cellScalars->SetNumberOfTuples(numPts);
          s = cellScalars->GetPointer(0);
          cellPoints = cell->GetPoints();
          for ( i=0; i < numPts; i++ )
          {
            *s++ = this->Plane->FunctionValue(cellPoints->GetPoint(i));
          }
          cell->Contour(0.0, cellScalars, loc, newVerts, newLines, newPolys,
                        inPD, outPD, inCD, cellId, outCD);
        }
      }//for all cells which intersect plane in this task
    }

    void Reduce()
    {
      CuttingFunctor::Reduce();
    }
  };

  // Process structured grids
  //=============================================================================
  typedef int EDGE_LIST;
  struct vtkPlaneCutCases
  {
    EDGE_LIST edges[16];
  };

  //=============================================================================
  //
  // Edges to intersect hexes (i.e., structured grid) assuming a plane cut. At
  // most one polygon can be created. Comments at end of line indicate marching
  // cubes case number (0->255) and base case number (0->15).  Converted
  // regular case table to reflect only plane cuts (i.e., single planar
  // polygon). Marked eliminated triangles by adding 10 and negating: 2 -> -12.
  static vtkPlaneCutCases  VTK_PLANE_CUT_CASES[] = {
  {{-1, -1, -1, -1, -1, -1, -1}}, /* 0 0 */
  {{ 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 1 1 */
  {{ 0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 2 1 */
  {{ 1, 3, 8, 9, -10, -8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 3 2 */
  {{ 1, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 4 1 */
  {{ -10, 3, 8, 1, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 5 3 */
  {{ -19, 11, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 6 2 */
  {{ -12, 3, 8, 2, 8, 11, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1}}, /* 7 5 */
  {{ 3, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 8 1 */
  {{ -10, 2, 10, 8, 0, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 9 2 */
  {{ -11, 0, 9, 2, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 10 3 */
  {{ 1, 2, 10, 8, 9, -1, -1}}, /* 11 5 */
  {{ 3, 1, 10, 11, -13, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 12 2 */
  {{ 0, 1, 11, 10, 8, -1, -1}}, /* 13 5 */
  {{ 3, 0, 9, 11, 10, -1, -1}}, /* 14 5 */
    {{ 9, 11, 8, 11, 10, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 15 8 */
  {{ 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 16 1 */
  {{ 4, 0, 3, 7, -14, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 17 2 */
  {{ -10, 9, 1, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 18 3 */
  {{ 4, 9, 1, 3, 7, -1, -1}}, /* 19 5 */
  {{ -11, 11, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 20 4 */
  {{ -13, 7, 4, 3, 4, 0, 1, 11, 2, -1, -1, -1, -1, -1, -1, -1}}, /* 21 7 */
  {{ -19, 11, 2, 9, 2, 0, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1}}, /* 22 7 */
    {{ 2, 9, 11, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1}}, /* 23 14 */
  {{ -18, 7, 4, 3, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 24 3 */
  {{10, 7, 4, 10, 2, -1, -1}}, /* 25 5 */
  {{ -19, 1, 0, 8, 7, 4, 2, 10, 3, -1, -1, -1, -1, -1, -1, -1}}, /* 26 6 */
    {{ 4, 10, 7, 9, 10, 4, 9, 2, 10, 9, 1, 2, -1, -1, -1, -1}}, /* 27 9 */
  {{ -13, 1, 11, 3, 11, 10, 7, 4, 8, -1, -1, -1, -1, -1, -1, -1}}, /* 28 7 */
    {{ 1, 11, 10, 1, 10, 4, 1, 4, 0, 7, 4, 10, -1, -1, -1, -1}}, /* 29 11 */
  {{ -14, 8, 7, 9, 10, 0, 9, 11, 10, 10, 3, 0, -1, -1, -1, -1}}, /* 30 12 */
  {{ 4, 10, 7, 11, 9, -1, -1}}, /* 31 5 */
  {{ 9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 32 1 */
  {{ -19, 4, 5, 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 33 3 */
  {{ 0, 4, 5, 1, -10, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 34 2 */
  {{ 8, 4, 5, 1, 3, -1, -1}}, /* 35 5 */
  {{ -11, 11, 2, 9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 36 3 */
  {{ -13, 8, 0, 1, 11, 2, 4, 5, 9, -1, -1, -1, -1, -1, -1, -1}}, /* 37 6 */
  {{ 5, 11, 2, 0, 4, -1, -1}}, /* 38 5 */
    {{ 2, 5, 11, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1}}, /* 39 9 */
  {{ -19, 4, 5, 2, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 40 4 */
  {{ -10, 2, 10, 0, 10, 8, 4, 5, 9, -1, -1, -1, -1, -1, -1, -1}}, /* 41 7 */
  {{ -10, 4, 5, 0, 5, 1, 2, 10, 3, -1, -1, -1, -1, -1, -1, -1}}, /* 42 7 */
    {{ 2, 5, 1, 2, 8, 5, 2, 10, 8, 4, 5, 8, -1, -1, -1, -1}}, /* 43 11 */
  {{-21, 10, 3, 11, 3, 1, 9, 4, 5, -1, -1, -1, -1, -1, -1, -1}}, /* 44 7 */
  {{ -14, 5, 9, 0, 1, 8, 8, 1, 11, 8, 11, 10, -1, -1, -1, -1}}, /* 45 12 */
    {{ 5, 0, 4, 5, 10, 0, 5, 11, 10, 10, 3, 0, -1, -1, -1, -1}}, /* 46 14 */
  {{ 5, 11, 10, 8, 4, -1, -1}}, /* 47 5 */
  {{ 9, 8, 7, 5, -19, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 48 2 */
  {{ 9, 0, 3, 7, 5, -1, -1}}, /* 49 5 */
  {{ 8, 7, 5, 1, 0, -1, -1}}, /* 50 5 */
    {{ 1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 51 8 */
  {{ -19, 8, 7, 9, 7, 5, 11, 2, 1, -1, -1, -1, -1, -1, -1, -1}}, /* 52 7 */
  {{-21, 2, 1, 9, 0, 5, 5, 0, 3, 5, 3, 7, -1, -1, -1, -1}}, /* 53 12 */
    {{ 8, 2, 0, 8, 5, 2, 8, 7, 5, 11, 2, 5, -1, -1, -1, -1}}, /* 54 11 */
  {{ 11, 2, 3, 7, 5, -1, -1}}, /* 55 5 */
  {{ -17, 5, 9, 7, 9, 8, 3, 2, 10, -1, -1, -1, -1, -1, -1, -1}}, /* 56 7 */
    {{ 9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 10, 7, -1, -1, -1, -1}}, /* 57 14 */
  {{ -12, 10, 3, 0, 8, 1, 1, 8, 7, 1, 7, 5, -1, -1, -1, -1}}, /* 58 12 */
  {{  1, 2, 10, 7, 5, -1, -1}}, /* 59 5 */
  {{ -19, 8, 5, 8, 7, 5, 11, 3, 1, 11, 10, 3, -1, -1, -1, -1}}, /* 60 10 */
  {{ -15, 0, 7, 5, 9, 0, 7, 0, 10, 1, 11, 0, 10, 0, 11, -1}}, /* 61 7 */
  {{-20, 0, 11, 10, 3, 0, 11, 0, 5, 8, 7, 0, 5, 0, 7, -1}}, /* 62 7 */
  {{10, 5, 7, 11, -15, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 63 2 */
  {{11, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 64 1 */
  {{ -10, 3, 8, 5, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 65 4 */
  {{ -19, 1, 0, 5, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 66 3 */
  {{ -11, 3, 8, 1, 8, 9, 5, 6, 11, -1, -1, -1, -1, -1, -1, -1}}, /* 67 7 */
  {{ 1, 5, 6, 2, -11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 68 2 */
  {{ -11, 5, 6, 1, 6, 2, 3, 8, 0, -1, -1, -1, -1, -1, -1, -1}}, /* 69 7 */
  {{  9, 5, 6, 2, 0, -1, -1}}, /* 70 5 */
    {{ 5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1}}, /* 71 11 */
  {{ -12, 10, 3, 11, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 72 3 */
  {{-20, 8, 0, 10, 0, 2, 11, 5, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 73 7 */
  {{ -10, 9, 1, 2, 10, 3, 5, 6, 11, -1, -1, -1, -1, -1, -1, -1}}, /* 74 6 */
  {{ -15, 6, 11, 1, 2, 9, 9, 2, 10, 9, 10, 8, -1, -1, -1, -1}}, /* 75 12 */
  {{ 6, 10, 3, 1, 5, -1, -1}}, /* 76 5 */
    {{ 0, 10, 8, 0, 5, 10, 0, 1, 5, 5, 6, 10, -1, -1, -1, -1}}, /* 77 14 */
    {{ 3, 6, 10, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1}}, /* 78 9 */
  {{ 6, 9, 8, 10, 5, -1, -1}}, /* 79 5 */
  {{ -15, 6, 11, 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 80 3 */
  {{ -14, 0, 3, 4, 3, 7, 6, 11, 5, -1, -1, -1, -1, -1, -1, -1}}, /* 81 7 */
  {{ -11, 0, 9, 5, 6, 11, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1}}, /* 82 6 */
  {{-21, 5, 6, 1, 7, 9, 1, 3, 7, 7, 4, 9, -1, -1, -1, -1}}, /* 83 12 */
  {{ -16, 2, 1, 6, 1, 5, 4, 8, 7, -1, -1, -1, -1, -1, -1, -1}}, /* 84 7 */
  {{ -11, 5, 2, 5, 6, 2, 3, 4, 0, 3, 7, 4, -1, -1, -1, -1}}, /* 85 10 */
  {{ -18, 7, 4, 9, 5, 0, 0, 5, 6, 0, 6, 2, -1, -1, -1, -1}}, /* 86 12 */
  {{ -17, 9, 3, 7, 4, 9, 3, 9, 2, 5, 6, 9, 2, 9, 6, -1}}, /* 87 7 */
  {{ -13, 2, 10, 7, 4, 8, 11, 5, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 88 6 */
  {{ -15, 6, 11, 4, 2, 7, 4, 0, 2, 2, 10, 7, -1, -1, -1, -1}}, /* 89 12 */
  {{ -10, 9, 1, 4, 8, 7, 2, 10, 3, 5, 6, 11, -1, -1, -1, -1}}, /* 90 13 */
  {{ -19, 1, 2, 9, 2, 10, 9, 10, 4, 7, 4, 10, 5, 6, 11, -1}}, /* 91 6 */
  {{ -18, 7, 4, 3, 5, 10, 3, 1, 5, 5, 6, 10, -1, -1, -1, -1}}, /* 92 12 */
  {{ -15, 10, 1, 5, 6, 10, 1, 10, 0, 7, 4, 10, 0, 10, 4, -1}}, /* 93 7 */
  {{ -10, 9, 5, 0, 5, 6, 0, 6, 3, 10, 3, 6, 8, 7, 4, -1}}, /* 94 6 */
  {{ 9, 5, 6, 10, 7, 4, -1}}, /* 95 3 */
  {{11, 9, 4, 6, -21, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 96 2 */
  {{ -14, 6, 11, 4, 11, 9, 0, 3, 8, -1, -1, -1, -1, -1, -1, -1}}, /* 97 7 */
  {{ 11, 1, 0, 4, 6, -1, -1}}, /* 98 5 */
    {{ 8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 11, 1, -1, -1, -1, -1}}, /* 99 14 */
  {{ 1, 9, 4, 6, 2, -1, -1}}, /* 100 5 */
  {{ -13, 8, 0, 1, 9, 2, 2, 9, 4, 2, 4, 6, -1, -1, -1, -1}}, /* 101 12 */
    {{ 0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 102 8 */
  {{ 2, 3, 8, 4, 6, -1, -1}}, /* 103 5 */
  {{-21, 9, 4, 11, 4, 6, 10, 3, 2, -1, -1, -1, -1, -1, -1, -1}}, /* 104 7 */
  {{ -10, 2, 8, 2, 10, 8, 4, 11, 9, 4, 6, 11, -1, -1, -1, -1}}, /* 105 10 */
  {{ -13, 2, 10, 0, 6, 1, 0, 4, 6, 6, 11, 1, -1, -1, -1, -1}}, /* 106 12 */
  {{ -16, 1, 4, 6, 11, 1, 4, 1, 8, 2, 10, 1, 8, 1, 10, -1}}, /* 107 7 */
    {{ 9, 4, 6, 9, 6, 3, 9, 3, 1, 10, 3, 6, -1, -1, -1, -1}}, /* 108 11 */
  {{ -18, 1, 10, 8, 0, 1, 10, 1, 6, 9, 4, 1, 6, 1, 4, -1}}, /* 109 7 */
  {{ 6, 10, 3, 0, 4, -1, -1}}, /* 110 5 */
  {{ 6, 10, 8, 4, -18, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 111 2 */
  {{ 7, 6, 11, 9, 8, -1, -1}}, /* 112 5 */
    {{ 0, 3, 7, 0, 7, 11, 0, 11, 9, 6, 11, 7, -1, -1, -1, -1}}, /* 113 11 */
    {{11, 7, 6, 1, 7, 11, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1}}, /* 114 9 */
  {{ 7, 6, 11, 1, 3, -1, -1}}, /* 115 5 */
    {{ 1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1}}, /* 116 14 */
  {{ -12, 9, 6, 2, 1, 9, 6, 9, 7, 0, 3, 9, 7, 9, 3, -1}}, /* 117 7 */
  {{ 0, 8, 7, 6, 2, -1, -1}}, /* 118 5 */
  {{ 7, 6, 2, 3, -12, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 119 2 */
  {{ -12, 10, 3, 11, 8, 6, 11, 9, 8, 8, 7, 6, -1, -1, -1, -1}}, /* 120 12 */
  {{ -12, 7, 0, 2, 10, 7, 0, 7, 9, 6, 11, 7, 9, 7, 11, -1}}, /* 121 7 */
  {{ -11, 0, 8, 1, 8, 7, 1, 7, 11, 6, 11, 7, 2, 10, 3, -1}}, /* 122 6 */
  {{  1, 2, 10, 7, 6, 11, -1}}, /* 123 3 */
  {{ -18, 6, 9, 8, 7, 6, 9, 6, 1, 10, 3, 6, 1, 6, 3, -1}}, /* 124 7 */
  {{ -10, 1, 9, 10, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 125 4 */
  {{  0, 8, 7, 6, 10, 3, -1}}, /* 126 3 */
  {{ 7, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 127 1 */
  {{ 7, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 128 1 */
  {{ -13, 8, 0, 10, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 129 3 */
  {{ -10, 9, 1, 10, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 130 4 */
  {{ -18, 9, 1, 8, 1, 3, 10, 6, 7, -1, -1, -1, -1, -1, -1, -1}}, /* 131 7 */
  {{ -21, 2, 1, 6, 7, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 132 3 */
  {{ -11, 11, 2, 3, 8, 0, 6, 7, 10, -1, -1, -1, -1, -1, -1, -1}}, /* 133 6 */
  {{ -12, 0, 9, 2, 9, 11, 6, 7, 10, -1, -1, -1, -1, -1, -1, -1}}, /* 134 7 */
  {{ -16, 7, 10, 2, 3, 11, 11, 3, 8, 11, 8, 9, -1, -1, -1, -1}}, /* 135 12 */
  {{ 7, 3, 2, 6, -17, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 136 2 */
  {{ 7, 8, 0, 2, 6, -1, -1}}, /* 137 5 */
  {{ -12, 6, 7, 2, 7, 3, 0, 9, 1, -1, -1, -1, -1, -1, -1, -1}}, /* 138 7 */
    {{ 1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1}}, /* 139 14 */
  {{ 11, 6, 7, 3, 1, -1, -1}}, /* 140 5 */
    {{11, 6, 7, 1, 11, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1}}, /* 141 9 */
    {{ 0, 7, 3, 0, 11, 7, 0, 9, 11, 6, 7, 11, -1, -1, -1, -1}}, /* 142 11 */
  {{ 11, 6, 7, 8, 9, -1, -1}}, /* 143 5 */
  {{ 6, 4, 8, 10, -16, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 144 2 */
  {{ 3, 10, 6, 4, 0, -1, -1}}, /* 145 5 */
  {{ -18, 10, 6, 8, 6, 4, 9, 1, 0, -1, -1, -1, -1, -1, -1, -1}}, /* 146 7 */
    {{ 9, 6, 4, 9, 3, 6, 9, 1, 3, 10, 6, 3, -1, -1, -1, -1}}, /* 147 11 */
  {{ -16, 4, 8, 6, 8, 10, 2, 1, 11, -1, -1, -1, -1, -1, -1, -1}}, /* 148 7 */
  {{ -11, 11, 2, 3, 10, 0, 0, 10, 6, 0, 6, 4, -1, -1, -1, -1}}, /* 149 12 */
  {{ -14, 8, 10, 4, 10, 6, 0, 9, 2, 2, 9, 11, -1, -1, -1, -1}}, /* 150 10 */
  {{-21, 3, 9, 11, 2, 3, 9, 3, 4, 10, 6, 3, 4, 3, 6, -1}}, /* 151 7 */
  {{ 8, 3, 2, 6, 4, -1, -1}}, /* 152 5 */
    {{ 0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 153 8 */
  {{ -11, 0, 9, 2, 4, 3, 2, 6, 4, 4, 8, 3, -1, -1, -1, -1}}, /* 154 12 */
  {{ 4, 9, 1, 2, 6, -1, -1}}, /* 155 5 */
    {{ 8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 11, -1, -1, -1, -1}}, /* 156 14 */
  {{ 0, 1, 11, 6, 4, -1, -1}}, /* 157 5 */
  {{ -14, 3, 6, 4, 8, 3, 6, 3, 11, 0, 9, 3, 11, 3, 9, -1}}, /* 158 7 */
  {{11, 6, 4, 9, -14, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 159 2 */
  {{ -14, 5, 9, 7, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 160 3 */
  {{ -10, 3, 8, 4, 5, 9, 10, 6, 7, -1, -1, -1, -1, -1, -1, -1}}, /* 161 6 */
  {{ -15, 1, 0, 5, 0, 4, 7, 10, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 162 7 */
  {{-20, 6, 7, 8, 4, 3, 3, 4, 5, 3, 5, 1, -1, -1, -1, -1}}, /* 163 12 */
  {{ -19, 4, 5, 11, 2, 1, 7, 10, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 164 6 */
  {{ -16, 7, 10, 1, 11, 2, 0, 3, 8, 4, 5, 9, -1, -1, -1, -1}}, /* 165 13 */
  {{ -17, 10, 6, 5, 11, 4, 4, 11, 2, 4, 2, 0, -1, -1, -1, -1}}, /* 166 12 */
  {{ -13, 8, 4, 3, 4, 5, 3, 5, 2, 11, 2, 5, 10, 6, 7, -1}}, /* 167 6 */
  {{ -17, 3, 2, 7, 2, 6, 5, 9, 4, -1, -1, -1, -1, -1, -1, -1}}, /* 168 7 */
  {{ -19, 4, 5, 0, 6, 8, 0, 2, 6, 6, 7, 8, -1, -1, -1, -1}}, /* 169 12 */
  {{ -13, 2, 6, 3, 6, 7, 1, 0, 5, 5, 0, 4, -1, -1, -1, -1}}, /* 170 10 */
  {{ -16, 8, 2, 6, 7, 8, 2, 8, 1, 4, 5, 8, 1, 8, 5, -1}}, /* 171 7 */
  {{ -19, 4, 5, 11, 6, 1, 1, 6, 7, 1, 7, 3, -1, -1, -1, -1}}, /* 172 12 */
  {{ -11, 11, 6, 1, 6, 7, 1, 7, 0, 8, 0, 7, 9, 4, 5, -1}}, /* 173 6 */
  {{ -14, 11, 0, 4, 5, 11, 0, 11, 3, 6, 7, 11, 3, 11, 7, -1}}, /* 174 7 */
  {{  11, 6, 7, 8, 4, 5, -1}}, /* 175 3 */
  {{  6, 5, 9, 8, 10, -1, -1}}, /* 176 5 */
    {{ 3, 10, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1}}, /* 177 9 */
    {{ 0, 8, 10, 0, 10, 5, 0, 5, 1, 5, 10, 6, -1, -1, -1, -1}}, /* 178 14 */
  {{ 3, 10, 6, 5, 1, -1, -1}}, /* 179 5 */
  {{ -11, 11, 2, 9, 10, 5, 9, 8, 10, 10, 6, 5, -1, -1, -1, -1}}, /* 180 12 */
  {{ -10, 3, 10, 0, 10, 6, 0, 6, 9, 5, 9, 6, 1, 11, 2, -1}}, /* 181 6 */
  {{-20, 5, 8, 10, 6, 5, 8, 5, 0, 11, 2, 5, 0, 5, 2, -1}}, /* 182 7 */
  {{ 3, 10, 6, 5, 11, 2, -1}}, /* 183 3 */
    {{ 5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1}}, /* 184 11 */
  {{ 6, 5, 9, 0, 2, -1, -1}}, /* 185 5 */
  {{ -11, 8, 5, 1, 0, 8, 5, 8, 6, 3, 2, 8, 6, 8, 2, -1}}, /* 186 7 */
  {{ 1, 2, 6, 5, -16, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 187 2 */
  {{ -11, 6, 3, 1, 11, 6, 3, 6, 8, 5, 9, 6, 8, 6, 9, -1}}, /* 188 7 */
  {{ 0, 1, 11, 6, 5, 9, -1}}, /* 189 3 */
  {{ -10, 8, 3, 5, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 190 4 */
  {{11, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 191 1 */
  {{10, 11, 5, 7, -20, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 192 2 */
  {{-20, 11, 5, 10, 5, 7, 8, 0, 3, -1, -1, -1, -1, -1, -1, -1}}, /* 193 7 */
  {{ -15, 7, 10, 5, 10, 11, 1, 0, 9, -1, -1, -1, -1, -1, -1, -1}}, /* 194 7 */
  {{-21, 5, 7, 11, 7, 10, 9, 1, 8, 8, 1, 3, -1, -1, -1, -1}}, /* 195 10 */
  {{ 10, 2, 1, 5, 7, -1, -1}}, /* 196 5 */
  {{ -10, 3, 8, 1, 7, 2, 1, 5, 7, 7, 10, 2, -1, -1, -1, -1}}, /* 197 12 */
    {{ 9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 10, -1, -1, -1, -1}}, /* 198 14 */
  {{ -17, 2, 5, 7, 10, 2, 5, 2, 9, 3, 8, 2, 9, 2, 8, -1}}, /* 199 7 */
  {{ 2, 11, 5, 7, 3, -1, -1}}, /* 200 5 */
    {{ 8, 0, 2, 8, 2, 5, 8, 5, 7, 11, 5, 2, -1, -1, -1, -1}}, /* 201 11 */
  {{ -19, 1, 0, 5, 3, 11, 5, 7, 3, 3, 2, 11, -1, -1, -1, -1}}, /* 202 12 */
  {{ -19, 2, 8, 9, 1, 2, 8, 2, 7, 11, 5, 2, 7, 2, 5, -1}}, /* 203 7 */
    {{ 1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 204 8 */
  {{ 7, 8, 0, 1, 5, -1, -1}}, /* 205 5 */
  {{ 3, 0, 9, 5, 3, -1, -1}}, /* 206 5 */
  {{ 9, 5, 7, 8, -17, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 207 2 */
  {{ 5, 4, 8, 10, 11, -1, -1}}, /* 208 5 */
    {{ 5, 4, 0, 5, 0, 10, 5, 10, 11, 10, 0, 3, -1, -1, -1, -1}}, /* 209 14 */
  {{ -10, 9, 1, 8, 11, 4, 8, 10, 11, 11, 5, 4, -1, -1, -1, -1}}, /* 210 12 */
  {{-21, 4, 10, 11, 5, 4, 10, 4, 3, 9, 1, 4, 3, 4, 1, -1}}, /* 211 7 */
    {{ 2, 1, 5, 2, 5, 8, 2, 8, 10, 4, 8, 5, -1, -1, -1, -1}}, /* 212 11 */
  {{ -10, 10, 4, 0, 3, 10, 4, 10, 5, 2, 1, 10, 5, 10, 1, -1}}, /* 213 7 */
  {{ -10, 5, 2, 0, 9, 5, 2, 5, 10, 4, 8, 5, 10, 5, 8, -1}}, /* 214 7 */
  {{ -19, 5, 4, 2, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 215 4 */
    {{ 2, 11, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1}}, /* 216 9 */
  {{ 2, 11, 5, 4, 0, -1, -1}}, /* 217 5 */
  {{ -13, 2, 11, 3, 11, 5, 3, 5, 8, 4, 8, 5, 0, 9, 1, -1}}, /* 218 6 */
  {{ 2, 11, 5, 4, 9, 1, -1}}, /* 219 3 */
  {{ 5, 4, 8, 3, 1, -1, -1}}, /* 220 5 */
  {{ 0, 1, 5, 4, -11, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 221 2 */
  {{ 5, 4, 8, 3, 0, 9, -1}}, /* 222 3 */
  {{ 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 223 1 */
  {{ 4, 7, 10, 11, 9, -1, -1}}, /* 224 5 */
  {{ -10, 3, 8, 4, 7, 9, 9, 7, 10, 9, 10, 11, -1, -1, -1, -1}}, /* 225 12 */
    {{ 1, 10, 11, 1, 4, 10, 1, 0, 4, 7, 10, 4, -1, -1, -1, -1}}, /* 226 11 */
  {{ -13, 4, 1, 3, 8, 4, 1, 4, 11, 7, 10, 4, 11, 4, 10, -1}}, /* 227 7 */
    {{ 4, 7, 10, 9, 4, 10, 9, 10, 2, 9, 2, 1, -1, -1, -1, -1}}, /* 228 9 */
  {{ -19, 4, 7, 9, 7, 10, 9, 10, 1, 2, 1, 10, 0, 3, 8, -1}}, /* 229 6 */
  {{ 4, 7, 10, 2, 0, -1, -1}}, /* 230 5 */
  {{4, 7, 10, 2, 3, 8, -1}}, /* 231 3 */
    {{ 2, 11, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1}}, /* 232 14 */
  {{ -19, 7, 11, 9, 4, 7, 11, 7, 2, 8, 0, 7, 2, 7, 0, -1}}, /* 233 7 */
  {{ -13, 11, 7, 3, 2, 11, 7, 11, 4, 1, 0, 11, 4, 11, 0, -1}}, /* 234 7 */
  {{ -11, 2, 11, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 235 4 */
  {{ 1, 9, 4, 7, 3, -1, -1}}, /* 236 5 */
  {{ 1, 9, 4, 7, 8, 0, -1}}, /* 237 3 */
  {{ 4, 7, 3, 0, -13, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 238 2 */
  {{ 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 239 1 */
    {{ 9, 8, 11, 11, 8, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 240 8 */
  {{ 9, 0, 3, 10, 11, -1, -1}}, /* 241 5 */
  {{ 11, 1, 0, 8, 10, -1, -1}}, /* 242 5 */
  {{ 3, 10, 11, 1, -21, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 243 2 */
  {{ 10, 2, 1, 9, 8, -1, -1}}, /* 244 5 */
  {{ 9, 0, 3, 10, 2, 1, -1}}, /* 245 3 */
  {{ 0, 8, 10, 2, -20, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 246 2 */
  {{ 3, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 247 1 */
  {{ 8, 3, 2, 11, 9, -1, -1}}, /* 248 5 */
  {{ 9, 0, 2, 11, -10, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 249 2 */
  {{ 8, 3, 2, 11, 1, 0, -1}}, /* 250 3 */
  {{ 1, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 251 1 */
  {{ 1, 9, 8, 3, -19, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 252 2 */
  {{ 0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 253 1 */
  {{ 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 254 1 */
  {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}}; /* 255 0 */


  //----------------------------------------------------------------------------
  static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                              {4,5}, {5,6}, {7,6}, {4,7},
                              {0,4}, {1,5}, {3,7}, {2,6}};

  template <typename T>
  void CutStructuredGrid(T *pts, vtkIdType ptId, vtkIdType cellId, int dims[3],
                         vtkIdType sliceOffset, vtkPoints *newPts, vtkCellArray *polys,
                         vtkPointData *inPd, vtkPointData *outPd, vtkCellData *inCd,
                         vtkCellData *outCd, const double *planeOrigin, const double *planeNormal)
  {
    static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
    EDGE_LIST  *edge;
    int i, j, index, *vert;

    // Here we have to retrieve the cell points and cell ids and do the hard work
    int v1, v2, newCellId;
    vtkIdType newIds[3];
    T t, *x1, *x2, x[3], deltaScalar;
    vtkIdType p1, p2;
    vtkIdType cellIds[8];
    double s[8];

    cellIds[0] = ptId;
    cellIds[1] = cellIds[0] + 1;
    cellIds[2] = cellIds[0] + 1 + dims[0];
    cellIds[3] = cellIds[0] + dims[0];
    cellIds[4] = cellIds[0] + sliceOffset;
    cellIds[5] = cellIds[1] + sliceOffset;
    cellIds[6] = cellIds[2] + sliceOffset;
    cellIds[7] = cellIds[3] + sliceOffset;

    // Get the points
    T *cellPts[8];
    for ( i=0; i<8; ++i )
    {
      cellPts[i] = pts + 3*cellIds[i];
      s[i] = (cellPts[i][0]-planeOrigin[0])*planeNormal[0] +
        (cellPts[i][1]-planeOrigin[1])*planeNormal[1] +
        (cellPts[i][2]-planeOrigin[2])*planeNormal[2];
    }

    // Return if we are not producing anything
    if ( (s[0] >= 0.0 && s[1] >= 0.0 && s[2] >= 0.0 && s[3] >= 0.0 &&
          s[4] >= 0.0 && s[5] >= 0.0 && s[6] >= 0.0 && s[7] >= 0.0) ||
         (s[0] < 0.0 && s[1] < 0.0 && s[2] < 0.0 && s[3] < 0.0 &&
          s[4] < 0.0 && s[5] < 0.0 && s[6] < 0.0 && s[7] < 0.0) )
    {
      return;
    }

    // Build the case table and start producing sn output polygon as necessary
    for ( i=0, index=0; i < 8; ++i)
    {
      if (s[i] >= 0.0)
      {
        index |= CASE_MASK[i];
      }
    }

    vtkPlaneCutCases *polyCase = VTK_PLANE_CUT_CASES + index;
    edge = polyCase->edges;

    // Produce the intersections
    for ( ; edge[0] > -1; edge += 3 )
    {
      for (i=0; i<3; i++) // insert triangle
      {
        vert = edges[edge[i]];
        deltaScalar = s[vert[1]] - s[vert[0]];
        v1 = vert[0]; v2 = vert[1];

        // linear interpolation
        t = ( deltaScalar == 0.0 ? 0.0 :
              (-s[v1]) / deltaScalar );

        x1 = cellPts[v1];
        x2 = cellPts[v2];

        for (j=0; j<3; j++)
        {
          x[j] = x1[j] + t * (x2[j] - x1[j]);
        }
        if ( (newIds[i] = newPts->InsertNextPoint(x)) >= 0 )
        {
          if ( outPd )
          {
            p1 = cellIds[v1];
            p2 = cellIds[v2];
            outPd->InterpolateEdge(inPd,newIds[i],p1,p2,t);
          }
        }
      }

      // insert polygon
      newCellId = polys->InsertNextCell(3,newIds);
      outCd->CopyData(inCd,cellId,newCellId);
    }//for all edges
  }

  // Process structured grids
  struct StructuredFunctor : public CuttingFunctor
  {
    int PointsType;

    StructuredFunctor(vtkDataSet* input, vtkDataObject* output, vtkPlane* plane,
                      vtkSphereTree* tree, double *origin, double* normal) :
      CuttingFunctor(input, output, plane, tree, origin, normal)
    {
      vtkStructuredGrid *sgrid = vtkStructuredGrid::SafeDownCast(input);
      this->InPoints = sgrid->GetPoints();
      this->PointsType = this->InPoints->GetDataType();
    }

    void Initialize()
    {
      CuttingFunctor::Initialize();
    }

    void operator()(vtkIdType cellId, vtkIdType endCellId)
    {
      // Actual computation.
      // Note the usage of thread local objects. These objects
      // persist for each thread across multiple execution of the
      // functor.
      vtkLocalDataType& localData = this->LocalData.Local();
      vtkPointLocator* loc = localData.Locator;
      vtkPoints* newPoints = loc->GetPoints();

      vtkPointData* inPD = this->Input->GetPointData();
      vtkCellData* inCD = this->Input->GetCellData();

      vtkPolyData* output = localData.Output;
      vtkPointData* outPD = output->GetPointData();
      vtkCellData* outCD = output->GetCellData();

      vtkCellArray* newPolys = this->NewPolys.Local();

      // Loop over the cell spheres, processing those cells whose
      // bounding sphere intersect with the plane.
      vtkIdType i, j, k;
      vtkIdType ptId;
      int dims[3], cellDims[3];
      vtkStructuredGrid *sgrid = vtkStructuredGrid::SafeDownCast(this->Input);
      sgrid->GetDimensions(dims);
      cellDims[0]=dims[0]-1; cellDims[1]=dims[1]-1; cellDims[2]=dims[2]-1;
      vtkIdType sliceOffset=static_cast<vtkIdType>(dims[0])*dims[1];
      vtkIdType cellSliceOffset=static_cast<vtkIdType>(cellDims[0])*cellDims[1];
      double *planeOrigin=this->Origin;
      double *planeNormal=this->Normal;
      void *ptsPtr = this->InPoints->GetVoidPointer(0);
      const unsigned char *selected = this->Selected + cellId;

      // Traverse this batch of cells (whose bounding sphere possibly
      // intersects the plane).
      for ( ; cellId < endCellId; ++cellId )
      {
        if ( *selected++ )
        {
          i = cellId % cellDims[0];
          j = (cellId / cellDims[0]) % cellDims[1];
          k = cellId / cellSliceOffset;
          ptId = i+ j*dims[0] + k*sliceOffset;
          if ( this->PointsType == VTK_FLOAT )
          {
            CutStructuredGrid<float>(static_cast<float*>(ptsPtr), ptId, cellId,
                                     dims, sliceOffset, newPoints, newPolys,
                                     inPD, outPD, inCD, outCD, planeOrigin, planeNormal);
          }
          else //double
          {
            CutStructuredGrid<double>(static_cast<double*>(ptsPtr), ptId, cellId,
                                      dims, sliceOffset, newPoints, newPolys,
                                      inPD, outPD, inCD, outCD, planeOrigin, planeNormal);
          }
        }//for all selected cells
      }//for each cell
    }//operator()

    void Reduce()
    {
      CuttingFunctor::Reduce();
    }
  };

}//anonymous namespace

//----------------------------------------------------------------------------
// Here is the VTK class proper.
// Construct object with a single contour value of 0.0.
vtkPlaneCutter::vtkPlaneCutter()
{
  this->Plane = vtkPlane::New();
  this->ComputeNormals = 0;
  this->InterpolateAttributes = 1;
  this->SphereTree = vtkSphereTree::New();
  this->SphereTree->BuildHierarchyOn();
  this->SphereTree->SetResolution(3);
}

//----------------------------------------------------------------------------
vtkPlaneCutter::~vtkPlaneCutter()
{
  this->SetPlane(nullptr);
  if ( this->SphereTree )
  {
    this->SphereTree->Delete();
    this->SphereTree = nullptr;
  }
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If the plane definition is modified,
// then this object is modified as well.
vtkMTimeType vtkPlaneCutter::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  if ( this->Plane != nullptr )
  {
    vtkMTimeType mTime2=this->Plane->GetMTime();
    return ( mTime2 > mTime ? mTime2 : mTime );
  }
  else
  {
    return mTime;
  }
}

//----------------------------------------------------------------------------
// If threading is enabled, then a different type of output is created.
int vtkPlaneCutter::
RequestDataObject( vtkInformation* vtkNotUsed(request),
                   vtkInformationVector **inputVector,
                   vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkImageData *imageData = vtkImageData::GetData(inputVector[0]);
  if ( imageData != nullptr )
  {
    vtkPolyData *output = vtkPolyData::GetData(outInfo);
    if (!output)
    {
      vtkPolyData* newOutput = vtkPolyData::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
  }
  else
  {
    vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::GetData(outInfo);
    if (!output)
    {
      vtkMultiBlockDataSet* newOutput = vtkMultiBlockDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPlaneCutter::
ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
               vtkInformationVector* outputVector)
{
  // generate the data
  if( request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()) )
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPlaneCutter::
RequestUpdateExtent(vtkInformation *vtkNotUsed(request),
                    vtkInformationVector **inputVector,
                    vtkInformationVector *vtkNotUsed(outputVector))
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPlaneCutter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPlaneCutter::
FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
// This method delegates to the appropriate algorithm
int vtkPlaneCutter::
RequestData(vtkInformation *request, vtkInformationVector **inputVector,
            vtkInformationVector *outputVector)
{
  vtkDebugMacro(<< "Executing plane cutter");

  // get the input and output
  vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
  vtkDataObject *output = vtkDataObject::GetData(outputVector);

  vtkPlane *plane = this->Plane;
  if ( this->Plane == nullptr )
  {
    vtkDebugMacro(<<"Cutting requires vtkPlane");
    return 0;
  }

  // Check input
  vtkIdType numPts, numCells;
  if ( input == nullptr || (numCells = input->GetNumberOfCells()) < 1 ||
       (numPts = input->GetNumberOfPoints()) < 1 )
  {
    vtkDebugMacro(<<"No input");
    return 0;
  }

  // Set up the cut operation
  double planeOrigin[3], planeNormal[3];
  plane->GetNormal(planeNormal);
  vtkMath::Normalize(planeNormal);
  plane->GetOrigin(planeOrigin);
  if (plane->GetTransform())
  {
    plane->GetTransform()->TransformNormalAtPoint(planeOrigin, planeNormal,
                                                  planeNormal);
    plane->GetTransform()->TransformPoint(planeOrigin,planeOrigin);
  }

  // Delegate the processing to the matching algorithm
  if ( input->GetDataObjectType() == VTK_IMAGE_DATA )
  {//let flying edges do the work
    vtkNew<vtkFlyingEdgesPlaneCutter> flyingEdges;
    flyingEdges->SetPlane(this->Plane);
    vtkNew<vtkPlane> xPlane;
    xPlane->SetOrigin(planeOrigin);
    xPlane->SetNormal(planeNormal);
    flyingEdges->SetPlane(xPlane.GetPointer());
    flyingEdges->SetComputeNormals(this->ComputeNormals);
    flyingEdges->SetInterpolateAttributes(this->InterpolateAttributes);
    return flyingEdges->
      ProcessRequest(request,inputVector,outputVector);
  }

  // Okay we'll be using a sphere tree. The tree's mtime will handle
  // changes to the input. Delegation occurs to the appropriate
  // algorithm.
  this->SphereTree->Build(input);

  if ( input->GetDataObjectType() == VTK_STRUCTURED_GRID )
  {
    StructuredFunctor functor(input, output, plane, this->SphereTree,
                              planeOrigin, planeNormal);
    vtkSMPTools::For(0, numCells, functor);
  }

  else //use appropriate algorithm
  {
    UnstructuredFunctor functor(input, output, plane, this->SphereTree,
                                planeOrigin, planeNormal);
    vtkSMPTools::For(0, numCells, functor);
  }

  // Generate normals across all points if requested
  if (this->ComputeNormals)
  {
    vtkNew<vtkFloatArray> newNormals;
    newNormals->SetNumberOfComponents(3);
    newNormals->SetName("Normals");
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPlaneCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Plane: " << this->Plane << "\n";
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Interpolate Attributes: " << (this->InterpolateAttributes ? "On\n" : "Off\n");
}
