#include "vtkStdString.h"
#include "vtkExodusIIReader.h"
#include "vtkExodusIIWriter.h"
#include "vtkFieldData.h"
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
  reader->SetGlobalResultArrayStatus ("CALIBER", 1);
  reader->SetGlobalResultArrayStatus ("GUNPOWDER", 1);
  reader->Update ();

  vtkMultiBlockDataSet* mbds = reader->GetOutput ();
  if (!mbds)
    {
    return 1;
    }
  vtkMultiBlockDataSet* elems = vtkMultiBlockDataSet::SafeDownCast (mbds->GetBlock (0));
  if (!elems)
    {
    return 1;
    }
  if (elems->GetNumberOfBlocks () != 2)
    {
    return 1;
    }

  vtkFieldData *ifieldData = elems->GetBlock(0)->GetFieldData ();
  int index;
  if (ifieldData->GetArray ("CALIBER", index) == 0)
    {
    cerr << "Expected to find array CALIBER in original data set" << endl;
    return 1;
    }

  if (ifieldData->GetArray ("GUNPOWDER", index) == 0)
    {
    cerr << "Expected to find array GUNPOWDER in original data set" << endl;
    return 1;
    }

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
  outputReader->SetGlobalResultArrayStatus ("CALIBER", 1);
  outputReader->SetGlobalResultArrayStatus ("GUNPOWDER", 1);
  outputReader->Update ();

  mbds = outputReader->GetOutput ();
  if (!mbds)
    {
    return 1;
    }
  elems = vtkMultiBlockDataSet::SafeDownCast (mbds->GetBlock (0));
  if (!elems)
    {
    return 1;
    }
  if (elems->GetNumberOfBlocks () != 2)
    {
    return 1;
    }

  ifieldData = elems->GetBlock(0)->GetFieldData ();
  if (ifieldData->GetArray ("CALIBER", index) == 0)
    {
    cerr << "Array CALIBER was not written to output" << endl;
    return 1;
    }

  if (ifieldData->GetArray ("GUNPOWDER", index) == 0)
    {
    cerr << "Array GUNPOWDER was not written to output" << endl;
    return 1;
    }

  vtkCompositeDataIterator* iter = mbds->NewIterator ();
  iter->InitTraversal ();
  vtkDataSet* ds = vtkDataSet::SafeDownCast (iter->GetCurrentDataObject ());
  iter->Delete ();
  if (!ds)
    {
    return 1;
    }

  VTK_CREATE (vtkDataSetMapper, mapper);
  mapper->SetInputData (ds);

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

  delete [] InputFile;

  return !retVal;

}
