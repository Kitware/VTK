#include "vtkStdString.h"
#include "vtkExodusIIReader.h"
#include "vtkExodusIIWriter.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPNGWriter.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New ();

int TestMultiBlockExodusWrite (int argc, char *argv[])
{
  char *InputFile;
  
  InputFile = 
    vtkTestUtilities::ExpandDataFileName (argc, argv, "Data/edgeFaceElem.exii");
  if (!InputFile)
    {
    return 1;
    }

  VTK_CREATE (vtkExodusIIReader, reader);
  if (!reader->CanReadFile (InputFile)) 
    {
    return 1;
    }
  reader->SetFileName (InputFile);

  VTK_CREATE (vtkTesting, testing);
  for (int i = 0; i < argc; i ++)
    {
    testing->AddArgument (argv[i]);
    }

  vtkStdString OutputFile;
  OutputFile = testing->GetTempDirectory ();
  OutputFile += "/testExodus.exii";

  VTK_CREATE (vtkExodusIIWriter, writer);
  writer->SetInputConnection (reader->GetOutputPort ());
  writer->SetFileName (OutputFile);
  writer->WriteOutBlockIdArrayOn ();
  writer->WriteOutGlobalNodeIdArrayOn ();
  writer->WriteOutGlobalElementIdArrayOn ();
  writer->WriteAllTimeStepsOn ();
  writer->Update ();

  VTK_CREATE (vtkExodusIIReader, outputReader);
  if (!outputReader->CanReadFile (OutputFile))
    {
    return 1;
    }
  outputReader->SetFileName (OutputFile);
  outputReader->Update ();

  vtkMultiBlockDataSet* mbds = outputReader->GetOutput ();
  vtkCompositeDataIterator* iter = mbds->NewIterator ();
  iter->InitTraversal ();
  vtkDataSet* ds = vtkDataSet::SafeDownCast (iter->GetCurrentDataObject ());
  if (!ds)
    {
    iter->Delete ();
    return 1;
    }

  VTK_CREATE (vtkDataSetMapper, mapper);
  mapper->SetInput (ds);

  VTK_CREATE (vtkActor, actor);
  actor->SetMapper (mapper);

  VTK_CREATE (vtkRenderer, renderer);
  renderer->AddActor (actor);
  renderer->SetBackground (0.0, 0.0, 0.0);
  vtkCamera* c = renderer->GetActiveCamera ();
  c->SetPosition (0.0, 10.0, 14.5);
  c->SetFocalPoint (0, 0, 0);
  c->SetViewUp (0.8, 0.3, -0.5);
  c->SetViewAngle (30);

  VTK_CREATE (vtkRenderWindow, renderWindow);
  renderWindow->AddRenderer (renderer);
  renderWindow->SetSize (256, 256);

  VTK_CREATE (vtkRenderWindowInteractor, irenderWindow);
  irenderWindow->SetRenderWindow (renderWindow);

  int retVal = vtkRegressionTestImage (renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    renderWindow->Render ();
    irenderWindow->Start ();
    retVal = vtkRegressionTester::PASSED;
    }

/*
  VTK_CREATE (vtkWindowToImageFilter, w2i); 
  w2i->SetInput (renderWindow);
  
  VTK_CREATE (vtkPNGWriter, img);
  img->SetInputConnection (w2i->GetOutputPort ());
  img->SetFileName ("TestMultiBlockExodusWrite.png");

  renderWindow->Render ();
  w2i->Modified ();
  img->Write ();
  return 1;
*/

  iter->Delete ();

  delete [] InputFile;

  return !retVal;

}
