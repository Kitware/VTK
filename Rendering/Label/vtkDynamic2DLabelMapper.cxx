// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkDynamic2DLabelMapper.h"
#include "Private/vtkLabeledFormatter.h"

#include "vtkActor2D.h"
#include "vtkArrayDispatch.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkExecutive.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"
#include "vtkSortDataArray.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTimerLog.h"
#include "vtkViewport.h"

#include "vtksys/FStream.hxx"

#include <cmath>
#include <fstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDynamic2DLabelMapper);

//------------------------------------------------------------------------------
// Creates a new label mapper

vtkDynamic2DLabelMapper::vtkDynamic2DLabelMapper()
{
  this->LabelWidth = nullptr;
  this->LabelHeight = nullptr;
  this->Cutoff = nullptr;

  this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "priority");
  this->ReversePriority = false;
  this->LabelHeightPadding = 50;
  this->LabelWidthPadding = 10;
  this->ReferenceScale = 1.0;

  // Set new default property
  auto prop = vtkSmartPointer<vtkTextProperty>::New();
  prop->SetFontSize(12);
  prop->SetBold(1);
  prop->SetItalic(0);
  prop->SetShadow(1);
  prop->SetFontFamilyToArial();
  prop->SetJustificationToCentered();
  prop->SetVerticalJustificationToCentered();
  prop->SetColor(1, 1, 1);
  this->SetLabelTextProperty(prop);
}

//------------------------------------------------------------------------------
vtkDynamic2DLabelMapper::~vtkDynamic2DLabelMapper()
{
  delete[] this->LabelWidth;
  delete[] this->LabelHeight;
  delete[] this->Cutoff;
}

//------------------------------------------------------------------------------
void vtkDynamic2DLabelMapper::SetPriorityArrayName(const char* name)
{
  this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, name);
}

//------------------------------------------------------------------------------
struct vtkDynamic2DLabelMapper::vtkDynamic2DLabelMapperFormatter : vtkLabeledFormatterInterface
{
  vtkDynamic2DLabelMapperFormatter(
    vtkDynamic2DLabelMapper* self, vtkIntArray* typeArr, int numCurLabels)
    : vtkLabeledFormatterInterface(self, typeArr, numCurLabels)
  {
  }

  void SetFormattedString(int i, const char* resultString) override
  {
    auto* self = static_cast<vtkDynamic2DLabelMapper*>(this->Self);
    self->TextMappers[i]->SetInput(resultString);
    int type = 0;
    if (this->TypeArr)
    {
      type = this->TypeArr->GetValue(i);
    }
    vtkTextProperty* prop = self->GetLabelTextProperty(type);
    if (!prop)
    {
      prop = self->GetLabelTextProperty(0);
    }
    self->TextMappers[i]->SetTextProperty(prop);
  }
};

//------------------------------------------------------------------------------
void vtkDynamic2DLabelMapper::BuildLabelsInternal(vtkDataSet* input)
{
  auto formatterInput =
    this->ResolveLabeledFormatterInput(input->GetPointData(), input->GetNumberOfPoints(), input);
  if (!formatterInput.Valid)
  {
    return;
  }
  if (this->NumberOfLabelsAllocated < (this->NumberOfLabels + formatterInput.NumCurLabels))
  {
    vtkErrorMacro("Number of labels must be allocated before this method is called.");
    return;
  }

  vtkDynamic2DLabelMapperFormatter formatter(
    this, formatterInput.TypeArr, formatterInput.NumCurLabels);
  formatter.Dispatch(formatterInput);
  this->NumberOfLabels += formatterInput.NumCurLabels;
}

//------------------------------------------------------------------------------
void vtkDynamic2DLabelMapper::RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor)
{
  int i, j;
  double x[3];
  vtkDataObject* input = this->GetExecutive()->GetInputData(0, 0);

  if (!input)
  {
    vtkErrorMacro(<< "Need input data to render labels (2)");
    return;
  }

  vtkTextProperty* tprop = this->GetLabelTextProperty();
  if (!tprop)
  {
    vtkErrorMacro(<< "Need text property to render labels");
    return;
  }

  this->GetInputAlgorithm()->Update();

  // Input might have changed
  input = this->GetExecutive()->GetInputData(0, 0);

  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
  vtkGraph* gInput = vtkGraph::SafeDownCast(input);
  if (!dsInput && !gInput)
  {
    vtkErrorMacro(<< "Input must be vtkDataSet or vtkGraph.");
    return;
  }

  // If no labels we are done
  vtkIdType numItems = dsInput ? dsInput->GetNumberOfPoints() : gInput->GetNumberOfVertices();
  if (numItems == 0)
  {
    return;
  }

  // Check to see whether we have to rebuild everything
  if (this->GetMTime() > this->BuildTime || input->GetMTime() > this->BuildTime)
  {
    vtkDebugMacro(<< "Rebuilding labels");

    if (dsInput)
    {
      this->BuildLabels();
    }
    else
    {
      const int numGVertex = gInput->GetNumberOfVertices();
      auto formatterInput =
        this->ResolveLabeledFormatterInput(gInput->GetVertexData(), numGVertex, gInput);
      if (!formatterInput.Valid)
      {
        return;
      }
      this->AllocateLabels(numGVertex);
      this->NumberOfLabels = 0;
      vtkDynamic2DLabelMapperFormatter formatter(this, formatterInput.TypeArr, numGVertex);
      formatter.Dispatch(formatterInput);
      this->NumberOfLabels = numGVertex;
      this->BuildTime.Modified();
    }

    //
    // Perform the label layout preprocessing
    //

    // Calculate height and width padding
    float widthPadding = 0, heightPadding = 0;
    if (this->NumberOfLabels > 0)
    {
      widthPadding = this->TextMappers[0]->GetHeight(viewport) * this->LabelWidthPadding / 100.0;
      heightPadding = this->TextMappers[0]->GetHeight(viewport) * this->LabelHeightPadding / 100.0;
    }

    // Calculate label widths / heights
    delete[] this->LabelWidth;
    this->LabelWidth = new float[this->NumberOfLabels];
    for (i = 0; i < this->NumberOfLabels; i++)
    {
      this->LabelWidth[i] = this->TextMappers[i]->GetWidth(viewport) + widthPadding;
    }

    delete[] this->LabelHeight;
    this->LabelHeight = new float[this->NumberOfLabels];
    for (i = 0; i < this->NumberOfLabels; i++)
    {
      this->LabelHeight[i] = this->TextMappers[i]->GetHeight(viewport) + heightPadding;
    }

    // Determine cutoff scales of each point
    delete[] this->Cutoff;
    this->Cutoff = new float[this->NumberOfLabels];

    vtkTimerLog* timer = vtkTimerLog::New();
    timer->StartTimer();

    vtkCoordinate* coord = vtkCoordinate::New();
    coord->SetViewport(viewport);
    vtkPoints* pts = vtkPoints::New();
    for (i = 0; i < this->NumberOfLabels; i++)
    {
      double* dc;
      double pti[3];
      if (dsInput)
      {
        dsInput->GetPoint(i, pti);
      }
      else
      {
        gInput->GetPoint(i, pti);
      }
      coord->SetValue(pti);
      dc = coord->GetComputedDoubleDisplayValue(nullptr);
      pts->InsertNextPoint(dc[0], dc[1], 0);
    }
    coord->Delete();

    timer->StopTimer();
    vtkDebugMacro("vtkDynamic2DLabelMapper computed display coordinates for "
      << timer->GetElapsedTime() << "s");
    timer->StartTimer();

    // Announce progress
    double progress = 0;
    this->InvokeEvent(vtkCommand::ProgressEvent, static_cast<void*>(&progress));
    int current = 0;
    int total = this->NumberOfLabels * (this->NumberOfLabels - 1) / 2;

    // Create an index array to store the offsets of the sorted elements.
    vtkIdTypeArray* index = vtkIdTypeArray::New();
    index->SetNumberOfValues(this->NumberOfLabels);
    for (i = 0; i < this->NumberOfLabels; i++)
    {
      index->SetValue(i, i);
    }

    // If the array is found, sort it and rearrange the corresponding index array.
    vtkAbstractArray* inputArr = this->GetInputAbstractArrayToProcess(1, input);
    if (inputArr)
    {
      // Don't sort the original array, instead make a copy.
      vtkAbstractArray* arr = vtkAbstractArray::CreateArray(inputArr->GetDataType());
      arr->DeepCopy(inputArr);
      vtkSortDataArray::Sort(arr, index);
      arr->Delete();
    }

    // We normally go from highest (at the end) to lowest (at the beginning).
    // If priorities are reversed, we go from lowest to highest.
    // If no sorted array was used, we just go from index 0 to index n-1.
    vtkIdType begin = this->NumberOfLabels - 1;
    vtkIdType end = -1;
    vtkIdType step = -1;
    if ((this->ReversePriority && inputArr) || (!this->ReversePriority && !inputArr))
    {
      begin = 0;
      end = this->NumberOfLabels;
      step = 1;
    }
    auto ptsArray = vtkAOSDataArrayTemplate<float>::FastDownCast(pts->GetData());
    for (i = begin; i != end; i += step)
    {
      vtkIdType indexI = index->GetValue(i);
      float* pti = ptsArray->GetPointer(3 * indexI);
      this->Cutoff[indexI] = VTK_FLOAT_MAX;
      for (j = begin; j != i; j += step)
      {
        vtkIdType indexJ = index->GetValue(j);
        float* ptj = ptsArray->GetPointer(3 * indexJ);
        float absX = std::abs(pti[0] - ptj[0]);
        float absY = std::abs(pti[1] - ptj[1]);
        float xScale = 2 * absX / (this->LabelWidth[indexI] + this->LabelWidth[indexJ]);
        float yScale = 2 * absY / (this->LabelHeight[indexI] + this->LabelHeight[indexJ]);
        float maxScale = xScale < yScale ? yScale : xScale;
        if (maxScale < this->Cutoff[indexJ] && maxScale < this->Cutoff[indexI])
        {
          this->Cutoff[indexI] = maxScale;
        }
        if (current % 100000 == 0)
        {
          progress = static_cast<double>(current) / total;
          this->InvokeEvent(vtkCommand::ProgressEvent, static_cast<void*>(&progress));
        }
        current++;
      }
    }
    index->Delete();
    progress = 1.0;
    this->InvokeEvent(vtkCommand::ProgressEvent, static_cast<void*>(&progress));

    pts->Delete();

    // Determine the reference scale
    this->ReferenceScale = this->GetCurrentScale(viewport);

    timer->StopTimer();
    vtkDebugMacro(
      "vtkDynamic2DLabelMapper computed label cutoffs for " << timer->GetElapsedTime() << "s");
    timer->Delete();
  }

  //
  // Draw labels visible in the current scale
  //

  // Determine the current scale
  double scale = 1.0;
  if (this->ReferenceScale != 0.0)
  {
    scale = this->GetCurrentScale(viewport) / this->ReferenceScale;
  }

  for (i = 0; i < this->NumberOfLabels; i++)
  {
    if (dsInput)
    {
      dsInput->GetPoint(i, x);
    }
    else
    {
      gInput->GetPoint(i, x);
    }
    if ((1.0 / scale) < this->Cutoff[i])
    {
      actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
      actor->GetPositionCoordinate()->SetValue(x);
      this->TextMappers[i]->RenderOpaqueGeometry(viewport, actor);
    }
  }
}

//------------------------------------------------------------------------------
double vtkDynamic2DLabelMapper::GetCurrentScale(vtkViewport* viewport)
{
  // The current scale is the size on the screen of 1 unit in the xy plane

  vtkRenderer* ren = vtkRenderer::SafeDownCast(viewport);
  if (!ren)
  {
    vtkErrorMacro("vtkDynamic2DLabelMapper only works in a vtkRenderer or subclass");
    return 1.0;
  }
  vtkCamera* camera = ren->GetActiveCamera();
  if (camera->GetParallelProjection())
  {
    // For parallel projection, the scale depends on the parallel scale
    double scale = (ren->GetSize()[1] / 2.0) / camera->GetParallelScale();
    return scale;
  }
  else
  {
    // For perspective projection, the scale depends on the view angle
    double viewAngle = camera->GetViewAngle();
    double distZ =
      camera->GetPosition()[2] > 0 ? camera->GetPosition()[2] : -camera->GetPosition()[2];
    double unitAngle = vtkMath::DegreesFromRadians(atan2(1.0, distZ));
    double scale = ren->GetSize()[1] * unitAngle / viewAngle;
    return scale;
  }
}

//------------------------------------------------------------------------------
void vtkDynamic2DLabelMapper::RenderOverlay(vtkViewport* viewport, vtkActor2D* actor)
{
  int i;
  double x[3];
  vtkDataObject* input = this->GetExecutive()->GetInputData(0, 0);
  vtkGraph* gInput = vtkGraph::SafeDownCast(input);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
  vtkIdType numPts = dsInput ? dsInput->GetNumberOfPoints() : gInput->GetNumberOfVertices();

  // Determine the current scale
  double scale = 1.0;
  if (this->ReferenceScale != 0.0)
  {
    scale = this->GetCurrentScale(viewport) / this->ReferenceScale;
  }

  if (!input)
  {
    vtkErrorMacro(<< "Need input data to render labels (1)");
    return;
  }

  vtkTimerLog* timer = vtkTimerLog::New();
  timer->StartTimer();

  for (i = 0; i < this->NumberOfLabels && i < numPts; i++)
  {
    if (dsInput)
    {
      dsInput->GetPoint(i, x);
    }
    else
    {
      gInput->GetPoint(i, x);
    }
    actor->SetPosition(x);
    double* display = actor->GetPositionCoordinate()->GetComputedDoubleDisplayValue(viewport);
    double screenX = display[0];
    double screenY = display[1];

    bool inside = viewport->IsInViewport(static_cast<int>(screenX + this->LabelWidth[i]),
                    static_cast<int>(screenY + this->LabelHeight[i])) ||
      viewport->IsInViewport(static_cast<int>(screenX + this->LabelWidth[i]),
        static_cast<int>(screenY - this->LabelHeight[i])) ||
      viewport->IsInViewport(static_cast<int>(screenX - this->LabelWidth[i]),
        static_cast<int>(screenY + this->LabelHeight[i])) ||
      viewport->IsInViewport(static_cast<int>(screenX - this->LabelWidth[i]),
        static_cast<int>(screenY - this->LabelHeight[i]));
    if (inside && (1.0f / scale) < this->Cutoff[i])
    {
      this->TextMappers[i]->RenderOverlay(viewport, actor);
    }
  }

  timer->StopTimer();
  vtkDebugMacro("vtkDynamic2DLabelMapper interactive time: " << timer->GetElapsedTime() << "s");
  timer->Delete();
}

//------------------------------------------------------------------------------
void vtkDynamic2DLabelMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ReversePriority: " << (this->ReversePriority ? "on" : "off") << endl;
  os << indent << "LabelHeightPadding: " << (this->LabelHeightPadding ? "on" : "off") << endl;
  os << indent << "LabelWidthPadding: " << (this->LabelWidthPadding ? "on" : "off") << endl;
}
VTK_ABI_NAMESPACE_END
