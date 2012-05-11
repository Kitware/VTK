/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImportExport.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageViewer.h"
#include "vtkImageReader.h"
#include "vtkImageImport.h"
#include "vtkImageExport.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPNMWriter.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

int ImportExport( int argc, char *argv[] )
{
  int i,j,k;

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkImageReader *reader = vtkImageReader::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0,63,0,63,1,93);
  reader->SetFilePrefix(fname);
  reader->SetDataMask(0x7fff);
  delete [] fname;

  // create exporter
  vtkImageExport *exporter = vtkImageExport::New();
  exporter->SetInputConnection(reader->GetOutputPort());
  exporter->ImageLowerLeftOn();

  // get info from exporter and create array to hold data
  int memsize = exporter->GetDataMemorySize();
  int *dimensions = exporter->GetDataDimensions();


  // export the data into the array
  short *data = new short[memsize/sizeof(short)];
  exporter->Export(data);

  // alternative method for getting data
  // short *data = exporter->GetPointerToData();

  // do a little something to the data

  for (i = 0; i < dimensions[2]; i++)
    {
    for (j = 0; j < dimensions[1]; j++)
      {
      for (k = 0; k < dimensions[0]; k++)
        {
        if (k % 10 == 0)
          {
          data[k + dimensions[0]*(j + dimensions[1]*i)] = 0;
          }
        if (j % 10 == 0)
          {
          data[k + dimensions[0]*(j + dimensions[1]*i)] = 1000;
          }
        }
      }
    }

  // create an importer to read the data back in
  vtkImageImport *importer = vtkImageImport::New();
  importer->SetWholeExtent(1,dimensions[0],1,dimensions[1],1,dimensions[2]);
  importer->SetDataExtentToWholeExtent();
  importer->SetDataScalarTypeToShort();
  importer->SetImportVoidPointer(data);
  importer->SetScalarArrayName("importedScalars");

  vtkImageViewer *viewer = vtkImageViewer::New();
  viewer->SetInputConnection(importer->GetOutputPort());
  viewer->SetZSlice(45);
  viewer->SetColorWindow(2000);
  viewer->SetColorLevel(1000);

  viewer->Render();

  int retVal = vtkRegressionTestImage( viewer->GetRenderWindow() );

  viewer->Delete();
  importer->Delete();
  exporter->Delete();
  reader->Delete();

  delete [] data;

  return !retVal;
}




