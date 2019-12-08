/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositePolyDataMapper2SharedArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkAppendFilter.h>
#include <vtkCellArray.h>
#include <vtkCellArrayIterator.h>
#include <vtkCompositeDataIterator.h>
#include <vtkCompositePolyDataMapper2.h>
#include <vtkCubeSource.h>
#include <vtkDataObjectTreeIterator.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataGroupFilter.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiBlockDataSetAlgorithm.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkRandomAttributeGenerator.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkUnstructuredGrid.h>

class vtkDualCubeSource : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkDualCubeSource* New();
  vtkTypeMacro(vtkDualCubeSource, vtkMultiBlockDataSetAlgorithm);

protected:
  vtkDualCubeSource() { this->SetNumberOfInputPorts(0); }

  ~vtkDualCubeSource() override = default;

  int RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    // get the output
    vtkMultiBlockDataSet* output =
      vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    vtkNew<vtkCubeSource> cube1;
    vtkNew<vtkRandomAttributeGenerator> id1;
    id1->SetDataTypeToFloat();
    id1->GeneratePointScalarsOn();
    id1->GenerateCellScalarsOn();
    id1->SetInputConnection(cube1->GetOutputPort());

    vtkNew<vtkCubeSource> cube2;
    cube2->SetCenter(1.5, 0., 0.);
    vtkNew<vtkRandomAttributeGenerator> id2;
    id2->SetInputConnection(cube2->GetOutputPort());
    id2->SetDataTypeToFloat();
    id2->GeneratePointScalarsOn();
    id2->GenerateCellScalarsOn();

    vtkNew<vtkCubeSource> cube3;
    cube3->SetCenter(0.75, -1.5, 0.);
    vtkNew<vtkRandomAttributeGenerator> id3;
    id3->SetInputConnection(cube3->GetOutputPort());
    id3->SetDataTypeToFloat();
    id3->GeneratePointScalarsOn();
    id3->GenerateCellScalarsOn();
    id3->Update();

    // Append geometry of the two first meshes
    vtkNew<vtkAppendFilter> append;
    append->AddInputConnection(id1->GetOutputPort());
    append->AddInputConnection(id2->GetOutputPort());
    append->Update();
    vtkUnstructuredGrid* aug = append->GetOutput();

    // Transfer appended geometry (not topology) to first and second meshes
    vtkPolyData* pd1 = vtkPolyData::SafeDownCast(id1->GetOutput());
    vtkIdType cube1npts = pd1->GetNumberOfPoints();
    pd1->SetPoints(aug->GetPoints());
    pd1->GetPointData()->ShallowCopy(aug->GetPointData());

    vtkPolyData* pd2 = vtkPolyData::SafeDownCast(id2->GetOutput());
    pd2->SetPoints(aug->GetPoints());
    pd2->GetPointData()->ShallowCopy(aug->GetPointData());

    { // Update connectivity of second mesh by shifting point ids
      vtkCellArray* polys = pd2->GetPolys();
      auto cellIter = vtk::TakeSmartPointer(polys->NewIterator());
      vtkNew<vtkIdList> cell;
      for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
      {
        cellIter->GetCurrentCell(cell);
        for (vtkIdType i = 0; i < cell->GetNumberOfIds(); i++)
        {
          cell->SetId(i, cell->GetId(i) + cube1npts);
        }
        cellIter->ReplaceCurrentCell(cell);
      }
    }

    // Create the multiblock dataset with the different meshes
    vtkNew<vtkMultiBlockDataGroupFilter> group;
    group->AddInputData(pd1);
    group->AddInputData(id3->GetOutput()); // This mesh has different arrays than the other two
    group->AddInputData(pd2);
    group->Update();

    output->ShallowCopy(group->GetOutput());
    return 1;
  }

private:
  vtkDualCubeSource(const vtkDualCubeSource&) = delete;
  void operator=(const vtkDualCubeSource&) = delete;
};
vtkStandardNewMacro(vtkDualCubeSource);

int TestCompositePolyDataMapper2SharedArray(int argc, char* argv[])
{
  vtkNew<vtkDualCubeSource> source;

  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  vtkNew<vtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(source->GetOutputPort());
  mapper->SetScalarModeToUsePointData();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  renderer->AddActor(actor);
  renderer->SetBackground(.3, .4, .5);

  renderer->ResetCamera();

  int retVal = vtkRegressionTestImageThreshold(renderWindow, 15);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return !retVal;
}
