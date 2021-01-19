Version History
---------------

### Changes in OSPRay Studio 0.6.0 (alpha)

-   Second alpha, compatible with OSPRay release v2.5.x

-   glTF loader improvements:
    Animation and skinning of triangle meshes
    KHR_lights_punctual support for glTF Lights and experimental environmental lights
    Initial glTF KHR_materials_* extensions and KHR_texture_transform support.
    Add support for glTF POINTS primitive for point clouds
    Support GLTF scene cameras
    Added rich image generation containing GeomIDs and InstanceIDs, exported as additional exr layers

-   Scene File: Load/Save scene file with model transforms, material, lights and camera state

-   Image export improvement
    Exported images and now correctly denoised
    batch-mode supports different image formats and exr options

-   Batch and plugin improvements
    animation in batch mode

-   UI improvements
    Lights editor, add/remove/edit lights for easy scene lighting
    Camera path and keyframe editor
    Camera type and parameter editor
    Rendering stats window added
    Add UI to adjust volume "filter" mode
    Transfer function widget for volumes

-   Core
    Add curves geometry
    Initial isosurface support for volumes
    Expose SceneGraph support for clipping geometries
    Implement an AssetsCatalogue to avoid re-importing assets, simply instance
    Import raw structured and spherical files with specific volume nodes

### OSPRay Studio 0.5.0 (alpha)

-   Initial public alpha release, compatible with OSPRay release v2.4.x
