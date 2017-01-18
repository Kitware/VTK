/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ExerciseUnstructuredGridRayCastMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "ExerciseUnstructuredGridRayCastMapper.h"

#include "vtkActor.h"
#include "vtkArrayCalculator.h"
#include "vtkAssignAttribute.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCubeSource.h"
#include "vtkDataSet.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkDoubleArray.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnstructuredGridVolumeRayCastFunction.h"
#include "vtkUnstructuredGridVolumeRayCastMapper.h"
#include "vtkUnstructuredGridVolumeRayIntegrator.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "vtkRegressionTestImage.h"

//=============================================================================

// A simple filter that will convert an array from independent scalars
// to dependent scalars.
class vtkClassifyVolume : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkClassifyVolume, vtkDataSetAlgorithm);
  static vtkClassifyVolume *New();

  vtkGetObjectMacro(TransferFunction, vtkVolumeProperty);
  vtkSetObjectMacro(TransferFunction, vtkVolumeProperty);

protected:
  vtkClassifyVolume();
  ~vtkClassifyVolume();

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

  virtual void Classify(vtkDataSetAttributes *in, vtkDataSetAttributes *out);

  vtkVolumeProperty *TransferFunction;

private:
  vtkClassifyVolume(const vtkClassifyVolume&) VTK_DELETE_FUNCTION;
  void operator=(const vtkClassifyVolume&) VTK_DELETE_FUNCTION;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkClassifyVolume);

vtkClassifyVolume::vtkClassifyVolume()
{
  this->TransferFunction = NULL;
}

vtkClassifyVolume::~vtkClassifyVolume()
{
  this->SetTransferFunction(NULL);
}

//-----------------------------------------------------------------------------

int vtkClassifyVolume::RequestData(vtkInformation *,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
                                     inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
                                    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->CopyStructure(input);
  this->Classify(input->GetPointData(), output->GetPointData());
  this->Classify(input->GetCellData(), output->GetCellData());

  return 1;
}

//-----------------------------------------------------------------------------

void vtkClassifyVolume::Classify(vtkDataSetAttributes *inAttrib,
                                 vtkDataSetAttributes *outAttrib)
{
  vtkDataArray *scalars = inAttrib->GetScalars();
  if (!scalars) return;

  if (scalars->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro(<< "Only 1-tuple scalars are supported now.");
    return;
  }
  vtkIdType numScalars = scalars->GetNumberOfTuples();

  vtkDoubleArray *colors = vtkDoubleArray::New();
  colors->SetName(scalars->GetName());

  if (this->TransferFunction->GetColorChannels() == 3)
  {
    colors->SetNumberOfComponents(4);
    colors->SetNumberOfTuples(numScalars);

    vtkColorTransferFunction *rgb
      = this->TransferFunction->GetRGBTransferFunction();
    vtkPiecewiseFunction *alpha = this->TransferFunction->GetScalarOpacity();

    for (vtkIdType i = 0; i < numScalars; i++)
    {
      double c[4];
      double x = scalars->GetComponent(i, 0);
      rgb->GetColor(x, c);
      c[3] = alpha->GetValue(x);
      colors->SetTuple(i, c);
    }
  }
  else
  {
    vtkErrorMacro(<< "Gray values are not supported now.");
  }

  outAttrib->SetScalars(colors);
  colors->Delete();
}

//=============================================================================

static vtkRenderer *NewTestViewport(RayCastFunctionCreator NewFunction,
                                    RayIntegratorCreator NewIntegrator,
                                    vtkVolumeProperty *volumeProperty,
                                    int UseCellData, int UseDependentComponents,
                                    int UseMultipleTransferFunctions)
{
  // Create the render window.
  vtkRenderer *ren = vtkRenderer::New();

  // Create a small mesh.  The coarser and more opaque the mesh, the easier it
  // is to see rendering errors.
  vtkImageMandelbrotSource *input = vtkImageMandelbrotSource::New();
  input->SetWholeExtent(0, 2, 0, 2, 0, 2);
  input->SetSizeCX(2, 2, 2, 2);
  input->SetMaximumNumberOfIterations(10);
  vtkAlgorithmOutput *outputPort = input->GetOutputPort(0);

  // Make sure we have only tetrahedra.
  vtkDataSetTriangleFilter *trifilter = vtkDataSetTriangleFilter::New();
  trifilter->SetInputConnection(0, outputPort);
  outputPort = trifilter->GetOutputPort(0);

  // Make multiple scalars if necessary.
  vtkArrayCalculator *calc = NULL;
  vtkAssignAttribute *assign = NULL;
  if (UseMultipleTransferFunctions)
  {
    calc = vtkArrayCalculator::New();
    calc->AddScalarArrayName("Iterations");
    calc->SetResultArrayName("Result");
    calc->SetFunction("Iterations*iHat + (10-Iterations)*jHat");
    calc->SetInputConnection(0, outputPort);
    outputPort = calc->GetOutputPort(0);

    assign = vtkAssignAttribute::New();
    assign->Assign(vtkDataSetAttributes::VECTORS, vtkDataSetAttributes::SCALARS,
                   vtkAssignAttribute::POINT_DATA);
    assign->SetInputConnection(0, outputPort);
    outputPort = assign->GetOutputPort(0);
  }

  // Convert to cell centered data if requested.
  vtkPointDataToCellData *celldata = NULL;
  if (UseCellData)
  {
    celldata = vtkPointDataToCellData::New();
    celldata->SetInputConnection(0, outputPort);
    celldata->PassPointDataOff();
    outputPort = celldata->GetOutputPort(0);
  }

  // Classify the data if testing dependent components.
  vtkClassifyVolume *classify = NULL;
  if (UseDependentComponents)
  {
    classify = vtkClassifyVolume::New();
    classify->SetTransferFunction(volumeProperty);
    classify->SetInputConnection(0, outputPort);
    outputPort = classify->GetOutputPort(0);
  }

  // Set up the mapper.
  vtkUnstructuredGridVolumeRayCastMapper *mapper
    = vtkUnstructuredGridVolumeRayCastMapper::New();
  mapper->SetInputConnection(0, outputPort);
  if (NewFunction)
  {
    vtkUnstructuredGridVolumeRayCastFunction *function = NewFunction();
    mapper->SetRayCastFunction(function);
    function->Delete();
  }
  if (NewIntegrator)
  {
    vtkUnstructuredGridVolumeRayIntegrator *integrator = NewIntegrator();
    mapper->SetRayIntegrator(integrator);
    integrator->Delete();
  }

  // The volume holds the mapper and property and can be used to position/orient
  // the volume.
  vtkVolume *volume = vtkVolume::New();
  volume->SetMapper(mapper);
  if (!UseDependentComponents)
  {
    volume->SetProperty(volumeProperty);
  }
  else
  {
    // Set up a volume property that does not have the transfer function.
    vtkVolumeProperty *vp = vtkVolumeProperty::New();
    vp->SetShade(volumeProperty->GetShade());
    vp->SetInterpolationType(volumeProperty->GetInterpolationType());
    vp->SetScalarOpacityUnitDistance(
                                volumeProperty->GetScalarOpacityUnitDistance());
    vp->IndependentComponentsOff();
    volume->SetProperty(vp);
    vp->Delete();
  }

  // Add the volume to the renderer.
  ren->AddVolume(volume);

  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(20.0);
  ren->GetActiveCamera()->Elevation(15.0);
  ren->GetActiveCamera()->Zoom(1.5);

  // Delete objects.  Will not actually be destroyed due to reference counting.
  input->Delete();
  trifilter->Delete();
  if (celldata) celldata->Delete();
  if (calc) calc->Delete();
  if (assign) assign->Delete();
  if (classify) classify->Delete();
  mapper->Delete();
  volume->Delete();

  return ren;
}

//-----------------------------------------------------------------------------

static vtkRenderer *NewPlaceholderViewport()
{
  vtkRenderer *ren = vtkRenderer::New();

  vtkCubeSource *cube = vtkCubeSource::New();

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(0, cube->GetOutputPort(0));
  cube->Delete();

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  mapper->Delete();

  ren->AddActor(actor);
  ren->ResetCamera();
  actor->Delete();

  return ren;
}

//-----------------------------------------------------------------------------

static vtkVolumeProperty *NewRGBVolumeProperty()
{
  // Create transfer mapping scalar value to opacity.
  vtkPiecewiseFunction *opacityTransferFunction = vtkPiecewiseFunction::New();
  opacityTransferFunction->AddPoint( 0.0, 0.0);
  opacityTransferFunction->AddPoint(10.0, 1.0);

  // Create transfer mapping scalar value to color.
  vtkColorTransferFunction *colorTransferFunction
    = vtkColorTransferFunction::New();
  colorTransferFunction->SetColorSpaceToHSV();
  colorTransferFunction->HSVWrapOn();
  colorTransferFunction->AddHSVPoint( 0.0, 4.0/6.0, 1.0, 1.0);
  colorTransferFunction->AddHSVPoint( 4.0, 2.0/6.0, 1.0, 1.0);
  colorTransferFunction->AddHSVPoint( 6.0, 1.0/6.0, 1.0, 1.0);
  colorTransferFunction->AddHSVPoint(10.0, 5.0/6.0, 1.0, 1.0);

  vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetScalarOpacityUnitDistance(0.75);

  // Delete objects.  Will not actually be destroyed due to reference counting.
  opacityTransferFunction->Delete();
  colorTransferFunction->Delete();

  return volumeProperty;
}

//-----------------------------------------------------------------------------

static vtkVolumeProperty *NewGrayVolumeProperty()
{
  // Create transfer mapping scalar value to opacity.
  vtkPiecewiseFunction *opacityTransferFunction = vtkPiecewiseFunction::New();
  opacityTransferFunction->AddPoint( 0.0, 0.0);
  opacityTransferFunction->AddPoint(10.0, 1.0);

  // Create transfer mapping scalar value to color.
  vtkPiecewiseFunction *grayTransferFunction = vtkPiecewiseFunction::New();
  grayTransferFunction->AddPoint( 0.0, 0.0);
  grayTransferFunction->AddPoint(10.0, 1.0);

  vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
  volumeProperty->SetColor(grayTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetScalarOpacityUnitDistance(0.75);

  // Delete objects.  Will not actually be destroyed due to reference counting.
  opacityTransferFunction->Delete();
  grayTransferFunction->Delete();

  return volumeProperty;
}

//-----------------------------------------------------------------------------

static vtkVolumeProperty *NewMultiTFVolumeProperty()
{
  vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetScalarOpacityUnitDistance(0.75);

  vtkColorTransferFunction *rgb;
  vtkPiecewiseFunction *a;

  // Create first tf.
  rgb = vtkColorTransferFunction::New();
  rgb->AddRGBPoint(0.0, 1.0, 0.0, 0.0);
  volumeProperty->SetColor(0, rgb);
  rgb->Delete();

  a = vtkPiecewiseFunction::New();
  a->AddPoint(2.9, 0.0);
  a->AddPoint(3.0, 1.0);
  volumeProperty->SetScalarOpacity(0, a);
  a->Delete();

  // Create second tf.
  rgb = vtkColorTransferFunction::New();
  rgb->AddRGBPoint(0.0, 0.0, 1.0, 1.0);
  volumeProperty->SetColor(1, rgb);
  rgb->Delete();

  a = vtkPiecewiseFunction::New();
  a->AddPoint(4.9, 0.0);
  a->AddPoint(5.0, 0.5);
  volumeProperty->SetScalarOpacity(1, a);
  a->Delete();

  // Create third tf.
  rgb = vtkColorTransferFunction::New();
  rgb->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  volumeProperty->SetColor(2, rgb);
  rgb->Delete();

  a = vtkPiecewiseFunction::New();
  a->AddPoint(0.0, 0.0);
  volumeProperty->SetScalarOpacity(2, a);
  a->Delete();

  return volumeProperty;
}

//-----------------------------------------------------------------------------

int ExerciseUnstructuredGridRayCastMapper(int argc, char *argv[],
                                          RayCastFunctionCreator NewFunction,
                                          RayIntegratorCreator NewIntegrator,
                                          int UseCellData,
                                          int TestDependentComponents)
{
  // Create the standard render window and interactor.
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetSize(300, 300);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  iren->SetDesiredUpdateRate(3.0);

  // The property describes how the data will look.  Establish various
  // rendering modes with the property and its transfer functions.
  vtkVolumeProperty *volumeProperty;
  vtkRenderer *viewport;

  // RGB transfer function.
  volumeProperty = NewRGBVolumeProperty();
  viewport = NewTestViewport(NewFunction, NewIntegrator, volumeProperty,
                             UseCellData, 0, 0);
  if (!viewport) return -1;
  viewport->SetViewport(0.0, 0.0, 0.5, 0.5);
  renWin->AddRenderer(viewport);
  volumeProperty->Delete();
  viewport->Delete();

  // Gray transfer function.
  volumeProperty = NewGrayVolumeProperty();
  viewport = NewTestViewport(NewFunction, NewIntegrator, volumeProperty,
                             UseCellData, 0, 0);
  if (!viewport) return -1;
  viewport->SetViewport(0.5, 0.0, 1.0, 0.5);
  renWin->AddRenderer(viewport);
  volumeProperty->Delete();
  viewport->Delete();

  if (TestDependentComponents)
  {
    // RGBA dependent components.
    volumeProperty = NewRGBVolumeProperty();
    viewport = NewTestViewport(NewFunction, NewIntegrator, volumeProperty,
                               UseCellData, 1, 0);
    volumeProperty->Delete();
  }
  else
  {
    viewport = NewPlaceholderViewport();
  }
  if (!viewport) return -1;
  viewport->SetViewport(0.0, 0.5, 0.5, 1.0);
  renWin->AddRenderer(viewport);
  viewport->Delete();

  // Multiple transfer functions
  volumeProperty = NewMultiTFVolumeProperty();
  viewport = NewTestViewport(NewFunction, NewIntegrator, volumeProperty,
                             UseCellData, 0, 1);
  if (!viewport) return -1;
  viewport->SetViewport(0.5, 0.5, 1.0, 1.0);
  renWin->AddRenderer(viewport);
  volumeProperty->Delete();
  viewport->Delete();

  int retVal = vtkRegressionTestImageThreshold(renWin, 70);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Delete objects.
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
