## Faster wrapper generation for large projects

The wrappers use a cache to keep track of which header files exist on the
system, in order to avoid calling the stat() function repeatedly during
header file searches.  This significantly accelerates the generation of
wrappers when there is a large number of include directories to search,
especially on Windows.
