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
  os << indent << "PyramidQualityMeasure: " << QualityMeasureNames[this->PyramidQualityMeasure]
     << endl;
  os << indent << "WedgeQualityMeasure: " << QualityMeasureNames[this->WedgeQualityMeasure] << endl;
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
  this->PyramidQualityMeasure = vtkMeshQuality::SHAPE;
  this->WedgeQualityMeasure = vtkMeshQuality::EDGE_RATIO;
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

  CellQualityType TriangleQuality, QuadQuality, TetQuality, PyramidQuality, WedgeQuality,
    HexQuality;
  vtkSmartPointer<vtkDoubleArray> qualityArray = nullptr;
  vtkSmartPointer<vtkDoubleArray> approxQualityArray = nullptr;
  vtkSmartPointer<vtkDoubleArray> volumeArray = nullptr;
  vtkIdType numberOfCells = in->GetNumberOfCells();
  double qTrim, qTriM, eQTri, eQTri2;
  double qQuam, qQuaM, eQQua, eQQua2;
  double qTetm, qTetM, eQTet, eQTet2;
  double qPyrm, qPyrM, eQPyr, eQPyr2;
  double qWedgem, qWedgeM, eQWedge, eQWedge2;
  double qHexm, qHexM, eQHex, eQHex2;
  double q;
  double V = 0.;
  vtkIdType nTri = 0;
  vtkIdType nQua = 0;
  vtkIdType nTet = 0;
  vtkIdType nPyr = 0;
  vtkIdType nWedge = 0;
  vtkIdType nHex = 0;
  vtkCell* cell;
  int progressNumer = 0;
  double progressDenom = 20.;

  // Initialize the min and max values, std deviations, etc.
  qTriM = qQuaM = qTetM = qPyrM = qWedgeM = qHexM = VTK_DOUBLE_MIN;
  qTrim = qQuam = qTetm = qPyrm = qWedgem = qHexm = VTK_DOUBLE_MAX;
  eQTri = eQTri2 = eQQua = eQQua2 = eQTet = eQTet2 = eQPyr = eQPyr2 = eQWedge = eQWedge2 = eQHex =
    eQHex2 = 0.;

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
    case vtkMeshQuality::EQUIANGLE_SKEW:
      TriangleQuality = TriangleEquiangleSkew;
      break;
    case vtkMeshQuality::NORMALIZED_INRADIUS:
      TriangleQuality = TriangleNormalizedInradius;
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
      QuadQuality = QuadMaxEdgeRatio;
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
    case vtkMeshQuality::EQUIANGLE_SKEW:
      QuadQuality = QuadEquiangleSkew;
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
    case vtkMeshQuality::EQUIANGLE_SKEW:
      TetQuality = TetEquiangleSkew;
      break;
    case vtkMeshQuality::EQUIVOLUME_SKEW:
      TetQuality = TetEquivolumeSkew;
      break;
    case vtkMeshQuality::MEAN_RATIO:
      TetQuality = TetMeanRatio;
      break;
    case vtkMeshQuality::NORMALIZED_INRADIUS:
      TetQuality = TetNormalizedInradius;
      break;
    case vtkMeshQuality::SQUISH_INDEX:
      TetQuality = TetSquishIndex;
      break;
    default:
      vtkWarningMacro("Bad TetQualityMeasure (" << this->GetTetQualityMeasure()
                                                << "), using RadiusRatio instead");
      TetQuality = TetRadiusRatio;
      break;
  }

  switch (this->GetPyramidQualityMeasure())
  {
    case vtkMeshQuality::EQUIANGLE_SKEW:
      PyramidQuality = PyramidEquiangleSkew;
      break;
    case vtkMeshQuality::JACOBIAN:
      PyramidQuality = PyramidJacobian;
      break;
    case vtkMeshQuality::SCALED_JACOBIAN:
      PyramidQuality = PyramidScaledJacobian;
      break;
    case vtkMeshQuality::SHAPE:
      PyramidQuality = PyramidShape;
      break;
    case vtkMeshQuality::VOLUME:
      PyramidQuality = PyramidVolume;
      break;
    default:
      vtkWarningMacro("Bad PyramidQualityMeasure (" << this->GetPyramidQualityMeasure()
                                                    << "), using Shape instead");
      PyramidQuality = PyramidShape;
      break;
  }

  switch (this->WedgeQualityMeasure)
  {
    case vtkMeshQuality::CONDITION:
      WedgeQuality = WedgeCondition;
      break;
    case vtkMeshQuality::DISTORTION:
      WedgeQuality = WedgeDistortion;
      break;
    case vtkMeshQuality::EDGE_RATIO:
      WedgeQuality = WedgeEdgeRatio;
      break;
    case vtkMeshQuality::EQUIANGLE_SKEW:
      WedgeQuality = WedgeEquiangleSkew;
      break;
    case vtkMeshQuality::JACOBIAN:
      WedgeQuality = WedgeJacobian;
      break;
    case vtkMeshQuality::MAX_ASPECT_FROBENIUS:
      WedgeQuality = WedgeMaxAspectFrobenius;
      break;
    case vtkMeshQuality::MAX_STRETCH:
      WedgeQuality = WedgeMaxStretch;
      break;
    case vtkMeshQuality::MEAN_ASPECT_FROBENIUS:
      WedgeQuality = WedgeMeanAspectFrobenius;
      break;
    case vtkMeshQuality::SCALED_JACOBIAN:
      WedgeQuality = WedgeScaledJacobian;
      break;
    case vtkMeshQuality::SHAPE:
      WedgeQuality = WedgeShape;
      break;
    case vtkMeshQuality::VOLUME:
      WedgeQuality = WedgeVolume;
      break;
    default:
      vtkWarningMacro("Bad WedgeQualityMeasure (" << this->GetWedgeQualityMeasure()
                                                  << "), using EdgeRatio instead");
      WedgeQuality = WedgeEdgeRatio;
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
    case vtkMeshQuality::EQUIANGLE_SKEW:
      HexQuality = HexEquiangleSkew;
      break;
    case vtkMeshQuality::NODAL_JACOBIAN_RATIO:
      HexQuality = HexNodalJacobianRatio;
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
      for (int i = 0; i < 5; ++i)
      {
        triAreaTuple[i] = 0;
        quadAreaTuple[i] = 0;
        tetVolTuple[i] = 0;
        pyrVolTuple[i] = 0;
        wedgeVolTuple[i] = 0;
        hexVolTuple[i] = 0;
      }
      for (vtkIdType c = 0; c < numberOfCells; ++c)
      {
        double area, volume; // area and volume
        cell = out->GetCell(c);

        int cellType = cell->GetCellType();
        LinearizeCell(cellType);

        switch (cellType)
        {
          case VTK_TRIANGLE:
            area = TriangleArea(cell);
            if (area > triAreaTuple[2])
            {
              if (triAreaTuple[0] == triAreaTuple[2])
              { // min == max => min has not been set
                triAreaTuple[0] = area;
              }
              triAreaTuple[2] = area;
            }
            else if (area < triAreaTuple[0])
            {
              triAreaTuple[0] = area;
            }
            triAreaTuple[1] += area;
            triAreaTuple[3] += area * area;
            nTri++;
            break;
          case VTK_QUAD:
            area = QuadArea(cell);
            if (area > quadAreaTuple[2])
            {
              if (quadAreaTuple[0] == quadAreaTuple[2])
              { // min == max => min has not been set
                quadAreaTuple[0] = area;
              }
              quadAreaTuple[2] = area;
            }
            else if (area < quadAreaTuple[0])
            {
              quadAreaTuple[0] = area;
            }
            quadAreaTuple[1] += area;
            quadAreaTuple[3] += area * area;
            nQua++;
            break;
          case VTK_TETRA:
            volume = TetVolume(cell);
            if (volume > tetVolTuple[2])
            {
              if (tetVolTuple[0] == tetVolTuple[2])
              { // min == max => min has not been set
                tetVolTuple[0] = volume;
              }
              tetVolTuple[2] = volume;
            }
            else if (volume < tetVolTuple[0])
            {
              tetVolTuple[0] = volume;
            }
            tetVolTuple[1] += volume;
            tetVolTuple[3] += volume * volume;
            nTet++;
            break;
          case VTK_PYRAMID:
            volume = PyramidVolume(cell);
            if (volume > pyrVolTuple[2])
            {
              if (pyrVolTuple[0] == pyrVolTuple[2])
              { // min == max => min has not been set
                pyrVolTuple[0] = volume;
              }
              pyrVolTuple[2] = volume;
            }
            else if (volume < pyrVolTuple[0])
            {
              pyrVolTuple[0] = volume;
            }
            pyrVolTuple[1] += volume;
            pyrVolTuple[3] += volume * volume;
            nPyr++;
            break;
          case VTK_WEDGE:
            volume = WedgeVolume(cell);
            if (volume > wedgeVolTuple[2])
            {
              if (wedgeVolTuple[0] == wedgeVolTuple[2])
              { // min == max => min has not been set
                wedgeVolTuple[0] = volume;
              }
              wedgeVolTuple[2] = volume;
            }
            else if (volume < wedgeVolTuple[0])
            {
              wedgeVolTuple[0] = volume;
            }
            wedgeVolTuple[1] += volume;
            wedgeVolTuple[3] += volume * volume;
            nWedge++;
            break;
          case VTK_HEXAHEDRON:
            volume = HexVolume(cell);
            if (volume > hexVolTuple[2])
            {
              if (hexVolTuple[0] == hexVolTuple[2])
              { // min == max => min has not been set
                hexVolTuple[0] = volume;
              }
              hexVolTuple[2] = volume;
            }
            else if (volume < hexVolTuple[0])
            {
              hexVolTuple[0] = volume;
            }
            hexVolTuple[1] += volume;
            hexVolTuple[3] += volume * volume;
            nHex++;
            break;
          default:
            break;
        }
      }
      triAreaTuple[4] = nTri;
      quadAreaTuple[4] = nQua;
      tetVolTuple[4] = nTet;
      pyrVolTuple[4] = nPyr;
      wedgeVolTuple[4] = nWedge;
      hexVolTuple[4] = nHex;
      vtkMeshQuality::TriangleAverageSize = triAreaTuple[1] / triAreaTuple[4];
      vtkMeshQuality::QuadAverageSize = quadAreaTuple[1] / quadAreaTuple[4];
      vtkMeshQuality::TetAverageSize = tetVolTuple[1] / tetVolTuple[4];
      vtkMeshQuality::PyramidAverageSize = pyrVolTuple[1] / pyrVolTuple[4];
      vtkMeshQuality::WedgeAverageSize = wedgeVolTuple[1] / wedgeVolTuple[4];
      vtkMeshQuality::HexAverageSize = hexVolTuple[1] / hexVolTuple[4];
      progressNumer = 20;
      progressDenom = 40.;
      nTri = 0;
      nQua = 0;
      nTet = 0;
      nPyr = 0;
      nWedge = 0;
      nHex = 0;

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

  int p;
  vtkIdType c = 0;
  vtkIdType sz = numberOfCells / 20 + 1;
  vtkIdType inner;
  this->UpdateProgress(progressNumer / progressDenom + 0.01);
  for (p = 0; p < 20; ++p)
  {
    for (inner = 0; (inner < sz && c < numberOfCells); ++c, ++inner)
    {
      cell = out->GetCell(c);
      V = 0.;

      int numberOfOutputQualities = this->LinearApproximation ? 2 : 1;
      vtkSmartPointer<vtkDoubleArray> qualityArrays[2] = { qualityArray, approxQualityArray };

      int cellType = cell->GetCellType();

      for (int qualityId = 0; qualityId < numberOfOutputQualities; ++qualityId)
      {
        switch (cellType)
        {
          case VTK_TRIANGLE:
            q = TriangleQuality(cell);
            if (q > qTriM)
            {
              if (qTrim > qTriM)
              {
                qTrim = q;
              }
              qTriM = q;
            }
            else if (q < qTrim)
            {
              qTrim = q;
            }
            eQTri += q;
            eQTri2 += q * q;
            ++nTri;
            break;
          case VTK_QUAD:
            q = QuadQuality(cell);
            if (q > qQuaM)
            {
              if (qQuam > qQuaM)
              {
                qQuam = q;
              }
              qQuaM = q;
            }
            else if (q < qQuam)
            {
              qQuam = q;
            }
            eQQua += q;
            eQQua2 += q * q;
            ++nQua;
            break;
          case VTK_TETRA:
            q = TetQuality(cell);
            if (q > qTetM)
            {
              if (qTetm > qTetM)
              {
                qTetm = q;
              }
              qTetM = q;
            }
            else if (q < qTetm)
            {
              qTetm = q;
            }
            eQTet += q;
            eQTet2 += q * q;
            ++nTet;
            if (this->Volume)
            {
              V = TetVolume(cell);
              if (!this->CompatibilityMode)
              {
                volumeArray->SetTuple1(0, V);
              }
            }
            break;
          case VTK_PYRAMID:
            q = PyramidQuality(cell);
            if (q > qPyrM)
            {
              if (qPyrm > qPyrM)
              {
                qPyrm = q;
              }
              qPyrM = q;
            }
            else if (q < qPyrm)
            {
              qPyrm = q;
            }
            eQPyr += q;
            eQPyr2 += q * q;
            ++nPyr;
            break;
          case VTK_WEDGE:
            q = WedgeQuality(cell);
            if (q > qWedgeM)
            {
              if (qWedgem > qWedgeM)
              {
                qWedgem = q;
              }
              qWedgeM = q;
            }
            else if (q < qWedgem)
            {
              qWedgem = q;
            }
            eQWedge += q;
            eQWedge2 += q * q;
            ++nWedge;
            break;
          case VTK_HEXAHEDRON:
            q = HexQuality(cell);
            if (q > qHexM)
            {
              if (qHexm > qHexM)
              {
                qHexm = q;
              }
              qHexM = q;
            }
            else if (q < qHexm)
            {
              qHexm = q;
            }
            eQHex += q;
            eQHex2 += q * q;
            ++nHex;
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

  if (nTri)
  {
    eQTri /= static_cast<double>(nTri);
    double multFactor = 1. / static_cast<double>(nTri > 1 ? nTri - 1 : nTri);
    eQTri2 = multFactor * (eQTri2 - static_cast<double>(nTri) * eQTri * eQTri);
  }
  else
  {
    qTrim = eQTri = qTriM = eQTri2 = 0.;
  }

  if (nQua)
  {
    eQQua /= static_cast<double>(nQua);
    double multFactor = 1. / static_cast<double>(nQua > 1 ? nQua - 1 : nQua);
    eQQua2 = multFactor * (eQQua2 - static_cast<double>(nQua) * eQQua * eQQua);
  }
  else
  {
    qQuam = eQQua = qQuaM = eQQua2 = 0.;
  }

  if (nTet)
  {
    eQTet /= static_cast<double>(nTet);
    double multFactor = 1. / static_cast<double>(nTet > 1 ? nTet - 1 : nTet);
    eQTet2 = multFactor * (eQTet2 - static_cast<double>(nTet) * eQTet * eQTet);
  }
  else
  {
    qTetm = eQTet = qTetM = eQTet2 = 0.;
  }

  if (nPyr)
  {
    eQPyr /= static_cast<double>(nPyr);
    double multFactor = 1. / static_cast<double>(nPyr > 1 ? nPyr - 1 : nPyr);
    eQPyr2 = multFactor * (eQPyr2 - static_cast<double>(nPyr) * eQPyr * eQPyr);
  }
  else
  {
    qPyrm = eQPyr = qPyrM = eQPyr2 = 0.;
  }

  if (nWedge)
  {
    eQWedge /= static_cast<double>(nWedge);
    double multFactor = 1. / static_cast<double>(nWedge > 1 ? nWedge - 1 : nWedge);
    eQWedge2 = multFactor * (eQWedge2 - static_cast<double>(nWedge) * eQWedge * eQWedge);
  }
  else
  {
    qWedgem = eQWedge = qWedgeM = eQWedge2 = 0.;
  }

  if (nHex)
  {
    eQHex /= static_cast<double>(nHex);
    double multFactor = 1. / static_cast<double>(nHex > 1 ? nHex - 1 : nHex);
    eQHex2 = multFactor * (eQHex2 - static_cast<double>(nHex) * eQHex * eQHex);
  }
  else
  {
    qHexm = eQHex = qHexM = eQHex2 = 0.;
  }

  double tuple[5];
  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Triangle Quality");
  qualityArray->SetNumberOfComponents(5);
  tuple[0] = qTrim;
  tuple[1] = eQTri;
  tuple[2] = qTriM;
  tuple[3] = eQTri2;
  tuple[4] = nTri;
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Quadrilateral Quality");
  qualityArray->SetNumberOfComponents(5);
  tuple[0] = qQuam;
  tuple[1] = eQQua;
  tuple[2] = qQuaM;
  tuple[3] = eQQua2;
  tuple[4] = nQua;
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Tetrahedron Quality");
  qualityArray->SetNumberOfComponents(5);
  tuple[0] = qTetm;
  tuple[1] = eQTet;
  tuple[2] = qTetM;
  tuple[3] = eQTet2;
  tuple[4] = nTet;
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Pyramid Quality");
  qualityArray->SetNumberOfComponents(5);
  tuple[0] = qPyrm;
  tuple[1] = eQPyr;
  tuple[2] = qPyrM;
  tuple[3] = eQPyr2;
  tuple[4] = nPyr;
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Wedge Quality");
  qualityArray->SetNumberOfComponents(5);
  tuple[0] = qWedgem;
  tuple[1] = eQWedge;
  tuple[2] = qWedgeM;
  tuple[3] = eQWedge2;
  tuple[4] = nWedge;
  qualityArray->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(qualityArray);

  qualityArray = vtkSmartPointer<vtkDoubleArray>::New();
  qualityArray->SetName("Mesh Hexahedron Quality");
  qualityArray->SetNumberOfComponents(5);
  tuple[0] = qHexm;
  tuple[1] = eQHex;
  tuple[2] = qHexM;
  tuple[3] = eQHex2;
  tuple[4] = nHex;
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
