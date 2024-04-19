// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCompositeCellGridMapper
 * @brief   a class that renders hierarchical cell-grid data
 *
 * This class uses a vtkCellGridMapper to render input data
 * which may be hierarchical. The input to this mapper may be
 * either vtkCellGrid or a vtkCompositeDataSet built from
 * cell-grids. If something other than vtkCellGrid is encountered,
 * an error message will be produced.
 * @sa
 * vtkCellGridMapper
 */

#ifndef vtkCompositeCellGridMapper_h
#define vtkCompositeCellGridMapper_h

#include "vtkMapper.h"

#include "vtkCompositeDataDisplayAttributes.h" // for ivar
#include "vtkRenderingCoreModule.h"            // For export macro.
#include "vtkSmartPointer.h"                   // for ivar
#include "vtkStateStorage.h"                   // for ivar
#include "vtkStringToken.h"                    // For ivars

#include <memory> // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGridMapper;
class vtkInformation;
class vtkRenderer;
class vtkActor;

class VTKRENDERINGCORE_EXPORT vtkCompositeCellGridMapper : public vtkMapper
{

public:
  static vtkCompositeCellGridMapper* New();
  vtkTypeMacro(vtkCompositeCellGridMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Standard method for rendering a mapper. This method will be
   * called by the actor.
   */
  void Render(vtkRenderer* ren, vtkActor* a) override;

  ///@{
  /**
   * Standard vtkProp method to get 3D bounds of a 3D prop
   */
  double* GetBounds() VTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]) override { this->Superclass::GetBounds(bounds); }
  ///@}

  /**
   * Release the underlying resources associated with this mapper
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  ///@{
  /**
   * Some introspection on the type of data the mapper will render
   * used by props to determine if they should invoke the mapper
   * on a specific rendering pass.
   */
  bool HasOpaqueGeometry() override;
  bool RecursiveHasTranslucentGeometry(vtkDataObject* dobj, unsigned int& flat_index);
  bool HasTranslucentPolygonalGeometry() override;
  ///@}

  /// Set/get the name of the process ID attribute used during selection.
  vtkSetStringTokenMacro(ProcessIdAttributeName);
  vtkGetStringTokenMacro(ProcessIdAttributeName);
  /// Set/get the name of the composite ID attribute used during selection.
  vtkSetStringTokenMacro(CompositeIdAttributeName);
  vtkGetStringTokenMacro(CompositeIdAttributeName);
  /// Set/get the name of the point ID attribute used during selection.
  vtkSetStringTokenMacro(PointIdAttributeName);
  vtkGetStringTokenMacro(PointIdAttributeName);
  /// Set/get the name of the cell ID attribute used during selection.
  vtkSetStringTokenMacro(CellIdAttributeName);
  vtkGetStringTokenMacro(CellIdAttributeName);

  /// FIXME: These are temporary. They exist to test what is needed
  ///        to use the CompositeCellGridMapper as a drop-in replacement
  ///        for the CompositePolyDataMapper2 inside ParaView's
  ///        GeometryRepresentation.
  void SetProcessIdArrayName(const char*) {}
  void SetCompositeIdArrayName(const char*) {}
  void SetPointIdArrayName(const char*) {}
  void SetCellIdArrayName(const char*) {}

  /// Set/get a data structure that can be used to control
  /// per-object visibility, opacity, and pickability.
  ///
  /// These methods exist for compatibility with ParaView
  /// but may be replaced in the future with a more flexible
  /// system that allows more control using CSS-style selectors
  /// and properties.
  ///
  /// Note that CompositeDataDisplayAttributes is null by default.
  vtkGetSmartPointerMacro(CompositeDataDisplayAttributes, vtkCompositeDataDisplayAttributes);
  virtual void SetCompositeDataDisplayAttributes(vtkCompositeDataDisplayAttributes*);

  /// Account for mtime of vtkCompositeDataDisplayAttributes
  vtkMTimeType GetMTime() override;

protected:
  vtkCompositeCellGridMapper();
  ~vtkCompositeCellGridMapper() override;

  /**
   * We need to override this method because the standard streaming
   * demand driven pipeline is not what we want - we are expecting
   * hierarchical data as input
   */
  vtkExecutive* CreateDefaultExecutive() override;

  /**
   * Need to define the type of data handled by this mapper.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * This is the build method for creating the internal polydata
   * mapper that do the actual work
   */
  void BuildRenderValues(
    vtkRenderer* renderer, vtkActor* actor, vtkDataObject* dobj, unsigned int& flatIndex);

  /**
   * BuildRenderValues uses this for each mapper. It is broken out so we can change types.
   */
  virtual vtkCellGridMapper* MakeAMapper();

  /**
   * Need to loop over the hierarchy to compute bounds
   */
  void ComputeBounds();

  /**
   * Time stamp for computation of bounds.
   */
  vtkTimeStamp BoundsMTime;

  // State at the time translucent tests were performed.
  vtkStateStorage TranslucentState;
  // State at the time render values were built.
  vtkStateStorage RenderValuesState;
  // Temporary state used for comparisons against above states.
  vtkStateStorage TempState;
  // Cached result of HasTranslucentPolygonalGeometry
  bool HasTranslucentGeometry = false;

  /// Names of attributes
  vtkStringToken ProcessIdAttributeName;
  vtkStringToken CompositeIdAttributeName;
  vtkStringToken PointIdAttributeName;
  vtkStringToken CellIdAttributeName;
  vtkSmartPointer<vtkCompositeDataDisplayAttributes> CompositeDataDisplayAttributes;

private:
  vtkCompositeCellGridMapper(const vtkCompositeCellGridMapper&) = delete;
  void operator=(const vtkCompositeCellGridMapper&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
