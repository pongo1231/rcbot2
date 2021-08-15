# RCBot2

## Information

This is a fork of [the RCBot2 plugin][rcbot2] written primarily by Cheeseh.

The primary goal of this fork is to perform many wide-sweeping changes to improve
maintainability and to bring the codebase up to modern C++ standards.

The [bots-united.com discord][] and [forums][bots-united forums] are the places to ask for
general RCBot2 support.

[rcbot2]: http://rcbot.bots-united.com/
[bots-united.com discord]: https://discord.gg/BbxR5wY
[bots-united forums]: http://rcbot.bots-united.com/forums/index.php?showforum=18

## Changes from upstream

- Build process uses [AMBuild][] instead of `make` or Visual Studio.  This removes the need for
Valve's cross platform make conversion tool, and is what AlliedModders uses to build
Metamod:Source and SourceMod.
- The plugin has been split into SDK-specific builds to ensure proper compatibility, using the
same loader shim SourceMod uses to load mod-specific builds.
	- This means all mod-specific changes are provided in the upstream SDK repository, instead
	of this repository having vendored code.
	- The shim is named `RCBot2Meta` to maintain compatibility with existing files; mod-specific
	plugins are named `rcbot.2.${MOD}`.
	- The `sdk-split` branch only contains modifications to get the project running on the
	new build tooling and SDK support without issues.  It should be fairly painless to merge
	(though it does remove `using namespace std;` for sanity).
- The usage of the install directory has been dropped.  In particular, waypoints must be located
under `rcbot2/waypoints/${MOD}` instead of nested under a folder matching the name of the
steamdir.
- Removed custom loadout and attribute support from the TF2 portion of the plugin. Other server
plugins (namely [tf2attributes][] and [TF2Items][], where the implementation was ported from)
are better-suited and maintained to handle that stuff; this plugin should only deal with bots
themselves.
- The Metamod:Source plugin can now optionally expose natives to SourceMod, adding some
functionality to access certain functionality of the RCBot2 plugin via SourceMod plugins.

[AMBuild]: https://wiki.alliedmods.net/AMBuild
[tf2attributes]: https://github.com/FlaminSarge/tf2attributes
[TF2Items]: https://github.com/asherkin/TF2Items

## Installation

1. [Install MetaMod:Source][].
2. Build the RCBot2 package, or [download the most recent automated build][autobuild].
    - For the latter, `package.tar.gz` is the Linux build; `package.zip` is the Windows build.
    - Automated builds are compiled with support for SourceMod native bindings.
    - The automated build uses Ubuntu 18.04 LTS as the Linux build runner &mdash; RCBot2 will
    fail to load on older Linux distributions with the error
    `` version `GLIBC_2.27' not found ``.
    - Regardless of how you get a build, it does not include things like the waypointing guide,
    hookinfo updater, and waypoints themselves.  You can download those from the
    [official release thread][].  Waypoints are also available at [this page][waypoints].
3. Extract the package into your game directory, similar to the process of installing MM:S.
4. Start the server or game client.
5. To verify that the installation was successful, type `rcbotd` in your server console or RCON.
You should see multiple lines starting with "[RCBot]".
    - If you are running this plugin in client mode, use `rcbot` instead.

If you are working with SourceMod interop, you will also need [SourceMod PR#1053][pr] for
plugins to recognize that the natives are available.  The pull request was merged in SourceMod
build 1.11.0.6466.

[Install MetaMod:Source]: https://wiki.alliedmods.net/Installing_Metamod:Source
[official release thread]: http://rcbot.bots-united.com/forums/index.php?showtopic=1994
[waypoints]: http://rcbot.bots-united.com/waypoints.php
[pr]: https://github.com/alliedmodders/sourcemod/pull/1053
[autobuild]: https://github.com/nosoop/rcbot2/releases

## Building

### Cloning from source

RCBot2's repo history had all sorts of build artifacts / binaries at various points in time, so
pulling the repository down normally takes an unusually long while.

I'd highly recommend passing in `--shallow-since 2019-07-19` to minimize the size of your
working repository.  This will create a repository with the earliest commit at
9a7f11ea40be12c9384e7530c2b77763394601db.

### Compiling on Windows / Linux

1. [Install the prerequisites for building SourceMod for your OS.][Building SourceMod]
2. Create a `build/` subdirectory, then run `configure.py`.
	- The project requires C++11 support.  It was previously confirmed to compile on
	GCC 5.4 (Ubuntu 16.04 LTS) and on MSVC 1900.
	- `configure.py` can be run with the following settings:
	`python ../configure.py -s present --mms_path ${MMS_PATH} --hl2sdk-root ${HL2SDK_ROOT}`
	- Specifying an `--sm-path` argument enables linking to SourceMod.  This does not mean
	SourceMod needs to be installed for RCBot2 to run.
	- Note that the automatic versioning system requires an installation of `git` and a
	relatively modern version of Python 3.
3. Run `ambuild`.  MetaMod:Source plugin is built and the base install files will be available
in `build/package`.

[Building SourceMod]: https://wiki.alliedmods.net/Building_SourceMod
