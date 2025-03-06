// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataSetAttributes.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkUnsignedCharArray.h"

int TestDataSetAttributes(int, char*[])
{
  int retVal = EXIT_SUCCESS;
  {
    // We test if vtkDataSetAttributes skips
    constexpr int EXT = 10;
    constexpr int N = EXT * EXT * EXT;
    const int EXTENT[] = { 0, EXT - 1, 0, EXT - 1, 0, EXT - 1 };
    constexpr int GHOST_INDICES[] = { 3, 15, 30, 40, -1 };

    auto makeGhostArray = [&](unsigned char ghostType)
    {
      vtkNew<vtkUnsignedCharArray> ghosts;
      ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
      ghosts->SetNumberOfValues(N);
      ghosts->FillValue(0);
      int i = 0;
      while (GHOST_INDICES[i] != -1)
      {
        ghosts->SetValue(GHOST_INDICES[i], ghostType);
        ++i;
      }
      return ghosts;
    };

    auto makeFD = [&](unsigned char ghostType)
    {
      vtkNew<vtkDataSetAttributes> fd;
      fd->AddArray(makeGhostArray(ghostType));
      return fd;
    };

    auto sourcePD =
      makeFD(vtkDataSetAttributes::DUPLICATEPOINT | vtkDataSetAttributes::HIDDENPOINT);
    auto sourceCD = makeFD(vtkDataSetAttributes::DUPLICATECELL | vtkDataSetAttributes::REFINEDCELL);
    auto refPD = makeFD(vtkDataSetAttributes::HIDDENPOINT);
    auto refCD = makeFD(vtkDataSetAttributes::REFINEDCELL);
    vtkNew<vtkDataSetAttributes> destPD, destCD;

    auto testCopyStructuredData = [&](vtkDataSetAttributes* from, vtkDataSetAttributes* to,
                                    vtkDataSetAttributes* ref, unsigned char ghostType)
    {
      to->CopyAllocate(ref);
      // Copying multiple arrays sharing ghosts should have DUPLICATEPOINT or DUPLICATECELL
      // disappear from the output (bit is turned OFF by ref)
      to->CopyStructuredData(from, EXTENT, EXTENT);
      to->CopyStructuredData(ref, EXTENT, EXTENT);
      int i = 0;
      auto ghosts = vtkArrayDownCast<vtkUnsignedCharArray>(
        to->GetAbstractArray(vtkDataSetAttributes::GhostArrayName()));
      for (int id = 0; id < N; ++id)
      {
        if ((GHOST_INDICES[i] == id && ghosts->GetValue(id) != ghostType) ||
          (GHOST_INDICES[i] != id && ghosts->GetValue(id)))
        {
          std::cout << "VAL " << (int)ghosts->GetValue(id) << " -- " << (int)ghostType << std::endl;
          vtkLog(ERROR, "Ghost values are not properly copied in CopyStructuredData.");
          return false;
        }
        i += GHOST_INDICES[i] == id;
      }
      return true;
    };

    if (!testCopyStructuredData(sourcePD, destPD, refPD, vtkDataSetAttributes::HIDDENPOINT) ||
      !testCopyStructuredData(sourceCD, destCD, refCD, vtkDataSetAttributes::REFINEDCELL))
    {
      retVal = EXIT_FAILURE;
    }
  }

  return retVal;
}
