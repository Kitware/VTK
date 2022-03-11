/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeshQuality.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  Copyright 2003-2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: dcthomp@sandia.gov,pppebay@sandia.gov

=========================================================================*/

// VTK_DEPRECATED_IN_9_2_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkMeshQuality.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"

#include "vtk_verdict.h"

#include <limits>

//----------------------------------------------------------------------------
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
  "NodalJacobianRatio", "NormalizedInradius", "SquishIndex", "None" };

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

  void GetStats(double array[5])
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
private:
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
    vtkNew<vtkGenericCell> cell;
    this->Output->GetCell(0, cell);
    // initialize min quality
    this->TriangleStats.Min = this->QuadStats.Min = this->TetStats.Min = this->PyrStats.Min =
      this->WedgeStats.Min = this->HexStats.Min = 0;
    // initialize total quality
    this->TriangleStats.Total = this->QuadStats.Total = this->TetStats.Total =
      this->PyrStats.Total = this->WedgeStats.Total = this->HexStats.Total = 0;
    // initialize max quality
    this->TriangleStats.Max = this->QuadStats.Max = this->TetStats.Max = this->PyrStats.Max =
      this->WedgeStats.Max = this->HexStats.Max = 0;
    // initialize total quality squared
    this->TriangleStats.Total2 = this->QuadStats.Total2 = this->TetStats.Total2 =
      this->PyrStats.Total2 = this->WedgeStats.Total2 = this->HexStats.Total2 = 0;
    // initialize number of cells
    this->TriangleStats.NumCells = this->QuadStats.NumCells = this->TetStats.NumCells =
      this->PyrStats.NumCells = this->WedgeStats.NumCells = this->HexStats.NumCells = 0;
  }

  void Initialize()
  {
    // initialize min quality
    this->TLTriangleStats.Local().Min = this->TLQuadStats.Local().Min =
      this->TLTetStats.Local().Min = this->TLPyrStats.Local().Min = this->TLWedgeStats.Local().Min =
        this->TLHexStats.Local().Min = 0;
    // initialize total quality
    this->TLTriangleStats.Local().Total = this->TLQuadStats.Local().Total =
      this->TLTetStats.Local().Total = this->TLPyrStats.Local().Total =
        this->TLWedgeStats.Local().Total = this->TLHexStats.Local().Total = 0;
    // initialize max quality
    this->TLTriangleStats.Local().Max = this->TLQuadStats.Local().Max =
      this->TLTetStats.Local().Max = this->TLPyrStats.Local().Max = this->TLWedgeStats.Local().Max =
        this->TLHexStats.Local().Max = 0;
    // initialize total quality squared
    this->TLTriangleStats.Local().Total2 = this->TLQuadStats.Local().Total2 =
      this->TLTetStats.Local().Total2 = this->TLPyrStats.Local().Total2 =
        this->TLWedgeStats.Local().Total2 = this->TLHexStats.Local().Total2 = 0;
    // initialize number of cells
    this->TLTriangleStats.Local().NumCells = this->TLQuadStats.Local().NumCells =
      this->TLTetStats.Local().NumCells = this->TLPyrStats.Local().NumCells =
        this->TLWedgeStats.Local().NumCells = this->TLHexStats.Local().NumCells = 0;
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
    vtkCell* cell;
    double area, volume; // area and volume

    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      this->Output->GetCell(cellId, genericCell);
      cell = genericCell->GetRepresentativeCell();

      int cellType = cell->GetCellType();
      LinearizeCell(cellType);

      switch (cellType)
      {
        case VTK_TRIANGLE:
          area = vtkMeshQuality::TriangleArea(cell);
          if (area > triStats.Max)
          {
            if (triStats.Min == triStats.Max)
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
            if (quadStats.Min == quadStats.Max)
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
            if (tetStats.Min == tetStats.Max)
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
            if (pyrStats.Min == pyrStats.Max)
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
            if (wedgeStats.Min == wedgeStats.Max)
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
            if (hexStats.Min == hexStats.Max)
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

  CellQualityStats GetTriangleStats() { return this->TriangleStats; }
  CellQualityStats GetQuadStats() { return this->QuadStats; }
  CellQualityStats GetTetStats() { return this->TetStats; }
  CellQualityStats GetPyrStats() { return this->PyrStats; }
  CellQualityStats GetWedgeStats() { return this->WedgeStats; }
  CellQualityStats GetHexStats() { return this->HexStats; }
};

/**
 * Functor to compute the quality stats of a mesh.
 */
class vtkMeshQualityFunctor
{
private:
  vtkSMPThreadLocalObject<vtkGenericCell> Cell;
  vtkMeshQuality* MeshQuality;
  vtkDataSet* Output;
  vtkSmartPointer<vtkDoubleArray> QualityArray;
  vtkSmartPointer<vtkDoubleArray> ApproxQualityArray;
  vtkSmartPointer<vtkDoubleArray> VolumeArray;
  using CellQualityType = vtkMeshQuality::CellQualityType;
  CellQualityType TriangleQuality, QuadQuality, TetQuality, PyramidQuality, WedgeQuality,
    HexQuality;
  vtkSMPThreadLocal<CellQualityStats> TLTriangleStats, TLQuadStats, TLTetStats, TLPyrStats,
    TLWedgeStats, TLHexStats;
  CellQualityStats TriangleStats, QuadStats, TetStats, PyrStats, WedgeStats, HexStats;

public:
  vtkMeshQualityFunctor(vtkMeshQuality* meshQuality, vtkDataSet* output,
    vtkSmartPointer<vtkDoubleArray> qualityArray,
    vtkSmartPointer<vtkDoubleArray> approxQualityArray, vtkSmartPointer<vtkDoubleArray> volumeArray,
    CellQualityType triangleQuality, CellQualityType quadQuality, CellQualityType tetQuality,
    CellQualityType pyramidQuality, CellQualityType wedgeQuality, CellQualityType hexQuality)
    : MeshQuality(meshQuality)
    , Output(output)
    , QualityArray(qualityArray)
    , ApproxQualityArray(approxQualityArray)
    , VolumeArray(volumeArray)
    , TriangleQuality(triangleQuality)
    , QuadQuality(quadQuality)
    , TetQuality(tetQuality)
    , PyramidQuality(pyramidQuality)
    , WedgeQuality(wedgeQuality)
    , HexQuality(hexQuality)
  {
    // instantiate any data-structure that needs to be cached for parallel execution.
    vtkNew<vtkGenericCell> cell;
    this->Output->GetCell(0, cell);
    // initialize min quality
    this->TriangleStats.Min = this->QuadStats.Min = this->TetStats.Min = this->PyrStats.Min =
      this->WedgeStats.Min = this->HexStats.Min = VTK_DOUBLE_MAX;
    // initialize total quality
    this->TriangleStats.Total = this->QuadStats.Total = this->TetStats.Total =
      this->PyrStats.Total = this->WedgeStats.Total = this->HexStats.Total = 0.0;
    // initialize max quality
    this->TriangleStats.Max = this->QuadStats.Max = this->TetStats.Max = this->PyrStats.Max =
      this->WedgeStats.Max = this->HexStats.Max = VTK_DOUBLE_MIN;
    // initialize total quality squared
    this->TriangleStats.Total2 = this->QuadStats.Total2 = this->TetStats.Total2 =
      this->PyrStats.Total2 = this->WedgeStats.Total2 = this->HexStats.Total2 = 0.0;
    // initialize number of cells
    this->TriangleStats.NumCells = this->QuadStats.NumCells = this->TetStats.NumCells =
      this->PyrStats.NumCells = this->WedgeStats.NumCells = this->HexStats.NumCells = 0;
  }

  void Initialize()
  {
    // initialize min quality
    this->TLTriangleStats.Local().Min = this->TLQuadStats.Local().Min =
      this->TLTetStats.Local().Min = this->TLPyrStats.Local().Min = this->TLWedgeStats.Local().Min =
        this->TLHexStats.Local().Min = VTK_DOUBLE_MAX;
    // initialize total quality
    this->TLTriangleStats.Local().Total = this->TLQuadStats.Local().Total =
      this->TLTetStats.Local().Total = this->TLPyrStats.Local().Total =
        this->TLWedgeStats.Local().Total = this->TLHexStats.Local().Total = 0.0;
    // initialize max quality
    this->TLTriangleStats.Local().Max = this->TLQuadStats.Local().Max =
      this->TLTetStats.Local().Max = this->TLPyrStats.Local().Max = this->TLWedgeStats.Local().Max =
        this->TLHexStats.Local().Max = VTK_DOUBLE_MIN;
    // initialize total quality squared
    this->TLTriangleStats.Local().Total2 = this->TLQuadStats.Local().Total2 =
      this->TLTetStats.Local().Total2 = this->TLPyrStats.Local().Total2 =
        this->TLWedgeStats.Local().Total2 = this->TLHexStats.Local().Total2 = 0.0;
    // initialize number of cells
    this->TLTriangleStats.Local().NumCells = this->TLQuadStats.Local().NumCells =
      this->TLTetStats.Local().NumCells = this->TLPyrStats.Local().NumCells =
        this->TLWedgeStats.Local().NumCells = this->TLHexStats.Local().NumCells = 0;
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
    vtkCell* cell;
    vtkDoubleArray* qualityArrays[2] = { this->QualityArray, this->ApproxQualityArray };
    double quality;
    double volume;

    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      this->Output->GetCell(cellId, genericCell);
      cell = genericCell->GetRepresentativeCell();
      volume = 0.0;

      int numberOfOutputQualities = this->MeshQuality->LinearApproximation ? 2 : 1;

      int cellType = cell->GetCellType();

      for (int qualityId = 0; qualityId < numberOfOutputQualities; ++qualityId)
      {
        switch (cellType)
        {
          case VTK_TRIANGLE:
            quality = this->TriangleQuality(cell);
            if (quality > triStats.Max)
            {
              if (triStats.Min > triStats.Max)
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
          case VTK_QUAD:
            quality = this->QuadQuality(cell);
            if (quality > quadStats.Max)
            {
              if (quadStats.Min > quadStats.Max)
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
          case VTK_TETRA:
            quality = this->TetQuality(cell);
            if (quality > tetStats.Max)
            {
              if (tetStats.Min > tetStats.Max)
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
            if (this->MeshQuality->Volume)
            {
              volume = vtkMeshQuality::TetVolume(cell);
              if (!this->MeshQuality->CompatibilityMode)
              {
                this->VolumeArray->SetValue(cellId, volume);
              }
            }
            break;
          case VTK_PYRAMID:
            quality = this->PyramidQuality(cell);
            if (quality > pyrStats.Max)
            {
              if (pyrStats.Min > pyrStats.Max)
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
            quality = this->WedgeQuality(cell);
            if (quality > wedgeStats.Max)
            {
              if (wedgeStats.Min > wedgeStats.Max)
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
            quality = this->HexQuality(cell);
            if (quality > hexStats.Max)
            {
              if (hexStats.Min > hexStats.Max)
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
          default:
            quality = std::numeric_limits<double>::quiet_NaN();
        }

        if (this->MeshQuality->SaveCellQuality)
        {
          if (this->MeshQuality->CompatibilityMode && this->MeshQuality->Volume)
          {
            double t[2] = { volume, quality };
            qualityArrays[qualityId]->SetTypedTuple(cellId, t);
          }
          else
          {
            qualityArrays[qualityId]->SetTypedTuple(cellId, &quality);
          }
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

  CellQualityStats GetTriangleStats() { return this->TriangleStats; }
  CellQualityStats GetQuadStats() { return this->QuadStats; }
  CellQualityStats GetTetStats() { return this->TetStats; }
  CellQualityStats GetPyrStats() { return this->PyrStats; }
  CellQualityStats GetWedgeStats() { return this->WedgeStats; }
  CellQualityStats GetHexStats() { return this->HexStats; }
};

//----------------------------------------------------------------------------
void vtkMeshQuality::PrintSelf(ostream& os, vtkIndent indent)
{
  const char onStr[] = "On";
  const char offStr[] = "Off";

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
  os << indent << "Volume: " << (this->Volume ? onStr : offStr) << endl;
  os << indent << "CompatibilityMode: " << (this->CompatibilityMode ? onStr : offStr) << endl;
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
  this->Volume = 0;
  this->CompatibilityMode = 0;
  this->LinearApproximation = false;
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetTriangleQualityMeasureFunction()
{
  switch (this->GetTriangleQualityMeasure())
  {
    case QualityMeasureTypes::AREA:
      return TriangleArea;
    case QualityMeasureTypes::EDGE_RATIO:
      return TriangleEdgeRatio;
    case QualityMeasureTypes::ASPECT_RATIO:
      return TriangleAspectRatio;
    case QualityMeasureTypes::RADIUS_RATIO:
      return TriangleRadiusRatio;
    case QualityMeasureTypes::ASPECT_FROBENIUS:
      return TriangleAspectFrobenius;
    case QualityMeasureTypes::MIN_ANGLE:
      return TriangleMinAngle;
    case QualityMeasureTypes::MAX_ANGLE:
      return TriangleMaxAngle;
    case QualityMeasureTypes::CONDITION:
      return TriangleCondition;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return TriangleScaledJacobian;
    case QualityMeasureTypes::RELATIVE_SIZE_SQUARED:
      return TriangleRelativeSizeSquared;
    case QualityMeasureTypes::SHAPE:
      return TriangleShape;
    case QualityMeasureTypes::SHAPE_AND_SIZE:
      return TriangleShapeAndSize;
    case QualityMeasureTypes::DISTORTION:
      return TriangleDistortion;
    case QualityMeasureTypes::EQUIANGLE_SKEW:
      return TriangleEquiangleSkew;
    case QualityMeasureTypes::NORMALIZED_INRADIUS:
      return TriangleNormalizedInradius;
    default:
      vtkWarningMacro("Bad TriangleQualityMeasure ("
        << static_cast<int>(this->GetTriangleQualityMeasure()) << "), using RadiusRatio instead");
      return TriangleRadiusRatio;
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetQuadQualityMeasureFunction()
{
  switch (this->GetQuadQualityMeasure())
  {
    case QualityMeasureTypes::EDGE_RATIO:
      return QuadEdgeRatio;
    case QualityMeasureTypes::ASPECT_RATIO:
      return QuadAspectRatio;
    case QualityMeasureTypes::RADIUS_RATIO:
      return QuadRadiusRatio;
    case QualityMeasureTypes::MED_ASPECT_FROBENIUS:
      return QuadMedAspectFrobenius;
    case QualityMeasureTypes::MAX_ASPECT_FROBENIUS:
      return QuadMaxAspectFrobenius;
    case QualityMeasureTypes::MIN_ANGLE:
      return QuadMinAngle;
    case QualityMeasureTypes::MAX_EDGE_RATIO:
      return QuadMaxEdgeRatio;
    case QualityMeasureTypes::SKEW:
      return QuadSkew;
    case QualityMeasureTypes::TAPER:
      return QuadTaper;
    case QualityMeasureTypes::WARPAGE:
      return QuadWarpage;
    case QualityMeasureTypes::AREA:
      return QuadArea;
    case QualityMeasureTypes::STRETCH:
      return QuadStretch;
    case QualityMeasureTypes::MAX_ANGLE:
      return QuadMaxAngle;
    case QualityMeasureTypes::ODDY:
      return QuadOddy;
    case QualityMeasureTypes::CONDITION:
      return QuadCondition;
    case QualityMeasureTypes::JACOBIAN:
      return QuadJacobian;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return QuadScaledJacobian;
    case QualityMeasureTypes::SHEAR:
      return QuadShear;
    case QualityMeasureTypes::SHAPE:
      return QuadShape;
    case QualityMeasureTypes::RELATIVE_SIZE_SQUARED:
      return QuadRelativeSizeSquared;
    case QualityMeasureTypes::SHAPE_AND_SIZE:
      return QuadShapeAndSize;
    case QualityMeasureTypes::SHEAR_AND_SIZE:
      return QuadShearAndSize;
    case QualityMeasureTypes::DISTORTION:
      return QuadDistortion;
    case QualityMeasureTypes::EQUIANGLE_SKEW:
      return QuadEquiangleSkew;
    default:
      vtkWarningMacro("Bad QuadQualityMeasure (" << static_cast<int>(this->GetQuadQualityMeasure())
                                                 << "), using EdgeRatio instead");
      return QuadEdgeRatio;
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetTetQualityMeasureFunction()
{
  switch (this->GetTetQualityMeasure())
  {
    case QualityMeasureTypes::EDGE_RATIO:
      return TetEdgeRatio;
    case QualityMeasureTypes::ASPECT_RATIO:
      return TetAspectRatio;
    case QualityMeasureTypes::RADIUS_RATIO:
      return TetRadiusRatio;
    case QualityMeasureTypes::ASPECT_FROBENIUS:
      return TetAspectFrobenius;
    case QualityMeasureTypes::MIN_ANGLE:
      return TetMinAngle;
    case QualityMeasureTypes::COLLAPSE_RATIO:
      return TetCollapseRatio;
    case QualityMeasureTypes::ASPECT_GAMMA:
      return TetAspectGamma;
    case QualityMeasureTypes::VOLUME:
      return TetVolume;
    case QualityMeasureTypes::CONDITION:
      return TetCondition;
    case QualityMeasureTypes::JACOBIAN:
      return TetJacobian;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return TetScaledJacobian;
    case QualityMeasureTypes::SHAPE:
      return TetShape;
    case QualityMeasureTypes::RELATIVE_SIZE_SQUARED:
      return TetRelativeSizeSquared;
    case QualityMeasureTypes::SHAPE_AND_SIZE:
      return TetShapeAndSize;
    case QualityMeasureTypes::DISTORTION:
      return TetDistortion;
    case QualityMeasureTypes::EQUIANGLE_SKEW:
      return TetEquiangleSkew;
    case QualityMeasureTypes::EQUIVOLUME_SKEW:
      return TetEquivolumeSkew;
    case QualityMeasureTypes::MEAN_RATIO:
      return TetMeanRatio;
    case QualityMeasureTypes::NORMALIZED_INRADIUS:
      return TetNormalizedInradius;
    case QualityMeasureTypes::SQUISH_INDEX:
      return TetSquishIndex;
    default:
      vtkWarningMacro("Bad TetQualityMeasure (" << static_cast<int>(this->GetTetQualityMeasure())
                                                << "), using RadiusRatio instead");
      return TetRadiusRatio;
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetPyramidQualityMeasureFunction()
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
      vtkWarningMacro("Bad PyramidQualityMeasure ("
        << static_cast<int>(this->GetPyramidQualityMeasure()) << "), using Shape instead");
      return PyramidShape;
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetWedgeQualityMeasureFunction()
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
      vtkWarningMacro("Bad WedgeQualityMeasure ("
        << static_cast<int>(this->GetWedgeQualityMeasure()) << "), using EdgeRatio instead");
      return WedgeEdgeRatio;
  }
}

vtkMeshQuality::CellQualityType vtkMeshQuality::GetHexQualityMeasureFunction()
{
  switch (this->GetHexQualityMeasure())
  {
    case QualityMeasureTypes::EDGE_RATIO:
      return HexEdgeRatio;
    case QualityMeasureTypes::MED_ASPECT_FROBENIUS:
      return HexMedAspectFrobenius;
    case QualityMeasureTypes::MAX_ASPECT_FROBENIUS:
      return HexMaxAspectFrobenius;
    case QualityMeasureTypes::MAX_EDGE_RATIO:
      return HexMaxEdgeRatio;
    case QualityMeasureTypes::SKEW:
      return HexSkew;
    case QualityMeasureTypes::TAPER:
      return HexTaper;
    case QualityMeasureTypes::VOLUME:
      return HexVolume;
    case QualityMeasureTypes::STRETCH:
      return HexStretch;
    case QualityMeasureTypes::DIAGONAL:
      return HexDiagonal;
    case QualityMeasureTypes::DIMENSION:
      return HexDimension;
    case QualityMeasureTypes::ODDY:
      return HexOddy;
    case QualityMeasureTypes::CONDITION:
      return HexCondition;
    case QualityMeasureTypes::JACOBIAN:
      return HexJacobian;
    case QualityMeasureTypes::SCALED_JACOBIAN:
      return HexScaledJacobian;
    case QualityMeasureTypes::SHEAR:
      return HexShear;
    case QualityMeasureTypes::SHAPE:
      return HexShape;
    case QualityMeasureTypes::RELATIVE_SIZE_SQUARED:
      return HexRelativeSizeSquared;
    case QualityMeasureTypes::SHAPE_AND_SIZE:
      return HexShapeAndSize;
    case QualityMeasureTypes::SHEAR_AND_SIZE:
      return HexShearAndSize;
    case QualityMeasureTypes::DISTORTION:
      return HexDistortion;
    case QualityMeasureTypes::EQUIANGLE_SKEW:
      return HexEquiangleSkew;
    case QualityMeasureTypes::NODAL_JACOBIAN_RATIO:
      return HexNodalJacobianRatio;
    default:
      vtkWarningMacro("Bad HexQualityMeasure (" << static_cast<int>(this->GetTetQualityMeasure())
                                                << "), using MaxAspectFrobenius instead");
      return HexMaxAspectFrobenius;
  }
}

//----------------------------------------------------------------------------
int vtkMeshQuality::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* in = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* out = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSmartPointer<vtkDoubleArray> qualityArray = nullptr;
  vtkSmartPointer<vtkDoubleArray> approxQualityArray = nullptr;
  vtkSmartPointer<vtkDoubleArray> volumeArray = nullptr;
  vtkIdType numberOfCells = in->GetNumberOfCells();

  CellQualityType TriangleQuality = this->GetTriangleQualityMeasureFunction(),
                  QuadQuality = this->GetQuadQualityMeasureFunction(),
                  TetQuality = this->GetTetQualityMeasureFunction(),
                  PyramidQuality = this->GetPyramidQualityMeasureFunction(),
                  WedgeQuality = this->GetWedgeQualityMeasureFunction(),
                  HexQuality = this->GetHexQualityMeasureFunction();

  out->ShallowCopy(in);

  if (this->SaveCellQuality)
  {
    qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
    if (this->CompatibilityMode)
    {
      if (this->Volume)
      {
        qualityArray->SetNumberOfComponents(2);
      }
      else
      {
        qualityArray->SetNumberOfComponents(1);
      }
    }
    else
    {
      qualityArray->SetNumberOfComponents(1);
    }
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

    if (!this->CompatibilityMode)
    {
      if (this->Volume)
      {
        volumeArray = vtkSmartPointer<vtkDoubleArray>::New();
        volumeArray->SetNumberOfComponents(1);
        volumeArray->SetNumberOfTuples(numberOfCells);
        volumeArray->SetName("Volume");
        out->GetCellData()->AddArray(volumeArray);
      }
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
    vtkDataArray* triAreaHint = in->GetFieldData()->GetArray("TriArea");
    vtkDataArray* quadAreaHint = in->GetFieldData()->GetArray("QuadArea");
    vtkDataArray* tetVolHint = in->GetFieldData()->GetArray("TetVolume");
    vtkDataArray* pyrVolHint = in->GetFieldData()->GetArray("PyrVolume");
    vtkDataArray* wedgeVolHint = in->GetFieldData()->GetArray("WedgeVolume");
    vtkDataArray* hexVolHint = in->GetFieldData()->GetArray("HexVolume");

    double triAreaTuple[5];
    double quadAreaTuple[5];
    double tetVolTuple[5];
    double pyrVolTuple[5];
    double wedgeVolTuple[5];
    double hexVolTuple[5];

    if (triAreaHint && triAreaHint->GetNumberOfTuples() > 0 &&
      triAreaHint->GetNumberOfComponents() == 5 && quadAreaHint &&
      quadAreaHint->GetNumberOfTuples() > 0 && quadAreaHint->GetNumberOfComponents() == 5 &&
      tetVolHint && tetVolHint->GetNumberOfTuples() > 0 &&
      tetVolHint->GetNumberOfComponents() == 5 && pyrVolHint &&
      pyrVolHint->GetNumberOfTuples() > 0 && pyrVolHint->GetNumberOfComponents() == 5 &&
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
      vtkSizeFunctor sizeFunctor(out);
      vtkSMPTools::For(0, numberOfCells, sizeFunctor);

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
      out->GetFieldData()->AddArray(triAreaHint);
      triAreaHint->Delete();

      quadAreaHint = vtkDoubleArray::New();
      quadAreaHint->SetName("QuadArea");
      quadAreaHint->SetNumberOfComponents(5);
      quadAreaHint->InsertNextTuple(quadAreaTuple);
      out->GetFieldData()->AddArray(quadAreaHint);
      quadAreaHint->Delete();

      tetVolHint = vtkDoubleArray::New();
      tetVolHint->SetName("TetVolume");
      tetVolHint->SetNumberOfComponents(5);
      tetVolHint->InsertNextTuple(tetVolTuple);
      out->GetFieldData()->AddArray(tetVolHint);
      tetVolHint->Delete();

      pyrVolHint = vtkDoubleArray::New();
      pyrVolHint->SetName("PyrVolume");
      pyrVolHint->SetNumberOfComponents(5);
      pyrVolHint->InsertNextTuple(pyrVolTuple);
      out->GetFieldData()->AddArray(pyrVolHint);
      pyrVolHint->Delete();

      wedgeVolHint = vtkDoubleArray::New();
      wedgeVolHint->SetName("WedgeVolume");
      wedgeVolHint->SetNumberOfComponents(5);
      wedgeVolHint->InsertNextTuple(wedgeVolTuple);
      out->GetFieldData()->AddArray(wedgeVolHint);
      wedgeVolHint->Delete();

      hexVolHint = vtkDoubleArray::New();
      hexVolHint->SetName("HexVolume");
      hexVolHint->SetNumberOfComponents(5);
      hexVolHint->InsertNextTuple(hexVolTuple);
      out->GetFieldData()->AddArray(hexVolHint);
      hexVolHint->Delete();
    }
  }

  vtkMeshQualityFunctor meshQualityFunctor(this, out, qualityArray, approxQualityArray, volumeArray,
    TriangleQuality, QuadQuality, TetQuality, PyramidQuality, WedgeQuality, HexQuality);
  vtkSMPTools::For(0, numberOfCells, meshQualityFunctor);

  CellQualityStats triangleStats = meshQualityFunctor.GetTriangleStats();
  CellQualityStats quadStats = meshQualityFunctor.GetQuadStats();
  CellQualityStats tetStats = meshQualityFunctor.GetTetStats();
  CellQualityStats pyrStats = meshQualityFunctor.GetPyrStats();
  CellQualityStats wedgeStats = meshQualityFunctor.GetWedgeStats();
  CellQualityStats hexStats = meshQualityFunctor.GetHexStats();

  if (triangleStats.NumCells)
  {
    triangleStats.Total /= static_cast<double>(triangleStats.NumCells);
    double multFactor = 1. /
      static_cast<double>(
        triangleStats.NumCells > 1 ? triangleStats.NumCells - 1 : triangleStats.NumCells);
    triangleStats.Total2 = multFactor *
      (triangleStats.Total2 -
        static_cast<double>(triangleStats.NumCells) * triangleStats.Total * triangleStats.Total);
  }
  else
  {
    triangleStats.Min = triangleStats.Max = triangleStats.Total = triangleStats.Total2 = 0.;
  }

  if (quadStats.NumCells)
  {
    quadStats.Total /= static_cast<double>(quadStats.NumCells);
    double multFactor = 1. /
      static_cast<double>(quadStats.NumCells > 1 ? quadStats.NumCells - 1 : quadStats.NumCells);
    quadStats.Total2 = multFactor *
      (quadStats.Total2 -
        static_cast<double>(quadStats.NumCells) * quadStats.Total * quadStats.Total);
  }
  else
  {
    quadStats.Min = quadStats.Max = quadStats.Total = quadStats.Total2 = 0.;
  }

  if (tetStats.NumCells)
  {
    tetStats.Total /= static_cast<double>(tetStats.NumCells);
    double multFactor =
      1. / static_cast<double>(tetStats.NumCells > 1 ? tetStats.NumCells - 1 : tetStats.NumCells);
    tetStats.Total2 = multFactor *
      (tetStats.Total2 - static_cast<double>(tetStats.NumCells) * tetStats.Total * tetStats.Total);
  }
  else
  {
    tetStats.Min = tetStats.Max = tetStats.Total = tetStats.Total2 = 0.;
  }

  if (pyrStats.NumCells)
  {
    pyrStats.Total /= static_cast<double>(pyrStats.NumCells);
    double multFactor =
      1. / static_cast<double>(pyrStats.NumCells > 1 ? pyrStats.NumCells - 1 : pyrStats.NumCells);
    pyrStats.Total2 = multFactor *
      (pyrStats.Total2 - static_cast<double>(pyrStats.NumCells) * pyrStats.Total * pyrStats.Total);
  }
  else
  {
    pyrStats.Min = pyrStats.Max = pyrStats.Total = pyrStats.Total2 = 0.;
  }

  if (wedgeStats.NumCells)
  {
    wedgeStats.Total /= static_cast<double>(wedgeStats.NumCells);
    double multFactor = 1. /
      static_cast<double>(wedgeStats.NumCells > 1 ? wedgeStats.NumCells - 1 : wedgeStats.NumCells);
    wedgeStats.Total2 = multFactor *
      (wedgeStats.Total2 -
        static_cast<double>(wedgeStats.NumCells) * wedgeStats.Total * wedgeStats.Total);
  }
  else
  {
    wedgeStats.Min = wedgeStats.Max = wedgeStats.Total = wedgeStats.Total2 = 0.;
  }

  if (hexStats.NumCells)
  {
    hexStats.Total /= static_cast<double>(hexStats.NumCells);
    double multFactor =
      1. / static_cast<double>(hexStats.NumCells > 1 ? hexStats.NumCells - 1 : hexStats.NumCells);
    hexStats.Total2 = multFactor *
      (hexStats.Total2 - static_cast<double>(hexStats.NumCells) * hexStats.Total * hexStats.Total);
  }
  else
  {
    hexStats.Min = hexStats.Max = hexStats.Total = hexStats.Total2 = 0.;
  }

  double tuple[5];
  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Triangle Quality");
  qualityArray->SetNumberOfComponents(5);
  triangleStats.GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Quadrilateral Quality");
  qualityArray->SetNumberOfComponents(5);
  quadStats.GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Tetrahedron Quality");
  qualityArray->SetNumberOfComponents(5);
  tetStats.GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Pyramid Quality");
  qualityArray->SetNumberOfComponents(5);
  pyrStats.GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Wedge Quality");
  qualityArray->SetNumberOfComponents(5);
  wedgeStats.GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Hexahedron Quality");
  qualityArray->SetNumberOfComponents(5);
  hexStats.GetStats(tuple);
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  return 1;
}

// Triangle quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleArea(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_area(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleEdgeRatio(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_edge_ratio(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleAspectRatio(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_aspect_ratio(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleRadiusRatio(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_radius_ratio(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleAspectFrobenius(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_aspect_frobenius(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleMinAngle(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_minimum_angle(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleMaxAngle(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_maximum_angle(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleCondition(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_condition(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleScaledJacobian(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_scaled_jacobian(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleRelativeSizeSquared(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  if (vtkMeshQuality::TriangleAverageSize == 0.0)
  {
    vtkLogF(ERROR, "TriangleAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::tri_relative_size_squared(3, pc, vtkMeshQuality::TriangleAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleShape(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_shape(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleShapeAndSize(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  if (vtkMeshQuality::TriangleAverageSize == 0.0)
  {
    vtkLogF(ERROR, "TriangleAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::tri_shape_and_size(3, pc, vtkMeshQuality::TriangleAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleDistortion(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_distortion(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleEquiangleSkew(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_equiangle_skew(3, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleNormalizedInradius(vtkCell* cell)
{
  double pc[3][3];

  vtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return verdict::tri_normalized_inradius(3, pc);
}

// Quadrangle quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadEdgeRatio(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_edge_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadAspectRatio(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_aspect_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadRadiusRatio(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_radius_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadMedAspectFrobenius(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_med_aspect_frobenius(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadMaxAspectFrobenius(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_max_aspect_frobenius(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadMinAngle(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_minimum_angle(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadMaxEdgeRatio(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_max_edge_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadSkew(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_skew(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadTaper(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_taper(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadWarpage(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_warpage(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadArea(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_area(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadStretch(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_stretch(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadMaxAngle(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_maximum_angle(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadOddy(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_oddy(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadCondition(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_condition(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadJacobian(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_jacobian(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadScaledJacobian(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_scaled_jacobian(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadShear(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_shear(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadShape(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_shape(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadRelativeSizeSquared(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  if (vtkMeshQuality::QuadAverageSize == 0.0)
  {
    vtkLogF(ERROR, "QuadAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::quad_relative_size_squared(4, pc, vtkMeshQuality::QuadAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadShapeAndSize(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  if (vtkMeshQuality::QuadAverageSize == 0.0)
  {
    vtkLogF(ERROR, "QuadAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::quad_shape_and_size(4, pc, vtkMeshQuality::QuadAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadShearAndSize(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  if (vtkMeshQuality::QuadAverageSize == 0.0)
  {
    vtkLogF(ERROR, "QuadAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::quad_shear_and_size(4, pc, vtkMeshQuality::QuadAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadDistortion(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_distortion(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::QuadEquiangleSkew(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::quad_equiangle_skew(4, pc);
}

// Tetrahedral quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::TetEdgeRatio(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_edge_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetAspectRatio(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_aspect_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetRadiusRatio(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_radius_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetAspectFrobenius(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_aspect_frobenius(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetMinAngle(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_minimum_angle(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetCollapseRatio(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_collapse_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetAspectGamma(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_aspect_gamma(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetVolume(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_volume(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetCondition(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_condition(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetJacobian(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_jacobian(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetScaledJacobian(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_scaled_jacobian(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetShape(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_shape(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetRelativeSizeSquared(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  if (vtkMeshQuality::TetAverageSize == 0.0)
  {
    vtkLogF(ERROR, "TetAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::tet_relative_size_squared(4, pc, vtkMeshQuality::TetAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetShapeAndSize(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  if (vtkMeshQuality::TetAverageSize == 0.0)
  {
    vtkLogF(ERROR, "TetAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::tet_shape_and_size(4, pc, vtkMeshQuality::TetAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetDistortion(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_distortion(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetEquiangleSkew(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_equiangle_skew(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetEquivolumeSkew(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_equivolume_skew(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetMeanRatio(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_mean_ratio(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetNormalizedInradius(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_normalized_inradius(4, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::TetSquishIndex(vtkCell* cell)
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::tet_squish_index(4, pc);
}

// Pyramid quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::PyramidEquiangleSkew(vtkCell* cell)
{
  double pc[5][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 5; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::pyramid_equiangle_skew(5, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::PyramidJacobian(vtkCell* cell)
{
  double pc[5][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 5; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::pyramid_jacobian(5, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::PyramidScaledJacobian(vtkCell* cell)
{
  double pc[5][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 5; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::pyramid_scaled_jacobian(5, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::PyramidShape(vtkCell* cell)
{
  double pc[5][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 5; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::pyramid_shape(5, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::PyramidVolume(vtkCell* cell)
{
  double pc[5][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 5; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::pyramid_volume(5, pc);
}

// Wedge quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeCondition(vtkCell* cell)
{
  double pc[6][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 6; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::wedge_condition(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeDistortion(vtkCell* cell)
{
  double pc[6][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 6; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::wedge_distortion(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeEdgeRatio(vtkCell* cell)
{
  double pc[6][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 6; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::wedge_edge_ratio(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeEquiangleSkew(vtkCell* cell)
{
  double pc[6][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 6; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::wedge_equiangle_skew(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeJacobian(vtkCell* cell)
{
  double pc[6][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 6; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::wedge_jacobian(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeMaxAspectFrobenius(vtkCell* cell)
{
  double pc[6][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 6; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::wedge_max_aspect_frobenius(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeMaxStretch(vtkCell* cell)
{
  double pc[6][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 6; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::wedge_max_stretch(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeMeanAspectFrobenius(vtkCell* cell)
{
  double pc[6][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 6; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::wedge_mean_aspect_frobenius(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeScaledJacobian(vtkCell* cell)
{
  double pc[6][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 6; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::wedge_scaled_jacobian(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeShape(vtkCell* cell)
{
  double pc[6][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 6; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::wedge_shape(6, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::WedgeVolume(vtkCell* cell)
{
  double pc[6][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 6; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::wedge_volume(6, pc);
}

// Hexahedral quality metrics

//----------------------------------------------------------------------------
double vtkMeshQuality::HexEdgeRatio(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_edge_ratio(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexMedAspectFrobenius(vtkCell* cell)
{
  double pc[8][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_med_aspect_frobenius(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexMaxAspectFrobenius(vtkCell* cell)
{
  double pc[8][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_max_aspect_frobenius(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexMaxEdgeRatio(vtkCell* cell)
{
  double pc[8][3];

  vtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_max_edge_ratio(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexSkew(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_skew(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexTaper(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_taper(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexVolume(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_volume(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexStretch(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_stretch(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexDiagonal(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_diagonal(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexDimension(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_dimension(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexOddy(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_oddy(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexCondition(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_condition(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexJacobian(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_jacobian(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexScaledJacobian(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_scaled_jacobian(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexShear(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_shear(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexShape(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_shape(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexRelativeSizeSquared(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  if (vtkMeshQuality::HexAverageSize == 0.0)
  {
    vtkLogF(ERROR, "HexAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::hex_relative_size_squared(8, pc, vtkMeshQuality::HexAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexShapeAndSize(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  if (vtkMeshQuality::HexAverageSize == 0.0)
  {
    vtkLogF(ERROR, "HexAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::hex_shape_and_size(8, pc, vtkMeshQuality::HexAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexShearAndSize(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  if (vtkMeshQuality::HexAverageSize == 0.0)
  {
    vtkLogF(ERROR, "HexAverageSize is not set. Execute vtkMeshQuality!");
    return 0.0;
  }
  return verdict::hex_shear_and_size(8, pc, vtkMeshQuality::HexAverageSize);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexDistortion(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_distortion(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexEquiangleSkew(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_equiangle_skew(8, pc);
}

//----------------------------------------------------------------------------
double vtkMeshQuality::HexNodalJacobianRatio(vtkCell* cell)
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
  {
    p->GetPoint(i, pc[i]);
  }

  return verdict::hex_nodal_jacobian_ratio(8, pc);
}
