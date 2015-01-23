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
// .NAME vtkLogoRepresentation - represent the vtkLogoWidget
// .SECTION Description

// This class provides support for interactively positioning a logo. A logo
// is defined by an instance of vtkImage. The properties of the image,
// including transparency, can be set with an instance of vtkProperty2D. To
// position the logo, use the superclass's Position and Position2 coordinates.

// .SECTION See Also
// vtkLogoWidget


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
  // Description:
  // Instantiate this class.
  static vtkLogoRepresentation *New();

  // Description:
  // Standard VTK class methods.
  vtkTypeMacro(vtkLogoRepresentation,vtkBorderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify/retrieve the image to display in the balloon.
  virtual void SetImage(vtkImageData *img);
  vtkGetObjectMacro(Image,vtkImageData);

  // Description:
  // Set/get the image property (relevant only if an image is shown).
  virtual void SetImageProperty(vtkProperty2D *p);
  vtkGetObjectMacro(ImageProperty,vtkProperty2D);

  // Description:
  // Satisfy the superclasses' API.
  virtual void BuildRepresentation();

  // Description:
  // These methods are necessary to make this representation behave as
  // a vtkProp.
  virtual void GetActors2D(vtkPropCollection *pc);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOverlay(vtkViewport*);

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
  void AdjustImageSize(double o[2], double borderSize[2], double imageSize[2]);

private:
  vtkLogoRepresentation(const vtkLogoRepresentation&);  //Not implemented
  void operator=(const vtkLogoRepresentation&);  //Not implemented
};

#endif
