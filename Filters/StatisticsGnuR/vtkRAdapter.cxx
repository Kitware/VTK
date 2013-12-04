
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkObjectFactory.h"
#include "vtkRAdapter.h"
#include "vtkAbstractArray.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTree.h"
#include "vtkArray.h"
#include "vtkArrayExtents.h"
#include "vtkArrayCoordinates.h"
#include "vtkTypedArray.h"
#include "vtkVariantArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkDataArrayCollection.h"
#include "vtkArrayData.h"
#include "vtkDataObjectCollection.h"
#include "vtkTreeDFSIterator.h"
#include "vtkSmartPointer.h"
#include "vtkNew.h"
#include "vtkEdgeListIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkMutableDirectedGraph.h"

#include <map>

#include <stdio.h>
#include <cassert>

#define R_NO_REMAP /* AVOID SOME SERIOUS STUPIDITY. DO NOT REMOVE. */

#include "R.h"
#include "Rdefines.h"
#include "R_ext/Parse.h"
#include "R_ext/Rdynload.h"


vtkStandardNewMacro(vtkRAdapter);

namespace
{

int R_FindArrayIndex(vtkArrayCoordinates& coordinates, const vtkArrayExtents& extents)
{

  vtkIdType i;
  int ret = 0;
  vtkIdType divisor = 1;
  vtkIdType d = coordinates.GetDimensions();

  for(i = 0; i < d; ++i)
    {
    ret = ret + coordinates[i]*divisor;
    divisor *= extents[i].GetSize();
    }

  return(ret);

}

} // End anonymous namespace

//----------------------------------------------------------------------------
vtkRAdapter::vtkRAdapter()
{

  this->vad =  vtkArrayData::New();
  this->vdoc = vtkDataObjectCollection::New();
  this->vdac = vtkDataArrayCollection::New();

}

//----------------------------------------------------------------------------
vtkRAdapter::~vtkRAdapter()
{

  if(this->vad)
    {
    this->vad->Delete();
    }

  if(this->vdoc)
    {
    this->vdoc->Delete();
    }

  if(this->vdac)
    {
    this->vdac->Delete();
    }

}

vtkDataArray* vtkRAdapter::RToVTKDataArray(SEXP variable)
{

  int i;
  int j;
  int nr;
  int nc;
  vtkDoubleArray * result;
  double * data;


  if( Rf_isMatrix(variable) || Rf_isVector(variable) )
    {
    nc = Rf_ncols(variable);
    nr = Rf_nrows(variable);

    result = vtkDoubleArray::New();

    result->SetNumberOfTuples(nr);
    result->SetNumberOfComponents(nc);

    data = new double[nc];

    for(i=0;i<nr;i++)
      {
      for(j=0;j<nc;j++)
        {
        if ( isReal(variable) )
          {
          data[j] = REAL(variable)[j*nr + i];
          }
        else if ( isInteger(variable) )
          {
          data[j] = static_cast<double>(INTEGER(variable)[j*nr + i]);
          }
        else
          {
          vtkErrorMacro(<< "Bad return variable, tried REAL and INTEGER.");
          }
        result->InsertTuple(i,data);
        }
      }

    delete [] data;
    this->vdac->AddItem(result);
    result->Delete();
    return(result);
    }
  else
    {
    return(0);
    }

}


SEXP vtkRAdapter::VTKDataArrayToR(vtkDataArray* da)
{

  SEXP a;
  int nr;
  int nc;
  int i;
  int j;
  double* data;

  nr = da->GetNumberOfTuples();
  nc = da->GetNumberOfComponents();

  PROTECT(a = Rf_allocMatrix(REALSXP,nr,nc));

  for(i=0;i<nr;i++)
    {
    for(j=0;j<nc;j++)
      {
      data = da->GetTuple(i);
      REAL(a)[j*nr + i] = data[j];
      }
    }

  return(a);

}

vtkArray* vtkRAdapter::RToVTKArray(SEXP variable)
{

  vtkArray::SizeT i;
  vtkArray::DimensionT j;
  vtkArray::DimensionT ndim;
  SEXP dims;
  vtkArrayExtents extents;
  vtkTypedArray<double>* da;
  da = vtkTypedArray<double>::SafeDownCast(vtkArray::CreateArray(vtkArray::DENSE, VTK_DOUBLE));

  dims = getAttrib(variable, R_DimSymbol);
  ndim = static_cast<vtkArray::DimensionT>(length(dims));

  if (!isMatrix(variable)&&!isArray(variable)&&isVector(variable))
    {
    ndim = 1;
    }

  extents.SetDimensions(ndim);

  if (isMatrix(variable)||isArray(variable))
    {
    for(j=0;j<ndim;j++)
      {
      extents[j] = vtkArrayRange(0,INTEGER(dims)[j]);
      }
    }
  else
    {
    extents[0] = vtkArrayRange(0,length(variable));
    }

  da->Resize(extents);

  vtkArrayCoordinates index;

  index.SetDimensions(ndim);

  for(i=0;i<da->GetSize();i++)
    {
    da->GetCoordinatesN(i,index);
    if ( isReal(variable) )
      {
      da->SetVariantValue(index,REAL(variable)[i]);
      }
    else if ( isInteger(variable) )
      {
      da->SetVariantValue(index,static_cast<double>(INTEGER(variable)[i]));
      }
    else
      {
      vtkErrorMacro(<< "Bad return variable, tried REAL and INTEGER.");
      }
    }

  this->vad->AddArray(da);
  da->Delete();
  return(da);

}

SEXP vtkRAdapter::VTKArrayToR(vtkArray* da)
{

  SEXP a;
  SEXP dim;
  vtkArray::SizeT i;
  vtkArray::DimensionT j;
  vtkArrayCoordinates coords;

  PROTECT(dim = Rf_allocVector(INTSXP, da->GetDimensions()));

  assert(da->GetExtents().ZeroBased());
  for(j=0;j<da->GetDimensions();j++)
    {
    INTEGER(dim)[j] = da->GetExtents()[j].GetSize();
    }

  PROTECT(a = Rf_allocArray(REALSXP, dim));

  for(i=0;i<da->GetSize();i++)
    {
    REAL(a)[i] = 0.0;
    }

  assert(da->GetExtents().ZeroBased());
  for(i=0;i<da->GetNonNullSize();i++)
    {
    da->GetCoordinatesN(i,coords);
    REAL(a)[R_FindArrayIndex(coords,da->GetExtents())] = da->GetVariantValue(coords).ToDouble();
    }

  UNPROTECT(1);

  return(a);

}

SEXP vtkRAdapter::VTKTableToR(vtkTable* table)
{

  SEXP a;
  SEXP b;
  SEXP names;
  int i;
  int j;
  int nr = table->GetNumberOfRows();
  int nc = table->GetNumberOfColumns();
  vtkVariant data;

  PROTECT(a = allocVector(VECSXP, nc));
  PROTECT(names = allocVector(STRSXP, nc));

  for(j=0;j<nc;j++)
    {
    SET_STRING_ELT(names,j,mkChar(table->GetColumn(j)->GetName()));
    if(vtkDataArray::SafeDownCast(table->GetColumn(j)))
      {
      PROTECT(b = allocVector(REALSXP,nr));
      SET_VECTOR_ELT(a,j,b);
      for(i=0;i<nr;i++)
        {
        data = table->GetValue(i,j);
        REAL(b)[i] = data.ToDouble();
        }
      }
    else
      {
      PROTECT(b = allocVector(STRSXP,nr));
      SET_VECTOR_ELT(a,j,b);
      for(i=0;i<nr;i++)
        {
        data = table->GetValue(i,j);
        SET_STRING_ELT(b,i,mkChar(data.ToString().c_str()));
        }
      }
    }

  setAttrib(a,R_NamesSymbol,names);

  return(a);

}

vtkTable* vtkRAdapter::RToVTKTable(SEXP variable)
{

  int i;
  int j;
  int nr;
  int nc;
  SEXP names;
  vtkVariant v;
  vtkTable* result;

  if( isMatrix(variable) )
    {
    nc = Rf_ncols(variable);
    nr = Rf_nrows(variable);
    result = vtkTable::New();

    for(j=0;j<nc;j++)
      {
      vtkDoubleArray* da = vtkDoubleArray::New();
      da->SetNumberOfComponents(1);
      names = getAttrib(variable, R_DimNamesSymbol);
      if(!isNull(names))
        {
        da->SetName(CHAR(STRING_ELT(VECTOR_ELT(names,1),j)));
        }
      else
        {
        v = j;
        da->SetName(v.ToString().c_str());
        }
      for(i=0;i<nr;i++)
        {
        da->InsertNextValue(REAL(variable)[j*nr + i]);
        }
      result->AddColumn(da);
      da->Delete();
      }
    }
  else if( isNewList(variable) )
    {
    nc = length(variable);
    nr = length(VECTOR_ELT(variable,0));
    for(j=1;j<nc;j++)
      {
      if(isReal(VECTOR_ELT(variable,j)) ||
         isInteger(VECTOR_ELT(variable,j)) ||
         isString(VECTOR_ELT(variable,j)) )
        {
        if(length(VECTOR_ELT(variable,j)) != nr)
          {
          vtkGenericWarningMacro(<<"Cannot convert R data type to vtkTable");
          return(0);
          }
        }
      else
        {
        vtkGenericWarningMacro(<<"Cannot convert R data type to vtkTable");
        return(0);
        }
      }

    result = vtkTable::New();
    names = getAttrib(variable, R_NamesSymbol);
    vtkAbstractArray *aa;
    for(j=0;j<nc;j++)
      {
      if(isReal(VECTOR_ELT(variable,j)))
        {
        vtkDoubleArray* da = vtkDoubleArray::New();
        da->SetNumberOfComponents(1);
        aa = da;
        for(i=0;i<nr;i++)
          {
          da->InsertNextValue(REAL(VECTOR_ELT(variable,j))[i]);
          }
        }
      else if(isInteger(VECTOR_ELT(variable,j)))
        {
        vtkIntArray* da = vtkIntArray::New();
        da->SetNumberOfComponents(1);
        aa = da;
        for(i=0;i<nr;i++)
          {
          da->InsertNextValue(INTEGER(VECTOR_ELT(variable,j))[i]);
          }
        }
      else
        {
        vtkStringArray* da = vtkStringArray::New();
        da->SetNumberOfComponents(1);
        aa = da;
        for(i=0;i<nr;i++)
          {
          da->InsertNextValue(CHAR(STRING_ELT(VECTOR_ELT(variable,j),i)));
          }
        }

      if(!isNull(names))
        {
        aa->SetName(CHAR(STRING_ELT(names,j)));
        }
      else
        {
        v = j;
        aa->SetName(v.ToString().c_str());
        }
      result->AddColumn(aa);
      aa->Delete();
      }
    }
  else
    {
    vtkGenericWarningMacro(<<"Cannot convert R data type to vtkTable");
    return(0);
    }

  this->vdoc->AddItem(result);
  result->Delete();
  return(result);

}

SEXP vtkRAdapter::VTKTreeToR(vtkTree* tree)
{
  SEXP r_tree;
  SEXP names;
  SEXP edge;
  SEXP edge_length;
  SEXP Nnode;
  SEXP node_label;
  SEXP tip_label;
  SEXP classname;


  int nedge, nnode, ntip;

  // R phylo tree is a list of 5 elements
  PROTECT(r_tree = allocVector(VECSXP, 5));
  PROTECT(names = allocVector(STRSXP, 5));

  // traverse the tree to reorder the leaf vertices according to the
  // phylo tree numbering rule;
  // newNodeId is the checkup table that maps a vertexId(starting from 0)
  // to its corresponding R tree point id (starting from 1)
  vtkIdType leafCount = 0;
  vtkTreeDFSIterator* iter = vtkTreeDFSIterator::New();
  iter->SetTree(tree);
  int nVerts = tree->GetNumberOfVertices();
  int *newNodeId = new int[nVerts];//including root vertex 0
  while (iter->HasNext())
    {// find out all the leaf nodes, and number them sequentially
    vtkIdType vertexId = iter->Next();
    newNodeId[vertexId] = 0;//initialize
    if (tree->IsLeaf(vertexId))
      {
      leafCount++;
      newNodeId[vertexId] = leafCount;
      }
    }

  // second tree traverse to reorder the node vertices
  int nodeId = leafCount;
  iter->Restart();
  vtkIdType vertexId;
  while (iter->HasNext())
    {
    vertexId = iter->Next();
    if (!tree->IsLeaf(vertexId))
      {
      nodeId++;
      newNodeId[vertexId] = nodeId;
      }
    }

  nedge = tree->GetNumberOfEdges();
  ntip  = leafCount;
  nnode = nedge - ntip + 1;

  // fill in R variables
  PROTECT(edge = allocMatrix(INTSXP, nedge,2));
  PROTECT(Nnode = allocVector(INTSXP, 1));
  PROTECT(tip_label = allocVector(STRSXP, ntip));
  PROTECT(edge_length = allocVector(REALSXP, nedge));
  PROTECT(node_label = allocVector(STRSXP, nnode));
  INTEGER(Nnode)[0] = nnode;

  int * e = INTEGER(edge);
  double * e_len = REAL(edge_length);

  // fill in e and e_len
  vtkSmartPointer<vtkEdgeListIterator> edgeIterator = vtkSmartPointer<vtkEdgeListIterator>::New();
  tree->GetEdges(edgeIterator);
  vtkEdgeType vEdge;
  int i = 0;
  vtkDoubleArray * weights = vtkDoubleArray::SafeDownCast((tree->GetEdgeData())->GetArray("weight"));
  while(edgeIterator->HasNext())
    {
    vEdge = edgeIterator->Next();
    e[i]  = newNodeId[vEdge.Source] ;
    e[i + nedge] = newNodeId[vEdge.Target];

    int eNum = tree->GetEdgeId(vEdge.Source, vEdge.Target);
    e_len[i] = weights->GetValue(eNum);
    i++;
    }

  // fill in  Nnode , tip_label and  node_label
  // use GetAbstractArray() instead of GetArray()
  vtkStringArray * labels = vtkStringArray::SafeDownCast((tree->GetVertexData())->GetAbstractArray("node name"));
  iter->Restart();
  while (iter->HasNext())
    {// find out all the leaf nodes, and number them sequentially
    vertexId = iter->Next();
    if (tree->IsLeaf(vertexId))
      {
      vtkStdString lab = labels->GetValue(vertexId);
      SET_STRING_ELT(tip_label, newNodeId[vertexId]-1, mkChar(lab.c_str()));
      }
    else
      {
      vtkStdString lab = labels->GetValue(vertexId);
      SET_STRING_ELT(node_label,newNodeId[vertexId]- ntip - 1, mkChar(lab.c_str())); //the starting id of the internal nodes is (ntip + 1)
      }
    }
  iter->Delete();

  // set all elements
  SET_VECTOR_ELT(r_tree, 0, edge);
  SET_VECTOR_ELT(r_tree, 1, Nnode);
  SET_VECTOR_ELT(r_tree, 2, tip_label);
  SET_VECTOR_ELT(r_tree, 3, edge_length);
  SET_VECTOR_ELT(r_tree, 4, node_label);

  SET_STRING_ELT(names, 0, mkChar("edge"));
  SET_STRING_ELT(names, 1, mkChar("Nnode"));
  SET_STRING_ELT(names, 2, mkChar("tip.label"));
  SET_STRING_ELT(names, 3, mkChar("edge.length"));
  SET_STRING_ELT(names, 4, mkChar("node.label"));

  setAttrib(r_tree,R_NamesSymbol,names);

  PROTECT(classname = allocVector(STRSXP, 1));
  SET_STRING_ELT(classname, 0, mkChar("phylo"));
  setAttrib(r_tree, R_ClassSymbol, classname);

  delete [] newNodeId;

  UNPROTECT(8);
  return r_tree;
}



vtkTree* vtkRAdapter::RToVTKTree(SEXP variable)
{
  int nedge, nnode, ntip;
  vtkTree * tree = vtkTree::New();

  if (isNewList(variable))
    {
    int nELT = length(variable);
    if (nELT < 4)
      {
      vtkErrorMacro(<<"RToVTKTree():R tree list does not contain required four elements!");
      return NULL;
      }

    //1) edge
    int * edge;
    SEXP r_edge = VECTOR_ELT(variable,0);
    if (isInteger(r_edge))
      {
      edge = INTEGER(r_edge);
      nedge = length(r_edge)/2;
      }
    else
      {
      vtkErrorMacro(<<"RToVTKTree(): \"edge\" array is not integer type. ");
      return NULL;
      }


    //2)Nnode
    SEXP r_nnode = VECTOR_ELT(variable, 1);
    if (isInteger(r_nnode))
      {
      nnode =  INTEGER(r_nnode)[0];
      if (length(r_nnode) != 1)
        {
        vtkErrorMacro(<<"RToVTKTree(): Expect a single scalar of \"Nnode\". ");
        return NULL;
        }
      }
    else
      {
      vtkErrorMacro(<<"RToVTKTree(): \"Nnode\" is not integer type. ");
      return NULL;
      }

    //3) tip.label
    SEXP r_tip_label = VECTOR_ELT(variable, 2);
    ntip = nedge - nnode + 1;
    vtkNew<vtkStringArray> tip_label;
    tip_label->SetNumberOfValues(ntip);
    if (isString(r_tip_label))
      {
      if (length(r_tip_label) != ntip)
        {
        vtkErrorMacro(<<"RToVTKTree(): \"node_label\"'s size does not match up with \"nnode\".");
        return NULL;
        }
      for (int i = 0; i < ntip; i++)
        {
        const char * a = CHAR(STRING_ELT(r_tip_label,i));
        tip_label->SetValue(i,a);
        }
      }

    //4) edge.length
    SEXP r_edge_length = VECTOR_ELT(variable, 3);
    double * edge_length;
    if (isReal(r_edge_length))
      {
      edge_length = REAL(r_edge_length);
      if (nedge != length(r_edge_length))
        {
        vtkErrorMacro(<<"RToVTKTree():  \"edge_length\"'s size does not match up with \"nedge\".");
        return NULL;
        }
      }

    // 5) node.label (optional)
    vtkNew<vtkStringArray> node_label;
    node_label->SetNumberOfValues(nnode);
    if ( nELT  == 5) //node labels provided by the r tree
      {
      SEXP r_node_label = VECTOR_ELT(variable, 4);

      if (isString(r_node_label))
        {
        if (length(r_node_label) != nnode)
          {
          vtkErrorMacro(<<"RToVTKTree(): \"node_label\"'s size does not match up with \"nnode\". ");
          return NULL;
          }
        for (int i = 0; i < nnode; i++)
          {
          node_label->SetValue(i,CHAR(STRING_ELT(r_node_label,i)));
          }
        }
      }
    else
      {
      for (int i = 0; i < nnode; i++)
        {
        node_label->SetValue(i,"");
        }
    }

    //------------  Build the VTKTree -----------
    vtkNew<vtkMutableDirectedGraph> builder;


    // Create all of the tree vertice (number of edges +1)
    // number of edges = nedge(in R tree)
    int numOfEdges = nedge;
    for(int i = 0; i <= numOfEdges; i++)
      {
      builder->AddVertex();
      }

    for(int i = 0; i < nedge; i++)
      {
      // -1 because R vertices begin with 1, whereas VTK vertices begin with 0.
      vtkIdType source = edge[i] - 1;
      vtkIdType target = edge[i+nedge] - 1;
      builder->AddEdge(source, target);
      }

    // Create the edge weight array
    vtkNew<vtkDoubleArray> weights;
    weights->SetNumberOfComponents(1);
    weights->SetName("weight");
    weights->SetNumberOfValues(numOfEdges);
    for (int i = 0; i < nedge; i++)
      {
      weights->SetValue(i, edge_length[i]);
      }
    builder->GetEdgeData()->AddArray(weights.GetPointer());

    // Create the names array
    // In R tree, the numeric id of the vertice is ordered such that the tips are listed first
    // followed by the internal nodes. The order are matching up with the label arrays (tip_label and node_label).
    vtkNew<vtkStringArray> names;
    names->SetNumberOfComponents(1);
    names->SetName("node name");
    names->SetNumberOfValues(ntip + nnode);
    for (int i = 0; i < ntip; i++)
      {
      names->SetValue(i, tip_label->GetValue(i));
      }
    for (int i = 0; i < nnode; i++)
      {
      names->SetValue(i + ntip, node_label->GetValue(i));
      }
    builder->GetVertexData()->AddArray(names.GetPointer());

    if (!tree->CheckedShallowCopy(builder.GetPointer()))
      {
      vtkErrorMacro(<<"Edges do not create a valid tree.");
      return NULL;
      }

    // Create the "node weight" array for the Vertices, in order to use
    // vtkTreeLayoutStrategy for visualizing the tree using vtkTreeHeatmapItem
    vtkNew<vtkDoubleArray> nodeWeights;
    nodeWeights->SetNumberOfTuples(tree->GetNumberOfVertices());

    vtkNew<vtkTreeDFSIterator> treeIterator;
    treeIterator->SetStartVertex(tree->GetRoot());
    treeIterator->SetTree(tree);
    while (treeIterator->HasNext())
      {
      vtkIdType vertex = treeIterator->Next();
      vtkIdType parent = tree->GetParent(vertex);
      double weight = 0.0;
      if (parent >= 0)
        {
        weight = weights->GetValue(tree->GetEdgeId(parent, vertex));
        weight += nodeWeights->GetValue(parent);
        }
      nodeWeights->SetValue(vertex, weight);
      }

    nodeWeights->SetName("node weight");
    tree->GetVertexData()->AddArray(nodeWeights.GetPointer());

    this->vdoc->AddItem(tree);
    tree->Delete();
    return tree;
    }
  else
    {
    vtkErrorMacro(<<"RToVTKTree(): R variable is not a list. ");
    return NULL;
    }
}


void vtkRAdapter::PrintSelf(ostream& os, vtkIndent indent)
{

  this->Superclass::PrintSelf(os,indent);

  if(this->vad)
    {
    this->vad->PrintSelf(os,indent);
    }

  if(this->vdoc)
    {
    this->vdoc->PrintSelf(os,indent);
    }

  if(this->vdac)
    {
    this->vdac->PrintSelf(os,indent);
    }


}

