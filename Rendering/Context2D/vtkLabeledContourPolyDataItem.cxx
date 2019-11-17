/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabeledContourPolyDataItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLabeledContourPolyDataItem.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include "vtkTextPropertyCollection.h"

#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"

#include "vtkBrush.h"
#include "vtkPen.h"

#include "vtkPointData.h"

#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkTextPropertyCollection.h"

#include "vtkTextRenderer.h"
#include "vtkTimerLog.h"

#include "vtkTransform.h"
#include "vtkTransform2D.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkWindow.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <vector>

//------------------------------------------------------------------------------
struct PDILabelMetric
{
  bool Valid;
  double Value;
  vtkTextProperty* TProp;
  std::string Text;
  // These measure the pixel size of the text texture:
  vtkTuple<int, 4> BoundingBox;
  vtkTuple<int, 2> Dimensions;
};

//------------------------------------------------------------------------------
struct PDILabelInfo
{
  // Position in actor space:
  vtkVector3d Position;

  // Which directions are label right and up in display space
  vtkVector2d RightD;
  vtkVector2d UpD;

  // Corner location (display space):
  vtkVector2i TLd;
  vtkVector2i TRd;
  vtkVector2i BRd;
  vtkVector2i BLd;
};

//------------------------------------------------------------------------------
struct PDILabelHelper
{
  double orientation;
};

namespace
{

// Circular iterator through a vtkTextPropertyCollection -----------------------
class TextPropLoop
{
  vtkTextPropertyCollection* TProps;

public:
  TextPropLoop(vtkTextPropertyCollection* col)
    : TProps(col)
  {
    TProps->InitTraversal();
  }

  vtkTextProperty* Next()
  {
    // The input checks should fail if this is the case:
    assert("No text properties set! Prerender check failed!" && TProps->GetNumberOfItems() != 0);

    vtkTextProperty* result = TProps->GetNextItem();
    if (!result)
    {
      TProps->InitTraversal();
      result = TProps->GetNextItem();
      assert("Text property traversal error." && result != nullptr);
    }
    return result;
  }
};

//------------------------------------------------------------------------------
double calculateSmoothness(double pathLength, double distance)
{
  return (pathLength - distance) / distance;
}

} // end anon namespace

//------------------------------------------------------------------------------
struct vtkLabeledContourPolyDataItem::Private
{
  vtkAbstractContextItem* item;

  // One entry per isoline.
  std::vector<PDILabelMetric> LabelMetrics;

  // One PDILabelInfo per label groups by isoline.
  std::vector<std::vector<PDILabelInfo> > LabelInfos;

  // Info for calculating display coordinates:
  vtkTuple<double, 16> AMVP;               // actor-model-view-projection matrix
  vtkTuple<double, 16> ActorMatrix;        // Actor model matrix
  vtkTuple<double, 16> InverseActorMatrix; // Inverse Actor model matrix
  vtkTuple<double, 4> ViewPort;            // viewport
  vtkTuple<double, 4> NormalizedViewPort;  // see vtkViewport::ViewToNormalizedVP
  vtkTuple<int, 2> WindowSize;
  vtkTuple<int, 2> ViewPortSize;
  vtkTuple<double, 2> DisplayOffset;
  vtkTuple<double, 4> ViewportBounds;

  // Needed to orient the labels
  vtkVector2d CameraRight;
  vtkVector2d CameraUp;

  vtkTuple<double, 9> forwardMatrix;
  vtkTuple<double, 9> inverseMatrix;

  // Render times:
  double PrepareTime;
  double RenderTime;

  vtkSmartPointer<vtkTextProperty> currentTextProp;

  // Project coordinates. Note that the vector objects must be unique.
  void ActorToWorld(const vtkVector3d& actor, vtkVector3d& world) const;
  void WorldToActor(const vtkVector3d& world, vtkVector3d& actor) const;
  void ActorToDisplay(const vtkVector3d& actor, vtkVector2i& display) const;
  void ActorToDisplay(const vtkVector3d& actor, vtkVector2d& display) const;

  // Camera axes:
  bool SetViewInfo(vtkContextScene* scene, vtkContextTransform* transform);

  // Visibility test (display space):
  template <typename ScalarType>
  bool PixelIsVisible(const vtkVector2<ScalarType>& dispCoord) const;

  bool LineCanBeLabeled(
    vtkPoints* points, vtkIdType numIds, const vtkIdType* ids, const PDILabelMetric& metrics);

  // Determine the first smooth position on the line defined by ids that is
  // 1.2x the length of the label (in display coordinates).
  // The position will be no less than skipDistance along the line from the
  // starting location. This can be used to ensure that labels are placed a
  // minimum distance apart.
  bool NextLabel(vtkPoints* points, vtkIdType& numIds, const vtkIdType*& ids,
    const PDILabelMetric& metrics, PDILabelInfo& info, double targetSmoothness,
    double skipDistance);

  // Configure the text actor:
  bool BuildLabel(vtkTextActor3D* actor, PDILabelHelper* helper, const PDILabelMetric& metric,
    const PDILabelInfo& info);

  // Compute the scaling factor and corner info for the label
  void ComputeLabelInfo(PDILabelInfo& info, const PDILabelMetric& metrics);

  // Test if the display quads overlap:
  bool TestOverlap(const PDILabelInfo& a, const PDILabelInfo& b);
};

vtkStandardNewMacro(vtkLabeledContourPolyDataItem);

//-----------------------------------------------------------------------------
vtkLabeledContourPolyDataItem::vtkLabeledContourPolyDataItem()
{
  this->SkipDistance = 0.;
  this->LabelVisibility = true;
  this->TextActors = nullptr;
  this->LabelHelpers = nullptr;
  this->NumberOfTextActors = 0;
  this->NumberOfUsedTextActors = 0;

  this->TextProperties = vtkSmartPointer<vtkTextPropertyCollection>::New();
  vtkNew<vtkTextProperty> defaultTProp;
  this->TextProperties->AddItem(defaultTProp);

  this->Internal = new vtkLabeledContourPolyDataItem::Private();
  this->Internal->currentTextProp = vtkSmartPointer<vtkTextProperty>::New();
  this->Internal->PrepareTime = 0.0;
  this->Internal->RenderTime = 0.0;

  this->Internal->item = this;

  this->Reset();
}

//-----------------------------------------------------------------------------
vtkLabeledContourPolyDataItem::~vtkLabeledContourPolyDataItem()
{
  this->FreeTextActors();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::Paint(vtkContext2D* painter)
{
  if (!this->CheckInputs())
  {
    return false;
  }

  if (!this->LabelVisibility)
  {
    return this->Superclass::Paint(painter);
  }

  if (this->CheckRebuild())
  {
    double startPrep = vtkTimerLog::GetUniversalTime();

    this->Reset();

    if (!this->PrepareRender())
    {
      return false;
    }

    if (!this->PlaceLabels())
    {
      return false;
    }

    if (!this->ResolveLabels())
    {
      return false;
    }

    if (!this->CreateLabels())
    {
      return false;
    }

    this->Internal->PrepareTime = vtkTimerLog::GetUniversalTime() - startPrep;
    this->LabelBuildTime.Modified();
  }

  double startRender = vtkTimerLog::GetUniversalTime();

  this->Superclass::Paint(painter);

  if (!this->RenderLabels(painter))
  {
    return false;
  }

  this->Internal->RenderTime = vtkTimerLog::GetUniversalTime() - startRender;

  return true;
}

//------------------------------------------------------------------------------
void vtkLabeledContourPolyDataItem::SetTextProperty(vtkTextProperty* tprop)
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
void vtkLabeledContourPolyDataItem::SetTextProperties(vtkTextPropertyCollection* coll)
{
  if (coll != this->TextProperties)
  {
    this->TextProperties = coll;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkTextPropertyCollection* vtkLabeledContourPolyDataItem::GetTextProperties()
{
  return this->TextProperties;
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkLabeledContourPolyDataItem::GetTextPropertyMapping()
{
  return this->TextPropertyMapping;
}

//------------------------------------------------------------------------------
void vtkLabeledContourPolyDataItem::SetTextPropertyMapping(vtkDoubleArray* mapping)
{
  if (this->TextPropertyMapping != mapping)
  {
    this->TextPropertyMapping = mapping;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkLabeledContourPolyDataItem::ComputeBounds()
{
  // this->PolyData->GetBounds(this->Bounds);
}

//------------------------------------------------------------------------------
void vtkLabeledContourPolyDataItem::Reset()
{
  this->Internal->LabelMetrics.clear();
  this->Internal->LabelInfos.clear();

  this->TextProperties->InitTraversal();
  while (vtkTextProperty* tprop = this->TextProperties->GetNextItem())
  {
    tprop->SetJustificationToCentered();
    tprop->SetVerticalJustificationToCentered();
  }
}

//------------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::CheckInputs()
{
  vtkPolyData* input = this->PolyData;
  if (!input)
  {
    vtkErrorMacro(<< "No input data!");
    return false;
  }

  if (!input->GetPoints())
  {
    vtkErrorMacro(<< "No points in dataset!");
    return false;
  }

  if (!input->GetPointData())
  {
    vtkErrorMacro(<< "No point data in dataset!");
    return false;
  }

  vtkCellArray* lines = input->GetLines();
  if (!lines)
  {
    vtkErrorMacro(<< "No lines in dataset!");
    return false;
  }

  vtkDataArray* scalars = input->GetPointData()->GetScalars();
  if (!scalars)
  {
    vtkErrorMacro(<< "No scalars in dataset!");
    return false;
  }

  vtkTextRenderer* tren = vtkTextRenderer::GetInstance();
  if (!tren)
  {
    vtkErrorMacro(<< "Text renderer unavailable.");
    return false;
  }

  if (this->TextProperties->GetNumberOfItems() == 0)
  {
    vtkErrorMacro(<< "No text properties set!");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::CheckRebuild()
{
  // // Get the highest mtime for the text properties:
  // vtkMTimeType tPropMTime = this->TextProperties->GetMTime();
  // this->TextProperties->InitTraversal();
  // while (vtkTextProperty *tprop = this->TextProperties->GetNextItem())
  // {
  //   tPropMTime = std::max(tPropMTime, tprop->GetMTime());
  // }

  // // Are we out of date?
  // if (this->LabelBuildTime.GetMTime() < this->PolyData->GetMTime() ||
  //     this->LabelBuildTime.GetMTime() < tPropMTime)
  // {
  //   return true;
  // }

  // FIXME: We should figure out how the 3D version managed to keep the
  // FIXME: stencils the right size for the viewport during interaction
  // FIXME: without rebuilding the labels.  For now we just rebuild every
  // FIXME: time or else when we zoom in the backgrounds get much bigger
  // FIXME: than the text.
  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::PrepareRender()
{
  vtkAbstractContextItem* parent = this->GetParent();
  vtkContextTransform* transform = vtkContextTransform::SafeDownCast(parent);

  if (transform == nullptr)
  {
    vtkErrorMacro(<< "No parent or parent is not a vtkContextTransform");
    return false;
  }

  if (!this->Internal->SetViewInfo(this->GetScene(), transform))
  {
    return false;
  }

  // Already checked that these exist in CheckInputs()
  vtkPolyData* input = this->PolyData;
  vtkCellArray* lines = input->GetLines();
  vtkDataArray* scalars = input->GetPointData()->GetScalars();
  vtkTextRenderer* tren = vtkTextRenderer::GetInstance();
  if (!tren)
  {
    vtkErrorMacro(<< "Text renderer unavailable.");
    return false;
  }

  // Maps scalar values to text properties:
  typedef std::map<double, vtkTextProperty*> LabelPropertyMapType;
  LabelPropertyMapType labelMap;

  // Initialize with the user-requested mapping, if it exists.
  if (this->TextPropertyMapping != nullptr)
  {
    vtkDoubleArray::Iterator valIt = this->TextPropertyMapping->Begin();
    vtkDoubleArray::Iterator valItEnd = this->TextPropertyMapping->End();
    TextPropLoop tprops(this->TextProperties);
    for (; valIt != valItEnd; ++valIt)
    {
      labelMap.insert(std::make_pair(*valIt, tprops.Next()));
    }
  }

  // Create the list of metrics, but no text property information yet.
  vtkIdType numPts;
  const vtkIdType* ids;
  for (lines->InitTraversal(); lines->GetNextCell(numPts, ids);)
  {
    this->Internal->LabelMetrics.push_back(PDILabelMetric());
    PDILabelMetric& metric = this->Internal->LabelMetrics.back();
    if (!(metric.Valid = (numPts > 0)))
    {
      // Mark as invalid and skip if there are no points.
      continue;
    }
    metric.Value = scalars->GetComponent(ids[0], 0);
    metric.Value = std::fabs(metric.Value) > 1e-6 ? metric.Value : 0.0;
    std::ostringstream str;
    str << metric.Value;
    metric.Text = str.str();

    // Beware future maintainers: The following line of code has been carefully
    // crafted to reach a zen-like harmony of compatibility between various
    // compilers that have differing syntactic requirements for creating a
    // pair containing a nullptr:
    // - Pedantically strict C++11 compilers (e.g. MSVC 2012) will not compile:
    //     std::make_pair<double, X*>(someDouble, nullptr);
    //   or any make_pair call with explicit template args and value arguments,
    //   as the signature expects an rvalue.

    // The value will be replaced in the next loop:
    labelMap.insert(
      std::pair<double, vtkTextProperty*>(metric.Value, static_cast<vtkTextProperty*>(nullptr)));
  }

  // Now that all present scalar values are known, assign text properties:
  TextPropLoop tprops(this->TextProperties);
  typedef LabelPropertyMapType::iterator LabelPropertyMapIter;
  for (LabelPropertyMapIter it = labelMap.begin(), itEnd = labelMap.end(); it != itEnd; ++it)
  {
    if (!it->second) // Skip if initialized from TextPropertyMapping
    {
      it->second = tprops.Next();
    }
  }

  // Update metrics with appropriate text info:
  typedef std::vector<PDILabelMetric>::iterator MetricsIter;
  for (MetricsIter it = this->Internal->LabelMetrics.begin(),
                   itEnd = this->Internal->LabelMetrics.end();
       it != itEnd; ++it)
  {
    if (!it->Valid)
    {
      continue;
    }

    // Look up text property for the scalar value:
    LabelPropertyMapIter tpropIt = labelMap.find(it->Value);
    assert("No text property assigned for scalar value." && tpropIt != labelMap.end());
    it->TProp = tpropIt->second;

    // Assign bounding box/dims.
    if (!tren->GetBoundingBox(
          it->TProp, it->Text, it->BoundingBox.GetData(), vtkTextActor3D::GetRenderedDPI()))
    {
      vtkErrorMacro(<< "Error calculating bounding box for string '" << it->Text << "'.");
      return false;
    }
    it->Dimensions[0] = it->BoundingBox[1] - it->BoundingBox[0] + 1;
    it->Dimensions[1] = it->BoundingBox[3] - it->BoundingBox[2] + 1;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::PlaceLabels()
{
  vtkPolyData* input = this->PolyData;
  vtkPoints* points = input->GetPoints();
  vtkCellArray* lines = input->GetLines();

  // Progression of smoothness tolerances to use.
  std::vector<double> tols;
  tols.push_back(0.010);
  tols.push_back(0.025);
  tols.push_back(0.050);
  tols.push_back(0.100);
  tols.push_back(0.200);
  tols.push_back(0.300);

  typedef std::vector<PDILabelMetric>::const_iterator MetricsIterator;
  MetricsIterator metric = this->Internal->LabelMetrics.begin();

  // Identify smooth parts of the isoline for labeling
  vtkIdType numIds;
  const vtkIdType* origIds;
  this->Internal->LabelInfos.reserve(this->Internal->LabelMetrics.size());
  for (lines->InitTraversal(); lines->GetNextCell(numIds, origIds); ++metric)
  {
    assert(metric != this->Internal->LabelMetrics.end());

    this->Internal->LabelInfos.push_back(std::vector<PDILabelInfo>());

    // Test if it is possible to place a label (e.g. the line is big enough
    // to not be completely obscured)
    if (this->Internal->LineCanBeLabeled(points, numIds, origIds, *metric))
    {
      std::vector<PDILabelInfo>& infos = this->Internal->LabelInfos.back();
      PDILabelInfo info;
      // If no labels are found, increase the tolerance:
      for (std::vector<double>::const_iterator it = tols.begin(), itEnd = tols.end();
           it != itEnd && infos.empty(); ++it)
      {
        vtkIdType nIds = numIds;
        const vtkIdType* ids = origIds;
        while (this->Internal->NextLabel(points, nIds, ids, *metric, info, *it, this->SkipDistance))
        {
          infos.push_back(info);
        }
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::ResolveLabels()
{
  typedef std::vector<PDILabelInfo>::iterator InnerIterator;
  typedef std::vector<std::vector<PDILabelInfo> >::iterator OuterIterator;

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
bool vtkLabeledContourPolyDataItem::CreateLabels()
{
  typedef std::vector<PDILabelMetric> MetricVector;
  typedef std::vector<PDILabelInfo> InfoVector;

  std::vector<InfoVector>::const_iterator outerLabels = this->Internal->LabelInfos.begin();
  std::vector<InfoVector>::const_iterator outerLabelsEnd = this->Internal->LabelInfos.end();

  // count the number of labels:
  vtkIdType numLabels = 0;
  while (outerLabels != outerLabelsEnd)
  {
    numLabels += static_cast<vtkIdType>((outerLabels++)->size());
  }

  if (!this->AllocateTextActors(numLabels))
  {
    vtkErrorMacro(<< "Error while allocating text actors.");
    return false;
  }

  outerLabels = this->Internal->LabelInfos.begin();
  MetricVector::const_iterator metrics = this->Internal->LabelMetrics.begin();
  MetricVector::const_iterator metricsEnd = this->Internal->LabelMetrics.end();
  vtkTextActor3D** actor = this->TextActors;
  vtkTextActor3D** actorEnd = this->TextActors + this->NumberOfUsedTextActors;
  PDILabelHelper** helper = this->LabelHelpers;

  while (metrics != metricsEnd && outerLabels != outerLabelsEnd && actor != actorEnd)
  {
    for (InfoVector::const_iterator label = outerLabels->begin(), labelEnd = outerLabels->end();
         label != labelEnd; ++label)
    {
      this->Internal->BuildLabel(*actor, *helper, *metrics, *label);
      ++actor;
      ++helper;
    }
    ++metrics;
    ++outerLabels;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::RenderLabels(vtkContext2D* painter)
{
  double pos[3];

  for (vtkIdType i = 0; i < this->NumberOfUsedTextActors; ++i)
  {
    this->TextActors[i]->GetPosition(pos);
    char* s = this->TextActors[i]->GetInput();

    this->Internal->currentTextProp->ShallowCopy(this->TextActors[i]->GetTextProperty());
    this->Internal->currentTextProp->SetOrientation(this->LabelHelpers[i]->orientation);

    painter->ApplyTextProp(this->Internal->currentTextProp);
    painter->DrawString((float)pos[0], (float)pos[1], s);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::AllocateTextActors(vtkIdType num)
{
  if (num != this->NumberOfUsedTextActors)
  {
    if (this->NumberOfTextActors < num || this->NumberOfTextActors > 2 * num)
    {
      this->FreeTextActors();

      // Leave some room to grow:
      this->NumberOfTextActors = num * 1.2;

      this->TextActors = new vtkTextActor3D*[this->NumberOfTextActors];
      for (vtkIdType i = 0; i < this->NumberOfTextActors; ++i)
      {
        this->TextActors[i] = vtkTextActor3D::New();
      }

      this->LabelHelpers = new PDILabelHelper*[this->NumberOfTextActors];
      for (vtkIdType i = 0; i < this->NumberOfTextActors; ++i)
      {
        this->LabelHelpers[i] = new PDILabelHelper();
      }
    }

    this->NumberOfUsedTextActors = num;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::FreeTextActors()
{
  for (vtkIdType i = 0; i < this->NumberOfTextActors; ++i)
  {
    if (this->TextActors[i] != nullptr)
    {
      this->TextActors[i]->Delete();
    }
    delete this->LabelHelpers[i];
  }
  delete[] this->TextActors;
  delete[] this->LabelHelpers;
  this->TextActors = nullptr;
  this->LabelHelpers = nullptr;
  this->NumberOfTextActors = 0;
  this->NumberOfUsedTextActors = 0;
  return true;
}

//-----------------------------------------------------------------------------
void vtkLabeledContourPolyDataItem::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkLabeledContourPolyDataItem::Private::ActorToWorld(
  const vtkVector3d& in, vtkVector3d& out) const
{
  const vtkTuple<double, 16>& x = this->ActorMatrix;
  double w;
  out[0] = in[0] * x[0] + in[1] * x[1] + in[2] * x[2] + x[3];
  out[1] = in[0] * x[4] + in[1] * x[5] + in[2] * x[6] + x[7];
  out[2] = in[0] * x[8] + in[1] * x[9] + in[2] * x[10] + x[11];
  w = in[0] * x[12] + in[1] * x[13] + in[2] * x[14] + x[15];
  out = out * (1. / w);
}

//------------------------------------------------------------------------------
void vtkLabeledContourPolyDataItem::Private::WorldToActor(
  const vtkVector3d& in, vtkVector3d& out) const
{
  const vtkTuple<double, 16>& x = this->InverseActorMatrix;
  double w;
  out[0] = in[0] * x[0] + in[1] * x[1] + in[2] * x[2] + x[3];
  out[1] = in[0] * x[4] + in[1] * x[5] + in[2] * x[6] + x[7];
  out[2] = in[0] * x[8] + in[1] * x[9] + in[2] * x[10] + x[11];
  w = in[0] * x[12] + in[1] * x[13] + in[2] * x[14] + x[15];
  out = out * (1. / w);
}

//------------------------------------------------------------------------------
void vtkLabeledContourPolyDataItem::Private::ActorToDisplay(
  const vtkVector3d& actor, vtkVector2i& out) const
{
  vtkVector2d v;
  this->ActorToDisplay(actor, v);
  out = vtkVector2i(v.Cast<int>().GetData());
}

//------------------------------------------------------------------------------
void vtkLabeledContourPolyDataItem::Private::ActorToDisplay(
  const vtkVector3d& actor, vtkVector2d& v) const
{
  vtkVector2f inputCoords(actor[0], actor[1]);
  vtkVector2f screenCoords = this->item->MapToScene(inputCoords);
  v[0] = screenCoords[0];
  v[1] = screenCoords[1];
}

//------------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::Private::SetViewInfo(
  vtkContextScene* contextScene, vtkContextTransform* transform)
{
  vtkRenderer* ren = contextScene->GetRenderer();

  vtkCamera* cam = ren->GetActiveCamera();
  if (!cam)
  {
    vtkGenericWarningMacro(<< "No active camera on renderer.");
    return false;
  }

  this->CameraRight.Set(1.0, 0.0);
  this->CameraUp.Set(0.0, 1.0);

  // figure out the same aspect ratio used by the render engine
  // (see vtkOpenGLCamera::Render())
  int lowerLeft[2];
  int usize, vsize;
  double aspect1[2];
  double aspect2[2];
  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);
  ren->ComputeAspect();
  ren->GetAspect(aspect1);
  ren->vtkViewport::ComputeAspect();
  ren->vtkViewport::GetAspect(aspect2);
  double aspectModification = (aspect1[0] * aspect2[1]) / (aspect1[1] * aspect2[0]);
  double aspect = aspectModification * usize / vsize;

  // Get the mvp (mcdc) matrix
  double mvp[16];
  vtkMatrix4x4* mat = cam->GetCompositeProjectionTransformMatrix(aspect, -1, 1);
  vtkMatrix4x4::DeepCopy(mvp, mat);

  vtkTransform2D* xform2D = transform->GetTransform();
  vtkMatrix3x3::DeepCopy(this->forwardMatrix.GetData(), xform2D->GetMatrix());
  vtkMatrix4x4::Invert(this->forwardMatrix.GetData(), this->inverseMatrix.GetData());

  // Apply the actor's matrix:
  vtkNew<vtkMatrix4x4> act;
  act->Identity();
  vtkMatrix4x4::DeepCopy(this->ActorMatrix.GetData(), act);

  vtkMatrix4x4::Multiply4x4(mvp, this->ActorMatrix.GetData(), this->AMVP.GetData());

  vtkMatrix4x4::Invert(this->ActorMatrix.GetData(), this->InverseActorMatrix.GetData());

  if (vtkWindow* win = ren->GetVTKWindow())
  {
    int* size = win->GetSize();
    this->WindowSize[0] = size[0];
    this->WindowSize[1] = size[1];

    size = ren->GetSize();
    this->ViewPortSize[0] = size[0];
    this->ViewPortSize[1] = size[1];

    ren->GetViewport(this->ViewPort.GetData());

    double* tvport = win->GetTileViewport();
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
    vtkGenericWarningMacro(<< "No render window present.");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::Private::LineCanBeLabeled(
  vtkPoints* points, vtkIdType numIds, const vtkIdType* ids, const PDILabelMetric& metrics)
{
  vtkTuple<int, 4> bbox(0);
  vtkVector3d actorCoord;
  vtkVector2i displayCoord;
  if (numIds > 0)
  {
    do
    {
      points->GetPoint(*(ids++), actorCoord.GetData());
      this->ActorToDisplay(actorCoord, displayCoord);
      --numIds;
    } while (numIds > 0 && !this->PixelIsVisible(displayCoord));

    if (!this->PixelIsVisible(displayCoord))
    {
      // No visible points
      return false;
    }

    bbox[0] = displayCoord.GetX();
    bbox[1] = displayCoord.GetX();
    bbox[2] = displayCoord.GetY();
    bbox[3] = displayCoord.GetY();
  }
  while (numIds-- > 0)
  {
    points->GetPoint(*(ids++), actorCoord.GetData());
    this->ActorToDisplay(actorCoord, displayCoord);
    if (this->PixelIsVisible(displayCoord))
    {
      bbox[0] = std::min(bbox[0], displayCoord.GetX());
      bbox[1] = std::max(bbox[1], displayCoord.GetX());
      bbox[2] = std::min(bbox[2], displayCoord.GetY());
      bbox[3] = std::max(bbox[3], displayCoord.GetY());
    }
  }

  // Must be at least twice the label length in at least one direction:
  return (
    metrics.Dimensions[0] * 2 < bbox[1] - bbox[0] || metrics.Dimensions[0] * 2 < bbox[3] - bbox[2]);
}

//------------------------------------------------------------------------------
template <typename ScalarType>
bool vtkLabeledContourPolyDataItem::Private::PixelIsVisible(
  const vtkVector2<ScalarType>& dispCoord) const
{
  return (dispCoord.GetX() >= this->ViewportBounds[0] &&
    dispCoord.GetX() <= this->ViewportBounds[1] && dispCoord.GetY() >= this->ViewportBounds[2] &&
    dispCoord.GetY() <= this->ViewportBounds[3]);
}

//------------------------------------------------------------------------------
bool vtkLabeledContourPolyDataItem::Private::NextLabel(vtkPoints* points, vtkIdType& numIds,
  const vtkIdType*& ids, const PDILabelMetric& metrics, PDILabelInfo& info, double targetSmoothness,
  double skipDistance)
{
  if (numIds < 2)
  {
    return false;
  }

  // First point in this call to NextLabel (index into ids).
  vtkIdType firstIdx = 0;
  vtkVector3d firstPoint;
  vtkVector2d firstPointDisplay;
  points->GetPoint(ids[firstIdx], firstPoint.GetData());
  this->ActorToDisplay(firstPoint, firstPointDisplay);

  // Start of current smooth run (index into ids).
  vtkIdType startIdx = 0;
  vtkVector3d startPoint;
  vtkVector2d startPointDisplay;
  points->GetPoint(ids[startIdx], startPoint.GetData());
  this->ActorToDisplay(startPoint, startPointDisplay);

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

  // Account for skip distance:
  while (segment.Norm() < skipDistance)
  {
    ++startIdx;
    points->GetPoint(ids[startIdx], startPoint.GetData());
    this->ActorToDisplay(startPoint, startPointDisplay);

    segment = startPointDisplay - firstPointDisplay;
  }

  // Find the first visible point
  while (startIdx + 1 < numIds && !this->PixelIsVisible(startPointDisplay))
  {
    ++startIdx;
    points->GetPoint(ids[startIdx], startPoint.GetData());
    this->ActorToDisplay(startPoint, startPointDisplay);
  }

  // Start point in current segment.
  vtkVector3d prevPoint = startPoint;
  vtkVector2d prevPointDisplay = startPointDisplay;

  // End point of current segment (index into ids).
  vtkIdType curIdx = startIdx + 1;
  vtkVector3d curPoint = prevPoint;
  vtkVector2d curPointDisplay = prevPointDisplay;

  while (curIdx < numIds)
  {
    // Copy cur --> prev
    prevPoint = curPoint;
    prevPointDisplay = curPointDisplay;
    prevStraight = straight;
    rPrevStraight = rStraight;

    // Update current:
    points->GetPoint(ids[curIdx], curPoint.GetData());
    this->ActorToDisplay(curPoint, curPointDisplay);

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
          this->ActorToDisplay(startPoint, startPointDisplay);
        } while (startIdx < numIds && !this->PixelIsVisible(startPointDisplay));

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

    // The direction of the text.
    vtkVector2d prevDisplay;
    vtkVector2d startDisplay;
    this->ActorToDisplay(prevPoint, prevDisplay);
    this->ActorToDisplay(startPoint, startDisplay);
    info.RightD = (prevDisplay - startDisplay).Normalized();

    // Ensure the text reads left->right:
    if (info.RightD.Dot(this->CameraRight) < 0.)
    {
      info.RightD = -info.RightD;
    }

    info.UpD[0] = info.RightD[1];
    info.UpD[1] = -info.RightD[0];

    if (info.UpD.Dot(this->CameraUp) < 0.)
    {
      info.UpD = -info.UpD;
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
    vtkVector3d offset = curPoint - prevPoint;
    double rSegmentActor = offset.Normalize();
    offset = offset * (targetLength * rSegmentActor / rSegment);
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
bool vtkLabeledContourPolyDataItem::Private::BuildLabel(vtkTextActor3D* actor,
  PDILabelHelper* helper, const PDILabelMetric& metric, const PDILabelInfo& info)

{
  assert(metric.Valid);
  actor->SetInput(metric.Text.c_str());

  helper->orientation = vtkMath::DegreesFromRadians(atan2(info.RightD[1], info.RightD[0]));

  actor->SetTextProperty(metric.TProp);
  actor->SetPosition(const_cast<double*>(info.Position.GetData()));

  return true;
}

//------------------------------------------------------------------------------
void vtkLabeledContourPolyDataItem::Private::ComputeLabelInfo(
  PDILabelInfo& info, const PDILabelMetric& metrics)
{
  vtkVector2d displayPosition;
  this->ActorToDisplay(info.Position, displayPosition);

  // Compute the corners of the quad.  Display coordinates are used to detect
  // collisions.  Note that we make this a little bigger (4px) than a tight
  // bbox to give a little breathing room around the text.
  vtkVector2d displayHalfWidth = (0.5 * metrics.Dimensions[0] + 2) * info.RightD;
  vtkVector2d displayHalfHeight = (0.5 * metrics.Dimensions[1] + 2) * info.UpD;

  info.TLd = (displayPosition + displayHalfHeight - displayHalfWidth).Cast<int>();
  info.TRd = (displayPosition + displayHalfHeight + displayHalfWidth).Cast<int>();
  info.BRd = (displayPosition - displayHalfHeight + displayHalfWidth).Cast<int>();
  info.BLd = (displayPosition - displayHalfHeight - displayHalfWidth).Cast<int>();
}

// Anonymous namespace for some TestOverlap helpers:
namespace
{

// Rotates the vector by -90 degrees.
void perp(vtkVector2i& vec)
{
  std::swap(vec[0], vec[1]);
  vec[1] = -vec[1];
}

// Project all points in other onto the line (point + t * direction).
// Return true if t is positive for all points in other (e.g. all points in
// 'other' are outside the polygon containing 'point').
bool allOutside(const vtkVector2i& point, const vtkVector2i& direction, const PDILabelInfo& other)
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
bool testAxis(const PDILabelInfo& poly, const vtkVector2i& edgeStart, const vtkVector2i& edgeEnd)
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
bool vtkLabeledContourPolyDataItem::Private::TestOverlap(
  const PDILabelInfo& a, const PDILabelInfo& b)
{
  // Note that the order of the points matters, must be CCW to get the correct
  // perpendicular vector:
  return !(testAxis(a, b.TLd, b.BLd) || testAxis(a, b.BLd, b.BRd) || testAxis(a, b.BRd, b.TRd) ||
    testAxis(a, b.TRd, b.TLd) || testAxis(b, a.TLd, a.BLd) || testAxis(b, a.BLd, a.BRd) ||
    testAxis(b, a.BRd, a.TRd) || testAxis(b, a.TRd, a.TLd));
}
