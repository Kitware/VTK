#include "vtkProperty2D.h"

vtkProperty2D::vtkProperty2D()
{
  this->Opacity = 1.0;
  this->Color[0] = 1.0;
  this->Color[1] = 0.0;
  this->Color[2] = 0.0;  
  this->CompositingOperator = VTK_SRC;
}

vtkProperty2D::~vtkProperty2D()
{

}






