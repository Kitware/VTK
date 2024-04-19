// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkOpenGLVolumeLookupTables
 * @brief Internal class that manages multiple lookup tables
 *
 */

#ifndef vtkOpenGLVolumeLookupTables_h
#define vtkOpenGLVolumeLookupTables_h

#include "vtkObject.h"

#include <vector> // for std::vector

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
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
  ~vtkOpenGLVolumeLookupTables() override;

  std::vector<T*> Tables;

private:
  vtkOpenGLVolumeLookupTables(const vtkOpenGLVolumeLookupTables&) = delete;
  void operator=(const vtkOpenGLVolumeLookupTables&) = delete;
};

VTK_ABI_NAMESPACE_END
#include "vtkOpenGLVolumeLookupTables.txx" // template implementations

#endif // vtkOpenGLVolumeLookupTables_h
// VTK-HeaderTest-Exclude: vtkOpenGLVolumeLookupTables.h
