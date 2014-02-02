/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImportExport.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkImageImport and vtkImageExport
// .SECTION Description
//
#include "vtkSmartPointer.h"
#include "vtkImageImport.h"
#include "vtkImageExport.h"
#include "vtkImageCast.h"
#include "vtkImageData.h"
#include <vtkImageEllipsoidSource.h>
#include <vtkImageChangeInformation.h>

// Compare 2 vtk Images, return true if they are the same.
// 'Same' here implies contents and metadata values (spacing, origin) are equal.
bool compareVtkImages( vtkImageData* p1, vtkImageData* p2);

// Sub-tests.
int ImportExportWithPipeline(int argc, char* argv[]);
int ImportExportNoPipeline(int argc, char* argv[]);

//------------------------------------------------------------------------------

int TestImportExport(int argc, char* argv[])
{
  const int retval1 = ImportExportWithPipeline(argc,argv);
  std::cout << "ImportExportWithPipeline Finished. Exit code: "
            << retval1 << std::endl;

  const int retval2 = ImportExportNoPipeline(argc,argv);
  std::cout << "ImportExportNoPipeline Finished. Exit code: "
            << retval2 << std::endl;

  if ( (retval1 == EXIT_SUCCESS) && (retval2 == EXIT_SUCCESS) )
    {
    std::cout <<"Test Passed" << std::endl;
    return EXIT_SUCCESS;
    }

  std::cout <<"Test Failed" << std::endl;
  return EXIT_FAILURE;
}

//------------------------------------------------------------------------------

// Very basic wrapper for a pass through filter
// using vtkImageImport and vtkImageExport
// Constructs an importer and exporter, and connects them.
struct vtkToVtkImportExport
{
  vtkToVtkImportExport() :
    Exporter(vtkSmartPointer<vtkImageExport>::New()),
    Importer(vtkSmartPointer<vtkImageImport>::New())
  {
    // Connect the importer and exporter.
    Importer->SetBufferPointerCallback( Exporter->GetBufferPointerCallback() );
    Importer->SetDataExtentCallback( Exporter->GetDataExtentCallback() );
    Importer->SetNumberOfComponentsCallback( Exporter->GetNumberOfComponentsCallback() );
    Importer->SetOriginCallback( Exporter->GetOriginCallback() );
    Importer->SetPipelineModifiedCallback( Exporter->GetPipelineModifiedCallback() );
    Importer->SetPropagateUpdateExtentCallback( Exporter->GetPropagateUpdateExtentCallback() );
    Importer->SetScalarTypeCallback( Exporter->GetScalarTypeCallback() );
    Importer->SetSpacingCallback( Exporter->GetSpacingCallback() );
    Importer->SetUpdateDataCallback( Exporter->GetUpdateDataCallback() );
    Importer->SetUpdateInformationCallback( Exporter->GetUpdateInformationCallback() );
    Importer->SetWholeExtentCallback( Exporter->GetWholeExtentCallback() );
    Importer->SetCallbackUserData( reinterpret_cast<void*>(Exporter.GetPointer()) );
  }

  vtkSmartPointer<vtkImageExport> Exporter;
  vtkSmartPointer<vtkImageImport> Importer;
};

//------------------------------------------------------------------------------

// Test vtkImageImport and vtkImageExport
// A simple image source and pipeline is created upstream of the
// pass-through filter, and a simple cast filter is placed downstream.
// - create and update the pipeline, and check that input to the pass through
//   is the same as the output.
// - then modify an upstream filter, update the pipeline and check input and
//   has changed and that it matches the output.
int ImportExportWithPipeline( int vtkNotUsed(argc), char *vtkNotUsed(argv) [] )
{

  // Simple data source
  vtkSmartPointer<vtkImageEllipsoidSource> source =
      vtkSmartPointer<vtkImageEllipsoidSource>::New();
  source->SetOutputScalarTypeToUnsignedShort();
  source->SetInValue(1000);
  source->SetOutValue(0);
  source->SetCenter(20,20,20);
  source->SetRadius(9,10,11);
  source->SetWholeExtent(0, 14, 0, 29, 0, 49);

  // non default origin,spacing.
  vtkSmartPointer<vtkImageChangeInformation> changer =
      vtkSmartPointer<vtkImageChangeInformation>::New();
  changer->SetOutputOrigin(1,2,3);
  changer->SetOutputSpacing(4,5,6);
  changer->SetInputConnection(source->GetOutputPort());

  // create exporter & importer and connect them
  vtkToVtkImportExport importExport;

  // Get the exporter and connect it
  vtkSmartPointer<vtkImageExport> exporter = importExport.Exporter;

  exporter->SetInputConnection(changer->GetOutputPort());

  // Get the importer to read the data back in
  vtkSmartPointer<vtkImageImport> importer = importExport.Importer;

  // basic downstream pipeline.
  vtkSmartPointer<vtkImageCast> imCast = vtkSmartPointer<vtkImageCast>::New();
  imCast->SetOutputScalarTypeToUnsignedShort();
  imCast->SetInputConnection(importer->GetOutputPort());

  // Update the pipeline, get output.
  imCast->Update();
  vtkSmartPointer<vtkImageData> imageAfter = imCast->GetOutput();

  // Update source, get the image that was input to the exporter/import.
  changer->Update();
  vtkSmartPointer<vtkImageData> imageBefore = changer->GetOutput();

  std::cout << "Comparing up/down stream images after first update..."
            << std::endl;
  bool isSame = compareVtkImages(imageBefore, imageAfter);

  if (!isSame)
    {
    std::cout << "ERROR: Images are different" << std::endl;
    return EXIT_FAILURE;
    }

  source->SetInValue(99);
  source->SetOutValue(10);
  source->SetWholeExtent(0, 4, 0, 9, 0, 12);

  imCast->Update();
  imageAfter = imCast->GetOutput();

  changer->UpdateInformation();
  changer->Update();
  imageBefore = changer->GetOutput();

  std::cout << "Comparing up/down stream images after upstream change..."
            << std::endl;
  isSame = compareVtkImages(imageBefore, imageAfter);

  if (!isSame)
    {
    std::cout << "ERROR: Images are different" << std::endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

// Test the import / export using image data as the input (no pipeline).
// 3 input vtkImageData are created.
//   The vtkImageData that was created first is intentionally tested last
//   so that the MTime of the new input data is actually less.
// First confirm that input and output match after a pipeline update.
// Then switch to another input and confirm the input and output match
// after a pipeline update.
// Then switch to a third input and confirm the input and output match
// after a pipeline update.
int ImportExportNoPipeline( int vtkNotUsed(argc), char *vtkNotUsed(argv) [] )
{
  // Simple data source
  vtkSmartPointer<vtkImageEllipsoidSource> source =
      vtkSmartPointer<vtkImageEllipsoidSource>::New();
  source->SetOutputScalarTypeToUnsignedShort();
  source->SetInValue(1000);
  source->SetOutValue(0);
  source->SetCenter(20,20,20);
  source->SetRadius(9,10,11);
  source->SetWholeExtent(0, 14, 0, 29, 0, 49);

  // Filter to apply non default origin, spacing.
  vtkSmartPointer<vtkImageChangeInformation> changer =
      vtkSmartPointer<vtkImageChangeInformation>::New();
  changer->SetOutputOrigin(1,2,3);
  changer->SetOutputSpacing(4,5,6);
  changer->SetInputConnection(source->GetOutputPort());
  changer->Update();
  vtkSmartPointer<vtkImageData> imageBefore1 = changer->GetOutput();

  // Create an alternate input data (2).
  source->SetWholeExtent(0, 14, 0, 29, 0, 10);
  changer->SetOutputOrigin(2,4,3);
  changer->SetOutputSpacing(1,3,6);
  changer->Update();
  vtkSmartPointer<vtkImageData> imageBefore2 = changer->GetOutput();

  // Create an alternate input data (3).
  source->SetWholeExtent(0, 2, 0, 4, 0, 6);
  changer->SetOutputOrigin(9,8,7);
  changer->Update();
  vtkSmartPointer<vtkImageData> imageBefore3 = changer->GetOutput();

  // create exporter & importer and connect them
  vtkToVtkImportExport importExport;

  // Get the exporter and connect it
  vtkSmartPointer<vtkImageExport> exporter = importExport.Exporter;

  // Start with image 2 so we can go back to an image with lower mTime.
  exporter->SetInputData(imageBefore2);

  // Get the importer to read the data back in
  vtkSmartPointer<vtkImageImport> importer = importExport.Importer;

  importer->Update();

  vtkSmartPointer<vtkImageData> imageAfter = importer->GetOutput();

  std::cout << "Comparing up/down stream images after first update."
            << std::endl;
  bool isSame = compareVtkImages(imageBefore2, imageAfter);

  if (!isSame)
    {
    std::cout << "ERROR: Images are different" << std::endl;
    return EXIT_FAILURE;
    }

  // Switch input
  exporter->SetInputData(imageBefore3);
  importer->Update();
  imageAfter = importer->GetOutput();

  std::cout << "Comparing up/down stream images after change of input (1)."
            << std::endl;
  isSame = compareVtkImages(imageBefore3, imageAfter);

  if (!isSame)
    {
    std::cout << "ERROR: Images are different" << std::endl;
    return EXIT_FAILURE;
    }

  // Switch back to first Data.
  exporter->SetInputData(imageBefore1);
  importer->Update();
  imageAfter = importer->GetOutput();

  std::cout << "Comparing up/down stream images after change of input (2)."
            << std::endl;
  isSame = compareVtkImages(imageBefore1, imageAfter);

  if (!isSame)
    {
    std::cout << "ERROR: Images are different" << std::endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

// Utility to compare images.
bool compareVtkImages( vtkImageData* leftImg, vtkImageData* rightImg)
{
  if ( leftImg == rightImg )
    {
    std::cerr << "Got same pointers." << std::endl;
    return true;  // This also implies NULL == NULL is ok.
    }

  if (!leftImg)
    {
    std::cerr << "Left image is NULL" << std::endl;
    return false;
    }

  if (!rightImg)
    {
    std::cerr << "Right image is NULL" << std::endl;
    return false;
    }

  bool isSame = true;
  const int numComp = leftImg->GetNumberOfScalarComponents();
  if ( numComp != rightImg->GetNumberOfScalarComponents())
    {
    std::cerr << "Number of components differs" << std::endl;
    isSame = false;
    }

  double origin1[3], origin2[3];
  leftImg->GetOrigin(origin1);
  rightImg->GetOrigin(origin2);
  if (!std::equal(origin1, origin1+3, origin2))
    {
    std::cerr << "Origins are different" << std::endl;
    std::cerr << "Left: " << origin1[0] << "," << origin1[1] << ","
              << origin1[2] << std::endl;
    std::cerr << "Right: " << origin2[0] << "," << origin2[1] << ","
              << origin2[2] << std::endl;
    isSame = false;
    }

  double spacing1[3], spacing2[3];
  leftImg->GetSpacing(spacing1);
  rightImg->GetSpacing(spacing2);

  if (!std::equal(spacing1, spacing1+3, spacing2))
    {
    std::cerr << "Spacings are different" << std::endl;
    std::cerr << "Left: " << spacing1[0] << "," << spacing1[1] << ","
              << spacing1[2] << std::endl;
    std::cerr << "Right: " << spacing2[0] << "," << spacing2[1] << ","
              << spacing2[2] << std::endl;
    isSame = false;
    }

  int p1Extent[6], p2Extent[6];
  leftImg->GetExtent(p1Extent);
  rightImg->GetExtent(p2Extent);

  if (!std::equal(p1Extent, p1Extent+6, p2Extent))
    {
    std::cerr << "Extents are different" << std::endl;
    std::cerr << "Left: " << p1Extent[0] << "," << p1Extent[1] << ","
              << p1Extent[2] << "," << p1Extent[3] << ","
              << p1Extent[4] << "," << p1Extent[5] << std::endl;
    std::cerr << "Right: " << p2Extent[0] << "," << p2Extent[1] << ","
              << p2Extent[2] << "," << p2Extent[3] << ","
              << p2Extent[4] << "," << p2Extent[5] << std::endl;
    isSame = false;
    }


  const int p1ScalarType = leftImg->GetScalarType();
  const int p2ScalarType = rightImg->GetScalarType();
  if (p1ScalarType != p2ScalarType)
    {
    std::cerr << "Scalar types differ " << std::endl
              << "Left: " << leftImg->GetScalarTypeAsString() << " ("
              << p1ScalarType << ")" << std::endl
              << "Right: "<< rightImg->GetScalarTypeAsString() << " ("
              << p2ScalarType << ")" << std::endl;
    // Tolerate different types if the values (cast to double) are the same.
    // isSame = false;
    }

  if (!isSame)
    {
    // Cannot check pixel values because array size is different.
    return false;
    }

  // We know both extents are the same here.
  for (int k=p1Extent[4]; k<=p1Extent[5]; ++k)
    {
    for (int j=p1Extent[2]; j<=p1Extent[3]; ++j)
      {
      for (int i=p1Extent[0]; i<p1Extent[1]; ++i)
        {
        for (int c=0; c<numComp; ++c)
          {
          const double v1 = leftImg->GetScalarComponentAsDouble(i,j,k,c);
          const double v2 = rightImg->GetScalarComponentAsDouble(i,j,k,c);
          if (v1 != v2)
            {
            std::cerr << "Data value mismatch at"
                      << " i="<< i << " j=" << j << " k=" << k << " c=" << c
                      << std::endl
                      << "Left: " << v1
                      << " Right: " << v2;
            return false;
            }
          }
        }
      }
    }

  // OK if we got here.
  return true;
}
