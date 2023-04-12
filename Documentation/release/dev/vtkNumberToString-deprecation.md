## vtkNumberToString operator() deprecation

In order to introduce more flexibility in vtkNumberToString,
the vtkNumberToString () operator has been deprecated and
the usage as change.

Previous usage:

```
vtkNumberToString convert;
std::cout<<convert(val)<<std::endl;
```

New usage:

```
vtkNumberToString converter;
std::cout<<converter.Convert(val)<<std::endl;
```

## vtkNumberToString lowExponent and highExponent parameters

vtkNumberToString now lets users specify a lowExponent and highExponent in
order to specify when scientific or fixed notation should be used.

eg:

```
vtkNumberToString converter;
converter.SetHighExponent(6);
std::cout<<converter.Convert(1e7)<<std::endl;
converter.SetHighExponent(7);
std::cout<<converter.Convert(1e7)<<std::endl;
converter.SetLowExponent(-6);
std::cout<<converter.Convert(1e-7)<<std::endl;
converter.SetLowExponent(-7);
std::cout<<converter.Convert(1e-7)<<std::endl;
```

Will output:

```
1e7
10000000
1e-7
0.0000001
```
