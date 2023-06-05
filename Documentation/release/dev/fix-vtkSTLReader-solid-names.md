## Fix IO Geometry STL Reader

Delimiting newlines should be preserved when the solid does not have name with it.

* Current code consumes delimiting newline so that the line for a solid without name is silently removed from header
* As a result, GetHeader() returns existing names only
* We should keep a blank line for the solid without name to map the name to STLSolidLabeling scalar value
