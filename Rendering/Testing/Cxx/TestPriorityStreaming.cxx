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

// .NAME Test the priority streaming support in VTK well.

#include "vtkActor.h"
#include "vtkArrayCalculator.h"
#include "vtkCamera.h"
#include "vtkClipDataSet.h"
#include "vtkContourFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkIdentityTransform.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPlane.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTesting.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPImageDataWriter.h"

#include "vtksys/SystemTools.hxx"

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
  std::string fname = test->GetTempDirectory();
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
    double p =  outVec->GetInformationObject(0)->
      Get(vtkStreamingDemandDrivenPipeline::PRIORITY());
    //cerr << piece << " " << p << endl;
    priority[piece] = p;
    }

  if (iw->GetNumberOfPieces() != 64 ||
      priority[36] != 0.0 ||
      priority[37] != 1.0)
    {
    delete [] priority;
    // Leave file around in case somebody wants to look at it
    // after the failed test...
    cerr << "Bad results for priority streaming test. "
         << "Contour didn't reject/accept pieces correctly.\n";
    return 1;
    }

  delete [] priority;

  //--------------------------------------------------------------------------
  // Now test meta-information persistance.
  // Meta information is either about the geometry (GI), or about
  // the attribute ranges (AI).
  //
  // Filters reject pieces based on valid meta-information.
  // Filters only pass through meta-information, when they are
  // known not to modify it. Calc filter can modify either.
  //
  // Downstream of a filter that stops information, downstream filters can not
  // reject any new pieces.
  //
  //READER -> CALC   -> CONTOUR        
  //SETSGI    STOPGI    Can not reject(noR) 

  vtkSmartPointer<vtkArrayCalculator> calc_A = 
    vtkSmartPointer<vtkArrayCalculator>::New();
  calc_A->SetInputConnection(ir->GetOutputPort());
  calc_A->SetFunction("1");
  
  vtkSmartPointer<vtkContourFilter> contour_A =
    vtkSmartPointer<vtkContourFilter>::New();
  contour_A->SetInputConnection(calc_A->GetOutputPort());
  contour_A->SetValue(0,50);
        
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(contour_A->GetExecutive());
  for (int i = 0; i < 64; i++)
    {
    double lpriority = 1.0;
    sddp->SetUpdateExtent(0, i, 64, 0); 
    lpriority = sddp->ComputePriority();
    if (lpriority != 1.0)
      {
      cerr << "Bad results for priority streaming test. "
           << "Should not be able to reject based on attribute ranges." << endl;
      return 1;
      }
    }

  //READER -> CALC -> CLIP
  //SETSAI    STOPAI    Can not reject(noR) 
  vtkSmartPointer<vtkArrayCalculator> calc_B = 
    vtkSmartPointer<vtkArrayCalculator>::New();
  calc_B->SetInputConnection(ir->GetOutputPort());
  calc_B->SetFunction("1");

  vtkSmartPointer<vtkClipDataSet> clip_B = 
    vtkSmartPointer<vtkClipDataSet>::New();
  clip_B->SetInputConnection(calc_B->GetOutputPort());
  vtkSmartPointer<vtkPlane> plane_B =
    vtkSmartPointer<vtkPlane>::New();
  plane_B->SetNormal(0,1,0);
  plane_B->SetOrigin(0,0,0);
  clip_B->SetClipFunction(plane_B);

  sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(clip_B->GetExecutive());
  for (int i = 0; i < 64; i++)
    {
    double lpriority = 1.0;
    sddp->SetUpdateExtent(0, i, 64, 0); 
    lpriority = sddp->ComputePriority();
    if (lpriority != 1.0)
      {
      cerr << "Bad results for priority streaming test. "
           << "Should not be able to reject based on geometric bounds ranges." << endl;
      return 1;
      }
    }

  // Pieces rejected are union of upstream rejections.
  // Downstream removal of meta info does not "unreject" pieces.
  //READER -> CONTOUR -> CLIP1    -> CALC
  //SETSGI    PASSGI     PASSGI+R    STOPGI
  //SETSAI    PASSAI+R   PASSAI      STOPAI

  vtkSmartPointer<vtkContourFilter> contour_C =
    vtkSmartPointer<vtkContourFilter>::New();
  contour_C->SetInputConnection(ir->GetOutputPort());
  contour_C->SetValue(0,50);

  vtkSmartPointer<vtkClipDataSet> clip_C1= 
    vtkSmartPointer<vtkClipDataSet>::New();
  clip_C1->SetInputConnection(ir->GetOutputPort());
  vtkSmartPointer<vtkPlane> plane_C1 =
    vtkSmartPointer<vtkPlane>::New();
  plane_C1->SetNormal(1,0,0);
  plane_C1->SetOrigin(0,0,0);
  clip_C1->SetClipFunction(plane_C1);

  vtkSmartPointer<vtkClipDataSet> clip_C2 = 
    vtkSmartPointer<vtkClipDataSet>::New();
  clip_C2->SetInputConnection(ir->GetOutputPort());
  vtkSmartPointer<vtkPlane> plane_C2 =
    vtkSmartPointer<vtkPlane>::New();
  plane_C2->SetNormal(0,1,0);
  plane_C2->SetOrigin(0,0,0);
  clip_C2->SetClipFunction(plane_C2);

  double *priority1 = new double [64];
  double *priority2 = new double [64];
  double *priority3 = new double [64];
  sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(contour_C->GetExecutive());
  for (int i = 0; i < 64; i++)
    {
    sddp->SetUpdateExtent(0, i, 64, 0); 
    priority1[i] = sddp->ComputePriority();
    }
  sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(clip_C1->GetExecutive());
  for (int i = 0; i < 64; i++)
    {
    sddp->SetUpdateExtent(0, i, 64, 0); 
    priority2[i] = sddp->ComputePriority();
    }
  sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(clip_C2->GetExecutive());
  for (int i = 0; i < 64; i++)
    {
    sddp->SetUpdateExtent(0, i, 64, 0); 
    priority3[i] = sddp->ComputePriority();
    }

  clip_C1->SetInputConnection(contour_C->GetOutputPort());
  clip_C2->SetInputConnection(clip_C1->GetOutputPort());

  vtkSmartPointer<vtkArrayCalculator> calc_C = 
    vtkSmartPointer<vtkArrayCalculator>::New();
  calc_C->SetInputConnection(clip_C2->GetOutputPort());
  calc_C->SetFunction("1");

  vtkSmartPointer<vtkClipDataSet> clip_C3 = 
    vtkSmartPointer<vtkClipDataSet>::New();
  clip_C3->SetInputConnection(calc_C->GetOutputPort());
  vtkSmartPointer<vtkPlane> plane_C3 =
    vtkSmartPointer<vtkPlane>::New();
  plane_C3->SetNormal(0,0,1);
  plane_C2->SetOrigin(0,0,0);
  clip_C3->SetClipFunction(plane_C3);

  sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(calc_C->GetExecutive());
  for (int i = 0; i < 64; i++)
    {
    bool fail = false;
    sddp->SetUpdateExtent(0, i, 64, 0); 
    double val = priority1[i] * priority2[i] * priority3[i];
    double lpriority = sddp->ComputePriority();
    if (lpriority != val)
      {
      cerr << "Chained priority " << i << " is wrong " 
           << priority1[i] << "*" << priority2[i] << "*" << priority3[i] << "!=" << lpriority << endl;
      fail = true;
      }
    if (i == 62 && lpriority != 1.0)
      {
      cerr << "Chained priority " << i << " should be 1.0" << endl;
      fail = true;
      }
    if (i == 63 && lpriority != 0.0)
      {
      cerr << "Chained priority " << i << " should be 0.0" << endl;
      fail = true;
      }
    if (fail)
      {
      delete[] priority1;
      delete[] priority2;
      delete[] priority3;
      return 1;
      }
    }

  delete[] priority1;
  delete[] priority2;
  delete[] priority3;
  
#if 0
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  renderer->GetActiveCamera()->SetPosition( 0, 0, 10);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(     0,   1,   0);
  renderer->SetBackground(0.0,0.0,0.0); 
  renWin->SetSize(300,300);
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);  
  vtkSmartPointer<vtkDataSetMapper> map1 = vtkSmartPointer<vtkDataSetMapper>::New();
  map1->SetInputConnection(clip_C3->GetOutputPort());
  vtkSmartPointer<vtkActor> act1 = vtkSmartPointer<vtkActor>::New();
  act1->SetMapper(map1);
  renderer->AddActor(act1);
  renWin->Render();
  iren->Start();
#endif

  // Delete the file since the test passed:
  vtksys::SystemTools::RemoveFile(fname.c_str());
  return 0;
}
