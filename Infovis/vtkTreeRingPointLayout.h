/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingPointLayout.h

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
// .NAME vtkTreeRingPointLayout - layout a vtkTree into a tree map
//
// .SECTION Description
// vtkTreeRingPointLayout assigns point values for laying out the hierarchical 
// edges of a hierarchical tree ring view.  This class requires that the 
// sector angles be given for each point prior to the algorithm run.
//
// .SECTION Thanks
// Thanks to Jason Shepherd from Sandia National Laboratories
// for developing this class.

#ifndef __vtkTreeRingPointLayout_h
#define __vtkTreeRingPointLayout_h

#include "vtkTreeAlgorithm.h"

class vtkTreeRingPointLayoutStrategy;

class VTK_INFOVIS_EXPORT vtkTreeRingPointLayout : public vtkTreeAlgorithm 
{
public:
  static vtkTreeRingPointLayout *New();

  vtkTypeRevisionMacro(vtkTreeRingPointLayout,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The field name to use for storing the sector for each vertex.
  // The rectangles are stored in a quadruple float array 
  // (innerRadius, outerRadius, startAngle, endAngle).
  vtkGetStringMacro(SectorsFieldName);
  vtkSetStringMacro(SectorsFieldName);

  // Description:
  // Define the tree ring's exterior radius (to be used to align
  //  hierarchical edges with existing tree ring sectors.)
  vtkSetMacro(ExteriorRadius, double);
  vtkGetMacro(ExteriorRadius, double);

  // Description:
  // The spacing of tree levels. Levels near zero give more space
  // to levels near the leaves, while levels near one (the default)
  // create evenly-spaced levels. Levels above one give more space
  // to levels near the root.
  vtkSetMacro(LogSpacingValue, double);
  vtkGetMacro(LogSpacingValue, double);

protected:
  vtkTreeRingPointLayout();
  ~vtkTreeRingPointLayout();

  char* SectorsFieldName;
  double ExteriorRadius;
  double LogSpacingValue;
  
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
private:
  vtkTreeRingPointLayout(const vtkTreeRingPointLayout&);  // Not implemented.
  void operator=(const vtkTreeRingPointLayout&);  // Not implemented.
};

#endif
