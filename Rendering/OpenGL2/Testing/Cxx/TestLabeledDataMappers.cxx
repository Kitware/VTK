// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// this test verifies that vtkFastLabelDataMapper works as expected

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkExtractSelection.h"
#include "vtkFastLabeledDataMapper.h"
#include "vtkFloatArray.h"
#include "vtkGenerateIds.h"
#include "vtkGeometryFilter.h"
#include "vtkNew.h"
#include "vtkPassThrough.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTrivialProducer.h"

#include "vtkDataSetAttributes.h"
#include "vtkDataSetMapper.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkProperty.h"
#include "vtkRendererCollection.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include <array>

namespace
{
struct TestContextData
{
  // Fast Labels
  vtkNew<vtkPlaneSource> plane;
  vtkNew<vtkTransformPolyDataFilter> xform;
  vtkNew<vtkTransform> matrix;
  vtkNew<vtkGenerateIds> ids;
  vtkNew<vtkFastLabeledDataMapper> labelMapper;
  vtkNew<vtkActor> labelActor;

  // Filtering Polydata Source Data
  vtkNew<vtkExtractSelection> filter;
  vtkNew<vtkFastLabeledDataMapper> filteredLabelMapper;
  vtkNew<vtkActor> filteredActor;

  // Origin Points
  vtkNew<vtkPolyDataMapper> originPointMapper;
  vtkNew<vtkActor> originPointActor;

  // Status Text
  vtkNew<vtkTextActor> statusTextLabelActor;
};

std::unique_ptr<TestContextData> testContext;

class ScopedTestContextInitializer
{
public:
  ScopedTestContextInitializer() { testContext.reset(new TestContextData()); }

  ~ScopedTestContextInitializer() { testContext.reset(nullptr); }
};

const char* LABEL_TYPES = "types";
const char* LABEL_TEXT_NAMES = "names";
const char* LABEL_FRAMES = "frames";

//-----------------------------------------------------------------------------
void AddTextProperty(vtkSmartPointer<vtkFastLabeledDataMapper> mapper, int idx, int font,
  int fontSize, int frameWidth, std::array<double, 4> color, std::array<double, 4> bgColor,
  std::array<double, 4> frameColor)
{
  vtkNew<vtkTextProperty> tprop;
  tprop->SetFontFamily(font);
  tprop->SetColor(color.data());
  tprop->SetOpacity(color[3]);
  tprop->SetBackgroundColor(bgColor.data());
  tprop->SetBackgroundOpacity(bgColor[3]);
  tprop->SetFontSize(fontSize);
  tprop->SetFrame(frameWidth > 0);
  tprop->SetFrameWidth(frameWidth);
  tprop->SetFrameColor(frameColor.data()); // alpha unused
  mapper->SetLabelTextProperty(tprop, idx);
}

//-----------------------------------------------------------------------------
void AddTextProperties(vtkSmartPointer<vtkFastLabeledDataMapper> mapper)
{
  AddTextProperty(
    mapper, 0, VTK_TIMES, 24, 2, { 1., 0., 0., 1. }, { 0., 1., .0, 1. }, { .0, .0, .1, 1. });
  AddTextProperty(
    mapper, 1, VTK_ARIAL, 24, 4, { 1., 1., 1., 1. }, { .2, 1., .2, 1. }, { .1, .6, .6, 1. });
  AddTextProperty(
    mapper, 2, VTK_COURIER, 24, 8, { 0., 0., 0., 1. }, { .8, 1., .8, 1. }, { .8, .2, .2, 1. });
  AddTextProperty(
    mapper, 3, VTK_ARIAL, 12, 1, { .8, 1., .2, 1. }, { .1, .4, .2, 1. }, { 0., 0., 0., 1. });
  AddTextProperty(
    mapper, 4, VTK_ARIAL, 32, 4, { .5, .5, .2, 1. }, { 0., 0., 1., 1. }, { .8, .5, .3, 1. });
  AddTextProperty(
    mapper, 5, VTK_TIMES, 16, 3, { 1., .2, 1., 1. }, { .2, 1., .6, 1. }, { .1, 0., .3, 1. });
  AddTextProperty(
    mapper, 6, VTK_COURIER, 18, 0, { 1., 1., 1., 1. }, { 0., 0., 0., 0. }, { 0., 0., 0., 0. });
  AddTextProperty(
    mapper, 7, VTK_COURIER, 22, 1, { 0., 0., 0., 1. }, { .2, 1., .2, 1. }, { 0., 0., 0., 1. });
  AddTextProperty(
    mapper, 8, VTK_TIMES, 18, 1, { 0., 1., 1., 1. }, { 0., 0., 0., 1. }, { 1., 1., 1., 1. });
  AddTextProperty(
    mapper, 9, VTK_ARIAL, 24, 4, { 1., .5, .5, 1. }, { .5, .5, 1., 1. }, { .5, 1., .5, 1. });
}

//-----------------------------------------------------------------------------
void UpdatePlaneArrays(int prefix = 0)
{
  testContext->plane->Update();
  auto dataset = testContext->plane->GetOutput();
  auto pointData = dataset->GetPointData();

  vtkNew<vtkIntArray> types;
  types->SetNumberOfComponents(1);
  types->SetName(LABEL_TYPES);
  vtkNew<vtkStringArray> names;
  names->SetName(LABEL_TEXT_NAMES);
  char buf[30];
  vtkNew<vtkFloatArray> frames;
  frames->SetNumberOfComponents(3);
  frames->SetName(LABEL_FRAMES);

  for (int i = 0; i < dataset->GetNumberOfPoints(); i++)
  {
    types->InsertNextValue(i % 10);
    if (prefix > 0)
    {
      snprintf(buf, 30, "%d_Z_%d_a", prefix, i);
    }
    else
    {
      snprintf(buf, 30, "Z_%d_a", i);
    }
    names->InsertNextValue(buf);
    double v = i / (double)dataset->GetNumberOfPoints();
    frames->InsertNextTuple3(v, v, v);
  }

  pointData->AddArray(types);
  pointData->AddArray(names);
  pointData->AddArray(frames);

  testContext->xform->Modified();
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> GetFilteredPolyDataInput()
{
  vtkNew<vtkPolyData> polyData;
  auto pointData = polyData->GetPointData();
  vtkNew<vtkIntArray> types;
  types->SetNumberOfComponents(1);
  types->SetName(LABEL_TYPES);
  vtkNew<vtkStringArray> names;
  names->SetName(LABEL_TEXT_NAMES);
  pointData->AddArray(types);
  pointData->AddArray(names);

  vtkNew<vtkPoints> points;
  polyData->SetPoints(points);

  int numPoints = 11;
  char buf[30];
  for (int i = 0; i < numPoints; i++)
  {
    types->InsertNextValue(i % 10);
    snprintf(buf, 30, "FPD_%d", i);
    names->InsertNextValue(buf);
    points->InsertNextPoint((i * 0.1) - 0.5, 0.8, 0.0);
  }

  return polyData;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkSelection> GetFilterSelection(int startIndex = 0, int endIndex = 10)
{
  vtkNew<vtkIdTypeArray> indicesSet;
  for (int i = startIndex; i <= endIndex; i++)
  {
    indicesSet->InsertNextValue(i);
  }

  vtkNew<vtkSelectionNode> selectionNode;
  selectionNode->SetFieldType(vtkSelectionNode::POINT);
  selectionNode->SetContentType(vtkSelectionNode::INDICES);
  selectionNode->SetSelectionList(indicesSet);

  vtkNew<vtkSelection> selection;
  selection->AddNode(selectionNode);
  return selection;
}

//-----------------------------------------------------------------------------
class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static KeyPressInteractorStyle* New();
  vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

  static void PrintControls()
  {
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "\t s: Select" << std::endl;
    std::cout << "\t C/c: Change Font" << std::endl;
    std::cout << "\t M/m: More/Less Data" << std::endl;
    std::cout << "\t B/b: Bigger/Smaller Transform" << std::endl;
    std::cout << "\t G/g: Bigger/Smaller Font Size" << std::endl;
    std::cout << "\t P/p: Toggles Perspective and Projection" << std::endl;
    std::cout << "\t i: Increment Prefix" << std::endl;

    std::cout << "Filtered PolyData:" << std::endl;
    std::cout << "\t T/t: Toggle PolyData Filter" << std::endl;

    std::cout << "" << std::endl;
    std::cout << "\t q: Quit" << std::endl;
    std::cout << std::endl;
  }

  void OnKeyPress() override
  {
    // Get the keypress
    std::string key = this->Interactor->GetKeySym();

    // "s" for "s"elect
    if (key == "s")
    {
      Select();
    }

    // Fonts
    static int cnt = -1;
    if (key == "c") // change font selection
    {
      cnt = (cnt - 1) % 7;
      while (cnt < 0)
      {
        cnt += 7;
      }
      FontChange(cnt);
    }
    if (key == "C") // change font selection
    {
      cnt = (cnt + 1) % 7;
      FontChange(cnt);
    }

    // Multiply Data
    if (key == "M") // "M" for "M"ore data
    {
      MultiplyData(2.0);
    }
    if (key == "m") // "m" for "l"ess data?
    {
      MultiplyData(0.5);
    }

    // Scale
    if (key == "B") // "B" for "B"igger transform
    {
      Scale(2.0);
    }
    if (key == "b") // "b" for "s"maller transform?
    {
      Scale(0.5);
    }

    // Font Size
    if (key == "G") // "G" for bigger font size
    {
      FontSize(6);
    }
    if (key == "g") // "g" for smaller font size
    {
      FontSize(-6);
    }

    // "P" or "p" to toggle Perspective and Projection.
    if (key == "P" || key == "p")
    {
      PerspectiveToggle();
    }

    // "I" to "I"ncrement general text prefixes
    if (key == "I" || key == "i")
    {
      static int prefix = 0;
      std::cout << "Incrementing Prefix to " << ++prefix << std::endl;
      UpdatePlaneArrays(prefix);
    }

    // "T" toggle polydata filtering
    if (key == "T" || key == "t")
    {
      FilterToggle();
    }

    // Forward events
    vtkInteractorStyleTrackballCamera::OnKeyPress();
    this->Renderer->GetRenderWindow()->Render();
  }

  //-----------------------------------------------------------------------------
  void FontChange(int cnt)
  {
    vtkNew<vtkTextProperty> p;
    p->SetBackgroundColor(0.5, 0.5, 0.5);

    cout << "Font Change: ";
    switch (cnt)
    {
      case 0:
        std::cout << "Arial" << std::endl;
        p->SetFontFamilyAsString("Arial");
        p->SetColor(1.0, 1.0, 1.0);
        p->SetBackgroundColor(1.0, 0.0, 0.0);
        p->SetFontSize(24);
        break;
      case 1:
        std::cout << "Arial grey w blue frame" << std::endl;
        p->SetFontFamilyAsString("Arial");
        p->SetColor(0.5, 0.5, 0.5);
        p->SetBackgroundColor(1.0, 1.0, 1.0);
        p->SetBackgroundOpacity(1.0);
        p->SetFontSize(24);
        p->FrameOn();
        p->SetFrameWidth(4);
        p->SetFrameColor(0.0, 0.0, 0.8);
        break;
      case 2:
        std::cout << "Courier" << std::endl;
        p->SetFontFamilyAsString("Courier");
        p->SetColor(0.0, 1.0, 0.0);
        p->SetBackgroundColor(0.5, 0.0, 0.5);
        p->SetBackgroundOpacity(0.9);
        p->SetFontSize(32);
        break;
      case 3:
        std::cout << "Times" << std::endl;
        p->SetFontFamilyAsString("Times");
        p->SetColor(0.0, 0.0, 1.0);
        p->SetBackgroundColor(1.0, 0.7, 0.4);
        p->SetBackgroundOpacity(0.1);
        p->SetFontSize(38);
        break;
      case 4:
        std::cout << "Courier Frames" << std::endl;
        p->SetFontFamilyAsString("Courier");
        p->SetColor(0.0, 0.0, 1.0);
        p->SetFontSize(36);
        p->SetBackgroundColor(1.0, 0.5, 1.0);
        p->SetBackgroundOpacity(1.0);
        p->FrameOn();
        p->SetFrameWidth(2);
        break;
      case 5:
        std::cout << "Courier Frames BIGGER" << std::endl;
        p->SetFontFamilyAsString("Courier");
        p->SetColor(0.0, 0.0, 1.0);
        p->SetFontSize(64);
        p->SetBackgroundColor(1.0, 0.5, 1.0);
        p->SetFrameColor(0.0, 0.5, 0.5);
        p->SetBackgroundOpacity(1.0);
        p->FrameOn();
        p->SetFrameWidth(4);
        break;
      case 6:
        std::cout << "Inconsolata" << std::endl;
        p->SetFontFamily(VTK_FONT_FILE);
        // relative path in this example, so has to be in same directory user runs from
        p->SetFontFile("Inconsolata.otf");
        // see vtkResourceFileLocator for a pattern to follow to manufacture an
        // absolute path to look for instead for distributable binaries etc
        p->SetColor(1.0, 1.0, 1.0);
        p->SetFontSize(32);
        p->FrameOff();
        p->SetBackgroundColor(0.8, 0.0, 0.8);
        p->SetBackgroundOpacity(1.0);
        break;
      default:
        std::cout << "Invalid Index " << cnt << std::endl;
    }
    testContext->statusTextLabelActor->SetTextProperty(p);
    testContext->labelMapper->SetLabelTextProperty(p, 1);
  }

  //-----------------------------------------------------------------------------
  void FontSize(int sizeDelta)
  {
    vtkTextProperty* p = testContext->labelMapper->GetLabelTextProperty(0);
    int fsize = p->GetFontSize();
    fsize += sizeDelta;
    std::cout << "Font Size: " << fsize << std::endl;
    p->SetFontSize(fsize);
    testContext->statusTextLabelActor->SetTextProperty(p);
    testContext->labelMapper->SetLabelTextProperty(p, 0);
  }

  //-----------------------------------------------------------------------------
  void MultiplyData(double multiplier)
  {
    int res[2];
    testContext->plane->GetResolution(res[0], res[1]);
    res[1] = static_cast<int>(static_cast<double>(res[1]) * multiplier);
    std::cout << "Plane Multiplier: " << multiplier << " Size: " << res[1] << std::endl;
    testContext->plane->SetResolution(res[0], res[1]);
    UpdatePlaneArrays();
  }

  //-----------------------------------------------------------------------------
  void Scale(double multiplier)
  {
    testContext->matrix->GetScale(scale);
    scale[0] = multiplier;
    testContext->matrix->Scale(scale);
    testContext->matrix->GetScale(scale);
    std::cout << "Scale Multiplier: " << multiplier << " Size: " << scale[0] << std::endl;
  }

  //-----------------------------------------------------------------------------
  void Select()
  {
    vtkNew<vtkHardwareSelector> selector;
    selector->SetRenderer(this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
    int* temp = this->Interactor->GetRenderWindow()->GetSize();
    unsigned int windowSize[4];
    windowSize[0] = temp[2] + 1;
    windowSize[1] = temp[3] + 1;
    windowSize[2] = temp[0] - 1;
    windowSize[3] = temp[1] - 1;
    selector->SetArea(windowSize);
    selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
    vtkSelection* selection = selector->Select();
    std::cout << "Selection has " << selection->GetNumberOfNodes() << " nodes." << std::endl;
#if 0
        for (unsigned int cnt = 0; cnt < selection->GetNumberOfNodes(); cnt++)
        {
          auto n = selection->GetNode(cnt);
          vtkIdTypeArray *da = vtkIdTypeArray::SafeDownCast(n->GetSelectionData()->GetArray("SelectedIds"));
          cout << da->GetClassName() << endl;
          for (int tup = 0; tup < da->GetNumberOfTuples(); tup++)
          {
            cout << "  ID[" << tup << "]=" << da->GetValue(tup) << endl;
          }
        }
#endif
    testContext->ids->Update();
    this->selectionExtraction->SetInputData(0, testContext->ids->GetOutput());
    this->selectionExtraction->SetInputData(1, selection);

    selection->Delete();
    this->selectionMapper->ScalarVisibilityOff();
    this->selectionMapper->SetInputConnection(this->selectionExtraction->GetOutputPort());

    this->selectionActor->SetMapper(this->selectionMapper);
    this->selectionActor->GetProperty()->SetColor(1, 0, 0);
    this->selectionActor->GetProperty()->SetPointSize(40);
    static bool hasset = false;
    if (!hasset)
    {
      this->Renderer->AddActor(this->selectionActor);
      hasset = true;
    }
  }

  //-----------------------------------------------------------------------------
  void PerspectiveToggle()
  {
    vtkCamera* cam = this->Renderer->GetActiveCamera();
    cam->SetParallelProjection(!cam->GetParallelProjection());
  }

  //-----------------------------------------------------------------------------
  void FilterToggle()
  {
    static bool enlarge = true;
    if (enlarge)
    {
      std::cout << "ToggleFilter: Enlarge" << std::endl;
      testContext->filter->SetInputData(1, GetFilterSelection(0, 10));
    }
    else
    {
      std::cout << "ToggleFilter: Reduce" << std::endl;
      testContext->filter->SetInputData(1, GetFilterSelection(3, 7));
    }
    enlarge = !enlarge;
  }

  //-----------------------------------------------------------------------------
  double scale[3] = { 1.0, 1.0, 1.0 };
  vtkRenderer* Renderer;

  vtkNew<vtkExtractSelection> selectionExtraction;
  vtkNew<vtkDataSetMapper> selectionMapper;
  vtkNew<vtkActor> selectionActor;
};
vtkStandardNewMacro(KeyPressInteractorStyle);

} // end anon namespace

int TestLabeledDataMappers(int argc, char* argv[])
{
  //-----------------------------------------------------------------------------
  KeyPressInteractorStyle::PrintControls();

  ::ScopedTestContextInitializer scopedTestContextInit;
  (void)scopedTestContextInit;

  //-----------------------------------------------------------------------------
  // General Labels

  // Create some data to label
  testContext->plane->SetResolution(10, 10);
  UpdatePlaneArrays();

  // Scale data
  testContext->xform->SetInputConnection(testContext->plane->GetOutputPort());
  testContext->xform->SetTransform(testContext->matrix);

  // Generate ids for labeling
  testContext->ids->SetInputConnection(testContext->xform->GetOutputPort());
  testContext->ids->PointIdsOn();

  // Map labels
  AddTextProperties(testContext->labelMapper);
  testContext->labelMapper->SetLabelModeToLabelFieldData();
  testContext->labelMapper->SetFieldDataName(LABEL_TEXT_NAMES);
  testContext->labelMapper->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, LABEL_TYPES);
#if 0
  // Disable: this to use TextProperty colors
  // testContext->labelMapper->SetFrameColorsName(LABEL_FRAMES);
#endif
#define INSRC 0
#if INSRC == 0
  testContext->labelMapper->SetInputConnection(testContext->ids->GetOutputPort());
#endif
#if INSRC == 1
  vtkNew<vtkPolyData> pd;
  pd->ShallowCopy(testContext->ids->GetOutput());
  testContext->labelMapper->SetInputData(pd);
#endif
#if INSRC == 2
  vtkNew<vtkTrivialProducer> tp;
  tp->SetOutput(testContext->ids->GetOutput());
  testContext->labelMapper->SetInputConnection(tp->GetOutputPort());
#endif
  testContext->labelActor->SetMapper(testContext->labelMapper);

  //-----------------------------------------------------------------------------
  // Filtered PolyData Labels
  testContext->filter->SetInputData(0, GetFilteredPolyDataInput());
  testContext->filter->SetInputData(1, GetFilterSelection(3, 7));
  vtkNew<vtkGeometryFilter> geometryFilter;
  geometryFilter->SetInputConnection(testContext->filter->GetOutputPort());

  AddTextProperties(testContext->filteredLabelMapper);
  testContext->filteredLabelMapper->SetLabelModeToLabelFieldData();
  testContext->filteredLabelMapper->SetFieldDataName(LABEL_TEXT_NAMES);
  testContext->filteredLabelMapper->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, LABEL_TYPES);
  testContext->filteredLabelMapper->SetInputConnection(geometryFilter->GetOutputPort());
  testContext->filteredActor->SetMapper(testContext->filteredLabelMapper);

  //-----------------------------------------------------------------------------
  // Status Text Mapping
  testContext->statusTextLabelActor->SetInput("0 1 3 12 Z_61_a 102");
  vtkNew<vtkTextProperty> statusTextProperty;
  statusTextProperty->SetFontFamilyAsString("Arial");
  statusTextProperty->SetFontSize(24);
  statusTextProperty->SetColor(1.0, 1.0, 1.0);
  statusTextProperty->SetBackgroundColor(1.0, 0.0, 0.0);
  testContext->statusTextLabelActor->SetTextProperty(statusTextProperty);

  //-----------------------------------------------------------------------------
  // Origin Points
  testContext->originPointMapper->SetInputConnection(testContext->ids->GetOutputPort());
  testContext->originPointActor->SetMapper(testContext->originPointMapper);
  testContext->originPointActor->GetProperty()->SetRepresentationToPoints();
  testContext->originPointActor->GetProperty()->RenderPointsAsSpheresOn();
  testContext->originPointActor->GetProperty()->SetPointSize(5);

  //-----------------------------------------------------------------------------
  // Rendering setup
  vtkNew<vtkRenderer> ren;
  ren->AddActor(testContext->originPointActor);
  ren->AddActor(testContext->labelActor);
  ren->AddActor(testContext->filteredActor);
  ren->AddActor(testContext->statusTextLabelActor);

  ren->SetBackground(.5, .5, 6.);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetMultiSamples(0);
  renWin->SetSize(500, 500);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renWin);

  vtkNew<KeyPressInteractorStyle> style;
  style->Renderer = ren;
  renderWindowInteractor->SetInteractorStyle(style);
  style->SetCurrentRenderer(ren);

  renWin->Render();
  testContext->labelMapper->ReleaseGraphicsResources(renWin);
  testContext->filteredLabelMapper->ReleaseGraphicsResources(renWin);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  style->SetCurrentRenderer(nullptr);
  testContext->labelActor->SetMapper(nullptr);
  return !retVal;
}
