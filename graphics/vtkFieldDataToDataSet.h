/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFieldDataToDataSet.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkFieldDataToDataSet - map field data to concrete dataset
// .SECTION Description
// vtkFieldDataToDataSet is an class that maps a data object (i.e., a field) into 
// a concrete dataset, i.e., gives structure to the field by defining a geometry
//  and topology, as well as defining dataset attribute data such as scalars, 
// vectors, tensors, etc.

// .SECTION See Also
// vtkDataObject vtkFieldData vtkDataSet vtkStructuredPoints vtkStructuredGrid
// vtkDataSetAttributes vtkScalars vtkDataArray

#ifndef __vtkFieldDataToDataSet_h
#define __vtkFieldDataToDataSet_h

#include "vtkSource.h"
#include "vtkFieldData.h"

class vtkPolyData;
class vtkStructuredPoints;
class vtkStructuredGrid;
class vtkRectilinearGrid;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkFieldDataToDataSet : public vtkSource
{
public:

// Description:
// Instantiate object with no input and no defined output.
  vtkFieldDataToDataSet();

  ~vtkFieldDataToDataSet();
  static vtkFieldDataToDataSet *New() {return new vtkFieldDataToDataSet;};
  const char *GetClassName() {return "vtkFieldDataToDataSet";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // All filters must provide a method to update the visualization 
  // pipeline. (Method interface inherited from vtkSource.)
  void Update();

  // Description:
  // Set the input to the filter.
  virtual void SetInput(vtkDataObject *input);
  void SetInput(vtkDataObject &input) {this->SetInput(&input);};
  vtkDataObject *GetInput() {return this->Input;};

  // get the output in different forms - does run-time checking
  vtkPolyData *GetPolyDataOutput();
  vtkStructuredPoints *GetStructuredPointsOutput();
  vtkStructuredGrid *GetStructuredGridOutput();
  vtkUnstructuredGrid *GetUnstructuredGridOutput();
  vtkRectilinearGrid *GetRectilinearGridOutput();

protected:
  vtkDataObject *Input;
  char Updating;

  // objects used to support the retrieval of output after mapping
  vtkPolyData *PolyData;
  vtkStructuredPoints *StructuredPoints;
  vtkStructuredGrid *StructuredGrid;
  vtkUnstructuredGrid *UnstructuredGrid;
  vtkRectilinearGrid *RectilinearGrid;
  
};

#endif


