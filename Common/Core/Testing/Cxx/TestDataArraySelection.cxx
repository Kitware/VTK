#include "vtkDataArraySelection.h"
#include "vtkNew.h"

#define TASSERT(x)                                                                                 \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "ERROR: failed at " << __LINE__ << "!" << endl; /*return EXIT_FAILURE;*/               \
  }

int TestDataArraySelection(int, char*[])
{
  vtkNew<vtkDataArraySelection> sel;
  sel->EnableArray("Temperature");
  sel->EnableArray("Pressure");
  sel->DisableArray("Pressure");
  sel->Print(cout);

  TASSERT(sel->ArrayExists("Temperature") && sel->ArrayIsEnabled("Temperature"));
  TASSERT(!sel->ArrayExists("Temperature2") && !sel->ArrayIsEnabled("Temperature2"));
  TASSERT(sel->ArrayExists("Pressure") && !sel->ArrayIsEnabled("Pressure"));

  vtkNew<vtkDataArraySelection> sel2;
  sel2->EnableArray("Pressure");
  sel2->EnableArray("Voltage");
  sel2->Union(sel);
  sel2->Print(cout);

  TASSERT(sel2->ArrayExists("Temperature") && sel2->ArrayIsEnabled("Temperature"));
  TASSERT(!sel2->ArrayExists("Temperature2") && !sel2->ArrayIsEnabled("Temperature2"));
  TASSERT(sel2->ArrayExists("Pressure") && sel2->ArrayIsEnabled("Pressure"));
  TASSERT(sel2->ArrayExists("Voltage") && sel2->ArrayIsEnabled("Voltage"));
  return EXIT_SUCCESS;
}
