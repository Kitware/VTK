#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkOMETIFFReader.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkVector.h>
#include <vtkVectorOperators.h>
#include <vtksys/RegularExpression.hxx>

#include <cstdlib>

int TestOMETIFFReader(int argc, char* argv[])
{
  std::string data;
  vtkVector3i size(0);
  vtkVector3d physicalSize(0.0);
  int sizeC = 0;
  int sizeT = 0;

  vtksys::RegularExpression regex("^([^x]+)x([^x]+)x([^x]+)$");
  for (int cc = 1; (cc + 1) < argc; cc++)
  {
    if (strcmp(argv[cc], "--data") == 0)
    {
      data = argv[++cc];
    }
    else if (strcmp(argv[cc], "--size") == 0)
    {
      if (regex.find(argv[++cc]))
      {
        size[0] = std::atoi(regex.match(1).c_str());
        size[1] = std::atoi(regex.match(2).c_str());
        size[2] = std::atoi(regex.match(3).c_str());
      }
    }
    else if (strcmp(argv[cc], "--physical-size") == 0)
    {
      if (regex.find(argv[++cc]))
      {
        physicalSize[0] = std::atof(regex.match(1).c_str());
        physicalSize[1] = std::atof(regex.match(2).c_str());
        physicalSize[2] = std::atof(regex.match(3).c_str());
      }
    }
    else if (strcmp(argv[cc], "--size_c") == 0)
    {
      sizeC = std::atoi(argv[++cc]);
    }
    else if (strcmp(argv[cc], "--size_t") == 0)
    {
      sizeT = std::atoi(argv[++cc]);
    }
  }

  vtkNew<vtkOMETIFFReader> reader;
  reader->SetFileName(data.c_str());
  reader->UpdateInformation();

  auto outInfo = reader->GetOutputInformation(0);
  if (sizeT >= 1 && outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) &&
    outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) == sizeT)
  {
    // verified timesteps.
  }
  else
  {
    vtkLogF(ERROR, "Failed to read timesteps; expected (%d), got (%d)", sizeT,
      outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()));
  }

  reader->Update();
  auto img = reader->GetOutput();
  if (img->GetPointData()->GetNumberOfArrays() != sizeC)
  {
    vtkLogF(ERROR, "Failed to read channels; expected (%d), got (%d)", sizeC,
      img->GetPointData()->GetNumberOfArrays());
  }

  vtkVector3i dims;
  img->GetDimensions(dims.GetData());
  if (dims != size)
  {
    vtkLogF(ERROR, "Failed due to size mismatch; expected (%d, %d, %d), got (%d, %d, %d)", size[0],
      size[1], size[2], dims[0], dims[1], dims[2]);
  }

  vtkVector3d spacing;
  img->GetSpacing(spacing.GetData());
  if ((spacing - physicalSize).Norm() > 0.00001)
  {
    vtkLogF(ERROR, "Physical size / spacing mismatch; expected (%f, %f, %f), got (%f, %f, %f)",
      physicalSize[0], physicalSize[1], physicalSize[2], spacing[0], spacing[1], spacing[2]);
  }

  // now read in multiple pieces.
  for (int cc = 0; cc < 4; ++cc)
  {
    reader->Modified();
    reader->UpdatePiece(cc, 4, 0);
  }

  return EXIT_SUCCESS;
}
