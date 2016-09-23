/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkMeshQuality.h"

#include "vtkDebugLeaks.h"
#include "vtkSmartPointer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkFieldData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"

static int DumpQualityStats( vtkMeshQuality* iq, const char *arrayname )
{
  cout << "  cardinality: "
       << iq->GetOutput()->GetFieldData()->GetArray( arrayname )->GetComponent( 0, 4 )
       << "  , range: "
       << iq->GetOutput()->GetFieldData()->GetArray( arrayname )->GetComponent( 0, 0 )
       << "  -  "
       << iq->GetOutput()->GetFieldData()->GetArray( arrayname )->GetComponent( 0, 2 )
       << endl;

  cout << "  average: " << iq->GetOutput()->GetFieldData()->GetArray( arrayname )->GetComponent( 0, 1 )
       << "  , standard deviation: "
       << sqrt(fabs(iq->GetOutput()->GetFieldData()->GetArray( arrayname )->GetComponent( 0, 3 )))
       << endl;

  return 0;
}

int MeshQuality( int argc, char* argv[] )
{
  vtkUnstructuredGridReader* mr = vtkUnstructuredGridReader::New();
  vtkUnstructuredGrid* ug;
  vtkMeshQuality* iq = vtkMeshQuality::New();
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/uGridEx.vtk");

  mr->SetFileName( fname );
  mr->Update();

  ug = mr->GetOutput();
  iq->SetInputConnection( mr->GetOutputPort() );
  iq->SaveCellQualityOn();
  cout << "SaveCellQuality: " << iq->GetSaveCellQuality() << endl;

  if ( ug->GetNumberOfCells() )
  {
    cout << endl;
    cout << "Triangle quality of mesh" << endl;
    cout << mr->GetFileName()
         << endl;

    iq->SetTriangleQualityMeasureToArea();
    iq->Update();
    cout << " Area:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToEdgeRatio();
    iq->Update();
    cout << " Edge Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToAspectRatio();
    iq->Update();
    cout << " Aspect Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToRadiusRatio();
    iq->Update();
    cout << " Radius Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToAspectFrobenius();
    iq->Update();
    cout << " Frobenius Norm:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToMinAngle();
    iq->Update();
    cout << " Minimal Angle:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToMaxAngle();
    iq->Update();
    cout << " Maximal Angle:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToCondition();
    iq->Update();
    cout << " Condition:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToScaledJacobian();
    iq->Update();
    cout << " ScaledJacobian:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToRelativeSizeSquared();
    iq->Update();
    cout << " RelativeSizeSquared:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToShape();
    iq->Update();
    cout << " Shape:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToShapeAndSize();
    iq->Update();
    cout << " ShapeAndSize:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    iq->SetTriangleQualityMeasureToDistortion();
    iq->Update();
    cout << " Distortion:"
         << endl;
    DumpQualityStats( iq, "Mesh Triangle Quality" );

    cout << endl;
    cout << "Quadrilatedral quality of mesh" << endl;
    cout << mr->GetFileName()
         << endl;

    iq->SetQuadQualityMeasureToEdgeRatio();
    iq->Update();
    cout << " Edge Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToAspectRatio();
    iq->Update();
    cout << " Aspect Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToRadiusRatio();
    iq->Update();
    cout << " Radius Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToMedAspectFrobenius();
    iq->Update();
    cout << " Average Frobenius Norm:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToMaxAspectFrobenius();
    iq->Update();
    cout << " Maximal Frobenius Norm:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToMaxEdgeRatios();
    iq->Update();
    cout << " Max Edge Ratios:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToSkew();
    iq->Update();
    cout << " Skew:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToTaper();
    iq->Update();
    cout << " Taper:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToWarpage();
    iq->Update();
    cout << " Warpage:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToArea();
    iq->Update();
    cout << " Area:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToStretch();
    iq->Update();
    cout << " Stretch:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToMinAngle();
    iq->Update();
    cout << " Min Angle:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToMaxAngle();
    iq->Update();
    cout << " Max Angle:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToOddy();
    iq->Update();
    cout << " Oddy:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToCondition();
    iq->Update();
    cout << " Condition:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToJacobian();
    iq->Update();
    cout << " Jacobian:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToScaledJacobian();
    iq->Update();
    cout << " Scaled Jacobian:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToShear();
    iq->Update();
    cout << " Shear:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToShape();
    iq->Update();
    cout << " Shape:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToRelativeSizeSquared();
    iq->Update();
    cout << " Relative Size Squared:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToShapeAndSize();
    iq->Update();
    cout << " Shape And Size:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToShearAndSize();
    iq->Update();
    cout << " Shear And Size:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    iq->SetQuadQualityMeasureToDistortion();
    iq->Update();
    cout << " Distortion:"
         << endl;
    DumpQualityStats( iq, "Mesh Quadrilateral Quality" );

    cout << endl;
    cout << "Tetrahedral quality of mesh" << endl;
    cout << mr->GetFileName()
         << endl;

    iq->SetTetQualityMeasureToEdgeRatio();
    iq->Update();
    cout << " Edge Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );

    iq->SetTetQualityMeasureToAspectRatio();
    iq->Update();
    cout << " Aspect Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );

    iq->SetTetQualityMeasureToRadiusRatio();
    iq->Update();
    cout << " Radius Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );

    iq->SetTetQualityMeasureToAspectFrobenius();
    iq->Update();
    cout << " Frobenius Norm:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );

    iq->SetTetQualityMeasureToMinAngle();
    iq->Update();
    cout << " Minimal Dihedral Angle:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );

    iq->SetTetQualityMeasureToCollapseRatio();
    iq->Update();
    cout << " Collapse Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );
    cout << endl;

    iq->SetTetQualityMeasureToAspectBeta();
    iq->Update();
    cout << " Aspect Beta:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );
    cout << endl;

    iq->SetTetQualityMeasureToAspectGamma();
    iq->Update();
    cout << " Aspect Gamma:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );
    cout << endl;

    iq->SetTetQualityMeasureToVolume();
    iq->Update();
    cout << " Volume:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );
    cout << endl;

    iq->SetTetQualityMeasureToCondition();
    iq->Update();
    cout << " Condition:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );
    cout << endl;

    iq->SetTetQualityMeasureToJacobian();
    iq->Update();
    cout << " Jacobian:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );
    cout << endl;

    iq->SetTetQualityMeasureToScaledJacobian();
    iq->Update();
    cout << " Scaled Jacobian:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );
    cout << endl;

    iq->SetTetQualityMeasureToShape();
    iq->Update();
    cout << " Shape:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );
    cout << endl;

    iq->SetTetQualityMeasureToRelativeSizeSquared();
    iq->Update();
    cout << " Relative Size Squared:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );
    cout << endl;

    iq->SetTetQualityMeasureToShapeAndSize();
    iq->Update();
    cout << " Shape And Size:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );
    cout << endl;

    iq->SetTetQualityMeasureToDistortion();
    iq->Update();
    cout << " Distortion:"
         << endl;
    DumpQualityStats( iq, "Mesh Tetrahedron Quality" );
    cout << endl;

    cout << "Hexahedral quality of mesh" << endl;
    cout << mr->GetFileName()
         << endl;

    iq->SetHexQualityMeasureToEdgeRatio();
    iq->Update();
    cout << " Edge Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToMedAspectFrobenius();
    iq->Update();
    cout << " Med Aspect Frobenius:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToMaxAspectFrobenius();
    iq->Update();
    cout << " Max Aspect Frobenius:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToMaxEdgeRatios();
    iq->Update();
    cout << " Max Edge Ratios:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToSkew();
    iq->Update();
    cout << " Skew:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToTaper();
    iq->Update();
    cout << " Taper:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToVolume();
    iq->Update();
    cout << " Volume:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToStretch();
    iq->Update();
    cout << " Stretch:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToDiagonal();
    iq->Update();
    cout << " Diagonal:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToDimension();
    iq->Update();
    cout << " Dimension:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToOddy();
    iq->Update();
    cout << " Oddy:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToCondition();
    iq->Update();
    cout << " Condition:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToJacobian();
    iq->Update();
    cout << " Jacobian:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToScaledJacobian();
    iq->Update();
    cout << " Scaled Jacobian:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToShear();
    iq->Update();
    cout << " Shear:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToShape();
    iq->Update();
    cout << " Shape:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToRelativeSizeSquared();
    iq->Update();
    cout << " Relative Size Squared:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToShapeAndSize();
    iq->Update();
    cout << " Shape And Size:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToShearAndSize();
    iq->Update();
    cout << " Shear And Size:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;

    iq->SetHexQualityMeasureToDistortion();
    iq->Update();
    cout << " Distortion:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl;
  }

  // Exersize remaining methods for coverage
  iq->Print(cout);

  // Check for warnings
  vtkSmartPointer<vtkTest::ErrorObserver>  warningObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  iq->AddObserver(vtkCommand::WarningEvent, warningObserver);
  iq->SetTriangleQualityMeasure (100000);
  iq->Update();

  if (warningObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << warningObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected warning" << std::endl;
    return EXIT_FAILURE;
  }
  warningObserver->Clear();

  iq->SetQuadQualityMeasure (100000);
  iq->Update();

  if (warningObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << warningObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected warning" << std::endl;
    return EXIT_FAILURE;
  }
  warningObserver->Clear();

  iq->SetTetQualityMeasure (100000);
  iq->Update();

  if (warningObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << warningObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected warning" << std::endl;
    return EXIT_FAILURE;
  }
  warningObserver->Clear();

  iq->SetHexQualityMeasure (100000);
  iq->Update();

  if (warningObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << warningObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected warning" << std::endl;
    return EXIT_FAILURE;
  }
  warningObserver->Clear();

  iq->Delete();
  mr->Delete();
  delete [] fname;

  return 0;
}
