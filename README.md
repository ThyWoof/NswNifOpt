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
  nswnifopt.exe [OPTION...]

      --remove-editor-marker  Remove Editor Marker
      --optimize-nif          optimize NIFs
      --is-head-part          NIF is a head part
      --no-remove-parallax    don't remove parallax on NIF optimize
      --no-calc-bounds        don't calc bounds on NIF optimize
  -i, --input arg             input file
  -o, --output arg            ouptput file
```