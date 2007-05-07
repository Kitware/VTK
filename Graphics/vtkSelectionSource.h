/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectionSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelectionSource - Generate selection from given set of ids
// vtkSelectionSource generates a vtkSelection from a set of 
// (piece id, cell id) pairs. It will only generate the selection values
// that match UPDATE_PIECE_NUMBER (i.e. piece == UPDATE_PIECE_NUMBER).

#ifndef __vtkSelectionSource_h
#define __vtkSelectionSource_h

#include "vtkSelectionAlgorithm.h"

//BTX
class vtkSelectionSourceInternals;
//ETX

class VTK_GRAPHICS_EXPORT vtkSelectionSource : public vtkSelectionAlgorithm
{
public:
  static vtkSelectionSource *New();
  vtkTypeRevisionMacro(vtkSelectionSource,vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Add a (piece, id) to the selection set. The source will generate
  // only the ids for which piece == UPDATE_PIECE_NUMBER.
  // If piece == -1, the id applies to all pieces.
  void AddID(vtkIdType piece, vtkIdType id);

  // Description:
  // Add a point in world space to probe at.
  void AddLocation(double x, double y, double z);

  // Description:
  // Removes all IDs.
  void RemoveAllIDs();
  void RemoveAllValues();


  
  // Description:
  // Set the content type for the generated selection.
  // Possible values are as defined by 
  // vtkSelection::SelectionContent.
  void SetContentType(int);
  vtkGetMacro(ContentType, int);

  // Description:
  // Set the field type for the generated selection.
  // Possible values are as defined by
  // vtkSelection::SelectionField.
  vtkSetMacro(FieldType, int);
  vtkGetMacro(FieldType, int);

protected:
  vtkSelectionSource();
  ~vtkSelectionSource();

  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  vtkSelectionSourceInternals* Internal;

  int ContentType;
  int FieldType;

private:
  vtkSelectionSource(const vtkSelectionSource&);  // Not implemented.
  void operator=(const vtkSelectionSource&);  // Not implemented.
};

#endif
