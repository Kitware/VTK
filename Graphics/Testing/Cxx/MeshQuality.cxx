/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkMeshQuality.h"

#include <math.h>

#include "vtkDebugLeaks.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkFieldData.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"

struct vtkMeshQualityTestRec
{
  vtkIdType cellId;
  int qualityMeasure;
  double qualityExpected;
  int significantDigits;
} tests[] = {
    {  0, VTK_QUALITY_RADIUS_RATIO, 1.00000,  5 },
    {  1, VTK_QUALITY_RADIUS_RATIO, 1.14383,  5 },
    {  2, VTK_QUALITY_RADIUS_RATIO, 1.74416,  5 },
    {  3, VTK_QUALITY_RADIUS_RATIO, 1.00000,  5 },
    {  4, VTK_QUALITY_RADIUS_RATIO, 1.07290,  5 },
    {  5, VTK_QUALITY_RADIUS_RATIO, 1.60536,  5 },
    {  6, VTK_QUALITY_RADIUS_RATIO, 1.00000,  5 },
    {  7, VTK_QUALITY_RADIUS_RATIO, 1.00000,  5 },
    {  8, VTK_QUALITY_RADIUS_RATIO, 1.00000,  5 },
    {  9, VTK_QUALITY_RADIUS_RATIO, 2.53125,  5 },
    { 10, VTK_QUALITY_RADIUS_RATIO, 2.21183,  5 },
    { 11, VTK_QUALITY_RADIUS_RATIO, 3.03199,  5 },

    {  0, VTK_QUALITY_ASPECT_RATIO, 1.     ,  5 },
    {  1, VTK_QUALITY_ASPECT_RATIO, 1.20621,  5 },
    {  2, VTK_QUALITY_ASPECT_RATIO, 2.06963,  5 },
    {  3, VTK_QUALITY_ASPECT_RATIO, 1.     ,  5 },
    {  4, VTK_QUALITY_ASPECT_RATIO, 1.21247,  5 },
    {  5, VTK_QUALITY_ASPECT_RATIO, 1.84043,  5 },
    {  6, VTK_QUALITY_ASPECT_RATIO, 1.     ,  5 },
    {  7, VTK_QUALITY_ASPECT_RATIO, 1.     ,  5 },
    {  8, VTK_QUALITY_ASPECT_RATIO, 1.     ,  5 },
    {  9, VTK_QUALITY_ASPECT_RATIO, 1.50778,  5 },
    { 10, VTK_QUALITY_ASPECT_RATIO, 1.68296,  5 },
    { 11, VTK_QUALITY_ASPECT_RATIO, 1.42328,  5 },

    {  3, VTK_QUALITY_EDGE_RATIO,   1.     ,  5 },
    {  4, VTK_QUALITY_EDGE_RATIO,   1.26066,  5 }
};

struct vtkMeshQualitySummaryRec
{
  const char* name;
  double valuesExpected[4];
  int significantDigits;
} summaryTests[] = {
    { "Mesh Triangle Quality",      1.00000, 1.22609, 1.60536, 1.57610,   5 },
    { "Mesh Quadrilateral Quality", 1.00000, 2.06214, 3.14245, 4.82697,   5 },
    { "Mesh Tetrahedron Quality",   1.00000, 1.29600, 1.74416, 1.78348,   5 },
    { "Mesh Hexahedron Quality",    1.00000, 1.00000, 1.00000, 1.00000,   5 }
};

// A utility routine that returns -1 if the numbers are identical to
// within sigDig significant digits (base ten) or the index of the
// first significantly different entries otherwise. Well, roughly,
// anyway... there may be a missing log10(5.0) factor...
vtkIdType CompareNumbers( double* expected, double* measured, vtkIdType len, int sigDig )
{
  vtkIdType i;
  double delta;
  for ( i=0; i<len; ++i )
    {
    delta = fabs( expected[i] - measured[i] );
    if ( delta != 0.0 )
      {
      if ( log10(fabs( expected[i]/delta )) < sigDig )
        {
        return i;
        }
      }
    }
  return -1;
}

int MeshQuality( int argc, char* argv[] )
{
  vtkDebugLeaks::PromptUserOff();

  vtkUnstructuredGridReader* mr = vtkUnstructuredGridReader::New();
  vtkUnstructuredGrid* ug;
  vtkMeshQuality* iq = vtkMeshQuality::New();
  int tst;
  int status=0;
  const char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/qualityEx.vtk");

  mr->SetFileName( argc > 1 ? argv[1] : fname );
  mr->Update();

  ug = mr->GetOutput();
  iq->SetInput( ug );

  if ( ug->GetNumberOfCells() )
    {
    // Fit the First, In Which All Quality Measures Are Measured
    for ( tst=0; tst<sizeof(tests)/sizeof(tests[0]); ++tst )
      {
      double q;
      vtkIdType cid = tests[tst].cellId;
      int cellType = ug->GetCellType( cid );
      switch (cellType)
        {
      case VTK_TRIANGLE:
        iq->SetTriangleQualityMeasure( tests[tst].qualityMeasure );
        break;
      case VTK_QUAD:
        iq->SetQuadQualityMeasure( tests[tst].qualityMeasure );
        break;
      case VTK_TETRA:
        iq->SetTetQualityMeasure( tests[tst].qualityMeasure );
        break;
      case VTK_HEXAHEDRON:
        iq->SetHexQualityMeasure( tests[tst].qualityMeasure );
        break;
      default:
        // ignore this cell, we can't handle it.
        continue;
        }
      iq->Update();
      iq->GetOutput()->GetCellData()->GetArray("Quality")->GetTuple( cid, &q );
      if ( CompareNumbers( &tests[tst].qualityExpected, &q, 1, tests[tst].significantDigits) >= 0 )
        {
        cerr << "Cell " << cid << ", type " << cellType
             << ", measure " << tests[tst].qualityMeasure
             << " had quality " << q << " but "
             << tests[tst].qualityExpected << " was expected." << endl;
        status = 1;
        }
      }

    // Fit the Second, In Which Summary Information Is Verified
    iq->SetTriangleQualityMeasure( 0 );
    iq->SetQuadQualityMeasure( 3 );
    iq->SetTetQualityMeasure( 0 );
    iq->SetHexQualityMeasure( 0 );
    iq->Update();

    for ( tst=0; tst<sizeof(summaryTests)/sizeof(summaryTests[0]); ++tst )
      {
      const char* tname = summaryTests[tst].name;
      double* tvals = summaryTests[tst].valuesExpected;
      double actual[5];
      vtkDataArray* d = iq->GetOutput()->GetFieldData()->GetArray( tname );
      if ( d && d->GetNumberOfComponents() == 5 && d->GetNumberOfTuples() == 1 )
        {
        d->GetTuple( 0, actual );
        int result = CompareNumbers( tvals, actual, 4, summaryTests[tst].significantDigits );
        if ( result >= 0 )
          {
          cerr << "Summary field data \"" << tname << "\" has incorrect value " << result
               << ", got " << actual[result] << ", expected " << tvals[result] << endl;
          status = 1;
          }
        }
      else
        {
        cerr << "Summary field data \"" << tname << "\" not present or has incorrect size." << endl;
        status = 1;
        }
      }

    // Fit the Third, In Which Compatibility Mode Is Humoured
    iq->CompatibilityModeOn();
    iq->Update();
    if ( iq->GetOutput()->GetCellData()->GetArray( "Quality" )->GetNumberOfComponents() != 2 )
      {
      cerr << "CompatibilityMode should generate tuples with 2 components." << endl;
      status = 1;
      }
    }
  else
    {
    cerr << "Could not read test mesh." << endl;
    status = 1;
    }

  iq->Delete();
  mr->Delete();
  delete [] fname;
  return status;
}

