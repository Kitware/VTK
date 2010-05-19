/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBranchExtentTranslator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBranchExtentTranslator - Uses alternative source for whole extent.
// .SECTION Description
// vtkBranchExtentTranslator is like extent translator, but it uses an 
// alternative source as a whole extent. The whole extent passed is assumed 
// to be a subextent of the original source.  we simply take the intersection 
// of the split extent and the whole extdent passed in.  We are attempting to
// make branching pipelines request consistent extents with the same piece 
// requests.  

// .SECTION Caveats
// This object is still under development.

#ifndef __vtkBranchExtentTranslator_h
#define __vtkBranchExtentTranslator_h

#include "vtkExtentTranslator.h"

class vtkImageData;

class VTK_PARALLEL_EXPORT vtkBranchExtentTranslator : public vtkExtentTranslator
{
public:
  static vtkBranchExtentTranslator *New();

  vtkTypeMacro(vtkBranchExtentTranslator,vtkExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the original upstream image source.
  virtual void SetOriginalSource(vtkImageData*);
  vtkGetObjectMacro(OriginalSource,vtkImageData);

  // Description:
  // Generates the extent from the pieces.
  int PieceToExtent();

  // Description:
  // This unstructured extent/piece is store here for the users convenience.
  // It is not used internally.  The intent was to let an "assignment" be made
  // when the translator/first source is created.  The translator/assignment
  // can be used for any new filter that uses the original source as output.
  // Branches will then have the same assignment.
  vtkSetMacro(AssignedPiece, int);
  vtkGetMacro(AssignedPiece, int);
  vtkSetMacro(AssignedNumberOfPieces, int);
  vtkGetMacro(AssignedNumberOfPieces, int);

protected:
  vtkBranchExtentTranslator();
  ~vtkBranchExtentTranslator();

  vtkImageData *OriginalSource;
  int AssignedPiece;
  int AssignedNumberOfPieces;
private:
  vtkBranchExtentTranslator(const vtkBranchExtentTranslator&);  // Not implemented.
  void operator=(const vtkBranchExtentTranslator&);  // Not implemented.
};

#endif

