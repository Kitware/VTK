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

#include  "vtkPolyData.h"
#include  "vtkStructuredPoints.h"
#include  "vtkStructuredGrid.h"
#include  "vtkRectilinearGrid.h"
#include  "vtkUnstructuredGrid.h"
#include  "vtkPiecewiseFunction.h"
#include  "vtkImageData.h"
#include  "vtkDataObject.h"
#include  "vtkDataSet.h"
#include  "vtkPointSet.h"
#include  "vtkUniformGrid.h"
#include  "vtkCompositeDataSet.h"
#include  "vtkMultiGroupDataSet.h"
#include  "vtkMultiBlockDataSet.h"
#include  "vtkHierarchicalDataSet.h"
#include  "vtkHierarchicalBoxDataSet.h"
#include  "vtkGenericDataSet.h"
#include  "vtkHyperOctree.h"
#include  "vtkTemporalDataSet.h"
#include  "vtkTable.h"
#include  "vtkGraph.h"
#include  "vtkTree.h"
#include  "vtkSelection.h"

vtkCxxRevisionMacro(vtkDataObjectTypes, "1.3");
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
  "vtkMultiGroupDataSet", 
  "vtkMultiBlockDataSet", 
  "vtkHierarchicalDataSet",
  "vtkHierarchicalBoxDataSet", 
  "vtkGenericDataSet", 
  "vtkHyperOctree", 
  "vtkTemporalDataSet", 
  "vtkTable", 
  "vtkGraph", 
  "vtkTree", 
  "vtkSelection",
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
  else if(strcmp(type, "vtkMultiGroupDataSet") == 0)
    {
    return vtkMultiGroupDataSet::New();
    }
  else if(strcmp(type, "vtkMultiBlockDataSet") == 0)
    {
    return vtkMultiBlockDataSet::New();
    }
  else if(strcmp(type, "vtkHierarchicalDataSet") == 0)
    {
    return vtkHierarchicalDataSet::New();
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
  else if(strcmp(type, "vtkGraph") == 0)
    {
    return vtkGraph::New();
    }
  else if(strcmp(type, "vtkTree") == 0)
    {
    return vtkTree::New();
    }
  else if(strcmp(type, "vtkSelection") == 0)
    {
    return vtkSelection::New();
    }
  else if(vtkObject* obj = vtkInstantiator::CreateInstance(type))
    {
    vtkDataObject* data = vtkDataObject::SafeDownCast(obj);
    if(!data)
      {
      obj->Delete();
      }
    return data;
    }

  return 0;
}
  
//----------------------------------------------------------------------------
void vtkDataObjectTypes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
