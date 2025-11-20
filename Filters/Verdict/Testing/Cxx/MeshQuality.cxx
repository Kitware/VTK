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
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTestUtilities.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

#include <iostream>

namespace
{
//------------------------------------------------------------------------------
int DumpQualityStats(vtkMeshQuality* iq, const char* arrayname)
{
  std::cout << "  cardinality: "
            << iq->GetOutput()->GetFieldData()->GetArray(arrayname)->GetComponent(0, 4)
            << "  , range: "
            << iq->GetOutput()->GetFieldData()->GetArray(arrayname)->GetComponent(0, 0) << "  -  "
            << iq->GetOutput()->GetFieldData()->GetArray(arrayname)->GetComponent(0, 2)
            << std::endl;

  std::cout << "  average: "
            << iq->GetOutput()->GetFieldData()->GetArray(arrayname)->GetComponent(0, 1)
            << "  , standard deviation: "
            << sqrt(fabs(iq->GetOutput()->GetFieldData()->GetArray(arrayname)->GetComponent(0, 3)))
            << std::endl;

  return 0;
}

//------------------------------------------------------------------------------
bool TestNonLinearCellsApprox(int linearType, const int cellTypes[], int numberOfCellTypes)
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
      if (!std::isnan(NaNQuality->GetValue(cellId)))
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

//------------------------------------------------------------------------------
bool TestNonLinearCells(int linearType, int nonLinearType,
  const vtkMeshQuality::QualityMeasureTypes metrics[], int numberOfMetrics)
{
  vtkNew<vtkCellTypeSource> ref, nonLinearCells;
  ref->SetBlocksDimensions(1, 1, 1);
  ref->SetCellType(linearType);
  ref->Update();
  nonLinearCells->SetBlocksDimensions(1, 1, 1);
  nonLinearCells->SetCellType(nonLinearType);
  nonLinearCells->Update();

  for (int metricId = 0; metricId < numberOfMetrics; ++metricId)
  {
    vtkNew<vtkMeshQuality> refQuality, nonLinearQuality;
    refQuality->SetInputConnection(ref->GetOutputPort());
    nonLinearQuality->SetInputConnection(nonLinearCells->GetOutputPort());
    switch (linearType)
    {
      case VTK_TRIANGLE:
        refQuality->SetTriangleQualityMeasure(metrics[metricId]);
        nonLinearQuality->SetTriangleQualityMeasure(metrics[metricId]);
        break;
      case VTK_QUAD:
        refQuality->SetQuadQualityMeasure(metrics[metricId]);
        nonLinearQuality->SetQuadQualityMeasure(metrics[metricId]);
        break;
      case VTK_TETRA:
        refQuality->SetTetQualityMeasure(metrics[metricId]);
        nonLinearQuality->SetTetQualityMeasure(metrics[metricId]);
        break;
      case VTK_PYRAMID:
        refQuality->SetPyramidQualityMeasure(metrics[metricId]);
        nonLinearQuality->SetPyramidQualityMeasure(metrics[metricId]);
        break;
      case VTK_WEDGE:
        refQuality->SetWedgeQualityMeasure(metrics[metricId]);
        nonLinearQuality->SetWedgeQualityMeasure(metrics[metricId]);
        break;
      case VTK_HEXAHEDRON:
        refQuality->SetHexQualityMeasure(metrics[metricId]);
        nonLinearQuality->SetHexQualityMeasure(metrics[metricId]);
        break;
      default:
        vtkLog(ERROR, "Unsupported cell type");
        return false;
    }
    refQuality->Update();
    nonLinearQuality->Update();

    auto refUG = vtkUnstructuredGrid::SafeDownCast(refQuality->GetOutputDataObject(0));
    auto refQualityArray =
      vtkArrayDownCast<vtkDoubleArray>(refUG->GetCellData()->GetAbstractArray("Quality"));
    auto nonLinearUG = vtkUnstructuredGrid::SafeDownCast(nonLinearQuality->GetOutputDataObject(0));
    auto nonLinearQualityArray =
      vtkArrayDownCast<vtkDoubleArray>(nonLinearUG->GetCellData()->GetAbstractArray("Quality"));

    for (vtkIdType cellId = 0; cellId < nonLinearQualityArray->GetNumberOfValues(); ++cellId)
    {
      if (std::isnan(nonLinearQualityArray->GetValue(cellId)) &&
        std::isnan(refQualityArray->GetValue(cellId)))
      {
        vtkLog(ERROR, "Non linear cells should not be nan");
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

  vtkNew<vtkPolyData> dummyPolyData;
  iq->SetInputData(dummyPolyData);
  iq->SetTriangleQualityMeasureToArea();
  iq->Update();

  ug = mr->GetOutput();
  iq->SetInputConnection(mr->GetOutputPort());
  iq->SaveCellQualityOn();
  std::cout << "SaveCellQuality: " << iq->GetSaveCellQuality() << std::endl;

  if (ug->GetNumberOfCells())
  {
    std::cout << std::endl;
    std::cout << "Triangle quality of mesh" << std::endl;
    std::cout << mr->GetFileName() << std::endl;
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToArea();
    iq->Update();
    std::cout << " Area:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToEdgeRatio();
    iq->Update();
    std::cout << " Edge Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToAspectRatio();
    iq->Update();
    std::cout << " Aspect Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToRadiusRatio();
    iq->Update();
    std::cout << " Radius Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToAspectFrobenius();
    iq->Update();
    std::cout << " Frobenius Norm:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToMinAngle();
    iq->Update();
    std::cout << " Minimal Angle:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToMaxAngle();
    iq->Update();
    std::cout << " Maximal Angle:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToCondition();
    iq->Update();
    std::cout << " Condition:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToScaledJacobian();
    iq->Update();
    std::cout << " Scaled Jacobian:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToRelativeSizeSquared();
    iq->Update();
    std::cout << " Relative Size Squared:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToShape();
    iq->Update();
    std::cout << " Shape:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToShapeAndSize();
    iq->Update();
    std::cout << " Shape And Size:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToDistortion();
    iq->Update();
    std::cout << " Distortion:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToEquiangleSkew();
    iq->Update();
    std::cout << " Equiangle Skew:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    iq->SetTriangleQualityMeasureToNormalizedInradius();
    iq->Update();
    std::cout << " Normalized Inradius:" << std::endl;
    DumpQualityStats(iq, "Mesh Triangle Quality");
    std::cout << std::endl;

    std::cout << std::endl;
    std::cout << "Quadrilatedral quality of mesh" << std::endl;
    std::cout << mr->GetFileName() << std::endl;

    iq->SetQuadQualityMeasureToEdgeRatio();
    iq->Update();
    std::cout << " Edge Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToAspectRatio();
    iq->Update();
    std::cout << " Aspect Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToRadiusRatio();
    iq->Update();
    std::cout << " Radius Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToMedAspectFrobenius();
    iq->Update();
    std::cout << " Average Frobenius Norm:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToMaxAspectFrobenius();
    iq->Update();
    std::cout << " Maximal Frobenius Norm:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToMaxEdgeRatio();
    iq->Update();
    std::cout << " Max Edge Ratios:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToSkew();
    iq->Update();
    std::cout << " Skew:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToTaper();
    iq->Update();
    std::cout << " Taper:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToWarpage();
    iq->Update();
    std::cout << " Warpage:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToArea();
    iq->Update();
    std::cout << " Area:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToStretch();
    iq->Update();
    std::cout << " Stretch:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToMinAngle();
    iq->Update();
    std::cout << " Min Angle:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToMaxAngle();
    iq->Update();
    std::cout << " Max Angle:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToOddy();
    iq->Update();
    std::cout << " Oddy:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToCondition();
    iq->Update();
    std::cout << " Condition:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToJacobian();
    iq->Update();
    std::cout << " Jacobian:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToScaledJacobian();
    iq->Update();
    std::cout << " Scaled Jacobian:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToShear();
    iq->Update();
    std::cout << " Shear:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToShape();
    iq->Update();
    std::cout << " Shape:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToRelativeSizeSquared();
    iq->Update();
    std::cout << " Relative Size Squared:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToShapeAndSize();
    iq->Update();
    std::cout << " Shape And Size:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToShearAndSize();
    iq->Update();
    std::cout << " Shear And Size:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToDistortion();
    iq->Update();
    std::cout << " Distortion:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    iq->SetQuadQualityMeasureToEquiangleSkew();
    iq->Update();
    std::cout << " EquiangleSkew:" << std::endl;
    DumpQualityStats(iq, "Mesh Quadrilateral Quality");
    std::cout << std::endl;

    std::cout << std::endl;
    std::cout << "Tetrahedral quality of mesh" << std::endl;
    std::cout << mr->GetFileName() << std::endl;

    iq->SetTetQualityMeasureToEdgeRatio();
    iq->Update();
    std::cout << " Edge Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToAspectRatio();
    iq->Update();
    std::cout << " Aspect Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToRadiusRatio();
    iq->Update();
    std::cout << " Radius Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToAspectFrobenius();
    iq->Update();
    std::cout << " Frobenius Norm:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToMinAngle();
    iq->Update();
    std::cout << " Minimal Dihedral Angle:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToCollapseRatio();
    iq->Update();
    std::cout << " Collapse Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToAspectGamma();
    iq->Update();
    std::cout << " Aspect Gamma:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToVolume();
    iq->Update();
    std::cout << " Volume:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToCondition();
    iq->Update();
    std::cout << " Condition:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToJacobian();
    iq->Update();
    std::cout << " Jacobian:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToScaledJacobian();
    iq->Update();
    std::cout << " Scaled Jacobian:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToShape();
    iq->Update();
    std::cout << " Shape:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToRelativeSizeSquared();
    iq->Update();
    std::cout << " Relative Size Squared:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToShapeAndSize();
    iq->Update();
    std::cout << " Shape And Size:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToDistortion();
    iq->Update();
    std::cout << " Distortion:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToEquiangleSkew();
    iq->Update();
    std::cout << " Equiangle Skew:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToEquivolumeSkew();
    iq->Update();
    std::cout << " Equivolume Skew:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToInradius();
    iq->Update();
    std::cout << " Inradius:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToMeanRatio();
    iq->Update();
    std::cout << " Mean Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToNormalizedInradius();
    iq->Update();
    std::cout << " Normalized Inradius:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    iq->SetTetQualityMeasureToSquishIndex();
    iq->Update();
    std::cout << " Squish Index:" << std::endl;
    DumpQualityStats(iq, "Mesh Tetrahedron Quality");
    std::cout << std::endl;

    std::cout << "Pyramid quality of mesh" << std::endl;
    std::cout << mr->GetFileName() << std::endl;

    iq->SetPyramidQualityMeasureToEquiangleSkew();
    iq->Update();
    std::cout << " Equiangle Skew:" << std::endl;
    DumpQualityStats(iq, "Mesh Pyramid Quality");
    std::cout << std::endl;

    iq->SetPyramidQualityMeasureToJacobian();
    iq->Update();
    std::cout << " Jacobian:" << std::endl;
    DumpQualityStats(iq, "Mesh Pyramid Quality");
    std::cout << std::endl;

    iq->SetPyramidQualityMeasureToScaledJacobian();
    iq->Update();
    std::cout << " Scaled Jacobian:" << std::endl;
    DumpQualityStats(iq, "Mesh Pyramid Quality");
    std::cout << std::endl;

    iq->SetPyramidQualityMeasureToShape();
    iq->Update();
    std::cout << " Shape:" << std::endl;
    DumpQualityStats(iq, "Mesh Pyramid Quality");
    std::cout << std::endl;

    iq->SetPyramidQualityMeasureToVolume();
    iq->Update();
    std::cout << " Volume:" << std::endl;
    DumpQualityStats(iq, "Mesh Pyramid Quality");
    std::cout << std::endl;

    std::cout << "Wedge quality of mesh" << std::endl;
    std::cout << mr->GetFileName() << std::endl;

    iq->SetWedgeQualityMeasureToCondition();
    iq->Update();
    std::cout << " Condition:" << std::endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    std::cout << std::endl;

    iq->SetWedgeQualityMeasureToDistortion();
    iq->Update();
    std::cout << " Distortion:" << std::endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    std::cout << std::endl;

    iq->SetWedgeQualityMeasureToEdgeRatio();
    iq->Update();
    std::cout << " Edge Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    std::cout << std::endl;

    iq->SetWedgeQualityMeasureToEquiangleSkew();
    iq->Update();
    std::cout << " Equiangle Skew:" << std::endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    std::cout << std::endl;

    iq->SetWedgeQualityMeasureToJacobian();
    iq->Update();
    std::cout << " Jacobian:" << std::endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    std::cout << std::endl;

    iq->SetWedgeQualityMeasureToMaxAspectFrobenius();
    iq->Update();
    std::cout << " Max Aspect Frobenius:" << std::endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    std::cout << std::endl;

    iq->SetWedgeQualityMeasureToMaxStretch();
    iq->Update();
    std::cout << " Max Stretch:" << std::endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    std::cout << std::endl;

    iq->SetWedgeQualityMeasureToMeanAspectFrobenius();
    iq->Update();
    std::cout << " Mean Aspect Frobenius:" << std::endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    std::cout << std::endl;

    iq->SetWedgeQualityMeasureToScaledJacobian();
    iq->Update();
    std::cout << " Scaled Jacobian:" << std::endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    std::cout << std::endl;

    iq->SetWedgeQualityMeasureToShape();
    iq->Update();
    std::cout << " Shape:" << std::endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    std::cout << std::endl;

    iq->SetWedgeQualityMeasureToVolume();
    iq->Update();
    std::cout << " Volume:" << std::endl;
    DumpQualityStats(iq, "Mesh Wedge Quality");
    std::cout << std::endl;

    std::cout << "Hexahedral quality of mesh" << std::endl;
    std::cout << mr->GetFileName() << std::endl;

    iq->SetHexQualityMeasureToEdgeRatio();
    iq->Update();
    std::cout << " Edge Ratio:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToMedAspectFrobenius();
    iq->Update();
    std::cout << " Med Aspect Frobenius:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToMaxAspectFrobenius();
    iq->Update();
    std::cout << " Max Aspect Frobenius:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToMaxEdgeRatio();
    iq->Update();
    std::cout << " Max Edge Ratios:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToSkew();
    iq->Update();
    std::cout << " Skew:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToTaper();
    iq->Update();
    std::cout << " Taper:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToVolume();
    iq->Update();
    std::cout << " Volume:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToStretch();
    iq->Update();
    std::cout << " Stretch:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToDiagonal();
    iq->Update();
    std::cout << " Diagonal:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToDimension();
    iq->Update();
    std::cout << " Dimension:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToOddy();
    iq->Update();
    std::cout << " Oddy:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToCondition();
    iq->Update();
    std::cout << " Condition:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToJacobian();
    iq->Update();
    std::cout << " Jacobian:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToScaledJacobian();
    iq->Update();
    std::cout << " Scaled Jacobian:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToShear();
    iq->Update();
    std::cout << " Shear:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToShape();
    iq->Update();
    std::cout << " Shape:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToRelativeSizeSquared();
    iq->Update();
    std::cout << " Relative Size Squared:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToShapeAndSize();
    iq->Update();
    std::cout << " Shape And Size:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToShearAndSize();
    iq->Update();
    std::cout << " Shear And Size:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;

    iq->SetHexQualityMeasureToDistortion();
    iq->Update();
    std::cout << " Distortion:" << std::endl;
    DumpQualityStats(iq, "Mesh Hexahedron Quality");
    std::cout << std::endl;
  }

  constexpr int TriangleTypes[] = { VTK_QUADRATIC_TRIANGLE, VTK_BIQUADRATIC_TRIANGLE,
    VTK_HIGHER_ORDER_TRIANGLE, VTK_LAGRANGE_TRIANGLE, VTK_BEZIER_TRIANGLE };
  constexpr vtkMeshQuality::QualityMeasureTypes QuadraticTriangleMetrics[] = {
    vtkMeshQuality::QualityMeasureTypes::AREA, vtkMeshQuality::QualityMeasureTypes::DISTORTION,
    vtkMeshQuality::QualityMeasureTypes::NORMALIZED_INRADIUS,
    vtkMeshQuality::QualityMeasureTypes::SCALED_JACOBIAN
  };
  constexpr vtkMeshQuality::QualityMeasureTypes BiQuadraticTriangleMetrics[] = {
    vtkMeshQuality::QualityMeasureTypes::AREA, vtkMeshQuality::QualityMeasureTypes::DISTORTION
  };

  constexpr int QuadTypes[] = { VTK_QUADRATIC_QUAD, VTK_QUADRATIC_LINEAR_QUAD, VTK_BIQUADRATIC_QUAD,
    VTK_HIGHER_ORDER_QUAD, VTK_LAGRANGE_QUADRILATERAL, VTK_BEZIER_QUADRILATERAL };
  constexpr vtkMeshQuality::QualityMeasureTypes QuadraticQuadMetrics[] = {
    vtkMeshQuality::QualityMeasureTypes::AREA, vtkMeshQuality::QualityMeasureTypes::DISTORTION
  };
  constexpr vtkMeshQuality::QualityMeasureTypes BiQuadraticQuadMetrics[] = {
    vtkMeshQuality::QualityMeasureTypes::AREA
  };

  constexpr int TetraTypes[] = { VTK_QUADRATIC_TETRA, VTK_HIGHER_ORDER_TETRAHEDRON,
    VTK_LAGRANGE_TETRAHEDRON, VTK_BEZIER_TETRAHEDRON };
  constexpr vtkMeshQuality::QualityMeasureTypes QuadraticTetraMetrics[] = {
    vtkMeshQuality::QualityMeasureTypes::DISTORTION,
    vtkMeshQuality::QualityMeasureTypes::EQUIVOLUME_SKEW,
    vtkMeshQuality::QualityMeasureTypes::INRADIUS, vtkMeshQuality::QualityMeasureTypes::JACOBIAN,
    vtkMeshQuality::QualityMeasureTypes::MEAN_RATIO,
    vtkMeshQuality::QualityMeasureTypes::NORMALIZED_INRADIUS,
    vtkMeshQuality::QualityMeasureTypes::SCALED_JACOBIAN,
    vtkMeshQuality::QualityMeasureTypes::VOLUME
  };

  constexpr int PyramidTypes[] = { VTK_QUADRATIC_PYRAMID, VTK_TRIQUADRATIC_PYRAMID,
    VTK_HIGHER_ORDER_PYRAMID, VTK_LAGRANGE_PYRAMID, VTK_BEZIER_PYRAMID };

  constexpr int WedgeTypes[] = { VTK_QUADRATIC_WEDGE, VTK_QUADRATIC_LINEAR_WEDGE,
    VTK_BIQUADRATIC_QUADRATIC_WEDGE, VTK_HIGHER_ORDER_WEDGE, VTK_LAGRANGE_WEDGE, VTK_BEZIER_WEDGE };

  constexpr int HexaTypes[] = { VTK_QUADRATIC_HEXAHEDRON, VTK_TRIQUADRATIC_HEXAHEDRON,
    VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, VTK_HIGHER_ORDER_HEXAHEDRON, VTK_LAGRANGE_HEXAHEDRON,
    VTK_BEZIER_HEXAHEDRON };
  constexpr vtkMeshQuality::QualityMeasureTypes QuadraticHexMetrics[] = {
    vtkMeshQuality::QualityMeasureTypes::DISTORTION, vtkMeshQuality::QualityMeasureTypes::VOLUME
  };
  constexpr vtkMeshQuality::QualityMeasureTypes TriQuadraticHexMetrics[] = {
    vtkMeshQuality::QualityMeasureTypes::DISTORTION, vtkMeshQuality::QualityMeasureTypes::JACOBIAN,
    vtkMeshQuality::QualityMeasureTypes::VOLUME
  };

  vtkLog(INFO, "Testing non linear triangles");
  TestNonLinearCellsApprox(VTK_TRIANGLE, TriangleTypes, sizeof(TriangleTypes) / sizeof(int));
  TestNonLinearCells(VTK_TRIANGLE, VTK_QUADRATIC_TRIANGLE, QuadraticTriangleMetrics,
    sizeof(QuadraticTriangleMetrics) / sizeof(vtkMeshQuality::QualityMeasureTypes));
  TestNonLinearCells(VTK_TRIANGLE, VTK_BIQUADRATIC_TRIANGLE, BiQuadraticTriangleMetrics,
    sizeof(BiQuadraticTriangleMetrics) / sizeof(vtkMeshQuality::QualityMeasureTypes));
  vtkLog(INFO, "Testing non linear quads");
  TestNonLinearCellsApprox(VTK_QUAD, QuadTypes, sizeof(QuadTypes) / sizeof(int));
  TestNonLinearCells(VTK_QUAD, VTK_QUADRATIC_QUAD, QuadraticQuadMetrics,
    sizeof(QuadraticQuadMetrics) / sizeof(vtkMeshQuality::QualityMeasureTypes));
  TestNonLinearCells(VTK_QUAD, VTK_BIQUADRATIC_QUAD, BiQuadraticQuadMetrics,
    sizeof(BiQuadraticQuadMetrics) / sizeof(vtkMeshQuality::QualityMeasureTypes));
  vtkLog(INFO, "Testing non linear tetras");
  TestNonLinearCellsApprox(VTK_TETRA, TetraTypes, sizeof(TetraTypes) / sizeof(int));
  TestNonLinearCells(VTK_TETRA, VTK_QUADRATIC_TETRA, QuadraticTetraMetrics,
    sizeof(QuadraticTetraMetrics) / sizeof(vtkMeshQuality::QualityMeasureTypes));
  vtkLog(INFO, "Testing non linear pyramids");
  TestNonLinearCellsApprox(VTK_PYRAMID, PyramidTypes, sizeof(PyramidTypes) / sizeof(int));
  vtkLog(INFO, "Testing non linear wedges");
  TestNonLinearCellsApprox(VTK_WEDGE, WedgeTypes, sizeof(WedgeTypes) / sizeof(int));
  vtkLog(INFO, "Testing non linear hexahedrons");
  TestNonLinearCellsApprox(VTK_HEXAHEDRON, HexaTypes, sizeof(HexaTypes) / sizeof(int));
  TestNonLinearCells(VTK_HEXAHEDRON, VTK_QUADRATIC_HEXAHEDRON, QuadraticHexMetrics,
    sizeof(QuadraticHexMetrics) / sizeof(vtkMeshQuality::QualityMeasureTypes));
  TestNonLinearCells(VTK_HEXAHEDRON, VTK_TRIQUADRATIC_HEXAHEDRON, TriQuadraticHexMetrics,
    sizeof(TriQuadraticHexMetrics) / sizeof(vtkMeshQuality::QualityMeasureTypes));

  // Exercise remaining methods for coverage
  iq->Print(std::cout);

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
