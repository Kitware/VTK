/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPriorityStreaming.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of the prioity streaming support in VTK

#include "vtkContourFilter.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTesting.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPImageDataWriter.h"

//---------------------------------------------------------------------------
int TestPriorityStreaming(int argc, char *argv[])
{
  // parse the arguments
  vtkSmartPointer<vtkTesting> test = 
    vtkSmartPointer<vtkTesting>::New();
  int cc;
  for ( cc = 1; cc < argc; cc ++ )
    {
    test->AddArgument(argv[cc]);
    }
  
  // first create a data file containing many pieces
  // first we want to create some data, a 256 cubed Mandelbrot src
  vtkSmartPointer<vtkImageMandelbrotSource> Mandelbrot = 
    vtkSmartPointer<vtkImageMandelbrotSource>::New();
  Mandelbrot->SetWholeExtent(0,127,0,127,0,127);
  Mandelbrot->SetOriginCX(-1.75,-1.25,-1,0);
  Mandelbrot->Update();
  
  // write out the image data file into many pieces
  vtkSmartPointer<vtkXMLImageDataWriter> iw =
    vtkSmartPointer<vtkXMLImageDataWriter>::New();
  iw->SetInputConnection(Mandelbrot->GetOutputPort());
  vtkstd::string fname = test->GetTempDirectory();
  fname += "/StreamTestFile.vti";
  iw->SetFileName(fname.c_str());
  iw->SetNumberOfPieces(64);
  iw->Write();

  // create a reader
  vtkSmartPointer<vtkXMLImageDataReader> ir =
    vtkSmartPointer<vtkXMLImageDataReader>::New();
  ir->SetFileName(fname.c_str());
  
  vtkSmartPointer<vtkContourFilter> contour =
    vtkSmartPointer<vtkContourFilter>::New();
  contour->SetInputConnection(ir->GetOutputPort());
  contour->SetValue(0,50);
  
  // lets get some priorities :-)
  vtkInformationVector *outVec = 
    contour->GetExecutive()->GetOutputInformation();
  vtkInformation *outInfo = outVec->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
               iw->GetNumberOfPieces());

  // build the UE request
  vtkSmartPointer<vtkInformation> UpdateExtentRequest =
    vtkSmartPointer<vtkInformation>::New();
  UpdateExtentRequest->Set
    (vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT());
  UpdateExtentRequest->Set(vtkExecutive::FORWARD_DIRECTION(), 
                           vtkExecutive::RequestUpstream);
  UpdateExtentRequest->Set(vtkExecutive::ALGORITHM_BEFORE_FORWARD(), 1);
  UpdateExtentRequest->Set(vtkExecutive::FROM_OUTPUT_PORT(), 0);
  
  // build the UEInfo request
  vtkSmartPointer<vtkInformation> UEInfoRequest =
    vtkSmartPointer<vtkInformation>::New();
  UEInfoRequest->Set
    (vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT_INFORMATION());
  UEInfoRequest->Set(vtkExecutive::FORWARD_DIRECTION(), 
                     vtkExecutive::RequestUpstream);
  UEInfoRequest->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
  UEInfoRequest->Set(vtkExecutive::FROM_OUTPUT_PORT(), 0);
  
  // store the in and out info
  vtkStreamingDemandDrivenPipeline *sdd = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(contour->GetExecutive());
  sdd->UpdateInformation();
  
  vtkInformationVector **inVec = 
    contour->GetExecutive()->GetInputInformation();

  int piece;
  double *priority = new double [iw->GetNumberOfPieces()];
  for (piece = 0; piece < iw->GetNumberOfPieces(); piece++)
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                 piece);
    contour->GetExecutive()->
      ProcessRequest(UpdateExtentRequest, inVec, outVec);
    // get the ue info
    contour->GetExecutive()->
      ProcessRequest(UEInfoRequest, inVec, outVec);

    // get the priority
    priority[piece] = 
      outVec->GetInformationObject(0)->
      Get(vtkStreamingDemandDrivenPipeline::PRIORITY());
    }

  if (iw->GetNumberOfPieces() != 64 ||
      priority[36] != 0.0 ||
      priority[37] != 1.0)
    {
    delete [] priority;
    cerr << "Bad results for prioity streaming test\n";
    return 1;
    }
  delete [] priority;
  return 0;
}
