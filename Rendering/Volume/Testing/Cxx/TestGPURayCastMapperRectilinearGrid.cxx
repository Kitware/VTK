// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This is a basic test that renders a rectilinear grid dataset with the GPU ray cast volume mapper.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataSetMapper.h"
#include "vtkDoubleArray.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridReader.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTypeFloat64Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

namespace TestGPURayCastMapperRectilinearGridNS
{
//----------------------------------------------------------------------------
vtkSmartPointer<vtkRectilinearGrid> ModifyGridSpacing(
  vtkSmartPointer<vtkRectilinearGrid> input, int direction)
{
  if (!input)
  {
    return nullptr;
  }

  vtkNew<vtkRectilinearGrid> output;
  output->DeepCopy(input);

  vtkNew<vtkDoubleArray> newCoords;
  newCoords->SetNumberOfComponents(1);

  vtkSmartPointer<vtkDataArray> coords = nullptr;
  switch (direction)
  {
    case 0:
      coords = input->GetXCoordinates();
      output->SetXCoordinates(newCoords);
      break;
    case 1:
      coords = input->GetYCoordinates();
      output->SetYCoordinates(newCoords);
      break;
    case 2:
    default:
      coords = input->GetZCoordinates();
      output->SetZCoordinates(newCoords);
      break;
  }

  vtkNew<vtkMinimalStandardRandomSequence> seq;
  seq->SetSeed(203542);
  newCoords->InsertNextTuple1(coords->GetTuple1(0));
  int i = 1;
  for (; i < coords->GetNumberOfTuples() - 1; i++)
  {
    seq->Next();
    // double val = newCoords->GetTuple1(i - 1);
    double val = i * 0.08 * seq->GetValue() * coords->GetTuple1(i);
    newCoords->InsertNextTuple1(val);
  }
  newCoords->InsertNextTuple1(i * 0.08 * coords->GetTuple1(i));
  return output;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkRectilinearGrid> ModifyDataType(
  vtkSmartPointer<vtkRectilinearGrid> input, int type_case)
{
  vtkNew<vtkRectilinearGrid> output;
  output->ShallowCopy(input);
  if (type_case == 0)
  {
    vtkNew<vtkTypeInt64Array> lscalars;
    lscalars->DeepCopy(input->GetPointData()->GetScalars());
    output->GetPointData()->AddArray(lscalars);
  }
  else
  {
    vtkNew<vtkTypeFloat64Array> lscalars;
    lscalars->DeepCopy(input->GetPointData()->GetScalars());
    output->GetPointData()->AddArray(lscalars);
  }
  return output;
}

} // end namespace TestGPURayCastMapperRectilinearGridNS

//----------------------------------------------------------------------------
int TestGPURayCastMapperRectilinearGrid(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/RectGrid2.vtk");

  vtkNew<vtkRectilinearGridReader> reader;
  reader->SetFileName(fname);
  reader->Update();

  vtkSmartPointer<vtkRectilinearGrid> rGrid = reader->GetOutput();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 450); // Intentional NPOT size

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.53, 0.53, 0.83);
  ctf->AddRGBPoint(1.77, 0, 0, 1);
  ctf->AddRGBPoint(3.53, 0, 1, 1);
  ctf->AddRGBPoint(5.2, 0, 1, 0);
  ctf->AddRGBPoint(6.97, 1, 1, 0);
  ctf->AddRGBPoint(8.73, 1, 0, 0);
  ctf->AddRGBPoint(10.39, 0.88, 0, 1);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0);
  pf->AddPoint(0.2, 1);
  pf->AddPoint(3, 0.5);
  pf->AddPoint(10.39, 1);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(ctf);
  volumeProperty->SetScalarOpacity(pf);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper[6];
  vtkNew<vtkVolume> volume[6];
  vtkNew<vtkRenderer> ren[6];
  ren[0]->SetViewport(0, 0, 0.5, 0.33);
  ren[1]->SetViewport(0.5, 0, 1, 0.33);
  ren[2]->SetViewport(0, 0.33, 0.5, 0.66);
  ren[3]->SetViewport(0.5, 0.33, 1, 0.66);
  ren[4]->SetViewport(0, 0.66, 0.5, 1);
  ren[5]->SetViewport(0.5, 0.66, 1, 1);
  vtkNew<vtkDataSetMapper> dsMapper[6];
  vtkNew<vtkActor> dsActor[6];

  for (int i = 0; i < 6; ++i)
  {
    mapper[i]->UseJitteringOn();
    if (i == 0)
    {
      mapper[i]->SetInputData(rGrid);
      dsMapper[i]->SetInputData(rGrid);
    }
    else if (i < 4)
    {
      vtkSmartPointer<vtkRectilinearGrid> newGrid =
        TestGPURayCastMapperRectilinearGridNS::ModifyGridSpacing(rGrid, i - 1);
      mapper[i]->SetInputData(newGrid);
      dsMapper[i]->SetInputData(newGrid);
    }
    else
    {
      vtkSmartPointer<vtkRectilinearGrid> newGrid =
        TestGPURayCastMapperRectilinearGridNS::ModifyDataType(rGrid, i - 4);
      mapper[i]->SetInputData(newGrid);
      dsMapper[i]->SetInputData(newGrid);
    }
    volume[i]->SetProperty(volumeProperty);
    volume[i]->SetMapper(mapper[i]);
    renWin->AddRenderer(ren[i]);
    dsMapper[i]->SetScalarVisibility(0);
    dsActor[i]->SetMapper(dsMapper[i]);
    dsActor[i]->GetProperty()->SetRepresentationToWireframe();
    dsActor[i]->GetProperty()->SetOpacity(0.5);
    ren[i]->AddActor(dsActor[i]);
    ren[i]->AddViewProp(volume[i]);
    ren[i]->ResetCamera();

    auto camera = ren[i]->GetActiveCamera();
    camera->Pitch(30);
    ren[i]->ResetCamera();
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  renWin->Render();
  delete[] fname;
  return vtkTesting::InteractorEventLoop(argc, argv, iren);
}
