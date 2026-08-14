// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkScivisExporter
 * @brief   Getting a scene out of a view, as an image or as geometry.
 *
 * vtkScivisExporter is the part of a view that writes what it is showing to a
 * file, reached as vtkScivisView::GetExporter().  Two things come out of a
 * scene: a picture of it, and the geometry in it.
 *
 * @code
 * view->GetExporter()->SaveScreenshot("frame.png");
 * view->GetExporter()->ExportScene("scene.gltf");
 * @endcode
 *
 * Both pick their format from the file extension, so choosing one is choosing a
 * name.  Images are written as .png, .jpg, .tif or .bmp; geometry as .gltf,
 * .obj, .vrml or .x3d.
 *
 * @par How a capture comes out:
 * Magnification and TransparentBackground describe every capture this exporter
 * makes rather than being repeated at each call, so an application that wants
 * everything at twice the window size says so once.  CaptureImage() hands back
 * the picture instead of writing it, for an application that has somewhere else
 * to put it.
 *
 * @par What it does to the view:
 * The scene is brought up to date and drawn before anything is written, so what
 * comes out is what is on screen.  A transparent capture has to turn the
 * background off to take it, and puts back what was there afterwards, so the
 * view looks the same before and after.
 *
 * @sa vtkScivisView vtkWindowToImageFilter
 */

#ifndef vtkScivisExporter_h
#define vtkScivisExporter_h

#include "vtkObject.h"
#include "vtkSmartPointer.h"      // For the returned image
#include "vtkViewsScivisModule.h" // For export macro
#include "vtkWeakPointer.h"       // For ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkScivisView;

class VTKVIEWSSCIVIS_EXPORT vtkScivisExporter : public vtkObject
{
public:
  static vtkScivisExporter* New();
  vtkTypeMacro(vtkScivisExporter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * How much larger than the render window a capture comes out, so 2 gives an
   * image twice the width and height.  Default is 1.
   */
  vtkSetClampMacro(Magnification, int, 1, VTK_INT_MAX);
  vtkGetMacro(Magnification, int);
  ///@}

  ///@{
  /**
   * Whether a capture has a transparent background rather than the view's.
   * Default is off.
   *
   * The image comes back with four components when this is on.  Taking it means
   * turning the view's background off for the moment it takes to read the
   * window, which is put back afterwards.
   */
  vtkSetMacro(TransparentBackground, bool);
  vtkGetMacro(TransparentBackground, bool);
  vtkBooleanMacro(TransparentBackground, bool);
  ///@}

  /**
   * Write a picture of the scene, as .png, .jpg, .tif or .bmp.  Returns false
   * if the name has no extension this understands, or none at all.
   */
  bool SaveScreenshot(const char* filename);

  /**
   * The same picture, handed back rather than written.  Returns null if the
   * window cannot be read.
   */
  vtkSmartPointer<vtkImageData> CaptureImage();

  /**
   * Write the geometry in the scene, as .gltf, .obj, .vrml/.wrl or .x3d.
   * Returns false if the name has no extension this understands.
   *
   * This is the scene rather than a picture of it, so Magnification and
   * TransparentBackground mean nothing here.
   */
  bool ExportScene(const char* filename);

protected:
  vtkScivisExporter();
  ~vtkScivisExporter() override;

private:
  friend class vtkScivisView;

  // The view this exports, held weakly: the exporter is reached through the
  // view, but Python can outlive it -- `e = view.exporter; del view` -- and an
  // exporter with nothing to export should say so rather than crash.
  void SetView(vtkScivisView* view);
  vtkWeakPointer<vtkScivisView> View;

  vtkScivisExporter(const vtkScivisExporter&) = delete;
  void operator=(const vtkScivisExporter&) = delete;

  int Magnification;
  bool TransparentBackground;
};

VTK_ABI_NAMESPACE_END
#endif
