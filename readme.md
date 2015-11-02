# blocks
Making a minecraft-style procedurally generated game in C++ with DirectX. Current goal is to implement all the voxel-based goodies (large procedural world, interesting terrain generation, building, fluid mechanics etc.), make it visually appealing and have a delightful experience along the way.

## Latest screenshot (October 20, 2015)
![Latest screenshot](http://s5.postimg.org/hp7yes2rb/screen_20_10_15.jpg, "Latest screenshot")

## Roadmap

**Already there:**
- Well-behaved game window
- Prototype worldgen based on Perlin noise
- Player movement
- Multiple (limited for now) block types
- Lighting
	* sunlight
	* fake AO (block-level)
- Day-night cycle
- Profiling and debug text output

**Next up:**
- Shadows
	* currently a single light-source: the sun
	* cascaded shadow mapping
	* filtering?
- Saving game
	* abstract out the game state
	* streaming to/from disk
	* compressed world format?
- Optimize chunk/mesh generation algorithm
	* already amortizing mesh generation across multiple frames
	* bottleneck: sending data to the GPU
	* use video memory to cache chunk meshes/update on demand
	* threaded chunk/mesh generation

**Big challenges:**
- Large view distance
	* LODs
- Interesting terrain
- Visual appearance
	* ambient light solution (sample sky occlusion? light probes?)
- Fluid dynamics
	* level of realism?
- Fun to play