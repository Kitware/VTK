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
  char *OutputFile;
  
  InputFile = 
    vtkTestUtilities::ExpandDataFileName (argc, argv, "data/edgeFaceElem.exii");
  if (!InputFile)
    {
    return 1;
    }

  OutputFile =  
    vtkTestUtilities::ExpandDataFileName (argc, argv, "data/testExodus.exii");
  if (!OutputFile)
    {
      return 1;
    }

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
  delete [] OutputFile;

  return 0;
}
