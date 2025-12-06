// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImporter
 * @brief   importer abstract class
 *
 * vtkImporter is an abstract class that specifies the protocol for
 * importing actors, cameras, lights and properties into a
 * vtkRenderWindow. The following takes place:
 * 1) Create a RenderWindow and Renderer if none is provided.
 * 2) Call ImportBegin, if ImportBegin returns False, return
 * 3) Call ReadData, which calls:
 *  a) Import the Actors
 *  b) Import the cameras
 *  c) Import the lights
 *  d) Import the Properties
 * 7) Call ImportEnd
 *
 * Subclasses optionally implement the ImportActors, ImportCameras,
 * ImportLights and ImportProperties or ReadData methods. An ImportBegin and
 * ImportEnd can optionally be provided to perform Importer-specific
 * initialization and termination.  The Read method initiates the import
 * process. If a RenderWindow is provided, its Renderer will contained
 * the imported objects. If the RenderWindow has no Renderer, one is
 * created. If no RenderWindow is provided, both a RenderWindow and
 * Renderer will be created. Both the RenderWindow and Renderer can be
 * accessed using Get methods.
 *
 * @sa
 * vtk3DSImporter vtkExporter
 */

#ifndef vtkImporter_h
#define vtkImporter_h

#include "vtkDataAssembly.h"   // for vtkDataAssembly
#include "vtkDeprecation.h"    // For VTK_DEPRECATED_IN_9_6_0
#include "vtkIOImportModule.h" // For export macro
#include "vtkResourceStream.h" // For Stream
#include "vtkSmartPointer.h"   // for vtkSmartPointer

#include "vtkObject.h"

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkActorCollection;
class vtkCollection;
class vtkDataSet;
class vtkDoubleArray;
class vtkInformationIntegerKey;
class vtkLightCollection;
class vtkRenderWindow;
class vtkRenderer;

class VTKIOIMPORT_EXPORT vtkImporter : public vtkObject
{
public:
  vtkTypeMacro(vtkImporter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file name of the file to read
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify stream to read from
   * When both `Stream` and `Filename` are set, it's left to the implementation to determine which
   * one is used. If both are not set, importer outputs nothing.
   */
  vtkSetSmartPointerMacro(Stream, vtkResourceStream);
  vtkGetSmartPointerMacro(Stream, vtkResourceStream);
  ///@}

  ///@{
  /**
   * Get the renderer that contains the imported actors, cameras and
   * lights.
   */
  vtkGetObjectMacro(Renderer, vtkRenderer);
  ///@}

  ///@{
  /**
   * Get the hierarchy of actors, cameras and lights in the renderer.
   * Implementations should strive to pack the hierarchy information from
   * the file in to a vtkDataAssembly using node names from the file.
   */
  vtkGetObjectMacro(SceneHierarchy, vtkDataAssembly);
  ///@}

  ///@{
  /**
   * Get collection of actors, cameras and lights that were imported by
   * this importer. Note that this may return empty collections
   * if not used in the concrete importer.
   */
  vtkActorCollection* GetImportedActors() { return this->ActorCollection.Get(); }
  vtkCollection* GetImportedCameras() { return this->CameraCollection.Get(); }
  vtkLightCollection* GetImportedLights() { return this->LightCollection.Get(); }
  ///@}

  ///@{
  /**
   * Set the vtkRenderWindow to contain the imported actors, cameras and
   * lights, If no vtkRenderWindow is set, one will be created and can be
   * obtained with the GetRenderWindow method. If the vtkRenderWindow has been
   * specified, the first vtkRenderer it has will be used to import the
   * objects. If the vtkRenderWindow has no Renderer, one will be created and
   * can be accessed using GetRenderer.
   */
  virtual void SetRenderWindow(vtkRenderWindow*);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  ///@}

  /**
   * Import the actors, cameras, lights and properties into a vtkRenderWindow
   * and return if it was successful of not.
   */
  VTK_UNBLOCKTHREADS
  bool Update();

  /**
   * Recover a printable string that let importer implementation
   * Describe their outputs.
   */
  virtual std::string GetOutputsDescription() { return ""; }

  enum class AnimationSupportLevel : unsigned char
  {
    NONE,
    UNIQUE,
    SINGLE,
    MULTI
  };

  /**
   * Get the level of animation support, this is coming either
   * from the file format or as a limitation of the implementation.
   * NONE: There is no support for animation, GetNumberOfAnimations() return -1.
   * UNIQUE: There will always will ever be, at most, a single animation, with any file,
   * GetNumberOfAnimations() returns 0 or 1. SINGLE: There can be multiple available animations, but
   * only one can be enable. Calling EnableAnimation(i) will disable other animations. MULTI: There
   * can be multiple animations and multiple ones can be enabled at the same time. Calling
   * EnableAnimation(i) will not disable other animation.
   *
   * In this base implementation, this method returns NONE.
   */
  virtual AnimationSupportLevel GetAnimationSupportLevel() { return AnimationSupportLevel::NONE; }

  /**
   * Get the number of available animations.
   * Return -1 if not provided by implementation.
   */
  virtual vtkIdType GetNumberOfAnimations();

  /**
   * Get the name of an animation.
   * Return an empty if not provided by implementation.
   */
  virtual std::string GetAnimationName(vtkIdType vtkNotUsed(animationIndex)) { return ""; }

  ///@{
  /**
   * Enable/Disable/Get the status of specific animations
   */
  virtual void EnableAnimation(vtkIdType vtkNotUsed(animationIndex)) {}
  virtual void DisableAnimation(vtkIdType vtkNotUsed(animationIndex)) {}
  virtual bool IsAnimationEnabled(vtkIdType vtkNotUsed(animationIndex)) { return false; }
  ///@}

  /**
   * Get the number of available cameras.
   * Return 0 if not provided by implementation.
   */
  virtual vtkIdType GetNumberOfCameras() { return 0; }

  /**
   * Get the name of a camera.
   * Return an empty string if not provided by implementation.
   */
  virtual std::string GetCameraName(vtkIdType vtkNotUsed(camIndex)) { return ""; }

  /**
   * Enable a specific camera.
   * If a negative index is provided, no camera from the importer is used.
   * Does nothing if not provided by implementation.
   */
  virtual void SetCamera(vtkIdType vtkNotUsed(camIndex)) {}

  /**
   * DEPRECATED, use the version without framerate
   * Get temporal information for the provided animationIndex and frameRate.
   * This implementation return false, but concrete class implementation
   * behavior is as follows.
   * frameRate is used to define the number of frames for one second of simulation,
   * set to zero if timeSteps are not needed.
   * If animation is present in the dataset, timeRange should be set by this method, return true.
   * If animation is present and frameRate > 0, nbTimeSteps and timeSteps should also be set, return
   * true. If animation is not present, return false.
   */
  VTK_DEPRECATED_IN_9_6_0("Use GetTemporalInformation without framerate parameter instead.")
  virtual bool GetTemporalInformation(vtkIdType animationIndex, double frameRate, int& nbTimeSteps,
    double timeRange[2], vtkDoubleArray* timeSteps);

  /**
   * Get temporal information for the provided animationIndex.
   * This implementation return false, but concrete class implementation
   * behavior is as follows.
   * If animation is present in the dataset, timeRange should be set by this method and if this
   * importer support time steps (aka KeyFrames), nbTimeSteps and timeSteps should also be set,
   * return true. If animation is not present, return false.
   */
  virtual bool GetTemporalInformation(
    vtkIdType animationIndex, double timeRange[2], int& nbTimeSteps, vtkDoubleArray* timeSteps);

  /**
   * Import the actors, camera, lights and properties at a specific time value.
   * Returns if successful or not.
   * If not reimplemented, returns true.
   */
  virtual bool UpdateAtTimeValue(double vtkNotUsed(timeValue)) { return true; };

  enum class InterpolateAnimationSupportLevel : unsigned char
  {
    NEVER,
    CAPABLE,
    ALWAYS,
  };

  /**
   * Get the level of animation interpolation support, this is coming either
   * from the file format or as a limitation of the implementation.
   * NEVER: There is no support for interpolation, updating at a time which is not a timestep will
   * just provide previous time step CAPABLE: This importer can interpolate between time steps,
   * using InterpolateBetweenTimeSteps to control the behavior ALWAYS: This importer always
   * interpolate between time steps because this format doesn't define time steps
   *
   * In this base implementation, this method returns NEVER.
   */
  virtual InterpolateAnimationSupportLevel GetInterpolateAnimationSupportLevel()
  {
    return InterpolateAnimationSupportLevel::NEVER;
  }

  ///@{
  /**
   * Enable/Disable time steps interpolation.
   * Default is true.
   */
  vtkSetMacro(InterpolateBetweenTimeSteps, bool);
  vtkGetMacro(InterpolateBetweenTimeSteps, bool);
  vtkBooleanMacro(InterpolateBetweenTimeSteps, bool);
  ///@}

  ///@{
  /**
   * Enable/Disable armature actors import if supported.
   * Default is false.
   */
  vtkSetMacro(ImportArmature, bool);
  vtkGetMacro(ImportArmature, bool);
  vtkBooleanMacro(ImportArmature, bool);
  ///@}

protected:
  vtkImporter();
  ~vtkImporter() override;

  virtual int ImportBegin() { return 1; }
  virtual void ImportEnd() {}
  virtual void ImportActors(vtkRenderer*) {}
  virtual void ImportCameras(vtkRenderer*) {}
  virtual void ImportLights(vtkRenderer*) {}
  virtual void ImportProperties(vtkRenderer*) {}
  virtual void ReadData();

  enum class UpdateStatusEnum : bool
  {
    SUCCESS,
    FAILURE
  };

  /**
   * Set the update status.
   * Importer implementation should set this during Import
   * if import fails for any reason.
   * vtkImporter::Update set this to SUCCESS on call.
   * Default is SUCCESS;
   */
  void SetUpdateStatus(UpdateStatusEnum updateStatus)
  {
    this->UpdateStatus = updateStatus;
    this->Modified();
  }

  /**
   * Get the update status
   */
  UpdateStatusEnum GetUpdateStatus() { return this->UpdateStatus; }

  static std::string GetDataSetDescription(vtkDataSet* ds, vtkIndent indent);
  static std::string GetArrayDescription(vtkAbstractArray* array, vtkIndent indent);

  vtkRenderer* Renderer = nullptr;
  vtkRenderWindow* RenderWindow = nullptr;
  vtkSmartPointer<vtkDataAssembly> SceneHierarchy;

  vtkNew<vtkActorCollection> ActorCollection;
  vtkNew<vtkCollection> CameraCollection;
  vtkNew<vtkLightCollection> LightCollection;

private:
  vtkImporter(const vtkImporter&) = delete;
  void operator=(const vtkImporter&) = delete;

  bool SetAndCheckUpdateStatus();

  char* FileName = nullptr;
  vtkSmartPointer<vtkResourceStream> Stream;

  UpdateStatusEnum UpdateStatus = UpdateStatusEnum::SUCCESS;
  bool ImportArmature = false;
  bool InterpolateBetweenTimeSteps = true;
};

VTK_ABI_NAMESPACE_END
#endif
