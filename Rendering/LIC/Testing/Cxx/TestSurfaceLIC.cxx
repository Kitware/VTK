/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSurfaceLIC.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObjectFactory.h"
#include "vtkCellData.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkSurfaceLICTestDriver.h"

#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/SystemTools.hxx>
#include <vector>
#include <string>

// Description:
// Serial regression test, parse command line, build the
// pipeline, call the driver.
int TestSurfaceLIC(int argc, char* argv[])
{
  vtkCompositeDataPipeline* prototype = vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(prototype);
  prototype->Delete();

  std::string filename;
  int num_steps = 40;
  double step_size = 0.4;
  int enhanced_lic = 1;
  int normalize_vectors = 1;
  int camera_config = 1;
  int generate_noise_texture = 0;
  int noise_type = 1;
  int noise_texture_size = 200;
  int noise_grain_size = 2;
  double min_noise_value = 0.0;
  double max_noise_value = 1.0;
  int number_of_noise_levels = 256;
  double impulse_noise_prob = 1.0;
  double impulse_noise_bg_value = 0.0;
  int noise_gen_seed = 1;
  int enhance_contrast = 0;
  double low_lic_contrast_enhancement_factor = 0.0;
  double high_lic_contrast_enhancement_factor = 0.0;
  double low_color_contrast_enhancement_factor = 0.0;
  double high_color_contrast_enhancement_factor = 0.0;
  int anti_alias = 0;
  int color_mode = 0;
  double lic_intensity = 0.8;
  double map_mode_bias = 0.0;
  int color_by_mag = 0;
  int mask_on_surface = 0;
  double mask_threshold = 0.0;
  double mask_intensity = 0.0;
  std::vector<double> mask_color_rgb;
  std::string vectors;

  vtksys::CommandLineArguments arg;
  arg.StoreUnusedArguments(1);
  arg.Initialize(argc, argv);

  // Fill up accepted arguments.
  typedef vtksys::CommandLineArguments argT;

  arg.AddArgument("--data", argT::EQUAL_ARGUMENT, &filename,
    "(required) Enter dataset to load (currently only *.[vtk|vtp] files are supported");
  arg.AddArgument("--num-steps", argT::EQUAL_ARGUMENT, &num_steps,
    "(optional: default 40) Number of steps in each direction");
  arg.AddArgument("--step-size", argT::EQUAL_ARGUMENT, &step_size,
    "(optional: default 0.4) Step size in pixels");
  arg.AddArgument("--enhanced-lic", argT::EQUAL_ARGUMENT, &enhanced_lic,
    "(optional: default 1) Enable enhanced algoruthm");
  arg.AddArgument("--color-by-mag", argT::EQUAL_ARGUMENT, &color_by_mag,
    "(optional: default 0) when set compute the magnitude of the vector and color the lic by this");
  arg.AddArgument("--vectors", argT::EQUAL_ARGUMENT, &vectors,
    "(optional: default active point vectors) Name of the vector field array");
  arg.AddArgument("--normalize-vectors", argT::EQUAL_ARGUMENT, &normalize_vectors,
    "(optional: default 1) Normalize vectors during integration");
  arg.AddArgument("--generate-noise-texture", argT::EQUAL_ARGUMENT, &generate_noise_texture,
    "(optional: default 0) Generate noise texture (if not generate use pickeled 200x200 noise texture.");
  arg.AddArgument("--noise-type", argT::EQUAL_ARGUMENT, &noise_type,
    "(optional: default 1) statistical distribution for noise generator, 0=Uniform, 1=Gaussian. 2=perlin");
  arg.AddArgument("--noise-texture-size", argT::EQUAL_ARGUMENT, &noise_texture_size,
    "(optional: default 200) side of the square texture in pixels");
  arg.AddArgument("--noise-grain-size", argT::EQUAL_ARGUMENT, &noise_grain_size,
    "(optional: default 1) number of pixels each noise value occupies");
  arg.AddArgument("--min-noise-value", argT::EQUAL_ARGUMENT, &min_noise_value,
    "(optional: default 0) darkest color noise can contain");
  arg.AddArgument("--max-noise-value", argT::EQUAL_ARGUMENT, &max_noise_value,
    "(optional: default 1) lightest color noise can contain");
  arg.AddArgument("--number-of-noise-levels", argT::EQUAL_ARGUMENT, &number_of_noise_levels,
    "(optional: default 256) number of gray scale colors");
  arg.AddArgument("--impulse-noise-prob", argT::EQUAL_ARGUMENT, &impulse_noise_prob,
    "(optional: default 1.0) probabilty a pixel will conatin a noise value");
  arg.AddArgument("--impulse-noise-bg-val", argT::EQUAL_ARGUMENT, &impulse_noise_bg_value,
    "(optional: default 1.0) bg_valabilty a pixel will conatin a noise value");
  arg.AddArgument("--noise-gen-seed", argT::EQUAL_ARGUMENT, &noise_gen_seed,
    "(optional: default 1) set the seed to the random number generator");
  arg.AddArgument("--enhance-contrast", argT::EQUAL_ARGUMENT, &enhance_contrast,
    "(optional: default 0) Nomralize colors after each pass");
  arg.AddArgument("--low-lic-contrast-enhancement-factor", argT::EQUAL_ARGUMENT, &low_lic_contrast_enhancement_factor,
    "(optional: default 0) lower normalization factor 0 is the min");
  arg.AddArgument("--high-lic-contrast-enhancement-factor", argT::EQUAL_ARGUMENT, &high_lic_contrast_enhancement_factor,
    "(optional: default 1) upper normalization factor, 0 is the max");
  arg.AddArgument("--low-color-contrast-enhancement-factor", argT::EQUAL_ARGUMENT, &low_color_contrast_enhancement_factor,
    "(optional: default 0) lower normalization factor 0 is the min");
  arg.AddArgument("--high-color-contrast-enhancement-factor", argT::EQUAL_ARGUMENT, &high_color_contrast_enhancement_factor,
    "(optional: default 1) upper normalization factor, 0 is the max");
  arg.AddArgument("--anti-alias", argT::EQUAL_ARGUMENT, &anti_alias,
    "(optional: default 0) apply anti-aliasing pass after lic to remove jagged artifacts");
  arg.AddArgument("--color-mode", argT::EQUAL_ARGUMENT, &color_mode,
    "(optional: default 0) choose color blending algorithm 0==blending 1==mapping");
  arg.AddArgument("--lic-intensity", argT::EQUAL_ARGUMENT, &lic_intensity,
    "(optional: default 0.8) Contribution of LIC in the final image [1.0 == max contribution]");
  arg.AddArgument("--map-mode-bias", argT::EQUAL_ARGUMENT, &map_mode_bias,
    "(optional: default 0.8) Contribution of LIC in the final image [1.0 == max contribution]");
  arg.AddArgument("--mask-on-surface", argT::EQUAL_ARGUMENT, &mask_on_surface,
    "(optional: default 0) Mask criteria is computed on surface-projected vectors");
  arg.AddArgument("--mask-threshold", argT::EQUAL_ARGUMENT, &mask_threshold,
    "(optional: default 0.0) Mask fragment if |V| < threshold");
  arg.AddArgument("--mask-intensity", argT::EQUAL_ARGUMENT, &mask_intensity,
    "(optional: default 0.0) blending factor for masked fragments");
  arg.AddArgument("--mask-color", argT::MULTI_ARGUMENT, &mask_color_rgb,
    "(optional: default pink=1.0 0.0 0.84705) mask color");
  arg.AddArgument("--camera-config", argT::EQUAL_ARGUMENT, &camera_config,
    "(optional: default 1) use a preset camera configuration");

  if (!arg.Parse() || filename == "")
  {
    cerr << "Usage: " << endl;
    cerr << arg.GetHelp() << endl;
    return 1;
  }

  if (mask_color_rgb.size() == 0)
  {
    // something bright for the default.
    mask_color_rgb.resize(3);
    mask_color_rgb[0] = 1.0;
    mask_color_rgb[1] = 0.0;
    mask_color_rgb[2] = 0.84705;
  }

  vtkSmartPointer<vtkDataObject> dataObj;
  std::string ext = vtksys::SystemTools::GetFilenameExtension(filename);
  if (ext == ".vtk")
  {
    vtkGenericDataObjectReader* reader = vtkGenericDataObjectReader::New();
    reader->SetFileName(filename.c_str());

    vtkDataSetSurfaceFilter* surface = vtkDataSetSurfaceFilter::New();
    surface->SetInputConnection(reader->GetOutputPort());
    surface->Update();

    dataObj = surface->GetOutputDataObject(0);

    reader->Delete();
    surface->Delete();
  }
  else
  if (ext == ".vtp")
  {
    vtkXMLPolyDataReader* reader = vtkXMLPolyDataReader::New();
    reader->SetFileName(filename.c_str());
    reader->Update();
    dataObj = reader->GetOutputDataObject(0);
    reader->Delete();
  }
  else
  if (ext == ".vtm")
  {
    vtkXMLMultiBlockDataReader* reader = vtkXMLMultiBlockDataReader::New();
    reader->SetFileName(filename.c_str());

    vtkDataSetSurfaceFilter* surface = vtkDataSetSurfaceFilter::New();
    surface->SetInputConnection(reader->GetOutputPort());
    surface->Update();

    dataObj = surface->GetOutputDataObject(0);

    reader->Delete();
    surface->Delete();
  }
  else
  {
    cerr << "Error: Unknown extension: '" << ext << "'"<< endl;
    vtkAlgorithm::SetDefaultExecutivePrototype(NULL);
    return 1;
  }

  if (!dataObj) // || dataObj->GetNumberOfElements(vtkDataObject::POINT) == 0)
  {
    cerr << "Error reading file: '" << filename.c_str() << "'" << endl;
    vtkAlgorithm::SetDefaultExecutivePrototype(NULL);
    return 1;
  }

  int status = vtkSurfaceLICTestDriver(
        argc,
        argv,
        dataObj,
        num_steps,
        step_size,
        enhanced_lic,
        normalize_vectors,
        camera_config,
        generate_noise_texture,
        noise_type,
        noise_texture_size,
        noise_grain_size,
        min_noise_value,
        max_noise_value,
        number_of_noise_levels,
        impulse_noise_prob,
        impulse_noise_bg_value,
        noise_gen_seed,
        enhance_contrast,
        low_lic_contrast_enhancement_factor,
        high_lic_contrast_enhancement_factor,
        low_color_contrast_enhancement_factor,
        high_color_contrast_enhancement_factor,
        anti_alias,
        color_mode,
        lic_intensity,
        map_mode_bias,
        color_by_mag,
        mask_on_surface,
        mask_threshold,
        mask_intensity,
        mask_color_rgb,
        vectors);

  vtkAlgorithm::SetDefaultExecutivePrototype(NULL);

  return status;
}
