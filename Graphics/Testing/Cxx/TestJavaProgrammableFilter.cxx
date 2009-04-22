
#include "vtksys/SystemTools.hxx"

#include "vtkDebugLeaks.h"
#include "vtkJavaProgrammableFilter.h"
#include "vtkJVMManager.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariant.h"

void PrintUsage()
{
  cerr << "Usage:" << endl
       << "  -j file - Path to vtk.jar" << endl
       << "  -cp path - Path to Algorithm subclass .class files or .jar" << endl
       << "  -class classname - Fully qualified name of Algorithm subclass " << endl
       << "     (use \"/\", not \".\" to separate packages)" << endl
       ;
}

int main(int argc, char* argv[])
{
  vtkStdString classPath;
  vtkStdString className;
  vtkStdString vtkJarPath;
  for (int i = 1; i < argc; ++i)
    {
    if (!strcmp(argv[i], "-I"))
      {
      continue;
      }
    if (!strcmp(argv[i], "-D") ||
        !strcmp(argv[i], "-T") ||
        !strcmp(argv[i], "-V"))
      {
      ++i;
      continue;
      }
    else if (!strcmp(argv[i], "-j"))
      {
      vtkJarPath = argv[++i];
      }
    else if (!strcmp(argv[i], "-cp"))
      {
      classPath = argv[++i];
      }
    else if (!strcmp(argv[i], "-class"))
      {
      className = argv[++i];
      }
    else
      {
      PrintUsage();
      return 1;
      }
    }
  
  if (className.length() == 0 || classPath.length() == 0 ||
      vtkJarPath.length() == 0)
    {
    PrintUsage();
    return 1;
    }

  // The VTK library path should be the same as the test executable path
  vtkStdString pathOut, errorMsg;
  if (!vtksys::SystemTools::FindProgramPath(argv[0], pathOut, errorMsg))
    {
    cerr << errorMsg;
    return 1;
    }
  vtkStdString vtkLibraryPath = vtksys::SystemTools::GetProgramPath(pathOut.c_str());

  vtkJVMManager::AddClassPath(vtkJarPath);
  vtkJVMManager::AddLibraryPath(vtkLibraryPath);
  
  vtkSmartPointer<vtkJavaProgrammableFilter> filter =
    vtkSmartPointer<vtkJavaProgrammableFilter>::New();
  int numRows = 5;
  int numCols = 7;
  double defaultVal = -1.0;
  filter->SetJavaClassPath(classPath);
  filter->SetJavaClassName(className);

  cerr << "Initializing filter ..." << endl;
  filter->Initialize();
  filter->SetParameter("Rows", numRows);
  filter->SetParameter("Columns", numCols);
  filter->SetParameter("Default Value", defaultVal);
  cerr << "... success." << endl;

  cerr << "Updating filter ..." << endl;
  filter->Update();
  cerr << "... success." << endl;

  cerr << "Checking output ..." << endl;
  vtkTable* output = vtkTable::SafeDownCast(filter->GetOutputDataObject(0));
  output->Dump(5);  
  if (output->GetNumberOfRows() != numRows || output->GetNumberOfColumns() != numCols)
    {
    cerr << "Filter did not generate a table of the correct size!" << endl;
    cerr << "Expected " << numRows << " rows and " << numCols << " columns" << endl;
    cerr << "Output has " << output->GetNumberOfRows() << " rows and " << output->GetNumberOfColumns() << " columns" << endl;
    return 1;
    }
  for (vtkIdType c = 0; c < numCols; ++c)
    {
    for (vtkIdType r = 0; r < numRows; ++r)
      {
      double val = output->GetValue(r, c).ToDouble();
      if (val != defaultVal)
        {
        cerr << "Bad value in table! " << val << " != " << defaultVal << endl;
        return 1;
        }
      }
    }
  cerr << "... success." << endl;

  vtkSmartPointer<vtkJVMManager> manager = vtkSmartPointer<vtkJVMManager>::New();
  manager->CreateJVM();
  manager->CallStaticMethod("vtk/vtkGlobalJavaHash", "DeleteAll", "()V");

  vtkJVMManager::RemoveAllClassPaths();
  vtkJVMManager::RemoveAllLibraryPaths();

  // There will be leaks, but this is because Java cannot guarantee to
  // delete everything. Succeed anyway.
  vtkDebugLeaks::SetExitError(0);

  return 0;
}
