// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Verifies that the forwarding setters on the ViewsRendering classes behave
// like vtkSetMacro: setting a property to the value it already has must not
// bump the object's modified time, while setting a different value must.

#include "vtkActor.h"
#include "vtkColorTransferFunction.h"
#include "vtkLightKit.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkProperty.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"
#include "vtkSphereSource.h"
#include "vtkStandardRenderView.h"
#include "vtkSurfaceRepresentation.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRepresentation.h"

#include <iostream>

#define CHECK(expr, msg)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      std::cerr << "FAILED: " << msg << "\n";                                                      \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

// Set a property to the value it already has; the modified time must not move.
#define CHECK_NOOP(object, setter, getter)                                                         \
  do                                                                                               \
  {                                                                                                \
    const vtkMTimeType before = (object)->GetMTime();                                              \
    (object)->setter((object)->getter());                                                          \
    CHECK(                                                                                         \
      (object)->GetMTime() == before, #setter " modified the object when given its own value");    \
  } while (false)

// Same, for the setters that take the components of a vector separately.
#define CHECK_NOOP_3(object, setter, getter)                                                       \
  do                                                                                               \
  {                                                                                                \
    const double v0 = (object)->getter()[0];                                                       \
    const double v1 = (object)->getter()[1];                                                       \
    const double v2 = (object)->getter()[2];                                                       \
    const vtkMTimeType before = (object)->GetMTime();                                              \
    (object)->setter(v0, v1, v2);                                                                  \
    CHECK(                                                                                         \
      (object)->GetMTime() == before, #setter " modified the object when given its own value");    \
  } while (false)

// Set a property to a genuinely different value; the modified time must move.
#define CHECK_MODIFIES(object, expr)                                                               \
  do                                                                                               \
  {                                                                                                \
    const vtkMTimeType before = (object)->GetMTime();                                              \
    expr;                                                                                          \
    CHECK((object)->GetMTime() > before, #expr " did not modify the object");                      \
  } while (false)

namespace
{

int TestSurfaceRepresentationSetters()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> rep;
  rep->SetInputConnection(sphere->GetOutputPort());

  // Actor and property forwarding.
  CHECK_NOOP_3(rep, SetColor, GetColor);
  CHECK_NOOP_3(rep, SetEdgeColor, GetEdgeColor);
  CHECK_NOOP_3(rep, SetPosition, GetPosition);
  CHECK_NOOP_3(rep, SetOrientation, GetOrientation);
  CHECK_NOOP_3(rep, SetScale, GetScale);
  CHECK_NOOP(rep, SetOpacity, GetOpacity);
  CHECK_NOOP(rep, SetSpecular, GetSpecular);
  CHECK_NOOP(rep, SetSpecularPower, GetSpecularPower);
  CHECK_NOOP(rep, SetLineWidth, GetLineWidth);
  CHECK_NOOP(rep, SetPointSize, GetPointSize);
  CHECK_NOOP(rep, SetLighting, GetLighting);
  CHECK_NOOP(rep, SetVisibility, GetVisibility);
  CHECK_NOOP(rep, SetPickable, GetPickable);
  CHECK_NOOP(rep, SetRepresentation, GetRepresentation);

  // Geometry filter forwarding.
  CHECK_NOOP(rep, SetFeatureAngle, GetFeatureAngle);
  CHECK_NOOP(rep, SetTriangulate, GetTriangulate);
  CHECK_NOOP(rep, SetNonlinearSubdivisionLevel, GetNonlinearSubdivisionLevel);
  CHECK_NOOP(rep, SetPassThroughCellIds, GetPassThroughCellIds);

  // Mapper forwarding.
  CHECK_NOOP(rep, SetScalarVisibility, GetScalarVisibility);
  CHECK_NOOP(rep, SetInterpolateScalarsBeforeMapping, GetInterpolateScalarsBeforeMapping);
  CHECK_NOOP(rep, SetLookupTable, GetLookupTable);
  {
    const double v0 = rep->GetScalarRange()[0];
    const double v1 = rep->GetScalarRange()[1];
    const vtkMTimeType before = rep->GetMTime();
    rep->SetScalarRange(v0, v1);
    CHECK(rep->GetMTime() == before, "SetScalarRange modified the object when given its own value");
  }

  // Scalar bar and selection display properties.
  CHECK_NOOP(rep, SetScalarBarVisibility, GetScalarBarVisibility);
  CHECK_NOOP_3(rep, SetSelectionColor, GetSelectionColor);
  CHECK_NOOP(rep, SetSelectionOpacity, GetSelectionOpacity);
  CHECK_NOOP(rep, SetSelectionLineWidth, GetSelectionLineWidth);
  CHECK_NOOP(rep, SetSelectionPointSize, GetSelectionPointSize);

  // Coloring is a compound operation; repeating it must still be a no-op.
  rep->ColorByPointArray("RTData");
  CHECK_MODIFIES(rep, rep->ColorByCellArray("Other"));
  {
    const vtkMTimeType before = rep->GetMTime();
    rep->ColorByCellArray("Other");
    CHECK(rep->GetMTime() == before, "ColorByCellArray repeated its own value and modified");
  }
  CHECK_MODIFIES(rep, rep->ColorByPointArray("RTData", 2));
  {
    const vtkMTimeType before = rep->GetMTime();
    rep->ColorByPointArray("RTData", 2);
    CHECK(rep->GetMTime() == before, "ColorByPointArray(component) repeated and modified");
  }
  CHECK_MODIFIES(rep, rep->ColorByFieldArray("BlockId"));
  {
    const vtkMTimeType before = rep->GetMTime();
    rep->ColorByFieldArray("BlockId");
    CHECK(rep->GetMTime() == before, "ColorByFieldArray repeated its own value and modified");
  }
  CHECK_NOOP(rep, SetFieldDataTupleId, GetFieldDataTupleId);
  CHECK_MODIFIES(rep, rep->SetFieldDataTupleId(3));

  // Real changes must still be seen.
  CHECK_MODIFIES(rep, rep->SetColor(0.1, 0.2, 0.3));
  CHECK_MODIFIES(rep, rep->SetOpacity(0.25));
  CHECK_MODIFIES(rep, rep->SetRepresentationToWireframe());
  // The geometry-level modes reconfigure the extraction filter, so they must be
  // seen as changes too, including on the way back to a property-level mode.
  CHECK_MODIFIES(rep, rep->SetRepresentationToOutline());
  CHECK_MODIFIES(rep, rep->SetRepresentationToFeatureEdges());
  CHECK_MODIFIES(rep, rep->SetRepresentationToSurface());
  CHECK_MODIFIES(rep, rep->SetVisibility(false));
  CHECK_MODIFIES(rep, rep->SetScalarBarVisibility(true));
  CHECK_MODIFIES(rep, rep->SetFeatureAngle(12.5));
  // The selection actor is created in wireframe, so surface is a real change.
  CHECK_NOOP(rep, SetSelectionRepresentation, GetSelectionRepresentation);
  CHECK_MODIFIES(rep, rep->SetSelectionRepresentation(vtkSurfaceRepresentation::SURFACE));

  // Switching away from SurfaceWithEdges must clear the edges again, which the
  // representation value alone does not capture.
  rep->SetRepresentationToSurfaceWithEdges();
  CHECK_MODIFIES(rep, rep->SetRepresentationToSurface());
  CHECK(
    rep->GetRepresentation() == vtkSurfaceRepresentation::SURFACE, "representation not Surface");

  return EXIT_SUCCESS;
}

int TestVolumeRepresentationSetters()
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  vtkNew<vtkVolumeRepresentation> rep;
  rep->SetInputConnection(wavelet->GetOutputPort());

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
  ctf->AddRGBPoint(255.0, 1.0, 0.0, 0.0);
  rep->SetColorTransferFunction(ctf);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0.0, 0.0);
  pf->AddPoint(255.0, 1.0);
  rep->SetScalarOpacity(pf);

  CHECK_NOOP(rep, SetColorTransferFunction, GetColorTransferFunction);
  CHECK_NOOP(rep, SetScalarOpacity, GetScalarOpacity);
  CHECK_NOOP(rep, SetScalarOpacityUnitDistance, GetScalarOpacityUnitDistance);
  CHECK_NOOP(rep, SetShade, GetShade);
  CHECK_NOOP(rep, SetAmbient, GetAmbient);
  CHECK_NOOP(rep, SetDiffuse, GetDiffuse);
  CHECK_NOOP(rep, SetSpecular, GetSpecular);
  CHECK_NOOP(rep, SetSpecularPower, GetSpecularPower);
  CHECK_NOOP(rep, SetInterpolationType, GetInterpolationType);
  CHECK_NOOP(rep, SetBlendMode, GetBlendMode);
  CHECK_NOOP(rep, SetRequestedRenderMode, GetRequestedRenderMode);
  CHECK_NOOP(rep, SetVisibility, GetVisibility);
  CHECK_NOOP(rep, SetScalarBarVisibility, GetScalarBarVisibility);
  CHECK_NOOP_3(rep, SetPosition, GetPosition);
  CHECK_NOOP_3(rep, SetOrientation, GetOrientation);
  CHECK_NOOP_3(rep, SetScale, GetScale);

  CHECK_MODIFIES(rep, rep->SetAmbient(0.42));
  CHECK_MODIFIES(rep, rep->SetShade(!rep->GetShade()));
  CHECK_MODIFIES(rep, rep->SetPosition(1.0, 2.0, 3.0));

  return EXIT_SUCCESS;
}

int TestStandardRenderViewSetters()
{
  vtkNew<vtkStandardRenderView> view;
  view->GetRenderWindow()->SetOffScreenRendering(true);
  view->SetWindowTitle("mtime test");

  CHECK_NOOP_3(view, SetBackground, GetBackground);
  CHECK_NOOP_3(view, SetBackground2, GetBackground2);
  CHECK_NOOP(view, SetGradientBackground, GetGradientBackground);
  CHECK_NOOP(view, SetWindowTitle, GetWindowTitle);
  CHECK_NOOP(view, SetOrientationAxesVisibility, GetOrientationAxesVisibility);
  CHECK_NOOP(view, SetOrientationAxesInteractive, GetOrientationAxesInteractive);
  CHECK_NOOP(view, SetUseLightKit, GetUseLightKit);
  CHECK_NOOP(view, SetKeyLightIntensity, GetKeyLightIntensity);
  CHECK_NOOP(view, SetKeyToFillRatio, GetKeyToFillRatio);
  CHECK_NOOP(view, SetKeyLightWarmth, GetKeyLightWarmth);
  CHECK_NOOP(view, SetMaintainLuminance, GetMaintainLuminance);
  CHECK_NOOP(view, SetInteractionMode, GetInteractionMode);
  CHECK_NOOP(view, SetSelectionMode, GetSelectionMode);
  CHECK_NOOP(view, SetSelectionFieldAssociation, GetSelectionFieldAssociation);
  {
    const int w = view->GetWindowSize()[0];
    const int h = view->GetWindowSize()[1];
    const vtkMTimeType before = view->GetMTime();
    view->SetWindowSize(w, h);
    CHECK(view->GetMTime() == before, "SetWindowSize modified the view when given its own value");
  }
  {
    const double elevation = view->GetKeyLightAngle()[0];
    const double azimuth = view->GetKeyLightAngle()[1];
    const vtkMTimeType before = view->GetMTime();
    view->SetKeyLightAngle(elevation, azimuth);
    CHECK(
      view->GetMTime() == before, "SetKeyLightAngle modified the view when given its own value");
  }

  CHECK_MODIFIES(view, view->SetBackground(0.1, 0.2, 0.3));
  CHECK_MODIFIES(view, view->SetWindowTitle("a different title"));
  CHECK_MODIFIES(view, view->SetKeyLightIntensity(0.125));
  CHECK_MODIFIES(view, view->SetGradientBackground(!view->GetGradientBackground()));

  return EXIT_SUCCESS;
}

// The representations and the view keep their state on the objects they own,
// so GetMTime() has to fold those in. Rendering must not disturb it, otherwise
// the pipeline would re-execute every frame.
int TestModifiedTimeAggregation()
{
  vtkNew<vtkStandardRenderView> view;
  view->GetRenderWindow()->SetOffScreenRendering(true);
  view->SetWindowSize(300, 300);

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());
  view->AddRepresentation(surface);

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-5, 5, -5, 5, -5, 5);
  vtkNew<vtkVolumeRepresentation> volume;
  volume->SetInputConnection(wavelet->GetOutputPort());
  view->AddRepresentation(volume);

  // A change made directly on an owned object has to be visible.
  CHECK_MODIFIES(surface, surface->GetActor()->GetProperty()->SetColor(0.9, 0.1, 0.1));
  CHECK_MODIFIES(surface, surface->GetScalarBarActor()->SetNumberOfLabels(7));
  CHECK_MODIFIES(volume, volume->GetVolume()->GetProperty()->SetAmbient(0.33));
  CHECK_MODIFIES(view, view->GetRenderer()->SetBackground(0.5, 0.5, 0.5));
  CHECK_MODIFIES(view, view->GetLightKit()->SetKeyLightIntensity(0.9));

  // Rendering must leave all three modified times alone.
  view->Render();
  const vtkMTimeType surfaceTime = surface->GetMTime();
  const vtkMTimeType volumeTime = volume->GetMTime();
  const vtkMTimeType viewTime = view->GetMTime();
  view->Render();
  view->Render();
  CHECK(surface->GetMTime() == surfaceTime, "rendering modified the surface representation");
  CHECK(volume->GetMTime() == volumeTime, "rendering modified the volume representation");
  CHECK(view->GetMTime() == viewTime, "rendering modified the view");

  return EXIT_SUCCESS;
}

}

int TestSetterModifiedTime(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  if (TestSurfaceRepresentationSetters() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  if (TestVolumeRepresentationSetters() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  if (TestStandardRenderViewSetters() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  if (TestModifiedTimeAggregation() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
