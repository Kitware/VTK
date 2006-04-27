/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageAccumulate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageData.h"
#include "vtkImageSinusoidSource.h"
#include "vtkImageAccumulate.h"
#include "vtkImageViewer.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include <math.h>

int ImageAccumulate(int , char *[])
{
  int rval = 0;

  // Create a blank image
  vtkImageSinusoidSource *sinus = vtkImageSinusoidSource::New();
  sinus->SetWholeExtent(0,512-1,0,512-1,0,64-1);
  sinus->SetAmplitude( 0 );
  sinus->Update();

  // Set some value
  sinus->GetOutput()->SetScalarComponentFromDouble (  0,   0,  0, 0, 1);
  sinus->GetOutput()->SetScalarComponentFromDouble ( 10,  10, 10, 0, 2);
  sinus->GetOutput()->SetScalarComponentFromDouble ( 10, 100, 20, 0, 3);
  sinus->GetOutput()->SetScalarComponentFromDouble (100,  10, 30, 0, 4);
  sinus->GetOutput()->SetScalarComponentFromDouble (100, 100, 40, 0, 5);

  vtkImageAccumulate *acc = vtkImageAccumulate::New();
  acc->SetInputConnection( sinus->GetOutputPort() );
  //acc->IgnoreZeroOn();
  acc->Update();

  acc->Print(cout);
  double min[3], max[3];
  acc->GetMin(min);
  acc->GetMax(max);
  if( min[0] != 0 )
    {
    cerr << "Min: " << min[0] << endl;
    rval++;
    }
  if( max[0] != 5 )
    {
    cerr << "Max: " << max[0] << endl;
    rval++;
    }
  double mean[3];
  long int voxcount;
  voxcount = acc->GetVoxelCount();
  acc->GetMean(mean);
  double m = double(1+2+3+4+5)/voxcount;
  if( fabs(mean[0] - m) > 1e-10 )
    {
    cerr << "Mean: " << mean[0] << endl;
    rval++;
    }


  // Test IgnoreZero option
  acc->IgnoreZeroOn();
  acc->Update();
  acc->Print( cout );

  // Simple test
  acc->GetMin(min);
  acc->GetMax(max);
  if( min[0] != 1 )
    {
    cerr << "Min: " << min[0] << endl;
    rval++;
    }
  if( max[0] != 5 )
    {
    cerr << "Max: " << max[0] << endl;
    rval++;
    }
  acc->GetMean(mean);
  if( mean[0] != double(1+2+3+4+5)/5  )
    {
    cerr << "Mean: " << mean[0] << endl;
    rval++;
    }

  sinus->Delete();
  acc->Delete();

  return rval;
}

