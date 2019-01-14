#include <vtkSmartPointer.h>
#include <vtkSampleFunction.h>
#include <vtkSphere.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

#include <vtkRTAnalyticSource.h>


#include "vtkTestErrorObserver.h"

int TestSampleFunction(int, char *[])
{
  // First test for errors and warniongs
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkSmartPointer<vtkSampleFunction> sf1 =
    vtkSmartPointer<vtkSampleFunction>::New();
  sf1->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  sf1->SetModelBounds(1, -1, 0, 1, 0, 1);

  // Check for model bounds error
  if (errorObserver->GetError())
  {
    std::cout << "Caught expected error: "
              << errorObserver->GetErrorMessage();
  }
  else
  {
    std::cout << "Failed to catch expected error regarding model bounds" << std::endl;
    return EXIT_FAILURE;
  }
  errorObserver->Clear();

  // Check for missing implicit function error
  sf1->Update();
  if (errorObserver->GetError())
  {
    std::cout << "Caught expected error: "
              << errorObserver->GetErrorMessage();
  }
  else
  {
    std::cout << "Failed to catch expected error regarding missing implicit function" << std::endl;
    return EXIT_FAILURE;
  }

  sf1->Print(std::cout);

  vtkSmartPointer<vtkSphere> sphere =
    vtkSmartPointer<vtkSphere>::New();

  double value = 2.0;
  double xmin = -value, xmax = value,
    ymin = -value, ymax = value,
    zmin = -value, zmax = value;

  double bounds[6];
  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;
  vtkSmartPointer<vtkSampleFunction> sf2 =
    vtkSmartPointer<vtkSampleFunction>::New();
  sf2->SetSampleDimensions(50,50,50);
  sf2->SetImplicitFunction(sphere);
  sf2->SetModelBounds(bounds);
  sf2->ComputeNormalsOn();
  sf2->Update();
  sf2->Print(std::cout);

  vtkSmartPointer<vtkSampleFunction> sf3 =
    vtkSmartPointer<vtkSampleFunction>::New();
  sf3->SetSampleDimensions(51,52,1);
  sf3->SetImplicitFunction(sphere);
  sf3->SetModelBounds(xmin, xmax, ymin, ymax, zmin, zmax);
  sf3->CappingOn();
  sf3->SetCapValue(1000);
  sf3->SetScalarArrayName("sphereScalars");
  sf3->SetNormalArrayName("sphereNormals");
  sf3->ComputeNormalsOff();
  sf3->Update();

  // Test ability to process a subset of the data (update extent)
  int dims[] = {10,17,37};
  int extent[] = {2,8, 3,13, 25,30};

  vtkSmartPointer<vtkSampleFunction> sf4 =
    vtkSmartPointer<vtkSampleFunction>::New();
  sf4->SetSampleDimensions(dims);
  sf4->SetImplicitFunction(sphere);
  sf4->SetModelBounds(xmin, xmax, ymin, ymax, zmin, zmax);
  sf4->ComputeNormalsOn();
  sf4->UpdateInformation();
  sf4->Update();

  vtkDataArray *sa = sf4->GetOutput()->GetPointData()->GetScalars();
  double *s4 = static_cast<double*>(sa->GetVoidPointer(0));

  vtkSmartPointer<vtkSampleFunction> sf5 =
    vtkSmartPointer<vtkSampleFunction>::New();
  sf5->SetSampleDimensions(dims);
  sf5->SetImplicitFunction(sphere);
  sf5->SetModelBounds(xmin, xmax, ymin, ymax, zmin, zmax);
  sf5->ComputeNormalsOn();
  sf5->UpdateExtent(extent);

  sa = sf5->GetOutput()->GetPointData()->GetScalars();
  double *s5 = static_cast<double*>(sa->GetVoidPointer(0));

  // Now ensure that within the extent the difference between
  // the data is zero.
  double sWE, sUE;
  int i, j, k;
  double *sPtr = s5;
  for (k=extent[4]; k<=extent[5]; ++k)
  {
    for (j=extent[2]; j<=extent[3]; ++j)
    {
      for (i=extent[0]; i<=extent[1]; ++i)
      {
        sWE = *(s4 + i + j*dims[0] + k*dims[0]*dims[1]);
        sUE = *sPtr++;
        if ( (sWE-sUE) != 0.0 )
        {
          std::cout << "Inconsistent update extent computation" << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
  }

  // Now exercise the Set/Get methods
  int dimensions[3];
  sf3->GetSampleDimensions(dimensions);
  std::cout << "Dimensions: "
            << dimensions[0] << ", "
            << dimensions[1] << ", "
            << dimensions[2] << std::endl;
  sf3->GetModelBounds(bounds);
  std::cout << "ModelBounds: "
            << bounds[0] << ", "
            << bounds[1] << ", "
            << bounds[2] << ", "
            << bounds[3] << ", "
            << bounds[4] << ", "
            << bounds[5] << std::endl;
  std::cout << "ImplicitFunction: " << sf3->GetImplicitFunction() << std::endl;
  std::cout << "Capping: " << sf3->GetCapping() << std::endl;
  std::cout << "CapValue: " << sf3->GetCapValue() << std::endl;
  std::cout << "ComputeNormals: " << sf3->GetComputeNormals() << std::endl;

  std::cout << "ScalarArrayName: " << sf3->GetScalarArrayName() << std::endl;
  std::cout << "NormalArrayName: " << sf3->GetNormalArrayName() << std::endl;

  std::cout << "Default OutputScalarType: " << sf3->GetOutputScalarType() << std::endl;
  sf3->SetOutputScalarTypeToDouble();
  std::cout << "After SetOutputScalarTypeToDouble, OutputScalarType: " << sf3->GetOutputScalarType() << std::endl;
  sf3->SetOutputScalarTypeToFloat();
  std::cout << "After SetOutputScalarTypeToFloat, OutputScalarType: " << sf3->GetOutputScalarType() << std::endl;
  sf3->SetOutputScalarTypeToLong();
  std::cout << "After SetOutputScalarTypeToLong, OutputScalarType: " << sf3->GetOutputScalarType() << std::endl;
  sf3->SetOutputScalarTypeToUnsignedLong();
  std::cout << "After SetOutputScalarTypeToUnsignedLong, OutputScalarType: " << sf3->GetOutputScalarType() << std::endl;
  sf3->SetOutputScalarTypeToInt();
  std::cout << "After SetOutputScalarTypeToInt, OutputScalarType: " << sf3->GetOutputScalarType() << std::endl;
  sf3->SetOutputScalarTypeToUnsignedInt();
  std::cout << "After SetOutputScalarTypeToUnsignedInt, OutputScalarType: " << sf3->GetOutputScalarType() << std::endl;
  sf3->SetOutputScalarTypeToShort();
  std::cout << "After SetOutputScalarTypeToShort, OutputScalarType: " << sf3->GetOutputScalarType() << std::endl;
  sf3->SetOutputScalarTypeToUnsignedShort();
  std::cout << "After SetOutputScalarTypeToUnsignedShort, OutputScalarType: " << sf3->GetOutputScalarType() << std::endl;
  sf3->SetOutputScalarTypeToChar();
  std::cout << "After SetOutputScalarTypeToChar, OutputScalarType: " << sf3->GetOutputScalarType() << std::endl;
  sf3->SetOutputScalarTypeToUnsignedChar();
  std::cout << "After SetOutputScalarTypeToUnsignedChar, OutputScalarType: " << sf3->GetOutputScalarType() << std::endl;
  return EXIT_SUCCESS;
}
