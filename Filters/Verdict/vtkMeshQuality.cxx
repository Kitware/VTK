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
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

#include "vtk_verdict.h"

#include <limits>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMeshQuality);

//----------------------------------------------------------------------------
double vtkMeshQuality::TriangleAverageSize = 0.0;
double vtkMeshQuality::QuadAverageSize = 0.0;
double vtkMeshQuality::TetAverageSize = 0.0;
double vtkMeshQuality::HexAverageSize = 0.0;

//----------------------------------------------------------------------------
const char* vtkMeshQuality::QualityMeasureNames[] = { "EdgeRatio", "AspectRatio", "RadiusRatio",
  "AspectFrobenius", "MedAspectFrobenius", "MaxAspectFrobenius", "MinAngle", "CollapseRatio",
  "MaxAngle", "Condition", "ScaledJacobian", "Shear", "RelativeSizeSquared", "Shape",
  "ShapeAndSize", "Distortion", "MaxEdgeRatio", "Skew", "Taper", "Volume", "Stretch", "Diagonal",
  "Dimension", "Oddy", "ShearAndSize", "Jacobian", "Warpage", "AspectGamma", "Area", "None" };

namespace
{
typedef double (*CellQualityType)(vtkCell*);
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

//----------------------------------------------------------------------------
void vtkMeshQuality::PrintSelf(ostream& os, vtkIndent indent)
{
  const char onStr[] = "On";
  const char offStr[] = "Off";

  this->Superclass::PrintSelf(os, indent);

  os << indent << "SaveCellQuality:   " << (this->SaveCellQuality ? onStr : offStr) << endl;
  os << indent << "TriangleQualityMeasure: " << QualityMeasureNames[this->TriangleQualityMeasure]
     << endl;
  os << indent << "QuadQualityMeasure: " << QualityMeasureNames[this->QuadQualityMeasure] << endl;
  os << indent << "TetQualityMeasure: " << QualityMeasureNames[this->TetQualityMeasure] << endl;
  os << indent << "HexQualityMeasure: " << QualityMeasureNames[this->HexQualityMeasure] << endl;
  os << indent << "Volume: " << (this->Volume ? onStr : offStr) << endl;
  os << indent << "CompatibilityMode: " << (this->CompatibilityMode ? onStr : offStr) << endl;
}

//----------------------------------------------------------------------------
vtkMeshQuality::vtkMeshQuality()
{
  this->SaveCellQuality = 1; // Default is On
  this->TriangleQualityMeasure = vtkMeshQuality::ASPECT_RATIO;
  this->QuadQualityMeasure = vtkMeshQuality::EDGE_RATIO;
  this->TetQualityMeasure = vtkMeshQuality::ASPECT_RATIO;
  this->HexQualityMeasure = vtkMeshQuality::MAX_ASPECT_FROBENIUS;
  this->Volume = 0;
  this->CompatibilityMode = 0;
  this->LinearApproximation = false;
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

  CellQualityType TriangleQuality, QuadQuality, TetQuality, HexQuality;
  vtkDoubleArray* quality = nullptr;
  vtkSmartPointer<vtkDoubleArray> approxQuality = nullptr;
  vtkDoubleArray* volume = nullptr;
  vtkIdType N = in->GetNumberOfCells();
  double qtrim, qtriM, Eqtri, Eqtri2;
  double qquam, qquaM, Eqqua, Eqqua2;
  double qtetm, qtetM, Eqtet, Eqtet2;
  double qhexm, qhexM, Eqhex, Eqhex2;
  double q;
  double V = 0.;
  vtkIdType ntri = 0;
  vtkIdType nqua = 0;
  vtkIdType ntet = 0;
  vtkIdType nhex = 0;
  vtkCell* cell;
  int progressNumer = 0;
  double progressDenom = 20.;

  // Initialize the min and max values, std deviations, etc.
  qtriM = qquaM = qtetM = qhexM = VTK_DOUBLE_MIN;
  qtrim = qquam = qtetm = qhexm = VTK_DOUBLE_MAX;
  Eqtri = Eqtri2 = Eqqua = Eqqua2 = Eqtet = Eqtet2 = Eqhex = Eqhex2 = 0.;

  switch (this->GetTriangleQualityMeasure())
  {
    case vtkMeshQuality::AREA:
      TriangleQuality = TriangleArea;
      break;
    case vtkMeshQuality::EDGE_RATIO:
      TriangleQuality = TriangleEdgeRatio;
      break;
    case vtkMeshQuality::ASPECT_RATIO:
      TriangleQuality = TriangleAspectRatio;
      break;
    case vtkMeshQuality::RADIUS_RATIO:
      TriangleQuality = TriangleRadiusRatio;
      break;
    case vtkMeshQuality::ASPECT_FROBENIUS:
      TriangleQuality = TriangleAspectFrobenius;
      break;
    case vtkMeshQuality::MIN_ANGLE:
      TriangleQuality = TriangleMinAngle;
      break;
    case vtkMeshQuality::MAX_ANGLE:
      TriangleQuality = TriangleMaxAngle;
      break;
    case vtkMeshQuality::CONDITION:
      TriangleQuality = TriangleCondition;
      break;
    case vtkMeshQuality::SCALED_JACOBIAN:
      TriangleQuality = TriangleScaledJacobian;
      break;
    case vtkMeshQuality::RELATIVE_SIZE_SQUARED:
      TriangleQuality = TriangleRelativeSizeSquared;
      break;
    case vtkMeshQuality::SHAPE:
      TriangleQuality = TriangleShape;
      break;
    case vtkMeshQuality::SHAPE_AND_SIZE:
      TriangleQuality = TriangleShapeAndSize;
      break;
    case vtkMeshQuality::DISTORTION:
      TriangleQuality = TriangleDistortion;
      break;
    default:
      vtkWarningMacro("Bad TriangleQualityMeasure (" << this->GetTriangleQualityMeasure()
                                                     << "), using RadiusRatio instead");
      TriangleQuality = TriangleRadiusRatio;
      break;
  }

  switch (this->GetQuadQualityMeasure())
  {
    case vtkMeshQuality::EDGE_RATIO:
      QuadQuality = QuadEdgeRatio;
      break;
    case vtkMeshQuality::ASPECT_RATIO:
      QuadQuality = QuadAspectRatio;
      break;
    case vtkMeshQuality::RADIUS_RATIO:
      QuadQuality = QuadRadiusRatio;
      break;
    case vtkMeshQuality::MED_ASPECT_FROBENIUS:
      QuadQuality = QuadMedAspectFrobenius;
      break;
    case vtkMeshQuality::MAX_ASPECT_FROBENIUS:
      QuadQuality = QuadMaxAspectFrobenius;
      break;
    case vtkMeshQuality::MIN_ANGLE:
      QuadQuality = QuadMinAngle;
      break;
    case vtkMeshQuality::MAX_EDGE_RATIO:
      QuadQuality = QuadMaxEdgeRatios;
      break;
    case vtkMeshQuality::SKEW:
      QuadQuality = QuadSkew;
      break;
    case vtkMeshQuality::TAPER:
      QuadQuality = QuadTaper;
      break;
    case vtkMeshQuality::WARPAGE:
      QuadQuality = QuadWarpage;
      break;
    case vtkMeshQuality::AREA:
      QuadQuality = QuadArea;
      break;
    case vtkMeshQuality::STRETCH:
      QuadQuality = QuadStretch;
      break;
    // case vtkMeshQualitys::MIN_ANGLE:
    case vtkMeshQuality::MAX_ANGLE:
      QuadQuality = QuadMaxAngle;
      break;
    case vtkMeshQuality::ODDY:
      QuadQuality = QuadOddy;
      break;
    case vtkMeshQuality::CONDITION:
      QuadQuality = QuadCondition;
      break;
    case vtkMeshQuality::JACOBIAN:
      QuadQuality = QuadJacobian;
      break;
    case vtkMeshQuality::SCALED_JACOBIAN:
      QuadQuality = QuadScaledJacobian;
      break;
    case vtkMeshQuality::SHEAR:
      QuadQuality = QuadShear;
      break;
    case vtkMeshQuality::SHAPE:
      QuadQuality = QuadShape;
      break;
    case vtkMeshQuality::RELATIVE_SIZE_SQUARED:
      QuadQuality = QuadRelativeSizeSquared;
      break;
    case vtkMeshQuality::SHAPE_AND_SIZE:
      QuadQuality = QuadShapeAndSize;
      break;
    case vtkMeshQuality::SHEAR_AND_SIZE:
      QuadQuality = QuadShearAndSize;
      break;
    case vtkMeshQuality::DISTORTION:
      QuadQuality = QuadDistortion;
      break;
    default:
      vtkWarningMacro("Bad QuadQualityMeasure (" << this->GetQuadQualityMeasure()
                                                 << "), using EdgeRatio instead");
      QuadQuality = QuadEdgeRatio;
      break;
  }

  switch (this->GetTetQualityMeasure())
  {
    case vtkMeshQuality::EDGE_RATIO:
      TetQuality = TetEdgeRatio;
      break;
    case vtkMeshQuality::ASPECT_RATIO:
      TetQuality = TetAspectRatio;
      break;
    case vtkMeshQuality::RADIUS_RATIO:
      TetQuality = TetRadiusRatio;
      break;
    case vtkMeshQuality::ASPECT_FROBENIUS:
      TetQuality = TetAspectFrobenius;
      break;
    case vtkMeshQuality::MIN_ANGLE:
      TetQuality = TetMinAngle;
      break;
    case vtkMeshQuality::COLLAPSE_RATIO:
      TetQuality = TetCollapseRatio;
      break;
    case vtkMeshQuality::ASPECT_GAMMA:
      TetQuality = TetAspectGamma;
      break;
    case vtkMeshQuality::VOLUME:
      TetQuality = TetVolume;
      break;
    case vtkMeshQuality::CONDITION:
      TetQuality = TetCondition;
      break;
    case vtkMeshQuality::JACOBIAN:
      TetQuality = TetJacobian;
      break;
    case vtkMeshQuality::SCALED_JACOBIAN:
      TetQuality = TetScaledJacobian;
      break;
    case vtkMeshQuality::SHAPE:
      TetQuality = TetShape;
      break;
    case vtkMeshQuality::RELATIVE_SIZE_SQUARED:
      TetQuality = TetRelativeSizeSquared;
      break;
    case vtkMeshQuality::SHAPE_AND_SIZE:
      TetQuality = TetShapeAndSize;
      break;
    case vtkMeshQuality::DISTORTION:
      TetQuality = TetDistortion;
      break;
    default:
      vtkWarningMacro("Bad TetQualityMeasure (" << this->GetTetQualityMeasure()
                                                << "), using RadiusRatio instead");
      TetQuality = TetRadiusRatio;
      break;
  }

  switch (this->GetHexQualityMeasure())
  {
    case vtkMeshQuality::EDGE_RATIO:
      HexQuality = HexEdgeRatio;
      break;
    case vtkMeshQuality::MED_ASPECT_FROBENIUS:
      HexQuality = HexMedAspectFrobenius;
      break;
    case vtkMeshQuality::MAX_ASPECT_FROBENIUS:
      HexQuality = HexMaxAspectFrobenius;
      break;
    case vtkMeshQuality::MAX_EDGE_RATIO:
      HexQuality = HexMaxEdgeRatio;
      break;
    case vtkMeshQuality::SKEW:
      HexQuality = HexSkew;
      break;
    case vtkMeshQuality::TAPER:
      HexQuality = HexTaper;
      break;
    case vtkMeshQuality::VOLUME:
      HexQuality = HexVolume;
      break;
    case vtkMeshQuality::STRETCH:
      HexQuality = HexStretch;
      break;
    case vtkMeshQuality::DIAGONAL:
      HexQuality = HexDiagonal;
      break;
    case vtkMeshQuality::DIMENSION:
      HexQuality = HexDimension;
      break;
    case vtkMeshQuality::ODDY:
      HexQuality = HexOddy;
      break;
    case vtkMeshQuality::CONDITION:
      HexQuality = HexCondition;
      break;
    case vtkMeshQuality::JACOBIAN:
      HexQuality = HexJacobian;
      break;
    case vtkMeshQuality::SCALED_JACOBIAN:
      HexQuality = HexScaledJacobian;
      break;
    case vtkMeshQuality::SHEAR:
      HexQuality = HexShear;
      break;
    case vtkMeshQuality::SHAPE:
      HexQuality = HexShape;
      break;
    case vtkMeshQuality::RELATIVE_SIZE_SQUARED:
      HexQuality = HexRelativeSizeSquared;
      break;
    case vtkMeshQuality::SHAPE_AND_SIZE:
      HexQuality = HexShapeAndSize;
      break;
    case vtkMeshQuality::SHEAR_AND_SIZE:
      HexQuality = HexShearAndSize;
      break;
    case vtkMeshQuality::DISTORTION:
      HexQuality = HexDistortion;
      break;
    default:
      vtkWarningMacro("Bad HexQualityMeasure (" << this->GetTetQualityMeasure()
                                                << "), using MaxAspectFrobenius instead");
      HexQuality = HexMaxAspectFrobenius;
      break;
  }

  out->ShallowCopy(in);

  if (this->SaveCellQuality)
  {
    quality = vtkDoubleArray::New();
    if (this->CompatibilityMode)
    {
      if (this->Volume)
      {
        quality->SetNumberOfComponents(2);
      }
      else
      {
        quality->SetNumberOfComponents(1);
      }
    }
    else
    {
      quality->SetNumberOfComponents(1);
    }
    quality->SetNumberOfTuples(N);
    quality->SetName("Quality");
    out->GetCellData()->AddArray(quality);
    out->GetCellData()->SetActiveAttribute("Quality", vtkDataSetAttributes::SCALARS);
    quality->Delete();

    if (this->LinearApproximation)
    {
      approxQuality = vtkSmartPointer<vtkDoubleArray>::New();
      approxQuality->SetNumberOfValues(N);
      approxQuality->SetName("Quality (Linear Approx)");
      out->GetCellData()->AddArray(approxQuality);
    }

    if (!this->CompatibilityMode)
    {
      if (this->Volume)
      {
        volume = vtkDoubleArray::New();
        volume->SetNumberOfComponents(1);
        volume->SetNumberOfTuples(N);
        volume->SetName("Volume");
        out->GetCellData()->AddArray(volume);
        volume->Delete();
      }
    }
  }

  // These measures require the average area/volume for all cells of the same type in the mesh.
  // Either use the hinted value (computed by a previous vtkMeshQuality filter) or compute it.
  if (this->GetTriangleQualityMeasure() == vtkMeshQuality::RELATIVE_SIZE_SQUARED ||
    this->GetTriangleQualityMeasure() == vtkMeshQuality::SHAPE_AND_SIZE ||
    this->GetQuadQualityMeasure() == vtkMeshQuality::RELATIVE_SIZE_SQUARED ||
    this->GetQuadQualityMeasure() == vtkMeshQuality::SHAPE_AND_SIZE ||
    this->GetQuadQualityMeasure() == vtkMeshQuality::SHEAR_AND_SIZE ||
    this->GetTetQualityMeasure() == vtkMeshQuality::RELATIVE_SIZE_SQUARED ||
    this->GetTetQualityMeasure() == vtkMeshQuality::SHAPE_AND_SIZE ||
    this->GetHexQualityMeasure() == vtkMeshQuality::RELATIVE_SIZE_SQUARED ||
    this->GetHexQualityMeasure() == vtkMeshQuality::SHAPE_AND_SIZE ||
    this->GetHexQualityMeasure() == vtkMeshQuality::SHEAR_AND_SIZE)
  {
    vtkDataArray* triAreaHint = in->GetFieldData()->GetArray("TriArea");
    vtkDataArray* quadAreaHint = in->GetFieldData()->GetArray("QuadArea");
    vtkDataArray* tetVolHint = in->GetFieldData()->GetArray("TetVolume");
    vtkDataArray* hexVolHint = in->GetFieldData()->GetArray("HexVolume");

    double triAreaTuple[5];
    double quadAreaTuple[5];
    double tetVolTuple[5];
    double hexVolTuple[5];

    if (triAreaHint && triAreaHint->GetNumberOfTuples() > 0 &&
      triAreaHint->GetNumberOfComponents() == 5 && quadAreaHint &&
      quadAreaHint->GetNumberOfTuples() > 0 && quadAreaHint->GetNumberOfComponents() == 5 &&
      tetVolHint && tetVolHint->GetNumberOfTuples() > 0 &&
      tetVolHint->GetNumberOfComponents() == 5 && hexVolHint &&
      hexVolHint->GetNumberOfTuples() > 0 && hexVolHint->GetNumberOfComponents() == 5)
    {
      triAreaHint->GetTuple(0, triAreaTuple);
      quadAreaHint->GetTuple(0, quadAreaTuple);
      tetVolHint->GetTuple(0, tetVolTuple);
      hexVolHint->GetTuple(0, hexVolTuple);
      vtkMeshQuality::TriangleAverageSize = triAreaTuple[1] / triAreaTuple[4];
      vtkMeshQuality::QuadAverageSize = quadAreaTuple[1] / quadAreaTuple[4];
      vtkMeshQuality::TetAverageSize = tetVolTuple[1] / tetVolTuple[4];
      vtkMeshQuality::HexAverageSize = hexVolTuple[1] / hexVolTuple[4];
    }
    else
    {
      for (int i = 0; i < 5; ++i)
      {
        triAreaTuple[i] = 0;
        quadAreaTuple[i] = 0;
        tetVolTuple[i] = 0;
        hexVolTuple[i] = 0;
      }
      for (vtkIdType c = 0; c < N; ++c)
      {
        double a, v; // area and volume
        cell = out->GetCell(c);

        int cellType = cell->GetCellType();
        LinearizeCell(cellType);

        switch (cellType)
        {
          case VTK_TRIANGLE:
            a = TriangleArea(cell);
            if (a > triAreaTuple[2])
            {
              if (triAreaTuple[0] == triAreaTuple[2])
              { // min == max => min has not been set
                triAreaTuple[0] = a;
              }
              triAreaTuple[2] = a;
            }
            else if (a < triAreaTuple[0])
            {
              triAreaTuple[0] = a;
            }
            triAreaTuple[1] += a;
            triAreaTuple[3] += a * a;
            ntri++;
            break;
          case VTK_QUAD:
            a = QuadArea(cell);
            if (a > quadAreaTuple[2])
            {
              if (quadAreaTuple[0] == quadAreaTuple[2])
              { // min == max => min has not been set
                quadAreaTuple[0] = a;
              }
              quadAreaTuple[2] = a;
            }
            else if (a < quadAreaTuple[0])
            {
              quadAreaTuple[0] = a;
            }
            quadAreaTuple[1] += a;
            quadAreaTuple[3] += a * a;
            nqua++;
            break;
          case VTK_TETRA:
            v = TetVolume(cell);
            if (v > tetVolTuple[2])
            {
              if (tetVolTuple[0] == tetVolTuple[2])
              { // min == max => min has not been set
                tetVolTuple[0] = v;
              }
              tetVolTuple[2] = v;
            }
            else if (v < tetVolTuple[0])
            {
              tetVolTuple[0] = v;
            }
            tetVolTuple[1] += v;
            tetVolTuple[3] += v * v;
            ntet++;
            break;
          case VTK_HEXAHEDRON:
            v = HexVolume(cell);
            if (v > hexVolTuple[2])
            {
              if (hexVolTuple[0] == hexVolTuple[2])
              { // min == max => min has not been set
                hexVolTuple[0] = v;
              }
              hexVolTuple[2] = v;
            }
            else if (v < hexVolTuple[0])
            {
              hexVolTuple[0] = v;
            }
            hexVolTuple[1] += v;
            hexVolTuple[3] += v * v;
            nhex++;
            break;
          default:
            break;
        }
      }
      triAreaTuple[4] = ntri;
      quadAreaTuple[4] = nqua;
      tetVolTuple[4] = ntet;
      hexVolTuple[4] = nhex;
      vtkMeshQuality::TriangleAverageSize = triAreaTuple[1] / triAreaTuple[4];
      vtkMeshQuality::QuadAverageSize = quadAreaTuple[1] / quadAreaTuple[4];
      vtkMeshQuality::TetAverageSize = tetVolTuple[1] / tetVolTuple[4];
      vtkMeshQuality::HexAverageSize = hexVolTuple[1] / hexVolTuple[4];
      progressNumer = 20;
      progressDenom = 40.;
      ntri = 0;
      nqua = 0;
      ntet = 0;
      nhex = 0;

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

      hexVolHint = vtkDoubleArray::New();
      hexVolHint->SetName("HexVolume");
      hexVolHint->SetNumberOfComponents(5);
      hexVolHint->InsertNextTuple(hexVolTuple);
      out->GetFieldData()->AddArray(hexVolHint);
      hexVolHint->Delete();
    }
  }

  int p;
  vtkIdType c = 0;
  vtkIdType sz = N / 20 + 1;
  vtkIdType inner;
  this->UpdateProgress(progressNumer / progressDenom + 0.01);
  for (p = 0; p < 20; ++p)
  {
    for (inner = 0; (inner < sz && c < N); ++c, ++inner)
    {
      cell = out->GetCell(c);
      V = 0.;

      int numberOfOutputQualities = this->LinearApproximation ? 2 : 1;
      vtkDoubleArray* qualityArrays[2] = { quality, approxQuality };

      int cellType = cell->GetCellType();

      for (int qualityId = 0; qualityId < numberOfOutputQualities; ++qualityId)
      {
        switch (cellType)
        {
          case VTK_TRIANGLE:
            q = TriangleQuality(cell);
            if (q > qtriM)
            {
              if (qtrim > qtriM)
              {
                qtrim = q;
              }
              qtriM = q;
            }
            else if (q < qtrim)
            {
              qtrim = q;
            }
            Eqtri += q;
            Eqtri2 += q * q;
            ++ntri;
            break;
          case VTK_QUAD:
            q = QuadQuality(cell);
            if (q > qquaM)
            {
              if (qquam > qquaM)
              {
                qquam = q;
              }
              qquaM = q;
            }
            else if (q < qquam)
            {
              qquam = q;
            }
            Eqqua += q;
            Eqqua2 += q * q;
            ++nqua;
            break;
          case VTK_TETRA:
            q = TetQuality(cell);
            if (q > qtetM)
            {
              if (qtetm > qtetM)
              {
                qtetm = q;
              }
              qtetM = q;
            }
            else if (q < qtetm)
            {
              qtetm = q;
            }
            Eqtet += q;
            Eqtet2 += q * q;
            ++ntet;
            if (this->Volume)
            {
              V = TetVolume(cell);
              if (!this->CompatibilityMode)
              {
                volume->SetTuple1(0, V);
              }
            }
            break;
          case VTK_HEXAHEDRON:
            q = HexQuality(cell);
            if (q > qhexM)
            {
              if (qhexm > qhexM)
              {
                qhexm = q;
              }
              qhexM = q;
            }
            else if (q < qhexm)
            {
              qhexm = q;
            }
            Eqhex += q;
            Eqhex2 += q * q;
            ++nhex;
            break;
          default:
            q = std::numeric_limits<double>::quiet_NaN();
        }

        if (this->SaveCellQuality)
        {
          if (this->CompatibilityMode && this->Volume)
          {
            double t[2] = { V, q };
            qualityArrays[qualityId]->SetTypedTuple(c, t);
          }
          else
          {
            qualityArrays[qualityId]->SetTypedTuple(c, &q);
          }
        }

        if (qualityId == 1)
        {
          break;
        }

        if (this->LinearApproximation)
        {
          LinearizeCell(cellType);
        }
      }
    }
    this->UpdateProgress(double(p + 1 + progressNumer) / progressDenom);
  }

  if (ntri)
  {
    Eqtri /= static_cast<double>(ntri);
    double multFactor = 1. / static_cast<double>(ntri > 1 ? ntri - 1 : ntri);
    Eqtri2 = multFactor * (Eqtri2 - static_cast<double>(ntri) * Eqtri * Eqtri);
  }
  else
  {
    qtrim = Eqtri = qtriM = Eqtri2 = 0.;
  }

  if (nqua)
  {
    Eqqua /= static_cast<double>(nqua);
    double multFactor = 1. / static_cast<double>(nqua > 1 ? nqua - 1 : nqua);
    Eqqua2 = multFactor * (Eqqua2 - static_cast<double>(nqua) * Eqqua * Eqqua);
  }
  else
  {
    qquam = Eqqua = qquaM = Eqqua2 = 0.;
  }

  if (ntet)
  {
    Eqtet /= static_cast<double>(ntet);
    double multFactor = 1. / static_cast<double>(ntet > 1 ? ntet - 1 : ntet);
    Eqtet2 = multFactor * (Eqtet2 - static_cast<double>(ntet) * Eqtet * Eqtet);
  }
  else
  {
    qtetm = Eqtet = qtetM = Eqtet2 = 0.;
  }

  if (nhex)
  {
    Eqhex /= static_cast<double>(nhex);
    double multFactor = 1. / static_cast<double>(nhex > 1 ? nhex - 1 : nhex);
    Eqhex2 = multFactor * (Eqhex2 - static_cast<double>(nhex) * Eqhex * Eqhex);
  }
  else
  {
    qhexm = Eqhex = qhexM = Eqhex2 = 0.;
  }

  double tuple[5];
  quality = vtkDoubleArray::New();
  quality->SetName("Mesh Triangle Quality");
  quality->SetNumberOfComponents(5);
  tuple[0] = qtrim;
  tuple[1] = Eqtri;
  tuple[2] = qtriM;
  tuple[3] = Eqtri2;
  tuple[4] = ntri;
  quality->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(quality);
  quality->Delete();

  quality = vtkDoubleArray::New();
  quality->SetName("Mesh Quadrilateral Quality");
  quality->SetNumberOfComponents(5);
  tuple[0] = qquam;
  tuple[1] = Eqqua;
  tuple[2] = qquaM;
  tuple[3] = Eqqua2;
  tuple[4] = nqua;
  quality->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(quality);
  quality->Delete();

  quality = vtkDoubleArray::New();
  quality->SetName("Mesh Tetrahedron Quality");
  quality->SetNumberOfComponents(5);
  tuple[0] = qtetm;
  tuple[1] = Eqtet;
  tuple[2] = qtetM;
  tuple[3] = Eqtet2;
  tuple[4] = ntet;
  quality->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(quality);
  quality->Delete();

  quality = vtkDoubleArray::New();
  quality->SetName("Mesh Hexahedron Quality");
  quality->SetNumberOfComponents(5);
  tuple[0] = qhexm;
  tuple[1] = Eqhex;
  tuple[2] = qhexM;
  tuple[3] = Eqhex2;
  tuple[4] = nhex;
  quality->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(quality);
  quality->Delete();

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
double vtkMeshQuality::QuadMaxEdgeRatios(vtkCell* cell)
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
