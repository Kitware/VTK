// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSurfaceNets3DNonManifoldCases
 * @brief   Provides utilities to identify vtkSurfaceNets3D's edge-cases that can lead to
 * non-manifold output geometry, and to resolve them by creating duplicate points.
 *
 * @section Introduction
 *
 * vtkSurfaceNets3D performs dual contouring on a 3D segmented multi-material image to generate a
 * surface mesh using the Flying Edges approach. As the name implies, the Flying Edges approach goes
 * through the edges of the input grid, instead of the voxels, to determine how the output geometry
 * should be generated.
 *
 * Assuming you are at the center of a 2x2x2 grid of voxels, an edge case has 12 bits describing
 * which of the 12 possible faces between the voxels will be generated or not. A face is generated
 * if a voxel either does not have a neighboring voxel in a particular direction (i.e., it has a
 * voxel with a background label), or if it has a neighboring voxel with a different label. In the
 * latter case, the generated face is shared by the two voxels, and is generated only once.
 *
 * In contrast, the more traditional Marching Cubes performs contouring (not dual) and goes through
 * the voxels of the input grid. As the name implies, it generates voxel cases, instead of edge
 * cases, to determine how to generate the output geometry.
 *
 * Assuming you are at the center of a 2x2x2 grid of voxels, a voxel case has 8 bits describing
 * which of the 8 voxels are present in the case (i.e., have a non-background label).
 *
 * Both the flying edges approach used by SurfaceNets and the marching cubes approach can lead to
 * non-manifold output geometry for certain configurations of voxels.
 *
 * @section Manifold Surface Meshes
 *
 * A manifold surface mesh is one where every edge is shared by exactly two faces and every vertex
 * has a single connected ring of faces around it, meaning the mesh locally resembles a flat disk
 * at every point, with no holes, gaps, or self-intersections.
 *
 * An open-manifold surface mesh relaxes this definition by allowing boundary edges — edges shared
 * by only one face. It locally resembles a flat disk at interior vertices and a half-disk at
 * boundary vertices.
 *
 * Since vtkSurfaceNets3D does not cap boundary faces, voxels at the image boundary generate
 * boundary edges, so the resulting surface mesh cannot be strictly manifold. The best we can do
 * is attempt to improve local topology (towards open-manifold behavior) in regions where the local
 * voxel/material configuration is solvable.
 *
 * Furthermore, since vtkSurfaceNets3D generates faces shared between voxels of different
 * non-background labels, any open-manifold objective is meaningful only per label region: if you
 * extract the faces corresponding to a single label from the output mesh, that sub-mesh is what we
 * try to keep open-manifold where possible. When a configuration cannot be resolved, this class
 * reports it as such and vtkSurfaceNets3D may still produce non-manifold output.
 *
 * @section Non-manifold cases
 *
 * In "Parallel hexahedral meshing from volume fractions", Steven J. Owen, Matthew L. Staten,
 * Marguerite C. Sorensen (https://doi.org/10.1007/s00366-012-0292-8), seven canonical voxel
 * configurations (ignoring different orientations) are described for a 2×2×2 grid of voxels,
 * which can lead to generating a non-manifold surface mesh.
 *
 * Before describing these cases, let's first provide some mathematical background necessary to
 * understand them and compute all their various rotated/reflected variants.
 *
 * We work in a 3D binary grid: {0,1}^3
 * → There are exactly 8 possible coordinates: (x,y,z) with x,y,z ∈ {0,1}.
 * → Each coordinate triple represents the position of a voxel in a 2×2×2 cube.
 *
 * Hamming distance definition:
 * d_H(a,b) = number of coordinates where a != b.
 * - d_H = 1 → voxels share a face.
 * - d_H = 2 → voxels touch at an edge or vertex (no shared face).
 * - d_H = 3 → voxels are opposite corners of the cube.
 *
 * For k chosen voxels:
 * - There are C(k,2) voxel pairs.
 * - We compute all pairwise Hamming distances.
 * - Sort them → get a "distance signature".
 *
 * The sorted distance signature is permutation-invariant and acts as a
 * shape descriptor in the binary cube space.
 * Each non-manifold case (case1...case11) is defined by a unique signature:
 *
 * --------------------------------------------------------------------
 * Case 1: Two voxels are at opposite corners (body diagonal).
 *         Example: voxels at (0,0,0) and (1,1,1).
 *
 *   Pseudo-3D view:
 *       · ·
 *       · ■    (back layer z=1)
 *   ■ ·        (front layer z=0)
 *   · ·
 *
 *   # Voxel-configurations in the binary grid: 4.
 *   Reason: 2 voxels, signature [3]
 * --------------------------------------------------------------------
 * Case 2: Two voxels are diagonally adjacent across a face (no shared edge).
 *         Example: voxels at (0,0,0) and (1,0,1).
 *
 *   Pseudo-3D view:
 *       · ·
 *       ■ ·
 *   ■ ·
 *   · ·
 *
 *   # Voxel-configurations  in the binary grid =  12.
 *   Reason: 2 voxels, signature [2]
 * --------------------------------------------------------------------
 * Case 3: Three voxels where, one pair shares a face, one pair shares only a vertex,
 *         and one pair is opposite corners. Shape looks like an "L" but spans entire cube.
 *         Example: voxels at (0,0,0), (1,0,0), and (1,1,1).
 *
 *   Pseudo-3D view:
 *       · ·
 *       · ■
 *   ■ ■
 *   · ·
 *
 *   # Voxel-configurations in the binary grid = 24.
 *   Reason: 3 voxels, signature [1, 2, 3]
 * --------------------------------------------------------------------
 * Case 4: Three voxels arranged so that all three share a common edge,
 *         but no common face, i.e. Forms an equilateral triangle in the cube.
 *         Example: voxels at (0,0,1), (1,0,0), and (0,1,0).
 *
 *   Pseudo-3D view:
 *       ■ ·
 *       · ·
 *   · ■
 *   ■ ·
 *
 *   # Voxel-configurations in the binary grid = 8.
 *   Reason: 3 voxels, signature [2, 2, 2]
 * --------------------------------------------------------------------
 * Case 5: Four voxels with two face-adjacent pairs, two vertex-adjacent pairs,
 *         and two body diagonals. Shape is like a bent rectangle spanning the cube.
 *         Example: voxels at (0,0,0), (0,0,1), (1,1,0), and (1,1,1).
 *
 *   Pseudo-3D view:
 *       ■ ·
 *       · ■
 *   ■ ·
 *   · ■
 *
 *   # Voxel-configurations in the binary grid = 6.
 *   Reason: 4 voxels, signature [1, 1, 2, 2, 3, 3]
 * --------------------------------------------------------------------
 * Case 6: Four voxels where three lie in the same horizontal layer and are
 *         face-adjacent in an L-shape, and the fourth is above one corner,
 *         touching the others only along edges.
 *         Example: voxels at (0,0,1), (1,0,0), (0,1,0), and (1,1,0).
 *
 *   Pseudo-3D view:
 *       ■ ·
 *       · ·
 *   · ■
 *   ■ ■
 *
 *   # Voxel-configurations =  24.
 *   Reason: 4 voxels, signature [1, 1, 2, 2, 2, 3]
 * --------------------------------------------------------------------
 * Case 7: Four voxels placed so each shares exactly three edges with the others,
 *         forming a twisted diagonal pattern through the grid.
 *         Example: voxels at (0,0,1), (1,0,0), (0,1,0), and (1,1,1).
 *
 *   Pseudo-3D view:
 *       ■ ·
 *       · ■
 *   · ■
 *   ■ ·
 *
 *   # Voxel-configurations =  2.
 *   Reason: 4 voxels, signature [2, 2, 2, 2, 2, 2]
 * --------------------------------------------------------------------
 *
 * We also have the complementary cases of non-manifold cases 1-4,
 * which are not mentioned in the paper but can be derived by inverting
 * the binary grid (i.e., swapping filled and empty voxels).
 *
 * --------------------------------------------------------------------
 * Case 8: Complement of Case 4. Five voxels fill the cube leaving three
 *         empty voxels that form an equilateral triangle (all pairwise
 *         distances equal 2). The three empty voxels each touch only
 *         diagonally, creating non-manifold edges throughout the surface.
 *         Example: all voxels except (0,0,1),(1,0,0),(0,1,0), i.e.
 *                  (0,0,0),(0,1,1),(1,0,1),(1,1,0),(1,1,1).
 *
 *   Pseudo-3D view:
 *       ■ ■
 *       · ■   (back layer z=1)
 *   · ■       (front layer z=0)
 *   ■ ·
 *
 *   # Voxel-configurations in the binary grid: 8.
 *   Reason: 5 voxels, signature [1, 1, 1, 2, 2, 2, 2, 2, 2, 3]
 * --------------------------------------------------------------------
 * Case 9: Complement of Case 3. Five voxels fill the cube leaving three
 *         empty voxels whose distances are [1,2,3] — one face-adjacent pair,
 *         one edge-diagonal pair, and one body-diagonal pair. The empty
 *         region spans the full cube, creating a non-manifold edge.
 *         Example: all voxels except (0,0,0),(1,0,0),(1,1,1), i.e.
 *                  (0,0,1),(0,1,0),(0,1,1),(1,0,1),(1,1,0).
 *
 *   Pseudo-3D view:
 *       ■ ·
 *       ■ ■   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   · ·
 *
 *   # Voxel-configurations in the binary grid: 24.
 *   Reason: 5 voxels, signature [1, 1, 1, 1, 2, 2, 2, 2, 3, 3]
 * --------------------------------------------------------------------
 * Case 10: Complement of Case 2. Six voxels fill the cube leaving two
 *          face-diagonal voxels empty. The two empty voxels touch only
 *          at an edge, creating a non-manifold edge where four faces meet.
 *          Example: all voxels except (0,0,0) and (1,0,1), i.e.
 *                   (0,0,1),(0,1,0),(0,1,1),(1,0,0),(1,1,0),(1,1,1).
 *
 *   Pseudo-3D view:
 *       ■ ■
 *       ■ ·   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   · ■
 *
 *   # Voxel-configurations in the binary grid: 12.
 *   Reason: 6 voxels, signature [1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3]
 * --------------------------------------------------------------------
 * Case 11: Complement of Case 1. Six voxels fill the cube leaving two
 *          opposite corners (body diagonal) empty. The two empty voxels
 *          are at opposite corners, so the surface pinches at two vertices
 *          forming a non-manifold bowtie through the solid.
 *          Example: all voxels except (0,0,0) and (1,1,1), i.e.
 *                   (0,0,1),(0,1,0),(0,1,1),(1,0,0),(1,0,1),(1,1,0).
 *
 *   Pseudo-3D view:
 *       ■ ·
 *       ■ ■   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   · ■
 *
 *   # Voxel-configurations in the binary grid: 4.
 *   Reason: 6 voxels, signature [1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3]
 * --------------------------------------------------------------------
 *
 * @section Voxel Configurations with Non-Manifold Material Variations
 *
 * Beyond the 11 non-manifold cases above, there exist 10 additional voxel configurations
 * that are geometrically manifold when all active voxels share the same material, but can
 * produce non-manifold edge cases when two or more active voxels carry different material labels.
 * These are referred to as manifold cases 12-21.
 *
 * --------------------------------------------------------------------
 * Case 12: Single voxel — only one active voxel in the cube.
 *          Example: voxel at (0,0,0).
 *
 *   Pseudo-3D view:
 *       · ·
 *       · ·   (back layer z=1)
 *   ■ ·       (front layer z=0)
 *   · ·
 *
 *   # Voxel-configurations in the binary grid: 8.
 *   Reason: 1 voxel, signature []
 * --------------------------------------------------------------------
 * Case 13: Two face-adjacent voxels.
 *          Example: voxels at (0,0,0) and (1,0,0).
 *
 *   Pseudo-3D view:
 *       · ·
 *       · ·   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   · ·
 *
 *   # Voxel-configurations in the binary grid: 12.
 *   Reason: 2 voxels, signature [1]
 * --------------------------------------------------------------------
 * Case 14: Three voxels in a gamma (L) shape — two face-adjacent pairs
 *          sharing a common voxel, with the two end voxels edge-adjacent.
 *          Example: voxels at (0,0,0), (1,0,0), and (0,1,0).
 *
 *   Pseudo-3D view:
 *       · ·
 *       · ·   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   ■ ·
 *
 *   # Voxel-configurations in the binary grid: 24.
 *   Reason: 3 voxels, signature [1, 1, 2]
 * --------------------------------------------------------------------
 * Case 15: Four voxels forming a 2x2 square in one plane.
 *          Example: voxels at (0,0,0), (1,0,0), (0,1,0), and (1,1,0).
 *
 *   Pseudo-3D view:
 *       · ·
 *       · ·   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   ■ ■
 *
 *   # Voxel-configurations in the binary grid: 6.
 *   Reason: 4 voxels, signature [1, 1, 1, 1, 2, 2]
 * --------------------------------------------------------------------
 * Case 16: Four voxels in a T-shape — three face-adjacent in a row, with
 *          the fourth attached face-adjacent to the middle one.
 *          Example: voxels at (0,0,0), (1,0,0), (0,1,0), and (0,0,1).
 *
 *   Pseudo-3D view:
 *       ■ ·
 *       · ·   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   ■ ·
 *
 *   # Voxel-configurations in the binary grid: 24.
 *   Reason: 4 voxels, signature [1, 1, 1, 2, 2, 2]
 * --------------------------------------------------------------------
 * Case 17: Four voxels in a zigzag chain — three sequential face-adjacent
 *          pairs forming a 3D chain.
 *          Example: voxels at (0,0,0), (1,0,0), (1,1,0), and (1,1,1).
 *
 *   Pseudo-3D view:
 *       · ·
 *       · ■   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   · ■
 *
 *   # Voxel-configurations in the binary grid: 24.
 *   Reason: 4 voxels, signature [1, 1, 1, 2, 2, 3]
 * --------------------------------------------------------------------
 * Case 18: Five voxels — four forming a 2x2 square in one plane, with the
 *          fifth attached face-adjacent to one corner of the square.
 *          Example: voxels at (0,0,0),(1,0,0),(0,1,0),(1,1,0),(0,0,1).
 *
 *   Pseudo-3D view:
 *       ■ ·
 *       · ·   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   ■ ■
 *
 *   # Voxel-configurations in the binary grid: 24.
 *   Reason: 5 voxels, signature [1, 1, 1, 2, 2, 2, 2, 2, 2, 3]
 * --------------------------------------------------------------------
 * Case 19: Six voxels forming an L-shape spanning two layers.
 *          Example: voxels at (0,0,0),(1,0,0),(0,1,0),(1,1,0),(0,0,1),(1,0,1).
 *
 *   Pseudo-3D view:
 *       ■ ■
 *       · ·   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   ■ ■
 *
 *   # Voxel-configurations in the binary grid: 24.
 *   Reason: 6 voxels, signature [1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3]
 * --------------------------------------------------------------------
 * Case 20: Seven voxels — all eight corners of the cube except one.
 *          Example: all voxels except (1,1,1).
 *
 *   Pseudo-3D view:
 *       ■ ■
 *       ■ ·   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   ■ ■
 *
 *   # Voxel-configurations in the binary grid: 8.
 *   Reason: 7 voxels, signature [1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,3]
 * --------------------------------------------------------------------
 * Case 21: Eight voxels — all eight corners of the cube active.
 *          Example: all voxels at (0,0,0) through (1,1,1).
 *
 *   Pseudo-3D view:
 *       ■ ■
 *       ■ ■   (back layer z=1)
 *   ■ ■       (front layer z=0)
 *   ■ ■
 *
 *   # Voxel-configurations in the binary grid: 1.
 *   Reason: 8 voxels, signature [1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3]
 * --------------------------------------------------------------------
 *
 * Non-manifold cases 1-11 can produce non-manifold configurations in two ways:
 *   1. When they are homogeneous (all active voxels share the same material) — the geometric
 *      pinch point of the non-manifold case itself creates a non-manifold junction.
 *   2. When they are non-homogeneous (active voxels carry different material labels) — a
 *      sub-voxel configuration within them matches one of the 11 non-manifold cases (1-11),
 *      creating a material-driven non-manifold situation within the larger configuration.
 *
 * The same applies to manifold cases 12-21: even though they are geometrically manifold,
 * non-homogeneous material assignments can create sub-voxel configurations that match one
 * of the 11 non-manifold cases, producing a non-manifold edge case.
 *
 * @section Intuition and terminology: face-connected components, splits, and complements
 *
 * This class resolves non-manifold output by duplicating the point generated at a triad and
 * assigning the faces around that triad to different duplicate points. The key question is:
 * for a given local configuration, which faces should be grouped together (share the same point)
 * to improve local topology (ideally open-manifold per label region) when the configuration is
 * solvable?
 *
 * We use the following terminology:
 *
 * - Face-connected component (or simply component): a set of active voxels in the 2×2×2 cube that
 *   are connected through face adjacency (Hamming distance 1). Each component corresponds to one
 *   manifold sub-configuration and typically requires its own duplicate point.
 *
 * - Split pattern: a particular assignment of the 12 possible faces (the 12 edge-case bits) to
 *   component indices. In the tables this is stored as PointIndices[12] plus the corresponding
 *   SubVoxelCases/SubEdgeCases.
 *
 * - Normal/Primary split: the split pattern obtained from a given basic voxel configuration by
 *   grouping its active voxels into face-connected components (Hamming distance 1) and assigning
 *   each generated face to the component that produced it.
 *
 *   Not every case has a normal/primary split:
 *   - Cases 1–8 have a normal split.
 *   - Cases 9–11 do not have a normal split (they are complements of cases 1–3).
 *   - Cases 12–21 are geometrically manifold and do not define normal/complementary splits at this
 *     level; they only become relevant through detected non-manifold sub-configurations.
 *
 * - Complementary split: some non-manifold situations have a complementary split via its
 *   complementary voxel configuration (the occupancy complement inside the 2×2×2 cube).
 *
 *   Complementary splits are not always available:
 *   - Cases 1–3 do not have a complementary split.
 *   - Cases 4–11 do have a complementary split.
 *   - Cases 9–11 have only the complementary split (no normal split).
 *   - Cases 12–21 do not have a complementary split at this level.
 *
 * Important: a voxel configuration and its paired complementary configuration can map to the same
 * 12-bit edge case. Since the lookup table is keyed by edge case, both split patterns
 * may appear in the same metadata span for that edge case as separate entries. In practice, such
 * pairs are usually stored as the first two entries for the edge case (table indices 0 and 1),
 * and the complementary split is selected by swapping between them.
 *
 * The complementary split matters mainly for homogeneous material assignments: if all active voxels
 * belong to the same label region, using the complementary split can avoid creating artificial
 * separations inside that region while still resolving the non-manifold topology.
 *
 * @section Precomputed non-manifold metadata (CreateNonManifoldMetaDataPerEdgeCase)
 *
 * vtkSurfaceNets3D’s “flying edges” traversal produces, at each triad point, a compact local
 * description of the 2×2×2 voxel neighborhood:
 *
 * - An edge case: a 12-bit mask describing which of the 12 inter-voxel faces are generated.
 * - A voxel case: an 8-bit mask describing which of the 8 voxel corners are active
 *   (non-background).
 * - The material labels at the 8 corners.
 *
 * The same edge case can arise from many different voxel cases and many different material
 * assignments, and only some of those assignments are actually non-manifold. To make runtime
 * resolution fast, this class precomputes a sparse lookup table that, for each edge case that can
 * be non-manifold, provides one or more candidate entries.
 *
 * Conceptually, each candidate entry corresponds to a basic voxel configuration (and possibly a
 * material-driven variation) and provides a split pattern: how to assign generated faces to
 * duplicate point indices, plus per-component sub-configurations used for manifold processing and
 * smoothing.
 *
 * Table generation (high level):
 *
 * 1. Enumerate all rotated/reflected variants of cases 1–21 and compute their basic voxel masks.
 * 2. For each voxel configuration, enumerate material variations ("color encodings") and compute
 *    the resulting edge case; keep only variations that can yield a non-manifold edge case per
 *    material assignment.
 * 3. For each kept variation, split the 2×2×2 active voxels into face-connected components
 *    (Hamming distance 1 connectivity) and record the resulting duplication pattern.
 *
 * The result is sparse: most of the 4096 theoretical 12-bit edge cases are either  geometrically
 * unreachable or always manifold, and therefore have no metadata entries.
 *
 * Sparsity statistics (informational):
 * - Out of the 4096 theoretically possible 12-bit edge cases, only 1097 are geometrically reachable
 *   from any voxel configuration.
 * - Out of the geometrically reachable configurations, only 54.4% (564/1097) can be non-manifold
 *   for some material assignment.
 * - The remaining are always treated as manifold and therefore have no metadata entries.
 *
 * Table-index outcome statistics (informational):
 * - Counts like "how often ManifoldIndex vs NonManifoldIndexUnsolvable occurs" depend on how you
 *   enumerate material variations. The exhaustive enumeration used for internal testing treats many
 *   highly irregular material assignments as equally likely. In typical segmented images, labels
 *   tend to be spatially coherent, so many of these exotic permutations are unlikely in practice.
 * - These numbers should not be interpreted as a probability distribution for real images; they
 *   primarily quantify the behavior under an exhaustive combinatorial enumeration.
 * - If we perform an exhaustive search of all the possible table indices that could result from all
 *   voxel configurations and their colorings, the following counts were observed:
 *     Table index -2 (ManifoldIndex): 5784
 *     Table index -1 (NonManifoldIndexUnsolvable): 14174
 *     Table index 0:  884
 *     Table index 1:  256
 *     Table index 2-9: 6 each
 *
 * @section Runtime lookup and solvability (GetNonManifoldTableIndex)
 *
 * At runtime, `GetNonManifoldTableIndex()` decides whether the neighborhood is:
 *
 * - Manifold (`ManifoldIndex`) → no point duplication is needed.
 * - Solvable non-manifold (returns a non-negative table index) → use the selected metadata
 *   entry to duplicate points and generate per-component sub-cases.
 * - Non-manifold but unresolvable (`NonManifoldIndexUnsolvable`) → the material assignment
 *   creates a non-manifold junction that cannot be eliminated using the available split patterns.
 *
 * The returned table index is a local indicator for a single triad neighborhood. Whether the final
 * surface contains an observable non-manifold edge/vertex can depend on how neighboring triads are
 * resolved. In particular, a neighborhood that is marked as unresolvable may still not create a
 * non-manifold artifact in the final mesh if adjacent neighborhoods introduce compatible point
 * duplications that avoid the problematic connectivity.
 *
 * A practical sufficient condition for full resolvability is local homogeneity: if, for every triad
 * neighborhood, all active voxels share the same label (for example, a single-material
 * segmentation, or multiple materials that never meet at a triad), then
 * `GetNonManifoldTableIndex()` will never return `NonManifoldIndexUnsolvable`. In that scenario,
 * all detected non-manifold situations are handled through the geometric split patterns, and the
 * resulting surface for each label region is guaranteed to be open-manifold. If label regions are
 * disjoint (do not share faces), the full output mesh is likewise open-manifold as a disjoint union
 * of those regions.
 *
 * Conceptually, the runtime decision has three stages:
 *
 * 1. Fast geometric prefilter: if the edge case has no metadata entries, it is guaranteed manifold.
 * 2. Material-aware classification: the corner labels are canonicalized into a
 *    color-encoded voxel case (labels on active corners only), which is used to decide whether the
 *    actual material configuration is non-manifold.
 *    (An edge case may have multiple metadata entries; entries are first filtered by the exact
 *    8-bit voxel-case bitmask before any material-dependent decision is applied.)
 * 3. Select a split pattern:
 *    - If the configuration is homogeneous and the table provides an alternate (complementary)
 *      grouping for the same geometric situation, the lookup may flip to that alternate split
 *      to preserve contiguous same-material flow through the junction.
 *    - For multi-material configurations, the lookup may resolve the non-manifoldness via a
 *      detected sub-voxel non-manifold case, but only when that sub-case has a complementary split
 *      available; otherwise the configuration is reported as unresolvable.
 *
 * In terms of “when is a configuration solvable?”, the implementation applies these rules:
 *
 * - If the edge case has no metadata entries → manifold.
 * - If the material-aware classification says the current label assignment is manifold → manifold.
 * - For homogeneous non-manifold configurations: if the metadata table provides both a primary and
 *   a complementary grouping (typically stored as indices 0 and 1 for that edge case), the
 *   complementary grouping is preferred to avoid artificial splits inside one material region.
 * - For multi-material configurations: a split is accepted only if it eliminates the non-manifold
 *   junction for the given material arrangement. In practice this requires either
 *   (a) a detected sub-configuration with a complementary split available, or
 *   (b) a primary split whose components do not contain unresolvable sub-non-manifold patterns
 *       (cases 1–3), as checked by `IsSplitSufficientForColorEncodedVoxelCase()`.
 * - For geometrically manifold cases (12–21): the configuration is only solvable when its
 *   non-manifoldness is caused by a solvable sub-configuration (with a complementary split).
 *
 * Note: when `VTK_SURFACE_NETS_3D_ALWAYS_SPLIT_CASES_1_3` is enabled, cases 1–3 are treated as
 * non-manifold and are always split regardless of material labels (these patterns are typically
 * interpreted as sampling artifacts without a meaningful continuous analogue).
 *
 * @section Applying a resolved entry (duplicate points, face assignment, and sub-cases)
 *
 * Once a non-negative table index has been selected, the corresponding metadata entry provides:
 *
 * - The duplicate-point assignment for each generated face via `PointIndices[12]`.
 * - The manifold sub-cases (`SubVoxelCases` / `SubEdgeCases`) for processing each component
 *   independently.
 *
 * Only points are duplicated (faces are not): the intent is to improve local topology per label
 * region without duplicating geometry across labels. Duplicating faces would replicate the same
 * geometric face into multiple label regions. By duplicating points instead, label regions can be
 * separated topologically while shared faces between different labels remain geometrically
 * coincident.
 *
 * @section Generation of Smoothing Stencils for Non-Manifold Edge Cases
 *
 * Given that we can resolve a subset of non-manifold neighborhoods by splitting them into manifold
 * sub-configurations, the next step is to apply smoothing to improve the geometric quality of the
 * output surface.
 * For each point in the output mesh, a smoothing stencil defines which neighboring points it
 * should be averaged with to compute its smoothed position.
 *
 * To resolve a non-manifold edge case, we split its voxel configuration into face-connected
 * groups, yielding sub-configurations each with their own sub edge case and sub voxel case.
 * Since each sub-configuration is manifold, it can be processed independently to generate
 * a well-defined smoothing stencil for its corresponding duplicate point.
 *
 * For non-manifold edge cases, we generate stencils for two kinds of points:
 *   - Each duplicate point, using its corresponding manifold sub edge case.
 *   - The non-manifold point itself, using the non-manifold edge case.
 *
 * The stencil for the non-manifold point is needed because, when a neighboring point's stencil
 * needs to connect to a duplicated point, rather than arbitrarily choosing one duplicate, it
 * connects to the non-manifold point whose position is averaged by points included in all
 * sub-configurations. This ensures that the smoothing operation is consistent across groups and
 * keeps all duplicate points geometrically close to each other after smoothing.
 *
 * (Material-aware split selection is part of the `GetNonManifoldTableIndex()` decision: when the
 * labels for the basic voxel case are homogeneous and an alternate grouping exists in the metadata
 * table, the lookup can flip to that alternate split to avoid introducing artificial separations
 * inside a single material region.)
 */

#ifndef vtkSurfaceNets3DNonManifoldCases_h
#define vtkSurfaceNets3DNonManifoldCases_h

#include "vtkFiltersCoreModule.h"
#include "vtkSetGet.h"

#include <array>
#include <cstddef>
#include <cstdint>

VTK_ABI_NAMESPACE_BEGIN
#define VTK_SURFACE_NETS_3D_ALWAYS_SPLIT_CASES_1_3
// #define VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8
class VTKFILTERSCORE_EXPORT vtkSurfaceNets3DNonManifoldCases
{
public:
#ifdef VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8
  static constexpr uint8_t MaxTableIndex = 9;
#else
  static constexpr uint8_t MaxTableIndex = 8;
#endif
  using NonManifoldCaseType = unsigned char;
  using EdgeCaseType = unsigned short;
  using VoxelCaseType = unsigned char;

  /**
   * For non-manifold edge cases:
   *   - NonManifoldCase: the 1-based NM case index [1-21] where [1-11] are the non-manifold cases,
   *                      and [12-21] are the manifold cases which have non-manifold inside
   *   - VoxelCase:       the 8-bit bitmask of active voxel corners
   *   - IsHomogeneous    whether all active corners of the basic voxel case share the same material
   *                      label or not,
   *   - PointIndices:    for each of the 12 face bits, the index of the connected component
   *                      (duplicate point group) it belongs to, or -1 if no face is generated
   *   - SubVoxelCases:   Up to 4 voxel cases, one per connected component, used to drive the
   *                      manifold rendering of each component separately
   *   - SubEdgeCases:    up to 4 sub edge cases, one per connected component,
   *                      used to drive the manifold rendering of each component separately
   */
  struct NonManifoldCaseMetadata
  {
    NonManifoldCaseType NonManifoldCase = 0;
    VoxelCaseType VoxelCase = 0;
    bool IsHomogeneous = true;
    std::array<int8_t, 12> PointIndices;
    std::array<VoxelCaseType, 4> SubVoxelCases;
    std::array<EdgeCaseType, 4> SubEdgeCases;
  };
  /**
   * Holds the local configuration of the 8 voxels surrounding a triad point in the grid.
   * This includes the 12-bit edge case bitmask indicating which edges of the voxel cell are
   * intersected by the surface net, the 8-bit voxel case bitmask indicating which of the 8
   * surrounding voxels are inside a labeled region, and the material labels of each of the
   * 8 surrounding voxels. Inactive voxels (i.e., those outside a labeled region) are assigned
   * the background label.
   *
   * @tparam T The type of the material labels (e.g., int, unsigned short).
   */
  template <class T>
  struct VoxelNeighborhood
  {
    EdgeCaseType EdgeCase;
    VoxelCaseType VoxelCase;
    std::array<T, 8> Labels;
  };

  enum TableIndexValues : int8_t
  {
    EmptyIndex = -3,
    ManifoldIndex = -2,
    NonManifoldIndexUnsolvable = -1,
    NonManifoldIndex0 = 0,
    NonManifoldIndex1 = 1,
    NonManifoldIndex2 = 2, // Used only by NonManifoldCase 8
    NonManifoldIndex3 = 3, // Used only by NonManifoldCase 8
    NonManifoldIndex4 = 4, // Used only by NonManifoldCase 8
    NonManifoldIndex5 = 5, // Used only by NonManifoldCase 8
    NonManifoldIndex6 = 6, // Used only by NonManifoldCase 8
    NonManifoldIndex7 = 7, // Used only by NonManifoldCase 8
    NonManifoldIndex8 = 8, // Used only by NonManifoldCase 8
#ifdef VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8
    NonManifoldIndex9 = 9, // Used only by NonManifoldCase 8
#endif
  };

  /**
   * Returns the index into the non-manifold metadata table for the given voxel neighborhood,
   * or a special sentinel value if the neighborhood is manifold or unresolvable.
   *
   * The lookup proceeds as follows:
   *
   * 1. The edge case is checked against the precomputed metadata table.
   *    If no entries exist for this edge case, ManifoldIndex is returned immediately.
   *
   * 2. The material labels at the active corners are canonicalized into a color-encoded voxel
   *    case and checked against the non-manifold case tables to determine whether the actual
   *    material configuration is truly non-manifold. This is necessary because the same edge
   *    case can arise from multiple material configurations, some of which are manifold and
   *    some non-manifold. If the configuration is manifold, ManifoldIndex is returned.
   *
   * 3. For each non-manifold entry compatible with the actual voxel case, the correct table
   *    index is determined based on the following decision tree:
   *
   *    1. If the voxel config matches one of the 11 non-manifold cases:
   *       a. If cases 1-3 (VTK_SURFACE_NETS_3D_ALWAYS_SPLIT_CASES_1_3):
   *          → always return table index 0 which represents the normal/primary split.
   *       b. If the configuration is homogeneous (all active voxels share the same material):
   *          i.  If a complementary split exists (cases 4-11):
   *              → return the complementary split (swap between the paired entries, typically
   *                table indices 0 and 1) to support contiguous same-material flow.
   *          ii. Else if a normal/primary split exists (cases 1-8):
   *              → return the normal/primary split for the basic voxel case.
   *       c. Else if a sub-voxel non-manifold configuration exists:
   *          i.  If the sub-voxel edge case equals the current edge case:
   *              → if a complementary split of the sub-voxel exists (cases 4-11):
   *                 return the complementary split of the sub-voxel case.
   *          ii. Else if the sub-voxel edge case is a subset of the current edge case:
   *              → if normal splits exist for both sub-voxel (cases 1-8) and basic (cases 1-8):
   *                 - if the split is sufficient for the material configuration
   *                   (IsSplitSufficientForColorEncodedVoxelCase), return the normal split
   *                   of the basic voxel case. For non-manifold case 8, which can have up to
   *                   9 valid table entries, the index is capped at MaxTableIndex unless
   *                   VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8 is
   *                   defined — entries beyond MaxTableIndex are treated as unsolvable by
   *                   default to limit complexity.
   *                 - otherwise return NonManifoldIndexUnsolvable.
   *              → otherwise return NonManifoldIndexUnsolvable.
   *
   *       Notes:
   *       - Cases 9-11 have no normal/primary split; they are resolved using the complementary
   *         split patterns of their paired configurations.
   *
   *    2. If the voxel config matches one of the manifold cases (cases 12+):
   *       a. These cases are non-manifold only under non-homogeneous material assignments.
   *          If a sub-voxel non-manifold configuration exists:
   *          i.  If the sub-voxel edge case equals the current edge case:
   *              → if a complementary split of the sub-voxel exists (cases 4-11):
   *                 return the complementary split of the sub-voxel case.
   *          → otherwise return NonManifoldIndexUnsolvable.
   *
   * @tparam T                The type of the material labels (e.g., uint8_t).
   * @param voxelNeighborhood The voxel neighborhood containing the edge case, voxel case
   *                          bitmask, and material labels at the 8 cube corners.
   * @return The 0-based index into the non-manifold metadata table if a compatible non-manifold
   *         configuration is found and resolved, ManifoldIndex if the configuration is manifold,
   *         or NonManifoldIndexUnsolvable if the configuration is non-manifold but could not be
   *         resolved for the given material configuration.
   */
  template <class T>
  static auto GetNonManifoldTableIndex(const VoxelNeighborhood<T>& voxelNeighborhood) -> int8_t;

  /**
   * Returns the number of non-manifold metadata entries for the given edge case.
   * Returns 0 if the edge case has no non-manifold configurations.
   *
   * @param edgeCase The 12-bit edge case to query.
   * @return The number of metadata entries for this edge case. Note that this value is unrelated to
   *         MaxTableIndex (which limits how many table indices are encoded in the triad
   *         state for certain configurations).
   */
  static VTK_ALWAYS_INLINE auto GetNumberOfNonManifoldCases(EdgeCaseType edgeCase) -> uint8_t
  {
    return EdgeCaseOffsets[edgeCase + 1] - EdgeCaseOffsets[edgeCase];
  }

  /**
   * Given an edge case and the table index for the non-manifold case, returns the corresponding
   * non-manifold case index, where 0 means manifold.
   *
   * @param edgeCase            A 12-bit edge case bitmask.
   * @param tableIndex          The index in the non-manifold cases table for the given edge case
   * @return The case index in [0, 21], where 0 means manifold.
   */
  static VTK_ALWAYS_INLINE auto GetNonManifoldCase(EdgeCaseType edgeCase, int8_t tableIndex)
    -> NonManifoldCaseType
  {
    if (tableIndex < 0 || tableIndex >= GetNumberOfNonManifoldCases(edgeCase))
    {
      return 0; // manifold case
    }
    return NonManifoldMetadata[EdgeCaseOffsets[edgeCase] + tableIndex].NonManifoldCase;
  }

  /**
   * Given an edge case and the table index for the non-manifold case, returns the corresponding
   * basic voxel case bitmask for the corresponding non-manifold configuration. The basic voxel case
   * is an 8-bit bitmask where each bit indicates whether the corresponding voxel corner is present
   * in the non-manifold configuration.
   *
   * @param edgeCase            A 12-bit edge case bitmask.
   * @param tableIndex          The index in the non-manifold cases table for the given edge case
   * @return The basic voxel case bitmask in [0, 255].
   */
  static VTK_ALWAYS_INLINE auto GetNonManifoldBasicVoxelCase(
    EdgeCaseType edgeCase, int8_t tableIndex) -> VoxelCaseType
  {
    return NonManifoldMetadata[EdgeCaseOffsets[edgeCase] + tableIndex].VoxelCase;
  }

  /**
   * Given an edge case and the table index for the non-manifold case, returns whether it's
   * homogeneous or not. Homogeneous means that all active corners of the basic voxel case share the
   * same material label.
   *
   * @param edgeCase            A 12-bit edge case bitmask.
   * @param tableIndex          The index in the non-manifold cases table for the given edge case
   * @return True if all active corners of the basic voxel case share the same material label, false
   * otherwise.
   */
  static VTK_ALWAYS_INLINE auto IsNonManifoldCaseHomogeneous(
    EdgeCaseType edgeCase, int8_t tableIndex) -> NonManifoldCaseType
  {
    return NonManifoldMetadata[EdgeCaseOffsets[edgeCase] + tableIndex].IsHomogeneous;
  }

  /**
   * Given the non-manifold case, returns the number of duplicate points. This is equal to the
   * number of face-connected groups plus one additional point used as a stencil point for
   * smoothing. For manifold cases (index 0), this is 1 (no duplicate points, no smoothing stencil
   * needed).
   *
   * @param nonManifoldCase The non-manifold case index in [0, 11], where 0 means manifold.
   * @return The number of duplicate points.
   */
  static constexpr VTK_ALWAYS_INLINE auto GetNumberOfPoints(NonManifoldCaseType nonManifoldCase)
    -> uint8_t
  {
    constexpr uint8_t pointsPerNonManifoldCase[12] = {
      1,     // case 0  is manifold, so 1 point (the center vertex) is generated
      2 + 1, // case 1  has 1 duplicate point,  so 2 points + 1 center point = 3 total
      2 + 1, // case 2  has 1 duplicate point,  so 2 points + 1 center point = 3 total
      2 + 1, // case 3  has 1 duplicate point,  so 2 points + 1 center point = 3 total
      3 + 1, // case 4  has 2 duplicate points, so 3 points + 1 center point = 4 total
      2 + 1, // case 5  has 1 duplicate point,  so 2 points + 1 center point = 3 total
      2 + 1, // case 6  has 1 duplicate point,  so 2 points + 1 center point = 3 total
      4 + 1, // case 7  has 3 duplicate points, so 4 points + 1 center point = 5 total
      2 + 1, // case 8  has 1 duplicate point,  so 2 points + 1 center point = 3 total
      2 + 1, // case 9  has 1 duplicate point,  so 2 points + 1 center point = 3 total
      2 + 1, // case 10 has 1 duplicate point,  so 2 points + 1 center point = 3 total
      2 + 1, // case 11 has 1 duplicate point,  so 2 points + 1 center point = 3 total
    };
    return pointsPerNonManifoldCase[nonManifoldCase];
  }

  /**
   * Given a quad id (bits 0–3 for yz-plane quads, 4–7 for xz-plane, 8–11 for xy-plane),
   * and the table index for the non-manifold case, returns the duplicate point index for that quad
   * in the current non-manifold configuration.
   *
   * The duplicate point index identifies which connected component the voxels generating that
   * face belong to. Components are formed by splitting all populated voxels in the 2x2x2 grid
   * into connected components where two voxels are directly connected if their Hamming distance
   * is exactly 1. Note that voxels within the same component do not need to be directly connected
   * to each other — they only need to be reachable through a path of direct connections.
   *
   * @tparam QuadId             Index of the quad (face-bit position) in [0, 11].
   * @param edgeCase            A 12-bit edge case bitmask.
   * @param tableIndex          The index in the non-manifold cases table for the given edge case
   *                            of a non-manifold case, false otherwise.
   * @return The duplicate point index for the given quad, or -1 if no face is generated.
   */
  template <uint8_t QuadId>
  static VTK_ALWAYS_INLINE auto GetNonManiFoldPointIndex(EdgeCaseType edgeCase, int8_t tableIndex)
    -> int8_t
  {
    static_assert(QuadId < 12, "QuadId must be between 0 and 11 (inclusive)");
    return NonManifoldMetadata[EdgeCaseOffsets[edgeCase] + tableIndex].PointIndices[QuadId];
  }

  /**
   * Given the non-manifold case, returns the number of manifold sub-cases. Each sub-case
   * corresponds to one face-connected group of voxels that needs to be processed independently to
   * resolve the non-manifold configuration. For manifold cases (index 0), this is 1.
   *
   * @param nonManifoldCase The non-manifold case index in [0, 11], where 0 means manifold.
   * @return The number of manifold sub-cases for the given non-manifold case.
   */
  static constexpr VTK_ALWAYS_INLINE auto GetNumberOfManifoldSubCases(
    NonManifoldCaseType nonManifoldCase) -> uint8_t
  {
    constexpr uint8_t subCasesPerNonManifoldCase[12] = {
      1, // case 0  is manifold, so 1 point (the center vertex) is generated
      2, // case 1  has 1 duplicate point,  so 2 sub cases
      2, // case 2  has 1 duplicate point,  so 2 sub cases
      2, // case 3  has 1 duplicate point,  so 2 sub cases
      3, // case 4  has 2 duplicate points, so 3 sub cases
      2, // case 5  has 1 duplicate point,  so 2 sub cases
      2, // case 6  has 1 duplicate point,  so 2 sub cases
      4, // case 7  has 3 duplicate points, so 4 sub cases
      2, // case 8  has 1 duplicate point,  so 2 sub cases
      2, // case 9  has 1 duplicate point,  so 2 sub cases
      2, // case 10 has 1 duplicate point,  so 2 sub cases
      2  // case 11 has 1 duplicate point,  so 2 sub cases
    };
    return subCasesPerNonManifoldCase[nonManifoldCase];
  }

  /**
   * Returns the manifold sub-edge cases for a given edge case and table index. Each
   * sub-edge case is a 12-bit mask representing the faces belonging to one face-connected
   * group of voxels. Unused sub-cases are zero.
   *
   * @param edgeCase   A 12-bit edge case bitmask.
   * @param tableIndex The index into the non-manifold metadata table for this edge case.
   * @return A reference to the array of up to 4 manifold sub-edge cases.
   */
  static VTK_ALWAYS_INLINE auto GetManifoldSubEdgeCases(EdgeCaseType edgeCase, int8_t tableIndex)
    -> const std::array<EdgeCaseType, 4>&
  {
    static constexpr std::array<EdgeCaseType, 4> Empty{};
    if (tableIndex < 0)
    {
      return Empty;
    }
    return NonManifoldMetadata[EdgeCaseOffsets[edgeCase] + tableIndex].SubEdgeCases;
  }

  /**
   * Computes a state code from a point count and table index using a
   * variable-width linear mapping. The state space is partitioned by point count,
   * with offsets for the 4-point and 5-point groups calculated dynamically using
   * MaxTableIndex to ensure the 3-point group is fully utilized.
   *
   * State space layout (without VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8):
   *   State 0:        Empty voxel (numPoints=0, EmptyIndex)
   *   State 1:        Manifold point (numPoints=1, ManifoldIndex)
   *   State 2:        Unsolvable non-manifold point (numPoints=1, NonManifoldIndexUnsolvable)
   *   States 3-11:    3-point solvable non-manifold (tableIndex 0-8, MaxTableIndex=8)
   *   States 12-13:   4-point solvable non-manifold (tableIndex 0-1)
   *   States 14-15:   5-point solvable non-manifold (tableIndex 0-1)
   *   Maximum state:  15
   *
   * State space layout (with VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8):
   *   State 0:        Empty voxel (numPoints=0, EmptyIndex)
   *   State 1:        Manifold point (numPoints=1, ManifoldIndex)
   *   State 2:        Unsolvable non-manifold point (numPoints=1, NonManifoldIndexUnsolvable)
   *   States 3-12:    3-point solvable non-manifold (tableIndex 0-9, MaxTableIndex=9)
   *   States 13-14:   4-point solvable non-manifold (tableIndex 0-1)
   *   States 15-16:   5-point solvable non-manifold (tableIndex 0-1)
   *   Maximum state:  16
   *
   * @param numPoints  The number of duplicate points to generate (0, 1, 3, 4, or 5).
   * @param tableIndex The metadata table index (EmptyIndex, ManifoldIndex,
   *                   NonManifoldIndexUnsolvable, or 0 to MaxTableIndex).
   * @return The computed state code.
   */
  static constexpr VTK_ALWAYS_INLINE auto ComputeState(uint8_t numPoints, int8_t tableIndex)
    -> uint8_t
  {
    switch (numPoints)
    {
      case 0:
        return 0;
      case 1:
        return tableIndex == ManifoldIndex ? 1 : 2;
      case 3:
        return 3 + tableIndex; // Indices 0-MaxTableIndex map to 3...
      case 4:
        // If MaxTableIndex is 9, offset is 3 + 10 = 13
        // If MaxTableIndex is 8, offset is 3 + 9 = 12
        return 3 + (MaxTableIndex + 1) + tableIndex;
      case 5:
        // If MaxTableIndex is 9, offset is 13 + 2 = 15
        // If MaxTableIndex is 8, offset is 12 + 2 = 14
        return 3 + (MaxTableIndex + 1) + 2 + tableIndex;
      default:
        return 1;
    }
  }

  /**
   * Decoded metadata for a state: number of points to generate and
   * the corresponding non-manifold table index.
   */
  struct StateInfo
  {
    // 0 (empty), 1 (manifold or unsolvable), 3, 4, or 5
    uint8_t NumPoints;
    // EmptyIndex, ManifoldIndex, NonManifoldIndexUnsolvable, or [0, MaxTableIndex]
    int8_t TableIndex;
  };

  /**
   * Decodes both the number of duplicate points and the metadata table index
   * from a state code in a single table lookup.
   *
   * @param state The state code computed via ComputeState.
   *              Valid range: [0, 15] without
   *              VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8,
   *              or [0, 16] with it.
   * @return StateInfo containing NumPoints and TableIndex.
   */
  static constexpr VTK_ALWAYS_INLINE auto GetStateInfo(uint8_t state) -> StateInfo
  {
#ifdef VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8
    constexpr StateInfo InfoTable[17] = { { 0, EmptyIndex }, // 0:  Empty
      { 1, ManifoldIndex },                                  // 1:  Manifold
      { 1, NonManifoldIndexUnsolvable },                     // 2:  Non-Manifold (Unsolvable)
      // 3-12: 3-point solvable (tableIndex 0-9)
      { 3, NonManifoldIndex0 }, { 3, NonManifoldIndex1 }, { 3, NonManifoldIndex2 },
      { 3, NonManifoldIndex3 }, { 3, NonManifoldIndex4 }, { 3, NonManifoldIndex5 },
      { 3, NonManifoldIndex6 }, { 3, NonManifoldIndex7 }, { 3, NonManifoldIndex8 },
      { 3, NonManifoldIndex9 },
      // 13-14: 4-point solvable (tableIndex 0-1)
      { 4, NonManifoldIndex0 }, { 4, NonManifoldIndex1 },
      // 15-16: 5-point solvable (tableIndex 0-1)
      { 5, NonManifoldIndex0 }, { 5, NonManifoldIndex1 } };
#else
    constexpr StateInfo InfoTable[16] = { { 0, EmptyIndex }, // 0:  Empty
      { 1, ManifoldIndex },                                  // 1:  Manifold
      { 1, NonManifoldIndexUnsolvable },                     // 2:  Non-Manifold (Unsolvable)
      // 3-11: 3-point solvable (tableIndex 0-8)
      { 3, NonManifoldIndex0 }, { 3, NonManifoldIndex1 }, { 3, NonManifoldIndex2 },
      { 3, NonManifoldIndex3 }, { 3, NonManifoldIndex4 }, { 3, NonManifoldIndex5 },
      { 3, NonManifoldIndex6 }, { 3, NonManifoldIndex7 }, { 3, NonManifoldIndex8 },
      // 12-13: 4-point solvable (tableIndex 0-1)
      { 4, NonManifoldIndex0 }, { 4, NonManifoldIndex1 },
      // 14-15: 5-point solvable (tableIndex 0-1)
      { 5, NonManifoldIndex0 }, { 5, NonManifoldIndex1 } };
#endif
    return InfoTable[state];
  }

private:
  /**
   * Returns the manifold sub-voxel cases for a given edge case and table index. Each
   * sub-voxel case is an 8-bit bitmask representing the active corners of one face-connected
   * group of voxels in the primary split. Unused sub-cases are zero.
   *
   * Used at runtime in conjunction with IsPrimarySplitValid() to check whether the primary
   * split is valid for a given material configuration — specifically, whether any face group
   * contains an unresolvable sub-non-manifold configuration of cases 1-3.
   *
   * @param edgeCase   A 12-bit edge case bitmask.
   * @param tableIndex The index into the non-manifold metadata table for this edge case.
   * @return A reference to the array of up to 4 manifold sub-voxel cases.
   */
  static VTK_ALWAYS_INLINE auto GetManifoldSubVoxelCases(EdgeCaseType edgeCase, int8_t tableIndex)
    -> const std::array<VoxelCaseType, 4>&
  {
    return NonManifoldMetadata[EdgeCaseOffsets[edgeCase] + tableIndex].SubVoxelCases;
  }

  static const std::array<uint16_t, 4097>& EdgeCaseOffsets;
  static const std::array<NonManifoldCaseMetadata, 3736>& NonManifoldMetadata;
};
VTK_ABI_NAMESPACE_END

#endif // vtkSurfaceNets3DNonManifoldCases_h
