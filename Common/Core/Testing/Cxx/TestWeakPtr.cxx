/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestWeakPtr.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkObjectBase.h"
#include "vtkWeakPtr.h"

#include <iostream>

#include <cstdlib>

#define ERROR(...) std::cerr << "ERROR: " __VA_ARGS__ << std::endl

int TestWeakPtr(int, char*[])
{
  // Test default construction.
  {
    vtkWeakPtr<vtkObject> weak;
    if (!weak.Expired())
    {
      ERROR("default construction is not expired");
    }
  }

  // Test pointer construction and assignment.
  {
    vtkNew<vtkObject> obj;

    vtkWeakPtr<vtkObject> weak(obj);
    if (weak.Expired())
    {
      ERROR("ptr construction is expired");
    }

    vtkWeakPtr<vtkObjectBase> weak_base(obj);
    if (weak_base.Expired())
    {
      ERROR("ptr construction (derived) is expired");
    }
  }

  // Test copy construction and assignment.
  {
    vtkNew<vtkObject> obj;

    vtkWeakPtr<vtkObject> weak1(obj);
    vtkWeakPtr<vtkObject> weak(weak1);
    if (weak.Expired())
    {
      ERROR("ptr copy construction is expired");
    }

    vtkWeakPtr<vtkObject> weak2;
    weak = weak2;
    if (!weak.Expired())
    {
      ERROR("default copy assignment is not expired");
    }

    vtkWeakPtr<vtkObjectBase> weak_base(obj);
    if (weak_base.Expired())
    {
      ERROR("ptr copy construction (derived) is expired");
    }

    weak_base = weak1;
    if (weak_base.Expired())
    {
      ERROR("copy assignment (derived) is not expired");
    }

    weak_base = weak2;
    if (!weak_base.Expired())
    {
      ERROR("ptr copy assignment (derived) is not expired");
    }
  }

  // Test move construction and assignment.
  {
    vtkNew<vtkObject> obj;

    vtkWeakPtr<vtkObject> weak1(obj);
    vtkWeakPtr<vtkObject> weak(std::move(weak1));
    if (weak.Expired())
    {
      ERROR("ptr move construction is expired");
    }
    if (!weak1.Expired()) // NOLINT(bugprone-use-after-move)
    {
      ERROR("move-from (construction) is not expired");
    }

    vtkWeakPtr<vtkObject> weak2;
    weak = std::move(weak2);
    if (!weak.Expired())
    {
      ERROR("move assignment is not expired");
    }
    if (!weak2.Expired()) // NOLINT(bugprone-use-after-move)
    {
      ERROR("move-from (assignment) is not expired");
    }

    weak1 = obj;
    vtkWeakPtr<vtkObjectBase> weak_base(std::move(weak1));
    if (weak_base.Expired())
    {
      ERROR("move construction (derived) is expired");
    }
    if (!weak1.Expired()) // NOLINT(bugprone-use-after-move)
    {
      ERROR("move-from (derived construction) is not expired");
    }

    weak_base = std::move(weak);
    if (!weak_base.Expired())
    {
      ERROR("move assignment (derived) is not expired");
    }
  }

  // Test comparisons.
  {
    vtkNew<vtkObject> obj1;
    vtkNew<vtkObject> obj2;

    vtkWeakPtr<vtkObject> weak0a;
    vtkWeakPtr<vtkObject> weak0b;
    vtkWeakPtr<vtkObject> weak1a(obj1);
    vtkWeakPtr<vtkObject> weak1b(obj1);
    vtkWeakPtr<vtkObject> weak2(obj2);

    if (weak0a.owner_before(weak0a))
    {
      ERROR("default constructed is truthy for `weak0a 'before' weak0a`");
    }
    if (weak0a.owner_before(weak0b) || weak0b.owner_before(weak0a))
    {
      ERROR("default constructed is truthy for `weak0a 'before' weak0b` (or vice versa)");
    }

    if (weak1a.owner_before(weak1a))
    {
      ERROR("ptr constructed is truthy for `weak1a 'before' weak1a`");
    }

    if (weak1a.owner_before(weak1b) || weak1b.owner_before(weak1a))
    {
      ERROR("ptr constructed is false-y for `weak1a == weak1b`");
    }

    if (!weak1a.owner_before(weak2) && !weak2.owner_before(weak1a))
    {
      ERROR("ptr constructed is truthy for `weak1a == weak2`");
    }
  }

  // Test `Lock`
  {
    vtkWeakPtr<vtkObject> weak;

    if (auto ptr = weak.Lock())
    {
      ERROR("default constructed gave a non-`nullptr` for `Lock`");
    }

    {
      vtkNew<vtkObject> obj;
      weak = obj;

      if (auto ptr = weak.Lock())
      {
        if (ptr != obj)
        {
          ERROR("ptr assignment gave the wrong value for `Lock`");
        }
      }
      else
      {
        ERROR("ptr assignment gave a `nullptr` for `Lock`");
      }
    }

    if (!weak.Expired())
    {
      ERROR("ptr assignment to a deleted object is not expired");
    }
  }

  return EXIT_SUCCESS;
}
