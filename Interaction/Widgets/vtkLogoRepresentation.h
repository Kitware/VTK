/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLogoRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLogoRepresentation
 * @brief   represent the vtkLogoWidget
 *
 *
 * This class provides support for interactively positioning a logo. A logo
 * is defined by an instance of vtkImage. The properties of the image,
 * including transparency, can be set with an instance of vtkProperty2D. To
 * position the logo, use the superclass's Position and Position2 coordinates.
 *
 * @sa
 * vtkLogoWidget
*/

#ifndef vtkLogoRepresentation_h
#define vtkLogoRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderRepresentation.h"

class vtkImageData;
class vtkImageProperty;
class vtkTexture;
class vtkPolyData;
class vtkPoionts;
class vtkPolyDataMapper2D;
class vtkTexturedActor2D;
class vtkProperty2D;


class VTKINTERACTIONWIDGETS_EXPORT vtkLogoRepresentation : public vtkBorderRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkLogoRepresentation *New();

  //@{
  /**
   * Standard VTK class methods.
   */
  vtkTypeMacro(vtkLogoRepresentation,vtkBorderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  //@{
  /**
   * Specify/retrieve the image to display in the balloon.
   */
  virtual void SetImage(vtkImageData *img);
  vtkGetObjectMacro(Image,vtkImageData);
  //@}

  //@{
  /**
   * Set/get the image property (relevant only if an image is shown).
   */
  virtual void SetImageProperty(vtkProperty2D *p);
  vtkGetObjectMacro(ImageProperty,vtkProperty2D);
  //@}

  /**
   * Satisfy the superclasses' API.
   */
  virtual void BuildRepresentation();

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp.
   */
  virtual void GetActors2D(vtkPropCollection *pc);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOverlay(vtkViewport*);
  //@}

protected:
  vtkLogoRepresentation();
  ~vtkLogoRepresentation();

  // data members
  vtkImageData  *Image;
  vtkProperty2D *ImageProperty;

  // Represent the image
  vtkTexture          *Texture;
  vtkPoints           *TexturePoints;
  vtkPolyData         *TexturePolyData;
  vtkPolyDataMapper2D *TextureMapper;
  vtkTexturedActor2D          *TextureActor;

  // Helper methods
  virtual void AdjustImageSize(double o[2], double borderSize[2], double imageSize[2]);

private:
  vtkLogoRepresentation(const vtkLogoRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLogoRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
