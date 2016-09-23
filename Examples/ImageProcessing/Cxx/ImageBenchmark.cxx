/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageBenchmark.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This program provides benchmarking of several VTK imaging filters.
// See the help text below for instructions on running this program.

#include "vtkFloatArray.h"
#include "vtkImageBSplineCoefficients.h"
#include "vtkImageBSplineInterpolator.h"
#include "vtkImageCast.h"
#include "vtkImageChangeInformation.h"
#include "vtkImageClip.h"
#include "vtkImageConvolve.h"
#include "vtkImageData.h"
#include "vtkImageFFT.h"
#include "vtkImageGaussianSmooth.h"
#include "vtkImageGaussianSource.h"
#include "vtkImageGridSource.h"
#include "vtkImageHistogram.h"
#include "vtkImageHistogramStatistics.h"
#include "vtkImageInterpolator.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkImageMapToColors.h"
#include "vtkImageMedian3D.h"
#include "vtkImageNoiseSource.h"
#include "vtkImageResize.h"
#include "vtkImageReslice.h"
#include "vtkImageSeparableConvolution.h"
#include "vtkImageShiftScale.h"
#include "vtkImageSincInterpolator.h"
#include "vtkImageStencilData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLookupTable.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiThreader.h"
#include "vtkPointData.h"
#include "vtkPNGWriter.h"
#include "vtkROIStencilSource.h"
#include "vtkSmartPointer.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkVersion.h"

#include <vtksys/Process.h>

#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>

// By default, the entry point is "main" (stand-alone program)
#ifndef IMAGE_BENCHMARK_MAIN
#define IMAGE_BENCHMARK_MAIN main
#endif

const char *HelpText =
"Usage: ImageBenchmark [options]\n"
"\n"
"Options:\n"
"  --runs N                      The number of runs to perform\n"
"  --threads N (or N-M or N,M,O) Request a certain number of threads\n"
"  --split-mode slab|beam|block  Use the specified splitting mode\n"
"  --enable-smp on|off           Use vtkSMPTools vs. vtkMultiThreader\n"
"  --clear-cache MBytes          Attempt to clear CPU cache between runs\n"
"  --bytes-per-piece N           Ask for N bytes per piece [65536]\n"
"  --min-piece-size XxYxZ        Minimum dimensions per piece [16x1x1]\n"
"  --size XxYxZ                  The image size [256x256x256]\n"
"  --type uchar|short|float      The data type for the input [short]\n"
"  --source <source>             Set the data source [gaussian]\n"
"  --filter <filter>[:options]   Set the filter to benchmark [all]\n"
"  --output filename.png         Output middle slice as a png file.\n"
"  --units mvps|mvptps|seconds   The output units (see below for details).\n"
"  --header                      Print a header line before the results.\n"
"  --verbose                     Print verbose output to stdout.\n"
"  --version                     Print the VTK version and exit.\n"
"  --help                        Print this message.\n"
"\n"
"This program prints benchmark results to stdout in csv format.  The default\n"
"units are megavoxels per second, but the --units option can specify units\n"
"of seconds, megavoxels per second (mvps), or megavoxels per thread per\n"
"second (mvptps).\n"
"\n"
"If more than three runs are done (by use of --runs), then the mean and\n"
"standard deviation over all of the runs except the first will be printed\n"
"(use --header to get the column headings).\n"
"\n"
"Sources: these are how the initial data set is produced.\n"
"  gaussian    A centered 3D gaussian.\n"
"  noise       Pseudo-random noise.\n"
"  grid        A grid, for checking rotations.\n"
"  mandelbrot  The mandelbrot set.\n"
"\n"
"Filters: these are the algorithms that can be benchmarked.\n"
"  median:kernelsize=3        Test vtkImageMedian3D.\n"
"  reslice:kernel=nearest     Test vtkImageReslice (see below).\n"
"  resize:kernelsize=1        Test vtkImageResize.\n"
"  convolve:kernelsize=3      Test vtkImageConvolve.\n"
"  separable:kernelsize=3     Test vtkImageSeparableConvolution.\n"
"  gaussian:kernelsize=3      Test vtkImageGaussianSmooth.\n"
"  bspline:degree=3           Test vtkImageBSplineCoefficients.\n"
"  fft                        Test vtkImageFFT.\n"
"  histogram:stencil          Test vtkImageHistogram.\n"
"  colormap:components=3      Test vtkImageMapToColors.\n"
"\n"
"The reslice filter takes the following options:\n"
"  stencil                    Spherical stencil (ignore voxels outside).\n"
"  kernel=nearest|linear|cubic|sinc|bspline   The interpolator to use.\n"
"  kernelsize=4               The kernelsize (sinc, bspline only).\n"
"  rotation=0/0/0/0           Rotation angle (degrees) and axis.\n"
"\n"
"The colormap filter takes the following options:\n"
"  components=3               Output components (3=RGB, 4=RGBA).\n"
"  greyscale                  Rescale but do not apply a vtkLookupTable.\n"
"\n";

const char *DefaultFilters[] =
{
  "colormap:components=3",
  "colormap:components=4",
  "colormap:components=1:greyscale",
  "colormap:components=2:greyscale",
  "colormap:components=3:greyscale",
  "colormap:components=4:greyscale",

  "resize:kernelsize=1",
  "resize:kernelsize=2",
  "resize:kernelsize=4",
  "resize:kernelsize=6",

  "reslice:kernel=nearest:rotation=0/0/0/1",
  "reslice:kernel=nearest:rotation=90/0/0/1",
  "reslice:kernel=nearest:rotation=90/0/1/0",
  "reslice:kernel=nearest:rotation=45/0/0/1",
  "reslice:kernel=nearest:rotation=60/0/1/1",

  "reslice:kernel=linear:rotation=60/0/1/1",
  "reslice:kernel=cubic:rotation=60/0/1/1",
  "reslice:kernel=bspline:rotation=60/0/1/1",
  "reslice:kernel=sinc:rotation=60/0/1/1",
  "reslice:kernel=sinc:rotation=60/0/1/1:stencil",

  "gaussian:kernelsize=3",
  "convolve:kernelsize=3",
  "separable:kernelsize=3",
  "resize:kernelsize=3",
  "median:kernelsize=3",

  "histogram",
  "histogram:stencil",
  "bspline:degree=3",

  NULL
};

bool Verbose = false;

// attempt to clear the CPU cache
static VTK_THREAD_RETURN_TYPE ClearOneCPUCache(void *arg)
{
  size_t cacheSize = *static_cast<size_t *>(
    static_cast<vtkMultiThreader::ThreadInfo *>(arg)->UserData);
  size_t n = cacheSize/4;
  // allocate a cache-sized chunk of memory
  unsigned int *bigmem = new unsigned int[n];
  // write random numbers to this memory
  unsigned int randNum = 1919872345;
  for (size_t i = 0; i < n; i++)
  {
    randNum = 1664525*randNum + 1013904223;
    bigmem[i] = randNum;
  }
  delete [] bigmem;

  return VTK_THREAD_RETURN_VALUE;
}

// attempt to clear all CPU caches on a multi-CPU machine
static void ClearCPUCache(size_t cacheSize)
{
  vtkSmartPointer<vtkMultiThreader> threader =
    vtkSmartPointer<vtkMultiThreader>::New();
  threader->SetSingleMethod(ClearOneCPUCache, &cacheSize);
  threader->SingleMethodExecute();
}

// verify that everything is set the way that we expect
static void PrintInfo(vtkThreadedImageAlgorithm *filter, std::ostream& os)
{
  os << "EnableSMP: " << filter->GetEnableSMP() << "\n";
  os << "NumberOfThreads: " << (filter->GetEnableSMP() ?
    vtkSMPTools::GetEstimatedNumberOfThreads() :
    filter->GetNumberOfThreads()) << "\n";
  vtkImageData *data = vtkImageData::SafeDownCast(filter->GetInput());
  os << "ScalarType: "
     << data->GetPointData()->GetScalars()->GetDataTypeAsString() << "\n";
  int *imsize = data->GetDimensions();
  os << "Dimensions: " << imsize[0] << "," << imsize[1] << "," << imsize[2]
     << "\n";
  int smode = filter->GetSplitMode();
  os << "SplitMode: "
     << (smode == 0 ? "Slab" : (smode == 1 ? "Beam" : "Block")) << "\n";
  os << "DesiredBytesPerPiece: " << filter->GetDesiredBytesPerPiece() << "\n";
  imsize = filter->GetMinimumPieceSize();
  os << "MinimumPieceSize: " << imsize[0] << "," << imsize[1] << ","
     << imsize[2] << "\n";
  os << "ClassName: " << filter->GetClassName() << "\n";
  vtkImageMedian3D *median = vtkImageMedian3D::SafeDownCast(filter);
  if (median)
  {
    imsize = median->GetKernelSize();
    os << "KernelSize: " << imsize[0] << "," << imsize[1] << "," << imsize[2]
       << "\n";
  }
  vtkImageReslice *reslice = vtkImageReslice::SafeDownCast(filter);
  if (reslice)
  {
    os << "Stencil: " << reslice->GetStencil() << "\n";
    vtkAbstractImageInterpolator *interp = reslice->GetInterpolator();
    if (vtkImageInterpolator::SafeDownCast(interp))
    {
      os << "InterpolationMode: "
         << reslice->GetInterpolationModeAsString() << "\n";
    }
    else
    {
      os << "Interpolator: " << interp->GetClassName() << "\n";
      vtkImageBSplineInterpolator *bspline =
        vtkImageBSplineInterpolator::SafeDownCast(interp);
      if (bspline)
      {
        os << "SplineDegree: " << bspline->GetSplineDegree() << "\n";
      }
      vtkImageSincInterpolator *sinc =
        vtkImageSincInterpolator::SafeDownCast(interp);
      if (sinc)
      {
        os << "WindowHalfWidth: " << sinc->GetWindowHalfWidth() << "\n";
      }
    }
    os << "ResliceAxes:";
    double axes[16] = { 1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0,
                        0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0 };
    if (reslice->GetResliceAxes())
    {
      vtkMatrix4x4::DeepCopy(axes, reslice->GetResliceAxes());
    }
    for (int k = 0; k < 16; k++)
    {
      os << ((k % 4 == 0) ? " " : "") << axes[k]
                << (k != 15 ? "," : "\n");
    }
  }
  vtkImageResize *resize = vtkImageResize::SafeDownCast(filter);
  if (resize)
  {
    vtkAbstractImageInterpolator *interp = resize->GetInterpolator();
    os << "Interpolate: " << resize->GetInterpolate() << "\n";
    if (interp)
    {
      os << "Interpolator: " << interp->GetClassName() << "\n";
      vtkImageBSplineInterpolator *bspline =
        vtkImageBSplineInterpolator::SafeDownCast(interp);
      if (bspline)
      {
        os << "SplineDegree: " << bspline->GetSplineDegree() << "\n";
      }
      vtkImageSincInterpolator *sinc =
        vtkImageSincInterpolator::SafeDownCast(interp);
      if (sinc)
      {
        os << "WindowHalfWidth: " << sinc->GetWindowHalfWidth() << "\n";
      }
    }
  }
  vtkImageConvolve *convolve = vtkImageConvolve::SafeDownCast(filter);
  if (convolve)
  {
    imsize = convolve->GetKernelSize();
    os << "KernelSize: " << imsize[0] << "," << imsize[1] << "," << imsize[2]
       << "\n";
  }
  vtkImageSeparableConvolution *separable =
    vtkImageSeparableConvolution::SafeDownCast(filter);
  if (separable)
  {
    os << "XKernel: " << separable->GetXKernel()->GetNumberOfTuples() << "\n";
    os << "YKernel: " << separable->GetYKernel()->GetNumberOfTuples() << "\n";
    os << "ZKernel: " << separable->GetZKernel()->GetNumberOfTuples() << "\n";
  }
  vtkImageGaussianSmooth *gaussian =
    vtkImageGaussianSmooth::SafeDownCast(filter);
  if (gaussian)
  {
    double *f = gaussian->GetStandardDeviations();
    os << "StandardDeviations: " << f[0] << "," << f[1] << "," << f[2] << "\n";
    f = gaussian->GetRadiusFactors();
    os << "RadiusFactors: " << f[0] << "," << f[1] << "," << f[2] << "\n";
  }
  vtkImageMapToColors *colors = vtkImageMapToColors::SafeDownCast(filter);
  if (colors)
  {
    os << "LookupTable: "
       << vtkLookupTable::SafeDownCast(colors->GetLookupTable()) << "\n";
    os << "OutputFormat: " << colors->GetOutputFormat() << "\n";
  }
  vtkImageBSplineCoefficients *bspline =
    vtkImageBSplineCoefficients::SafeDownCast(filter);
  if (bspline)
  {
    os << "SplineDegree: " << bspline->GetSplineDegree() << "\n";
  }
  vtkImageHistogram *histogram = vtkImageHistogram::SafeDownCast(filter);
  if (histogram)
  {
    os << "Stencil : " << histogram->GetStencil() << "\n";
  }
}
// create the filter that will be benchmarked
static vtkThreadedImageAlgorithm *CreateFilter(
  const std::string& filterName, const int size[3])
{
  // filter name may be followed by colon and comma-separated args
  std::vector<std::string> args;
  size_t pos = filterName.find(':');
  if (pos == std::string::npos)
  {
    args.push_back(filterName);
  }
  else
  {
    args.push_back(filterName.substr(0, pos));
    do
    {
      pos++;
      size_t endpos = filterName.find(':', pos);
      args.push_back(filterName.substr(pos, endpos - pos));
      pos = endpos;
    }
    while (pos != std::string::npos);
  }

  //-----
  // all the available filters and their options
  //-----
  if (args[0] == "median")
  {
    vtkSmartPointer<vtkImageMedian3D> filter =
      vtkSmartPointer<vtkImageMedian3D>::New();

    int kernelsize = 3;

    if (args.size() > 1)
    {
      size_t n = args[1].find('=');
      if (args[1].substr(0, n) != "kernelsize" ||
          n == std::string::npos || n + 1 == args[1].size() ||
          args[1][n+1] < '1' || args[1][n+1] > '9' ||
          args.size() > 2)
      {
        std::cerr << "median options: kernelsize=N\n";
        return NULL;
      }
      std::string num = args[1].substr(n+1);
      kernelsize = std::atoi(num.c_str());
    }

    filter->SetKernelSize(kernelsize, kernelsize, kernelsize);
    filter->Register(NULL);
    return filter;
  }
  //-----
  else if (args[0] == "reslice")
  {
    vtkSmartPointer<vtkImageReslice> filter =
      vtkSmartPointer<vtkImageReslice>::New();

    bool mask = false;
    std::string kernel;
    int kernelsize = 0;
    double rotation[4] = { 0.0, 0.0, 0.0, 0.0 };

    for (size_t k = 1; k < args.size(); k++)
    {
      size_t n = args[k].find('=');
      std::string key = args[k].substr(0, n);
      if (key == "stencil")
      {
        if (n != std::string::npos)
        {
          std::cerr << "reslice stencil option takes no args\n";
          return NULL;
        }
        mask = true;
      }
      else if (key == "kernel")
      {
        if (n == std::string::npos || n + 1 == args[k].size())
        {
          std::cerr << "reslice kernel should be kernel=name\n";
          return NULL;
        }
        kernel = args[k].substr(n+1);
      }
      else if (key == "kernelsize")
      {
        if (n == std::string::npos || n + 1 == args[k].size() ||
            args[k][n+1] < '0' || args[k][n+1] > '9')
        {
          std::cerr << "reslice kernelsize should be kernelsize=N\n";
          return NULL;
        }
        std::string num = args[k].substr(n+1);
        kernelsize = std::atoi(num.c_str());
        if (kernelsize < 1 || kernelsize > 10)
        {
          std::cerr << "reslice kernelsize must be between 1 and 10\n";
          return NULL;
        }
      }
      else if (key == "rotation")
      {
        int j = 0;
        while (n != std::string::npos && j < 4)
        {
          n++;
          size_t endpos = args[k].find('/', n);
          std::string num = args[k].substr(n, endpos-n);
          rotation[j++] = atof(num.c_str());
          n = endpos;
        }
        if (n != std::string::npos || j != 4)
        {
          std::cerr << "reslice rotation format: rotation=degrees/x/y/z\n";
          return NULL;
        }
      }
      else
      {
        std::cerr << "reslice does not take option " << key << "\n";
        return NULL;
      }
    }

    // create a spherical mask (or circular for 2D)
    if (mask)
    {
      vtkSmartPointer<vtkROIStencilSource> stencil =
        vtkSmartPointer<vtkROIStencilSource>::New();
      stencil->SetOutputWholeExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);
      if (size[2] == 1)
      {
        stencil->SetShapeToCylinderZ();
      }
      else if (size[1] == 1)
      {
        stencil->SetShapeToCylinderY();
      }
      else if (size[0] == 1)
      {
        stencil->SetShapeToCylinderX();
      }
      else
      {
        stencil->SetShapeToEllipsoid();
      }
      stencil->SetBounds(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);
      stencil->Update();
      filter->SetStencilData(stencil->GetOutput());
    }

    // if kernel not set but kernelsize > 1, default to bspline
    if (kernelsize > 1 && kernel.empty())
    {
      kernel = "bspline";
    }

    if (kernel == "bspline")
    {
      if (kernelsize == 0)
      {
        kernelsize = 4;
      }
      vtkSmartPointer<vtkImageBSplineInterpolator> interpolator =
        vtkSmartPointer<vtkImageBSplineInterpolator>::New();
      interpolator->SetSplineDegree(kernelsize - 1);
      filter->SetInterpolator(interpolator);
    }
    else if (kernel == "sinc")
    {
      if (kernelsize % 2 != 0)
      {
        std::cerr << "reslice sinc kernelsize must be even\n";
        return NULL;
      }
      if (kernelsize == 0)
      {
        kernelsize = 6;
      }
      vtkSmartPointer<vtkImageSincInterpolator> interpolator =
        vtkSmartPointer<vtkImageSincInterpolator>::New();
      interpolator->SetWindowHalfWidth(kernelsize/2);
      filter->SetInterpolator(interpolator);
    }
    else if (kernel == "cubic")
    {
      filter->SetInterpolationModeToCubic();
    }
    else if (kernel == "linear")
    {
      filter->SetInterpolationModeToLinear();
    }
    else if (kernel != "nearest" && !kernel.empty())
    {
      std::cerr << "reslice kernel " << kernel << " not recognized\n";
      return NULL;
    }

    // create the transform
    if (rotation[1] != 0.0 || rotation[2] != 0.0 || rotation[3] != 0.0)
    {
      vtkSmartPointer<vtkTransform> transform =
        vtkSmartPointer<vtkTransform>::New();
      transform->RotateWXYZ(rotation[0], rotation[1], rotation[2], rotation[3]);
      vtkSmartPointer<vtkMatrix4x4> matrix =
        vtkSmartPointer<vtkMatrix4x4>::New();
      matrix->DeepCopy(transform->GetMatrix());
      for (int i = 0; i < 4; i++)
      {
        for (int j = 0; j < 4; j++)
        {
          // clean up numerical error for pure 90 degree rotations
          double x = matrix->GetElement(i, j);
          if (fabs(x) < 1e-12)
          {
            x = 0.0;
          }
          else if (fabs(1.0 - x) < 1e-12)
          {
            x = 1.0;
          }
          else if (fabs(1.0 + x) < 1e-12)
          {
            x = -1.0;
          }
          matrix->SetElement(i, j, x);
        }
      }
      filter->SetResliceAxes(matrix);
    }

    filter->SetOutputExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);

    filter->Register(NULL);
    return filter;
  }
  //-----
  else if (args[0] == "resize")
  {
    vtkSmartPointer<vtkImageResize> filter =
      vtkSmartPointer<vtkImageResize>::New();

    int kernelsize = 1;

    for (size_t k = 1; k < args.size(); k++)
    {
      size_t n = args[k].find('=');
      std::string key = args[k].substr(0, n);
      if (key == "kernelsize")
      {
        if (n == std::string::npos || n + 1 == args[k].size() ||
            args[k][n+1] < '0' || args[k][n+1] > '9')
        {
          std::cerr << "resize kernelsize should be kernelsize=N\n";
          return NULL;
        }
        std::string num = args[k].substr(n+1);
        kernelsize = std::atoi(num.c_str());
        if (kernelsize < 1 || kernelsize > 10)
        {
          std::cerr << "resize kernelsize must be between 1 and 10\n";
          return NULL;
        }
      }
      else
      {
        std::cerr << "resize does not take option " << key << "\n";
        return NULL;
      }
    }

    if (kernelsize > 1)
    {
      vtkSmartPointer<vtkImageBSplineInterpolator> interpolator =
        vtkSmartPointer<vtkImageBSplineInterpolator>::New();
      interpolator->SetSplineDegree(kernelsize - 1);
      filter->SetInterpolator(interpolator);
    }
    else
    {
      filter->InterpolateOff();
    }

    filter->Register(NULL);
    return filter;
  }
  //-----
  else if (args[0] == "convolve")
  {
    vtkSmartPointer<vtkImageConvolve> filter =
      vtkSmartPointer<vtkImageConvolve>::New();

    int kernelsize = 3;

    if (args.size() > 1)
    {
      size_t n = args[1].find('=');
      if (args[1].substr(0, n) != "kernelsize" ||
          n == std::string::npos || n + 1 == args[1].size() ||
          args[1][n+1] < '1' || args[1][n+1] > '9' ||
          args.size() > 2)
      {
        std::cerr << "convolve options: kernelsize=N\n";
        return NULL;
      }
      std::string num = args[1].substr(n+1);
      kernelsize = std::atoi(num.c_str());
      if (kernelsize != 3 && kernelsize != 5 && kernelsize != 7)
      {
        std::cerr << "convolve kernelsize must be 3, 5, or 7\n";
        return NULL;
      }
    }

    int ksize[3] = { kernelsize, kernelsize, kernelsize };
    if (size[2] == 1)
    {
      ksize[2] = 1;
    }

    double kernel[343];
    double sum = 0.0;
    for (int z = 0; z < ksize[2]; z++)
    {
      double dz = z - 0.5*(ksize[2] - 1);
      double r = dz*dz;
      for (int y = 0; y < ksize[1]; y++)
      {
        double dy = y - 0.5*(ksize[1] - 1);
        r += dy*dy;
        for (int x = 0; x < ksize[0]; x++)
        {
          double dx = x - 0.5*(ksize[0] - 1);
          r += dx*dx;
          double v = exp(-r/(kernelsize*kernelsize));
          kernel[((z*ksize[1] + y)*ksize[0]) + x] = v;
          sum += v;
        }
      }
    }
    for (int k = 0; k < ksize[0]*ksize[1]*ksize[2]; k++)
    {
      kernel[k] /= sum;
    }

    if (size[2] == 1)
    {
      if (kernelsize == 3)
      {
        filter->SetKernel3x3(kernel);
      }
      else if (kernelsize == 5)
      {
        filter->SetKernel5x5(kernel);
      }
      else
      {
        filter->SetKernel7x7(kernel);
      }
    }
    else
    {
      if (kernelsize == 3)
      {
        filter->SetKernel3x3x3(kernel);
      }
      else if (kernelsize == 5)
      {
        filter->SetKernel5x5x5(kernel);
      }
      else
      {
        filter->SetKernel7x7x7(kernel);
      }
    }

    filter->Register(NULL);
    return filter;
  }
  //-----
  else if (args[0] == "separable")
  {
    vtkSmartPointer<vtkImageSeparableConvolution> filter =
      vtkSmartPointer<vtkImageSeparableConvolution>::New();

    int kernelsize = 3;

    if (args.size() > 1)
    {
      size_t n = args[1].find('=');
      if (args[1].substr(0, n) != "kernelsize" ||
          n == std::string::npos || n + 1 == args[1].size() ||
          args[1][n+1] < '1' || args[1][n+1] > '9' ||
          args.size() > 2)
      {
        std::cerr << "separable options: kernelsize=N\n";
        return NULL;
      }
      std::string num = args[1].substr(n+1);
      kernelsize = std::atoi(num.c_str());
      if (kernelsize % 2 != 1)
      {
        std::cerr << "separable kernelsize must be odd\n";
        return NULL;
      }
    }

    vtkSmartPointer<vtkFloatArray> kernel =
      vtkSmartPointer<vtkFloatArray>::New();
    kernel->SetNumberOfValues(kernelsize);

    double sum = 0.0;
    for (int k = 0; k < kernelsize; k++)
    {
      double d = k - 0.5*(kernelsize - 1);
      double v = exp(-d*d/(kernelsize*kernelsize));
      kernel->SetValue(k, v);
      sum += v;
    }
    for (vtkIdType k = 0; k < kernelsize; k++)
    {
      kernel->SetValue(k, kernel->GetValue(k) / sum);
    }

    vtkSmartPointer<vtkFloatArray> kernel2 =
      vtkSmartPointer<vtkFloatArray>::New();
    kernel2->SetNumberOfValues(1);
    kernel2->SetValue(0, 1.0);

    filter->SetXKernel(kernel);
    filter->SetYKernel(kernel);
    if (size[2] > 1)
    {
      filter->SetZKernel(kernel);
    }
    else
    {
      filter->SetZKernel(kernel2);
    }

    filter->Register(NULL);
    return filter;
  }
  //-----
  else if (args[0] == "gaussian")
  {
    vtkSmartPointer<vtkImageGaussianSmooth> filter =
      vtkSmartPointer<vtkImageGaussianSmooth>::New();

    int kernelsize = 3;

    if (args.size() > 1)
    {
      size_t n = args[1].find('=');
      if (args[1].substr(0, n) != "kernelsize" ||
          n == std::string::npos || n + 1 == args[1].size() ||
          args[1][n+1] < '1' || args[1][n+1] > '9' ||
          args.size() > 2)
      {
        std::cerr << "gaussian options: kernelsize=N\n";
        return NULL;
      }
      std::string num = args[1].substr(n+1);
      kernelsize = std::atoi(num.c_str());
      if (kernelsize % 2 != 1)
      {
        std::cerr << "gaussian kernelsize must be odd\n";
        return NULL;
      }
    }

    double stdev = (kernelsize-1.0)*0.25;
    if (size[2] > 1)
    {
      filter->SetStandardDeviations(stdev, stdev, stdev);
    }
    else
    {
      filter->SetStandardDeviations(stdev, stdev, 0.0);
    }
    filter->SetRadiusFactors(2.0, 2.0, 2.0);

    filter->Register(NULL);
    return filter;
  }
  //-----
  else if (args[0] == "colormap")
  {
    vtkSmartPointer<vtkImageMapToColors> filter =
      vtkSmartPointer<vtkImageMapToColors>::New();

    bool grey = false;
    int comps = 4;

    for (size_t k = 1; k < args.size(); k++)
    {
      size_t n = args[k].find('=');
      if (args[k].substr(0, n) == "components")
      {
        if (n == std::string::npos || n + 1 == args[k].size() ||
            args[k][n+1] < '1' || args[k][n+1] > '4')
        {
          std::cerr << "colormap components=N where N = 1, 2, 3, or 4\n";
          return NULL;
        }
        std::string num = args[k].substr(n+1);
        comps = std::atoi(num.c_str());
      }
      else if (args[k] == "greyscale")
      {
        grey = true;
      }
      else
      {
        std::cerr << "colormap options: greyscale, components=N\n";
        return NULL;
      }
    }

    if (grey)
    {
      vtkSmartPointer<vtkScalarsToColors> table =
        vtkSmartPointer<vtkScalarsToColors>::New();
      table->SetRange(0, 255);
      filter->SetLookupTable(table);
    }
    else
    {
      vtkSmartPointer<vtkLookupTable> table =
        vtkSmartPointer<vtkLookupTable>::New();
      table->SetRange(0, 255);
      filter->SetLookupTable(table);
    }
    filter->SetOutputFormat(comps);

    filter->Register(NULL);
    return filter;
  }
  //-----
  else if (args[0] == "bspline")
  {
    vtkSmartPointer<vtkImageBSplineCoefficients> filter =
      vtkSmartPointer<vtkImageBSplineCoefficients>::New();

    if (args.size() > 1)
    {
      size_t n = args[1].find('=');
      if (args[1].substr(0, n) != "degree" ||
          n == std::string::npos || n + 1 == args[1].size() ||
          args[1][n+1] < '1' || args[1][n+1] > '9' ||
          args.size() > 2)
      {
        std::cerr << "bspline options: degree=N\n";
        return NULL;
      }
      std::string num = args[1].substr(n+1);
      filter->SetSplineDegree(std::atoi(num.c_str()));
    }

    filter->Register(NULL);
    return filter;
  }
  //-----
  else if (args[0] == "fft")
  {
    vtkSmartPointer<vtkImageFFT> filter =
      vtkSmartPointer<vtkImageFFT>::New();

    if (args.size() > 1)
    {
      std::cerr << "fft takes no options\n";
      return NULL;
    }

    filter->Register(NULL);
    return filter;
  }
  //-----
  else if (args[0] == "histogram")
  {
    vtkSmartPointer<vtkImageHistogram> filter =
      vtkSmartPointer<vtkImageHistogram>::New();

    bool mask = false;

    for (size_t k = 1; k < args.size(); k++)
    {
      size_t n = args[k].find('=');
      std::string key = args[k].substr(0, n);
      if (key == "stencil")
      {
        if (n != std::string::npos)
        {
          std::cerr << "histogram stencil option takes no args\n";
          return NULL;
        }
        mask = true;
      }
      else
      {
        std::cerr << "histogram options: stencil\n";
        return NULL;
      }
    }

    // create a spherical mask (or circular for 2D)
    if (mask)
    {
      vtkSmartPointer<vtkROIStencilSource> stencil =
        vtkSmartPointer<vtkROIStencilSource>::New();
      stencil->SetOutputWholeExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);
      if (size[2] == 1)
      {
        stencil->SetShapeToCylinderZ();
      }
      else if (size[1] == 1)
      {
        stencil->SetShapeToCylinderY();
      }
      else if (size[0] == 1)
      {
        stencil->SetShapeToCylinderX();
      }
      else
      {
        stencil->SetShapeToEllipsoid();
      }
      stencil->SetBounds(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);
      stencil->Update();
      filter->SetStencilData(stencil->GetOutput());
    }

    filter->AutomaticBinningOff();
    filter->SetNumberOfBins(256);
    filter->SetBinOrigin(0.0);
    filter->SetBinSpacing(1.0);

    filter->Register(NULL);
    return filter;
  }

  cerr << "unrecognized option for --filter\n";

  return NULL;
}

// create the source data
static vtkImageData *CreateData(
  const std::string& sourceName, const int scalarType, const int size[3])
{
  vtkImageData *output = NULL;

  if (sourceName == "gaussian")
  {
    vtkSmartPointer<vtkImageGaussianSource> source =
      vtkSmartPointer<vtkImageGaussianSource>::New();
    source->SetWholeExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);
    source->SetCenter(0.5*(size[0]-1), 0.5*(size[1]-1), 0.5*(size[2]-1));
    int maxdim = (size[0] > size[1] ? size[0] : size[1]);
    maxdim = (maxdim > size[2] ? maxdim : size[2]);
    source->SetStandardDeviation(0.25*(maxdim-1));
    source->SetMaximum(255.0);

    vtkSmartPointer<vtkImageCast> cast =
      vtkSmartPointer<vtkImageCast>::New();
    cast->SetInputConnection(source->GetOutputPort());
    cast->SetOutputScalarType(scalarType);
    cast->Update();

    output = cast->GetOutput();
    output->Register(NULL);
  }
  else if (sourceName == "noise")
  {
    vtkSmartPointer<vtkImageNoiseSource> source =
      vtkSmartPointer<vtkImageNoiseSource>::New();
    source->SetMinimum(0.0);
    source->SetMinimum(255.0);
    source->SetWholeExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);

    vtkSmartPointer<vtkImageCast> cast =
      vtkSmartPointer<vtkImageCast>::New();
    cast->SetInputConnection(source->GetOutputPort());
    cast->SetOutputScalarType(scalarType);
    cast->Update();

    output = cast->GetOutput();
    output->Register(NULL);
  }
  else if (sourceName == "mandelbrot")
  {
    vtkSmartPointer<vtkImageMandelbrotSource> source =
      vtkSmartPointer<vtkImageMandelbrotSource>::New();
    source->SetWholeExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);

    vtkSmartPointer<vtkImageCast> cast =
      vtkSmartPointer<vtkImageCast>::New();
    cast->SetInputConnection(source->GetOutputPort());
    cast->SetOutputScalarType(scalarType);
    cast->Update();

    output = cast->GetOutput();
    output->Register(NULL);
  }
  else if (sourceName == "grid")
  {
    vtkSmartPointer<vtkImageGridSource> source =
      vtkSmartPointer<vtkImageGridSource>::New();
    source->SetDataExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);
    source->SetLineValue(255.0);
    source->SetFillValue(0.0);
    source->SetDataScalarType(scalarType);
    source->Update();
    output = source->GetOutput();
    output->Register(NULL);
  }
  else
  {
    cerr << "unrecognized option for --source\n";
    return NULL;
  }

  // standardize the geometry of the output
  vtkSmartPointer<vtkImageChangeInformation> change =
    vtkSmartPointer<vtkImageChangeInformation>::New();
  change->SetInputData(output);
  change->SetOutputSpacing(1.0, 1.0, 1.0);
  change->CenterImageOn();
  change->Update();
  output->Delete();
  output = change->GetOutput();
  output->Register(NULL);

  return output;
}

// Convert a small integer (0 <= i < 1000) to a string
static std::string SmallIntToString(int i)
{
  std::string s;
  if (i > 99)
  {
    s.push_back('0' + (i % 1000)/100);
  }
  if (i > 9)
  {
    s.push_back('0' + (i % 100)/10);
  }
  s.push_back('0' + (i % 10));
  return s;
}

// get a string parameter from the argument list
bool GetParameter(int argc, char *argv[], int argi, std::string *val)
{
  if (argi+1 >= argc || argv[argi+1][0] == '-')
  {
    std::cerr << "option " << argv[argi] << " needs an argument.\n";
    return false;
  }

  *val = argv[argi+1];
  return true;
}

// get a boolean parameter from the argument list
bool GetParameter(int argc, char *argv[], int argi, bool *val)
{
  std::string s;
  if (!GetParameter(argc, argv, argi, &s))
  {
    return false;
  }
  if (s == "on" || s == "yes" || s == "true")
  {
    *val = true;
    return true;
  }
  if (s == "off" || s == "no" || s == "false")
  {
    *val = false;
    return true;
  }

  std::cerr << "option " << argv[argi] << " needs a boolean argument.\n";
  return false;
}

// get an integer parameter from the argument list
bool GetParameter(int argc, char *argv[], int argi, int *val, size_t size=1)
{
  std::string s;
  if (!GetParameter(argc, argv, argi, &s))
  {
    return false;
  }
  size_t n = 0;
  int d = 0;
  bool foundDigit = false;
  for (size_t i = 0; i < s.size(); i++)
  {
    if (s[i] >= '0' && s[i] <= '9')
    {
      foundDigit = true;
      d *= 10;
      d += s[i] - '0';
      if (i + 1 != s.size())
      {
        continue;
      }
    }
    if (foundDigit)
    {
      foundDigit = false;
      if (n < size)
      {
        val[n] = d;
      }
      n++;
      d = 0;
    }
  }

  if (n != size)
  {
    if (size == 1)
    {
      std::cerr << "option " << argv[argi] << " needs an integer.\n";
    }
    else
    {
      std::cerr << "option " << argv[argi] << " needs " << size << " ints.\n";
    }
    return false;
  }

  return true;
}

// get range of positive integers
bool GetParameter(int argc, char *argv[], int argi,
                  std::vector<int> *value)
{
  std::string s;
  if (!GetParameter(argc, argv, argi, &s))
  {
    return false;
  }
  int d = 0;
  int prev = 1;
  bool foundDigit = false;
  char sep = 0;
  for (size_t i = 0; i < s.size(); i++)
  {
    if (s[i] >= '0' && s[i] <= '9')
    {
      foundDigit = true;
      d *= 10;
      d += s[i] - '0';
      if (i + 1 != s.size())
      {
        continue;
      }
    }
    else if (s[i] != ',' && s[i] != '-')
    {
      std::cerr << "option " << argv[argi] << " badly formatted.\n";
      return false;
    }
    if (foundDigit)
    {
      foundDigit = false;
      if (sep == '-')
      {
        if (prev <= d)
        {
          while (++prev < d)
          {
            value->push_back(prev);
          }
        }
        else
        {
          while (--prev > d)
          {
            value->push_back(prev);
          }
        }
      }
      value->push_back(d);
      prev = d;
      d = 0;
    }
    sep = s[i];
  }

  if (value->empty())
  {
    std::cerr << "option " << argv[argi] << " needs an integer.\n";
    return false;
  }

  return true;
}

// Run a benchmark on the named filter
static bool RunBenchmark(
  const std::string& filterName,
  const std::string& sourceName,
  const int size[3],
  int scalarType,
  const std::string& splitMode,
  bool useSMP,
  vtkIdType bytesPerPiece,
  int minPieceSize[3],
  int clearCacheSize,
  std::vector<int>& threads,
  int runs,
  const std::string& units,
  bool reportFilter,
  const std::string& outputFile,
  bool slave)
{
  vtkSmartPointer<vtkThreadedImageAlgorithm> filter;
  filter.TakeReference(CreateFilter(filterName, size));
  if (!filter.GetPointer())
  {
    return false;
  }

  vtkSmartPointer<vtkImageData> data;
  data.TakeReference(CreateData(sourceName, scalarType, size));
  if (!data.GetPointer())
  {
    return false;
  }

  if (splitMode == "slab")
  {
    filter->SetSplitModeToSlab();
  }
  else if (splitMode == "beam")
  {
    filter->SetSplitModeToBeam();
  }
  else if (splitMode == "block")
  {
    filter->SetSplitModeToBlock();
  }

  filter->SetEnableSMP(useSMP);
  if (useSMP)
  {
    if (bytesPerPiece)
    {
      filter->SetDesiredBytesPerPiece(bytesPerPiece);
    }
    if (minPieceSize[0] > 0 && minPieceSize[1] > 0 && minPieceSize[2] > 0)
    {
      filter->SetMinimumPieceSize(minPieceSize);
    }
  }
  else
  {
    if (!threads.empty())
    {
      filter->SetNumberOfThreads(threads[0]);
    }
  }

  filter->SetInputData(data);

  if (Verbose)
  {
    PrintInfo(filter, std::cout);
  }

  // prepare for execution and timing
  vtkSmartPointer<vtkTimerLog> log = vtkSmartPointer<vtkTimerLog>::New();

  std::vector<double> results;
  results.reserve(runs);

  for (int i = 0; i < runs; i++)
  {
    filter->Modified();
    if (clearCacheSize)
    {
      ClearCPUCache(clearCacheSize*1024*1024);
    }
    log->StartTimer();
    filter->Update();
    log->StopTimer();
    double t = log->GetElapsedTime();
    if (!units.empty() && units[0] == 's')
    {
      results.push_back(t);
    }
    else
    {
      double megaVoxels = 0.000001;
      megaVoxels *= size[0];
      megaVoxels *= size[1];
      megaVoxels *= size[2];
      if (units == "mvptps")
      {
        megaVoxels /= (filter->GetEnableSMP() ?
                       vtkSMPTools::GetEstimatedNumberOfThreads() :
                       filter->GetNumberOfThreads());
      }
      results.push_back(megaVoxels/t);
    }
  }

  // write the result
  if (threads.size() > 1 || (slave && threads.size() == 1))
  {
    std::cout << threads[0] << ",";
  }
  double sum = 0.0;
  double sumsq = 0.0;
  for (size_t j = 0; j < results.size(); j++)
  {
    if (j > 0)
    {
      sum += results[j];
      sumsq += results[j]*results[j];
    }
    if (j != 0)
    {
      std::cout << ",";
    }
    std::cout << results[j];
  }
  // average of all but the first
  size_t n = results.size() - 1;
  if (n > 1)
  {
    std::cout << "," << sum/n;
    std::cout << "," << sqrt((sumsq - sum*sum/n)/(n - 1));
  }
  if (reportFilter)
  {
    std::cout << "," << filterName;
  }
  std::cout << std::endl;

  if (!outputFile.empty())
  {
    size_t l = outputFile.length();
    for (size_t j = 0; j < outputFile.length(); j++)
    {
      if (outputFile[j] == '.')
      {
        l = j;
      }
    }
    std::string pngFile = outputFile.substr(0, l) + ".png";

    vtkSmartPointer<vtkImageData> image;

    vtkSmartPointer<vtkImageClip> clip =
      vtkSmartPointer<vtkImageClip>::New();
    clip->SetInputData(filter->GetOutput());
    clip->SetOutputWholeExtent(0, size[0], 0, size[1], size[2]/2, size[2]/2);
    clip->ClipDataOn();
    clip->Update();
    image = clip->GetOutput();

    if (image->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
      // rescale the image to 8-bit
      vtkSmartPointer<vtkImageHistogramStatistics> stats =
        vtkSmartPointer<vtkImageHistogramStatistics>::New();
      stats->SetInputConnection(clip->GetOutputPort());
      stats->Update();

      double range[2];
      stats->GetAutoRange(range);
      if (range[0] > 0.0)
      {
        range[0] = 0.0;
      }

      vtkSmartPointer<vtkImageShiftScale> scale =
        vtkSmartPointer<vtkImageShiftScale>::New();
      scale->SetInputData(image);
      scale->SetShift(-range[0]);
      scale->SetScale(255.0/(range[1] - range[0]));
      scale->ClampOverflowOn();
      scale->SetOutputScalarTypeToUnsignedChar();
      scale->Update();
      image = scale->GetOutput();
    }

    vtkSmartPointer<vtkPNGWriter> writer =
      vtkSmartPointer<vtkPNGWriter>::New();
    writer->SetInputData(image);
    writer->SetFileName(pngFile.c_str());
    writer->Write();
  }

  return true;
}

// The main() entry point
int IMAGE_BENCHMARK_MAIN(int argc, char *argv[])
{
  bool slave = false;
  bool header = false;
  bool reportFilter = false;
  bool useSMP = vtkThreadedImageAlgorithm::GetGlobalDefaultEnableSMP();
  int runs = 1;
  std::vector<int> threads;
  std::string splitMode;
  int clearCacheSize = 0;
  int bytesPerPiece = 0;
  int minPieceSize[3] = { 0, 0, 0 };
  int size[3] = { 256, 256, 256 };
  int scalarType = VTK_SHORT;
  std::string sourceName = "gaussian";
  std::string filterName;
  std::string units = "mvps";
  std::string outputFile;

  int argi = 1;
  while (argi < argc)
  {
    std::string opt = argv[argi];
    if (opt.compare(0, 1, "-") != 0)
    {
      std::cerr << "expected an option, got " << opt.c_str() << "\n";
    }
    if (opt == "-h" || opt == "-help" || opt == "--help")
    {
      std::cout << HelpText;
      return 0;
    }
    if (opt == "--version")
    {
      std::cout << "ImageBenchmark "
                << vtkVersion::GetVTKVersion() << "\n";
      return 0;
    }

    if (opt == "--runs")
    {
      if (!GetParameter(argc, argv, argi++, &runs))
      {
        return 1;
      }
      argi++;
    }
    else if (opt == "--clear-cache")
    {
      if (!GetParameter(argc, argv, argi++, &clearCacheSize))
      {
        return 1;
      }
      argi++;
    }
    else if (opt == "--threads")
    {
      if (!GetParameter(argc, argv, argi++, &threads))
      {
        return 1;
      }
      argi++;
    }
    else if (opt == "--split-mode")
    {
      if (!GetParameter(argc, argv, argi++, &splitMode))
      {
        return 1;
      }
      argi++;
      if (splitMode != "slab" && splitMode != "beam" && splitMode != "block")
      {
        std::cerr << "option " << opt << " needs slab, beam, or block\n";
        return 1;
      }
    }
    else if (opt == "--enable-smp")
    {
      if (!GetParameter(argc, argv, argi++, &useSMP))
      {
        return 1;
      }
      argi++;
    }
    else if (opt == "--bytes-per-piece")
    {
      if (!GetParameter(argc, argv, argi++, &bytesPerPiece))
      {
        return 1;
      }
      argi++;
    }
    else if (opt == "--min-piece-size")
    {
      if (!GetParameter(argc, argv, argi++, minPieceSize, 3))
      {
        return 1;
      }
      argi++;
    }
    else if (opt == "--size")
    {
      if (!GetParameter(argc, argv, argi++, size, 3))
      {
        return 1;
      }
      argi++;
    }
    else if (opt == "--type")
    {
      std::string typeString;
      if (!GetParameter(argc, argv, argi++, &typeString))
      {
        return 1;
      }
      argi++;
      if (typeString == "uchar")
      {
        scalarType = VTK_UNSIGNED_CHAR;
      }
      else if (typeString == "short")
      {
        scalarType = VTK_SHORT;
      }
      else if (typeString == "float")
      {
        scalarType = VTK_FLOAT;
      }
      else
      {
        std::cerr << "option " << opt << " needs uchar, short, or float\n";
        return 1;
      }
    }
    else if (opt == "--source")
    {
      if (!GetParameter(argc, argv, argi++, &sourceName))
      {
        return 1;
      }
      argi++;
    }
    else if (opt == "--filter")
    {
      if (!GetParameter(argc, argv, argi++, &filterName))
      {
        return 1;
      }
      argi++;
    }
    else if (opt == "--output")
    {
      if (!GetParameter(argc, argv, argi++, &outputFile))
      {
        return 1;
      }
      argi++;
    }
    else if (opt == "--units")
    {
      if (!GetParameter(argc, argv, argi++, &units))
      {
        return 1;
      }
      argi++;
      if (units != "s" && units != "seconds" &&
          units != "mvps" && units != "mvptps")
      {
        std::cerr << "option " << opt << " needs mvps, mvptps, or seconds\n";
        return 1;
      }
    }
    else if (opt == "--header")
    {
      header = true;
      argi++;
    }
    else if (opt == "--slave")
    {
      slave = true;
      argi++;
    }
    else if (opt == "--verbose" || opt == "-v")
    {
      Verbose = true;
      argi++;
    }
    else
    {
      std::cerr << "unrecognized option " << opt << "\n";
      return 1;
    }
  }

  // add a filter column if filter will vary
  if (filterName.empty())
  {
    reportFilter = true;
  }

  // write the column headers
  if (header)
  {
    if (threads.size() > 1)
    {
      std::cout << "Threads,";
    }
    for (int j = 0; j < runs; j++)
    {
      if (j != 0)
      {
        std::cout << ",";
      }
      if (!units.empty() && units[0] == 's')
      {
        std::cout << "T" << j;
      }
      else
      {
        std::cout << "R" << j;
      }
    }
    if (runs > 2)
    {
      std::cout << ",Average";
      std::cout << ",StdDev";
    }
    if (reportFilter)
    {
      std::cout << ",Filter";
    }
    std::cout << std::endl;
  }

  if (threads.size() > 1)
  {
    // if a list was given for the numbers of threads, re-run the executable
    // (vtkSMPTools might only allow one initialization per process)
    for (size_t k = 0; k < threads.size(); k++)
    {
      std::string threadopt = SmallIntToString(threads[k]);

      // create sub-process argument list
      std::vector<const char *> commandLine;
      for (int i = 0; i < argc; i++)
      {
        std::string arg = argv[i];

        // don't add --header arg to subprocesses
        if (arg != "--header")
        {
          commandLine.push_back(argv[i]);
        }

        if (arg == "--threads")
        {
          // modify the --thread arg so that it specifies just one number
          commandLine.push_back(threadopt.c_str());
          i++;
        }
      }
      commandLine.push_back("--slave");
      commandLine.push_back(NULL);

      // create and run the subprocess
      vtksysProcess *process = vtksysProcess_New();
      vtksysProcess_SetCommand(process, &commandLine[0]);
      vtksysProcess_Execute(process);

      int pipe;
      do
      {
        char *cp;
        int length;
        pipe = vtksysProcess_WaitForData(process, &cp, &length, NULL);
        switch (pipe)
        {
          case vtksysProcess_Pipe_STDOUT:
            std::cout.write(cp, length);
            break;

          case vtksysProcess_Pipe_STDERR:
            std::cerr.write(cp, length);
            break;
        }
      }
      while (pipe != vtksysProcess_Pipe_None);

      vtksysProcess_WaitForExit(process, NULL);
      int rval = vtksysProcess_GetExitValue(process);
      if (rval != 0)
      {
        return rval;
      }

      vtksysProcess_Delete(process);
    }

    return 0;
  }

  // set the number of threads
  if (useSMP)
  {
    if (!threads.empty())
    {
      vtkSMPTools::Initialize(threads[0]);
    }
  }

  const char *requestedFilters[] = {
    filterName.c_str(), NULL
  };
  const char **filters =
    (filterName.empty() ? DefaultFilters : requestedFilters);

  for (int k = 0; filters[k] != 0; k++)
  {
    if (!RunBenchmark(filters[k], sourceName, size, scalarType,
                      splitMode, useSMP, bytesPerPiece, minPieceSize,
                      clearCacheSize, threads, runs, units, reportFilter,
                      outputFile, slave))
    {
      return 1;
    }
  }

  return 0;
}
