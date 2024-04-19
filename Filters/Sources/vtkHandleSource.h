// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHandleSource
 * @brief   interface that can be inherited to define handler sources for any kind of interaction.
 *
 * vtkHandleSource is a pure abstract class defining an interface for handler sources.
 * Any child of this class is supposed to define an access to its position, size and direction, if
 * any. On this purpose, the internal getters/setters are left to be redefined by the subclasses.
 * It is derived by vtkPointHandleSource for example.
 * @sa
 * vtkPointHandleSource, vtkCameraHandleSource
 */

#ifndef vtkHandleSource_h
#define vtkHandleSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSOURCES_EXPORT vtkHandleSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkHandleSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get if the handle should take account on this->Direction.
   * The meaning of this direction varies on
   * the subclasses implementation.
   * The default value is false.
   */
  vtkSetMacro(Directional, bool);
  vtkGetMacro(Directional, bool);
  vtkBooleanMacro(Directional, bool);
  ///@}

  ///@{
  /**
   * Set the position of the handle.
   */
  void SetPosition(const double pos[3]) { this->SetPosition(pos[0], pos[1], pos[2]); }
  virtual void SetPosition(double xPos, double yPos, double zPos) = 0;
  ///@}

  ///@{
  /**
   * Get the position of the handle.
   */
  void GetPosition(double pos[3]);
  virtual double* GetPosition() = 0;
  ///@}

  ///@{
  /**
   * Set the direction of the handle.
   * The direction meaning depends on subclasses implementations.
   */
  void SetDirection(const double dir[3]) { this->SetDirection(dir[0], dir[1], dir[2]); }
  virtual void SetDirection(double xDir, double yDir, double zDir) = 0;
  ///@}

  ///@{
  /**
   * Get the direction of the handle.
   * The direction meaning depends on subclasses implementations.
   */
  void GetDirection(double dir[3]);
  virtual double* GetDirection() = 0;
  ///@}

  ///@{
  /**
   * Set/Get the size of the handle.
   * The size use depends on subclasses implementations.
   * The default value is 0.5.
   */
  vtkSetMacro(Size, double);
  vtkGetMacro(Size, double);
  ///@}

  vtkHandleSource(const vtkHandleSource&) = delete;
  void operator=(const vtkHandleSource&) = delete;

protected:
  vtkHandleSource();
  ~vtkHandleSource() override = default;

  // Flag to indicate if the handle should be aware of any direction.
  bool Directional = false;

  double Size = 0.5;
};

VTK_ABI_NAMESPACE_END
#endif
