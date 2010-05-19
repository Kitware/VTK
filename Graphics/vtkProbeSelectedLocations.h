/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeSelectedLocations.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProbeSelectedLocations - similar to vtkExtractSelectedLocations
// except that it interpolates the point attributes at the probe locations.
// .SECTION Description
// vtkProbeSelectedLocations is similar to vtkExtractSelectedLocations except
// that it interpolates the point attributes at the probe location. This is
// equivalent to the vtkProbeFilter except that the probe locations are provided
// by a vtkSelection. The FieldType of the input vtkSelection is immaterial and
// is ignored. The ContentType of the input vtkSelection must be
// vtkSelection::LOCATIONS.

#ifndef __vtkProbeSelectedLocations_h
#define __vtkProbeSelectedLocations_h

#include "vtkExtractSelectionBase.h"

class VTK_GRAPHICS_EXPORT vtkProbeSelectedLocations : public vtkExtractSelectionBase
{
public:
  static vtkProbeSelectedLocations* New();
  vtkTypeMacro(vtkProbeSelectedLocations, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkProbeSelectedLocations();
  ~vtkProbeSelectedLocations();

  // Description:
  // Sets up empty output dataset
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  int RequestData(vtkInformation *, vtkInformationVector **, 
    vtkInformationVector *);

private:
  vtkProbeSelectedLocations(const vtkProbeSelectedLocations&); // Not implemented.
  void operator=(const vtkProbeSelectedLocations&); // Not implemented.
//ETX
};

#endif


