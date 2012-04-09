/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpTo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWarpTo - deform geometry by warping towards a point
// .SECTION Description
// vtkWarpTo is a filter that modifies point coordinates by moving the
// points towards a user specified position.

#ifndef __vtkWarpTo_h
#define __vtkWarpTo_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkWarpTo : public vtkPointSetAlgorithm
{
public:
  static vtkWarpTo *New();
  vtkTypeMacro(vtkWarpTo,vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the value to scale displacement.
  vtkSetMacro(ScaleFactor,double);
  vtkGetMacro(ScaleFactor,double);

  // Description:
  // Set/Get the position to warp towards.
  vtkGetVectorMacro(Position,double,3);
  vtkSetVector3Macro(Position,double);

  // Description:
  // Set/Get the Absolute ivar. Turning Absolute on causes scale factor
  // of the new position to be one unit away from Position.
  vtkSetMacro(Absolute,int);
  vtkGetMacro(Absolute,int);
  vtkBooleanMacro(Absolute,int);

  int FillInputPortInformation(int port, vtkInformation *info);

protected:
  vtkWarpTo();
  ~vtkWarpTo() {};

  int RequestDataObject(vtkInformation *request,
                        vtkInformationVector **inputVector,
                        vtkInformationVector *outputVector);
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *);
  double ScaleFactor;
  double Position[3];
  int   Absolute;
private:
  vtkWarpTo(const vtkWarpTo&);  // Not implemented.
  void operator=(const vtkWarpTo&);  // Not implemented.
};

#endif
