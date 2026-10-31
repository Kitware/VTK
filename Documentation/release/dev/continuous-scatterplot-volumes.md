## vtkContinuousScatterplot computes correct fragment volumes

`vtkContinuousScatterplot` now builds every fragment as a closed polyhedron
with its faces wound outward, so the volume in each bin is the volume of the
domain where the two fields fall in that bin, and the bins add up to the
volume of the domain. Before, two of the faces of every tetrahedron were wound
inward, each cut face had the same winding for the fragment and the residual,
and the points of a cut face could be ordered so that the face crossed itself.
The bins held several times the volume of the domain, unevenly, so the
continuous scatterplots change. Each slice of a field also goes in its own
bin now; rounding could put a slice in the bin next to it.
