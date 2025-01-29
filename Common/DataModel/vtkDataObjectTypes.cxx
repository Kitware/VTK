// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_5_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkDataObjectTypes.h"

#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkArrayData.h"
#include "vtkBSPCuts.h"
#include "vtkCellGrid.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDirectedAcyclicGraph.h"
#include "vtkDirectedGraph.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkGenericDataSet.h"
#include "vtkGraph.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMolecule.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPath.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkReebGraph.h"
#include "vtkSelection.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkTable.h"
#include "vtkTree.h"
#include "vtkUndirectedGraph.h"
#include "vtkUniformGrid.h"
#include "vtkUniformHyperTreeGrid.h"
#include "vtkUnstructuredGrid.h"

#include <map>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDataObjectTypes);

// This list should contain the data object class names in
// the same order as the #define's in vtkType.h. Make sure
// this list is nullptr terminated.
static const char* vtkDataObjectTypesStrings[] = {
  "vtkPolyData",
  "vtkStructuredPoints",
  "vtkStructuredGrid",
  "vtkRectilinearGrid",
  "vtkUnstructuredGrid",
  "vtkPiecewiseFunction",
  "vtkImageData",
  "vtkDataObject",
  "vtkDataSet",
  "vtkPointSet",
  "vtkUniformGrid",
  "vtkCompositeDataSet",
  "vtkMultiGroupDataSet", // OBSOLETE
  "vtkMultiBlockDataSet",
  "vtkHierarchicalDataSet",    // OBSOLETE
  "vtkHierarchicalBoxDataSet", // OBSOLETE
  "vtkGenericDataSet",
  "vtkHyperOctree",     // OBSOLETE
  "vtkTemporalDataSet", // OBSOLETE
  "vtkTable",
  "vtkGraph",
  "vtkTree",
  "vtkSelection",
  "vtkDirectedGraph",
  "vtkUndirectedGraph",
  "vtkMultiPieceDataSet",
  "vtkDirectedAcyclicGraph",
  "vtkArrayData",
  "vtkReebGraph",
  "vtkUniformGridAMR",
  "vtkNonOverlappingAMR",
  "vtkOverlappingAMR",
  "vtkHyperTreeGrid",
  "vtkMolecule",
  "vtkPistonDataObject", // OBSOLETE
  "vtkPath",
  "vtkUnstructuredGridBase",
  "vtkPartitionedDataSet",
  "vtkPartitionedDataSetCollection",
  "vtkUniformHyperTreeGrid",
  "vtkExplicitStructuredGrid",
  "vtkDataObjectTree",
  "vtkAbstractElectronicData",
  "vtkOpenQubeElectronicData",
  "vtkAnnotation",
  "vtkAnnotationLayers",
  "vtkBSPCuts",
  "vtkGeoJSONFeature",
  "vtkImageStencilData",
  "vtkCellGrid",
  nullptr,
};

namespace
{
bool IsTypeIdValid(int typeId)
{
  return (typeId >= VTK_POLY_DATA && typeId <= VTK_CELL_GRID);
}
}

//------------------------------------------------------------------------------
const char* vtkDataObjectTypes::GetClassNameFromTypeId(int type)
{
  if (::IsTypeIdValid(type))
  {
    return vtkDataObjectTypesStrings[type];
  }
  else
  {
    return "UnknownClass";
  }
}

//------------------------------------------------------------------------------
int vtkDataObjectTypes::GetTypeIdFromClassName(const char* classname)
{
  if (!classname)
  {
    return -1;
  }
  for (int idx = 0; vtkDataObjectTypesStrings[idx] != nullptr; ++idx)
  {
    if (strcmp(vtkDataObjectTypesStrings[idx], classname) == 0)
    {
      return idx;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
vtkDataObject* vtkDataObjectTypes::NewDataObject(int type)
{
  switch (type)
  {
    case VTK_POLY_DATA:
      return vtkPolyData::New();
    case VTK_STRUCTURED_POINTS:
      return vtkStructuredPoints::New();
    case VTK_STRUCTURED_GRID:
      return vtkStructuredGrid::New();
    case VTK_RECTILINEAR_GRID:
      return vtkRectilinearGrid::New();
    case VTK_UNSTRUCTURED_GRID:
      return vtkUnstructuredGrid::New();
    case VTK_PIECEWISE_FUNCTION:
      return vtkPiecewiseFunction::New();
    case VTK_IMAGE_DATA:
      return vtkImageData::New();
    case VTK_DATA_OBJECT:
      return vtkDataObject::New();
    case VTK_DATA_SET:
      return nullptr;
    case VTK_POINT_SET:
      return vtkPointSet::New();
    case VTK_UNIFORM_GRID:
      return vtkUniformGrid::New();
    case VTK_COMPOSITE_DATA_SET:
      return nullptr;
    case VTK_MULTIGROUP_DATA_SET:
      return nullptr;
    case VTK_MULTIBLOCK_DATA_SET:
      return vtkMultiBlockDataSet::New();
    case VTK_HIERARCHICAL_DATA_SET:
      return nullptr;
    case VTK_HIERARCHICAL_BOX_DATA_SET:
      // VTK_DEPRECATED_IN_9_5_0
      vtkLogF(
        WARNING, "VTK_HIERARCHICAL_BOX_DATA_SET is deprecated, use VTK_OVERLAPPING_AMR instead");
      return vtkHierarchicalBoxDataSet::New();
    case VTK_GENERIC_DATA_SET:
      return nullptr;
    case VTK_HYPER_OCTREE:
      return nullptr;
    case VTK_TEMPORAL_DATA_SET:
      return nullptr;
    case VTK_TABLE:
      return vtkTable::New();
    case VTK_GRAPH:
      return nullptr;
    case VTK_TREE:
      return vtkTree::New();
    case VTK_SELECTION:
      return vtkSelection::New();
    case VTK_DIRECTED_GRAPH:
      return vtkDirectedGraph::New();
    case VTK_UNDIRECTED_GRAPH:
      return vtkUndirectedGraph::New();
    case VTK_MULTIPIECE_DATA_SET:
      return vtkMultiPieceDataSet::New();
    case VTK_DIRECTED_ACYCLIC_GRAPH:
      return vtkDirectedAcyclicGraph::New();
    case VTK_ARRAY_DATA:
      return vtkArrayData::New();
    case VTK_REEB_GRAPH:
      return vtkReebGraph::New();
    case VTK_UNIFORM_GRID_AMR:
      return vtkUniformGridAMR::New();
    case VTK_NON_OVERLAPPING_AMR:
      return vtkNonOverlappingAMR::New();
    case VTK_OVERLAPPING_AMR:
      return vtkOverlappingAMR::New();
    case VTK_HYPER_TREE_GRID:
      return vtkHyperTreeGrid::New();
    case VTK_MOLECULE:
      return vtkMolecule::New();
    case VTK_PISTON_DATA_OBJECT:
      return nullptr;
    case VTK_PATH:
      return vtkPath::New();
    case VTK_UNSTRUCTURED_GRID_BASE:
      return nullptr;
    case VTK_PARTITIONED_DATA_SET:
      return vtkPartitionedDataSet::New();
    case VTK_PARTITIONED_DATA_SET_COLLECTION:
      return vtkPartitionedDataSetCollection::New();
    case VTK_UNIFORM_HYPER_TREE_GRID:
      return vtkUniformHyperTreeGrid::New();
    case VTK_EXPLICIT_STRUCTURED_GRID:
      return vtkExplicitStructuredGrid::New();
    case VTK_DATA_OBJECT_TREE:
      return nullptr;
    case VTK_ABSTRACT_ELECTRONIC_DATA:
      return nullptr;
    case VTK_OPEN_QUBE_ELECTRONIC_DATA:
      /**
       * we cannot support creating this since its not part of this module
       */
      return nullptr;
    case VTK_ANNOTATION:
      return vtkAnnotation::New();
    case VTK_ANNOTATION_LAYERS:
      return vtkAnnotationLayers::New();
    case VTK_BSP_CUTS:
      return vtkBSPCuts::New();
    case VTK_GEO_JSON_FEATURE:
      /**
       * we cannot support creating this since its not part of this module
       */
      return nullptr;
    case VTK_IMAGE_STENCIL_DATA:
      /**
       * we cannot support creating this since its not part of this module
       */
      return nullptr;
    case VTK_CELL_GRID:
      return vtkCellGrid::New();
    default:
      vtkLogF(WARNING, "Unknown data type '%d'", type);
      return nullptr;
  }
}

//------------------------------------------------------------------------------
vtkDataObject* vtkDataObjectTypes::NewDataObject(const char* classname)
{
  const auto typeId = vtkDataObjectTypes::GetTypeIdFromClassName(classname);
  auto dobj = typeId != -1 ? vtkDataObjectTypes::NewDataObject(typeId) : nullptr;
  if (dobj == nullptr)
  {
    vtkLogF(WARNING, "'NewDataObject' cannot create '%s'.", (classname ? classname : "(nullptr)"));
  }
  return dobj;
}

//------------------------------------------------------------------------------
void vtkDataObjectTypes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkDataObjectTypes::Validate()
{
  for (int i = 0; vtkDataObjectTypesStrings[i] != nullptr; i++)
  {
    const char* cls = vtkDataObjectTypesStrings[i];
    vtkDataObject* obj = vtkDataObjectTypes::NewDataObject(cls);

    if (obj == nullptr)
    {
      continue;
    }

    int type = obj->GetDataObjectType();
    obj->Delete();

    if (strcmp(vtkDataObjectTypesStrings[type], cls) != 0)
    {
      cerr << "ERROR: In " __FILE__ ", line " << __LINE__ << endl;
      cerr << "Type mismatch for: " << cls << endl;
      cerr << "The value looked up in vtkDataObjectTypesStrings using ";
      cerr << "the index returned by GetDataObjectType() does not match the object type." << endl;
      cerr << "Value from vtkDataObjectTypesStrings[obj->GetDataObjectType()]): ";
      cerr << vtkDataObjectTypesStrings[type] << endl;
      cerr << "Check that the correct value is being returned by GetDataObjectType() ";
      cerr << "for this object type. Also check that the values in vtkDataObjectTypesStrings ";
      cerr << "are in the same order as the #define's in vtkType.h.";
      return EXIT_FAILURE;
    }
  }

  if (vtkDataObjectTypes::TypeIdIsA(VTK_DATA_SET, VTK_DATA_OBJECT) &&
    !vtkDataObjectTypes::TypeIdIsA(VTK_DATA_SET, VTK_TABLE) &&
    vtkDataObjectTypes::TypeIdIsA(VTK_PARTITIONED_DATA_SET_COLLECTION, VTK_COMPOSITE_DATA_SET) &&
    vtkDataObjectTypes::TypeIdIsA(VTK_MULTIBLOCK_DATA_SET, VTK_DATA_OBJECT_TREE) &&
    vtkDataObjectTypes::TypeIdIsA(VTK_OVERLAPPING_AMR, VTK_UNIFORM_GRID_AMR) &&
    vtkDataObjectTypes::TypeIdIsA(VTK_UNSTRUCTURED_GRID, VTK_POINT_SET) &&
    vtkDataObjectTypes::TypeIdIsA(VTK_UNSTRUCTURED_GRID, VTK_DATA_SET) &&
    vtkDataObjectTypes::TypeIdIsA(VTK_HIERARCHICAL_BOX_DATA_SET, VTK_UNIFORM_GRID_AMR) &&
    vtkDataObjectTypes::TypeIdIsA(VTK_CELL_GRID, VTK_DATA_OBJECT))
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}

//------------------------------------------------------------------------------
bool vtkDataObjectTypes::TypeIdIsA(int typeId, int targetTypeId)
{
  if (!::IsTypeIdValid(typeId) || !::IsTypeIdValid(targetTypeId))
  {
    return false;
  }

  if (typeId == targetTypeId)
  {
    return true;
  }

  if (vtkDataObjectTypes::GetCommonBaseTypeId(typeId, targetTypeId) == targetTypeId)
  {
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
int vtkDataObjectTypes::GetCommonBaseTypeId(int typeA, int typeB)
{
  if (!::IsTypeIdValid(typeA) && !::IsTypeIdValid(typeB))
  {
    return -1;
  }
  else if (!::IsTypeIdValid(typeA) && ::IsTypeIdValid(typeB))
  {
    return typeB;
  }
  else if (::IsTypeIdValid(typeA) && !::IsTypeIdValid(typeB))
  {
    return typeA;
  }

  auto computeBranch = [](int type)
  {
    // list immediate base-classes, no need to list any that are direct subclasses
    // of vtkDataObject since that's assumed by this point.
    static const std::map<int, int> bases = { { VTK_UNIFORM_HYPER_TREE_GRID, VTK_HYPER_TREE_GRID },
      { VTK_UNDIRECTED_GRAPH, VTK_GRAPH }, { VTK_DIRECTED_GRAPH, VTK_GRAPH },
      { VTK_MOLECULE, VTK_UNDIRECTED_GRAPH }, { VTK_DIRECTED_ACYCLIC_GRAPH, VTK_DIRECTED_GRAPH },
      { VTK_REEB_GRAPH, VTK_DIRECTED_GRAPH }, { VTK_TREE, VTK_DIRECTED_ACYCLIC_GRAPH },
      { VTK_RECTILINEAR_GRID, VTK_DATA_SET }, { VTK_POINT_SET, VTK_DATA_SET },
      { VTK_IMAGE_DATA, VTK_DATA_SET }, { VTK_UNSTRUCTURED_GRID_BASE, VTK_POINT_SET },
      { VTK_STRUCTURED_GRID, VTK_POINT_SET }, { VTK_POLY_DATA, VTK_POINT_SET },
      { VTK_PATH, VTK_POINT_SET }, { VTK_EXPLICIT_STRUCTURED_GRID, VTK_POINT_SET },
      { VTK_UNSTRUCTURED_GRID, VTK_UNSTRUCTURED_GRID_BASE }, { VTK_UNIFORM_GRID, VTK_IMAGE_DATA },
      { VTK_STRUCTURED_POINTS, VTK_IMAGE_DATA }, { VTK_OVERLAPPING_AMR, VTK_UNIFORM_GRID_AMR },
      { VTK_HIERARCHICAL_BOX_DATA_SET, VTK_OVERLAPPING_AMR },
      { VTK_NON_OVERLAPPING_AMR, VTK_UNIFORM_GRID_AMR },
      { VTK_DATA_OBJECT_TREE, VTK_COMPOSITE_DATA_SET },
      { VTK_PARTITIONED_DATA_SET_COLLECTION, VTK_DATA_OBJECT_TREE },
      { VTK_PARTITIONED_DATA_SET, VTK_DATA_OBJECT_TREE },
      { VTK_MULTIPIECE_DATA_SET, VTK_PARTITIONED_DATA_SET },
      { VTK_MULTIBLOCK_DATA_SET, VTK_DATA_OBJECT_TREE },
      { VTK_OPEN_QUBE_ELECTRONIC_DATA, VTK_ABSTRACT_ELECTRONIC_DATA } };

    std::vector<int> branch;
    do
    {
      branch.push_back(type);
      auto iter = bases.find(type);
      type = (iter != bases.end()) ? iter->second : VTK_DATA_OBJECT;
    } while (type != VTK_DATA_OBJECT);
    branch.push_back(VTK_DATA_OBJECT);
    std::reverse(branch.begin(), branch.end());
    return branch;
  };

  std::vector<int> branchA = computeBranch(typeA);
  std::vector<int> branchB = computeBranch(typeB);

  int baseType = VTK_DATA_OBJECT;
  size_t index = 0;
  for (size_t max = std::min(branchA.size(), branchB.size()); index < max; ++index)
  {
    if (branchA[index] == branchB[index])
    {
      baseType = branchA[index];
    }
    else
    {
      break;
    }
  }
  return baseType;
}
VTK_ABI_NAMESPACE_END
