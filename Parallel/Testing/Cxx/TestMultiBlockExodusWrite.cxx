#include "vtkStdString.h"
#include "vtkExodusIIReader.h"
#include "vtkExodusIIWriter.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

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

  VTK_CREATE (vtkTesting, testing);
  for (int i = 0; i < argc; i ++)
    {
    testing->AddArgument (argv[i]);
    }

  vtkStdString OutputFile;
  OutputFile = testing->GetTempDirectory ();
  OutputFile += "/testExodus.exii";

  VTK_CREATE (vtkExodusIIReader, reader);
  if (!reader->CanReadFile (InputFile)) 
    {
    return 1;
    }
  reader->SetFileName (InputFile);

  VTK_CREATE (vtkExodusIIWriter, writer);
  writer->SetInputConnection (reader->GetOutputPort ());
  writer->SetFileName (OutputFile);
  writer->WriteOutBlockIdArrayOn ();
  writer->WriteOutGlobalNodeIdArrayOn ();
  writer->WriteOutGlobalElementIdArrayOn ();
  writer->WriteAllTimeStepsOn ();
  writer->Update ();

  delete [] InputFile;

  return 0;
}
