#include "vtkProperty2D.h"

vtkProperty2D::vtkProperty2D()
{

  vtkDebugMacro(<<"vtkProperty2D::vtkProperty2D");

  // Find out how color and opacity are defaulted
  this->Opacity = 1.0; 

  this->Color[0] = 1.0;
  this->Color[1] = 0.0;
  this->Color[2] = 0.0;  

  this->Operator = VTK_SRC;

}

vtkProperty2D::~vtkProperty2D()
{
  vtkDebugMacro(<<"vtkProperty2D::~vtkProperty2D");
}

void vtkProperty2D::SetColor(float r, float g, float b)
{
  vtkDebugMacro(<<"vtkProperty2D::SetColor");

  this->Color[0] = r;
  this->Color[1] = g;
  this->Color[2] = b;
  this->Modified();
}

void vtkProperty2D::SetColor(float c[3])
{
  vtkDebugMacro(<<"vtkProperty2D::SetColor");

  this->Color[0] = c[0];
  this->Color[1] = c[1];
  this->Color[2] = c[2];
  this->Modified();

}

float* vtkProperty2D::GetColor()
{
  vtkDebugMacro(<<"vtkProperty2D::GetColor");

  return this->Color;
}

void vtkProperty2D::GetColor(float c[3])
{
  vtkDebugMacro(<<"vtkProperty2D::GetColor");

  c[0] = this->Color[0];
  c[1] = this->Color[1];
  c[2] = this->Color[2];

}

void vtkProperty2D::SetCompositingOperator(int op)
{
  vtkDebugMacro(<<"vtkProperty2D::SetCompositingOperator");

  this->Operator = op;
  this->Modified();

}

int vtkProperty2D::GetCompositingOperator()
{
  vtkDebugMacro(<<"vtkProperty2D::GetCompositingOperator");

  return this->Operator;
}








