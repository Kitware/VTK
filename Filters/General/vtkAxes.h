/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAxes - create an x-y-z axes
// .SECTION Description
// vtkAxes creates three lines that form an x-y-z axes. The origin of the
// axes is user specified (0,0,0 is default), and the size is specified with
// a scale factor. Three scalar values are generated for the three lines and
// can be used (via color map) to indicate a particular coordinate axis.

#ifndef __vtkAxes_h
#define __vtkAxes_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkAxes : public vtkPolyDataAlgorithm
{
public:
  static vtkAxes *New();

  vtkTypeMacro(vtkAxes,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the origin of the axes.
  vtkSetVector3Macro(Origin,double);
  vtkGetVectorMacro(Origin,double,3);

  // Description:
  // Set the scale factor of the axes. Used to control size.
  vtkSetMacro(ScaleFactor,double);
  vtkGetMacro(ScaleFactor,double);

  // Description:
  // If Symetric is on, the the axis continue to negative values.
  vtkSetMacro(Symmetric,int);
  vtkGetMacro(Symmetric,int);
  vtkBooleanMacro(Symmetric,int);

  // Description:
  // Option for computing normals.  By default they are computed.
  vtkSetMacro(ComputeNormals, int);
  vtkGetMacro(ComputeNormals, int);
  vtkBooleanMacro(ComputeNormals, int);

protected:
  vtkAxes();
  ~vtkAxes() {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  // This source does not know how to generate pieces yet.
  int ComputeDivisionExtents(vtkDataObject *output,
                             int idx, int numDivisions);

  double Origin[3];
  double ScaleFactor;

  int Symmetric;
  int ComputeNormals;
private:
  vtkAxes(const vtkAxes&);  // Not implemented.
  void operator=(const vtkAxes&);  // Not implemented.
};

#endif


