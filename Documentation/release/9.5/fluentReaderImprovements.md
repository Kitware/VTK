# Add zone section selection to FLUENT Reader

You can now select the zone sections you wish to load when reading a FLUENT file.
Therefore, the output multiblock will only contain the selected zones.

You can now choose to cache or not the data using the CacheData option. This allows to avoid re-parsing the file to gain IO performances at the expense of a greater memory cost, or the other way around.
Because of zone sections interdependency in the FLUENT format, some unselected zone sections may still need to be read from the file, even if they are not part of the output multiblock.
Here is the general file parsing logic:
- If any cell zone is enabled, the whole file needs to be read
- Otherwise, only the necessary zones are read (nodes, faces, data arrays,...)
  Therefore, unselecting a zone will not always improve the file's reading time, but will lower the output' size.
