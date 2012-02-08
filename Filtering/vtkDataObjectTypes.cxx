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

#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"

#include  "vtkAnnotation.h"
#include  "vtkAnnotationLayers.h"
#include  "vtkCompositeDataSet.h"
#include  "vtkDataObject.h"
#include  "vtkDataSet.h"
#include  "vtkDirectedAcyclicGraph.h"
#include  "vtkDirectedGraph.h"
#include  "vtkGenericDataSet.h"
#include  "vtkGraph.h"
#include  "vtkHierarchicalBoxDataSet.h"
#include  "vtkHyperOctree.h"
#include  "vtkImageData.h"
#include  "vtkMultiBlockDataSet.h"
#include  "vtkMultiPieceDataSet.h"
#include  "vtkPiecewiseFunction.h"
#include  "vtkPointSet.h"
#include  "vtkPolyData.h"
#include  "vtkRectilinearGrid.h"
#include  "vtkReebGraph.h"
#include  "vtkSelection.h"
#include  "vtkStructuredGrid.h"
#include  "vtkStructuredPoints.h"
#include  "vtkTable.h"
#include  "vtkTemporalDataSet.h"
#include  "vtkTree.h"
#include  "vtkUndirectedGraph.h"
#include  "vtkUniformGrid.h"
#include  "vtkUnstructuredGrid.h"

#ifdef VTK_USE_N_WAY_ARRAYS
#include  "vtkArrayData.h"
#endif

#ifdef VTK_USE_CHEMISTRY
#include  "vtkMolecule.h"
#endif

vtkStandardNewMacro(vtkDataObjectTypes);

// This list should contain the data object class names in
// the same order as the #define's in vtkType.h. Make sure
// this list is NULL terminated.
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
  "vtkHierarchicalDataSet", // OBSOLETE
  "vtkHierarchicalBoxDataSet", 
  "vtkGenericDataSet", 
  "vtkHyperOctree", 
  "vtkTemporalDataSet", 
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
#ifdef VTK_USE_CHEMISTRY
  "vtkMolecule",
#endif
  NULL
};

//----------------------------------------------------------------------------
const char* vtkDataObjectTypes::GetClassNameFromTypeId(int type)
{
  static int numClasses = 0;
  
  // find length of table
  if (numClasses == 0)
    {
    while (vtkDataObjectTypesStrings[numClasses] != NULL)
      {
      numClasses++;
      }
    }

  if (type < numClasses)
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

  for(int idx=0; vtkDataObjectTypesStrings[idx] != NULL; idx++)
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
  
  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDataObjectTypes::NewDataObject(const char* type)
{
  
  if (!type)
    {
    vtkGenericWarningMacro("NewDataObject(): You are trying to instantiate DataObjectType \"" << type 
               << "\" which does not exist.");
    return 0;
    }

  // Check for some standard types and then try the instantiator.
  if(strcmp(type, "vtkImageData") == 0)
    {
    return vtkImageData::New();
    }
  else if(strcmp(type, "vtkDataObject") == 0)
    {
    return vtkDataObject::New();
    }
  else if(strcmp(type, "vtkPolyData") == 0)
    {
    return vtkPolyData::New();
    }
  else if(strcmp(type, "vtkRectilinearGrid") == 0)
    {
    return vtkRectilinearGrid::New();
    }
  else if(strcmp(type, "vtkStructuredGrid") == 0)
    {
    return vtkStructuredGrid::New();
    }
  else if(strcmp(type, "vtkStructuredPoints") == 0)
    {
    return vtkStructuredPoints::New();
    }
  else if(strcmp(type, "vtkUnstructuredGrid") == 0)
    {
    return vtkUnstructuredGrid::New();
    }
  else if(strcmp(type, "vtkUniformGrid") == 0)
    {
    return vtkUniformGrid::New();
    }
  else if(strcmp(type, "vtkMultiBlockDataSet") == 0)
    {
    return vtkMultiBlockDataSet::New();
    }
  else if(strcmp(type, "vtkHierarchicalBoxDataSet") == 0)
    {
    return vtkHierarchicalBoxDataSet::New();
    }
  else if(strcmp(type, "vtkHyperOctree") == 0)
    {
    return vtkHyperOctree::New();
    }
  else if(strcmp(type, "vtkTemporalDataSet") == 0)
    {
    return vtkTemporalDataSet::New();
    }
  else if(strcmp(type, "vtkTable") == 0)
    {
    return vtkTable::New();
    }
  else if(strcmp(type, "vtkTree") == 0)
    {
    return vtkTree::New();
    }
  else if(strcmp(type, "vtkSelection") == 0)
    {
    return vtkSelection::New();
    }
  else if(strcmp(type, "vtkDirectedGraph") == 0)
    {
    return vtkDirectedGraph::New();
    }
  else if(strcmp(type, "vtkUndirectedGraph") == 0)
    {
    return vtkUndirectedGraph::New();
    }
  else if(strcmp(type, "vtkMultiPieceDataSet") == 0)
    {
    return vtkMultiPieceDataSet::New();
    }
  else if(strcmp(type, "vtkDirectedAcyclicGraph") == 0)
    {
    return vtkDirectedAcyclicGraph::New();
    }
  else if(strcmp(type, "vtkAnnotation") == 0)
    {
    return vtkAnnotation::New();
    }
  else if(strcmp(type, "vtkAnnotationLayers") == 0)
    {
    return vtkAnnotationLayers::New();
    }
  else if(strcmp(type, "vtkReebGraph") == 0)
    {
    return vtkReebGraph::New();
    }
#ifdef VTK_USE_CHEMISTRY
  else if(strcmp(type, "vtkMolecule") == 0)
    {
    return vtkMolecule::New();
    }
#endif
#ifdef VTK_USE_N_WAY_ARRAYS
  else if(strcmp(type, "vtkArrayData") == 0)
    {
    return vtkArrayData::New();
    }
#endif
  else if(vtkObject* obj = vtkInstantiator::CreateInstance(type))
    {
    vtkDataObject* data = vtkDataObject::SafeDownCast(obj);
    if(!data)
      {
      obj->Delete();
      }

    if(data == NULL)
      {
      vtkGenericWarningMacro("NewDataObject(): You are trying to instantiate DataObjectType \"" << type 
                 << "\" which does not exist.");
      }
    return data;
    }

  vtkGenericWarningMacro("NewDataObject(): You are trying to instantiate DataObjectType \"" << type 
             << "\" which does not exist.");

  return 0;
}
  
//----------------------------------------------------------------------------
void vtkDataObjectTypes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
