# RedCore-Example-Mod

## Overview
This is a template repo for quickly getting a new NSMBU mod project up and running for [RedCore](https://github.com/Zenith-Team/RedCore).

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

### Developing
The project is already set up for you with all the necessary headers and configurations, so you may get started coding immediately. The standard development loop is:
- Edit code
- Build with `tachyon compile`
- Test with `tachyon launch`

To get proper autocomplete and smart IDE features, it is recommended to use an editor which supports the [clangd](https://clangd.llvm.org) language server as a plugin or backend (make sure to uninstall any other C++ plugins such as Microsoft's). Then, to activate the configuration for the project, run `tachyon compile US --compiledb` once which will generate a `compile_commands.json` file. Restart the IDE and autocomplete should be active.

A simple example actor has been included to demonstrate registration and avoid rewriting boilerplate, but it is highly recommended to change the names and paths from the default "example" placeholders to a unique identifier for your mod in the following places:
- `project.json5` config
- `include` directory
- `main.xml` editor patch
- `Main.cpp` registrar namespace
-  `namespace example` everywhere else

### Distribution
To build a release of your code mod for others to download and use in their own mods, follow the steps below:
- Compile the code for every target with `tachyon package`, this will produce a `package.zip`.
- Pack your Pyamoto editor patch into `patch.zip`.
- Create a GitHub release on your repo, following [semver](https://semver.org/) for the name. Upload `package.zip` and `patch.zip` to the artifacts and publish it as latest.

Users may now extract the `package.zip` into their mod to apply the code/assets, and install the editor patch by linking `https://github.com/YourName/Repo` in Pyamoto. Developers may now install your mod in their environment by running `tachyon pm install YourName/Repo`.

To pack your mod as a self-contained bundle, including all dependencies and assets, run `tachyon bundle`. This produces a `bundle-ExampleMod-1.0.0.zip` which is ready to be installed as a standalone mod either as a Cemu graphic pack or a [Telkin Loader](https://github.com/Zenith-Team/Telkin) mod on console. Uploading this as a GitHub release also allows one-click-install as a Cemu graphic pack by hotlinking to the .zip file directly in the "Download Pack from URL" menu.
