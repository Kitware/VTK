#include "vtkWrapped.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkWrapped);

vtkWrapped::vtkWrapped() = default;

vtkWrapped::~vtkWrapped() = default;

void vtkWrapped::PrintSelf(std::ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

std::string vtkWrapped::GetString() const
{
  return "wrapped";
}
