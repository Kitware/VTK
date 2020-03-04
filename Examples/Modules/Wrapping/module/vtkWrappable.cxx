#include "vtkWrappable.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkWrappable);

vtkWrappable::vtkWrappable() {}

vtkWrappable::~vtkWrappable() {}

void vtkWrappable::PrintSelf(std::ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

std::string vtkWrappable::GetString() const
{
  return "wrapped";
}
