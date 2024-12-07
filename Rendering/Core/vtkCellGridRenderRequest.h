// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class    vtkCellGridRenderRequest
 * @brief    State used by vtkCellGridMapper during rendering.
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

#ifndef vtkCellGridRenderRequest_h
#define vtkCellGridRenderRequest_h
#include "vtkCellGridQuery.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkStringToken.h"         // for ivars

#include <memory>        // for unique_ptr
#include <unordered_map> // for this->State

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkCellGridMapper;
class vtkRenderer;
class vtkWindow;

class VTKRENDERINGCORE_EXPORT vtkCellGridRenderRequest : public vtkCellGridQuery
{
public:
  class VTKRENDERINGCORE_EXPORT BaseState
  {
  public:
    BaseState() = default;
    virtual ~BaseState() = default; // Force class to be polymorphic.
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

  vtkTypeMacro(vtkCellGridRenderRequest, vtkCellGridQuery);
  static vtkCellGridRenderRequest* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Set/get the mapper which owns this request (so responders can inspect its configuration).
  ///
  /// Note that the render request does **NOT** increase the reference count
  /// of the mapper (thus it does not take ownership). This is because the
  /// request is owned by the mapper and we do not want to create a reference loop.
  virtual void SetMapper(vtkCellGridMapper* mapper);
  vtkGetObjectMacro(Mapper, vtkCellGridMapper);

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
  bool Initialize() override;
  /// This is invoked after processing all cell types during a render.
  bool Finalize() override;

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
  StateType* GetState(vtkStringToken cellType, bool create = true)
  {
    auto it = this->State.find(cellType);
    if (it != this->State.end())
    {
      return static_cast<StateType*>(it->second.get());
    }

    if (create)
    {
      it = this->State.insert(std::make_pair(cellType, std::unique_ptr<BaseState>(new StateType)))
             .first;
      return static_cast<StateType*>(it->second.get());
    }

    return nullptr;
  }

protected:
  vtkCellGridRenderRequest() = default;
  ~vtkCellGridRenderRequest() override;

  vtkCellGridMapper* Mapper{ nullptr };
  vtkActor* Actor{ nullptr };
  vtkRenderer* Renderer{ nullptr };
  vtkWindow* Window{ nullptr };
  bool IsReleasingResources{ false };
  char ShapesToDraw{ RenderableGeometry::ALL };
  std::unordered_map<vtkStringToken, std::unique_ptr<BaseState>> State;

private:
  vtkCellGridRenderRequest(const vtkCellGridRenderRequest&) = delete;
  void operator=(const vtkCellGridRenderRequest&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridRenderRequest_h
