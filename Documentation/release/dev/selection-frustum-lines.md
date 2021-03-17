## Frustum-selection of lines and poly-lines

A bug in frustum selection of lines and poly-lines has been fixed.
Previously, lines and polylines were treated the same
as polygons — which lead to their being selected when
the frustum covered their interior area but not any
portion of the line itself (i.e., selection would pick
the line when it should not). Now, Cyrus-Beck clipping is used
to obtain the correct result.
