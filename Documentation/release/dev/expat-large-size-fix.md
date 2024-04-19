## Fix for reading binary XML files > 2Gb on Windows

Fix an error encountered when reading binary format XML files
on Windows - a missing define for Expat caused the offset used reading
the position in the file to be limited to 2Gb. Fix the define,
and add a check when using external Expat that the necessary feature,
`XML_LARGE_SIZE`, is enabled.
