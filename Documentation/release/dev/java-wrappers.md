## Improve string handling performance of java wrappers

The wrapper code has been updated to make conversion of Java to C++ strings more efficient.
Converting java strings to utf8 byte arrays (and vice versa) is now handled on the Java side. This requires a minimum Java version of 1.7 for compiling the wrappers.

Note: The public API of the java wrapper classes is unaltered, so there are no backward compatability issues for downstream users.
