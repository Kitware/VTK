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
// .NAME vtkDicer - divide dataset into spatially aggregated pieces
// .SECTION Description
// vtkDicer separates the cells of a dataset into spatially aggregated 
// pieces. These pieces can then be operated on by other filters (e.g.,
// vtkThreshold). One application is to break very large polygonal models
// into pieces and performing viewing and occlusion culling on the pieces.
//
// To use this filter, you must specify the number of points per piece. The
// filter attempts to create groups of points (containing the number of cells
// specified). The groups are created to minimize the size of the bounding 
// box of the group. The filter indicates which group the points belong in
// by creating a scalar value corresponding to group number. Use the method
// GetNumberOfPieces() to determine how many pieces were found.

// .SECTION Caveats
// The number of cells per group will not always be less than the requested
// value. The groups are not guaranteed to generate the minimal bounding 
// boxes.

// .SECTION See Also
// vtkThreshold

#ifndef __vtkDicer_h
#define __vtkDicer_h

#include "vtkDataSetToDataSetFilter.h"
#include "vtkOBBTree.h"

class VTK_EXPORT vtkDicer : public vtkDataSetToDataSetFilter 
{
public:
  vtkDicer();
  static vtkDicer *New() {return new vtkDicer;};
  const char *GetClassName() {return "vtkDicer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the number of cells per group (i.e., piece).
  vtkSetMacro(NumberOfPointsPerPiece,int);
  vtkGetMacro(NumberOfPointsPerPiece,int);

  // Description:
  // Get the number of pieces the object was broken into. The return value
  // is updated when the filter executes.
  vtkGetMacro(NumberOfPieces,int);

protected:
  // Usual data generation method
  void Execute();

  int NumberOfPointsPerPiece;
  short NumberOfPieces;

  //implementation ivars and methods
  void BuildTree(vtkIdList *ptIds, vtkOBBNode *OBBptr);
  void MarkPoints(vtkOBBNode *OBBptr, vtkShortScalars *groupIds);
  void DeleteTree(vtkOBBNode *OBBptr);
  vtkFloatPoints *PointsList;
  
};

#endif


