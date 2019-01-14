/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSVGExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSVGExporter
 * @brief Exports vtkContext2D scenes to SVG.
 *
 * This exporter draws context2D scenes into a SVG file.
 *
 * Limitations:
 * - The Nearest/Linear texture properties are ignored, since SVG doesn't
 *   provide any reliable control over interpolation.
 * - Embedded fonts are experimental and poorly tested. Viewer support is
 *   lacking at the time of writing, hence the feature is largely useless. By
 *   default, fonts are not embedded since they're basically useless bloat.
 *   (this option is not exposed in vtkSVGExporter).
 * - TextAsPath is enabled by default, since viewers differ wildly in how they
 *   handle text objects (eg. Inkscape renders at expected size, but webkit is
 *   way too big).
 * - Pattern fills and markers are not shown on some viewers, e.g. KDE's okular
 *   (Webkit seems to work, though).
 * - Clipping seems to be broken in most viewers. Webkit is buggy and forces the
 *   clip coordinates to objectBoundingBox, even when explicitly set to
 *   userSpaceOnUse.
 * - Many viewers anti-alias the output, leaving thin outlines around the
 *   triangles that make up larger polygons. This is a viewer issue and there
 *   not much we can do about it from the VTK side of things (and most viewers
 *   don't seem to have an antialiasing toggle, either...).
 *
 * If ActiveRenderer is specified then it exports contents of
 * ActiveRenderer. Otherwise it exports contents of all renderers.
 */

#ifndef vtkSVGExporter_h
#define vtkSVGExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkExporter.h"

class vtkContextActor;
class vtkRenderer;
class vtkSVGContextDevice2D;
class vtkXMLDataElement;

class VTKIOEXPORT_EXPORT vtkSVGExporter: public vtkExporter
{
public:
  static vtkSVGExporter* New();
  vtkTypeMacro(vtkSVGExporter, vtkExporter)
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /** The title of the exported document. @{ */
  vtkSetStringMacro(Title)
  vtkGetStringMacro(Title)
  /** @} */

  /** A description of the exported document. @{ */
  vtkSetStringMacro(Description)
  vtkGetStringMacro(Description)
  /** @} */

  /** The name of the exported file. @{ */
  vtkSetStringMacro(FileName)
  vtkGetStringMacro(FileName)
  /** @} */

  /**
   * If true, draw all text as path objects rather than text objects. Enabling
   * this option will:
   *
   * - Improve portability (text will look exactly the same everywhere).
   * - Increase file size (text objects are much more compact than paths).
   * - Prevent text from being easily edited (text metadata is lost).
   *
   * Note that some text (e.g. MathText) is always rendered as a path.
   *
   * The default is true, as many browsers and SVG viewers render text
   * inconsistently.
   *
   * @{
   */
  vtkSetMacro(TextAsPath, bool)
  vtkGetMacro(TextAsPath, bool)
  vtkBooleanMacro(TextAsPath, bool)
  /**@}*/

  /**
   * If true, the background will be drawn into the output document. Default
   * is true.
   * @{
   */
  vtkSetMacro(DrawBackground, bool)
  vtkGetMacro(DrawBackground, bool)
  vtkBooleanMacro(DrawBackground, bool)
  /**@}*/

  /**
   * Set the threshold for subdividing gradient-shaded polygons/line. Default
   * value is 1, and lower values yield higher quality and larger files. Larger
   * values will reduce the number of primitives, but will decrease quality.
   *
   * A triangle / line will not be subdivided further if all of it's vertices
   * satisfy the equation:
   *
   * |v1 - v2|^2 < thresh
   *
   * e.g. the squared norm of the vector between any verts must be greater than
   * the threshold for subdivision to occur.
   *
   * @{
   */
  vtkSetMacro(SubdivisionThreshold, float)
  vtkGetMacro(SubdivisionThreshold, float)
  /**@}*/

protected:
  vtkSVGExporter();
  ~vtkSVGExporter() override;

  void WriteData() override;

  void WriteSVG();
  void PrepareDocument();
  void RenderContextActors();
  void RenderBackground(vtkRenderer *ren);
  void RenderContextActor(vtkContextActor *actor,
                          vtkRenderer *renderer);

  char *Title;
  char *Description;
  char *FileName;

  vtkSVGContextDevice2D *Device;
  vtkXMLDataElement *RootNode;
  vtkXMLDataElement *PageNode;
  vtkXMLDataElement *DefinitionNode;

  float SubdivisionThreshold;
  bool DrawBackground;
  bool TextAsPath;

private:
  vtkSVGExporter(const vtkSVGExporter&) = delete;
  void operator=(const vtkSVGExporter&) = delete;
};

#endif // vtkSVGExporter_h
