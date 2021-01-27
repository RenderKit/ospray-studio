# OSPRay Studio

-   [Overview](index.md)
-   [Getting Started](quickstart.md)
-   [API Documentation](api.md)
-   [Gallery](gallery.md)

## Download OSPRay Studio

OSPRay Studio is available for Linux, macOS, and Windows.

Binary packages of the latest release of OSPRay Studio can be downloaded
as a part of [Intel oneAPI Rendering
Toolkit](https://software.intel.com/content/www/us/en/develop/tools/oneapi/download.html#renderkit).

OSPRay Studio sources can be downloaded from the default `master` branch
of [OSPRay Studio Github
repository](https://github.com/ospray/ospray_studio/).

## Building OSPRay Studio

OSPRay Studio has the following required and optional dependencies.

### Required dependencies

-   [CMake](https://www.cmake.org) (v3.1+) and any C++14 compiler
-   Intel [OSPRay](https://www.github.com/ospray/ospray) (v2.4.0) and
    its dependencies - OSPRay Studio builds on top of OSPRay.
    Instructions on building OSPRay are provided
    [here](http://www.ospray.org/downloads.html#building-and-finding-ospray)
    -   Intel [Open VKL](https://www.github.com/openvkl/openvkl)
        (v0.11.0)
    -   Intel [Embree](https://www.github.com/embree/embree) (v3.8.0)
    -   Intel oneAPI Rendering Toolkit common library
        [rkcommon](https://www.github.com/ospray/rkcommon)
    -   Intel [Threading Building
        Blocks](https://www.threadingbuildingblocks.org/)
-   OpenGL and [GLFW](https://www.glfw.org/) (v3.x) - for the windowing
    environment

### Optional Dependencies

-   Intel [Open Image Denoise](https://openimagedenoise.github.io/) -
    for denoising frames. To use with OSPRay Studio, OSPRay must be
    built with `-DBUILD_OIDN=ON` in CMake
-   [OpenImageIO]() and [OpenEXR]() - to support images in a variety of
    file formats. Set `OPENIMAGEIO_ROOT` and `OPENEXR_ROOT` to the
    respective install directories to use these libraries

### Building on Linux and macOS

-   Follow OSPRay's build instructions to install it, which will also
    fulfill most other required dependencies. Set the following
    environment variables to easily locate OSPRay, Open VKL, Embree, and
    rkcommon during CMake.

    ``` bash
    export ospray_DIR = ${OSPRAY_INSTALL_LOCATION}
    export openvkl_DIR = ${OPENVKL_INSTALL_LOCATION}
    export embree_DIR = ${EMBREE_INSTALL_LOCATION}
    export rkcommon_DIR = ${RKCOMMON_INSTALL_LOCATION}
    ```

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
    export LD_LIBRARY_PATH=${OSPRAY_INSTALL}/lib64:${OPENVKL_INSTALL}/lib64:...:$LD_LIBRARY_PATH
    # then run!
    ./ospStudio
    ```

### Building on Windows

Use CMake (cmake-gui) to configure and generate a Microsoft Visual
Studio solution file for OSPRay Studio.

-   Specify the source folder and the build directory in CMake
-   Specify `ospray_DIR`, `openvkl_DIR` and `rkcommon_DIR` CMake
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

## Getting Started

At its core, OSPRay Studio maintains a scenegraph of nodes representing
the current scene as a *directed acyclic graph*. This simplifies user
interaction with the scene and hides the maintenance of OSPRay state.
The scenegraph manages the four primary OSPRay components: the
`FrameBuffer`, `Camera`, `Renderer`, and `World`. OSPRay Studio
currently offers three different modes in which it can operate. You can
choose between these modes when launching OSPRay Studio at the command
line.

### OSPRay Studio Modes

-   **GUI Mode** - `./ospStudio [gui] [file1 [file2 ...]]`

This is the default mode in OSPRay Studio. Objects can be loaded from
command line or via the UI. This mode provides full control over the
scenegraph, allowing manipulation of the camera, objects, lights, and
more. This mode also allows you to create scene files (.sg files), which
can be used to load a full scene in any mode.

-   **Batch Mode** - `./ospStudio batch [options] [file1 [file2 ...]]`

This mode provides offline rendering for the objects (or scene file)
provided. Batch mode currently supports saving single image or frames for
animation scenes. For more information on available options, run
`./ospStudio batch --help`.

-   **Timeseries Mode** - `./ospStudio timeseries [options]`

This mode can be used to import timestep volume datasets in raw binary
or OpenVDB formats. This mode is similar to GUI mode but provides extra
support and UI for loading and manipulating time-varying data.

### GUI Mode Features

Scenes can be loaded in OBJ and GLTF format using `import` option under file
menu. Animated scenes can be loaded using `import and animate`. Animation with
skinning is currently supported in GLTF format. 

Demo scenes can be loaded in File -&gt; Demo Scene. These are a good
starting point to try out various OSPRay Studio controls. The Edit menu
allows immediate editing of renderer and frame buffer settings. The View
Menu contains editors for more complex behaviors, including

-   Keyframes for animated camera paths
-   Camera snapshots (which can be saved and used across multiple
    sessions)
-   Adding, removing, and manipulating lights in the scene
-   Manipulating geometry and materials within the scene
-   Animating time-varying data (e.g. animated glTF files or
    simulations)

The save feature of the GUI mode allows you to save scene files (.sg files)
which can be later reloaded.  These are JSON-formatted files describing the
current scene, including loaded objects/data, lights, and camera state. Scene
files are human-readable and editable. Note that they do not include the
actual data of objects in the scene, but rather instruct OSPRay Studio to load
and transform the specified objects.

Material, Light and Camera information can be saved separately from scene
files.

Loaded scenes can be saved as images in following formats
-   EXR/HDR/PPM/PFM/JPG/PNG

With EXR format, additional channel information can also be exported
-   albedo
-   normal
-   depth
-   metadata (geometric and instance IDs of scene objects)
