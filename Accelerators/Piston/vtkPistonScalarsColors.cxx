
#include "vtkPistonScalarsColors.h"

#include <vtkObjectFactory.h>
#include <vtkScalarsToColors.h>

#include <vector>

vtkStandardNewMacro(vtkPistonScalarsColors);

vtkCxxSetObjectMacro(vtkPistonScalarsColors, LookupTable, vtkScalarsToColors);

//-----------------------------------------------------------------------------
vtkPistonScalarsColors::vtkPistonScalarsColors() : vtkObject(),
  NumberOfValues(256),
  LookupTable(0)
{
  this->TableRange[0] = this->TableRange[1] = 0.0;
  this->ComputeColorsTime.Modified();
  this->ComputeColorsfTime.Modified();
}

//-----------------------------------------------------------------------------
vtkPistonScalarsColors::~vtkPistonScalarsColors()
{
  this->SetLookupTable(0);
}

//-----------------------------------------------------------------------------
void vtkPistonScalarsColors::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TableRange: " << this->TableRange[0]
     << indent << this->TableRange[1] << endl;
  os << indent << "NumberOfValues: " << this->NumberOfValues << endl;

  os << indent << "ComputeColorsTime: "
     << this->ComputeColorsTime.GetMTime() << endl;
  os << indent << "ScalarsColors: " << &this->ScalarsColors << "\n";

  os << indent << "ComputerColorsfTime: "
     << this->ComputeColorsfTime.GetMTime() << endl;
  os << indent << "ScalarsColorsf: " << &this->ScalarsColorsf << "\n";
}

//-----------------------------------------------------------------------------
void vtkPistonScalarsColors::SetTableRange(double range[2])
{
  this->SetTableRange(range[1], range[2]);
}

//-----------------------------------------------------------------------------
void vtkPistonScalarsColors::SetTableRange(double rmin, double rmax)
{
  if (rmax < rmin)
    {
    vtkErrorMacro("Bad table range: ["<<rmin<<", "<<rmax<<"]");
    return;
    }

  if (this->TableRange[0] == rmin && this->TableRange[1] == rmax)
    {
    return;
    }

  this->TableRange[0] = rmin;
  this->TableRange[1] = rmax;

  this->Modified();
}

//-----------------------------------------------------------------------------
std::vector<unsigned char>* vtkPistonScalarsColors::ComputeScalarsColors(
  int numberOfChanels)
{
  if(!this->LookupTable)
    {
    vtkErrorMacro(<< "Invalid look up table");
    return NULL;
    }

  if(numberOfChanels < 1)
    {
    vtkErrorMacro(<< "Cannot have less than one chanel");
    return NULL;
    }

  if(numberOfChanels > VTK_RGBA)
    {
    vtkErrorMacro(<< "Cannot have more than four (RGBA) chanels");
    return NULL;
    }

  if(!(this->LookupTable->GetMTime() > this->GetMTime() ||
      this->ComputeColorsTime.GetMTime() < this->GetMTime()))
    {
    return &this->ScalarsColors;
    }

  std::vector<float> values (this->NumberOfValues);
  float *valueptr = &values[0];
  this->ComputeValues(valueptr);

  // Point to the first element
  valueptr = &values[0];

  // Colors for those values;
  this->ScalarsColors.clear();

  this->ScalarsColors.resize(this->NumberOfValues * numberOfChanels);
  unsigned char *colorptr = &this->ScalarsColors[0];

  this->LookupTable->SetRange(this->TableRange);
  this->LookupTable->Build();
  this->LookupTable->MapScalarsThroughTable(valueptr, colorptr,
    VTK_FLOAT, this->NumberOfValues, 1, numberOfChanels);

  this->Modified();

  // Now update build time (should be done last)
  this->ComputeColorsTime.Modified();

  return &this->ScalarsColors;
}

//-----------------------------------------------------------------------------
std::vector<float>* vtkPistonScalarsColors::ComputeScalarsColorsf(
  int numberOfChanels)
{
  if(!this->LookupTable)
    {
    vtkErrorMacro(<< "Invalid look up table");
    return NULL;
    }

  if(numberOfChanels < 1)
    {
    vtkErrorMacro(<< "Cannot have less than one chanel");
    return NULL;
    }

  if(numberOfChanels > VTK_RGBA)
    {
    vtkErrorMacro(<< "Cannot have more than four (RGBA) chanels");
    return NULL;
    }

  if(!(this->LookupTable->GetMTime() > this->GetMTime() ||
      this->ComputeColorsfTime.GetMTime() < this->GetMTime()))
    {
    return &this->ScalarsColorsf;
    }

  std::vector<float> values (this->NumberOfValues);
  float *valueptr = &values[0];
  this->ComputeValues(valueptr);

  // Point to the first element
  valueptr = &values[0];

  // Colors for those values;
  this->ScalarsColorsf.clear();

  unsigned char *scalarColors =
    new unsigned char[this->NumberOfValues * numberOfChanels];
  unsigned char *colorptr = scalarColors;

  this->LookupTable->SetRange(this->TableRange);
  this->LookupTable->Build();
  this->LookupTable->MapScalarsThroughTable(valueptr, colorptr,
    VTK_FLOAT, this->NumberOfValues, 1, numberOfChanels);

  // Convert unsigned char color to float color
  this->ScalarsColorsf.resize(this->NumberOfValues * 3);
  for (int i = 0, j = 0; i < this->NumberOfValues; ++i, j += 3)
    {
      float r = (float)colorptr[i*3+0] / 256.0;
      float g = (float)colorptr[i*3+1] / 256.0;
      float b = (float)colorptr[i*3+2] / 256.0;

      this->ScalarsColorsf[j] = r;
      this->ScalarsColorsf[j+1] = g;
      this->ScalarsColorsf[j+2] = b;
    }

  delete [] scalarColors;

  this->Modified();

  // Now update build time (should be done last)
  this->ComputeColorsfTime.Modified();

  return &this->ScalarsColorsf;
}

//-----------------------------------------------------------------------------
void vtkPistonScalarsColors::ComputeValues(float *values)
{
  if(!values)
    {
    return;
    }

  for (int i = 0; i < this->NumberOfValues; ++i)
    {
    *values = this->TableRange[0] +
      i * ((this->TableRange[1] - this->TableRange[0]) /
           (float) this->NumberOfValues);
    values++;
    }
}
