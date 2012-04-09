/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTableRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkQtTableRepresentation - set up a vtkTable in a Qt model
//
// .SECTION Description
//
// This class is a wrapper around vtkQtTableModelAdapter.  It
// performs the following functions:
//
// <ul>
// <li>Keep track of the key column, first data column, and last data column.
//     Populate the appropriate ivars on the Qt adapter.
// <li>Assign colors to each of the data series using a vtkLookupTable.
//     A default lookup table is provided or the user can supply one
//     using SetColorTable().
// </ul>
//
// The user must supply the following items:
// <ul>
// <li>the name of the column that contains the series names,
// <li>the names of the first and last data columns
//     (this range should not contain the key column), and
// <li>(optionally) a vtkLookupTable to use when assigning colors.
// </ul>
//
// .SECTION Caveats
//
// Call SetInputConnection with a table connection
// BEFORE the representation is added to a view or strange things
// may happen, including segfaults.

#ifndef __vtkQtTableRepresentation_h
#define __vtkQtTableRepresentation_h

#include "QVTKWin32Header.h"
#include "vtkDataRepresentation.h"

class vtkDoubleArray;
class vtkLookupTable;
class vtkQtTableModelAdapter;

// ----------------------------------------------------------------------

class QVTK_EXPORT vtkQtTableRepresentation : public vtkDataRepresentation
{
public:
  vtkTypeMacro(vtkQtTableRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set/get the lookup table that will be used to determine colors
  // for each series.  The table's range should be [0, 1).
  void SetColorTable(vtkLookupTable *t);
  vtkGetObjectMacro(ColorTable, vtkLookupTable);

  // Description:
  // Set/get the name of the column that contains series names.  This
  // must be called BEFORE the representation is added to a view.
  void SetKeyColumn(const char* col);
  char* GetKeyColumn();

  // Description:
  // Set/get the name of the first data column.  This must be called
  // BEFORE the representation is added to a view.
  vtkSetStringMacro(FirstDataColumn);
  vtkGetStringMacro(FirstDataColumn);

  // Description:
  // Set/get the name of the last data column.  This must be called
  // BEFORE the representation is added to a view.
  vtkSetStringMacro(LastDataColumn);
  vtkGetStringMacro(LastDataColumn);

 protected:
  vtkQtTableRepresentation();
  ~vtkQtTableRepresentation();

  // Description:
  // Update the table representation
  void UpdateTable();

  vtkSetStringMacro(KeyColumnInternal);
  vtkGetStringMacro(KeyColumnInternal);

  // ----------------------------------------------------------------------
  vtkQtTableModelAdapter *ModelAdapter;
  vtkLookupTable *ColorTable;
  vtkDoubleArray *SeriesColors;
  char *KeyColumnInternal;
  char *FirstDataColumn;
  char *LastDataColumn;

  // Description:
  // Prepare the input connections to this representation.
  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual void ResetModel();
  virtual void CreateSeriesColors();

  // Description:
  // This should set the model type to DATA, METADATA or FULL
  // depending on what you want.
  virtual void SetModelType() { };

private:
  vtkQtTableRepresentation(const vtkQtTableRepresentation &);
  void operator=(const vtkQtTableRepresentation &);

};

#endif
