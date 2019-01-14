/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAVIWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkAVIWriter
// .SECTION Description
//

#include "vtkAVIWriter.h"
#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkImageMapToColors.h"
#include "vtkLookupTable.h"
#include "vtksys/SystemTools.hxx"

int TestAVIWriter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int err = 0;
  int cc = 0;
  int exists = 0;
  unsigned long length = 0;
  vtkImageMandelbrotSource* Fractal0 = vtkImageMandelbrotSource::New();
  Fractal0->SetWholeExtent( 0, 247, 0, 247, 0, 0 );
  Fractal0->SetProjectionAxes( 0, 1, 2 );
  Fractal0->SetOriginCX( -1.75, -1.25, 0, 0 );
  Fractal0->SetSizeCX( 2.5, 2.5, 2, 1.5 );
  Fractal0->SetMaximumNumberOfIterations( 100);

  vtkImageCast* cast = vtkImageCast::New();
  cast->SetInputConnection(Fractal0->GetOutputPort());
  cast->SetOutputScalarTypeToUnsignedChar();

  vtkLookupTable* table = vtkLookupTable::New();
  table->SetTableRange(0, 100);
  table->SetNumberOfColors(100);
  table->Build();
  table->SetTableValue(99, 0, 0, 0);

  vtkImageMapToColors* colorize = vtkImageMapToColors::New();
  colorize->SetOutputFormatToRGB();
  colorize->SetLookupTable(table);
  colorize->SetInputConnection(cast->GetOutputPort());

  vtkAVIWriter *w = vtkAVIWriter::New();
  w->SetInputConnection(colorize->GetOutputPort());
  w->SetFileName("TestAVIWriter.avi");
  cout << "Writing file TestAVIWriter.avi..." << endl;
  w->Start();
  for ( cc = 2; cc < 99; cc ++ )
  {
    cout << ".";
    Fractal0->SetMaximumNumberOfIterations(cc);
    table->SetTableRange(0, cc);
    table->SetNumberOfColors(cc);
    table->ForceBuild();
    table->SetTableValue(cc-1, 0, 0, 0);
    w->Write();
  }
  w->End();
  cout << endl;
  cout << "Done writing file TestAVIWriter.avi..." << endl;
  w->Delete();

  exists = (int) vtksys::SystemTools::FileExists("TestAVIWriter.avi");
  length = vtksys::SystemTools::FileLength("TestAVIWriter.avi");
  cout << "TestAVIWriter.avi file exists: " << exists << endl;
  cout << "TestAVIWriter.avi file length: " << length << endl;
  if (!exists)
  {
    err = 3;
    cerr << "ERROR: 3 - Test failing because TestAVIWriter.avi file doesn't exist..." << endl;
  }
  if (0==length)
  {
    err = 4;
    cerr << "ERROR: 4 - Test failing because TestAVIWriter.avi file has zero length..." << endl;
  }

  colorize->Delete();
  table->Delete();
  cast->Delete();
  Fractal0->Delete();

  // err == 0 means test passes...
  //
  return err;
}
