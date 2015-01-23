/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArraySelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataArraySelection - Store on/off settings for data arrays for a vtkSource.
// .SECTION Description
// vtkDataArraySelection can be used by vtkSource subclasses to store
// on/off settings for whether each vtkDataArray in its input should
// be passed in the source's output.  This is primarily intended to
// allow file readers to configure what data arrays are read from the
// file.

#ifndef vtkDataArraySelection_h
#define vtkDataArraySelection_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkDataArraySelectionInternals;

class VTKCOMMONCORE_EXPORT vtkDataArraySelection : public vtkObject
{
public:
  vtkTypeMacro(vtkDataArraySelection,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkDataArraySelection* New();

  // Description:
  // Enable the array with the given name.  Creates a new entry if
  // none exists.
  void EnableArray(const char* name);

  // Description:
  // Disable the array with the given name.  Creates a new entry if
  // none exists.
  void DisableArray(const char* name);

  // Description:
  // Return whether the array with the given name is enabled.  If
  // there is no entry, the array is assumed to be disabled.
  int ArrayIsEnabled(const char* name);

  // Description:
  // Return whether the array with the given name exists.
  int ArrayExists(const char* name);

  // Description:
  // Enable all arrays that currently have an entry.
  void EnableAllArrays();

  // Description:
  // Disable all arrays that currently have an entry.
  void DisableAllArrays();

  // Description:
  // Get the number of arrays that currently have an entry.
  int GetNumberOfArrays();

  // Description:
  // Get the number of arrays that are enabled.
  int GetNumberOfArraysEnabled();

  // Description:
  // Get the name of the array entry at the given index.
  const char* GetArrayName(int index);

  // Description:
  // Get an index of the array with the given name.
  int GetArrayIndex(const char *name);

  // Description:
  // Get the index of an array with the given name among those
  // that are enabled.  Returns -1 if the array is not enabled.
  int GetEnabledArrayIndex(const char* name);

  // Description:
  // Get whether the array at the given index is enabled.
  int GetArraySetting(const char* name)
    {
    return this->GetArraySetting(this->GetArrayIndex(name));
    }
  int GetArraySetting(int index);

  // Description:
  // Remove all array entries.
  void RemoveAllArrays();

  //BTX
  // Description:
  // Add to the list of arrays that have entries.  For arrays that
  // already have entries, the settings are untouched.  For arrays
  // that don't already have an entry, they are assumed to be enabled.
  // This method should be called only by the filter owning this
  // object.
  int AddArray(const char* name);

  // Description:
  // Remove an array setting given its index.
  void RemoveArrayByIndex(int index);

  // Description:
  // Remove an array setting given its name.
  void RemoveArrayByName(const char* name);

  // Description:
  // Set the list of arrays that have entries.  For arrays that
  // already have entries, the settings are copied.  For arrays that
  // don't already have an entry, they are assigned the given default
  // status.  If no default status is given, it is assumed to be on.
  // There will be no more entries than the names given.  This method
  // should be called only by the filter owning this object.  The
  // signature with the default must have a different name due to a
  // bug in the Borland C++ 5.5 compiler.
  void SetArrays(const char* const* names, int numArrays);
  void SetArraysWithDefault(const char* const* names, int numArrays,
                            int defaultStatus);
  //ETX

  // Description:
  // Copy the selections from the given vtkDataArraySelection instance.
  void CopySelections(vtkDataArraySelection* selections);
protected:
  vtkDataArraySelection();
  ~vtkDataArraySelection();

  // Internal implementation details.
  vtkDataArraySelectionInternals* Internal;

private:
  vtkDataArraySelection(const vtkDataArraySelection&);  // Not implemented.
  void operator=(const vtkDataArraySelection&);  // Not implemented.
};

#endif
