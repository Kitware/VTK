/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractEdges.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractEdges - extract cell edges from any type of data
// .SECTION Description
// vtkExtractEdges is a filter to extract edges from a dataset. Edges
// are extracted as lines or polylines.

// .SECTION See Also
// vtkFeatureEdges

#ifndef __vtkExtractEdges_h
#define __vtkExtractEdges_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkExtractEdges : public vtkDataSetToPolyDataFilter
{
public:
  static vtkExtractEdges *New();
  vtkTypeRevisionMacro(vtkExtractEdges,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get a spatial locator for merging points. By
  // default an instance of vtkMergePoints is used.
  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

  // Description:
  // Return MTime also considering the locator.
  unsigned long GetMTime();

protected:
  vtkExtractEdges();
  ~vtkExtractEdges();

  // Usual data generation method
  void Execute();

  vtkPointLocator *Locator;
private:
  vtkExtractEdges(const vtkExtractEdges&);  // Not implemented.
  void operator=(const vtkExtractEdges&);  // Not implemented.
};

#endif


