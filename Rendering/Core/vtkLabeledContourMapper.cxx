/*==============================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabeledContourMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

==============================================================================*/

#include "vtkLabeledContourMapper.h"

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCoordinate.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkTextPropertyCollection.h"
#include "vtkTextRenderer.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkWindow.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

//------------------------------------------------------------------------------
struct LabelMetric
{
  double value;
  std::string Text;
  vtkTuple<int, 4> BoundingBox;
  vtkTuple<int, 2> Dimensions;
};

//------------------------------------------------------------------------------
struct LabelInfo
{
  // World coordinate:
  vtkVector3d Position;

  // Orientation (normalized, world space)
  vtkVector3d Right; // Left --> Right
  vtkVector3d Up; // Bottom --> Top

  // Corner locations (world space):
  vtkVector3d TLw;
  vtkVector3d TRw;
  vtkVector3d BRw;
  vtkVector3d BLw;

  // Corner location (display space):
  vtkVector2i TLd;
  vtkVector2i TRd;
  vtkVector2i BRd;
  vtkVector2i BLd;

  // Factor to scale the text actor by:
  double Scale;
};

namespace {

//------------------------------------------------------------------------------
double calculateSmoothness(double pathLength, double distance)
{
  return (pathLength - distance) / distance;
}

} // end anon namespace

//------------------------------------------------------------------------------
struct vtkLabeledContourMapper::Private
{
  // One entry per isoline.
  std::vector<LabelMetric> LabelMetrics;

  // One LabelInfo per label groups by isoline.
  std::vector<std::vector<LabelInfo> > LabelInfos;

  // Info for calculating display coordinates:
  vtkTuple<double, 16> MVP; // model-view-projection matrix
  vtkTuple<double, 4> ViewPort; // viewport
  vtkTuple<double, 4> NormalizedViewPort; // see vtkViewport::ViewToNormalizedVP
  vtkTuple<int, 2> WindowSize;
  vtkTuple<int, 2> ViewPortSize;
  vtkTuple<double, 2> DisplayOffset;
  vtkTuple<double, 4> ViewportBounds;

  // Needed to orient the labels
  vtkVector3d CameraRight;
  vtkVector3d CameraUp;
  vtkVector3d CameraForward;

  // Render times:
  double PrepareTime;
  double RenderTime;

  // Only want to print the stencil warning once:
  bool AlreadyWarnedAboutStencils;

  // Project coordinates:
  void WorldToDisplay(const vtkVector3d &world, vtkVector2i &display);
  void WorldToDisplay(const vtkVector3d &world, vtkVector2d &display);

  // Camera axes:
  bool SetViewInfo(vtkRenderer *ren);

  // Visibility test (display space):
  template <typename ScalarType>
  bool PixelIsVisible(const vtkVector2<ScalarType> &dispCoord) const;

  bool LineCanBeLabeled(vtkPoints *points, vtkIdType numIds,
                        const vtkIdType *ids, const LabelMetric &metrics);

  // Determine the first smooth position on the line defined by ids that is
  // 1.5x the length of the label (in display coordinates).
  bool NextLabel(vtkPoints *points, vtkIdType &numIds, vtkIdType *&ids,
                 const LabelMetric &metrics, LabelInfo &info,
                 double targetSmoothness);

  // Configure the text actor:
  bool BuildLabel(vtkTextActor3D *actor, const LabelMetric &metric,
                  const LabelInfo &info);

  // Compute the scaling factor and corner info for the label
  void ComputeLabelInfo(LabelInfo &info, const LabelMetric &metrics);

  // Test if the display quads overlap:
  bool TestOverlap(const LabelInfo &a, const LabelInfo &b);
};

//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkLabeledContourMapper)

//------------------------------------------------------------------------------
vtkLabeledContourMapper::vtkLabeledContourMapper()
{
  this->LabelVisibility = true;
  this->TextActors = NULL;
  this->NumberOfTextActors = 0;
  this->NumberOfUsedTextActors = 0;

  this->StencilQuads = NULL;
  this->StencilQuadsSize = 0;
  this->StencilQuadIndices = NULL;
  this->StencilQuadIndicesSize = 0;

  this->TextProperties = vtkSmartPointer<vtkTextPropertyCollection>::New();
  this->Internal = new vtkLabeledContourMapper::Private();
  this->Internal->PrepareTime = 0.0;
  this->Internal->RenderTime = 0.0;
  this->Internal->AlreadyWarnedAboutStencils = false;

  this->Reset();
}

//------------------------------------------------------------------------------
vtkLabeledContourMapper::~vtkLabeledContourMapper()
{
  this->FreeStencilQuads();
  this->FreeTextActors();

  delete this->Internal;
}

//------------------------------------------------------------------------------
void vtkLabeledContourMapper::Render(vtkRenderer *ren, vtkActor *act)
{
  if (!this->CheckInputs(ren))
    {
    return;
    }

  if (!this->LabelVisibility)
    {
    this->RenderPolyData(ren, act);
    return;
    }

  if (this->CheckRebuild(ren, act))
    {
    double startPrep = vtkTimerLog::GetUniversalTime();

    this->Reset();

    if (!this->PrepareRender(ren, act))
      {
      return;
      }

    if (!this->PlaceLabels())
      {
      return;
      }

    if (!this->ResolveLabels())
      {
      return;
      }

    if (!this->CreateLabels())
      {
      return;
      }

    if (!this->BuildStencilQuads())
      {
      return;
      }

    this->Internal->PrepareTime = vtkTimerLog::GetUniversalTime() - startPrep;
    this->BuildTime.Modified();
    }

  double startRender = vtkTimerLog::GetUniversalTime();

  if (!this->ApplyStencil(ren, act))
    {
    return;
    }

  if (!this->RenderPolyData(ren, act))
    {
    this->RemoveStencil();
    return;
    }

  if (!this->RemoveStencil())
    {
    return;
    }

  if (!this->RenderLabels(ren, act))
    {
    return;
    }

  this->Internal->RenderTime = vtkTimerLog::GetUniversalTime() - startRender;
}

//------------------------------------------------------------------------------
void vtkLabeledContourMapper::SetInputData(vtkPolyData *input)
{
  this->SetInputDataInternal(0, input);
}

//------------------------------------------------------------------------------
vtkPolyData *vtkLabeledContourMapper::GetInput()
{
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//------------------------------------------------------------------------------
double *vtkLabeledContourMapper::GetBounds()
{
  if (this->GetNumberOfInputConnections(0) == 0)
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
    }
  else
    {
    this->ComputeBounds();
    return this->Bounds;
    }
}

//------------------------------------------------------------------------------
void vtkLabeledContourMapper::GetBounds(double bounds[])
{
  this->Superclass::GetBounds(bounds);
}

//------------------------------------------------------------------------------
void vtkLabeledContourMapper::SetTextProperty(vtkTextProperty *tprop)
{
  if (this->TextProperties->GetNumberOfItems() != 1 ||
      this->TextProperties->GetItemAsObject(0) != tprop)
    {
    this->TextProperties->RemoveAllItems();
    this->TextProperties->AddItem(tprop);
    this->Modified();
    }
}

//------------------------------------------------------------------------------
void vtkLabeledContourMapper::SetTextProperties(vtkTextPropertyCollection *coll)
{
  if (coll != this->TextProperties)
    {
    this->TextProperties = coll;
    this->Modified();
    }
}

//------------------------------------------------------------------------------
vtkTextPropertyCollection *vtkLabeledContourMapper::GetTextProperties()
{
  return this->TextProperties;
}

//------------------------------------------------------------------------------
void vtkLabeledContourMapper::ComputeBounds()
{
  this->GetInput()->GetBounds(this->Bounds);
}

//------------------------------------------------------------------------------
void vtkLabeledContourMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LabelVisiblity: " << (this->LabelVisibility ? "On\n"
                                                               : "Off\n")
     << indent << "NumberOfTextActors: " << this->NumberOfTextActors << "\n"
     << indent << "NumberOfUsedTextActors: "
     << this->NumberOfUsedTextActors << "\n"
     << indent << "StencilQuadsSize: " << this->StencilQuadsSize << "\n"
     << indent << "StencilQuadIndicesSize: "
     << this->StencilQuadIndicesSize << "\n"
     << indent << "BuildTime: " << this->BuildTime.GetMTime() << "\n"
     << indent << "PolyDataMapper:\n";
  this->PolyDataMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TextProperties:\n";
  this->TextProperties->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
int vtkLabeledContourMapper::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

//------------------------------------------------------------------------------
void vtkLabeledContourMapper::Reset()
{
  this->Internal->LabelMetrics.clear();
  this->Internal->LabelInfos.clear();

  this->TextProperties->InitTraversal();
  while (vtkTextProperty *tprop = this->TextProperties->GetNextItem())
    {
    tprop->SetJustificationToCentered();
    tprop->SetVerticalJustificationToCentered();
    }
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::CheckInputs(vtkRenderer *ren)
{
  vtkPolyData *input = this->GetInput();
  if (!input)
    {
    vtkErrorMacro(<<"No input data!")
    return false;
    }

  if (!input->GetPoints())
    {
    vtkErrorMacro(<<"No points in dataset!");
    return false;
    }

  if (!input->GetPointData())
    {
    vtkErrorMacro(<<"No point data in dataset!");
    return false;
    }

  vtkCellArray *lines = input->GetLines();
  if (!lines)
    {
    vtkErrorMacro(<<"No lines in dataset!");
    return false;
    }

  vtkDataArray *scalars = input->GetPointData()->GetScalars();
  if (!scalars)
    {
    vtkErrorMacro(<<"No scalars in dataset!");
    return false;
    }

  vtkTextRenderer *tren = vtkTextRenderer::GetInstance();
  if (!tren)
    {
    vtkErrorMacro(<<"Text renderer unavailable.");
    return false;
    }

  if (this->TextProperties->GetNumberOfItems() == 0)
    {
    vtkErrorMacro(<<"No text properties set!");
    return false;
    }

  // Print a warning if stenciling is not enabled:
  vtkRenderWindow *win = ren->GetRenderWindow();
  if (!this->Internal->AlreadyWarnedAboutStencils && win)
    {
    if (win->GetStencilCapable() == 0)
      {
      vtkWarningMacro(<< "Stenciling is not enabled in the render window. "
                         "Isoline labels will have artifacts. To fix this, "
                         "call vtkRenderWindow::StencilCapableOn().")
      this->Internal->AlreadyWarnedAboutStencils = true;
      }
    }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::CheckRebuild(vtkRenderer *, vtkActor *act)
{
  // Get the highest mtime for the text properties:
  unsigned long int tPropMTime = this->TextProperties->GetMTime();
  this->TextProperties->InitTraversal();
  while (vtkTextProperty *tprop = this->TextProperties->GetNextItem())
    {
    tPropMTime = std::max(tPropMTime, tprop->GetMTime());
    }

  // Are we out of date?
  if (this->BuildTime.GetMTime() < this->GetInput()->GetMTime() ||
      this->BuildTime.GetMTime() < tPropMTime)
    {
    return true;
    }

  // Is there enough time allocated? (i.e. is this not an interactive render?)
  if (act->GetAllocatedRenderTime() >=
      (this->Internal->RenderTime + this->Internal->PrepareTime))
    {
    return true;
    }

  return false;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::PrepareRender(vtkRenderer *ren, vtkActor *)
{
  if (!this->Internal->SetViewInfo(ren))
    {
    return false;
    }

  // Already checked that these exist in CheckInputs()
  vtkPolyData *input = this->GetInput();
  vtkCellArray *lines = input->GetLines();
  vtkDataArray *scalars = input->GetPointData()->GetScalars();
  vtkTextRenderer *tren = vtkTextRenderer::GetInstance();

  vtkIdType numPts;
  vtkIdType *ids;
  vtkIdType cellId = 0;
  for (lines->InitTraversal(); lines->GetNextCell(numPts, ids); ++cellId)
    {
    vtkTextProperty *tprop = this->GetTextPropertyForCellId(cellId);
    LabelMetric metric;
    metric.value = numPts > 0 ? scalars->GetComponent(ids[0], 0) : 0.;
    std::ostringstream str;
    str << metric.value;
    metric.Text = str.str();
    if (!tren->GetBoundingBox(tprop, metric.Text, metric.BoundingBox.GetData()))
      {
      vtkErrorMacro(<<"Error calculating bounding box for string '"
                    << metric.Text << "'.");
      return false;
      }
    metric.Dimensions[0] = metric.BoundingBox[1] - metric.BoundingBox[0] + 1;
    metric.Dimensions[1] = metric.BoundingBox[3] - metric.BoundingBox[2] + 1;
    this->Internal->LabelMetrics.push_back(metric);
    }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::PlaceLabels()
{
  vtkPolyData *input = this->GetInput();
  vtkPoints *points = input->GetPoints();
  vtkCellArray *lines = input->GetLines();

  // Progression of smoothness tolerances to use.
  std::vector<double> tols;
  tols.push_back(0.010);
  tols.push_back(0.025);
  tols.push_back(0.050);
  tols.push_back(0.100);
  tols.push_back(0.200);
  tols.push_back(0.300);

  typedef std::vector<LabelMetric>::const_iterator MetricsIterator;
  MetricsIterator metric = this->Internal->LabelMetrics.begin();

  // Identify smooth parts of the isoline for labeling
  vtkIdType numIds;
  vtkIdType *origIds;
  this->Internal->LabelInfos.reserve(this->Internal->LabelMetrics.size());
  for (lines->InitTraversal(); lines->GetNextCell(numIds, origIds); ++metric)
    {
    assert(metric != this->Internal->LabelMetrics.end());

    this->Internal->LabelInfos.push_back(std::vector<LabelInfo>());

    // Test if it is possible to place a label (e.g. the line is big enough
    // to not be completely obscured)
    if (this->Internal->LineCanBeLabeled(points, numIds, origIds, *metric))
      {
      std::vector<LabelInfo> &infos = this->Internal->LabelInfos.back();
      LabelInfo info;
      // If no labels are found, increase the tolerance:
      for (std::vector<double>::const_iterator it = tols.begin(),
           itEnd = tols.end(); it != itEnd && infos.empty(); ++it)
        {
        vtkIdType nIds = numIds;
        vtkIdType *ids = origIds;
        while (this->Internal->NextLabel(points, nIds, ids, *metric, info, *it))
          {
          infos.push_back(info);
          }
        }
      }
    }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::ResolveLabels()
{
  typedef std::vector<LabelInfo>::iterator InnerIterator;
  typedef std::vector<std::vector<LabelInfo> >::iterator OuterIterator;

  bool removedA = false;
  bool removedB = false;

  OuterIterator outerA = this->Internal->LabelInfos.begin();
  OuterIterator outerEnd = this->Internal->LabelInfos.end();
  while (outerA != outerEnd)
    {
    InnerIterator innerA = outerA->begin();
    InnerIterator innerAEnd = outerA->end();
    while (innerA != innerAEnd)
      {
      removedA = false;
      OuterIterator outerB = outerA;
      while (!removedA && outerB != outerEnd)
        {
        InnerIterator innerB = outerA == outerB ? innerA + 1 : outerB->begin();
        InnerIterator innerBEnd = outerB->end();
        while (!removedA && innerB != innerBEnd)
          {
          removedB = false;
          // Does innerA overlap with innerB?
          if (this->Internal->TestOverlap(*innerA, *innerB))
            {
            // Remove the label that has the most labels for its isoline:
            if (outerA->size() > outerB->size())
              {
              // Remove innerA
              innerA = outerA->erase(innerA);
              innerAEnd = outerA->end();
              removedA = true;
              }
            else
              {
              // Remove innerB
              // Need to update A's iterators if outerA == outerB
              if (outerA == outerB)
                {
                // We know that aIdx < bIdx, so removing B won't change
                // the position of A:
                size_t aIdx = innerA - outerA->begin();
                innerB = outerB->erase(innerB);
                innerBEnd = outerB->end();
                innerA = outerA->begin() + aIdx;
                innerAEnd = outerA->end();
                }
              else
                {
                innerB = outerB->erase(innerB);
                innerBEnd = outerB->end();
                }
              removedB = true;
              }
            }
          // Erase will increment B if we removed it.
          if (!removedB)
            {
            ++innerB;
            }
          }
        ++outerB;
        }
      // Erase will increment A if we removed it.
      if (!removedA)
        {
        ++innerA;
        }
      }
    ++outerA;
    }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::CreateLabels()
{
  typedef std::vector<LabelMetric> MetricVector;
  typedef std::vector<LabelInfo> InfoVector;

  std::vector<InfoVector>::const_iterator outerLabels =
      this->Internal->LabelInfos.begin();
  std::vector<InfoVector>::const_iterator outerLabelsEnd =
      this->Internal->LabelInfos.end();

  // count the number of labels:
  vtkIdType numLabels = 0;
  while (outerLabels != outerLabelsEnd)
    {
    numLabels += (outerLabels++)->size();
    }

  if (!this->AllocateTextActors(numLabels))
    {
    vtkErrorMacro(<< "Error while allocating text actors.");
    return false;
    }

  outerLabels = this->Internal->LabelInfos.begin();
  MetricVector::const_iterator metrics = this->Internal->LabelMetrics.begin();
  MetricVector::const_iterator metricsEnd = this->Internal->LabelMetrics.end();
  vtkTextActor3D **actor = this->TextActors;
  vtkTextActor3D **actorEnd = this->TextActors + this->NumberOfUsedTextActors;

  vtkIdType cellId = 0;
  while (metrics != metricsEnd &&
         outerLabels != outerLabelsEnd &&
         actor != actorEnd)
    {
    vtkTextProperty *tprop = this->GetTextPropertyForCellId(cellId);

    for (InfoVector::const_iterator label = outerLabels->begin(),
         labelEnd = outerLabels->end(); label != labelEnd; ++label)
      {
      (*actor)->SetTextProperty(tprop);
      this->Internal->BuildLabel(*actor, *metrics, *label);
      ++actor;
      }

    ++cellId;
    ++metrics;
    ++outerLabels;
    }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::ApplyStencil(vtkRenderer *, vtkActor *)
{
  // Handled in backend override.
  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::RenderPolyData(vtkRenderer *ren, vtkActor *act)
{
  this->PolyDataMapper->SetInputConnection(this->GetInputConnection(0, 0));
  this->PolyDataMapper->Render(ren, act);
  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::RemoveStencil()
{
  // Handled in backend override.
  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::RenderLabels(vtkRenderer *ren, vtkActor *)
{
  for (vtkIdType i = 0; i < this->NumberOfUsedTextActors; ++i)
    {
    this->TextActors[i]->RenderTranslucentPolygonalGeometry(ren);
    }
  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::AllocateTextActors(vtkIdType num)
{
  if (num != this->NumberOfUsedTextActors)
    {
    if (this->NumberOfTextActors < num ||
        this->NumberOfTextActors > 2 * num)
      {
      this->FreeTextActors();

      // Leave some room to grow:
      this->NumberOfTextActors = num * 1.2;

      this->TextActors = new vtkTextActor3D*[this->NumberOfTextActors];
      for (vtkIdType i = 0; i < this->NumberOfTextActors; ++i)
        {
        this->TextActors[i] = vtkTextActor3D::New();
        }
      }

    this->NumberOfUsedTextActors = num;
    }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::FreeTextActors()
{
  for (vtkIdType i = 0; i < this->NumberOfTextActors; ++i)
    {
    this->TextActors[i]->Delete();
    }

  delete [] this->TextActors;
  this->TextActors = NULL;
  this->NumberOfTextActors = 0;
  this->NumberOfUsedTextActors = 0;
  return true;
}

//------------------------------------------------------------------------------
void vtkLabeledContourMapper::FreeStencilQuads()
{
  if (this->StencilQuads)
    {
    delete [] this->StencilQuads;
    this->StencilQuads = NULL;
    this->StencilQuadsSize = 0;

    delete [] this->StencilQuadIndices;
    this->StencilQuadIndices = NULL;
    this->StencilQuadIndicesSize = 0;
    }
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::BuildStencilQuads()
{
  vtkIdType quadCount = this->NumberOfUsedTextActors * 12;
  vtkIdType idxCount = this->NumberOfUsedTextActors * 6;
  if (quadCount != this->StencilQuadsSize)
    {
    this->FreeStencilQuads();
    this->StencilQuads = new float[quadCount];
    this->StencilQuadsSize = quadCount;
    this->StencilQuadIndices = new unsigned int[idxCount];
    this->StencilQuadIndicesSize = idxCount;
    }

  unsigned int qIndex = 0; // quad array index
  unsigned int iIndex = 0; // index array index
  unsigned int eIndex = 0; // index array element

  typedef std::vector<LabelInfo>::const_iterator InnerIterator;
  typedef std::vector<std::vector<LabelInfo> >::const_iterator OuterIterator;

  for (OuterIterator out = this->Internal->LabelInfos.begin(),
       outEnd = this->Internal->LabelInfos.end(); out != outEnd; ++out)
    {
    for (InnerIterator in = out->begin(), inEnd = out->end(); in != inEnd; ++in)
      {
      this->StencilQuads[qIndex +  0] = static_cast<float>(in->TLw[0]);
      this->StencilQuads[qIndex +  1] = static_cast<float>(in->TLw[1]);
      this->StencilQuads[qIndex +  2] = static_cast<float>(in->TLw[2]);
      this->StencilQuads[qIndex +  3] = static_cast<float>(in->TRw[0]);
      this->StencilQuads[qIndex +  4] = static_cast<float>(in->TRw[1]);
      this->StencilQuads[qIndex +  5] = static_cast<float>(in->TRw[2]);
      this->StencilQuads[qIndex +  6] = static_cast<float>(in->BRw[0]);
      this->StencilQuads[qIndex +  7] = static_cast<float>(in->BRw[1]);
      this->StencilQuads[qIndex +  8] = static_cast<float>(in->BRw[2]);
      this->StencilQuads[qIndex +  9] = static_cast<float>(in->BLw[0]);
      this->StencilQuads[qIndex + 10] = static_cast<float>(in->BLw[1]);
      this->StencilQuads[qIndex + 11] = static_cast<float>(in->BLw[2]);

      this->StencilQuadIndices[iIndex + 0] = eIndex + 0;
      this->StencilQuadIndices[iIndex + 1] = eIndex + 1;
      this->StencilQuadIndices[iIndex + 2] = eIndex + 2;
      this->StencilQuadIndices[iIndex + 3] = eIndex + 0;
      this->StencilQuadIndices[iIndex + 4] = eIndex + 2;
      this->StencilQuadIndices[iIndex + 5] = eIndex + 3;

      qIndex += 12;
      iIndex += 6;
      eIndex += 4;
      }
    }

  return true;
}

//------------------------------------------------------------------------------
vtkTextProperty *
vtkLabeledContourMapper::GetTextPropertyForCellId(vtkIdType cellId) const
{
  int idx = static_cast<int>(cellId % this->TextProperties->GetNumberOfItems());
  return this->TextProperties->GetItem(idx);
}

//------------------------------------------------------------------------------
inline void vtkLabeledContourMapper::Private::WorldToDisplay(
    const vtkVector3d &world, vtkVector2i &display)
{
  vtkVector2d v;
  this->WorldToDisplay(world, v);
  display = vtkVector2i(v.Cast<int>().GetData());
}

//------------------------------------------------------------------------------
inline void vtkLabeledContourMapper::Private::WorldToDisplay(
    const vtkVector3d &world, vtkVector2d &v)
{
  // This is adapted from vtkCoordinate's world to display conversion. We
  // reimplement it here for efficiency.

  // vtkRenderer::WorldToView
  double w;
  v[0] = world[0] * MVP[0]  + world[1] * MVP[1]  + world[2] * MVP[2]  + MVP[3];
  v[1] = world[0] * MVP[4]  + world[1] * MVP[5]  + world[2] * MVP[6]  + MVP[7];
  w    = world[0] * MVP[12] + world[1] * MVP[13] + world[2] * MVP[14] + MVP[15];
  v = v * (1. / w);

  // vtkViewport::ViewToNormalizedViewport
  v[0] = this->NormalizedViewPort[0] + ((v[0] + 1.) / 2.) *
      (this->NormalizedViewPort[2] - this->NormalizedViewPort[0]);
  v[1] = this->NormalizedViewPort[1] + ((v[1] + 1.) / 2.) *
      (this->NormalizedViewPort[3] - this->NormalizedViewPort[1]);
  v[0] = (v[0] - this->ViewPort[0]) /
      (this->ViewPort[2] - this->ViewPort[0]);
  v[1] = (v[1] - this->ViewPort[1]) /
      (this->ViewPort[3] - this->ViewPort[1]);

  // vtkViewport::NormalizedViewportToViewport
  v[0] *= this->ViewPortSize[0] - 1.;
  v[1] *= this->ViewPortSize[1] - 1.;

  // vtkViewport::ViewportToNormalizedDisplay
  // vtkViewport::NormalizedDisplayToDisplay
  v[0] += this->DisplayOffset[0];
  v[1] += this->DisplayOffset[1];
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::Private::SetViewInfo(vtkRenderer *ren)
{
  vtkMatrix4x4 *mat = ren->GetActiveCamera()->GetModelViewTransformMatrix();
  this->CameraRight.Set(mat->GetElement(0, 0),
                        mat->GetElement(0, 1),
                        mat->GetElement(0, 2));
  this->CameraUp.Set(mat->GetElement(1, 0),
                     mat->GetElement(1, 1),
                     mat->GetElement(1, 2));
  this->CameraForward.Set(mat->GetElement(2, 0),
                          mat->GetElement(2, 1),
                          mat->GetElement(2, 2));

  mat = ren->GetActiveCamera()->GetCompositeProjectionTransformMatrix(
        ren->GetTiledAspectRatio(), 0., 1.);
  vtkMatrix4x4::DeepCopy(this->MVP.GetData(), mat);

  if (vtkWindow *win = ren->GetVTKWindow())
    {
    int *size = win->GetSize();
    this->WindowSize[0] = size[0];
    this->WindowSize[1] = size[1];

    size = ren->GetSize();
    this->ViewPortSize[0] = size[0];
    this->ViewPortSize[1] = size[1];

    ren->GetViewport(this->ViewPort.GetData());

    double *tvport = win->GetTileViewport();
    this->NormalizedViewPort[0] = std::max(this->ViewPort[0], tvport[0]);
    this->NormalizedViewPort[1] = std::max(this->ViewPort[1], tvport[1]);
    this->NormalizedViewPort[2] = std::min(this->ViewPort[2], tvport[2]);
    this->NormalizedViewPort[3] = std::min(this->ViewPort[3], tvport[3]);

    this->ViewportBounds[0] = this->ViewPort[0] * this->WindowSize[0];
    this->ViewportBounds[1] = this->ViewPort[2] * this->WindowSize[0];
    this->ViewportBounds[2] = this->ViewPort[1] * this->WindowSize[1];
    this->ViewportBounds[3] = this->ViewPort[3] * this->WindowSize[1];

    this->DisplayOffset[0] = static_cast<double>(this->ViewportBounds[0]) + 0.5;
    this->DisplayOffset[1] = static_cast<double>(this->ViewportBounds[2]) + 0.5;
    }
  else
    {
    vtkGenericWarningMacro(<<"No render window present.");
    return false;
    }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::Private::LineCanBeLabeled(
    vtkPoints *points, vtkIdType numIds, const vtkIdType *ids,
    const LabelMetric &metrics)
{
  vtkTuple<int, 4> bbox(0);
  vtkVector3d world;
  vtkVector2i display;
  if (numIds > 0)
    {
    do
      {
      points->GetPoint(*(ids++), world.GetData());
      this->WorldToDisplay(world, display);
      --numIds;
      }
    while (numIds > 0 && !this->PixelIsVisible(display));

    if (!this->PixelIsVisible(display))
      {
      // No visible points
      return false;
      }

    bbox[0] = display.GetX();
    bbox[1] = display.GetX();
    bbox[2] = display.GetY();
    bbox[3] = display.GetY();
    }
  while (numIds-- > 0)
    {
    points->GetPoint(*(ids++), world.GetData());
    this->WorldToDisplay(world, display);
    if (this->PixelIsVisible(display))
      {
      bbox[0] = std::min(bbox[0], display.GetX());
      bbox[1] = std::max(bbox[1], display.GetX());
      bbox[2] = std::min(bbox[2], display.GetY());
      bbox[3] = std::max(bbox[3], display.GetY());
      }
    }

  // Must be at least twice the label length in at least one direction:
  return (metrics.Dimensions[0] * 2 < bbox[1] - bbox[0] ||
          metrics.Dimensions[0] * 2 < bbox[3] - bbox[2]);
}

//------------------------------------------------------------------------------
template <typename ScalarType>
bool vtkLabeledContourMapper::Private::PixelIsVisible(
    const vtkVector2<ScalarType> &dispCoord) const
{
  return (dispCoord.GetX() >= this->ViewportBounds[0] &&
          dispCoord.GetX() <= this->ViewportBounds[1] &&
          dispCoord.GetY() >= this->ViewportBounds[2] &&
          dispCoord.GetY() <= this->ViewportBounds[3]);
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::Private::NextLabel(
    vtkPoints *points, vtkIdType &numIds, vtkIdType *&ids,
    const LabelMetric &metrics, LabelInfo &info, double targetSmoothness)
{
  if (numIds < 3)
    {
    return false;
    }

  // Start of current smooth run (index into ids).
  vtkIdType startIdx = 0;
  vtkVector3d startPoint;
  vtkVector2d startPointDisplay;
  points->GetPoint(ids[startIdx], startPoint.GetData());
  this->WorldToDisplay(startPoint, startPointDisplay);

  // Find the first visible point:
  while (startIdx + 1 < numIds && !this->PixelIsVisible(startPointDisplay))
    {
    ++startIdx;
    points->GetPoint(ids[startIdx], startPoint.GetData());
    this->WorldToDisplay(startPoint, startPointDisplay);
    }

  // Start point in current segment.
  vtkVector3d prevPoint = startPoint;
  vtkVector2d prevPointDisplay = startPointDisplay;

  // End point of current segment (index into ids).
  vtkIdType curIdx = startIdx + 1;
  vtkVector3d curPoint = prevPoint;
  vtkVector2d curPointDisplay = prevPointDisplay;

  // Accumulated length of segments since startId
  std::vector<double> segmentLengths;
  double rAccum = 0.;

  // Straight-line distance from start --> previous
  double rPrevStraight = 0.;

  // Straight-line distance from start --> current
  double rStraight = 0.;

  // Straight-line distance from prev --> current
  double rSegment = 0.;

  // Minimum length of a smooth segment in display space
  const double minLength = 1.2 * metrics.Dimensions[0];

  // Vector of segment prev --> current
  vtkVector2d segment(0, 0);

  // Vector of segment start --> current
  vtkVector2d prevStraight(0, 0);
  vtkVector2d straight(0, 0);

  // Smoothness of start --> current
  double smoothness = 0;

  while (curIdx < numIds)
    {
    // Copy cur --> prev
    prevPoint = curPoint;
    prevPointDisplay = curPointDisplay;
    prevStraight = straight;
    rPrevStraight = rStraight;

    // Update current:
    points->GetPoint(ids[curIdx], curPoint.GetData());
    this->WorldToDisplay(curPoint, curPointDisplay);

    // Calculate lengths and smoothness.
    segment = curPointDisplay - prevPointDisplay;
    straight = curPointDisplay - startPointDisplay;
    rSegment = segment.Norm();
    rStraight = straight.Norm();
    segmentLengths.push_back(rSegment);
    rAccum += rSegment;
    smoothness = calculateSmoothness(rAccum, rStraight);

    // Are we still dealing with a reasonably smooth line?
    // The first check tests if we've traveled far enough to get a fair estimate
    // of smoothness.
    if (rAccum < 10. || smoothness <= targetSmoothness)
      {
      // Advance to the next point:
      ++curIdx;
      continue;
      }
    else
      {
      // The line is no longer smooth "enough". Was start --> previous long
      // enough (twice label width)?
      if (rPrevStraight >= minLength)
        {
        // We have a winner!
        break;
        }
      else
        {
        // This startIdx won't work. On to the next visible startIdx.
        do
          {
          ++startIdx;
          points->GetPoint(ids[startIdx], startPoint.GetData());
          this->WorldToDisplay(startPoint, startPointDisplay);
          }
        while (startIdx < numIds && !this->PixelIsVisible(startPointDisplay));

        prevPoint = startPoint;
        prevPointDisplay = startPointDisplay;
        curPoint = startPoint;
        curPointDisplay = startPointDisplay;
        curIdx = startIdx + 1;
        rAccum = 0.;
        rPrevStraight = 0.;
        segmentLengths.clear();
        continue;
        }
      }
    }

  // Was the last segment ok?
  if (rPrevStraight >= minLength)
    {
    // The final index of the segment:
    vtkIdType endIdx = curIdx - 1;

    // The direction of the text:
    info.Right = (prevPoint - startPoint).Normalized();
    // Ensure the text reads left->right:
    if (info.Right.Dot(this->CameraRight) < 0.)
      {
      info.Right = -info.Right;
      }

    // The up vector. Cross the forward direction with the orientation and
    // ensure that the result vector is in the same hemisphere as CameraUp
    info.Up = info.Right.Compare(this->CameraForward, 10e-10)
        ? this->CameraUp
        : info.Right.Cross(this->CameraForward).Normalized();
    if (info.Up.Dot(this->CameraUp) < 0.)
      {
      info.Up = -info.Up;
      }

    // Walk through the segment lengths to find where the center is for label
    // placement:
    double targetLength = rPrevStraight * 0.5;
    rAccum = 0.;
    size_t endIdxOffset = 1;
    for (; endIdxOffset <= segmentLengths.size(); ++endIdxOffset)
      {
      rSegment = segmentLengths[endIdxOffset - 1];
      double tmp = rAccum + rSegment;
      if (tmp > targetLength)
        {
        break;
        }
      rAccum = tmp;
      }
    targetLength -= rAccum;
    points->GetPoint(ids[startIdx + endIdxOffset - 1], prevPoint.GetData());
    points->GetPoint(ids[startIdx + endIdxOffset], curPoint.GetData());
    this->WorldToDisplay(prevPoint, prevPointDisplay);
    this->WorldToDisplay(curPoint, curPointDisplay);
    vtkVector3d offset = curPoint - prevPoint;
    double rSegmentWorld = offset.Normalize();
    offset = offset * (targetLength * rSegmentWorld / rSegment);
    info.Position = prevPoint + offset;

    this->ComputeLabelInfo(info, metrics);

    // Update the cell array:
    ids += endIdx;
    numIds -= endIdx;

    return true;
    }

  return false;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourMapper::Private::BuildLabel(vtkTextActor3D *actor,
                                                  const LabelMetric &metric,
                                                  const LabelInfo &info)

{
  actor->SetInput(metric.Text.c_str());
  actor->SetPosition(const_cast<double*>(info.Position.GetData()));

  vtkNew<vtkTransform> xform;
  xform->PostMultiply();

  xform->Translate((-info.Position).GetData());

  xform->Scale(info.Scale, info.Scale, info.Scale);

  vtkVector3d forward = info.Up.Cross(info.Right);
  double rot[16];
  rot[4 * 0 + 0] = info.Right[0];
  rot[4 * 1 + 0] = info.Right[1];
  rot[4 * 2 + 0] = info.Right[2];
  rot[4 * 3 + 0] = 0;
  rot[4 * 0 + 1] = info.Up[0];
  rot[4 * 1 + 1] = info.Up[1];
  rot[4 * 2 + 1] = info.Up[2];
  rot[4 * 3 + 1] = 0;
  rot[4 * 0 + 2] = forward[0];
  rot[4 * 1 + 2] = forward[1];
  rot[4 * 2 + 2] = forward[2];
  rot[4 * 3 + 2] = 0;
  rot[4 * 0 + 3] = 0;
  rot[4 * 1 + 3] = 0;
  rot[4 * 2 + 3] = 0;
  rot[4 * 3 + 3] = 1;
  xform->Concatenate(rot);

  xform->Translate(info.Position.GetData());

  actor->SetUserTransform(xform.GetPointer());

  return true;
}

//------------------------------------------------------------------------------
void vtkLabeledContourMapper::Private::ComputeLabelInfo(
    LabelInfo &info, const LabelMetric &metrics)
{
  // Compute scaling factor. Use the Up vector for deltas as we know it is
  // perpendicular to the view axis:
  vtkVector3d delta = info.Up * (0.5 * metrics.Dimensions[0]);
  vtkVector3d leftWorld = info.Position - delta;
  vtkVector3d rightWorld = info.Position + delta;
  vtkVector2d leftDisplay;
  vtkVector2d rightDisplay;
  this->WorldToDisplay(leftWorld, leftDisplay);
  this->WorldToDisplay(rightWorld, rightDisplay);
  info.Scale = static_cast<double>(metrics.Dimensions[0]) /
      (rightDisplay - leftDisplay).Norm();

  // Compute the corners of the quad. World coordinates are used to create the
  // stencil, display coordinates are used to detect collisions.
  // Note that we make this a little bigger (4px) than a tight bbox to give a
  // little breathing room around the text.
  vtkVector3d halfWidth =
      ((0.5 * metrics.Dimensions[0] + 2) * info.Scale) * info.Right;
  vtkVector3d halfHeight =
      ((0.5 * metrics.Dimensions[1] + 2) * info.Scale) * info.Up;
  info.TLw = info.Position + halfHeight - halfWidth;
  info.TRw = info.Position + halfHeight + halfWidth;
  info.BRw = info.Position - halfHeight + halfWidth;
  info.BLw = info.Position - halfHeight - halfWidth;
  this->WorldToDisplay(info.TLw, info.TLd);
  this->WorldToDisplay(info.TRw, info.TRd);
  this->WorldToDisplay(info.BRw, info.BRd);
  this->WorldToDisplay(info.BLw, info.BLd);
}

// Anonymous namespace for some TestOverlap helpers:
namespace {

// Rotates the vector by -90 degrees.
void perp(vtkVector2i &vec)
{
  std::swap(vec[0], vec[1]);
  vec[1] = -vec[1];
}

// Project all points in other onto the line (point + t * direction).
// Return true if t is positive for all points in other (e.g. all points in
// 'other' are outside the polygon containing 'point').
bool allOutside(const vtkVector2i &point, const vtkVector2i &direction,
                const LabelInfo &other)
{
  vtkVector2i testVector;

  testVector = other.TLd - point;
  if (direction.Dot(testVector) <= 0)
    {
    return false;
    }

  testVector = other.TRd - point;
  if (direction.Dot(testVector) <= 0)
    {
    return false;
    }

  testVector = other.BRd - point;
  if (direction.Dot(testVector) <= 0)
    {
    return false;
    }

  testVector = other.BLd - point;
  if (direction.Dot(testVector) <= 0)
    {
    return false;
    }

  return true;
}

// Generate a vector pointing out from each edge of the rectangle. Do this
// by traversing the corners counter-clockwise and using the perp() function.
// Use allOutside() to determine whether the other polygon is outside the edge.
// Return true if the axis separates the polygons.
bool testAxis(const LabelInfo &poly, const vtkVector2i &edgeStart,
              const vtkVector2i &edgeEnd)
{
  // Vector pointing out of polygon:
  vtkVector2i direction = edgeEnd - edgeStart;
  perp(direction);

  return allOutside(edgeStart, direction, poly);
}

} // end anon namespace

//------------------------------------------------------------------------------
// Implements axis separation method for detecting polygon intersection.
// Ref: http://www.geometrictools.com/Documentation/MethodOfSeparatingAxes.pdf
// In essence, look for an axis that separates the two rectangles.
// Return true if overlap occurs.
bool vtkLabeledContourMapper::Private::TestOverlap(const LabelInfo &a,
                                                   const LabelInfo &b)
{
  // Note that the order of the points matters, must be CCW to get the correct
  // perpendicular vector:
  return !(testAxis(a, b.TLd, b.BLd) ||
           testAxis(a, b.BLd, b.BRd) ||
           testAxis(a, b.BRd, b.TRd) ||
           testAxis(a, b.TRd, b.TLd) ||
           testAxis(b, a.TLd, a.BLd) ||
           testAxis(b, a.BLd, a.BRd) ||
           testAxis(b, a.BRd, a.TRd) ||
           testAxis(b, a.TRd, a.TLd));
}
