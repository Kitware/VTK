/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDicer.h
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
  // Description:
  // Instantiate an object.
  static vtkDicer *New();

  const char *GetClassName() {return "vtkDicer";};
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
  vtkDicer(const vtkDicer&) {};
  void operator=(const vtkDicer&) {};

  virtual void UpdatePieceMeasures();

  int           NumberOfPointsPerPiece;
  int           NumberOfPieces;
  unsigned long MemoryLimit;
  int           NumberOfActualPieces;
  int           FieldData;
  int           DiceMode;

};

#endif


