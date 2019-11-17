/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeLookupTables.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=============================================================================
Copyright and License information
=============================================================================*/
/**
 * @class vtkOpenGLVolumeLookupTables
 * @brief Internal class that manages multiple lookup tables
 *
 */

#ifndef vtkOpenGLVolumeLookupTables_h
#define vtkOpenGLVolumeLookupTables_h
#ifndef __VTK_WRAP__

#include "vtkObject.h"

// STL includes
#include <vector>

// Forward declarations
class vtkWindow;

template <class T>
class vtkOpenGLVolumeLookupTables : public vtkObject
{
public:
  vtkTypeMacro(vtkOpenGLVolumeLookupTables, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkOpenGLVolumeLookupTables<T>* New();

  /**
   * Create internal lookup tables
   */
  virtual void Create(std::size_t numberOfTables);

  /**
   * Get access to the raw table pointer
   */
  T* GetTable(std::size_t i) const;

  /**
   * Get number of tables
   */
  std::size_t GetNumberOfTables() const;

  /**
   * Release graphics resources
   */
  void ReleaseGraphicsResources(vtkWindow* win);

protected:
  vtkOpenGLVolumeLookupTables() = default;
  virtual ~vtkOpenGLVolumeLookupTables() override;

  std::vector<T*> Tables;

private:
  vtkOpenGLVolumeLookupTables(const vtkOpenGLVolumeLookupTables&) = delete;
  void operator=(const vtkOpenGLVolumeLookupTables&) = delete;
};

#include "vtkOpenGLVolumeLookupTables.txx"

#endif // __VTK_WRAP__
#endif // vtkOpenGLVolumeLookupTables_h
// VTK-HeaderTest-Exclude: vtkOpenGLVolumeLookupTables.h
