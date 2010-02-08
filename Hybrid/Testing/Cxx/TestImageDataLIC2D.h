/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageDataLIC2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef  __TestImageDataLIC2D_h
#define  __TestImageDataLIC2D_h

#include "vtkGenericDataObjectReader.h"
#include "vtkImageDataLIC2D.h"
#include "vtkImageData.h"
#include "vtkImageIterator.h"
#include "vtkImagePermute.h"
#include "vtkImageShiftScale.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkProbeFilter.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkTimerLog.h"
#include "vtkUnstructuredGrid.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkTesting.h"
#include <vtksys/CommandLineArguments.hxx>
#include "vtkImageMapToColors.h"
#include "vtkLookupTable.h"

#define CREATE_NEW(var, class) vtkSmartPointer<class> var = vtkSmartPointer<class>::New();


//-----------------------------------------------------------------------------
void Merge(vtkImageData* dest, vtkImageData* src)
{
  if (!src || !dest)
    {
    return;
    }


  if (src->GetScalarType() != dest->GetScalarType())
    {
    cout << src->GetScalarTypeAsString() << ", " << dest->GetScalarTypeAsString() << endl;
    abort();
    }

  vtkImageIterator<unsigned char> inIt(src, src->GetExtent());
  int outextent[6];
  src->GetExtent(outextent);

  vtkImageIterator<unsigned char> outIt(dest, outextent);

  while (!outIt.IsAtEnd() && !inIt.IsAtEnd())
    {
    unsigned char* spanOut = outIt.BeginSpan();
    unsigned char* spanIn = inIt.BeginSpan();
    unsigned char* outSpanEnd = outIt.EndSpan();
    unsigned char* inSpanEnd = inIt.EndSpan();
    if (outSpanEnd != spanOut && inSpanEnd != spanIn)
      {
      size_t minO = outSpanEnd - spanOut;
      size_t minI = inSpanEnd - spanIn;
      memcpy(spanOut, spanIn, (minO < minI)? minO : minI);
      }
    inIt.NextSpan();
    outIt.NextSpan();
    }
}

// Example demonstrating use of vtkImageDataLIC2D filter.
// Typical usage:
// ./bin/ImageDataLIC2D --data=<vtk file> --output=<png file>
int ImageDataLIC2D(int argc, char* argv[])
{
  vtkstd::string filename;
  vtkstd::string noise_filename;
  int resolution = 10;
  int magnification = 1;
  vtkstd::string outputpath;
  int num_partitions = 1;
  int num_steps = 40;

  vtksys::CommandLineArguments arg;
  arg.StoreUnusedArguments(1);
  arg.Initialize(argc, argv);

  typedef vtksys::CommandLineArguments argT;
  arg.AddArgument("--data", argT::EQUAL_ARGUMENT, &filename,
    "(required) Enter dataset to load (currently only *.vtk files are supported");
  arg.AddArgument("--res", argT::EQUAL_ARGUMENT, &resolution,
    "(optional: default 10) Number of sample per unit distance");
  arg.AddArgument("--mag", argT::EQUAL_ARGUMENT, &magnification,
    "(optional: default 1) Magnification");
  arg.AddArgument("--output", argT::EQUAL_ARGUMENT, &outputpath,
    "(optional) Output png image");
  arg.AddArgument("--partitions", argT::EQUAL_ARGUMENT, &num_partitions,
    "(optional: default 1) Number of partitions");
  arg.AddArgument("--num-steps", argT::EQUAL_ARGUMENT, &num_steps,
    "(optional: default 40) Number of steps in each direction");
  arg.AddArgument("--noise", argT::EQUAL_ARGUMENT, &noise_filename,
    "(optional) Specify the filename to a png image file to use as the noise texture.");

  if (!arg.Parse() || filename=="")
    {
    cerr << "Problem parsing arguments." << endl;
    cerr << arg.GetHelp() << endl;
    return 1;
    }

  if (magnification < 1)
    {
    cout << "WARNING: Magnification \'" << magnification  << "\' is invalid."
      " Forcing a magnification of 1.";
    magnification = 1;
    }

  if (num_steps < 1)
    {
    cout << "WARNING: Number of steps cannot be less than 1. Forcing 10.";
    num_steps = 10;
    }

  CREATE_NEW(reader,vtkGenericDataObjectReader);
  reader->SetFileName(filename.c_str());
  reader->Update();
  
  double bounds[6];
  vtkDataSet::SafeDownCast(reader->GetOutput())->GetBounds(bounds);

  // If 3D use XY slice, otherwise use non-trivial slice.
  int dataDesc = VTK_XY_PLANE;
  if (bounds[0] == bounds[1])
    {
    dataDesc = VTK_YZ_PLANE;
    }
  else if (bounds[2] == bounds[3])
    {
    dataDesc = VTK_XZ_PLANE;
    }
  else if (bounds[4] == bounds[5])
    {
    dataDesc = VTK_XY_PLANE;
    }

  CREATE_NEW(probeData,vtkImageData);
  probeData->SetOrigin(bounds[0], bounds[2], bounds[4]);
  int width = 0;
  int height = 0;
  switch (dataDesc)
    {
  case VTK_XY_PLANE:
    width = static_cast<int>(ceil((bounds[1]-bounds[0]) * resolution));
    height = static_cast<int>(ceil((bounds[3]-bounds[2]) * resolution));
    probeData->SetDimensions(width, height, 1);
    probeData->SetSpacing(
      (bounds[1]-bounds[0])/double(width), (bounds[3]-bounds[2])/double(height), 1);
    break;

  case VTK_YZ_PLANE:
    width = static_cast<int>(ceil((bounds[3]-bounds[2]) * resolution));
    height = static_cast<int>(ceil((bounds[5]-bounds[4]) * resolution));
    probeData->SetDimensions(1, width, height);
    probeData->SetSpacing(
      1, (bounds[3]-bounds[2])/double(width), (bounds[5]-bounds[4])/double(height));
    break;

  case VTK_XZ_PLANE:
    width = static_cast<int>(ceil((bounds[1]-bounds[0]) * resolution));
    height = static_cast<int>(ceil((bounds[5]-bounds[4]) * resolution));
    probeData->SetDimensions(width, 1, height);
    probeData->SetSpacing(
      (bounds[1]-bounds[0])/double(width), 1, (bounds[5]-bounds[4])/double(height));
    break;
    }

  CREATE_NEW(probe,vtkProbeFilter);
  probe->SetSource(reader->GetOutput());
  probe->SetInput(probeData);
  probe->Update();
  
  CREATE_NEW(renWin, vtkRenderWindow);
  renWin->Render();

  CREATE_NEW(output, vtkImageData);
  output->SetDimensions(width * magnification, height * magnification, 1);
  output->SetSpacing(probeData->GetSpacing());
  output->SetOrigin(probeData->GetOrigin());
  output->SetScalarTypeToUnsignedChar();
  output->SetNumberOfScalarComponents(3);
  output->AllocateScalars();
  
  CREATE_NEW( filter, vtkImageDataLIC2D );
  if (  filter->SetContext( renWin ) == 0  )
    {
    cout << "Required OpenGL extensions / GPU not supported." << endl;
    return 0;
    }
    
  filter->SetInputConnection(0, probe->GetOutputPort(0));

  if (noise_filename != "")
    {
    CREATE_NEW(pngReader,vtkPNGReader);
    pngReader->SetFileName(noise_filename.c_str());
    filter->SetInputConnection(1, pngReader->GetOutputPort(0));
    }

  filter->SetSteps(num_steps);
  filter->SetStepSize(0.8/magnification);
  filter->SetMagnification(magnification);
  filter->UpdateInformation();

  int original_extents[6];
  probeData->GetExtent(original_extents);

  for (int kk=0; kk < num_partitions; kk++)
    {
    vtkStreamingDemandDrivenPipeline* sddp = vtkStreamingDemandDrivenPipeline::SafeDownCast(
      filter->GetExecutive());
    sddp->SetUpdateExtent(0, kk, num_partitions, 0);

    vtkTimerLog* timer = vtkTimerLog::New();
    timer->StartTimer();
    filter->Update();
    if ( filter->GetFBOSuccess() == 0 ||
         filter->GetLICSuccess() == 0 )
      {
      timer->Delete();
      sddp   = NULL;
      timer  = NULL;
      return 0;
      }
    timer->StopTimer();

    //cout << "Time: " << timer->GetElapsedTime() << endl;
    timer->Delete();

    CREATE_NEW(clone, vtkImageData);
    clone->ShallowCopy(filter->GetOutput());
    
    // input is double between 0.0 and 1.0. Cast it between [0, 255].
    CREATE_NEW(caster, vtkImageShiftScale);
    caster->SetInput(clone);
    caster->SetShift(0.0);
    caster->SetScale(255.0);
    caster->SetOutputScalarTypeToUnsignedChar();

    CREATE_NEW(permuter, vtkImagePermute);
    permuter->SetInputConnection(caster->GetOutputPort());
    switch (dataDesc)
      {
    case VTK_XY_PLANE:
      permuter->SetFilteredAxes(0, 1, 2);
      break;

    case VTK_YZ_PLANE:
      permuter->SetFilteredAxes(1, 2, 0);
      break;

    case VTK_XZ_PLANE:
      permuter->SetFilteredAxes(0, 2, 1);
      break;
      }
    permuter->Update();
    ::Merge(output, permuter->GetOutput());
    }

  CREATE_NEW(tester, vtkTesting);
  for (int cc=0; cc < argc; cc++)
    {
    tester->AddArgument(argv[cc]);
    }

  if (outputpath != "")
    {
    CREATE_NEW(writer, vtkPNGWriter);
    writer->SetFileName(outputpath.c_str());
    writer->SetInput(output);
    writer->Write();
    }

  return (!tester->IsValidImageSpecified() || 
    (tester->RegressionTest(output, 10) == vtkTesting::PASSED))? /*success*/ 0 : /*failure*/ 1;
}

#endif
