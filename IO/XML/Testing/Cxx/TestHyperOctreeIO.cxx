/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperOctreeIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example creates, saves to disk, reloads from disk, and then draws
// a vtkHyperOctree. The purpose is to check that disk IO of HyperOctrees
// works.

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkLookupTable.h"

#include "vtkPolyDataMapper.h"
#include "vtkTimerLog.h"
#include "vtkHyperOctreeSampleFunction.h"
#include "vtkHyperOctreeContourFilter.h"
#include "vtkXMLHyperOctreeWriter.h"
#include "vtkXMLHyperOctreeReader.h"
#include "vtkSphere.h"
#include "vtkCamera.h"

#include "vtkMultiBlockDataSet.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkHyperOctreeSurfaceFilter.h"

#include "vtkFieldData.h"
#include "vtkCharArray.h"
#include "vtkDataObject.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

int TestHyperOctreeIO(int argc, char* argv[])
{
  int dimension = 3;
  int levels = 5;
  int skipreader = 0;
  int binary = 2;
  int compressed = 1;
  int showcontour = 1;
  int ncontours = 1;
  int rewrite = 0;
#ifdef HYPEROCTREEIO_STANDALONE
  int interactive = 0;
#endif

  if (argc > 1)
    {
    for (int i = 0; i < argc; i++)
      {
      if (!strcmp("-dim",argv[i]))
        {
        i++;
        dimension = atoi(argv[i]);
        if (dimension < 1)
          {
          dimension = 1;
          }
        if (dimension > 3)
          {
          dimension = 3;
          }
        }
      else if (!strcmp("-levels",argv[i]))
        {
        i++;
        levels = atoi(argv[i]);
        if (levels < 1)
          {
          levels = 1;
          }
        if (levels > 10)
          {
          levels = 10;
          }
        }
      else if (!strcmp("-skipreader",argv[i]))
        {
        skipreader = 1;
        }
      else if (!strcmp("-binary",argv[i]))
        {
        binary = 1;
        }
      else if (!strcmp("-appended",argv[i]))
        {
        binary = 2;
        }
      else if (!strcmp("-ncompressed",argv[i]))
        {
        compressed = 0;
        }
      else if (!strcmp("-rewrite",argv[i]))
        {
        rewrite = 1;
        }
      else if (!strcmp("-showsurface",argv[i]))
        {
        showcontour = 0;
        }
      else if (!strcmp("-ncontours",argv[i]))
        {
        i++;
        ncontours = atoi(argv[i]);
        if (ncontours < 1)
          {
          ncontours = 1;
          }
        if (ncontours > 3)
          {
          ncontours = 3;
          }
        }
#ifdef HYPEROCTREEIO_STANDALONE
      else if (!strcmp("-interactive",argv[i]))
        {
        interactive = 1;
        }
      else if (i > 1)
        {
        cout << "Unrecognized argument " << argv[i] << endl;
        }
#endif
      }
    }

  //-----------------------------------------------------------------
  vtkTimerLog *timer=vtkTimerLog::New();

  //-----------------------------------------------------------------
  // Standard rendering classes
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  //-----------------------------------------------------------------
  // Generate the data

  vtkHyperOctreeSampleFunction *source=vtkHyperOctreeSampleFunction::New();
  vtkSphere *f=vtkSphere::New();
  f->SetRadius(1);
  f->SetCenter(1,1,0);
  source->SetImplicitFunction(f);
  source->SetThreshold(0.2);
  f->Delete();

  source->SetDimension(dimension);
  source->SetWidth(2);
  source->SetHeight(3);
  source->SetDepth(4);
  source->SetLevels(levels);
  source->SetMinLevels(0);

  cout<<"update source..."<<endl;
  timer->StartTimer();
  source->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source updated"<<endl;
  cout<<"source time="<<timer->GetElapsedTime()<<" s"<<endl;

  //Adds some field data to the HyperOctree to test IO of field data
  vtkFieldData *fd = source->GetOutput()->GetFieldData();
  vtkCharArray *ca = vtkCharArray::New();
  ca->InsertNextValue('T');
  ca->InsertNextValue('E');
  ca->InsertNextValue('S');
  ca->InsertNextValue('T');
  ca->SetName("FDTestArray");
  fd->AddArray(ca);
  ca->Delete();

  //------------------------------------------------------------------
  // Test saving it to file
  vtkXMLHyperOctreeWriter *writerX=vtkXMLHyperOctreeWriter::New();
  writerX->SetInputConnection(0,source->GetOutputPort(0));
  writerX->SetFileName("HyperOctreeSample.vto");
  writerX->SetDataModeToAscii();
  if (binary==1)
    {
    writerX->SetDataModeToBinary();
    }
  if (binary==2)
    {
    writerX->SetDataModeToAppended();
    }
  if (!compressed)
    {
    writerX->SetCompressor(NULL);
    }

  cout<<"update writerX..."<<endl;
  timer->StartTimer();
  writerX->Write();
  timer->StopTimer();
  cout<<"HyperOctree written"<<endl;
  cout<<"writerX time="<<timer->GetElapsedTime()<<" s"<<endl;
  writerX->Delete();

  //------------------------------------------------------------------
  // Test reading back the saved file

  vtkXMLHyperOctreeReader *readerX=vtkXMLHyperOctreeReader::New();
  readerX->SetFileName("HyperOctreeSample.vto");

  cout<<"update readerX..."<<endl;
  timer->StartTimer();
  readerX->Update();
  timer->StopTimer();
  cout<<"readerX updated"<<endl;
  cout<<"readerX time="<<timer->GetElapsedTime()<<" s"<<endl;

  if (rewrite)
    {
    writerX=vtkXMLHyperOctreeWriter::New();
    writerX->SetInputConnection(0,readerX->GetOutputPort(0));
    writerX->SetFileName("HyperOctreeSample2.vto");
    writerX->SetDataModeToAscii();
    if (binary==1)
      {
      writerX->SetDataModeToBinary();
      }
    if (binary==2)
      {
      writerX->SetDataModeToAppended();
      }
    if (!compressed)
      {
      writerX->SetCompressor(NULL);
      }
    writerX->Write();
    writerX->Delete();
    cout<<"HyperOctree written again"<<endl;
    }

  // -----------------------------------------------------------------
  // Display the results with either contour or surface
  vtkHyperOctreeContourFilter *contour=vtkHyperOctreeContourFilter::New();
  contour->SetNumberOfContours(ncontours);
  contour->SetValue(0,0.5);
  if (ncontours > 1)
    {
    contour->SetValue(1,4.0);
    }
  if (ncontours > 2)
    {
    contour->SetValue(2,8.0);
    }

  vtkMultiBlockDataSet *hds=vtkMultiBlockDataSet::New();
  hds->SetNumberOfBlocks(1);

  if (skipreader)
    {
    hds->SetBlock(0, source->GetOutput());
    contour->SetInputConnection(0, source->GetOutputPort(0));
    }
  else
    {
    hds->SetBlock(0, readerX->GetOutput());
    contour->SetInputConnection(0, readerX->GetOutputPort(0));
    }

  source->Delete();
  readerX->Delete();

  cout<<"update contour..."<<endl;
  timer->StartTimer();
  contour->Update();
  timer->StopTimer();
  cout<<"contour updated"<<endl;
  cout<<"contour time="<<timer->GetElapsedTime()<<" s"<<endl;

  vtkHyperOctreeSurfaceFilter *surface=vtkHyperOctreeSurfaceFilter::New();
  vtkCompositeDataPipeline *exec = vtkCompositeDataPipeline::New();
  // Make sure we call SetExecutive right after the filter creation and
  // before the SetInput call.
  surface->SetExecutive(exec);
  surface->SetInputData(hds);
  exec->Delete();
  hds->Delete();

  // This creates a blue to red lut.
  vtkLookupTable *lut = vtkLookupTable::New();
  lut->SetHueRange (0.667, 0.0);

  vtkPolyDataMapper *cmapper = vtkPolyDataMapper::New();
  cmapper->SetInputConnection(0, contour->GetOutputPort(0) );
  cmapper->SetLookupTable(lut);
  cmapper->SetScalarModeToUseCellData();

  vtkCompositePolyDataMapper *smapper=vtkCompositePolyDataMapper::New();
  smapper->SetInputConnection(0,surface->GetOutputPort(0));
  smapper->SetLookupTable(lut);
  smapper->SetScalarModeToUseCellData();

  if(contour->GetOutput()->GetCellData()!=0)
    {
    if(contour->GetOutput()->GetCellData()->GetScalars()!=0)
      {
      smapper->SetScalarRange( contour->GetOutput()->GetCellData()->
                               GetScalars()->GetRange());
      }
    }
  surface->Delete();
  contour->Delete();

  vtkActor *actor = vtkActor::New();
  if (showcontour)
    {
    actor->SetMapper(cmapper);
    }
  else
    {
    actor->SetMapper(smapper);
    }
  renderer->AddActor(actor);

  // Standard testing code.
  renderer->SetBackground(0.5,0.5,0.5);
  renWin->SetSize(300,300);
  vtkCamera *cam=renderer->GetActiveCamera();
  renderer->ResetCamera();
  cam->Azimuth(180);
  renWin->Render();

  int retVal = 1;
#ifdef HYPEROCTREEIO_STANDALONE
  if (interactive)
    {
    iren->Start();
    }
#else
  retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  if (retVal == 1)
    {
    //test passed
    unlink("HyperOctreeSample.vto");
    }
#endif

  // Cleanup

  actor->Delete();
  smapper->Delete();
  cmapper->Delete();
  lut->Delete();

  iren->Delete();
  renWin->Delete();
  renderer->Delete();

  timer->Delete();

  //regresstest and IOCxxTests have opposite senses
  return !retVal;
}
