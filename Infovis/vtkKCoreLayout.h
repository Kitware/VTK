/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKCoreLayout.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkKCoreLayout - Produces a layout for a graph labeled with K-Core
//                        information.
// .SECTION Description
//
// vtkKCoreLayout creates coordinates for each vertex that can be used to
// perform a layout for a k-core view.
// Prerequisite:  Vertices must have an attribute array containing their
//                k-core number.  This layout is based on the algorithm
//                described in the paper: "k-core decomposition: a tool
//                for the visualization of large scale networks", by
//                Ignacio Alvarez-Hamelin et. al.
//
// Input port 0: graph

#ifndef __vtkKCoreLayout_h
#define __vtkKCoreLayout_h

#include "vtkGraphAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkKCoreLayout : public vtkGraphAlgorithm
{
public:
  static vtkKCoreLayout* New();
  vtkTypeMacro(vtkKCoreLayout,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Convenience function provided for setting the graph input.
  void SetGraphConnection(vtkAlgorithmOutput*);

  vtkKCoreLayout();
  ~vtkKCoreLayout();

  int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Set the name of the vertex attribute array storing k-core labels.
  // Default: kcore
  vtkSetStringMacro(KCoreLabelArrayName);

  // Description:
  // Set whether or not to convert output to cartesian coordinates.  If false, coordinates
  // will be returned in polar coordinates (radius, angle).
  // Default: False
  vtkGetMacro( Cartesian, bool );
  vtkSetMacro( Cartesian, bool );
  vtkBooleanMacro( Cartesian, bool );

  // Description:
  // Polar coordinates array name for radius values.
  // This is only used if OutputCartesianCoordinates is False.
  // Default: coord_radius
  vtkSetStringMacro(PolarCoordsRadiusArrayName);
  vtkGetStringMacro(PolarCoordsRadiusArrayName);

  // Description:
  // Polar coordinates array name for angle values in radians.
  // This is only used if OutputCartesianCoordinates is False.
  // Default: coord_angle
  vtkSetStringMacro(PolarCoordsAngleArrayName);
  vtkGetStringMacro(PolarCoordsAngleArrayName);

  // Description:
  // Cartesian coordinates array name for the X coordinates.
  // This is only used if OutputCartesianCoordinates is True.
  // Default: coord_x
  vtkSetStringMacro(CartesianCoordsXArrayName);
  vtkGetStringMacro(CartesianCoordsXArrayName);

  // Description:
  // Cartesian coordinates array name for the Y coordinates.
  // This is only used if OutputCartesianCoordinates is True.
  // Default: coord_y
  vtkSetStringMacro(CartesianCoordsYArrayName);
  vtkGetStringMacro(CartesianCoordsYArrayName);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

protected:
  char * KCoreLabelArrayName;

  char * PolarCoordsRadiusArrayName;
  char * PolarCoordsAngleArrayName;

  char * CartesianCoordsXArrayName;
  char * CartesianCoordsYArrayName;

  bool Cartesian;

private:
  vtkKCoreLayout(const vtkKCoreLayout&);   // Not implemented
  void operator=(const vtkKCoreLayout&);   // Not implemented
};

#endif
