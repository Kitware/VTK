#ifndef vtkWrappable_h
#define vtkWrappable_h

#include "vtkWrappableModule.h" // for export macro

#include "vtkObject.h"

#include <string> // for std::string

class VTKWRAPPABLE_EXPORT vtkWrappable : public vtkObject
{
public:
  static vtkWrappable* New();
  vtkTypeMacro(vtkWrappable, vtkObject);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  std::string GetString() const;

protected:
  vtkWrappable();
  ~vtkWrappable() override;

private:
  vtkWrappable(const vtkWrappable&) = delete;
  void operator=(const vtkWrappable&) = delete;
};

#endif
