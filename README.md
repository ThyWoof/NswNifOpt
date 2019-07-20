# NswNifOpt

Nintendo Switch Skyrim NIF Optimizer

# Copyrights

1. NIF lib from https://github.com/ousnius/BodySlide-and-Outfit-Studio
2. Command line parser from https://github.com/jarro2783/cxxopts

## Build (Visual Studio Code)

1. Install https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2017
2. Install https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools (https://github.com/microsoft/vscode-cpptools)

## Usage

```
Usage:
  \NswNifOpt\bin\nswnifopt.exe [OPTION...]

      --remove-editor-marker  remove Editor Marker
      --pretty-sort-blocks    pretty sort blocks
      --trim-textures-path    pretty sort blocks
      --mirror-shape-x        mirror shape over x axis
      --mirror-shape-y        mirror shape over y axis
      --mirror-shape-z        mirror shape over z axis
      --rotate-shape-x arg    rotate shape over x axis by ARG angle
      --rotate-shape-y arg    rotate shape over y axis by ARG angle
      --rotate-shape-z arg    rotate shape over z axis by ARG angle
      --scale-shape-x arg     scale shape on x axis by factor ARG
      --scale-shape-y arg     scale shape on y axis by factor ARG
      --scale-shape-z arg     scale shape on z axis by factor ARG
      --offset-shape-x arg    offset shape over x axis by ARG distance
      --offset-shape-y arg    offset shape over y axis by ARG distance
      --offset-shape-z arg    offset shape over z axis by ARG distance
      --optimize-for-sse      optimize NIF for SSE (modifiers below)
      --is-head-part          ONLY for parts like head, ear, mouth and hair
      --no-remove-parallax    don't remove parallax flags and meshes
      --no-calc-bounds        don't calculate new bounds spheres for meshes
  -i, --input arg             input file
  -o, --output arg            ouptput file
```