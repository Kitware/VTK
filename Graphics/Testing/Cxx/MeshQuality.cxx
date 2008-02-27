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
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkFieldData.h"

int DumpQualityStats( vtkMeshQuality* iq, const char *arrayname )
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
  iq->SetInput( ug );

  if ( ug->GetNumberOfCells() )
    {
    cout << endl; 
    cout << "Triangle quality of mesh" << endl;
    cout << mr->GetFileName()
         << endl;

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
    
    iq->SetQuadQualityMeasureToMinAngle();
    iq->Update();
    cout << " Minimal Angle:"
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

    cout << "Hexahedral quality of mesh" << endl;
    cout << mr->GetFileName()
         << endl;
    
    iq->SetHexQualityMeasureToEdgeRatio();
    iq->Update();
    cout << " Edge Ratio:"
         << endl;
    DumpQualityStats( iq, "Mesh Hexahedron Quality" );
    cout << endl; 
    }

  iq->Delete();
  mr->Delete();
  delete [] fname;

  return 0;
}
