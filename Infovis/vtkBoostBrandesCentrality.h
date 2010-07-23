/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostBrandesCentrality.h

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
// .NAME vtkBoostBrandesCentrality - Compute Brandes betweenness centrality
// on a vtkGraph

// .SECTION Description

// This vtk class uses the Boost brandes_betweeness_centrality
// generic algorithm to compute betweenness centrality on
// the input graph (a vtkGraph).

// .SECTION See Also
// vtkGraph vtkBoostGraphAdapter

#ifndef __vtkBoostBrandesCentrality_h
#define __vtkBoostBrandesCentrality_h

#include "vtkVariant.h" // For variant type

#include "vtkGraphAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkBoostBrandesCentrality : public vtkGraphAlgorithm
{
public:
  static vtkBoostBrandesCentrality *New();
  vtkTypeMacro(vtkBoostBrandesCentrality, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the flag that sets the rule whether or not to use the
  // edge weight array as set using \c SetEdgeWeightArrayName.
  vtkSetMacro(UseEdgeWeightArray, bool);
  vtkBooleanMacro(UseEdgeWeightArray, bool);

  vtkSetMacro(InvertEdgeWeightArray, bool);
  vtkBooleanMacro(InvertEdgeWeightArray, bool);

  // Description:
  // Get/Set the name of the array that needs to be used as the edge weight.
  // The array should be a vtkDataArray.
  vtkGetStringMacro(EdgeWeightArrayName);
  vtkSetStringMacro(EdgeWeightArrayName);

protected:
  vtkBoostBrandesCentrality();
  ~vtkBoostBrandesCentrality();

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);


private:

  bool        UseEdgeWeightArray;
  bool        InvertEdgeWeightArray;
  char*       EdgeWeightArrayName;

  vtkBoostBrandesCentrality(const vtkBoostBrandesCentrality&);  // Not implemented.
  void operator=(const vtkBoostBrandesCentrality&);  // Not implemented.
};

#endif
