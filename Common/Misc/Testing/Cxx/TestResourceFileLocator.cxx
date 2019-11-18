#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkResourceFileLocator.h"
#include "vtkVersion.h"

#include <vtksys/SystemTools.hxx>

int TestResourceFileLocator(int, char*[])
{
  auto vtklib = vtkGetLibraryPathForSymbol(GetVTKVersion);
  if (vtklib.empty())
  {
    cerr << "FAILED to locate `GetVTKVersion`." << endl;
    return EXIT_FAILURE;
  }
  const std::string vtkdir = vtksys::SystemTools::GetFilenamePath(vtklib);

  vtkNew<vtkResourceFileLocator> locator;
  locator->SetLogVerbosity(vtkLogger::VERBOSITY_INFO);
  auto path = locator->Locate(vtkdir, "Testing/Temporary");
  if (path.empty())
  {
    cerr << "FAILED to locate 'Testing/Temporary' dir." << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
