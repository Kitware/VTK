#ifndef vtkWrapped_h
#define vtkWrapped_h

#include "vtkWrappableModule.h" // for export macro

#include "vtkObject.h"

#include <string> // for std::string

class VTKWRAPPABLE_EXPORT vtkWrapped : public vtkObject
{
public:
  static vtkWrapped* New();
  vtkTypeMacro(vtkWrapped, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  std::string GetString() const;

protected:
  vtkWrapped();
  ~vtkWrapped() override;

private:
  vtkWrapped(const vtkWrapped&) = delete;
  void operator=(const vtkWrapped&) = delete;
};

#endif
