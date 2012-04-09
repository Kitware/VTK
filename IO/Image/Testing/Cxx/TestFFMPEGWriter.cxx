/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFFMPEGWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestFFMPEGWriter - Tests vtkFFMPEGWriter.
// .SECTION Description
// Creates a scene and uses FFMPEGWriter to generate a movie file. Test passes
// if the file exists and has non zero length.


#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkImageMapToColors.h"
#include "vtkLookupTable.h"
#include "vtkFFMPEGWriter.h"
#include "vtksys/SystemTools.hxx"

int TestFFMPEGWriter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
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

  vtkFFMPEGWriter *w = vtkFFMPEGWriter::New();
  w->SetInputConnection(colorize->GetOutputPort());
  w->SetFileName("TestFFMPEGWriter.avi");
  cout << "Writing file TestFFMPEGWriter.avi..." << endl;
  w->SetBitRate(1024*1024*30);
  w->SetBitRateTolerance(1024*1024*3);
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
  cout << "Done writing file TestFFMPEGWriter.avi..." << endl;
  w->Delete();

  exists = (int) vtksys::SystemTools::FileExists("TestFFMPEGWriter.avi");
  length = vtksys::SystemTools::FileLength("TestFFMPEGWriter.avi");
  cout << "TestFFMPEGWriter.avi file exists: " << exists << endl;
  cout << "TestFFMPEGWriter.avi file length: " << length << endl;
  if (!exists)
    {
    err = 1;
    cerr << "ERROR: 1 - Test failing because TestFFMPEGWriter.avi file doesn't exist..." << endl;
    }
  else
    {
    vtksys::SystemTools::RemoveFile("TestFFMPEGWriter.avi");
    }
  if (0==length)
    {
    err = 2;
    cerr << "ERROR: 2 - Test failing because TestFFMPEGWriter.avi file has zero length..." << endl;
    }

  colorize->Delete();
  table->Delete();
  cast->Delete();
  Fractal0->Delete();

  // err == 0 means test passes...
  //
  return err;
}
