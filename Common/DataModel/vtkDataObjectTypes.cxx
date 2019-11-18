/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkDataObjectTypes.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObjectTypes.h"

#include "vtkObjectFactory.h"

#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
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
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNonOverlappingAMR.h"
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

#include "vtkArrayData.h"
#include "vtkMolecule.h"

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
  nullptr,
};

//----------------------------------------------------------------------------
const char* vtkDataObjectTypes::GetClassNameFromTypeId(int type)
{
  static int numClasses = 0;

  // find length of table
  if (numClasses == 0)
  {
    while (vtkDataObjectTypesStrings[numClasses] != nullptr)
    {
      numClasses++;
    }
  }

  if (type >= 0 && type < numClasses)
  {
    return vtkDataObjectTypesStrings[type];
  }
  else
  {
    return "UnknownClass";
  }
}

//----------------------------------------------------------------------------
int vtkDataObjectTypes::GetTypeIdFromClassName(const char* classname)
{
  if (!classname)
  {
    return -1;
  }

  for (int idx = 0; vtkDataObjectTypesStrings[idx] != nullptr; idx++)
  {
    if (strcmp(vtkDataObjectTypesStrings[idx], classname) == 0)
    {
      return idx;
    }
  }

  return -1;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDataObjectTypes::NewDataObject(int type)
{
  const char* className = vtkDataObjectTypes::GetClassNameFromTypeId(type);
  if (strcmp(className, "UnknownClass") != 0)
  {
    return vtkDataObjectTypes::NewDataObject(className);
  }

  return nullptr;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDataObjectTypes::NewDataObject(const char* type)
{

  if (!type)
  {
    vtkGenericWarningMacro("NewDataObject(): You are trying to instantiate DataObjectType \""
      << type << "\" which does not exist.");
    return nullptr;
  }

  // Check for some standard types.
  if (strcmp(type, "vtkImageData") == 0)
  {
    return vtkImageData::New();
  }
  else if (strcmp(type, "vtkDataObject") == 0)
  {
    return vtkDataObject::New();
  }
  else if (strcmp(type, "vtkPolyData") == 0)
  {
    return vtkPolyData::New();
  }
  else if (strcmp(type, "vtkRectilinearGrid") == 0)
  {
    return vtkRectilinearGrid::New();
  }
  else if (strcmp(type, "vtkStructuredGrid") == 0)
  {
    return vtkStructuredGrid::New();
  }
  else if (strcmp(type, "vtkStructuredPoints") == 0)
  {
    return vtkStructuredPoints::New();
  }
  else if (strcmp(type, "vtkUnstructuredGrid") == 0)
  {
    return vtkUnstructuredGrid::New();
  }
  else if (strcmp(type, "vtkUniformGrid") == 0)
  {
    return vtkUniformGrid::New();
  }
  else if (strcmp(type, "vtkMultiBlockDataSet") == 0)
  {
    return vtkMultiBlockDataSet::New();
  }
  else if (strcmp(type, "vtkHierarchicalBoxDataSet") == 0)
  {
    return vtkHierarchicalBoxDataSet::New();
  }
  else if (strcmp(type, "vtkOverlappingAMR") == 0)
  {
    return vtkOverlappingAMR::New();
  }
  else if (strcmp(type, "vtkNonOverlappingAMR") == 0)
  {
    return vtkNonOverlappingAMR::New();
  }
  else if (strcmp(type, "vtkHyperTreeGrid") == 0)
  {
    return vtkHyperTreeGrid::New();
  }
  else if (strcmp(type, "vtkUniformHyperTreeGrid") == 0)
  {
    return vtkUniformHyperTreeGrid::New();
  }
  else if (strcmp(type, "vtkTable") == 0)
  {
    return vtkTable::New();
  }
  else if (strcmp(type, "vtkTree") == 0)
  {
    return vtkTree::New();
  }
  else if (strcmp(type, "vtkSelection") == 0)
  {
    return vtkSelection::New();
  }
  else if (strcmp(type, "vtkDirectedGraph") == 0)
  {
    return vtkDirectedGraph::New();
  }
  else if (strcmp(type, "vtkUndirectedGraph") == 0)
  {
    return vtkUndirectedGraph::New();
  }
  else if (strcmp(type, "vtkMultiPieceDataSet") == 0)
  {
    return vtkMultiPieceDataSet::New();
  }
  else if (strcmp(type, "vtkDirectedAcyclicGraph") == 0)
  {
    return vtkDirectedAcyclicGraph::New();
  }
  else if (strcmp(type, "vtkAnnotation") == 0)
  {
    return vtkAnnotation::New();
  }
  else if (strcmp(type, "vtkAnnotationLayers") == 0)
  {
    return vtkAnnotationLayers::New();
  }
  else if (strcmp(type, "vtkReebGraph") == 0)
  {
    return vtkReebGraph::New();
  }
  else if (strcmp(type, "vtkMolecule") == 0)
  {
    return vtkMolecule::New();
  }
  else if (strcmp(type, "vtkArrayData") == 0)
  {
    return vtkArrayData::New();
  }
  else if (strcmp(type, "vtkPath") == 0)
  {
    return vtkPath::New();
  }
  else if (strcmp(type, "vtkPartitionedDataSet") == 0)
  {
    return vtkPartitionedDataSet::New();
  }
  else if (strcmp(type, "vtkPartitionedDataSetCollection") == 0)
  {
    return vtkPartitionedDataSetCollection::New();
  }
  else if (strcmp(type, "vtkExplicitStructuredGrid") == 0)
  {
    return vtkExplicitStructuredGrid::New();
  }

  vtkGenericWarningMacro("NewDataObject(): You are trying to instantiate DataObjectType \""
    << type << "\" which does not exist.");

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkDataObjectTypes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkDataObjectTypes::Validate()
{
  int rc = 0;

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
      rc = 1;
      break;
    }
  }
  return rc;
}
