/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeshQuality.h
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
// .NAME vtkMeshQuality - calculate quality of tetrahedral meshes
// .SECTION Description
// vtkMeshQuality will calculate the normalized quality ratio of the cells
// in a tetrahedral mesh according to the equation:
// <p> ratio = (radius of circumscribed sphere)/(radius of inscribed sphere)/3.
// <p> The minumum (and ideal) quality ratio is 1.0 for regular tetrahedra,
// i.e. all sides of equal length.  Larger values indicate poorer mesh
// quality.  The resulting quality values (and the tetrahedron volumes)
// are set as the Scalars of the FieldData of the output.  This class was
// developed by Leila Baghdadi at the John P. Robarts Research Institute.

#ifndef __vtkMeshQuality_h
#define __vtkMeshQuality_h

#include "vtkDataSetToDataObjectFilter.h"

class VTK_GRAPHICS_EXPORT vtkMeshQuality : public vtkDataSetToDataObjectFilter
{
public:
  static vtkMeshQuality *New();  
  vtkTypeRevisionMacro(vtkMeshQuality,vtkDataSetToDataObjectFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on/off the calculation of volume for each cell (Default: On).
  // The volume the first component of the cell scalars in the output
  // data.
  vtkSetMacro(Volume,int);
  vtkGetMacro(Volume,int);
  vtkBooleanMacro(Volume,int);

  // Description:
  // Turn on/off the calculation of the quality ratio for each cell
  // (Default: On).  The ratio is 1 for a regular tetrahedron and
  // greater than one for other tetrahedrons.  The values are stored
  // in the second component of the cell scalars in the output data,
  // unless Volume calculation is off in which case the values are
  // stored in the first scalar component.
  vtkSetMacro(Ratio,int);
  vtkGetMacro(Ratio,int);
  vtkBooleanMacro(Ratio,int);
 

protected:
  vtkMeshQuality();
  ~vtkMeshQuality();
  void Execute();
  double Insphere(double p1[3], double p2[3], double p3[3],
                  double p4[3], double center[3]);

  int Volume;
  int Ratio;


private:
  vtkMeshQuality(const vtkMeshQuality&);  // Not implemented.
  void operator=(const vtkMeshQuality&);  // Not implemented.

};

#endif

