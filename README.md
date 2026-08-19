# Ucology
**Ucology** is a **New Super Mario Bros. U** custom code pack that adds and modifies environmental and ambient actors.

## Features
- Expanded level decorations
- Misc. ambient actors
- Configurable player attention targets

### Adding Custom Decoration Types
When using the decoration manager actor, the user can select either the vanilla decorations or a custom configuration. With a custom configuration, up to **five** different textures can be used per zone.

Provided in `uco_flower.szs` and `uco_butterfly.szs` in this project are three flower textures and five butterfly textures. **It is the user's responsibility to modify these files** if they wish to add or replace textures. Explanations for the naming schemes are provided in the following sections. All numbers are zero-indexed.

#### Flowers
The decoration manager has up to **fifteen** texture sets for flowers, with **fifteen** flower textures per set. Flower textures, found in `actor/uco_flower.szs`, are named `flower_AA_BB`, where `AA` refers to the set number and `BB` refers to the flower texture index.

#### Butterflies
The decoration manager has up to **fifteen** textures for butterflies, found in `actor/uco_flower.szs`. Textures within that file are named `butterfly_AA`, where `AA` refers to the butterfly texture index.



## Setup Guide
### Compiling
Install [Tachyon](https://github.com/Zenith-Team/Tachyon) (requires [Node.js](https://nodejs.org/) v24+)
```yml
npm i -g --allow-remote=root https://github.com/Zenith-Team/Tachyon/releases/latest/download/tachyon.tgz
```
Build and run the project for your region (example with `US`)
```rb
tachyon pm install
tachyon compile US
tachyon launch US
```
