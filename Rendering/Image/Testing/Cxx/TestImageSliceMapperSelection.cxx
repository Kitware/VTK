// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Test hardware selection for vtkImageSliceMapper rendered content
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkRendererCollection.h"
#include "vtkSmartPointer.h"
#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractSelection.h>
#include <vtkGeometryFilter.h>
#include <vtkHardwareSelector.h>
#include <vtkIdTypeArray.h>
#include <vtkImageData.h>
#include <vtkImageSlice.h>
#include <vtkImageSliceMapper.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOpenGLImageSliceMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkTransform.h>
#include <vtkUnsignedCharArray.h>

#include <iostream>

// Callback to highlight selected area in a 5x5 neighborhood of the mouse click
class SelectionCallback : public vtkCommand
{
public:
  static SelectionCallback* New() { return new SelectionCallback; }
  void SetImageSlice(vtkImageSlice* slice) { this->ImageSlice = slice; }
  void SetRenderer(vtkRenderer* renderer) { this->Renderer = renderer; }
  void SetImageData(vtkImageData* imageData) { this->ImageData = imageData; }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkRenderWindowInteractor* iren = static_cast<vtkRenderWindowInteractor*>(caller);
    int eventPos[2];
    iren->GetEventPosition(eventPos);
    // Define a 3x3 pixel area centered at the mouse click
    int x0 = eventPos[0] - 1;
    int y0 = eventPos[1] - 1;
    int x1 = eventPos[0] + 1;
    int y1 = eventPos[1] + 1;
    // Clamp to render window size
    int* size = iren->GetRenderWindow()->GetSize();
    x0 = std::max(0, x0);
    y0 = std::max(0, y0);
    x1 = std::min(size[0] - 1, x1);
    y1 = std::min(size[1] - 1, y1);

    vtkRenderer* renderer = iren->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
    vtkNew<vtkHardwareSelector> selector;
    selector->SetActorPassOnly(false);
    selector->SetRenderer(renderer);
    selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
    selector->SetArea(x0, y0, x1, y1);
    vtkSmartPointer<vtkSelection> selection = vtk::TakeSmartPointer(selector->Select());
    if (selection && selection->GetNumberOfNodes() > 0)
    {
      vtkSelectionNode* node = selection->GetNode(0);
      vtkIdTypeArray* ids = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      if (ids)
      {
        std::cout << "Selected cell IDs: ";
        for (vtkIdType i = 0; i < ids->GetNumberOfTuples(); ++i)
        {
          vtkIdType cellId = ids->GetValue(i);
          std::cout << cellId << " ";
        }
        std::cout << std::endl;
      }
      else
      {
        std::cout << "No cell IDs found in selection list." << std::endl;
      }
      // Extract the selected cells from the input image
      vtkNew<vtkExtractSelection> extractSelection;
      extractSelection->SetInputData(0, this->ImageData);
      extractSelection->SetInputData(1, selection);

      // Convert the extracted selection to geometry
      vtkNew<vtkGeometryFilter> geometryFilter;
      geometryFilter->SetInputConnection(extractSelection->GetOutputPort());
      geometryFilter->Update();

      this->OverlayMapper->SetInputConnection(geometryFilter->GetOutputPort());
      this->OverlayMapper->SetScalarVisibility(0);
      this->OverlaySlice->SetMapper(this->OverlayMapper);
      this->OverlaySlice->GetProperty()->SetColor(1.0, 0.0, 0.5); // Magenta
      this->Renderer->AddViewProp(this->OverlaySlice);
    }
    iren->GetRenderWindow()->Render();
  }

private:
  vtkImageSlice* ImageSlice = nullptr;
  vtkRenderer* Renderer = nullptr;
  vtkImageData* ImageData = nullptr;
  vtkNew<vtkActor> OverlaySlice;
  vtkNew<vtkPolyDataMapper> OverlayMapper;
};

int TestImageSliceMapperSelection(int argc, char* argv[])
{
  // Create a simple image
  vtkNew<vtkImageData> image;
  image->SetDimensions(100, 100, 1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  int centerX = 50;
  int centerY = 50;
  double maxDist = std::sqrt(centerX * centerX + centerY * centerY);
  for (int y = 0; y < 100; ++y)
  {
    for (int x = 0; x < 100; ++x)
    {
      unsigned char* pixel = static_cast<unsigned char*>(image->GetScalarPointer(x, y, 0));
      double dx = x - centerX;
      double dy = y - centerY;
      double dist = std::sqrt(dx * dx + dy * dy);
      // Map distance to 0-255 (center is 255, edge is 0)
      unsigned char value = static_cast<unsigned char>(255 * (1.0 - dist / maxDist));
      pixel[0] = value;
    }
  }

  vtkNew<vtkImageSliceMapper> mapper;
  mapper->SetInputData(image);

  vtkNew<vtkImageSlice> slice;
  slice->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddViewProp(slice);
  renderer->SetBackground(1, 1, 1);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  interactor->SetInteractorStyle(style);

  // Set up hardware selector
  vtkNew<vtkHardwareSelector> selector;
  selector->SetRenderer(renderer);
  // Select a single pixel near the center of the image
  int x = 65, y = 86;
  selector->SetArea(x - 1, y - 1, x + 1, y + 1);
  selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
  renderWindow->Render();
  vtkSmartPointer<vtkSelection> selection = vtk::TakeSmartPointer(selector->Select());
  vtkSmartPointer<vtkIdTypeArray> ids;
  if (selection && selection->GetNumberOfNodes() > 0)
  {
    if (selection->GetNumberOfNodes() != 1)
    {
      std::cerr << "Unexpected number of selection nodes: " << selection->GetNumberOfNodes()
                << std::endl;
      return EXIT_FAILURE;
    }
    vtkSmartPointer<vtkSelectionNode> node = selection->GetNode(0);
    if (!node || node->GetSelectionList() == nullptr)
    {
      std::cerr << "Selection node or selection list is null!" << std::endl;
      return EXIT_FAILURE;
    }
    ids = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
    if (ids == nullptr)
    {
      std::cerr << "Selection list is not a vtkIdTypeArray!" << std::endl;
      return EXIT_FAILURE;
    }
    if (ids->GetNumberOfTuples() == 0)
    {
      std::cerr << "No cells selected!" << std::endl;
      return EXIT_FAILURE;
    }
    if (ids->GetNumberOfTuples() != 4)
    { // Expecting 4 cells for a 3x3 pixel area
      std::cerr << "Unexpected number of selected cells. Expected 4. Got "
                << ids->GetNumberOfTuples() << std::endl;
      return EXIT_FAILURE;
    }
    vtkIdType expectedIds[4] = { 1790, 1791, 1889, 1890 };
    std::cout << "Selected cell IDs: ";
    for (vtkIdType i = 0; i < ids->GetNumberOfTuples(); ++i)
    {
      std::cout << ids->GetValue(i) << " ";
      if (ids->GetValue(i) != expectedIds[i])
      {
        std::cerr << "Selected cellId: " << ids->GetValue(i) << ", expected: " << expectedIds[i]
                  << std::endl;
        return EXIT_FAILURE;
      }
    }
  }
  else
  {
    std::cerr << "No selection made!" << std::endl;
    return EXIT_FAILURE;
  }

  // Now highlight a larger area around the selection
  selector->SetArea(x - 15, y - 15, x + 15, y + 15);
  selection = vtk::TakeSmartPointer(selector->Select());
  vtkSmartPointer<vtkSelectionNode> node = selection->GetNode(0);
  ids = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());

  // Highlight the selected cells using a different color
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(3); // RGB
  colors->SetName("Colors");

  vtkIdType numCells = ids->GetNumberOfTuples();
  colors->SetNumberOfTuples(numCells);

  for (vtkIdType i = 0; i < numCells; ++i)
  {
    if (ids->GetValue(i) != -1)
    {
      // Selected cell: highlight color (e.g., orange)
      unsigned char color[3] = { 255, 155, 0 };
      colors->SetTypedTuple(i, color);
    }
    else
    {
      // Non-selected cell: default color (e.g., white)
      unsigned char color[3] = { 255, 255, 255 };
      colors->SetTypedTuple(i, color);
    }
  }

  // Create a new vtkImageData for the selected cells
  vtkNew<vtkImageData> selectedImageData;

  // Determine the bounds and origin for the selected cells
  int minX = INT_MAX, minY = INT_MAX, minZ = INT_MAX;
  int maxX = INT_MIN, maxY = INT_MIN, maxZ = INT_MIN;

  for (vtkIdType i = 0; i < numCells; ++i)
  {
    if (ids->GetValue(i) != -1)
    {
      int ijk[3];
      // Assuming 'image' is a vtkImageData* and 'ijk' is an int[3]
      int dims[3];
      image->GetDimensions(dims);
      // Compute IJK indices for cell ii
      int ii = ids->GetValue(i);
      int k = ii / ((dims[0] - 1) * (dims[1] - 1));
      int j = (ii % ((dims[0] - 1) * (dims[1] - 1))) / (dims[0] - 1);
      int idi = ii % (dims[0] - 1);
      ijk[0] = idi;
      ijk[1] = j;
      ijk[2] = k;
      // method to get cell coordinates
      minX = std::min(minX, ijk[0]);
      minY = std::min(minY, ijk[1]);
      minZ = std::min(minZ, ijk[2]);
      maxX = std::max(maxX, ijk[0]);
      maxY = std::max(maxY, ijk[1]);
      maxZ = std::max(maxZ, ijk[2]);
    }
  }

  // Set the origin and extent for the new image data
  selectedImageData->SetOrigin(image->GetOrigin()[0] + minX * image->GetSpacing()[0],
    image->GetOrigin()[1] + minY * image->GetSpacing()[1],
    image->GetOrigin()[2] + minZ * image->GetSpacing()[2]);
  selectedImageData->SetSpacing(image->GetSpacing());
  selectedImageData->SetExtent(0, maxX - minX, 0, maxY - minY, 0, maxZ - minZ);

  selectedImageData->GetCellData()->SetScalars(colors);

  // Create a vtkActor for the overlay
  vtkNew<vtkDataSetMapper> overlayMapper;
  overlayMapper->SetInputData(selectedImageData);

  vtkNew<vtkActor> overlayActor;
  overlayActor->SetMapper(overlayMapper);
  // overlayActor->GetProperty()->SetOpacity(0.5); // Set overlay transparency

  // Add the overlay actor to the renderer
  renderer->AddActor(overlayActor);

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<SelectionCallback> callback;
    callback->SetImageSlice(slice);
    callback->SetRenderer(renderer);
    callback->SetImageData(image);
    interactor->AddObserver(vtkCommand::LeftButtonPressEvent, callback);
    interactor->Start();
  }

  return EXIT_SUCCESS;
}
