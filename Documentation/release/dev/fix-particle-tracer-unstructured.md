## Fix vtkParticleTracer with unstructured data

Fix an issue leading to crash in vtkParticleTracer where the locators
for unstructured data would not have been created.

## vtkParticleTracer::CachedData private

Previously protected member vtkParticleTracer::CachedData is now a private
member. It is not intended to be used by classes inheriting vtkParticleTracer.
