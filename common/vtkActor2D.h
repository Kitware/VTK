#ifndef __vtkActor2D_h
#define __vtkActor2D_h

#include "vtkReferenceCount.h"
#include "vtkTransform.h"

class vtkMapper2D;
class vtkProperty2D;
class vtkViewport;

#define VTK_VIEW_COORD    0
#define VTK_DISPLAY_COORD 1
#define VTK_WORLD_COORD   2

class VTK_EXPORT vtkActor2D : public vtkReferenceCount
{
public:

  vtkActor2D();
  ~vtkActor2D();

  static vtkActor2D* New() {return new vtkActor2D;};

  const char *GetClassName() {return "vtkActor2D";};
  vtkActor2D& operator=(const vtkActor2D& actor);

  void Render(vtkViewport *viewport);

  vtkSetObjectMacro(Mapper, vtkMapper2D);

  vtkSetVector2Macro(Scale, float);
  vtkGetVectorMacro(Scale, float, 2);

  vtkSetMacro(Orientation, float);
  vtkGetMacro(Orientation, float);

  vtkSetMacro(LayerNumber, int);
  vtkGetMacro(LayerNumber, int);

  vtkSetMacro(Visibility, int);
  vtkGetMacro(Visibility, int);
  vtkBooleanMacro(Visibility, int);

  vtkProperty2D* GetProperty();
  vtkSetObjectMacro(Property, vtkProperty2D);

  vtkGetVectorMacro(ViewPosition, float, 2);
  void SetViewPosition(float XPos, float YPos);
  void SetViewPosition(float arr[2]) {this->SetViewPosition(arr[0], arr[1]);};

  vtkGetVectorMacro(DisplayPosition, int, 2);
  void SetDisplayPosition(int XPos, int YPos);
  void SetDisplayPosition(int arr[2]) {this->SetDisplayPosition(arr[0], arr[1]);};

  vtkGetVectorMacro(WorldPosition, float, 3);
  void SetWorldPosition(float XPos, float YPos, float ZPos);
  void SetWorldPosition(float arr[3]) {this->SetWorldPosition(arr[0], arr[1], arr[2]);};

  int *GetComputedDisplayPosition(vtkViewport* viewport);

  vtkSetMacro(PositionType, int);
  vtkGetMacro(PositionType, int);

  void SetPositionTypeToView() {this->SetPositionType(VTK_VIEW_COORD);};
  void SetPositionTypeToDisplay() {this->SetPositionType(VTK_DISPLAY_COORD);};
  void SetPositionTypeToWorld() {this->SetPositionType(VTK_WORLD_COORD);};
  
protected:

  float Orientation;
  float Scale[2];

  float ViewPosition[2];
  int   DisplayPosition[2];
  float   WorldPosition[3];
  int   ComputedDisplayPosition[2];

  int   PositionType;

  int LayerNumber;
  int Visibility;
  int SelfCreatedProperty;
  vtkTransform Transform;

  vtkProperty2D *Property;
  vtkMapper2D *Mapper;

};

#endif


