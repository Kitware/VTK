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
#include "vtkSmartPointer.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkTestUtilities.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositePolyDataMapper2.h"

#include "vtkCompositeSurfaceLICMapper.h"
#include "vtkSurfaceLICMapper.h"
#include "vtkSurfaceLICInterface.h"

#include "vtk_glew.h"

#include <vector>
#include <string>

#ifndef vtkFloatingPointTemplateMacro
#define vtkFloatingPointTemplateMacro(call) \
  vtkTemplateMacroCase(VTK_DOUBLE, double, call); \
  vtkTemplateMacroCase(VTK_FLOAT, float, call);
#endif

// Helper to compute range
static
void Range(vtkDataArray *S, double *range)
{
  double Srange[2];
  S->GetRange(Srange);
  range[0] = Srange[0] < range[0] ? Srange[0] : range[0];
  range[1] = Srange[1] > range[1] ? Srange[1] : range[1];
}

// helper to compute magnitude
static
vtkDataArray *Magnitude(vtkDataArray *V)
{
  vtkIdType nTups = V->GetNumberOfTuples();
  vtkIdType nComps = V->GetNumberOfComponents();
  vtkDataArray *magV = V->NewInstance();
  magV->SetNumberOfTuples(nTups);
  switch (V->GetDataType())
  {
    vtkFloatingPointTemplateMacro(
      VTK_TT *pV = (VTK_TT*)V->GetVoidPointer(0);
      VTK_TT *pMagV = (VTK_TT*)magV->GetVoidPointer(0);
      for (vtkIdType i=0; i<nTups; ++i)
      {
        VTK_TT mag = VTK_TT(0);
        for (vtkIdType j=0; j<nComps; ++j)
        {
          VTK_TT v = pV[i*nComps+j];
          mag += v*v;
        }
        pMagV[i] = sqrt(mag);
      }
      );
  default:
    cerr << "ERROR: vectors must be float or double" << endl;
    break;
  }
  return magV;
}

// Compute the magnitude of the named vector and add it to
// dataset, return range.
static
vtkDataArray *Magnitude(vtkDataSet *ds, std::string &vectors)
{
  vtkDataArray *V = NULL;
  V = ds->GetPointData()->GetArray(vectors.c_str());
  if (V == NULL)
  {
    cerr << "ERROR: point vectors " << vectors << " not found" << endl;
    return NULL;
  }
  vtkDataArray *magV = Magnitude(V);
  std::string magVName = "mag"+vectors;
  magV->SetName(magVName.c_str());
  return magV;
}

// This example demonstrates the use of vtkSurfaceLICmapper for rendering
// geometry with LIC on the surface.
int vtkSurfaceLICTestDriver(
      int argc,
      char **argv,
      vtkDataObject *dataObj,
      int num_steps,
      double step_size,
      int enhanced_lic,
      int normalize_vectors,
      int camera_config,
      int generate_noise_texture,
      int noise_type,
      int noise_texture_size,
      int noise_grain_size,
      double min_noise_value,
      double max_noise_value,
      int number_of_noise_levels,
      double impulse_noise_prob,
      double impulse_noise_bg_value,
      int noise_gen_seed,
      int enhance_contrast,
      double low_lic_contrast_enhancement_factor,
      double high_lic_contrast_enhancement_factor,
      double low_color_contrast_enhancement_factor,
      double high_color_contrast_enhancement_factor,
      int anti_alias,
      int color_mode,
      double lic_intensity,
      double map_mode_bias,
      int color_by_mag,
      int mask_on_surface,
      double mask_threshold,
      double mask_intensity,
      std::vector<double> &mask_color_rgb,
      std::string &vectors)
{
  // Set up the render window, renderer, interactor.
  vtkSmartPointer<vtkRenderWindow> renWin
    = vtkSmartPointer<vtkRenderWindow>::New();

  vtkSmartPointer<vtkRenderer> renderer
    = vtkSmartPointer<vtkRenderer>::New();

  vtkSmartPointer<vtkRenderWindowInteractor> iren
    = vtkSmartPointer<vtkRenderWindowInteractor>::New();

  renWin->AddRenderer(renderer);
  iren->SetRenderWindow(renWin);

  if (camera_config == 1)
  {
    renWin->SetSize(300, 300);
  }
  else
  if (camera_config == 2)
  {
    renWin->SetSize(300, 270);
  }
  else
  if (camera_config == 3)
  {
    renWin->SetSize(400, 340);
  }
  else
  if (camera_config == 4)
  {
    renWin->SetSize(364, 256);
  }
  renWin->Render();

  if (!vtkSurfaceLICInterface::IsSupported(renWin))
  {
    cerr
      << "WARNING: The rendering context does not support required extensions."
      << endl;
    dataObj = NULL;
    renWin = NULL;
    renderer = NULL;
    iren = NULL;
    vtkAlgorithm::SetDefaultExecutivePrototype(NULL);
    return 0;
  }

  // Create a mapper and insert the vtkSurfaceLICmapper mapper into the
  // mapper chain. This is essential since the entire logic of performin the
  // LIC is present in the vtkSurfaceLICmapper.

  vtkSmartPointer<vtkCompositeSurfaceLICMapper> mapper
    = vtkSmartPointer<vtkCompositeSurfaceLICMapper>::New();
  // vtkSmartPointer<vtkSurfaceLICMapper> mapper
  //   = vtkSmartPointer<vtkSurfaceLICMapper>::New();

  // print details of the test
  // convenient for debugging failed
  // tests on remote dashboards.
  const char *vtkGLVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  const char *vtkGLVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
  const char *vtkGLRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  const char *vtkLICClass = mapper->GetClassName();
  std::string details
    = std::string("\n\n====================================================================\n")
    + std::string("VTK mapper:\n")
    + std::string("    ") + std::string(vtkLICClass) + std::string("\n")
    + std::string("OpenGL:\n")
    + std::string("    ") + std::string(vtkGLVersion?vtkGLVersion:"unknown") + std::string("\n")
    + std::string("    ") + std::string(vtkGLRenderer?vtkGLRenderer:"unknown") + std::string("\n")
    + std::string("    ") + std::string(vtkGLVendor?vtkGLVendor:"unknown") + std::string("\n")
    + std::string("====================================================================\n\n\n");
  cerr << details << endl;

  // If user chose a vector field, select it.
  if (vectors != "")
  {
    mapper->SetInputArrayToProcess(0, 0, 0,
      vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
      vectors.c_str());
  }
  else
  {
    cerr << "ERROR: vectors must be set using --vectors." << endl;
    return 1;
  }

  // Set the mapper input
  mapper->SetInputDataObject(dataObj);

  if (color_by_mag)
  {
    if ( vectors.empty() )
    {
      cerr << "ERROR: color by mag requires using --vectors." << endl;
      vtkAlgorithm::SetDefaultExecutivePrototype(NULL);
      return 1;
    }

    const char *magVName = NULL;
    double range[2] = {VTK_FLOAT_MAX, -VTK_FLOAT_MAX};
    vtkCompositeDataSet *cd = dynamic_cast<vtkCompositeDataSet*>(dataObj);
    if (cd)
    {
      vtkCompositeDataIterator* iter = cd->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkDataSet* ds = dynamic_cast<vtkDataSet*>(iter->GetCurrentDataObject());
        if (ds && ds->GetNumberOfCells())
        {
          vtkDataArray *magV = Magnitude(ds, vectors);
          magVName = magV->GetName();
          Range(magV, range);
          ds->GetPointData()->SetScalars(magV);
          magV->Delete();
        }
      }
      iter->Delete();
    }
    vtkDataSet* ds = dynamic_cast<vtkDataSet*>(dataObj);
    if (ds && ds->GetNumberOfCells())
    {
      vtkDataArray *magV = Magnitude(ds, vectors);
      magVName = magV->GetName();
      Range(magV, range);
      ds->GetPointData()->SetScalars(magV);
      magV->Delete();
    }
    if (!magVName)
    {
      cerr << "ERROR: color by mag could not generate magV." << endl;
      vtkAlgorithm::SetDefaultExecutivePrototype(NULL);
      return 1;
    }
    vtkColorTransferFunction *lut = vtkColorTransferFunction::New();
    lut->SetColorSpaceToRGB();
    lut->AddRGBPoint(range[0], 0.0, 0.0, 1.0);
    lut->AddRGBPoint(range[1], 1.0, 0.0, 0.0);
    lut->SetColorSpaceToDiverging();
    lut->Build();
    mapper->SetLookupTable(lut);
    mapper->SetScalarModeToUsePointData();
    mapper->SetScalarVisibility(1);
    mapper->SelectColorArray(magVName);
    mapper->SetUseLookupTableScalarRange(1);
    mapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
    lut->Delete();
  }

  // Pass parameters.
  vtkSurfaceLICInterface *li = mapper->GetLICInterface();
  li->SetNumberOfSteps(num_steps);
  li->SetStepSize(step_size);
  li->SetEnhancedLIC(enhanced_lic);
  li->SetGenerateNoiseTexture(generate_noise_texture);
  li->SetNoiseType(noise_type);
  li->SetNormalizeVectors(normalize_vectors);
  li->SetNoiseTextureSize(noise_texture_size);
  li->SetNoiseGrainSize(noise_grain_size);
  li->SetMinNoiseValue(min_noise_value);
  li->SetMaxNoiseValue(max_noise_value);
  li->SetNumberOfNoiseLevels(number_of_noise_levels);
  li->SetImpulseNoiseProbability(impulse_noise_prob);
  li->SetImpulseNoiseBackgroundValue(impulse_noise_bg_value);
  li->SetNoiseGeneratorSeed(noise_gen_seed);
  li->SetEnhanceContrast(enhance_contrast);
  li->SetLowLICContrastEnhancementFactor(low_lic_contrast_enhancement_factor);
  li->SetHighLICContrastEnhancementFactor(high_lic_contrast_enhancement_factor);
  li->SetLowColorContrastEnhancementFactor(low_color_contrast_enhancement_factor);
  li->SetHighColorContrastEnhancementFactor(high_color_contrast_enhancement_factor);
  li->SetAntiAlias(anti_alias);
  li->SetColorMode(color_mode);
  li->SetLICIntensity(lic_intensity);
  li->SetMapModeBias(map_mode_bias);
  li->SetMaskOnSurface(mask_on_surface);
  li->SetMaskThreshold(mask_threshold);
  li->SetMaskIntensity(mask_intensity);
  li->SetMaskColor(&mask_color_rgb[0]);

  vtkSmartPointer<vtkActor> actor
    = vtkSmartPointer<vtkActor>::New();

  actor->SetMapper(mapper);
  renderer->AddActor(actor);
  renderer->SetBackground(0.3, 0.3, 0.3);
  mapper = NULL;
  actor = NULL;

  vtkCamera *camera = renderer->GetActiveCamera();

  if (camera_config == 1)
  {
    renWin->SetSize(300, 300);
    renderer->SetBackground(0.3216, 0.3412, 0.4314);
    renderer->SetBackground2(0.0, 0.0, 0.1647);
    renderer->GradientBackgroundOn();
    camera->SetFocalPoint(-1.88, -0.98, -1.04);
    camera->SetPosition(13.64, 4.27, -31.59);
    camera->SetViewAngle(30);
    camera->SetViewUp(0.41, 0.83, 0.35);
    renderer->ResetCamera();
  }
  else
  if (camera_config == 2)
  {
    renWin->SetSize(300, 270);
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    camera->SetPosition(1.0, 0.0, 0.0);
    camera->SetViewAngle(30);
    camera->SetViewUp(0.0, 0.0, 1.0);
    renderer->ResetCamera();
    camera->Zoom(1.2);
  }
  else
  if (camera_config == 3)
  {
    renWin->SetSize(400, 340);
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    camera->SetPosition(1.0, 0.0, 0.0);
    camera->SetViewAngle(30);
    camera->SetViewUp(0.0, 0.0, 1.0);
    renderer->ResetCamera();
    camera->Zoom(1.4);
  }
  else
  if (camera_config == 4)
  {
    renWin->SetSize(364, 256);
    renderer->SetBackground(0.3216, 0.3412, 0.4314);
    renderer->SetBackground2(0.0, 0.0, 0.1647);
    renderer->GradientBackgroundOn();
    camera->SetFocalPoint(-30.3, 15.2, 7.0);
    camera->SetPosition(64.7, 3.2, -14.0);
    camera->SetViewAngle(30);
    camera->SetViewUp(0.25, 0.5, 0.8);
    //renderer->ResetCamera();
    camera->Zoom(1.09);
  }

  int retVal = vtkTesting::Test(argc, argv, renWin, 75);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renWin->Render();
    iren->Start();
  }

  renderer = NULL;
  renWin = NULL;
  iren = NULL;

  if ((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR))
  {
    return 0;
  }
  // test failed.
  return 1;
}
