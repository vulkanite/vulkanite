# Vulkanite - An OWL-like Productivity Library for Vulkan Ray Tracing


## What is Vulkanite

This library is an attempt to build something like OWL for Vulkan (for
Vulkan Ray Tracing, to be specific - this library only foucsses on the
ray tracing part!). For those that don't know, OWL
(https://github.com/NVIDIA/owl) is a library that aims at making it
easier to build OptiX/RTX Ray Tracing programs by offering a simpler,
slightly higher-level API on top of the low-level OptiX driver
API. This library - vulkanite - does exactly the same, with an, in
fact, very similer API as used by OWL - just for VulkanRT rather than
for OptiX. 

Ie, Vulkanite - like OWL - allows the user to use the key concepts of
RTX ray tracing - like Closest-Hit, Any-Hit, and Intersection
Programs, Compute Kernels, and Hardware- and/or Driver-provided
acceleration structure traversal, etc; but without having to engage
with the much more "verbose" (and often error prone) low-level
driver-API programming of low-level Vulkan, instead allowing to
interact with somewhat higher-level abstractions like *geometries*,
*geometry types*, geometry *groups*, and the respective
AH/CH/IS/bounds programs, etc.

## Key Abstractions in Vulkanite

Like OWL, Vulkanite offers the following abstractions:

- *Geometry Types* (`VTKGeomType`s) describe the set of programs --
closest hit, anyhit, and, where applicable, intersection and bounds
programs -- that are to be executed for a certain geometric object, as
well as the size of data to be reserved in the shader biding table for
that object. What data gets stored there, and what the programs do
with that, is up to the user. Unlike RTX (but like OWL) user
geometries in vulkanite also support *bounds programs* to provide a
user geometries' primtivie bounding boxes (for acceleration structure
construction) on the device.

- *Geometries* (`VTKGeom`s) are geometric objects like a triangle mesh, a set of
spheres, etc, that rays can be traced against. There are two different
*kinds* of geometries; namely *triangles geometries* (with hardware
accelerated ray-triangle intersection) one one hand, and *user
geometries* where the user provides intersection and bounds programs
on the other. Given an existing geometry type `gt`, a geometry can be
created with a simple `vktGeomCreate(gt)`; then followed by setting
the resulting geom's vertex and index buffers
(`vktTrianglesSetVertices/Indices()`) if a triangle mesh, and/or its
other program data using `vktGeomSetData(...)`.

- Geometry *Accels* (`VTKAccel`s) are groups of one more more
geometries that share a common ray tracing acceleration structure. As
for geometries there are two kinds of accels: those for triangles
geometries, and a separate one for user geometries. All geometries in
a triangles accel have to be of the triangles geometry kind, but can
still have different geometry *types* (ie, programs); the same
applies, in reverse, to user geometry accels. Like in OWL, accels in
vulkanite can be created as simply as
`vtk<User|Trianlges>AccelCreate(<list of geoms>)`, and can then be
built or rebuilt with a single `vktAccelBuild(accel)`.

- *Instances* work the same way as in OWL - the top-level world is a
list of instances of user- or triangle accels. Each instance refers to
one such accel (and optionally, a affine transform matrix). Instances
are created by creating a *instance accel*, which can then also be
built/rebuilt like any other accel, and can be queried for a
"traversable" (`VKTTraversableHandle`) that device programs can then
trace rays against.

- Buffers (`VTKBuffer`) are - well - buffers ... 'nough said. Unlike
OWL there are (currently) no different device/managed/host-pinned
buffers; instead, buffers always live on the device, but can be
mapped/unmapped on the host to read/write. Each buffer can be queries
for its device-side address using `vktBufferGetAddress()`; the result
is a device side address that is invalid on the host, but that can be
passed to device programs for these programs to access that buffer's
data on the device.

- *Compute* programs (`VTKCompute`) enable applications to run
general, non-ray tracing compute kernels to do thinks, like, for
example, fill a device-side buffer, run a filter/conversion kernel
over a frame buffer, run a global shade kernel over a buffer of
already traced rays, etc. 

## Key differences to OWL

- Whereas all device-side "compute" and "shading" in OWL happens in
CUDA (ie, a closest-hit program is written in CUDA), in vulkanite all
device code is written in `slang`. This means we can no longer as
easily share header files across both host and device side code, but
... it means we can compile code for every platofrm supported by
Vulkan and Slang.

- OWL provides a set of `owl::common::` types for vectors, matrices,
etc, that work in both C++ and CUDA, and that therefore can easily be
used in both host and device code (eg, both host and device could use
the same `owl::common::vec3f` type for a 3-float vector or point). In
vulkanite, we instead largely rely on the slang `float3` etc types on
the device, and whatever he or she wants on the host.

- Whereas OWL has a pretty sophisticaed "parameters" system where
geometry types "declare" indvidual parameters (such as, for example,
`vec3f diffuse` and/or a `float ior`), in vulkanite each geometry,
compute kernel, or push constant that expects some chunk of
device-side data is merely *descibed* through the *size* of that data
(eg, the `sizeof()` some device-side struct the user wants to store
for that object), and is then set through a `owlGeomSet(void *)` with
a (host-side) pointer to the data to be copied into that device-side
object. There is no mapping, no translating, no reference counting,
etc--the host-side data is copied into the device-side object, and
nothing more; if any of the host side data needs some sort of
"translating" (eg, a host-side "buffer" to a device side "address")
then this has to be done by the user.

- OWL supports automatic/implicit multi-GPU where a single OWLContext
can span multiple GPUs; vulkanite doesn't do that, and each vulkanite
context is for exactly one GPU. You can still have multiple such
contexts in the same application, but these have to be managed
manually by the application.

## Kudos

- this project is largely inspired by, and has stolen lots of pieces
  from, the OWL project (now at https://github.com/nvidia/owl), as
  well as from Nate Morrical's GPRT project
  (https://github.com/gprt/gprt)


