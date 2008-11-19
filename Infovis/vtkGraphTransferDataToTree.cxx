/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphTransferDataToTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkGraphTransferDataToTree.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkEdgeListIterator.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTree.h"
#include "vtkVariantArray.h"
#include "vtkIntArray.h"

#include <vtksys/stl/map>
using vtksys_stl::map;

//---------------------------------------------------------------------------
template <typename T>
vtkVariant vtkGetValue(T* arr, vtkIdType index)
{
  return vtkVariant(arr[index]);
}

//---------------------------------------------------------------------------
static vtkVariant vtkGetVariantValue(vtkAbstractArray* arr, vtkIdType i)
{
  vtkVariant val;
  switch(arr->GetDataType())
    {
    vtkExtraExtendedTemplateMacro(val = vtkGetValue(
      static_cast<VTK_TT*>(arr->GetVoidPointer(0)), i));
    }
  return val;
}


vtkCxxRevisionMacro(vtkGraphTransferDataToTree, "1.3");
vtkStandardNewMacro(vtkGraphTransferDataToTree);

vtkGraphTransferDataToTree::vtkGraphTransferDataToTree()
{
  this->SetNumberOfInputPorts(2);
  this->DirectMapping = false;
  this->DefaultValue = 1;
  this->SourceArrayName = 0;
  this->TargetArrayName = 0;
}

vtkGraphTransferDataToTree::~vtkGraphTransferDataToTree()
{
  this->SetSourceArrayName(0);
  this->SetTargetArrayName(0);
}

int vtkGraphTransferDataToTree::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
    return 1;
    }
  return 0;
}

int vtkGraphTransferDataToTree::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *graphInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *treeInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkGraph *graph = vtkGraph::SafeDownCast(
    graphInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree *tree = vtkTree::SafeDownCast(
    treeInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree *output = vtkTree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

    // Copy the input into the output
  output->ShallowCopy(tree);

  // If graph or tree is empty, we're done.
  if (graph->GetNumberOfVertices() == 0 ||
      tree->GetNumberOfVertices() == 0)
    {
    return 1;
    }

  if( this->SourceArrayName == 0 || this->TargetArrayName == 0 )
  {
    vtkErrorMacro( "Must specify source and target array names for the transfer." );
    return 0;
  }
  
  // Create a map from graph indices to tree indices
  // If we are using DirectMapping this is trivial
  // we just create an identity map
  map<vtkIdType, vtkIdType> graphIndexToTreeIndex;
  if (this->DirectMapping)
    {
    if (graph->GetNumberOfVertices() > tree->GetNumberOfVertices())
      {
      vtkErrorMacro("Cannot have more graph vertices than tree vertices using direct mapping.");
      return 0;
      }
    // Create identity map.
    for (vtkIdType gv = 0; gv < graph->GetNumberOfVertices(); gv++)
      {
      graphIndexToTreeIndex[gv] = gv;
      }
    }
    
  // Okay if we do not have direct mapping then we need
  // to do some templated madness to go from an arbitrary
  // type to a nice vtkIdType to vtkIdType mapping
  if (!this->DirectMapping)
    {
    // Check for valid pedigree id arrays.
    vtkAbstractArray* graphIdArray = 
      graph->GetVertexData()->GetPedigreeIds();
    if (!graphIdArray)
      {
      vtkErrorMacro("Graph pedigree id array not found.");
      return 0;
      }
    vtkAbstractArray* treeIdArray = 
      tree->GetVertexData()->GetPedigreeIds();
    if (!treeIdArray)
      {
      vtkErrorMacro("Tree pedigree id array not found.");
      return 0;
      }

    map<vtkVariant,vtkIdType,vtkVariantLessThan> graphIdMap;
    
    // Create a map from graph id to graph index
    for (int i=0; i<graph->GetNumberOfVertices(); ++i)
      {
      graphIdMap[vtkGetVariantValue(graphIdArray,i)] = i;
      }
      
    // Now create the map from graph index to tree index
    for (int i=0; i<tree->GetNumberOfVertices(); ++i)
      {
      vtkVariant id = vtkGetVariantValue(treeIdArray,i);
      if (graphIdMap.count(id))
        {
        graphIndexToTreeIndex[graphIdMap[id]] = i;
        }
      }
    }
  
  int i;
  vtkAbstractArray *sourceArray = graph->GetVertexData()->GetAbstractArray( this->SourceArrayName );
  vtkAbstractArray *targetArray = vtkAbstractArray::CreateArray(sourceArray->GetDataType());
  targetArray->SetName( this->TargetArrayName );

  targetArray->SetNumberOfComponents(sourceArray->GetNumberOfComponents());
  targetArray->SetNumberOfTuples(output->GetNumberOfVertices());

  for( i = 0; i < output->GetNumberOfVertices(); i++)
  {
    targetArray->InsertVariantValue(i, this->DefaultValue);
  }
  
  for( i = 0; i < graph->GetNumberOfVertices(); i++)
  {
    targetArray->SetTuple(graphIndexToTreeIndex[i], i, sourceArray);
  }

  output->GetVertexData()->AddArray(targetArray);
  targetArray->Delete();

  return 1;
}

vtkVariant vtkGraphTransferDataToTree::GetDefaultValue()
{
  return this->DefaultValue;
}

void vtkGraphTransferDataToTree::SetDefaultValue(vtkVariant value)
{
  this->DefaultValue = value;
}

void vtkGraphTransferDataToTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DirectMapping: " << this->DirectMapping << endl;
  os << indent << "DefaultValue: " << this->DefaultValue.ToString() << endl;
  os << indent << "SourceArrayName: " << (this->SourceArrayName ? this->SourceArrayName : "(none)") << endl;
  os << indent << "TargetArrayName: " << (this->TargetArrayName ? this->TargetArrayName : "(none)") << endl;
}

