/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCirclePackToPolyData.h

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
// .NAME vtkCirclePackToPolyData - converts a tree to a polygonal data
// representing a circle packing of the hierarchy.
//
// .SECTION Description
// This algorithm requires that the vtkCirclePackLayout filter has already
// been applied to the data in order to create the triple array
// (Xcenter, Ycenter, Radius) of circle bounds or each vertex of the tree.

#ifndef __vtkCirclePackToPolyData_h
#define __vtkCirclePackToPolyData_h

#include "vtkPolyDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkCirclePackToPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkCirclePackToPolyData *New();

  vtkTypeMacro(vtkCirclePackToPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The field containing triples of the form (Xcenter, Ycenter, Radius).
  //
  // This field may be added to the tree using vtkCirclePackLayout.
  // This array must be set.
  virtual void SetCirclesArrayName(const char* name)
    { this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name); }

  // Description:
  // Define the number of sides used in output circles.
  // Default is 100.
  vtkSetMacro(Resolution, unsigned int);
  vtkGetMacro(Resolution, unsigned int);

  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkCirclePackToPolyData();
  ~vtkCirclePackToPolyData();

  unsigned int Resolution;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkCirclePackToPolyData(const vtkCirclePackToPolyData&);  // Not implemented.
  void operator=(const vtkCirclePackToPolyData&);  // Not implemented.
  void CreateCircle(const double& x,
                    const double& y,
                    const double& z,
                    const double& radius,
                    const int& resolution,
                    vtkPolyData* polyData);
};

#endif
