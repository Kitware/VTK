

#ifndef __vtkProperty2D_h
#define __vtkProperty2D_h

#include "vtkObject.h"

#define VTK_BLACK	        0   // BLACKNESS   // R2_BLACK
#define VTK_NOT_DEST		1   // DSTINVERT   // R2_NOT
#define VTK_SRC_AND_DEST	2   // SRCAND      // R2_MASKPEN
#define VTK_SRC_OR_DEST		3   // SRCPAINT    // R2_MERGEPEN
#define VTK_NOT_SRC		4   // NOTSRCCOPY  // R2_NOTCOPYPEN
#define VTK_SRC_XOR_DEST	5   // SRCINVERT   // R2_XORPEN
#define VTK_SRC_AND_notDEST	6   // SRCERASE    // R2_MERGEPENNOT
#define VTK_SRC			7   // SRCCOPY    // R2_COPYPEN
#define VTK_WHITE		8   // WHITENESS   // R2_WHITE

class vtkViewport;

class VTK_EXPORT vtkProperty2D : public vtkObject
{
public:

  vtkProperty2D();
  ~vtkProperty2D();
  static vtkProperty2D *New() {return new vtkProperty2D;};

  vtkGetMacro(Opacity, float);
  vtkSetMacro(Opacity, float);

  void SetColor(float r, float g, float b);
  void SetColor(float c[3]);
  float* GetColor();
  void GetColor(float c[3]);

  void SetCompositingOperator(int); 
  int GetCompositingOperator();
  void SetCompositingOperatorToBlack() {this->Operator = VTK_BLACK;};
  void SetCompositingOperatorToNotDest() {this->Operator = VTK_NOT_DEST;};
  void SetCompositingOperatorToSrcAndDest() {this->Operator = VTK_SRC_AND_DEST;};
  void SetCompositingOperatorToSrcOrDest() {this->Operator = VTK_SRC_OR_DEST;};
  void SetCompositingOperatorToNotSrc() {this->Operator = VTK_NOT_SRC;};
  void SetCompositingOperatorToSrcXorDest() {this->Operator = VTK_SRC_XOR_DEST;};
  void SetCompositingOperatorToSrcAndNotDest() {this->Operator = VTK_SRC_AND_notDEST;};
  void SetCompositingOperatorToSrc() {this->Operator = VTK_SRC;};
  void SetCompositingOperatorToWhite() {this->Operator = VTK_WHITE;};

  void Render (vtkViewport* viewport)  { viewport;}
  


protected:
  float Opacity;
  float Color[3];
  int Operator;
};
  
  
#endif


