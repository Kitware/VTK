/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGetPolyDataGhostCells.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkGetPolyDataGhostCells

#ifndef __vtkGetPolyDataGhostCells_h
#define __vtkGetPolyDataGhostCells_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkPolyDataCollection.h"

class VTK_EXPORT vtkGetPolyDataGhostCells : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkGetPolyDataGhostCells *New();

  vtkTypeMacro(vtkGetPolyDataGhostCells, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a piece of a dataset from which to get ghost cells.  The first
  // input added is the one that the ghost cells will be added to.
  void AddInput(vtkPolyData *in);

  // Description:
  // Get any input of this filter.
  vtkPolyData *GetInput(int idx);
  vtkPolyData *GetInput() { return this->GetInput( 0 ); };
  
  // Description:
  // Remove a dataset from the list of data to append.
  void RemoveInput(vtkPolyData *in);

  // Description:
  // Returns a copy of the input array.  Modifications to this list
  // will not be reflected in the actual inputs.
  vtkPolyDataCollection *GetInputList();

protected:
  vtkGetPolyDataGhostCells();
  ~vtkGetPolyDataGhostCells();
  vtkGetPolyDataGhostCells(const vtkGetPolyDataGhostCells&) {};
  void operator=(const vtkGetPolyDataGhostCells&) {};

  // Usual data generation method
  void Execute();
  
  void AddGhostLevel(vtkPolyData *output, int ghostLevel,
		     vtkPoints *points, vtkPointLocator **locators,
		     int numInputs, vtkGhostLevels *ghostLevels);
  
  int IsCellInserted(int *pointIds, int numPoints, vtkPolyData *data);
  
  // list pieces of data set from which to get ghost cells.
  // Here as a convenience.  It is a copy of the input array.
  vtkPolyDataCollection *InputList;

 private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkDataSet not a vtkDataObject."); };
  void RemoveInput(vtkDataObject *input)
    { this->vtkProcessObject::RemoveInput(input); };
};


#endif
