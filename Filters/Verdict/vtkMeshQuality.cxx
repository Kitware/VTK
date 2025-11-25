// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2003-2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkMeshQuality.h"

#include "vtkAssume.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCellTypes.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"

#include "vtk_verdict.h"

#include <limits>

//----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMeshQuality);

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleAverageSize = 0.0;
double vtkMeshQuality::QuadAverageSize = 0.0;
double vtkMeshQuality::TetAverageSize = 0.0;
double vtkMeshQuality::PyramidAverageSize = 0.0;
double vtkMeshQuality::WedgeAverageSize = 0.0;
double vtkMeshQuality::HexAverageSize = 0.0;

//----------------------------------------------------------------------------
const char* vtkMeshQuality::QualityMeasureNames[] = { "EdgeRatio", "AspectRatio", "RadiusRatio",
  "AspectFrobenius", "MedAspectFrobenius", "MaxAspectFrobenius", "MinAngle", "CollapseRatio",
  "MaxAngle", "Condition", "ScaledJacobian", "Shear", "RelativeSizeSquared", "Shape",
  "ShapeAndSize", "Distortion", "MaxEdgeRatio", "Skew", "Taper", "Volume", "Stretch", "Diagonal",
  "Dimension", "Oddy", "ShearAndSize", "Jacobian", "Warpage", "AspectGamma", "Area",
  "EquiangleSkew", "EquivolumeSkew", "MaxStretch", "MeanAspectFrobenius", "MeanRatio",
  "NodalJacobianRatio", "NormalizedInradius", "SquishIndex", "Inradius", "None" };

namespace
{
//----------------------------------------------------------------------------
void LinearizeCell(int& cellType)
{
  switch (cellType)
  {
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_BIQUADRATIC_TRIANGLE:
    case VTK_HIGHER_ORDER_TRIANGLE:
    case VTK_LAGRANGE_TRIANGLE:
    case VTK_BEZIER_TRIANGLE:
      cellType = VTK_TRIANGLE;
      break;
    case VTK_QUADRATIC_QUAD:
    case VTK_QUADRATIC_LINEAR_QUAD:
    case VTK_HIGHER_ORDER_QUAD:
    case VTK_LAGRANGE_QUADRILATERAL:
    case VTK_BEZIER_QUADRILATERAL:
    case VTK_BIQUADRATIC_QUAD:
      cellType = VTK_QUAD;
      break;
    case VTK_QUADRATIC_TETRA:
    case VTK_HIGHER_ORDER_TETRAHEDRON:
    case VTK_LAGRANGE_TETRAHEDRON:
    case VTK_BEZIER_TETRAHEDRON:
      cellType = VTK_TETRA;
      break;
    case VTK_QUADRATIC_PYRAMID:
    case VTK_TRIQUADRATIC_PYRAMID:
    case VTK_HIGHER_ORDER_PYRAMID:
    case VTK_LAGRANGE_PYRAMID:
    case VTK_BEZIER_PYRAMID:
      cellType = VTK_PYRAMID;
      break;
    case VTK_QUADRATIC_WEDGE:
    case VTK_QUADRATIC_LINEAR_WEDGE:
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
    case VTK_HIGHER_ORDER_WEDGE:
    case VTK_LAGRANGE_WEDGE:
    case VTK_BEZIER_WEDGE:
      cellType = VTK_WEDGE;
      break;
    case VTK_QUADRATIC_HEXAHEDRON:
    case VTK_TRIQUADRATIC_HEXAHEDRON:
    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
    case VTK_HIGHER_ORDER_HEXAHEDRON:
    case VTK_LAGRANGE_HEXAHEDRON:
    case VTK_BEZIER_HEXAHEDRON:
      cellType = VTK_HEXAHEDRON;
      break;
    default:
      break;
  }
}
} // anonymous namespace

/**
 * Struct to save stats for each quality measure.
 */
struct CellQualityStats
{
  double Min, Total, Max, Total2;
  vtkIdType NumCells;

  void GetStats(double array[5]) const
  {
    array[0] = this->Min;
    array[1] = this->Total;
    array[2] = this->Max;
    array[3] = this->Total2;
    array[4] = static_cast<double>(this->NumCells);
  }
};

/**
 * Functor to calculate the size stats of a mesh.
 */
class vtkSizeFunctor
{
  vtkSMPThreadLocalObject<vtkGenericCell> Cell;
  vtkDataSet* Output;
  vtkSMPThreadLocal<CellQualityStats> TLTriangleStats, TLQuadStats, TLTetStats, TLPyrStats,
    TLWedgeStats, TLHexStats;
  CellQualityStats TriangleStats, QuadStats, TetStats, PyrStats, WedgeStats, HexStats;

public:
  vtkSizeFunctor(vtkDataSet* output)
    : Output(output)
  {
    // instantiate any data-structure that needs to be cached for parallel execution.
    if (auto pd = vtkPolyData::SafeDownCast(this->Output))
    {
      if (pd->NeedToBuildCells())
      {
        pd->BuildCells();
      }
    }
    // initialize stats
    this->TriangleStats = this->QuadStats = this->TetStats = this->PyrStats = this->WedgeStats =
      this->HexStats = { 0, 0, 0, 0, 0 };
  }

  void Initialize()
  {
    // initialize stats
    this->TLTriangleStats.Local() = this->TLQuadStats.Local() = this->TLTetStats.Local() =
      this->TLPyrStats.Local() = this->TLWedgeStats.Local() =
        this->TLHexStats.Local() = { 0, 0, 0, 0, 0 };
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    CellQualityStats& triStats = this->TLTriangleStats.Local();
    CellQualityStats& quadStats = this->TLQuadStats.Local();
    CellQualityStats& tetStats = this->TLTetStats.Local();
    CellQualityStats& pyrStats = this->TLPyrStats.Local();
    CellQualityStats& wedgeStats = this->TLWedgeStats.Local();
    CellQualityStats& hexStats = this->TLHexStats.Local();
    vtkGenericCell* genericCell = this->Cell.Local();
    double area, volume; // area and volume

    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      this->Output->GetCell(cellId, genericCell);
      vtkCell* cell = genericCell->GetRepresentativeCell();

      int cellType = cell->GetCellType();
      LinearizeCell(cellType);

      switch (cellType)
      {
        case VTK_TRIANGLE:
          area = vtkMeshQuality::TriangleArea(cell);
          if (area > triStats.Max)
          {
            if (VTK_UNLIKELY(triStats.Min == triStats.Max))
            { // min == max => min has not been set
              triStats.Min = area;
            }
            triStats.Max = area;
          }
          else if (area < triStats.Min)
          {
            triStats.Min = area;
          }
          triStats.Total += area;
          triStats.Total2 += area * area;
          ++triStats.NumCells;
          break;
        case VTK_QUAD:
          area = vtkMeshQuality::QuadArea(cell);
          if (area > quadStats.Max)
          {
            if (VTK_UNLIKELY(quadStats.Min == quadStats.Max))
            { // min == max => min has not been set
              quadStats.Min = area;
            }
            quadStats.Max = area;
          }
          else if (area < quadStats.Min)
          {
            quadStats.Min = area;
          }
          quadStats.Total += area;
          quadStats.Total2 += area * area;
          ++quadStats.NumCells;
          break;
        case VTK_TETRA:
          volume = vtkMeshQuality::TetVolume(cell);
          if (volume > tetStats.Max)
          {
            if (VTK_UNLIKELY(tetStats.Min == tetStats.Max))
            { // min == max => min has not been set
              tetStats.Min = volume;
            }
            tetStats.Max = volume;
          }
          else if (volume < tetStats.Min)
          {
            tetStats.Min = volume;
          }
          tetStats.Total += volume;
          tetStats.Total2 += volume * volume;
          ++tetStats.NumCells;
          break;
        case VTK_PYRAMID:
          volume = vtkMeshQuality::PyramidVolume(cell);
          if (volume > pyrStats.Max)
          {
            if (VTK_UNLIKELY(pyrStats.Min == pyrStats.Max))
            { // min == max => min has not been set
              pyrStats.Min = volume;
            }
            pyrStats.Max = volume;
          }
          else if (volume < pyrStats.Min)
          {
            pyrStats.Min = volume;
          }
          pyrStats.Total += volume;
          pyrStats.Total2 += volume * volume;
          ++pyrStats.NumCells;
          break;
        case VTK_WEDGE:
          volume = vtkMeshQuality::WedgeVolume(cell);
          if (volume > wedgeStats.Max)
          {
            if (VTK_UNLIKELY(wedgeStats.Min == wedgeStats.Max))
            { // min == max => min has not been set
              wedgeStats.Min = volume;
            }
            wedgeStats.Max = volume;
          }
          else if (volume < wedgeStats.Min)
          {
            wedgeStats.Min = volume;
          }
          wedgeStats.Total += volume;
          wedgeStats.Total2 += volume * volume;
          ++wedgeStats.NumCells;
          break;
        case VTK_HEXAHEDRON:
          volume = vtkMeshQuality::HexVolume(cell);
          if (volume > hexStats.Max)
          {
            if (VTK_UNLIKELY(hexStats.Min == hexStats.Max))
            { // min == max => min has not been set
              hexStats.Min = volume;
            }
            hexStats.Max = volume;
          }
          else if (volume < hexStats.Min)
          {
            hexStats.Min = volume;
          }
          hexStats.Total += volume;
          hexStats.Total2 += volume * volume;
          ++hexStats.NumCells;
          break;
        default:
          break;
      }
    }
  }
  void Reduce()
  {
    vtkSMPThreadLocal<CellQualityStats>::iterator iter[6] = { this->TLTriangleStats.begin(),
      this->TLQuadStats.begin(), this->TLTetStats.begin(), this->TLPyrStats.begin(),
      this->TLWedgeStats.begin(), this->TLHexStats.begin() };
    CellQualityStats* stats[6] = { &this->TriangleStats, &this->QuadStats, &this->TetStats,
      &this->PyrStats, &this->WedgeStats, &this->HexStats };
    for (; iter[0] != this->TLTriangleStats.end();
         ++iter[0], ++iter[1], ++iter[2], ++iter[3], ++iter[4], ++iter[5])
    {
      for (int i = 0; i < 6; ++i)
      {
        stats[i]->Min = std::min(iter[i]->Min, stats[i]->Min);
        stats[i]->Total += iter[i]->Total;
        stats[i]->Max = std::max(iter[i]->Max, stats[i]->Max);
        stats[i]->Total2 += iter[i]->Total2;
        stats[i]->NumCells += iter[i]->NumCells;
      }
    }
  }

  CellQualityStats GetTriangleStats() const { return this->TriangleStats; }
  CellQualityStats GetQuadStats() const { return this->QuadStats; }
  CellQualityStats GetTetStats() const { return this->TetStats; }
  CellQualityStats GetPyrStats() const { return this->PyrStats; }
  CellQualityStats GetWedgeStats() const { return this->WedgeStats; }
  CellQualityStats GetHexStats() const { return this->HexStats; }
};

/**
 * Functor to compute the quality stats of a mesh.
 */
class vtkMeshQuality::vtkMeshQualityFunctor
{
  vtkSMPThreadLocalObject<vtkGenericCell> Cell;
  vtkMeshQuality* MeshQuality;
  vtkDataSet* Output;
  vtkSmartPointer<vtkDoubleArray> QualityArray;
  vtkSmartPointer<vtkDoubleArray> ApproxQualityArray;
  vtkSmartPointer<vtkDoubleArray> VolumeArray;
  CellQualityType NanQuality = [](vtkCell*, bool)
  { return std::numeric_limits<double>::quiet_NaN(); };
  CellQualityType TriangleQuality, QuadraticTriangleQuality, BiQuadraticTriangleQuality,
    QuadQuality, QuadraticQuadQuality, TetQuality, QuadraticTetQuality, PyramidQuality,
    WedgeQuality, HexQuality, QuadraticHexQuality, TriQuadraticHexQuality;
  vtkSMPThreadLocal<CellQualityStats> TLTriangleStats, TLQuadStats, TLTetStats, TLPyrStats,
    TLWedgeStats, TLHexStats;
  CellQualityStats TriangleStats, QuadStats, TetStats, PyrStats, WedgeStats, HexStats;

public:
  vtkMeshQualityFunctor(vtkMeshQuality* meshQuality, vtkDataSet* output,
    vtkSmartPointer<vtkDoubleArray> qualityArray,
    vtkSmartPointer<vtkDoubleArray> approxQualityArray, vtkSmartPointer<vtkDoubleArray> volumeArray)
    : MeshQuality(meshQuality)
    , Output(output)
    , QualityArray(qualityArray)
    , ApproxQualityArray(approxQualityArray)
    , VolumeArray(volumeArray)
    , TriangleQuality(meshQuality->GetTriangleQualityMeasureFunction())
    , QuadraticTriangleQuality(vtkMeshQualityFunctor::NanQuality)
    , BiQuadraticTriangleQuality(vtkMeshQualityFunctor::NanQuality)
    , QuadQuality(meshQuality->GetQuadQualityMeasureFunction())
    , QuadraticQuadQuality(vtkMeshQualityFunctor::NanQuality)
    , TetQuality(meshQuality->GetTetQualityMeasureFunction())
    , QuadraticTetQuality(vtkMeshQualityFunctor::NanQuality)
    , PyramidQuality(meshQuality->GetPyramidQualityMeasureFunction())
    , WedgeQuality(meshQuality->GetWedgeQualityMeasureFunction())
    , HexQuality(meshQuality->GetHexQualityMeasureFunction())
    , QuadraticHexQuality(vtkMeshQualityFunctor::NanQuality)
    , TriQuadraticHexQuality(vtkMeshQualityFunctor::NanQuality)
  {
    // instantiate any data-structure that needs to be cached for parallel execution.
    if (auto pd = vtkPolyData::SafeDownCast(this->Output))
    {
      if (pd->NeedToBuildCells())
      {
        pd->BuildCells();
      }
    }
    if (!meshQuality->GetLinearApproximation())
    {
      vtkNew<vtkCellTypes> cellTypes;
      this->Output->GetDistinctCellTypes(cellTypes);
      for (vtkIdType i = 0; i < cellTypes->GetNumberOfTypes(); ++i)
      {
        int cellType = cellTypes->GetCellType(i);
        switch (cellType)
        {
          case VTK_QUADRATIC_TRIANGLE:
            this->QuadraticTriangleQuality =
              meshQuality->GetQuadraticTriangleQualityMeasureFunction();
            break;
          case VTK_BIQUADRATIC_TRIANGLE:
            this->BiQuadraticTriangleQuality =
              meshQuality->GetBiQuadraticTriangleQualityMeasureFunction();
            break;
          case VTK_QUADRATIC_QUAD:
            this->QuadraticQuadQuality = meshQuality->GetQuadraticQuadQualityMeasureFunction();
            break;
          case VTK_QUADRATIC_TETRA:
            this->QuadraticTetQuality = meshQuality->GetQuadraticTetQualityMeasureFunction();
            break;
          case VTK_QUADRATIC_HEXAHEDRON:
            this->QuadraticHexQuality = meshQuality->GetQuadraticHexQualityMeasureFunction();
            break;
          case VTK_TRIQUADRATIC_HEXAHEDRON:
            this->TriQuadraticHexQuality = meshQuality->GetTriQuadraticHexQualityMeasureFunction();
            break;
          default:
            break;
        }
      }
    }
    // initialize stats
    this->TriangleStats = this->QuadStats = this->TetStats = this->PyrStats = this->WedgeStats =
      this->HexStats = { VTK_DOUBLE_MAX, 0.0, VTK_DOUBLE_MIN, 0.0, 0 };
  }

  void Initialize()
  {
    // initialize stats
    this->TLTriangleStats.Local() = this->TLQuadStats.Local() = this->TLTetStats.Local() =
      this->TLPyrStats.Local() = this->TLWedgeStats.Local() =
        this->TLHexStats.Local() = { VTK_DOUBLE_MAX, 0.0, VTK_DOUBLE_MIN, 0.0, 0 };
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    CellQualityStats& triStats = this->TLTriangleStats.Local();
    CellQualityStats& quadStats = this->TLQuadStats.Local();
    CellQualityStats& tetStats = this->TLTetStats.Local();
    CellQualityStats& pyrStats = this->TLPyrStats.Local();
    CellQualityStats& wedgeStats = this->TLWedgeStats.Local();
    CellQualityStats& hexStats = this->TLHexStats.Local();
    vtkGenericCell* genericCell = this->Cell.Local();
    vtkDoubleArray* qualityArrays[2] = { this->QualityArray, this->ApproxQualityArray };
    double quality;

    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      this->Output->GetCell(cellId, genericCell);
      vtkCell* cell = genericCell->GetRepresentativeCell();

      int numberOfOutputQualities = this->MeshQuality->LinearApproximation ? 2 : 1;

      int cellType = cell->GetCellType();

      for (int qualityId = 0; qualityId < numberOfOutputQualities; ++qualityId)
      {
        switch (cellType)
        {
          case VTK_TRIANGLE:
            quality = this->TriangleQuality(cell, /*linearApproximation*/ qualityId);
            if (quality > triStats.Max)
            {
              if (VTK_UNLIKELY(triStats.Min > triStats.Max))
              {
                triStats.Min = quality;
              }
              triStats.Max = quality;
            }
            else if (quality < triStats.Min)
            {
              triStats.Min = quality;
            }
            triStats.Total += quality;
            triStats.Total2 += quality * quality;
            ++triStats.NumCells;
            break;
          case VTK_QUADRATIC_TRIANGLE:
            quality = this->QuadraticTriangleQuality(cell, /*linearApproximation*/ false);
            if (!std::isnan(quality)) // supported quality measure
            {
              if (quality > triStats.Max)
              {
                if (VTK_UNLIKELY(triStats.Min > triStats.Max))
                {
                  triStats.Min = quality;
                }
                triStats.Max = quality;
              }
              else if (quality < triStats.Min)
              {
                triStats.Min = quality;
              }
              triStats.Total += quality;
              triStats.Total2 += quality * quality;
              ++triStats.NumCells;
            }
            break;
          case VTK_BIQUADRATIC_TRIANGLE:
            quality = this->BiQuadraticTriangleQuality(cell, /*linearApproximation*/ false);
            if (!std::isnan(quality)) // supported quality measure
            {
              if (quality > triStats.Max)
              {
                if (VTK_UNLIKELY(triStats.Min > triStats.Max))
                {
                  triStats.Min = quality;
                }
                triStats.Max = quality;
              }
              else if (quality < triStats.Min)
              {
                triStats.Min = quality;
              }
              triStats.Total += quality;
              triStats.Total2 += quality * quality;
              ++triStats.NumCells;
            }
            break;
          case VTK_QUAD:
            quality = this->QuadQuality(cell, /*linearApproximation*/ qualityId);
            if (quality > quadStats.Max)
            {
              if (VTK_UNLIKELY(quadStats.Min > quadStats.Max))
              {
                quadStats.Min = quality;
              }
              quadStats.Max = quality;
            }
            else if (quality < quadStats.Min)
            {
              quadStats.Min = quality;
            }
            quadStats.Total += quality;
            quadStats.Total2 += quality * quality;
            ++quadStats.NumCells;
            break;
          case VTK_BIQUADRATIC_QUAD:
            quality = this->QuadraticQuadQuality(cell, /*linearApproximation*/ false);
            if (!std::isnan(quality)) // supported quality measure
            {
              if (quality > quadStats.Max)
              {
                if (VTK_UNLIKELY(quadStats.Min > quadStats.Max))
                {
                  quadStats.Min = quality;
                }
                quadStats.Max = quality;
              }
              else if (quality < quadStats.Min)
              {
                quadStats.Min = quality;
              }
              quadStats.Total += quality;
              quadStats.Total2 += quality * quality;
              ++quadStats.NumCells;
            }
            break;
          case VTK_TETRA:
            quality = this->TetQuality(cell, /*linearApproximation*/ qualityId);
            if (quality > tetStats.Max)
            {
              if (VTK_UNLIKELY(tetStats.Min > tetStats.Max))
              {
                tetStats.Min = quality;
              }
              tetStats.Max = quality;
            }
            else if (quality < tetStats.Min)
            {
              tetStats.Min = quality;
            }
            tetStats.Total += quality;
            tetStats.Total2 += quality * quality;
            ++tetStats.NumCells;
            break;
          case VTK_QUADRATIC_TETRA:
            quality = this->QuadraticTetQuality(cell, /*linearApproximation*/ false);
            if (!std::isnan(quality)) // supported quality measure
            {
              if (quality > tetStats.Max)
              {
                if (VTK_UNLIKELY(tetStats.Min > tetStats.Max))
                {
                  tetStats.Min = quality;
                }
                tetStats.Max = quality;
              }
              else if (quality < tetStats.Min)
              {
                tetStats.Min = quality;
              }
              tetStats.Total += quality;
              tetStats.Total2 += quality * quality;
              ++tetStats.NumCells;
            }
            break;
          case VTK_PYRAMID:
            quality = this->PyramidQuality(cell, /*linearApproximation*/ qualityId);
            if (quality > pyrStats.Max)
            {
              if (VTK_UNLIKELY(pyrStats.Min > pyrStats.Max))
              {
                pyrStats.Min = quality;
              }
              pyrStats.Max = quality;
            }
            else if (quality < pyrStats.Min)
            {
              pyrStats.Min = quality;
            }
            pyrStats.Total += quality;
            pyrStats.Total2 += quality * quality;
            ++pyrStats.NumCells;
            break;
          case VTK_WEDGE:
            quality = this->WedgeQuality(cell, /*linearApproximation*/ qualityId);
            if (quality > wedgeStats.Max)
            {
              if (VTK_UNLIKELY(wedgeStats.Min > wedgeStats.Max))
              {
                wedgeStats.Min = quality;
              }
              wedgeStats.Max = quality;
            }
            else if (quality < wedgeStats.Min)
            {
              wedgeStats.Min = quality;
            }
            wedgeStats.Total += quality;
            wedgeStats.Total2 += quality * quality;
            ++wedgeStats.NumCells;
            break;
          case VTK_HEXAHEDRON:
            quality = this->HexQuality(cell, /*linearApproximation*/ qualityId);
            if (quality > hexStats.Max)
            {
              if (VTK_UNLIKELY(hexStats.Min > hexStats.Max))
              {
                hexStats.Min = quality;
              }
              hexStats.Max = quality;
            }
            else if (quality < hexStats.Min)
            {
              hexStats.Min = quality;
            }
            hexStats.Total += quality;
            hexStats.Total2 += quality * quality;
            ++hexStats.NumCells;
            break;
          case VTK_QUADRATIC_HEXAHEDRON:
            quality = this->QuadraticHexQuality(cell, /*linearApproximation*/ false);
            if (!std::isnan(quality)) // supported quality measure
            {
              if (quality > hexStats.Max)
              {
                if (VTK_UNLIKELY(hexStats.Min > hexStats.Max))
                {
                  hexStats.Min = quality;
                }
                hexStats.Max = quality;
              }
              else if (quality < hexStats.Min)
              {
                hexStats.Min = quality;
              }
              hexStats.Total += quality;
              hexStats.Total2 += quality * quality;
              ++hexStats.NumCells;
            }
            break;
          case VTK_TRIQUADRATIC_HEXAHEDRON:
            quality = this->TriQuadraticHexQuality(cell, /*linearApproximation*/ false);
            if (!std::isnan(quality)) // supported quality measure
            {
              if (quality > hexStats.Max)
              {
                if (VTK_UNLIKELY(hexStats.Min > hexStats.Max))
                {
                  hexStats.Min = quality;
                }
                hexStats.Max = quality;
              }
              else if (quality < hexStats.Min)
              {
                hexStats.Min = quality;
              }
              hexStats.Total += quality;
              hexStats.Total2 += quality * quality;
              ++hexStats.NumCells;
            }
            break;
          default:
            quality = std::numeric_limits<double>::quiet_NaN();
        }

        if (this->MeshQuality->SaveCellQuality)
        {
          qualityArrays[qualityId]->SetTypedComponent(cellId, 0, quality);
        }

        if (qualityId == 1)
        {
          break;
        }

        if (this->MeshQuality->LinearApproximation)
        {
          LinearizeCell(cellType);
        }
      }
    }
  }
  void Reduce()
  {
    vtkSMPThreadLocal<CellQualityStats>::iterator iter[6] = { this->TLTriangleStats.begin(),
      this->TLQuadStats.begin(), this->TLTetStats.begin(), this->TLPyrStats.begin(),
      this->TLWedgeStats.begin(), this->TLHexStats.begin() };
    CellQualityStats* stats[6] = { &this->TriangleStats, &this->QuadStats, &this->TetStats,
      &this->PyrStats, &this->WedgeStats, &this->HexStats };
    for (; iter[0] != this->TLTriangleStats.end();
         ++iter[0], ++iter[1], ++iter[2], ++iter[3], ++iter[4], ++iter[5])
    {
      for (int i = 0; i < 6; ++i)
      {
        stats[i]->Min = std::min(iter[i]->Min, stats[i]->Min);
        stats[i]->Total += iter[i]->Total;
        stats[i]->Max = std::max(iter[i]->Max, stats[i]->Max);
        stats[i]->Total2 += iter[i]->Total2;
        stats[i]->NumCells += iter[i]->NumCells;
      }
    }
  }

  CellQualityStats GetTriangleStats() const { return this->TriangleStats; }
  CellQualityStats GetQuadStats() const { return this->QuadStats; }
  CellQualityStats GetTetStats() const { return this->TetStats; }
  CellQualityStats GetPyrStats() const { return this->PyrStats; }
  CellQualityStats GetWedgeStats() const { return this->WedgeStats; }
  CellQualityStats GetHexStats() const { return this->HexStats; }
};

//----------------------------------------------------------------------------
void vtkMeshQuality::PrintSelf(ostream& os, vtkIndent indent)
{
  constexpr char onStr[] = "On";
  constexpr char offStr[] = "Off";

  this->Superclass::PrintSelf(os, indent);

  os << indent << "SaveCellQuality:   " << (this->SaveCellQuality ? onStr : offStr) << endl;
  os << indent << "TriangleQualityMeasure: "
     << QualityMeasureNames[static_cast<int>(this->TriangleQualityMeasure)] << endl;
  os << indent
     << "QuadQualityMeasure: " << QualityMeasureNames[static_cast<int>(this->QuadQualityMeasure)]
     << endl;
  os << indent
     << "TetQualityMeasure: " << QualityMeasureNames[static_cast<int>(this->TetQualityMeasure)]
     << endl;
  os << indent << "PyramidQualityMeasure: "
     << QualityMeasureNames[static_cast<int>(this->PyramidQualityMeasure)] << endl;
  os << indent
     << "WedgeQualityMeasure: " << QualityMeasureNames[static_cast<int>(this->WedgeQualityMeasure)]
     << endl;
  os << indent
     << "HexQualityMeasure: " << QualityMeasureNames[static_cast<int>(this->HexQualityMeasure)]
     << endl;
}

//----------------------------------------------------------------------------
vtkMeshQuality::vtkMeshQuality()
{
  this->SaveCellQuality = 1; // Default is On
  this->TriangleQualityMeasure = QualityMeasureTypes::ASPECT_RATIO;
  this->QuadQualityMeasure = QualityMeasureTypes::EDGE_RATIO;
  this->TetQualityMeasure = QualityMeasureTypes::ASPECT_RATIO;
  this->PyramidQualityMeasure = QualityMeasureTypes::SHAPE;
  this->WedgeQualityMeasure = QualityMeasureTypes::EDGE_RATIO;
  this->HexQualityMeasure = QualityMeasureTypes::MAX_ASPECT_FROBENIUS;
  this->LinearApproximation = false;
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetTriangleQualityMeasureFunction() const
{
  switch (this->GetTriangleQualityMeasure())
  {
    case QualityMeasureTypes::AREA:
      return TriangleArea;
    case QualityMeasureTypes::ASPECT_FROBENIUS:
      return TriangleAspectFrobenius;
    case QualityMeasureTypes::ASPECT_RATIO:
      return TriangleAspectRatio;
    case QualityMeasureTypes::CONDITION:
      return TriangleCondition;
    case QualityMeasureTypes::DISTORTION:
      return TriangleDistortion;
    case QualityMeasureTypes::EDGE_RATIO:
      return TriangleEdgeRatio;
    case QualityMeasureTypes::EQUIANGLE_SKEW:
      return TriangleEquiangleSkew;
    case QualityMeasureTypes::MAX_ANGLE:
      return TriangleMaxAngle;
    case QualityMeasureTypes::MIN_ANGLE:
      return TriangleMinAngle;
    case QualityMeasureTypes::NORMALIZED_INRADIUS:
      return TriangleNormalizedInradius;
    case QualityMeasureTypes::RADIUS_RATIO:
      return TriangleRadiusRatio;
    case QualityMeasureTypes::RELATIVE_SIZE_SQUARED:
      return TriangleRelativeSizeSquared;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return TriangleScaledJacobian;
    case QualityMeasureTypes::SHAPE:
      return TriangleShape;
    case QualityMeasureTypes::SHAPE_AND_SIZE:
      return TriangleShapeAndSize;
    default:
      vtkWarningMacro(
        << "Triangle does not support metric ("
        << this->QualityMeasureNames[static_cast<int>(this->GetTriangleQualityMeasure())]
        << "). Use another metric.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetQuadraticTriangleQualityMeasureFunction() const
{
  switch (this->GetTriangleQualityMeasure())
  {
    case QualityMeasureTypes::AREA:
      return TriangleArea;
    case QualityMeasureTypes::DISTORTION:
      return TriangleDistortion;
    case QualityMeasureTypes::NORMALIZED_INRADIUS:
      return TriangleNormalizedInradius;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return TriangleScaledJacobian;
    default:
      vtkWarningMacro(
        << "Quadratic Triangle does not support metric ("
        << this->QualityMeasureNames[static_cast<int>(this->GetTriangleQualityMeasure())]
        << "), use another metric and/or enable LinearApproximation.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetBiQuadraticTriangleQualityMeasureFunction() const
{
  switch (this->GetTriangleQualityMeasure())
  {
    case QualityMeasureTypes::AREA:
      return TriangleArea;
    case QualityMeasureTypes::DISTORTION:
      return TriangleDistortion;
    default:
      vtkWarningMacro(
        << "Bi-Quadratic Triangle does not support metric ("
        << this->QualityMeasureNames[static_cast<int>(this->GetTriangleQualityMeasure())]
        << "). Use another metric and/or enable LinearApproximation.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetQuadQualityMeasureFunction() const
{
  switch (this->GetQuadQualityMeasure())
  {
    case QualityMeasureTypes::AREA:
      return QuadArea;
    case QualityMeasureTypes::ASPECT_RATIO:
      return QuadAspectRatio;
    case QualityMeasureTypes::CONDITION:
      return QuadCondition;
    case QualityMeasureTypes::DISTORTION:
      return QuadDistortion;
    case QualityMeasureTypes::EDGE_RATIO:
      return QuadEdgeRatio;
    case QualityMeasureTypes::EQUIANGLE_SKEW:
      return QuadEquiangleSkew;
    case QualityMeasureTypes::JACOBIAN:
      return QuadJacobian;
    case QualityMeasureTypes::MAX_ANGLE:
      return QuadMaxAngle;
    case QualityMeasureTypes::MAX_ASPECT_FROBENIUS:
      return QuadMaxAspectFrobenius;
    case QualityMeasureTypes::MAX_EDGE_RATIO:
      return QuadMaxEdgeRatio;
    case QualityMeasureTypes::MED_ASPECT_FROBENIUS:
      return QuadMedAspectFrobenius;
    case QualityMeasureTypes::MIN_ANGLE:
      return QuadMinAngle;
    case QualityMeasureTypes::ODDY:
      return QuadOddy;
    case QualityMeasureTypes::RADIUS_RATIO:
      return QuadRadiusRatio;
    case QualityMeasureTypes::RELATIVE_SIZE_SQUARED:
      return QuadRelativeSizeSquared;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return QuadScaledJacobian;
    case QualityMeasureTypes::SHAPE:
      return QuadShape;
    case QualityMeasureTypes::SHAPE_AND_SIZE:
      return QuadShapeAndSize;
    case QualityMeasureTypes::SHEAR:
      return QuadShear;
    case QualityMeasureTypes::SHEAR_AND_SIZE:
      return QuadShearAndSize;
    case QualityMeasureTypes::SKEW:
      return QuadSkew;
    case QualityMeasureTypes::STRETCH:
      return QuadStretch;
    case QualityMeasureTypes::TAPER:
      return QuadTaper;
    case QualityMeasureTypes::WARPAGE:
      return QuadWarpage;
    default:
      vtkWarningMacro(<< "Quad does not support metric ("
                      << this->QualityMeasureNames[static_cast<int>(this->GetQuadQualityMeasure())]
                      << "), Use another metric.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetQuadraticQuadQualityMeasureFunction() const
{
  switch (this->GetQuadQualityMeasure())
  {
    case QualityMeasureTypes::AREA:
      return QuadArea;
    case QualityMeasureTypes::DISTORTION:
      return QuadDistortion;
    default:
      vtkWarningMacro(<< "Quadratic Quad does not support metric ("
                      << this->QualityMeasureNames[static_cast<int>(this->GetQuadQualityMeasure())]
                      << "). Use another metric and/or enable LinearApproximation.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetBiQuadraticQuadQualityMeasureFunction() const
{
  switch (this->GetQuadQualityMeasure())
  {
    case QualityMeasureTypes::AREA:
      return QuadArea;
    default:
      vtkWarningMacro(<< "Bi-Quadratic Quad does not support metric ("
                      << this->QualityMeasureNames[static_cast<int>(this->GetQuadQualityMeasure())]
                      << "). Use another metric and/or enable LinearApproximation.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetTetQualityMeasureFunction() const
{
  switch (this->GetTetQualityMeasure())
  {
    case QualityMeasureTypes::ASPECT_FROBENIUS:
      return TetAspectFrobenius;
    case QualityMeasureTypes::ASPECT_GAMMA:
      return TetAspectGamma;
    case QualityMeasureTypes::ASPECT_RATIO:
      return TetAspectRatio;
    case QualityMeasureTypes::COLLAPSE_RATIO:
      return TetCollapseRatio;
    case QualityMeasureTypes::CONDITION:
      return TetCondition;
    case QualityMeasureTypes::DISTORTION:
      return TetDistortion;
    case QualityMeasureTypes::EDGE_RATIO:
      return TetEdgeRatio;
    case QualityMeasureTypes::EQUIANGLE_SKEW:
      return TetEquiangleSkew;
    case QualityMeasureTypes::EQUIVOLUME_SKEW:
      return TetEquivolumeSkew;
    case QualityMeasureTypes::INRADIUS:
      return TetInradius;
    case QualityMeasureTypes::JACOBIAN:
      return TetJacobian;
    case QualityMeasureTypes::MEAN_RATIO:
      return TetMeanRatio;
    case QualityMeasureTypes::MIN_ANGLE:
      return TetMinAngle;
    case QualityMeasureTypes::NORMALIZED_INRADIUS:
      return TetNormalizedInradius;
    case QualityMeasureTypes::RADIUS_RATIO:
      return TetRadiusRatio;
    case QualityMeasureTypes::RELATIVE_SIZE_SQUARED:
      return TetRelativeSizeSquared;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return TetScaledJacobian;
    case QualityMeasureTypes::SHAPE:
      return TetShape;
    case QualityMeasureTypes::SHAPE_AND_SIZE:
      return TetShapeAndSize;
    case QualityMeasureTypes::SQUISH_INDEX:
      return TetSquishIndex;
    case QualityMeasureTypes::VOLUME:
      return TetVolume;
    default:
      vtkWarningMacro(<< "Tet does not support metric ("
                      << this->QualityMeasureNames[static_cast<int>(this->GetTetQualityMeasure())]
                      << "), Use another metric.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetQuadraticTetQualityMeasureFunction() const
{
  switch (this->GetTetQualityMeasure())
  {
    case QualityMeasureTypes::DISTORTION:
      return TetDistortion;
    case QualityMeasureTypes::EQUIVOLUME_SKEW:
      return TetEquivolumeSkew;
    case QualityMeasureTypes::INRADIUS:
      return TetInradius;
    case QualityMeasureTypes::JACOBIAN:
      return TetJacobian;
    case QualityMeasureTypes::MEAN_RATIO:
      return TetMeanRatio;
    case QualityMeasureTypes::NORMALIZED_INRADIUS:
      return TetNormalizedInradius;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return TetScaledJacobian;
    case QualityMeasureTypes::VOLUME:
      return TetVolume;
    default:
      vtkWarningMacro(<< "Quadratic Tet does not support metric ("
                      << this->QualityMeasureNames[static_cast<int>(this->GetTetQualityMeasure())]
                      << "), Use another metric and/or enable LinearApproximation.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetPyramidQualityMeasureFunction() const
{
  switch (this->GetPyramidQualityMeasure())
  {
    case QualityMeasureTypes::EQUIANGLE_SKEW:
      return PyramidEquiangleSkew;
    case QualityMeasureTypes::JACOBIAN:
      return PyramidJacobian;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return PyramidScaledJacobian;
    case QualityMeasureTypes::SHAPE:
      return PyramidShape;
    case QualityMeasureTypes::VOLUME:
      return PyramidVolume;
    default:
      vtkWarningMacro(
        << "Pyramid does not support metric ("
        << this->QualityMeasureNames[static_cast<int>(this->GetPyramidQualityMeasure())]
        << "), Use another metric.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetWedgeQualityMeasureFunction() const
{
  switch (this->WedgeQualityMeasure)
  {
    case QualityMeasureTypes::CONDITION:
      return WedgeCondition;
    case QualityMeasureTypes::DISTORTION:
      return WedgeDistortion;
    case QualityMeasureTypes::EDGE_RATIO:
      return WedgeEdgeRatio;
    case QualityMeasureTypes::EQUIANGLE_SKEW:
      return WedgeEquiangleSkew;
    case QualityMeasureTypes::JACOBIAN:
      return WedgeJacobian;
    case QualityMeasureTypes::MAX_ASPECT_FROBENIUS:
      return WedgeMaxAspectFrobenius;
    case QualityMeasureTypes::MAX_STRETCH:
      return WedgeMaxStretch;
    case QualityMeasureTypes::MEAN_ASPECT_FROBENIUS:
      return WedgeMeanAspectFrobenius;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return WedgeScaledJacobian;
    case QualityMeasureTypes::SHAPE:
      return WedgeShape;
    case QualityMeasureTypes::VOLUME:
      return WedgeVolume;
    default:
      vtkWarningMacro(<< "Wedge does not support metric ("
                      << this->QualityMeasureNames[static_cast<int>(this->GetWedgeQualityMeasure())]
                      << "), Use another metric.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetHexQualityMeasureFunction() const
{
  switch (this->GetHexQualityMeasure())
  {
    case QualityMeasureTypes::CONDITION:
      return HexCondition;
    case QualityMeasureTypes::DIAGONAL:
      return HexDiagonal;
    case QualityMeasureTypes::DIMENSION:
      return HexDimension;
    case QualityMeasureTypes::DISTORTION:
      return HexDistortion;
    case QualityMeasureTypes::EDGE_RATIO:
      return HexEdgeRatio;
    case QualityMeasureTypes::EQUIANGLE_SKEW:
      return HexEquiangleSkew;
    case QualityMeasureTypes::JACOBIAN:
      return HexJacobian;
    case QualityMeasureTypes::MAX_ASPECT_FROBENIUS:
      return HexMaxAspectFrobenius;
    case QualityMeasureTypes::MAX_EDGE_RATIO:
      return HexMaxEdgeRatio;
    case QualityMeasureTypes::MED_ASPECT_FROBENIUS:
      return HexMedAspectFrobenius;
    case QualityMeasureTypes::NODAL_JACOBIAN_RATIO:
      return HexNodalJacobianRatio;
    case QualityMeasureTypes::ODDY:
      return HexOddy;
    case QualityMeasureTypes::RELATIVE_SIZE_SQUARED:
      return HexRelativeSizeSquared;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return HexScaledJacobian;
    case QualityMeasureTypes::SHAPE:
      return HexShape;
    case QualityMeasureTypes::SHAPE_AND_SIZE:
      return HexShapeAndSize;
    case QualityMeasureTypes::SHEAR:
      return HexShear;
    case QualityMeasureTypes::SHEAR_AND_SIZE:
      return HexShearAndSize;
    case QualityMeasureTypes::SKEW:
      return HexSkew;
    case QualityMeasureTypes::STRETCH:
      return HexStretch;
    case QualityMeasureTypes::TAPER:
      return HexTaper;
    case QualityMeasureTypes::VOLUME:
      return HexVolume;
    default:
      vtkWarningMacro(<< "Hex does not support metric ("
                      << this->QualityMeasureNames[static_cast<int>(this->GetHexQualityMeasure())]
                      << "), Use another metric.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetQuadraticHexQualityMeasureFunction() const
{
  switch (this->GetHexQualityMeasure())
  {
    case QualityMeasureTypes::DISTORTION:
      return HexDistortion;
    case QualityMeasureTypes::VOLUME:
      return HexVolume;
    default:
      vtkWarningMacro(<< "Quadratic Hex does not support metric ("
                      << this->QualityMeasureNames[static_cast<int>(this->GetHexQualityMeasure())]
                      << "), Use another metric and/or enable LinearApproximation.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetTriQuadraticHexQualityMeasureFunction() const
{
  switch (this->GetHexQualityMeasure())
  {
    case QualityMeasureTypes::DISTORTION:
      return HexDistortion;
    case QualityMeasureTypes::JACOBIAN:
      return HexJacobian;
    case QualityMeasureTypes::VOLUME:
      return HexVolume;
    default:
      vtkWarningMacro(<< "Tri-Quadratic Hex does not support metric ("
                      << this->QualityMeasureNames[static_cast<int>(this->GetHexQualityMeasure())]
                      << "), Use another metric and/or enable LinearApproximation.");
      return [](vtkCell*, bool) { return std::numeric_limits<double>::quiet_NaN(); };
  }
}

//----------------------------------------------------------------------------
void vtkMeshQuality::ComputeAverageCellSize(vtkDataSet* dataset)
{
  vtkDataArray* triAreaHint = dataset->GetFieldData()->GetArray("TriArea");
  vtkDataArray* quadAreaHint = dataset->GetFieldData()->GetArray("QuadArea");
  vtkDataArray* tetVolHint = dataset->GetFieldData()->GetArray("TetVolume");
  vtkDataArray* pyrVolHint = dataset->GetFieldData()->GetArray("PyrVolume");
  vtkDataArray* wedgeVolHint = dataset->GetFieldData()->GetArray("WedgeVolume");
  vtkDataArray* hexVolHint = dataset->GetFieldData()->GetArray("HexVolume");

  double triAreaTuple[5];
  double quadAreaTuple[5];
  double tetVolTuple[5];
  double pyrVolTuple[5];
  double wedgeVolTuple[5];
  double hexVolTuple[5];

  if (triAreaHint && triAreaHint->GetNumberOfTuples() > 0 &&
    triAreaHint->GetNumberOfComponents() == 5 && quadAreaHint &&
    quadAreaHint->GetNumberOfTuples() > 0 && quadAreaHint->GetNumberOfComponents() == 5 &&
    tetVolHint && tetVolHint->GetNumberOfTuples() > 0 && tetVolHint->GetNumberOfComponents() == 5 &&
    pyrVolHint && pyrVolHint->GetNumberOfTuples() > 0 && pyrVolHint->GetNumberOfComponents() == 5 &&
    wedgeVolHint && wedgeVolHint->GetNumberOfTuples() > 0 &&
    wedgeVolHint->GetNumberOfComponents() == 5 && hexVolHint &&
    hexVolHint->GetNumberOfTuples() > 0 && hexVolHint->GetNumberOfComponents() == 5)
  {
    triAreaHint->GetTuple(0, triAreaTuple);
    quadAreaHint->GetTuple(0, quadAreaTuple);
    tetVolHint->GetTuple(0, tetVolTuple);
    pyrVolHint->GetTuple(0, pyrVolTuple);
    wedgeVolHint->GetTuple(0, wedgeVolTuple);
    hexVolHint->GetTuple(0, hexVolTuple);
    vtkMeshQuality::TriangleAverageSize = triAreaTuple[1] / triAreaTuple[4];
    vtkMeshQuality::QuadAverageSize = quadAreaTuple[1] / quadAreaTuple[4];
    vtkMeshQuality::TetAverageSize = tetVolTuple[1] / tetVolTuple[4];
    vtkMeshQuality::PyramidAverageSize = pyrVolTuple[1] / pyrVolTuple[4];
    vtkMeshQuality::WedgeAverageSize = wedgeVolTuple[1] / wedgeVolTuple[4];
    vtkMeshQuality::HexAverageSize = hexVolTuple[1] / hexVolTuple[4];
  }
  else
  {
    vtkSizeFunctor sizeFunctor(dataset);
    vtkSMPTools::For(0, dataset->GetNumberOfCells(), sizeFunctor);

    sizeFunctor.GetTriangleStats().GetStats(triAreaTuple);
    sizeFunctor.GetQuadStats().GetStats(quadAreaTuple);
    sizeFunctor.GetTetStats().GetStats(tetVolTuple);
    sizeFunctor.GetPyrStats().GetStats(pyrVolTuple);
    sizeFunctor.GetWedgeStats().GetStats(wedgeVolTuple);
    sizeFunctor.GetHexStats().GetStats(hexVolTuple);

    vtkMeshQuality::TriangleAverageSize = triAreaTuple[1] / triAreaTuple[4];
    vtkMeshQuality::QuadAverageSize = quadAreaTuple[1] / quadAreaTuple[4];
    vtkMeshQuality::TetAverageSize = tetVolTuple[1] / tetVolTuple[4];
    vtkMeshQuality::PyramidAverageSize = pyrVolTuple[1] / pyrVolTuple[4];
    vtkMeshQuality::WedgeAverageSize = wedgeVolTuple[1] / wedgeVolTuple[4];
    vtkMeshQuality::HexAverageSize = hexVolTuple[1] / hexVolTuple[4];

    // Save info as field data for downstream filters
    triAreaHint = vtkDoubleArray::New();
    triAreaHint->SetName("TriArea");
    triAreaHint->SetNumberOfComponents(5);
    triAreaHint->InsertNextTuple(triAreaTuple);
    dataset->GetFieldData()->AddArray(triAreaHint);
    triAreaHint->Delete();

    quadAreaHint = vtkDoubleArray::New();
    quadAreaHint->SetName("QuadArea");
    quadAreaHint->SetNumberOfComponents(5);
    quadAreaHint->InsertNextTuple(quadAreaTuple);
    dataset->GetFieldData()->AddArray(quadAreaHint);
    quadAreaHint->Delete();

    tetVolHint = vtkDoubleArray::New();
    tetVolHint->SetName("TetVolume");
    tetVolHint->SetNumberOfComponents(5);
    tetVolHint->InsertNextTuple(tetVolTuple);
    dataset->GetFieldData()->AddArray(tetVolHint);
    tetVolHint->Delete();

    pyrVolHint = vtkDoubleArray::New();
    pyrVolHint->SetName("PyrVolume");
    pyrVolHint->SetNumberOfComponents(5);
    pyrVolHint->InsertNextTuple(pyrVolTuple);
    dataset->GetFieldData()->AddArray(pyrVolHint);
    pyrVolHint->Delete();

    wedgeVolHint = vtkDoubleArray::New();
    wedgeVolHint->SetName("WedgeVolume");
    wedgeVolHint->SetNumberOfComponents(5);
    wedgeVolHint->InsertNextTuple(wedgeVolTuple);
    dataset->GetFieldData()->AddArray(wedgeVolHint);
    wedgeVolHint->Delete();

    hexVolHint = vtkDoubleArray::New();
    hexVolHint->SetName("HexVolume");
    hexVolHint->SetNumberOfComponents(5);
    hexVolHint->InsertNextTuple(hexVolTuple);
    dataset->GetFieldData()->AddArray(hexVolHint);
    hexVolHint->Delete();
  }
}

//----------------------------------------------------------------------------
int vtkMeshQuality::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  vtkDataSet* in = vtkDataSet::GetData(inputVector[0], 0);
  vtkDataSet* out = vtkDataSet::GetData(outputVector, 0);
  out->ShallowCopy(in);

  vtkIdType numberOfCells = in->GetNumberOfCells();

  vtkSmartPointer<vtkDoubleArray> qualityArray = nullptr;
  vtkSmartPointer<vtkDoubleArray> approxQualityArray = nullptr;
  vtkSmartPointer<vtkDoubleArray> volumeArray = nullptr;
  if (this->SaveCellQuality)
  {
    qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
    qualityArray->SetNumberOfComponents(1);
    qualityArray->SetNumberOfTuples(numberOfCells);
    qualityArray->SetName("Quality");
    out->GetCellData()->AddArray(qualityArray);
    out->GetCellData()->SetActiveAttribute("Quality", vtkDataSetAttributes::SCALARS);

    if (this->LinearApproximation)
    {
      approxQualityArray = vtkSmartPointer<vtkDoubleArray>::New();
      approxQualityArray->SetNumberOfValues(numberOfCells);
      approxQualityArray->SetName("Quality (Linear Approx)");
      out->GetCellData()->AddArray(approxQualityArray);
    }
  }

  // These measures require the average area/volume for all cells of the same type in the mesh.
  // Either use the hinted value (computed by a previous vtkMeshQuality filter) or compute it.
  if (this->GetTriangleQualityMeasure() == QualityMeasureTypes::RELATIVE_SIZE_SQUARED ||
    this->GetTriangleQualityMeasure() == QualityMeasureTypes::SHAPE_AND_SIZE ||
    this->GetQuadQualityMeasure() == QualityMeasureTypes::RELATIVE_SIZE_SQUARED ||
    this->GetQuadQualityMeasure() == QualityMeasureTypes::SHAPE_AND_SIZE ||
    this->GetQuadQualityMeasure() == QualityMeasureTypes::SHEAR_AND_SIZE ||
    this->GetTetQualityMeasure() == QualityMeasureTypes::RELATIVE_SIZE_SQUARED ||
    this->GetTetQualityMeasure() == QualityMeasureTypes::SHAPE_AND_SIZE ||
    this->GetHexQualityMeasure() == QualityMeasureTypes::RELATIVE_SIZE_SQUARED ||
    this->GetHexQualityMeasure() == QualityMeasureTypes::SHAPE_AND_SIZE ||
    this->GetHexQualityMeasure() == QualityMeasureTypes::SHEAR_AND_SIZE)
  {
    vtkMeshQuality::ComputeAverageCellSize(out);
  }

  vtkMeshQualityFunctor meshQualityFunctor(
    this, out, qualityArray, approxQualityArray, volumeArray);
  vtkSMPTools::For(0, numberOfCells, meshQualityFunctor);

  std::array<CellQualityStats, 6> stats = { meshQualityFunctor.GetTriangleStats(),
    meshQualityFunctor.GetQuadStats(), meshQualityFunctor.GetTetStats(),
    meshQualityFunctor.GetPyrStats(), meshQualityFunctor.GetWedgeStats(),
    meshQualityFunctor.GetHexStats() };

  for (auto& stat : stats)
  {
    if (stat.NumCells)
    {
      stat.Total /= static_cast<double>(stat.NumCells);
      double multFactor =
        1. / static_cast<double>(stat.NumCells > 1 ? stat.NumCells - 1 : stat.NumCells);
      stat.Total2 =
        multFactor * (stat.Total2 - static_cast<double>(stat.NumCells) * stat.Total * stat.Total);
    }
    else
    {
      stat.Min = stat.Max = stat.Total = stat.Total2 = 0.;
    }
  }

  double tuple[5];
  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Triangle Quality");
  qualityArray->SetNumberOfComponents(5);
  stats[0].GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Quadrilateral Quality");
  qualityArray->SetNumberOfComponents(5);
  stats[1].GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Tetrahedron Quality");
  qualityArray->SetNumberOfComponents(5);
  stats[2].GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Pyramid Quality");
  qualityArray->SetNumberOfComponents(5);
  stats[3].GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Wedge Quality");
  qualityArray->SetNumberOfComponents(5);
  stats[4].GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Hexahedron Quality");
  qualityArray->SetNumberOfComponents(5);
  stats[5].GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  return 1;
}

// Triangle quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleArea(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    (ct == VTK_QUADRATIC_TRIANGLE || ct == VTK_BIQUADRATIC_TRIANGLE) && !linearApproximation
    ? points->GetNumberOfTuples()
    : 3;
  return verdict::tri_area(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleAspectFrobenius(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tri_aspect_frobenius(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleAspectRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tri_aspect_ratio(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleCondition(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tri_condition(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleDistortion(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    (ct == VTK_QUADRATIC_TRIANGLE || ct == VTK_BIQUADRATIC_TRIANGLE) && !linearApproximation
    ? points->GetNumberOfTuples()
    : 3;
  return verdict::tri_distortion(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleEdgeRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tri_edge_ratio(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleEquiangleSkew(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tri_equiangle_skew(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleMinAngle(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tri_minimum_angle(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleMaxAngle(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tri_maximum_angle(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleNormalizedInradius(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_QUADRATIC_TRIANGLE && !linearApproximation ? points->GetNumberOfTuples() : 3;
  return verdict::tri_normalized_inradius(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleRadiusRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tri_radius_ratio(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleRelativeSizeSquared(
  vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  if (vtkMeshQuality::TriangleAverageSize == 0.0)
  {
    vtkLogF(ERROR, "TriangleAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::tri_relative_size_squared(3, pc, vtkMeshQuality::TriangleAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleScaledJacobian(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_QUADRATIC_TRIANGLE && !linearApproximation ? points->GetNumberOfTuples() : 3;
  return verdict::tri_scaled_jacobian(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleShape(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tri_shape(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleShapeAndSize(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  if (vtkMeshQuality::TriangleAverageSize == 0.0)
  {
    vtkLogF(ERROR, "TriangleAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::tri_shape_and_size(3, pc, vtkMeshQuality::TriangleAverageSize);
}

// Quadrangle quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadArea(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    (ct == VTK_QUADRATIC_QUAD || ct == VTK_BIQUADRATIC_QUAD) && !linearApproximation
    ? points->GetNumberOfTuples()
    : 4;
  return verdict::quad_area(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadAspectRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_aspect_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadCondition(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_condition(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadDistortion(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_QUADRATIC_QUAD && !linearApproximation ? points->GetNumberOfTuples() : 4;
  return verdict::quad_distortion(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadEdgeRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_edge_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadEquiangleSkew(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_equiangle_skew(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadJacobian(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_jacobian(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadMaxAngle(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_maximum_angle(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadMaxEdgeRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_max_edge_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadMaxAspectFrobenius(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_max_aspect_frobenius(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadMedAspectFrobenius(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_med_aspect_frobenius(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadMinAngle(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_minimum_angle(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadOddy(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_oddy(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadRadiusRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_radius_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadRelativeSizeSquared(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  if (vtkMeshQuality::QuadAverageSize == 0.0)
  {
    vtkLogF(ERROR, "QuadAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::quad_relative_size_squared(4, pc, vtkMeshQuality::QuadAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadScaledJacobian(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_scaled_jacobian(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadShape(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_shape(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadShapeAndSize(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  if (vtkMeshQuality::QuadAverageSize == 0.0)
  {
    vtkLogF(ERROR, "QuadAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::quad_shape_and_size(4, pc, vtkMeshQuality::QuadAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadShear(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_shear(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadShearAndSize(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  if (vtkMeshQuality::QuadAverageSize == 0.0)
  {
    vtkLogF(ERROR, "QuadAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::quad_shear_and_size(4, pc, vtkMeshQuality::QuadAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadSkew(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_skew(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadStretch(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_stretch(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadTaper(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_taper(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadWarpage(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::quad_warpage(4, pc);
}

// Tetrahedral quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::TetAspectFrobenius(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tet_aspect_frobenius(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetAspectGamma(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tet_aspect_gamma(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetAspectRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tet_aspect_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetCollapseRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tet_collapse_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetCondition(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tet_condition(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetDistortion(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_QUADRATIC_TETRA && !linearApproximation ? points->GetNumberOfTuples() : 4;
  return verdict::tet_distortion(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetEdgeRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tet_edge_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetEquiangleSkew(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tet_equiangle_skew(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetEquivolumeSkew(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_QUADRATIC_TETRA && !linearApproximation ? points->GetNumberOfTuples() : 4;
  return verdict::tet_equivolume_skew(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetInradius(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_QUADRATIC_TETRA && !linearApproximation ? points->GetNumberOfTuples() : 4;
  return verdict::tet_inradius(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetJacobian(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_QUADRATIC_TETRA && !linearApproximation ? points->GetNumberOfTuples() : 4;
  return verdict::tet_jacobian(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetMeanRatio(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_QUADRATIC_TETRA && !linearApproximation ? points->GetNumberOfTuples() : 4;
  return verdict::tet_mean_ratio(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetMinAngle(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tet_minimum_angle(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetNormalizedInradius(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_QUADRATIC_TETRA && !linearApproximation ? points->GetNumberOfTuples() : 4;
  return verdict::tet_normalized_inradius(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetRadiusRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tet_radius_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetRelativeSizeSquared(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  if (vtkMeshQuality::TetAverageSize == 0.0)
  {
    vtkLogF(ERROR, "TetAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::tet_relative_size_squared(4, pc, vtkMeshQuality::TetAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetScaledJacobian(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_QUADRATIC_TETRA && !linearApproximation ? points->GetNumberOfTuples() : 4;
  return verdict::tet_scaled_jacobian(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetShape(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tet_shape(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetShapeAndSize(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  if (vtkMeshQuality::TetAverageSize == 0.0)
  {
    vtkLogF(ERROR, "TetAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::tet_shape_and_size(4, pc, vtkMeshQuality::TetAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetSquishIndex(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::tet_squish_index(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetVolume(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_QUADRATIC_TETRA && !linearApproximation ? points->GetNumberOfTuples() : 4;
  return verdict::tet_volume(numPts, pc);
}

// Pyramid quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::PyramidEquiangleSkew(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::pyramid_equiangle_skew(5, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::PyramidJacobian(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::pyramid_jacobian(5, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::PyramidScaledJacobian(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::pyramid_scaled_jacobian(5, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::PyramidShape(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::pyramid_shape(5, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::PyramidVolume(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::pyramid_volume(5, pc);
}

// Wedge quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeCondition(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  double pc[6][3];
  points->GetTypedTuple(1, pc[0]);
  points->GetTypedTuple(0, pc[1]);
  points->GetTypedTuple(2, pc[2]);
  points->GetTypedTuple(4, pc[3]);
  points->GetTypedTuple(3, pc[4]);
  points->GetTypedTuple(5, pc[5]);
  return verdict::wedge_condition(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeDistortion(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  double pc[6][3];
  points->GetTypedTuple(1, pc[0]);
  points->GetTypedTuple(0, pc[1]);
  points->GetTypedTuple(2, pc[2]);
  points->GetTypedTuple(4, pc[3]);
  points->GetTypedTuple(3, pc[4]);
  points->GetTypedTuple(5, pc[5]);
  return verdict::wedge_distortion(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeEdgeRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  double pc[6][3];
  points->GetTypedTuple(1, pc[0]);
  points->GetTypedTuple(0, pc[1]);
  points->GetTypedTuple(2, pc[2]);
  points->GetTypedTuple(4, pc[3]);
  points->GetTypedTuple(3, pc[4]);
  points->GetTypedTuple(5, pc[5]);
  return verdict::wedge_edge_ratio(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeEquiangleSkew(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  double pc[6][3];
  points->GetTypedTuple(1, pc[0]);
  points->GetTypedTuple(0, pc[1]);
  points->GetTypedTuple(2, pc[2]);
  points->GetTypedTuple(4, pc[3]);
  points->GetTypedTuple(3, pc[4]);
  points->GetTypedTuple(5, pc[5]);
  return verdict::wedge_equiangle_skew(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeJacobian(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  double pc[6][3];
  points->GetTypedTuple(1, pc[0]);
  points->GetTypedTuple(0, pc[1]);
  points->GetTypedTuple(2, pc[2]);
  points->GetTypedTuple(4, pc[3]);
  points->GetTypedTuple(3, pc[4]);
  points->GetTypedTuple(5, pc[5]);
  return verdict::wedge_jacobian(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeMaxAspectFrobenius(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  double pc[6][3];
  points->GetTypedTuple(1, pc[0]);
  points->GetTypedTuple(0, pc[1]);
  points->GetTypedTuple(2, pc[2]);
  points->GetTypedTuple(4, pc[3]);
  points->GetTypedTuple(3, pc[4]);
  points->GetTypedTuple(5, pc[5]);
  return verdict::wedge_max_aspect_frobenius(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeMaxStretch(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  double pc[6][3];
  points->GetTypedTuple(1, pc[0]);
  points->GetTypedTuple(0, pc[1]);
  points->GetTypedTuple(2, pc[2]);
  points->GetTypedTuple(4, pc[3]);
  points->GetTypedTuple(3, pc[4]);
  points->GetTypedTuple(5, pc[5]);
  return verdict::wedge_max_stretch(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeMeanAspectFrobenius(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  double pc[6][3];
  points->GetTypedTuple(1, pc[0]);
  points->GetTypedTuple(0, pc[1]);
  points->GetTypedTuple(2, pc[2]);
  points->GetTypedTuple(4, pc[3]);
  points->GetTypedTuple(3, pc[4]);
  points->GetTypedTuple(5, pc[5]);
  return verdict::wedge_mean_aspect_frobenius(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeScaledJacobian(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  double pc[6][3];
  points->GetTypedTuple(1, pc[0]);
  points->GetTypedTuple(0, pc[1]);
  points->GetTypedTuple(2, pc[2]);
  points->GetTypedTuple(4, pc[3]);
  points->GetTypedTuple(3, pc[4]);
  points->GetTypedTuple(5, pc[5]);
  return verdict::wedge_scaled_jacobian(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeShape(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  double pc[6][3];
  points->GetTypedTuple(1, pc[0]);
  points->GetTypedTuple(0, pc[1]);
  points->GetTypedTuple(2, pc[2]);
  points->GetTypedTuple(4, pc[3]);
  points->GetTypedTuple(3, pc[4]);
  points->GetTypedTuple(5, pc[5]);
  return verdict::wedge_shape(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeVolume(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  double pc[6][3];
  points->GetTypedTuple(1, pc[0]);
  points->GetTypedTuple(0, pc[1]);
  points->GetTypedTuple(2, pc[2]);
  points->GetTypedTuple(4, pc[3]);
  points->GetTypedTuple(3, pc[4]);
  points->GetTypedTuple(5, pc[5]);
  return verdict::wedge_volume(6, pc);
}

// Hexahedral quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::HexCondition(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_condition(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexDiagonal(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_diagonal(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexDimension(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_dimension(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexDistortion(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    (ct == VTK_QUADRATIC_HEXAHEDRON || ct == VTK_TRIQUADRATIC_HEXAHEDRON) && !linearApproximation
    ? points->GetNumberOfTuples()
    : 8;
  return verdict::hex_distortion(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexEdgeRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_edge_ratio(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexEquiangleSkew(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_equiangle_skew(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexJacobian(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    ct == VTK_TRIQUADRATIC_HEXAHEDRON && !linearApproximation ? points->GetNumberOfTuples() : 8;
  return verdict::hex_jacobian(numPts, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexMaxAspectFrobenius(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_max_aspect_frobenius(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexMaxEdgeRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_max_edge_ratio(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexMedAspectFrobenius(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_med_aspect_frobenius(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexNodalJacobianRatio(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_nodal_jacobian_ratio(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexOddy(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_oddy(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexRelativeSizeSquared(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  if (vtkMeshQuality::HexAverageSize == 0.0)
  {
    vtkLogF(ERROR, "HexAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::hex_relative_size_squared(8, pc, vtkMeshQuality::HexAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexScaledJacobian(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_scaled_jacobian(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexShape(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_shape(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexShapeAndSize(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  if (vtkMeshQuality::HexAverageSize == 0.0)
  {
    vtkLogF(ERROR, "HexAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::hex_shape_and_size(8, pc, vtkMeshQuality::HexAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexShear(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_shear(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexShearAndSize(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  if (vtkMeshQuality::HexAverageSize == 0.0)
  {
    vtkLogF(ERROR, "HexAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::hex_shear_and_size(8, pc, vtkMeshQuality::HexAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexSkew(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_skew(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexStretch(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_stretch(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexTaper(vtkCell* cell, bool vtkNotUsed(linearApproximation))
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  return verdict::hex_taper(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexVolume(vtkCell* cell, bool linearApproximation)
{
  auto points = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData());
  auto pc = reinterpret_cast<double(*)[3]>(points->GetPointer(0));
  const int ct = cell->GetCellType();
  const int numPts =
    (ct == VTK_QUADRATIC_HEXAHEDRON || ct == VTK_TRIQUADRATIC_HEXAHEDRON) && !linearApproximation
    ? points->GetNumberOfTuples()
    : 8;
  return verdict::hex_volume(numPts, pc);
}
VTK_ABI_NAMESPACE_END
