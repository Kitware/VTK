// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class    vtkOpenGLCellGridRenderRequest
 * @brief    State used by vtkOpenGLCellGridMapper during rendering.
 *
 * This is a vtkCellGridQuery subclass that mappers can use to draw cells
 * into a renderer using an actor and, subsequently, to release resources.
 *
 * Note that this request has two modes: it will either instruct
 * responders to draw cells (IsReleasingResources == false) or instruct
 * responders to release OpenGL objects for a particular window
 * (when IsReleasingResources == true).
 * Responders must call GetIsReleasingResources() and only perform
 * one task or the other, depending on the returned value.
 */

#ifndef vtkOpenGLCellGridRenderRequest_h
#define vtkOpenGLCellGridRenderRequest_h
#include "vtkCellGridQuery.h"
#include "vtkRenderingCellGridModule.h" // For export macro
#include "vtkStringToken.h"             // for ivars

#include <memory>        // for unique_ptr
#include <unordered_map> // for this->State

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkOpenGLCellGridMapper;
class vtkRenderer;
class vtkWindow;

class VTKRENDERINGCELLGRID_EXPORT vtkOpenGLCellGridRenderRequest : public vtkCellGridQuery
{
public:
  /// An empty base class that responders should inherit to store state using GetState<Subclass>().
  class StateBase
  {
  public:
    virtual ~StateBase() = default;
  };

  /// An enumeration of which shapes to render.
  ///
  /// A cell may be represented by its interior and/or its boundaries
  /// of any dimension. These enumerants indicate the dimension of
  /// shape to render as a representation of the cell.
  ///
  /// The default is to render the cell's shape itself (if possible)
  /// and any sides for which arrays exist. Note that the cell-grid
  /// mapper does not currently support volume rendering, so VOLUME
  /// is ignored; if you wish to render volumetric cells, you must
  /// run the vtkCellGridExtractSurface filter to generate side-set
  /// arrays for boundaries of interest.
  enum RenderableGeometry : unsigned char
  {
    VERTICES = 1,
    EDGES = 2,
    FACES = 4,
    VOLUMES = 8,
    SURFACE_WITH_EDGES = EDGES | FACES,
    ALL = VERTICES | EDGES | FACES | VOLUMES
  };

  vtkTypeMacro(vtkOpenGLCellGridRenderRequest, vtkCellGridQuery);
  static vtkOpenGLCellGridRenderRequest* New();
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  /// Set/get the mapper which owns this request (so responders can inspect its configuration).
  ///
  /// Note that the render request does **NOT** increase the reference count
  /// of the mapper (thus it does not take ownership). This is because the
  /// request is owned by the mapper and we do not want to create a reference loop.
  virtual void SetMapper(vtkOpenGLCellGridMapper* mapper);
  vtkGetObjectMacro(Mapper, vtkOpenGLCellGridMapper);

  /// Set/get the actor which responders should use to draw cells.
  ///
  /// Note that the render request does **NOT** increase the reference count
  /// of the actor (thus it does not take ownership). This is to avoid creating
  /// a reference loop.
  virtual void SetActor(vtkActor* actor);
  vtkGetObjectMacro(Actor, vtkActor);

  /// Set/get the renderer responders should use to draw cells.
  ///
  /// Note that the render request does **NOT** increase the reference count
  /// of the renderer (thus it does not take ownership). This is to avoid creating
  /// a reference loop.
  virtual void SetRenderer(vtkRenderer* renderer);
  vtkGetObjectMacro(Renderer, vtkRenderer);

  /// Set/get a window (used when IsReleasingResources is true).
  ///
  /// Note that the render request does **NOT** increase the reference count
  /// of the window (thus it does not take ownership). This is to avoid creating
  /// a reference loop.
  virtual void SetWindow(vtkWindow* window);
  vtkGetObjectMacro(Window, vtkWindow);

  /// Set/get what geometric data to draw for each cell.
  ///
  /// The DEFAULT is currently equivalent to ALL.
  /// Note that any combination of RenderableGeometry
  /// enumerants is accepted.
  ///
  /// \sa RenderableGeometry
  vtkGetMacro(ShapesToDraw, char);
  vtkSetClampMacro(ShapesToDraw, char, 1, 9);

  /// This is invoked before processing any cell types during a render.
  void Initialize() override;
  /// This is invoked after processing all cell types during a render.
  void Finalize() override;

  /**
   * Set/get whether the request should render (false) or release resources (true).
   * The latter should be performed as a separate query after rendering.
   *
   * Note that after a successful call to Query with IsReleasingResources set to
   * true, the Finalize() method will reset IsReleasingResources to false, which
   * results in the request being marked modified.
   */
  vtkGetMacro(IsReleasingResources, bool);
  vtkSetMacro(IsReleasingResources, bool);

  /**
   * Return a state object of the given type.
   * This method is intended for responders to store data with the request.
   */
  template <typename StateType>
  StateType* GetState(vtkStringToken cellType)
  {
    auto it = this->State.find(cellType);
    if (it == this->State.end())
    {
      it = this->State.insert(std::make_pair(cellType, std::unique_ptr<StateBase>(new StateType)))
             .first;
    }
    return static_cast<StateType*>(it->second.get());
  }

protected:
  vtkOpenGLCellGridRenderRequest();
  ~vtkOpenGLCellGridRenderRequest() override;

  vtkOpenGLCellGridMapper* Mapper{ nullptr };
  vtkActor* Actor{ nullptr };
  vtkRenderer* Renderer{ nullptr };
  vtkWindow* Window{ nullptr };
  bool IsReleasingResources{ false };
  char ShapesToDraw{ RenderableGeometry::ALL };
  std::unordered_map<vtkStringToken, std::unique_ptr<StateBase>> State;

private:
  vtkOpenGLCellGridRenderRequest(const vtkOpenGLCellGridRenderRequest&) = delete;
  void operator=(const vtkOpenGLCellGridRenderRequest&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLCellGridRenderRequest_h
