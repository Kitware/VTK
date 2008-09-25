/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageStencilData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// The test creates two box-shaped image stencils from rectangular polydata.
// The stencils are added / subtracted, converted to an image and compared
// to a baseline

#include "vtkPolyDataToImageStencil.h"
#include "vtkLinearExtrusionFilter.h"
#include "vtkImageStencil.h"
#include "vtkImageStencilData.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkSmartPointer.h"
#include "vtkMatrixToLinearTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkMatrix4x4.h"
#include "vtkTesting.h"


//----------------------------------------------------------------------------
static vtkSmartPointer< vtkImageStencilData > 
CreateBoxStencilData(double d1, double d2 )
{
  // Create two stencil data from polydata's
  
  vtkPolyData * pd = vtkPolyData::New();
  pd->Allocate(1, 1);
  vtkPoints * points = vtkPoints::New();
  points->InsertNextPoint( d1, d1, 0.0 );
  points->InsertNextPoint( d2, d1, 0.0 );
  points->InsertNextPoint( d2, d2, 0.0 );
  points->InsertNextPoint( d1, d2, 0.0 );
  pd->SetPoints(points);
  vtkIdType ptIds[4];
  ptIds[0] = 0; ptIds[1] = 1; ptIds[2] = 2; ptIds[3] = 3;
  pd->InsertNextCell(VTK_QUAD, 4, ptIds);
  points->Delete();

  // Extrude the contour along the normal to the plane the contour lies on.
  vtkLinearExtrusionFilter *extrudeFilter = vtkLinearExtrusionFilter::New();
  extrudeFilter->SetInput( pd );
  extrudeFilter->SetScaleFactor(1);
  extrudeFilter->SetExtrusionTypeToNormalExtrusion();
  extrudeFilter->SetVector( 0, 0, 1);
  extrudeFilter->Update();

  // Apply a transformation to the output polydata that subtracts 0.5 from 
  // the z co-ordinate.
  
  const double m[16] = {1,0,0,0,0,1,0,0,0,0,1,-0.5,0,0,0,1};
  vtkMatrixToLinearTransform *linearTransform = vtkMatrixToLinearTransform::New();
  linearTransform->GetMatrix()->DeepCopy( m );
  vtkTransformPolyDataFilter *transformPolyData = vtkTransformPolyDataFilter::New();
  transformPolyData->SetInput( extrudeFilter->GetOutput() );
  transformPolyData->SetTransform( linearTransform );
  transformPolyData->Update();
  linearTransform->Delete();

  // Rasterize the polydata (sweep it along the plane the contour lies on, 
  // bounded by the extrusion) and get extents into a stencil
  vtkPolyDataToImageStencil *contourStencilFilter 
                            = vtkPolyDataToImageStencil::New();
  contourStencilFilter->SetInput( transformPolyData->GetOutput() );

  vtkImageData *image = vtkImageData::New();
  image->SetSpacing( 1.0, 1.0, 1.0  );
  image->SetOrigin(  0.0, 0.0, 0.0 );
  image->SetExtent(static_cast<int>(d1)-2,static_cast<int>(d2)+2,
                   static_cast<int>(d1)-2,static_cast<int>(d2)+2, 0, 0 );
  image->SetScalarTypeToUnsignedChar();
  image->AllocateScalars();

  vtkImageStencil *stencil = vtkImageStencil::New();
  stencil->SetInput( image );
  stencil->SetStencil( contourStencilFilter->GetOutput() );
  stencil->SetBackgroundValue(0);
  stencil->Update();
  vtkSmartPointer< vtkImageStencilData > 
    stencilData = contourStencilFilter->GetOutput();

  extrudeFilter->Delete();
  transformPolyData->Delete();
  contourStencilFilter->Delete();
  stencil->Delete();
  image->Delete();
  pd->Delete();
  
  return stencilData;
}

//----------------------------------------------------------------------------
static void GetStencilDataAsImageData(
  vtkImageStencilData * stencilData, vtkImageData *image )
{
  int extent[6];
  stencilData->GetExtent( extent );
  extent[5] = extent[4]; // Otherwise we cannot write it out as a PNG!
  int extent1[6] = {0,50,0,50,0,0};
  image->SetExtent(extent1);
  image->SetScalarTypeToUnsignedChar();
  image->SetNumberOfScalarComponents(3);
  image->AllocateScalars();

  // Fill image with zeroes
  for (int y=extent1[2]; y <= extent1[3]; y++)
    {
    unsigned char *ptr=static_cast<unsigned char *>(
      image->GetScalarPointer(extent1[0], y, extent1[4] ));
    for (int x=extent1[0]; x <= extent1[1]; x++)
      {
      *ptr = 0;  ++ptr;
      *ptr = 0;  ++ptr;
      *ptr = 0;  ++ptr;
      }
    }

  vtkIdType increments[3];
  image->GetIncrements( increments );

  int iter = 0;
  for (int y=extent1[2]; y <= extent1[3]; y++, iter = 0)
    {
    int r1,r2;
    int moreSubExtents = 1;
    while( moreSubExtents )
      {
      moreSubExtents = stencilData->GetNextExtent(
        r1, r2, extent1[0], extent1[1], y, extent1[4], iter);

      // sanity check
      if (r1 <= r2 )
        {
        unsigned char *beginExtent =static_cast<unsigned char *>(
          image->GetScalarPointer( r1, y, extent1[4] ));
        unsigned char *endExtent   =static_cast<unsigned char *>(
          image->GetScalarPointer( r2, y, extent1[4] ));
        while (beginExtent <= endExtent)
          {
          *beginExtent     = static_cast<unsigned char>(255);
          *(beginExtent+1) = static_cast<unsigned char>(255);
          *(beginExtent+2) = static_cast<unsigned char>(255);
          beginExtent += increments[0];
          }
        }
      } // end for each extent tuple
    } // end for each scan line
}

//----------------------------------------------------------------------------
int TestImageStencilData( int argc, char * argv [] )
{
  vtkSmartPointer< vtkImageStencilData > stencil1 = 
    CreateBoxStencilData(10.0, 30.0);
  vtkSmartPointer< vtkImageStencilData > stencil2 = 
    CreateBoxStencilData(20.0, 40.0);

  vtkImageData *image = vtkImageData::New();
  vtkTesting *testing = vtkTesting::New();
  int cc;
  for ( cc = 1; cc < argc; cc ++ )
    {
    testing->AddArgument(argv[cc]);
    }

  if (atoi(argv[1]) == 1)
    {
    // Test Add stencils
    stencil1->Add(stencil2);
    GetStencilDataAsImageData(stencil1, image);
    }
  else if (atoi(argv[1]) == 2)
    {
    // Test subtraction of stencils
    stencil1->Subtract(stencil2);
    GetStencilDataAsImageData(stencil1, image);
    }
  else if (atoi(argv[1]) == 3)
    {
    // Test clipping of stencils
    stencil1->Add(stencil2);
    int clipExtents1[6] = { 15, 35, 15, 35, 0, 0 };
    stencil1->Clip( clipExtents1 );
    int clipExtents2[6] = { 35, 39, 35, 39, 0, 0 };
    stencil2->Clip( clipExtents2 );
    stencil1->Add(stencil2);
    GetStencilDataAsImageData(stencil1, image);
    }
  else
    {
    return EXIT_FAILURE;
    }

  int retval = testing->RegressionTest( image, 10 );
  testing->Delete();
  image->Delete();

  return !retval;
}

