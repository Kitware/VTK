/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMP4Writer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkIOMovieConfigure.h"

#ifdef VTK_USE_MICROSOFT_MEDIA_FOUNDATION
#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkImageMapToColors.h"
#include "vtkLogger.h"
#include "vtkLookupTable.h"
#include "vtkMP4Writer.h"
#include "vtkTestUtilities.h"

#include "vtksys/SystemTools.hxx"
#else
#include "vtkTesting.h"
#endif

int TestMP4Writer(int argc, char* argv[])
{
#ifdef VTK_USE_MICROSOFT_MEDIA_FOUNDATION
  vtkNew<vtkImageMandelbrotSource> fractal;
  fractal->SetWholeExtent(0, 247, 0, 247, 0, 0);
  fractal->SetProjectionAxes(0, 1, 2);
  fractal->SetOriginCX(-1.75, -1.25, 0, 0);
  fractal->SetSizeCX(2.5, 2.5, 2, 1.5);
  fractal->SetMaximumNumberOfIterations(100);

  vtkNew<vtkImageCast> cast;
  cast->SetInputConnection(fractal->GetOutputPort());
  cast->SetOutputScalarTypeToUnsignedChar();

  vtkNew<vtkLookupTable> table;
  table->SetTableRange(0, 100);
  table->SetNumberOfColors(100);
  table->Build();
  table->SetTableValue(99, 0, 0, 0);

  vtkNew<vtkImageMapToColors> colorize;
  colorize->SetOutputFormatToRGB();
  colorize->SetLookupTable(table);
  colorize->SetInputConnection(cast->GetOutputPort());

  // Clear out results from previous runs of this test.
  const char* fileName = "TestMP4Writer.mp4";
  vtksys::SystemTools::RemoveFile(fileName);

  vtkNew<vtkMP4Writer> w;

  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    vtkLog(ERROR, "Could not determine temporary directory.");
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string outputFile = testDirectory + std::string("/") + std::string("fileName");

  w->SetInputConnection(colorize->GetOutputPort());
  w->SetFileName(outputFile.c_str());
  vtkLog(INFO, "Writing file " << outputFile);
  w->Start();
  for (int cc = 2; cc < 99; cc++)
  {
    fractal->SetMaximumNumberOfIterations(cc);
    table->SetTableRange(0, cc);
    table->SetNumberOfColors(cc);
    table->ForceBuild();
    table->SetTableValue(cc - 1, 0, 0, 0);
    w->Write();
  }
  w->End();
  vtkLog(INFO, "Done writing file '" << outputFile << "'");

  bool exists = (int)vtksys::SystemTools::FileExists(outputFile.c_str());
  unsigned long length = vtksys::SystemTools::FileLength(outputFile.c_str());
  bool success = true;
  if (!exists)
  {
    success = false;
    vtkLog(ERROR, "Test failing because file '" << outputFile.c_str() << "' doesn't exist...");
  }
  if (0 == length)
  {
    success = false;
    vtkLog(ERROR, "Test failing because file '" << outputFile.c_str() << "' has zero length...");
  }

  return success ? 0 : -1;
#else
  // The next two lines prevent an unused parameter warning
  (void)argc;
  (void)argv;

  return VTK_SKIP_RETURN_CODE;
#endif
}
