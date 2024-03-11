// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSplitSharpEdgesPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyDataNormals.h"
#include "vtkSMPTools.h"

#include <cmath>
#include <numeric>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSplitSharpEdgesPolyData);

//----------------------------------------------------------------------------
vtkSplitSharpEdgesPolyData::vtkSplitSharpEdgesPolyData()
  : FeatureAngle(30.0)
  , OutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION)
{
}

//----------------------------------------------------------------------------
vtkSplitSharpEdgesPolyData::~vtkSplitSharpEdgesPolyData() = default;

//----------------------------------------------------------------------------
void vtkSplitSharpEdgesPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FeatureAngle: " << this->FeatureAngle << "\n";
  os << indent << "OutputPointsPrecision: " << this->OutputPointsPrecision << "\n";
}

//-----------------------------------------------------------------------------
// Mark polygons around vertex.  Create new vertex (if necessary) and
// replace (i.e., split mesh).
struct vtkSplitSharpEdgesPolyData::MarkAndSplitFunctor
{
  vtkPolyData* Input;
  vtkPolyData* Output;
  vtkFloatArray* CellNormals;
  vtkIdList* Map;
  const double CosAngle;
  vtkSplitSharpEdgesPolyData* Filter;

  struct CellPointReplacementInformation
  {
    vtkIdType CellId;
    int NumberOfRegions;
    CellPointReplacementInformation()
      : CellId(0)
      , NumberOfRegions(0)
    {
    }
    CellPointReplacementInformation(vtkIdType cellId, int numberOfRegions)
      : CellId(cellId)
      , NumberOfRegions(numberOfRegions)
    {
    }
  };
  std::vector<std::vector<CellPointReplacementInformation>> CellPointsReplacementInfo;

  struct LocalData
  {
    vtkSmartPointer<vtkIdList> TempCellPointIds;
    vtkSmartPointer<vtkIdList> CellIds;
    std::vector<int> Visited; // Used to check if cell is visited and the number of regions
  };
  vtkSMPThreadLocal<LocalData> TLData;

  MarkAndSplitFunctor(vtkPolyData* input, vtkPolyData* output, vtkFloatArray* cellNormals,
    vtkIdList* map, vtkSplitSharpEdgesPolyData* filter)
    : Input(input)
    , Output(output)
    , CellNormals(cellNormals)
    , Map(map)
    , CosAngle(std::cos(vtkMath::RadiansFromDegrees(filter->GetFeatureAngle())))
    , Filter(filter)
  {
    this->CellPointsReplacementInfo.resize(this->Input->GetNumberOfPoints());
  }

  void Initialize()
  {
    auto& tlData = this->TLData.Local();
    tlData.TempCellPointIds = vtkSmartPointer<vtkIdList>::New();
    tlData.CellIds = vtkSmartPointer<vtkIdList>::New();
    tlData.Visited.resize(this->Input->GetNumberOfCells(), -1);
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& tlData = this->TLData.Local();
    auto& tempCellPointIds = tlData.TempCellPointIds;
    auto& cellIds = tlData.CellIds;
    auto& visited = tlData.Visited;
    float* cellNormals = this->CellNormals->GetPointer(0);

    vtkIdType ncells, *cells, i, j, numPts;
    const vtkIdType* pts;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((end - begin) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType pointId = begin; pointId < end; pointId++)
    {
      if (pointId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }
        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }
      // Get the cells using this point and make sure that we have to do something
      this->Input->GetPointCells(pointId, ncells, cells);
      if (ncells <= 1)
      {
        continue; // point does not need to be further disconnected
      }

      // Start moving around the "cycle" of points using the point. Label
      // each point as requiring a visit. Then label each subregion of cells
      // connected to this point that are connected (and not separated by
      // a feature edge) with a given region number. For each N regions
      // created, N-1 duplicate (split) points are created. The split point
      // replaces the current point ptId in the polygons connectivity array.
      //
      // Start by initializing the cells as unvisited
      for (i = 0; i < ncells; i++)
      {
        visited[cells[i]] = -1;
      }

      // Loop over all cells and mark the region that each is in.
      int numRegions = 0;
      vtkIdType spot, neiPt[2], nei, cellId, neiCellId;
      float *thisNormal, *neiNormal;
      for (j = 0; j < ncells; j++) // for all cells connected to point
      {
        if (visited[cells[j]] < 0) // for all unvisited cells
        {
          visited[cells[j]] = numRegions;
          // okay, mark all the cells connected to this seed cell and using ptId
          this->Input->GetCellPoints(cells[j], numPts, pts, tempCellPointIds);
          if (numPts < 3)
          {
            continue;
          }

          // find the two edges
          for (spot = 0; spot < numPts; spot++)
          {
            if (pts[spot] == pointId)
            {
              break;
            }
          }

          if (spot == 0)
          {
            neiPt[0] = pts[spot + 1];
            neiPt[1] = pts[numPts - 1];
          }
          else if (spot == (numPts - 1))
          {
            neiPt[0] = pts[spot - 1];
            neiPt[1] = pts[0];
          }
          else
          {
            neiPt[0] = pts[spot + 1];
            neiPt[1] = pts[spot - 1];
          }

          for (i = 0; i < 2; i++) // for each of the two edges of the seed cell
          {
            cellId = cells[j];
            nei = neiPt[i];
            while (cellId >= 0) // while we can grow this region
            {
              this->Input->GetCellEdgeNeighbors(cellId, pointId, nei, cellIds);
              if (cellIds->GetNumberOfIds() == 1 && visited[(neiCellId = cellIds->GetId(0))] < 0)
              {
                thisNormal = cellNormals + 3 * cellId;
                neiNormal = cellNormals + 3 * neiCellId;

                if (vtkMath::Dot(thisNormal, neiNormal) > this->CosAngle)
                {
                  // visit and arrange to visit next edge neighbor
                  visited[neiCellId] = numRegions;
                  cellId = neiCellId;
                  this->Input->GetCellPoints(cellId, numPts, pts, tempCellPointIds);

                  for (spot = 0; spot < numPts; spot++)
                  {
                    if (pts[spot] == pointId)
                    {
                      break;
                    }
                  }

                  if (spot == 0)
                  {
                    nei = (pts[spot + 1] != nei ? pts[spot + 1] : pts[numPts - 1]);
                  }
                  else if (spot == (numPts - 1))
                  {
                    nei = (pts[spot - 1] != nei ? pts[spot - 1] : pts[0]);
                  }
                  else
                  {
                    nei = (pts[spot + 1] != nei ? pts[spot + 1] : pts[spot - 1]);
                  }

                } // if not separated by edge angle
                else
                {
                  cellId = -1; // separated by edge angle
                }
              } // if can move to edge neighbor
              else
              {
                cellId = -1; // separated by previous visit, boundary, or non-manifold
              }
            } // while visit wave is propagating
          }   // for each of the two edges of the starting cell
          numRegions++;
        } // if cell is unvisited
      }   // for all cells connected to point ptId

      if (numRegions <= 1)
      {
        continue; // a single region, no splitting ever required
      }

      // store all cells not in the first region that require splitting
      auto& cellPointReplacementInfo = this->CellPointsReplacementInfo[pointId];
      for (j = 0; j < ncells; ++j)
      {
        if (visited[cells[j]] > 0)
        {
          cellPointReplacementInfo.emplace_back(cells[j], visited[cells[j]]);
        }
      }
    }
  }

  void Reduce()
  {
    // This part needs to be done sequentially.
    vtkNew<vtkIdList> tempCellPointIds;
    vtkIdType replacementPointId;
    vtkIdType lastId;
    for (vtkIdType pointId = 0; pointId < this->Input->GetNumberOfPoints(); ++pointId)
    {
      // For all cells not in the first region, the info pointId is
      // replaced with a new pointId, which is a duplicate of the first
      // point, but disconnected topologically.
      lastId = this->Map->GetNumberOfIds();
      for (auto& cellPointReplacementInfo : this->CellPointsReplacementInfo[pointId])
      {
        const auto& cellId = cellPointReplacementInfo.CellId;
        const auto& numberOfRegions = cellPointReplacementInfo.NumberOfRegions;
        replacementPointId = lastId + numberOfRegions - 1;
        this->Map->InsertId(replacementPointId, pointId);
        this->Output->ReplaceCellPoint(cellId, pointId, replacementPointId, tempCellPointIds);
      } // for all cells connected to pointId and not in first region that require splitting
    }
  }
};

//----------------------------------------------------------------------------
int vtkSplitSharpEdgesPolyData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  auto input = vtkPolyData::GetData(inputVector[0]);
  auto output = vtkPolyData::GetData(outputVector);

  vtkPoints* inPoints = input->GetPoints();
  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  const vtkIdType numInPoints = input->GetNumberOfPoints();
  const vtkIdType numInPolys = input->GetNumberOfPolys();
  if (numInPoints == 0)
  {
    return 1;
  }
  if (numInPolys == 0)
  {
    // don't do anything! pass data through
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return 1;
  }

  ///////////////////////////////////////////////////////////////////
  // Build Cells And Links if needed
  ///////////////////////////////////////////////////////////////////
  if (input->NeedToBuildCells())
  {
    input->BuildCells();
  }
  if (!input->GetLinks())
  {
    input->BuildLinks();
  }
  this->UpdateProgress(0.30);
  if (this->CheckAbort())
  {
    return 1;
  }

  // create a copy because we're modifying it
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();

  output->SetPoints(inPoints);
  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());
  vtkNew<vtkCellArray> outPolys;
  outPolys->DeepCopy(input->GetPolys());
  output->SetPolys(outPolys);
  outCD->PassData(inCD);
  output->BuildCells(); // builds connectivity

  ///////////////////////////////////////////////////////////////////
  //  Splitting will create new points.  We have to create index
  //  array to map new points into old points.
  ///////////////////////////////////////////////////////////////////
  vtkSmartPointer<vtkFloatArray> cellNormals = vtkPolyDataNormals::GetCellNormals(input);

  // Splitting will create new points.  We have to create index array
  // to map new points into old points.
  vtkNew<vtkIdList> newToOldPointsMap;
  newToOldPointsMap->SetNumberOfIds(numInPoints);
  vtkSMPTools::For(0, numInPoints, [&](vtkIdType begin, vtkIdType end) {
    std::iota(newToOldPointsMap->GetPointer(begin), newToOldPointsMap->GetPointer(end), begin);
  });

  MarkAndSplitFunctor functor(input, output, cellNormals, newToOldPointsMap, this);
  vtkSMPTools::For(0, numInPoints, functor);
  const vtkIdType numOutPoints = newToOldPointsMap->GetNumberOfIds();

  vtkDebugMacro(<< "Created " << numNewPts - numPts << " new points");

  //  Now need to map attributes of old points into new points.
  outPD->CopyNormalsOff();
  outPD->CopyAllocate(inPD, numOutPoints);

  vtkNew<vtkPoints> newPoints;
  // set precision for the points in the output
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPoints->SetDataType(input->GetPoints()->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPoints->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }

  newPoints->SetNumberOfPoints(numOutPoints);
  outPD->SetNumberOfTuples(numOutPoints);
  vtkIdType* mapPtr = newToOldPointsMap->GetPointer(0);
  vtkSMPTools::For(0, numOutPoints, [&](vtkIdType begin, vtkIdType end) {
    double p[3];
    for (vtkIdType newPointId = begin; newPointId < end; newPointId++)
    {
      const vtkIdType& oldPointId = mapPtr[newPointId];
      inPoints->GetPoint(oldPointId, p);
      newPoints->SetPoint(newPointId, p);
      outPD->CopyData(inPD, oldPointId, newPointId);
    }
  });
  output->SetPoints(newPoints);

  // set the normals in the output
  outCD->SetNormals(cellNormals);

  return 1;
}
VTK_ABI_NAMESPACE_END
