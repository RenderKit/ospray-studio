# OSPRay Studio

This is release v0.12.0 (devel) of Intel® OSPRay Studio. It is released under the
Apache 2.0 license.

Visit [**OSPRay Studio**](http://www.ospray.org/ospray_studio)
(http://www.ospray.org/ospray_studio) for more information.

See [what's
new](https://github.com/ospray/ospray_studio/blob/master/CHANGELOG.md)
in this release.

## Overview

Intel OSPRay Studio is an open source and interactive visualization and
ray tracing application that leverages [Intel OSPRay](https://www.ospray.org)
as its core rendering engine. It can be used to load complex scenes requiring
high fidelity rendering or very large scenes requiring supercomputing resources.

The main control structure is a *scene graph* which allows users to
create an abstract scene in a *directed acyclical graph* manner. Scenes
can either be imported or created using scene graph nodes and structure
support. The scenes can then be rendered either with OSPRay's pathtracer
or scivis renderer.

More information can be found in the [**high-level feature
description**](https://github.com/ospray/ospray_studio/blob/master/FEATURES.md).

Building OSPRay Studio
========================

CMake Superbuild
----------------

### Required dependencies for superbuild

-   [CMake](https://www.cmake.org) (v3.15+) and any C++14 compiler

For convenience, OSPRay Studio provides a CMake Superbuild script which will
pull down its dependencies i.e. GLFW, OSPRay, rkcommon and TBB. It builds OSPRay
Studio without OpemImageIO and OpenEXR support.  `stb_image` is used for all
image operations by default instead. 

To use the superbuild run with:

``` sh
mkdir build
cd build
cmake ..
cmake --build .
```

For other full set of options, run:

``` sh
ccmake ..
```

or

``` sh
cmake-gui ..
```

Standard CMake build
--------------------

For standard cmake process turn off cmake option `OSPRAY_INSTALL` and provide
following required dependencies with their respective cmake options as will be
listed in OS-specific building process below. 

### Required dependencies

-   [CMake](https://www.cmake.org) (v3.15+) and any C++14 compiler
-   Intel [OSPRay](https://www.github.com/ospray/ospray) (v2.10.0) and its
    dependencies - OSPRay Studio builds on top of OSPRay. Instructions on
    building OSPRay are provided
    [here](http://www.ospray.org/downloads.html#building-and-finding-ospray).
    OSPRay and OSPRay Studio have the following common dependencies which Studio
    can hence leverage from an OSPRay build.
    -   Intel oneAPI Rendering Toolkit common library
        [rkcommon](https://www.github.com/ospray/rkcommon) (v1.10.0)
    -   Intel [Threading Building Blocks](https://www.threadingbuildingblocks.org/)
-   OpenGL and [GLFW](https://www.glfw.org) (v3.3.4) - for the windowing environment


### Optional Dependencies

-   Intel [Open Image Denoise](https://openimagedenoise.github.io) - (v1.4.3 or
    newer) for denoising frames. To use with OSPRay Studio, OSPRay must be built
    with `-DBUILD_OIDN=ON` in CMake.
-   [OpenVDB](https://www.openvdb.org/) to support loading VDB formatted volume files.
-   [OpenImageIO](http://openimageio.org/) and [OpenEXR](https://www.openexr.com/)
    (pre-3.x versions) to support images in a variety of file formats.  Set `OPENIMAGEIO_ROOT`
    and `OPENEXR_ROOT` to the respective install directories to use these libraries.
    (tested with OpenImageIO v2.3.16 and OpenEXR v2.5.8)
-   [Python] (3.9.7) (https://python.org) for python bindings

### Building on Linux and macOS

-   Follow OSPRay's build instructions to install it, which will also
    fulfill most other required dependencies. Set the following
    environment variables to easily locate OSPRay and
    rkcommon during CMake.

    

    ``` bash
    export ospray_DIR = ${OSPRAY_INSTALL_LOCATION}
    export rkcommon_DIR = ${RKCOMMON_INSTALL_LOCATION}
    export TBB_DIR = ${TBB_INSTALL_LOCATION}
    ```

    Alternatively, [CMAKE_PREFIX_PATH](https://cmake.org/cmake/help/latest/variable/CMAKE_PREFIX_PATH.html)
    can be set to find the OSPRay install and other dependencies.

-   Clone OSPRay Studio

    ``` bash
    git clone https://github.com/ospray/ospray_studio/
    ```

-   Create build directory and change directory to it (we recommend
    keeping a separate build directory)

    ``` bash
    cd ospray_studio
    mkdir build
    cd build
    ```

-   Then run the typical CMake routine

    ``` bash
    cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ... # or use ccmake
    make -j `nproc` # or cmake --build .
    ```

-   To run OSPRay Studio, make sure `LD_LIBRARY_PATH` (on Linux) or
    `DYLD_LIBRARY_PATH` (on macOS) contains all dependencies. For
    example,

    ``` bash
    export LD_LIBRARY_PATH=${OSPRAY_INSTALL}/lib64:...:$LD_LIBRARY_PATH
    # then run!
    ./ospStudio
    ```

### Building on Windows

Use CMake (cmake-gui) to configure and generate a Microsoft Visual
Studio solution file for OSPRay Studio.

-   Specify the source folder and the build directory in CMake
-   Specify `ospray_DIR`, `rkcommon_DIR` CMake
    variables for the respective install locations
-   Click 'Configure' and select the appropriate generator (we recommend
    using at least Visual Studio 15 2017)
-   Select x64 as an optional parameter for the generator (32-bit builds
    are not supported)
-   Click 'Generate' to create `ospray_studio.sln`. Open this in Visual
    Studio and compile

You can optionally use the CMake command line:

``` pwsh
cmake --build . --config Release --target install
```
