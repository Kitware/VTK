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

void vtkProperty2D::PrintSelf(ostream& os, vtkIndent indent)
{

  this->vtkObject::PrintSelf(os, indent);

  os << indent << "Opacity: " << this->Opacity << "\n";
  os << indent << "Color: (" << this->Color[0] << ", "
			     << this->Color[1] << ", "
			     << this->Color[2] << ")\n";

  char compString[100];

  switch (this->CompositingOperator)
    {
    case VTK_BLACK:
	strcpy(compString, "VTK_BLACK\0");
	break;
    case VTK_NOT_DEST:
        strcpy(compString, "VTK_NOT_DEST\0");
 	break;
    case VTK_SRC_AND_DEST:
	strcpy(compString, "VTK_SRC_AND_DEST\0");
	break;
    case VTK_SRC_OR_DEST:
	strcpy(compString, "VTK_SRC_OR_DEST\0");
	break;
    case VTK_NOT_SRC:
	strcpy(compString, "VTK_NOT_SRC\0");
	break;
    case VTK_SRC_XOR_DEST:
	strcpy(compString, "VTK_SRC_XOR_DEST\0");
	break;
    case VTK_SRC_AND_notDEST:
	strcpy(compString, "VTK_SRC_AND_notDEST\0");
	break;
    case VTK_SRC:
	strcpy(compString, "VTK_SRC\0");
	break;
    case VTK_WHITE:
 	strcpy(compString, "VTK_WHITE\0");
	break;
    default:
	strcpy(compString, "UNKNOWN!\0");
	break;
    }

  os << indent << "Compositing Operator: " << compString << "\n";
}




