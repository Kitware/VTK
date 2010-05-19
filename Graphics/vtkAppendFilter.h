/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAppendFilter - appends one or more datasets together into a single unstructured grid
// .SECTION Description
// vtkAppendFilter is a filter that appends one of more datasets into a single
// unstructured grid. All geometry is extracted and appended, but point 
// attributes (i.e., scalars, vectors, normals, field data, etc.) are extracted 
// and appended only if all datasets have the point attributes available. 
// (For example, if one dataset has scalars but another does not, scalars will 
// not be appended.)

// .SECTION See Also
// vtkAppendPolyData

#ifndef __vtkAppendFilter_h
#define __vtkAppendFilter_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkDataSetCollection;

class VTK_GRAPHICS_EXPORT vtkAppendFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkAppendFilter *New();

  vtkTypeMacro(vtkAppendFilter,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get any input of this filter.
//BTX
  vtkDataSet *GetInput(int idx);
  vtkDataSet *GetInput() 
    {return this->GetInput( 0 );}
//ETX

  // Description:
  // Remove a dataset from the list of data to append.
  void RemoveInput(vtkDataSet *in);

  // Description:
  // Returns a copy of the input array.  Modifications to this list
  // will not be reflected in the actual inputs.
  vtkDataSetCollection *GetInputList();

protected:
  vtkAppendFilter();
  ~vtkAppendFilter();

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  virtual int FillInputPortInformation(int port, vtkInformation *info);
  
  // Description:
  // This function appends multiple blocks / pieces into a vtkUnstructuredGrid
  // data by using a point locator to merge duplicate points (when ghost cell
  // information is not available from the input data blocks / pieces).
  // This function should be called by RequestData() only.
  int     AppendBlocksWithPointLocator( vtkInformationVector ** inputVector,
                                        vtkInformationVector  * outputVector );
  

  // list of data sets to append together.
  // Here as a convenience.  It is a copy of the input array.
  vtkDataSetCollection *InputList;

private:
  vtkAppendFilter(const vtkAppendFilter&);  // Not implemented.
  void operator=(const vtkAppendFilter&);  // Not implemented.
};


#endif


