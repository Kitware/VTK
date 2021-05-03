## Export additional data types to the Java wrapper classes

Byte, short, long and float data types have been added to the Java wrapper code in addition to the numeric data types previously exported; int and double. Also all forms of any overloaded functions in the VTK public API, containing the same arguments but different type declarations, are now output so that the generated Java classes more closely match their C++ counterparts.
