// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2003 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkMeshQuality.h"

#include "vtkCellData.h"
#include "vtkCellTypeSource.h"
#include "vtkDebugLeaks.h"
#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTestUtilities.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

namespace
{
//------------------------------------------------------------------------------
int DumpQualityStats(vtkMeshQuality* iq, const char* arrayname)
{
  cout << "  cardinality: "
       << iq->GetOutput()->GetFieldData()->GetArray(arrayname)->GetComponent(0, 4)
       << "  , range: " << iq->GetOutput()->GetFieldData()->GetArray(arrayname)->GetComponent(0, 0)
       << "  -  " << iq->GetOutput()->GetFieldData()->GetArray(arrayname)->GetComponent(0, 2)
       << endl;

  cout << "  average: " << iq->GetOutput()->GetFieldData()->GetArray(arrayname)->GetComponent(0, 1)
       << "  , standard deviation: "
       << sqrt(fabs(iq->GetOutput()->GetFieldData()->GetArray(arrayname)->GetComponent(0, 3)))
       << endl;

  return 0;
}

//------------------------------------------------------------------------------
bool TestNonLinearCells(int linearType, const int cellTypes[], int numberOfCellTypes)
{
  vtkNew<vtkCellTypeSource> ref, nonLinearCells;

  ref->SetBlocksDimensions(1, 1, 1);
  ref->SetCellType(linearType);

  vtkNew<vtkMeshQuality> refQuality, nonLinearQuality;

  refQuality->SetInputConnection(ref->GetOutputPort());
  refQuality->Update();

  auto refUG = vtkUnstructuredGrid::SafeDownCast(refQuality->GetOutputDataObject(0));
  vtkCellData* refCD = refUG->GetCellData();
  auto refQualityArray = vtkArrayDownCast<vtkDoubleArray>(refCD->GetAbstractArray("Quality"));

  nonLinearCells->SetBlocksDimensions(1, 1, 1);
  nonLinearQuality->SetInputConnection(nonLinearCells->GetOutputPort());
  nonLinearQuality->LinearApproximationOn();

  for (int id = 0; id < numberOfCellTypes; ++id)
  {
    nonLinearCells->SetCellType(cellTypes[id]);
    nonLinearQuality->Update();

    auto outUG = vtkUnstructuredGrid::SafeDownCast(nonLinearQuality->GetOutputDataObject(0));
    vtkCellData* outCD = outUG->GetCellData();

    auto NaNQuality = vtkArrayDownCast<vtkDoubleArray>(outCD->GetAbstractArray("Quality"));
    auto approxQuality =
      vtkArrayDownCast<vtkDoubleArray>(outCD->GetAbstractArray("Quality (Linear Approx)"));

    for (vtkIdType cellId = 0; cellId < NaNQuality->GetNumberOfValues(); ++cellId)
    {
      if (NaNQuality->GetValue(cellId) == NaNQuality->GetValue(cellId))
      {
        vtkLog(ERROR, "Non linear cells should be tagged NaN");
        return false;
      }
      if (approxQuality->GetValue(cellId) != refQualityArray->GetValue(cellId))
      {
        vtkLog(ERROR, "Linear approximation failed for non linear cells");
        return false;
      }
    }
  }

  return true;
}
} // anonymous namespace

//------------------------------------------------------------------------------
int MeshQuality(int argc, char* argv[])
{
  vtkUnstructuredGridReader* mr = vtkUnstructuredGridReader::New();
  vtkUnstructuredGrid* ug;
  vtkMeshQuality* iq = vtkMeshQuality::New();
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/uGridEx.vtk");

  mr->SetFileName(fname);
  mr->Update();

  ug = mr->GetOutput();
  iq->SetInputConnection(mr->GetOutputPort());
  iq->SaveCellQualityOn();
  cout << "SaveCellQuality: " << iq->GetSaveCellQuality() << endl;

  if (ug->GetNumberOfCells())
  {
    cout << endl;
    cout << "Triangle quality of mesh" << endl;
    cout << mr->GetFileName() << endl;
    cout << endl;

    iq->SetTriangleQualityMeasureToArea();
    iq->Update();
    cout << " Area:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToEdgeRatio();
    iq->Update();
    cout << " Edge Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToAspectRatio();
    iq->Update();
    cout << " Aspect Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToRadiusRatio();
    iq->Update();
    cout << " Radius Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToAspectFrobenius();
    iq->Update();
    cout << " Frobenius Norm:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToMinAngle();
    iq->Update();
    cout << " Minimal Angle:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToMaxAngle();
    iq->Update();
    cout << " Maximal Angle:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToCondition();
    iq->Update();
    cout << " Condition:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToScaledJacobian();
    iq->Update();
    cout << " Scaled Jacobian:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToRelativeSizeSquared();
    iq->Update();
    cout << " Relative Size Squared:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToShape();
    iq->Update();
    cout << " Shape:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToShapeAndSize();
    iq->Update();
    cout << " Shape And Size:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToDistortion();
    iq->Update();
    cout << " Distortion:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToEquiangleSkew();
    iq->Update();
    cout << " Equiangle Skew:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    iq->SetTriangleQualityMeasureToNormalizedInradius();
    iq->Update();
    cout << " Normalized Inradius:" << endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    cout << endl;

    cout << endl;
    cout << "Quadrilatedral quality of mesh" << endl;
    cout << mr->GetFileName() << endl;

    iq->SetQuadQualityMeasureToEdgeRatio();
    iq->Update();
    cout << " Edge Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToAspectRatio();
    iq->Update();
    cout << " Aspect Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToRadiusRatio();
    iq->Update();
    cout << " Radius Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToMedAspectFrobenius();
    iq->Update();
    cout << " Average Frobenius Norm:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToMaxAspectFrobenius();
    iq->Update();
    cout << " Maximal Frobenius Norm:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToMaxEdgeRatio();
    iq->Update();
    cout << " Max Edge Ratios:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToSkew();
    iq->Update();
    cout << " Skew:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToTaper();
    iq->Update();
    cout << " Taper:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToWarpage();
    iq->Update();
    cout << " Warpage:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToArea();
    iq->Update();
    cout << " Area:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToStretch();
    iq->Update();
    cout << " Stretch:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToMinAngle();
    iq->Update();
    cout << " Min Angle:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToMaxAngle();
    iq->Update();
    cout << " Max Angle:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToOddy();
    iq->Update();
    cout << " Oddy:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToCondition();
    iq->Update();
    cout << " Condition:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToJacobian();
    iq->Update();
    cout << " Jacobian:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToScaledJacobian();
    iq->Update();
    cout << " Scaled Jacobian:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToShear();
    iq->Update();
    cout << " Shear:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToShape();
    iq->Update();
    cout << " Shape:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToRelativeSizeSquared();
    iq->Update();
    cout << " Relative Size Squared:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToShapeAndSize();
    iq->Update();
    cout << " Shape And Size:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToShearAndSize();
    iq->Update();
    cout << " Shear And Size:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToDistortion();
    iq->Update();
    cout << " Distortion:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    iq->SetQuadQualityMeasureToEquiangleSkew();
    iq->Update();
    cout << " EquiangleSkew:" << endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    cout << endl;

    cout << endl;
    cout << "Tetrahedral quality of mesh" << endl;
    cout << mr->GetFileName() << endl;

    iq->SetTetQualityMeasureToEdgeRatio();
    iq->Update();
    cout << " Edge Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToAspectRatio();
    iq->Update();
    cout << " Aspect Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToRadiusRatio();
    iq->Update();
    cout << " Radius Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToAspectFrobenius();
    iq->Update();
    cout << " Frobenius Norm:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToMinAngle();
    iq->Update();
    cout << " Minimal Dihedral Angle:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToCollapseRatio();
    iq->Update();
    cout << " Collapse Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToAspectGamma();
    iq->Update();
    cout << " Aspect Gamma:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToVolume();
    iq->Update();
    cout << " Volume:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToCondition();
    iq->Update();
    cout << " Condition:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToJacobian();
    iq->Update();
    cout << " Jacobian:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToScaledJacobian();
    iq->Update();
    cout << " Scaled Jacobian:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToShape();
    iq->Update();
    cout << " Shape:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToRelativeSizeSquared();
    iq->Update();
    cout << " Relative Size Squared:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToShapeAndSize();
    iq->Update();
    cout << " Shape And Size:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToDistortion();
    iq->Update();
    cout << " Distortion:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToEquiangleSkew();
    iq->Update();
    cout << " Equiangle Skew:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToEquivolumeSkew();
    iq->Update();
    cout << " Equivolume Skew:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToMeanRatio();
    iq->Update();
    cout << " Mean Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToNormalizedInradius();
    iq->Update();
    cout << " Normalized Inradius:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    iq->SetTetQualityMeasureToSquishIndex();
    iq->Update();
    cout << " Squish Index:" << endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    cout << endl;

    cout << "Pyramid quality of mesh" << endl;
    cout << mr->GetFileName() << endl;

    iq->SetPyramidQualityMeasureToEquiangleSkew();
    iq->Update();
    cout << " Equiangle Skew:" << endl;
    DumpQualityStats(iq, "Mesh Pyramid Quality");
    cout << endl;

    iq->SetPyramidQualityMeasureToJacobian();
    iq->Update();
    cout << " Jacobian:" << endl;
    DumpQualityStats(iq, "Mesh Pyramid Quality");
    cout << endl;

    iq->SetPyramidQualityMeasureToScaledJacobian();
    iq->Update();
    cout << " Scaled Jacobian:" << endl;
    DumpQualityStats(iq, "Mesh Pyramid Quality");
    cout << endl;

    iq->SetPyramidQualityMeasureToShape();
    iq->Update();
    cout << " Shape:" << endl;
    DumpQualityStats(iq, "Mesh Pyramid Quality");
    cout << endl;

    iq->SetPyramidQualityMeasureToVolume();
    iq->Update();
    cout << " Volume:" << endl;
    DumpQualityStats(iq, "Mesh Pyramid Quality");
    cout << endl;

    cout << "Wedge quality of mesh" << endl;
    cout << mr->GetFileName() << endl;

    iq->SetWedgeQualityMeasureToCondition();
    iq->Update();
    cout << " Condition:" << endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    cout << endl;

    iq->SetWedgeQualityMeasureToDistortion();
    iq->Update();
    cout << " Distortion:" << endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    cout << endl;

    iq->SetWedgeQualityMeasureToEdgeRatio();
    iq->Update();
    cout << " Edge Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    cout << endl;

    iq->SetWedgeQualityMeasureToEquiangleSkew();
    iq->Update();
    cout << " Equiangle Skew:" << endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    cout << endl;

    iq->SetWedgeQualityMeasureToJacobian();
    iq->Update();
    cout << " Jacobian:" << endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    cout << endl;

    iq->SetWedgeQualityMeasureToMaxAspectFrobenius();
    iq->Update();
    cout << " Max Aspect Frobenius:" << endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    cout << endl;

    iq->SetWedgeQualityMeasureToMaxStretch();
    iq->Update();
    cout << " Max Stretch:" << endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    cout << endl;

    iq->SetWedgeQualityMeasureToMeanAspectFrobenius();
    iq->Update();
    cout << " Mean Aspect Frobenius:" << endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    cout << endl;

    iq->SetWedgeQualityMeasureToScaledJacobian();
    iq->Update();
    cout << " Scaled Jacobian:" << endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    cout << endl;

    iq->SetWedgeQualityMeasureToShape();
    iq->Update();
    cout << " Shape:" << endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    cout << endl;

    iq->SetWedgeQualityMeasureToVolume();
    iq->Update();
    cout << " Volume:" << endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    cout << endl;

    cout << "Hexahedral quality of mesh" << endl;
    cout << mr->GetFileName() << endl;

    iq->SetHexQualityMeasureToEdgeRatio();
    iq->Update();
    cout << " Edge Ratio:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToMedAspectFrobenius();
    iq->Update();
    cout << " Med Aspect Frobenius:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToMaxAspectFrobenius();
    iq->Update();
    cout << " Max Aspect Frobenius:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToMaxEdgeRatio();
    iq->Update();
    cout << " Max Edge Ratios:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToSkew();
    iq->Update();
    cout << " Skew:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToTaper();
    iq->Update();
    cout << " Taper:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToVolume();
    iq->Update();
    cout << " Volume:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToStretch();
    iq->Update();
    cout << " Stretch:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToDiagonal();
    iq->Update();
    cout << " Diagonal:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToDimension();
    iq->Update();
    cout << " Dimension:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToOddy();
    iq->Update();
    cout << " Oddy:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToCondition();
    iq->Update();
    cout << " Condition:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToJacobian();
    iq->Update();
    cout << " Jacobian:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToScaledJacobian();
    iq->Update();
    cout << " Scaled Jacobian:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToShear();
    iq->Update();
    cout << " Shear:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToShape();
    iq->Update();
    cout << " Shape:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToRelativeSizeSquared();
    iq->Update();
    cout << " Relative Size Squared:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToShapeAndSize();
    iq->Update();
    cout << " Shape And Size:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToShearAndSize();
    iq->Update();
    cout << " Shear And Size:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;

    iq->SetHexQualityMeasureToDistortion();
    iq->Update();
    cout << " Distortion:" << endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    cout << endl;
  }

  constexpr int TriangleTypes[] = { VTK_QUADRATIC_TRIANGLE, VTK_BIQUADRATIC_TRIANGLE,
    VTK_HIGHER_ORDER_TRIANGLE, VTK_LAGRANGE_TRIANGLE, VTK_BEZIER_TRIANGLE };

  constexpr int QuadTypes[] = { VTK_QUADRATIC_QUAD, VTK_QUADRATIC_LINEAR_QUAD, VTK_BIQUADRATIC_QUAD,
    VTK_HIGHER_ORDER_QUAD, VTK_LAGRANGE_QUADRILATERAL, VTK_BEZIER_QUADRILATERAL };

  constexpr int TetraTypes[] = { VTK_QUADRATIC_TETRA, VTK_HIGHER_ORDER_TETRAHEDRON,
    VTK_LAGRANGE_TETRAHEDRON, VTK_BEZIER_TETRAHEDRON };

  constexpr int PyramidTypes[] = { VTK_QUADRATIC_PYRAMID, VTK_TRIQUADRATIC_PYRAMID,
    VTK_HIGHER_ORDER_PYRAMID, VTK_LAGRANGE_PYRAMID, VTK_BEZIER_PYRAMID };

  constexpr int WedgeTypes[] = { VTK_QUADRATIC_WEDGE, VTK_QUADRATIC_LINEAR_WEDGE,
    VTK_BIQUADRATIC_QUADRATIC_WEDGE, VTK_HIGHER_ORDER_WEDGE, VTK_LAGRANGE_WEDGE, VTK_BEZIER_WEDGE };

  constexpr int HexaTypes[] = { VTK_QUADRATIC_HEXAHEDRON, VTK_TRIQUADRATIC_HEXAHEDRON,
    VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, VTK_HIGHER_ORDER_HEXAHEDRON, VTK_LAGRANGE_HEXAHEDRON,
    VTK_BEZIER_HEXAHEDRON };

  vtkLog(INFO, "Testing non linear triangles");
  TestNonLinearCells(VTK_TRIANGLE, TriangleTypes, sizeof(TriangleTypes) / sizeof(int));
  vtkLog(INFO, "Testing non linear quads");
  TestNonLinearCells(VTK_QUAD, QuadTypes, sizeof(QuadTypes) / sizeof(int));
  vtkLog(INFO, "Testing non linear tetras");
  TestNonLinearCells(VTK_TETRA, TetraTypes, sizeof(TetraTypes) / sizeof(int));
  vtkLog(INFO, "Testing non linear pyramids");
  TestNonLinearCells(VTK_PYRAMID, PyramidTypes, sizeof(PyramidTypes) / sizeof(int));
  vtkLog(INFO, "Testing non linear wedges");
  TestNonLinearCells(VTK_WEDGE, WedgeTypes, sizeof(WedgeTypes) / sizeof(int));
  vtkLog(INFO, "Testing non linear hexahedrons");
  TestNonLinearCells(VTK_HEXAHEDRON, HexaTypes, sizeof(HexaTypes) / sizeof(int));

  // Exercise remaining methods for coverage
  iq->Print(cout);

  // Check for warnings
  auto warningObserver = vtkSmartPointer<vtkTest::ErrorObserver>::New();

  iq->AddObserver(vtkCommand::WarningEvent, warningObserver);
  iq->SetTriangleQualityMeasure(vtkMeshQuality::QualityMeasureTypes::NONE);
  iq->Update();

  if (warningObserver->GetWarning())
  {
    std::cout << "Caught expected warning: " << warningObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected warning" << std::endl;
    return EXIT_FAILURE;
  }
  warningObserver->Clear();

  iq->SetQuadQualityMeasure(vtkMeshQuality::QualityMeasureTypes::NONE);
  iq->Update();

  if (warningObserver->GetWarning())
  {
    std::cout << "Caught expected warning: " << warningObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected warning" << std::endl;
    return EXIT_FAILURE;
  }
  warningObserver->Clear();

  iq->SetTetQualityMeasure(vtkMeshQuality::QualityMeasureTypes::NONE);
  iq->Update();

  if (warningObserver->GetWarning())
  {
    std::cout << "Caught expected warning: " << warningObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected warning" << std::endl;
    return EXIT_FAILURE;
  }
  warningObserver->Clear();

  iq->SetPyramidQualityMeasure(vtkMeshQuality::QualityMeasureTypes::NONE);
  iq->Update();

  if (warningObserver->GetWarning())
  {
    std::cout << "Caught expected warning: " << warningObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected warning" << std::endl;
    return EXIT_FAILURE;
  }
  warningObserver->Clear();

  iq->SetWedgeQualityMeasure(vtkMeshQuality::QualityMeasureTypes::NONE);
  iq->Update();

  if (warningObserver->GetWarning())
  {
    std::cout << "Caught expected warning: " << warningObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected warning" << std::endl;
    return EXIT_FAILURE;
  }
  warningObserver->Clear();

  iq->SetHexQualityMeasure(vtkMeshQuality::QualityMeasureTypes::NONE);
  iq->Update();

  if (warningObserver->GetWarning())
  {
    std::cout << "Caught expected warning: " << warningObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected warning" << std::endl;
    return EXIT_FAILURE;
  }
  warningObserver->Clear();

  iq->Delete();
  mr->Delete();
  delete[] fname;

  return 0;
}
