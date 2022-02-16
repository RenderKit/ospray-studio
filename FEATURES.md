High-level description of some of the OSPRay Studio features.

Studio provides a thin wrapper on top of the OSPRay API, as such, the rendering
features are better described in the, excellent, OSPRay documentation
(https://www.ospray.org/documentation.html)

OBJ/MTL importer:

-   native triangle mesh and quad mesh primitives
-   basic OBJ materials, with extension for any OSPRay material via "type" keyword
-   texture and "texture transformations" supported on all OSPRay texturable material parameters via "map_" parameter prefix
-   support for UDIM texture tiles with common "*1001*" filenaming convension

glTF importer:

-   mesh and points primitives, vanilla materials, animation (including camera), skinning
-   KHR_lights_punctual: "directional", "point", "spot"; custom extension for OSPRay "sunSky" and "hdri" light types
-   KHR_materials_pbrSpecularGlossiness: "diffuseFactor", "diffuseTexture", "specularFactor", "glossinessFactor"
-   KHR_materials_clearcoat: "clearcoatFactor", "clearcoatRoughnessFactor", "clearcoatTexture", "clearcoatRoughnessTexture", "clearcoatNormalTexture"
-   KHR_materials_transmission: "transmissionFactor", "transmissionTexture"
-   KHR_materials_sheen: "sheenColorFactor", "sheenColorTexture", "sheenRoughnessFactor", "sheenRoughnessTexture"
-   KHR_materials_ior: "ior"
-   KHR_materials_volume: "thicknessFactor", "thicknessTexture", "attenuationDistance", "attenuationColor"
-   KHR_materials_specular: "specularFactor", "specularTexture"
-   KHR_texture_transform: "offset", "rotation", "scale"
-   BIT_scene_background: "background-uri", "rotation"
-   BIT_asset_info: "title"
-   BIT_reference_link: "id", "title"
-   BIT_node_info: "id"
-   EXT_cameras_sensor, respecting
    -   "imageSensor": "pixels", "pixelSize"
    -   "lens": "focalLength", "apertureRadius", "focusDistance", "centerPointShift"
-   support for UDIM texture tiles with common "*1001*" filenaming convension

PCD importer:

-   file importer for common "Point Cloud Data" (https://pointclouds.org/) format

Volume rendering:

-   importers for raw structured, raw spherical, particle volumes, and OpenVDB file formats
-   scenegraph support to structure, unstructured, particle volumes, and vdb data formats
-   controls for transfer functions and implicit iso-surfaces, as supported by OSPRay and OpenVKL

Meta data export, using plugin "metadata" via `ospStudio batch -p metadata -m ...`:

-   in exr: id (instanceID), objectID, depth, worldPosition
-   ID maps in prefix.id.json and prefix.objectId.json
-   3D bounding boxes in prefix.bboxId.json

batch mode:

-   scriptable headless rendering for offline images
-   setting animation speed (fps)
-   selecting frames (sub-range) to be rendered
-   selecting camera (if multiple specified in glTF)
-   selecting scene configuration from command line
-   setting rendering parameters (maxContribution, accumLimit, variance)

python bindings:

-   the scene graph is exposed via pybind11 binding for full scriptable support

JSON scene file format:

-   the scenegraph state may loaded/saved to a JSON file

Extendable via plugins:

-   plugin mechanism allows extension of file importer, scene graph internals, and UI widgets/appearance
-   several of the plugins under development are featured on the website
