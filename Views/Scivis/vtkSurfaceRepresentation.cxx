// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSurfaceRepresentation.h"

#include "vtkAbstractMapper.h"
#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkBlockProperties.h"
#include "vtkCellData.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataArray.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkExtractSelection.h"
#include "vtkFieldData.h"
#include "vtkGeometryFilterDispatcher.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkScivisView.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include <algorithm>
#include <cstring>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSurfaceRepresentation);

namespace
{

// The attributes that `fieldAssoc`, one of the vtkDataObject::FIELD_ASSOCIATION_*
// values, names on `dataset`. Null for an association a dataset does not have.
vtkFieldData* AttributesFor(vtkDataSet* dataset, int fieldAssoc)
{
  switch (fieldAssoc)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      return dataset->GetPointData();
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      return dataset->GetCellData();
    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      return dataset->GetFieldData();
    default:
      return nullptr;
  }
}

// vtkAbstractMapper::GetScalars() reports where it found the scalars as 0 for
// points, 1 for cells and 2 for field data.
int AssociationForCellFlag(int cellFlag)
{
  switch (cellFlag)
  {
    case 1:
      return vtkDataObject::FIELD_ASSOCIATION_CELLS;
    case 2:
      return vtkDataObject::FIELD_ASSOCIATION_NONE;
    default:
      return vtkDataObject::FIELD_ASSOCIATION_POINTS;
  }
}

// The scalars `mapper` will render from `dataObject`, resolved the way the
// mapper resolves them, taking the first block of a composite dataset that has
// an answer.
vtkDataArray* FindRenderedScalars(vtkDataObject* dataObject, vtkMapper* mapper, int& cellFlag)
{
  if (auto* composite = vtkCompositeDataSet::SafeDownCast(dataObject))
  {
    auto iterator = vtkSmartPointer<vtkCompositeDataIterator>::Take(composite->NewIterator());
    for (iterator->InitTraversal(); !iterator->IsDoneWithTraversal(); iterator->GoToNextItem())
    {
      if (vtkDataArray* scalars =
            FindRenderedScalars(iterator->GetCurrentDataObject(), mapper, cellFlag))
      {
        return scalars;
      }
    }
    return nullptr;
  }

  auto* dataset = vtkDataSet::SafeDownCast(dataObject);
  if (!dataset)
  {
    return nullptr;
  }

  return vtkAbstractMapper::GetScalars(dataset, mapper->GetScalarMode(),
    mapper->GetArrayAccessMode(), mapper->GetArrayId(), mapper->GetArrayName(), cellFlag);
}

// Widen `range` to cover `arrayName` wherever it is found in `dataObject`,
// descending into the blocks of a composite dataset. Returns whether the array
// was found at all; `range` is only touched when it was.
bool AccumulateRange(
  vtkDataObject* dataObject, const char* arrayName, int fieldAssoc, int component, double range[2])
{
  if (auto* composite = vtkCompositeDataSet::SafeDownCast(dataObject))
  {
    bool found = false;
    auto iterator = vtkSmartPointer<vtkCompositeDataIterator>::Take(composite->NewIterator());
    for (iterator->InitTraversal(); !iterator->IsDoneWithTraversal(); iterator->GoToNextItem())
    {
      found |=
        AccumulateRange(iterator->GetCurrentDataObject(), arrayName, fieldAssoc, component, range);
    }
    return found;
  }

  auto* dataset = vtkDataSet::SafeDownCast(dataObject);
  if (!dataset)
  {
    return false;
  }

  vtkFieldData* attributes = AttributesFor(dataset, fieldAssoc);
  vtkDataArray* array = attributes ? attributes->GetArray(arrayName) : nullptr;
  if (!array || component >= array->GetNumberOfComponents())
  {
    // vtkDataArray::GetRange() leaves the range untouched for a component the
    // array does not have, so asking would tell us nothing.
    return false;
  }

  double blockRange[2];
  array->GetRange(blockRange, component);
  range[0] = std::min(range[0], blockRange[0]);
  range[1] = std::max(range[1], blockRange[1]);
  return true;
}

}

//------------------------------------------------------------------------------
vtkSurfaceRepresentation::vtkSurfaceRepresentation()
{
  this->SetNumberOfInputPorts(1);

  this->GeometryFilter->UseOutlineOff();
  // Explicitly enable pass-through of original IDs so that ConvertSelection
  // can map rendered surface cell/point IDs back to the original dataset.
  this->GeometryFilter->PassThroughCellIdsOn();
  this->GeometryFilter->PassThroughPointIdsOn();

  this->RepresentationValue = SURFACE;

  // The lookup table owns the range that scalars are mapped through.  Without
  // this the mapper writes its own range over the table's on every render,
  // which silently discards the range of a map handed to SetColorMap().
  this->Mapper->UseLookupTableScalarRangeOn();

  // vtkMapper builds its default lookup table lazily, which would leave
  // GetColorMap() modifying the representation the first time it is asked.
  // Settle the table here so that the getter stays a getter.
  this->Mapper->GetLookupTable();

  // vtkCompositePolyDataMapper has no display attributes until it is given
  // some, and an accessor that returns null until the caller guesses to build
  // one is no use.  Give it a set here so per-block properties can just be set.
  vtkNew<vtkCompositeDataDisplayAttributes> blockAttributes;
  this->Mapper->SetCompositeDataDisplayAttributes(blockAttributes);

  // Per-block appearance lives in its own object; it needs the mapper it acts
  // on and the representation whose data a block index is resolved against.
  this->Blocks->SetMapper(this->Mapper);
  this->Blocks->SetRepresentation(this);

  this->Actor->SetMapper(this->Mapper);

  // The clipping planes belong to the contract, which owns the collection; the
  // mapper follows it from here on, including the selection's mapper so that a
  // highlight is clipped along with what it highlights.
  this->Mapper->SetClippingPlanes(this->GetClippingPlanes());
  this->SelectionMapper->SetClippingPlanes(this->GetClippingPlanes());

  // Selection visualization pipeline
  this->SelectionGeometryFilter->UseOutlineOff();
  this->SelectionGeometryFilter->SetInputConnection(this->SelectionExtractor->GetOutputPort());
  this->SelectionMapper->SetInputConnection(this->SelectionGeometryFilter->GetOutputPort());
  this->SelectionMapper->ScalarVisibilityOff();
  this->SelectionMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->SelectionActor->SetMapper(this->SelectionMapper);
  this->SelectionActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  this->SelectionActor->GetProperty()->SetLineWidth(2.0);
  this->SelectionActor->GetProperty()->SetPointSize(8.0);
  this->SelectionActor->GetProperty()->SetRenderPointsAsSpheres(true);
  this->SelectionActor->GetProperty()->SetRepresentationToWireframe();
  this->SelectionRepresentationValue = WIREFRAME;
  this->UserSetSelectionRepresentation = false;
  this->SelectionActor->GetProperty()->LightingOff();
  this->SelectionActor->SetVisibility(false);
  this->SelectionActor->SetPickable(false);
}

//------------------------------------------------------------------------------
vtkSurfaceRepresentation::~vtkSurfaceRepresentation() = default;

//------------------------------------------------------------------------------
int vtkSurfaceRepresentation::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkAlgorithmOutput* inputPort = this->GetInternalOutputPort();
  if (inputPort)
  {
    this->GeometryFilter->SetInputConnection(inputPort);
    this->GeometryFilter->Update();
    this->Mapper->SetInputDataObject(this->GeometryFilter->GetOutputDataObject(0));
  }
  return 1;
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::AddToView(vtkScivisView* view)
{
  if (!view)
  {
    return false;
  }
  view->GetRenderer()->AddActor(this->Actor);
  view->GetRenderer()->AddActor(this->SelectionActor);
  return true;
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::RemoveFromView(vtkScivisView* view)
{
  if (!view)
  {
    return false;
  }
  view->GetRenderer()->RemoveActor(this->Actor);
  view->GetRenderer()->RemoveActor(this->SelectionActor);
  return true;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkSurfaceRepresentation::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  mTime = std::max(mTime, this->GeometryFilter->GetMTime());
  mTime = std::max(mTime, this->Mapper->GetMTime());
  // vtkActor::GetMTime() already accounts for the property.
  mTime = std::max(mTime, this->Actor->GetMTime());
  mTime = std::max(mTime, this->SelectionExtractor->GetMTime());
  mTime = std::max(mTime, this->SelectionGeometryFilter->GetMTime());
  mTime = std::max(mTime, this->SelectionMapper->GetMTime());
  mTime = std::max(mTime, this->SelectionActor->GetMTime());
  return mTime;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetRepresentation(int type)
{
  // POINTS through SURFACE_WITH_EDGES only configure the property; OUTLINE and
  // FEATURE_EDGES also change what the geometry filter extracts.  Because the
  // modes are mutually exclusive, every case sets all four values.
  int representation;
  bool edgeVisibility;
  bool useOutline;
  bool generateFeatureEdges;
  switch (type)
  {
    case POINTS:
      representation = VTK_POINTS;
      edgeVisibility = false;
      useOutline = false;
      generateFeatureEdges = false;
      break;
    case WIREFRAME:
      representation = VTK_WIREFRAME;
      edgeVisibility = false;
      useOutline = false;
      generateFeatureEdges = false;
      break;
    case SURFACE:
      representation = VTK_SURFACE;
      edgeVisibility = false;
      useOutline = false;
      generateFeatureEdges = false;
      break;
    case SURFACE_WITH_EDGES:
      representation = VTK_SURFACE;
      edgeVisibility = true;
      useOutline = false;
      generateFeatureEdges = false;
      break;
    case OUTLINE:
      // The filter emits the bounding box as lines, so the property mode only
      // has to avoid asking for a surface.
      representation = VTK_WIREFRAME;
      edgeVisibility = false;
      useOutline = true;
      generateFeatureEdges = false;
      break;
    case FEATURE_EDGES:
      representation = VTK_WIREFRAME;
      edgeVisibility = false;
      useOutline = false;
      generateFeatureEdges = true;
      break;
    default:
      vtkWarningMacro("Unknown representation type: " << type);
      return;
  }
  if (this->RepresentationValue == type)
  {
    return;
  }
  this->RepresentationValue = type;
  vtkProperty* prop = this->Actor->GetProperty();
  prop->SetRepresentation(representation);
  prop->SetEdgeVisibility(edgeVisibility);
  this->GeometryFilter->SetUseOutline(useOutline);
  this->GeometryFilter->SetGenerateFeatureEdges(generateFeatureEdges);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkSurfaceRepresentation::GetRepresentation()
{
  return this->RepresentationValue;
}

//------------------------------------------------------------------------------
const char* vtkSurfaceRepresentation::GetRepresentationAsString()
{
  switch (this->RepresentationValue)
  {
    case POINTS:
      return "Points";
    case WIREFRAME:
      return "Wireframe";
    case SURFACE:
      return "Surface";
    case SURFACE_WITH_EDGES:
      return "SurfaceWithEdges";
    case OUTLINE:
      return "Outline";
    case FEATURE_EDGES:
      return "FeatureEdges";
    default:
      return "Unknown";
  }
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetColor(double r, double g, double b)
{
  double* current = this->GetColor();
  if (current[0] == r && current[1] == g && current[2] == b)
  {
    return;
  }
  this->Actor->GetProperty()->SetColor(r, g, b);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkSurfaceRepresentation::GetColor()
{
  return this->Actor->GetProperty()->GetColor();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetOpacity(double val)
{
  if (this->GetOpacity() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetOpacity(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetOpacity()
{
  return this->Actor->GetProperty()->GetOpacity();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetEdgeColor(double r, double g, double b)
{
  double* current = this->GetEdgeColor();
  if (current[0] == r && current[1] == g && current[2] == b)
  {
    return;
  }
  this->Actor->GetProperty()->SetEdgeColor(r, g, b);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkSurfaceRepresentation::GetEdgeColor()
{
  return this->Actor->GetProperty()->GetEdgeColor();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetScalarVisibility(bool val)
{
  if (this->GetScalarVisibility() == val)
  {
    return;
  }
  this->Mapper->SetScalarVisibility(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetScalarVisibility()
{
  return this->Mapper->GetScalarVisibility() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByPointArray(const char* arrayName)
{
  if (this->IsColoringBy(arrayName, VTK_SCALAR_MODE_USE_POINT_FIELD_DATA))
  {
    return;
  }
  this->Mapper->SetScalarVisibility(true);
  this->Mapper->SetScalarModeToUsePointFieldData();
  this->Mapper->SelectColorArray(arrayName);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByPointArray(const char* arrayName, int component)
{
  this->ColorByPointArray(arrayName);
  this->ColorByComponent(component);
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByCellArray(const char* arrayName)
{
  if (this->IsColoringBy(arrayName, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA))
  {
    return;
  }
  this->Mapper->SetScalarVisibility(true);
  this->Mapper->SetScalarModeToUseCellFieldData();
  this->Mapper->SelectColorArray(arrayName);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByCellArray(const char* arrayName, int component)
{
  this->ColorByCellArray(arrayName);
  this->ColorByComponent(component);
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByFieldArray(const char* arrayName)
{
  if (this->IsColoringBy(arrayName, VTK_SCALAR_MODE_USE_FIELD_DATA))
  {
    return;
  }
  this->Mapper->SetScalarVisibility(true);
  this->Mapper->SetScalarModeToUseFieldData();
  this->Mapper->SelectColorArray(arrayName);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByFieldArray(const char* arrayName, int component)
{
  this->ColorByFieldArray(arrayName);
  this->ColorByComponent(component);
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::IsColoringBy(const char* arrayName, int scalarMode)
{
  if (!this->Mapper->GetScalarVisibility() || this->Mapper->GetScalarMode() != scalarMode ||
    this->Mapper->GetArrayAccessMode() != VTK_GET_ARRAY_BY_NAME)
  {
    return false;
  }
  const char* current = this->Mapper->GetArrayName();
  if (!current || !arrayName)
  {
    return current == arrayName;
  }
  return strcmp(current, arrayName) == 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByComponent(int component)
{
  vtkScalarsToColors* lut = this->Mapper->GetLookupTable();
  if (this->Mapper->GetColorMode() == VTK_COLOR_MODE_MAP_SCALARS &&
    lut->GetVectorMode() == vtkScalarsToColors::COMPONENT && lut->GetVectorComponent() == component)
  {
    return;
  }
  this->Mapper->SetColorModeToMapScalars();
  lut->SetVectorModeToComponent();
  lut->SetVectorComponent(component);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetColorMap(vtkScalarsToColors* map)
{
  if (this->GetColorMap() == map)
  {
    return;
  }
  this->Mapper->SetLookupTable(map);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkScalarsToColors* vtkSurfaceRepresentation::GetColorMap()
{
  return this->Mapper->GetLookupTable();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetVisibility(bool val)
{
  if (this->GetVisibility() == val)
  {
    return;
  }
  this->Actor->SetVisibility(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetVisibility()
{
  return this->Actor->GetVisibility() != 0;
}

//------------------------------------------------------------------------------
vtkActor* vtkSurfaceRepresentation::GetActor()
{
  return this->Actor;
}

//------------------------------------------------------------------------------
vtkSelection* vtkSurfaceRepresentation::ConvertSelection(
  vtkScivisView* vtkNotUsed(view), vtkSelection* selection)
{
  if (!selection || selection->GetNumberOfNodes() == 0)
  {
    this->SelectionActor->SetVisibility(false);
    // Return an empty selection so the base class UpdateSelection()
    // clears this representation's AnnotationLink.
    return vtkSelection::New();
  }

  vtkAlgorithmOutput* inputPort = this->GetInternalOutputPort();
  if (!inputPort)
  {
    return nullptr;
  }

  // Filter selection nodes to only those relevant to this representation.
  // Hardware-selector results tag each node with PROP identifying the picked actor.
  // Frustum selections have no PROP and apply to all representations.
  //
  // For INDICES selections (hardware pick), the IDs refer to the rendered
  // polydata (geometry filter output), not the original dataset.  Map them
  // back using vtkOriginalCellIds / vtkOriginalPointIds arrays that the
  // geometry filter produces when PassThroughCellIds/PointIds is on.
  //
  // When the geometry filter output is a composite dataset (e.g. from a
  // vtkPartitionedDataSet input), each selection node carries a
  // COMPOSITE_INDEX identifying which leaf it belongs to.  We look up the
  // original-ID arrays from the matching leaf and preserve COMPOSITE_INDEX
  // on the mapped node so that vtkExtractSelection extracts from the
  // correct partition.

  // For non-composite output, get the single surface polydata.
  // For composite output, build parallel leaf maps so we can translate
  // COMPOSITE_INDEX values from the GF output tree to the input tree
  // (they differ because the GF may wrap the output in an extra level).
  vtkDataObject* gfOutput = this->GeometryFilter->GetOutputDataObject(0);
  vtkPolyData* singleSurface = vtkPolyData::SafeDownCast(gfOutput);
  vtkDataObjectTree* compositeOutput = vtkDataObjectTree::SafeDownCast(gfOutput);

  // Build parallel arrays: for each leaf index, store the GF-output leaf
  // polydata and the corresponding input-tree flat index.
  std::vector<vtkPolyData*> gfLeaves;
  std::vector<unsigned int> gfLeafFlatIndices;
  std::vector<unsigned int> inputLeafFlatIndices;

  if (compositeOutput)
  {
    auto* iter = compositeOutput->NewTreeIterator();
    iter->VisitOnlyLeavesOn();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      gfLeaves.push_back(vtkPolyData::SafeDownCast(iter->GetCurrentDataObject()));
      gfLeafFlatIndices.push_back(iter->GetCurrentFlatIndex());
      iter->GoToNextItem();
    }
    iter->Delete();

    // Walk the input tree to get corresponding flat indices.
    vtkDataObject* inputObj = inputPort->GetProducer()->GetOutputDataObject(0);
    vtkDataObjectTree* inputTree = vtkDataObjectTree::SafeDownCast(inputObj);
    if (inputTree)
    {
      auto* iiter = inputTree->NewTreeIterator();
      iiter->VisitOnlyLeavesOn();
      iiter->InitTraversal();
      while (!iiter->IsDoneWithTraversal())
      {
        inputLeafFlatIndices.push_back(iiter->GetCurrentFlatIndex());
        iiter->GoToNextItem();
      }
      iiter->Delete();
    }
  }

  vtkSelection* filtered = vtkSelection::New();
  for (unsigned int i = 0; i < selection->GetNumberOfNodes(); ++i)
  {
    vtkSelectionNode* node = selection->GetNode(i);

    // Frustum nodes apply to all representations — pass through as-is.
    if (node->GetContentType() == vtkSelectionNode::FRUSTUM)
    {
      filtered->AddNode(node);
      continue;
    }

    // Filter by PROP to match only this representation's actor.
    vtkInformation* nodeProps = node->GetProperties();
    if (nodeProps->Has(vtkSelectionNode::PROP()))
    {
      vtkObjectBase* obj = nodeProps->Get(vtkSelectionNode::PROP());
      if (obj != static_cast<vtkObjectBase*>(this->Actor.Get()))
      {
        continue;
      }
    }

    // For INDICES nodes, map surface IDs back to original dataset IDs.
    if (node->GetContentType() == vtkSelectionNode::INDICES)
    {
      vtkIdTypeArray* selList = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      if (!selList || selList->GetNumberOfTuples() == 0)
      {
        continue;
      }

      bool isPoint = (node->GetFieldType() == vtkSelectionNode::POINT);

      // Find the correct surface leaf for this selection node.
      vtkPolyData* leafSurface = singleSurface;
      int inputFlatIndex = -1;

      if (!leafSurface && !gfLeaves.empty() && nodeProps->Has(vtkSelectionNode::COMPOSITE_INDEX()))
      {
        int gfFlatIndex = nodeProps->Get(vtkSelectionNode::COMPOSITE_INDEX());
        // Find which leaf in the GF output matches this flat index.
        for (size_t li = 0; li < gfLeafFlatIndices.size(); ++li)
        {
          if (static_cast<int>(gfLeafFlatIndices[li]) == gfFlatIndex)
          {
            leafSurface = gfLeaves[li];
            // Map to the corresponding input tree flat index.
            if (li < inputLeafFlatIndices.size())
            {
              inputFlatIndex = static_cast<int>(inputLeafFlatIndices[li]);
            }
            break;
          }
        }
      }

      vtkIdTypeArray* origIds = nullptr;
      if (leafSurface)
      {
        origIds = isPoint ? vtkIdTypeArray::SafeDownCast(
                              leafSurface->GetPointData()->GetArray("vtkOriginalPointIds"))
                          : vtkIdTypeArray::SafeDownCast(
                              leafSurface->GetCellData()->GetArray("vtkOriginalCellIds"));
      }

      if (origIds)
      {
        // Map surface IDs to original dataset IDs.
        vtkNew<vtkIdTypeArray> mappedIds;
        mappedIds->SetNumberOfComponents(1);
        for (vtkIdType j = 0; j < selList->GetNumberOfTuples(); ++j)
        {
          vtkIdType surfaceId = selList->GetValue(j);
          if (surfaceId >= 0 && surfaceId < origIds->GetNumberOfTuples())
          {
            mappedIds->InsertNextValue(origIds->GetValue(surfaceId));
          }
        }
        if (mappedIds->GetNumberOfTuples() > 0)
        {
          vtkNew<vtkSelectionNode> mappedNode;
          mappedNode->SetContentType(vtkSelectionNode::INDICES);
          mappedNode->SetFieldType(node->GetFieldType());
          mappedNode->SetSelectionList(mappedIds);
          // Use the INPUT tree's flat index (not the GF output's).
          if (inputFlatIndex >= 0)
          {
            mappedNode->GetProperties()->Set(vtkSelectionNode::COMPOSITE_INDEX(), inputFlatIndex);
          }
          filtered->AddNode(mappedNode);
        }
      }
      else
      {
        // No original ID arrays available — pass through as-is.
        filtered->AddNode(node);
      }
    }
    else
    {
      // Other content types — pass through.
      filtered->AddNode(node);
    }
  }

  if (filtered->GetNumberOfNodes() == 0)
  {
    this->SelectionActor->SetVisibility(false);
    // Return the empty filtered selection so the base class UpdateSelection()
    // clears this representation's AnnotationLink.
    return filtered;
  }

  // Detect if this is a point selection so we can adjust the visual style.
  bool isPointSelection = false;
  for (unsigned int i = 0; i < filtered->GetNumberOfNodes(); ++i)
  {
    vtkSelectionNode* fnode = filtered->GetNode(i);
    if (fnode->GetFieldType() == vtkSelectionNode::POINT)
    {
      isPointSelection = true;
      break;
    }
  }

  // For point selections, render as points; for cell selections, as wireframe.
  // An explicit SetSelectionRepresentation() opts out of this entirely.
  if (!this->UserSetSelectionRepresentation)
  {
    this->SelectionRepresentationValue = isPointSelection ? POINTS : WIREFRAME;
    this->SelectionActor->GetProperty()->SetRepresentation(
      isPointSelection ? VTK_POINTS : VTK_WIREFRAME);
  }

  this->SelectionExtractor->SetInputConnection(0, inputPort);
  this->SelectionExtractor->SetInputData(1, filtered);
  this->SelectionExtractor->Update();

  // Check if extraction produced any geometry.  For composite outputs,
  // the extraction result is also composite — check total cell count.
  vtkDataObject* extOutput = this->SelectionExtractor->GetOutputDataObject(0);
  vtkDataSet* extractedDS = vtkDataSet::SafeDownCast(extOutput);
  vtkCompositeDataSet* extractedComposite = vtkCompositeDataSet::SafeDownCast(extOutput);
  vtkIdType totalCells = 0;
  vtkIdType totalPoints = 0;
  if (extractedDS)
  {
    totalCells = extractedDS->GetNumberOfCells();
    totalPoints = extractedDS->GetNumberOfPoints();
  }
  else if (extractedComposite)
  {
    auto* iter = extractedComposite->NewIterator();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      vtkDataSet* leaf = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (leaf)
      {
        totalCells += leaf->GetNumberOfCells();
        totalPoints += leaf->GetNumberOfPoints();
      }
      iter->GoToNextItem();
    }
    iter->Delete();
  }

  if (totalCells == 0 && totalPoints == 0)
  {
    this->SelectionActor->SetVisibility(false);
    return filtered;
  }

  this->SelectionGeometryFilter->SetInputConnection(this->SelectionExtractor->GetOutputPort());
  this->SelectionGeometryFilter->Update();

  this->SelectionMapper->SetInputDataObject(this->SelectionGeometryFilter->GetOutputDataObject(0));
  this->SelectionActor->SetVisibility(true);

  // Return the filtered selection with mapped IDs — base class will store
  // it in the AnnotationLink and delete it (since filtered != selection).
  return filtered;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetSelectionRepresentation(int type)
{
  int representation;
  bool edgeVisibility;
  switch (type)
  {
    case POINTS:
      representation = VTK_POINTS;
      edgeVisibility = false;
      break;
    case WIREFRAME:
      representation = VTK_WIREFRAME;
      edgeVisibility = false;
      break;
    case SURFACE:
      representation = VTK_SURFACE;
      edgeVisibility = false;
      break;
    case SURFACE_WITH_EDGES:
      representation = VTK_SURFACE;
      edgeVisibility = true;
      break;
    case OUTLINE:
    case FEATURE_EDGES:
      vtkWarningMacro("The selection is drawn from the representation's own geometry, so the "
        << (type == OUTLINE ? "OUTLINE" : "FEATURE_EDGES") << " mode cannot be applied to it.");
      return;
    default:
      vtkWarningMacro("Unknown selection representation type: " << type);
      return;
  }
  // Record the user's intent even when the style is unchanged, so the
  // automatic point/wireframe choice never overrides it later.
  this->UserSetSelectionRepresentation = true;
  if (this->SelectionRepresentationValue == type)
  {
    return;
  }
  this->SelectionRepresentationValue = type;
  vtkProperty* prop = this->SelectionActor->GetProperty();
  prop->SetRepresentation(representation);
  prop->SetEdgeVisibility(edgeVisibility);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkSurfaceRepresentation::GetSelectionRepresentation()
{
  return this->SelectionRepresentationValue;
}

//------------------------------------------------------------------------------
vtkActor* vtkSurfaceRepresentation::GetSelectionActor()
{
  return this->SelectionActor;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ResetColorArray()
{
  if (this->Mapper->GetScalarMode() == VTK_SCALAR_MODE_DEFAULT &&
    this->Mapper->GetArrayAccessMode() == VTK_GET_ARRAY_BY_ID)
  {
    return;
  }
  this->Mapper->SetScalarModeToDefault();
  this->Mapper->SelectColorArray(-1);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkDataArray* vtkSurfaceRepresentation::GetRenderedScalars(int& fieldAssoc)
{
  if (!this->Mapper->GetScalarVisibility())
  {
    // Nothing is being mapped to colors at all.
    return nullptr;
  }

  if (this->GetNumberOfInputConnections(0) > 0)
  {
    this->Update();
  }

  int cellFlag = 0;
  vtkDataArray* scalars =
    ::FindRenderedScalars(this->Mapper->GetInputDataObject(0, 0), this->Mapper, cellFlag);
  fieldAssoc = ::AssociationForCellFlag(cellFlag);
  return scalars;
}

//------------------------------------------------------------------------------
const char* vtkSurfaceRepresentation::GetRenderedArrayName()
{
  int fieldAssoc = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  vtkDataArray* scalars = this->GetRenderedScalars(fieldAssoc);
  return scalars ? scalars->GetName() : nullptr;
}

//------------------------------------------------------------------------------
int vtkSurfaceRepresentation::GetRenderedFieldAssociation()
{
  int fieldAssoc = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  this->GetRenderedScalars(fieldAssoc);
  return fieldAssoc;
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetDataRange(
  const char* arrayName, int fieldAssoc, double range[2], int component)
{
  if (!arrayName || arrayName[0] == '\0' || this->GetNumberOfInputConnections(0) < 1)
  {
    return false;
  }

  // Report over the input rather than over the extracted surface. A surface and
  // a volume coloring by the same array have to contribute comparable numbers
  // to the scalar bar they share, and the interior values the geometry filter
  // drops are still part of the array's range.
  this->GetInputAlgorithm(0, 0)->Update();

  double found[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  if (!AccumulateRange(this->GetInputDataObject(0, 0), arrayName, fieldAssoc, component, found))
  {
    return false;
  }

  range[0] = found[0];
  range[1] = found[1];
  return true;
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetBounds(double bounds[6])
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return false;
  }
  this->Update();

  const double* actorBounds = this->Actor->GetBounds();
  if (!actorBounds || actorBounds[0] > actorBounds[1])
  {
    // vtkProp3D reports an inverted box when there is nothing to bound.
    return false;
  }
  std::copy(actorBounds, actorBounds + 6, bounds);
  return true;
}

//------------------------------------------------------------------------------
vtkProperty* vtkSurfaceRepresentation::GetProperty()
{
  return this->Actor->GetProperty();
}

//------------------------------------------------------------------------------
vtkGeometryFilterDispatcher* vtkSurfaceRepresentation::GetGeometryFilter()
{
  return this->GeometryFilter;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetColorMode(int mode)
{
  if (this->GetColorMode() == mode)
  {
    return;
  }
  switch (mode)
  {
    case MAP_SCALARS:
      this->Mapper->SetColorModeToMapScalars();
      break;
    case DIRECT_SCALARS:
      this->Mapper->SetColorModeToDirectScalars();
      break;
    default:
      vtkWarningMacro("Unknown color mode: " << mode);
      return;
  }
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkSurfaceRepresentation::GetColorMode()
{
  return this->Mapper->GetColorMode() == VTK_COLOR_MODE_DIRECT_SCALARS ? DIRECT_SCALARS
                                                                       : MAP_SCALARS;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetFieldDataTupleId(vtkIdType id)
{
  if (this->GetFieldDataTupleId() == id)
  {
    return;
  }
  this->Mapper->SetFieldDataTupleId(id);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkIdType vtkSurfaceRepresentation::GetFieldDataTupleId()
{
  return this->Mapper->GetFieldDataTupleId();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetInterpolateScalarsBeforeMapping(bool val)
{
  if (this->GetInterpolateScalarsBeforeMapping() == val)
  {
    return;
  }
  this->Mapper->SetInterpolateScalarsBeforeMapping(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetInterpolateScalarsBeforeMapping()
{
  return this->Mapper->GetInterpolateScalarsBeforeMapping() != 0;
}

//------------------------------------------------------------------------------
vtkBlockProperties* vtkSurfaceRepresentation::GetBlocks()
{
  return this->Blocks;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RepresentationValue: " << this->RepresentationValue << " ("
     << this->GetRepresentationAsString() << ")\n";
  os << indent << "FieldDataTupleId: " << this->Mapper->GetFieldDataTupleId() << "\n";
  os << indent << "SelectionRepresentationValue: " << this->SelectionRepresentationValue << "\n";
  os << indent << "UserSetSelectionRepresentation: " << this->UserSetSelectionRepresentation
     << "\n";
  os << indent << "GeneratePointNormals: " << this->GeometryFilter->GetGeneratePointNormals()
     << "\n";
  os << indent << "GenerateCellNormals: " << this->GeometryFilter->GetGenerateCellNormals() << "\n";
  os << indent << "FeatureAngle: " << this->GeometryFilter->GetFeatureAngle() << "\n";
  os << indent << "Triangulate: " << this->GeometryFilter->GetTriangulate() << "\n";
  os << indent
     << "NonlinearSubdivisionLevel: " << this->GeometryFilter->GetNonlinearSubdivisionLevel()
     << "\n";
  os << indent << "MatchBoundariesIgnoringCellOrder: "
     << this->GeometryFilter->GetMatchBoundariesIgnoringCellOrder() << "\n";
  os << indent << "PassThroughCellIds: " << this->GeometryFilter->GetPassThroughCellIds() << "\n";
  os << indent << "PassThroughPointIds: " << this->GeometryFilter->GetPassThroughPointIds() << "\n";
  os << indent
     << "BlockColorsDistinctValues: " << this->GeometryFilter->GetBlockColorsDistinctValues()
     << "\n";
  os << indent << "HideInternalAMRFaces: " << this->GeometryFilter->GetHideInternalAMRFaces()
     << "\n";
  os << indent << "UseNonOverlappingAMRMetaDataForOutlines: "
     << this->GeometryFilter->GetUseNonOverlappingAMRMetaDataForOutlines() << "\n";
  os << indent << "GenerateProcessIds: " << this->GeometryFilter->GetGenerateProcessIds() << "\n";
  os << indent << "Actor: " << this->Actor << "\n";
  os << indent << "Mapper: " << this->Mapper << "\n";
  os << indent << "GeometryFilter: " << this->GeometryFilter << "\n";
  os << indent << "SelectionActor: " << this->SelectionActor << "\n";
}

VTK_ABI_NAMESPACE_END
