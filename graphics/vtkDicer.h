/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDicer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkDicer - abstract superclass to divide dataset into pieces
// .SECTION Description
// Subclasses of vtkDicer divides the input dataset into separate
// pieces.  These pieces can then be operated on by other filters
// (e.g., vtkThreshold). One application is to break very large
// polygonal models into pieces and performing viewing and occlusion
// culling on the pieces. Multiple pieces can also be streamed through
// the visualization pipeline.
//
// To use this filter, you must specify the execution mode of the
// filter; i.e., set the way that the piece size is controlled (do
// this by setting the DiceMode ivar). The filter does not change the
// geometry or topology of the input dataset, rather it generates
// integer numbers that indicate which piece a particular point
// belongs to (i.e., it modifies the point and cell attribute
// data). The integer number can be placed into the output scalar
// data, or the output field data.

// .SECTION Caveats
// The number of pieces generated may not equal the specified number
// of pieces. Use the method GetNumberOfActualPieces() after filter
// execution to get the actual number of pieces generated.

// .SECTION See Also
// vtkOBBDicer vtkConnectedDicer vtkSpatialDicer

#ifndef __vtkDicer_h
#define __vtkDicer_h

#include "vtkDataSetToDataSetFilter.h"

#define VTK_DICE_MODE_NUMBER_OF_POINTS 0
#define VTK_DICE_MODE_SPECIFIED_NUMBER 1
#define VTK_DICE_MODE_MEMORY_LIMIT 2

class VTK_EXPORT vtkDicer : public vtkDataSetToDataSetFilter 
{
public:
  vtkTypeMacro(vtkDicer,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the flag which controls whether to generate point scalar
  // data or point field data. If this flag is off, scalar data is
  // generated.  Otherwise, field data is generated. Note that the
  // generated the data are integer numbers indicating which piece a
  // particular point belongs to.
  vtkSetMacro(FieldData,int);
  vtkGetMacro(FieldData,int);
  vtkBooleanMacro(FieldData,int);

  // Description:
  // Specify the method to determine how many pieces the data should be
  // broken into. By default, the number of points per piece is used.
  vtkSetClampMacro(DiceMode,int,VTK_DICE_MODE_NUMBER_OF_POINTS,VTK_DICE_MODE_MEMORY_LIMIT);
  vtkGetMacro(DiceMode,int);
  void SetDiceModeToNumberOfPointsPerPiece()
    {this->SetDiceMode(VTK_DICE_MODE_NUMBER_OF_POINTS);};
  void SetDiceModeToSpecifiedNumberOfPieces()
    {this->SetDiceMode(VTK_DICE_MODE_SPECIFIED_NUMBER);};
  void SetDiceModeToMemoryLimitPerPiece()
    {this->SetDiceMode(VTK_DICE_MODE_MEMORY_LIMIT);};

  // Description:
  // Use the following method after the filter has updated to
  // determine the actual number of pieces the data was separated
  // into.
  vtkGetMacro(NumberOfActualPieces,int);

  // Description:
  // Control piece size based on the maximum number of points per piece.
  // (This ivar has effect only when the DiceMode is set to 
  // SetDiceModeToNumberOfPoints().)
  vtkSetClampMacro(NumberOfPointsPerPiece,int,1000,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfPointsPerPiece,int);

  // Description:
  // Set/Get the number of pieces the object is to be separated into.
  // (This ivar has effect only when the DiceMode is set to
  // SetDiceModeToSpecifiedNumber()). Note that the ivar
  // NumberOfPieces is a target - depending on the particulars of the
  // data, more or less number of pieces than the target value may be
  // created.
  vtkSetClampMacro(NumberOfPieces,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfPieces,int);

  // Description:
  // Control piece size based on a memory limit.  (This ivar has
  // effect only when the DiceMode is set to
  // SetDiceModeToMemoryLimit()). The memory limit should be set in
  // kilobytes.
  vtkSetClampMacro(MemoryLimit,unsigned long,100,VTK_LARGE_INTEGER);
  vtkGetMacro(MemoryLimit,unsigned long);

protected:
  vtkDicer();
  ~vtkDicer() {};
  vtkDicer(const vtkDicer&);
  void operator=(const vtkDicer&);

  virtual void UpdatePieceMeasures();

  int           NumberOfPointsPerPiece;
  int           NumberOfPieces;
  unsigned long MemoryLimit;
  int           NumberOfActualPieces;
  int           FieldData;
  int           DiceMode;

};

#endif


