/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPWarpVector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPWarpVector - multithreaded vtkWarpVector
// .SECTION Description
// Just like parent, but uses the SMP framework to do the work on many threads.

#ifndef vtkSMPWarpVector_h__
#define vtkSMPWarpVector_h__

#include "vtkFiltersSMPModule.h" // For export macro
#include "vtkWarpVector.h"

class vtkInformation;
class vtkInformationVector;

class VTKFILTERSSMP_EXPORT vtkSMPWarpVector : public vtkWarpVector
{
public :
  vtkTypeMacro(vtkSMPWarpVector,vtkWarpVector);
  static vtkSMPWarpVector *New();
  void PrintSelf(ostream& os, vtkIndent indent);

protected :
  vtkSMPWarpVector();
  ~vtkSMPWarpVector();


  // Description:
  // Overridden to use threads.
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *);

private :
  vtkSMPWarpVector(const vtkSMPWarpVector&);  // Not implemented.
  void operator=(const vtkSMPWarpVector&);  // Not implemented.

};

#endif //vtkSMPWarpVector_h__
