# OSPRay Studio

OSPRay Studio is an application to showcase OSPRay's rendering capabilities. It
builds on the ```ospray_sg``` scene graph library found in the OSPRay project.

NOTE: This project is currently in a "pre-alpha" state and is subject to
changes at any time...stability is not (yet) guaranteed!

## Build Instructions

OSPRay Studio's only dependency is OSPRay itself (```1.7.0+```). It is
assumed that there exists an OSPRay install somewhere, which you can point to
with ```ospray_DIR``` either as an environment variable or CMake variable.
Thus to build, follow these steps:

1. Clone the source

```bash
git clone http://github.com/ospray/ospray_studio
```

2. Create a build directory

```bash
cd ospray_studio
mkdir build
cd build
```

3. Run CMake

```bash
export ospray_DIR=${OSPRAY_INSTALL_LOCATION}
cmake ..
```

4. Compile

```bash
make -j `nproc`
```

5. Run!

```bash
./ospStudio
```
