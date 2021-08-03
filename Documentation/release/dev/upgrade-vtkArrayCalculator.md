#Upgrade vtkArrayCalculator

The goal of this merge request is to upgrade the vtkArrayCalculator filter to:
* use c++ containers
* improve functionality
* improve performance
* solve known issues https://gitlab.kitware.com/paraview/paraview/-/issues/20030, https://gitlab.kitware.com/paraview/paraview/-/issues/20602

1) Containers: Raw pointers have been replaced with std::vector/std::string which reduces the LOC, and simplifies the code
   1) This is a public API change which might affect users of the vtkArrayCalculator.
2) Functionality: A new function parser (wrapper to be more exact) has been developed using the [ExprTk library](https://github.com/ArashPartow/exprtk). This library is the best for this purpose, and includes a lot of extra functionality compared to the ``vtkFunctionParser``. The only limitation of this library is the fact that you can't define a function that returns a vector. Therefore to implement norm, and cross, regular expression tricks have been employed. These tricks come with a specific limitation, i.e., the input vector of this function can't be a sub-expression that includes '(', ')' or ',', e.g. norm(cross(v1,v2)).
3) Performance: vtkSMPTools have been utilized to parallelize this filter by creating multiple parser instances for each thread
4) Known issues: An input variable name must either be sanitized or enclosed in quotes

It should be noted that the user can select if he wants to use the ``vtkFunctionParser`` or the new ``vtkExprTkFunctionParser`` (default).
