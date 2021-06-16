# Graphics Engine

Graphics engine for the course Graphics Programming in the second year of DAE.

# Goal

The base foundations were given, a game engine called Overlord engine. The goal was to add custom implementations to this engine and make a simple game with it.

[Gameplay video](https://vimeo.com/563656610)

# Topics

* NVIDIA PhysX
* Custom HLSL shaders
  * Material shaders (diffuse, specular, phong, blinn, ambient, environment mapping, opacity)
  * Sprite / Texture rendering via shaders
  * Skybox shader
  * Shadowmapping for static and skinned meshes
  * Particle shader with billboarding
  * Post processing
* Particle effects
* Model animator for character mesh animations (exported from mixamo, custom implementation in code)

# In depth

## Shaders
We learned to write shaders in this course and used the tool FXComposer for this. Nsight Graphics was used for live debugging. 

## Shadowmapping
A shadowmap gets rendered from the viewpoint of the light. This shadow depth texture is then given to both PosNormTex3D_Shadow.fx and it's skinned counterpart. Every pixels depth value from the camera viewpoint then gets evaluated compared to this shadowmap. 

## Particle emitter
The particle emitter makes use of an object pool of particles. This buffer is then directly mapped to the GPU

The shader effect file goes through this buffer of particles. Vertices for the texture get created via the geometry shader. 
The vertices get rotated and billboarding is applied so that they always face the camera. 

## Model animator
All animation files, together with the mesh were translated into .ovm files with the given Overlord tool. These are binary files that are then read via the model animator
The correct animation keyframes are found depending on the progression of the animation. 

The bone transforms then get linearly interpolated between the keyframes. 
These bone transforms are fed into the shader files (PosNormTex3D_Skinned_Shadow.fx) where vertices are translated depending on the bone transforms.

# End note
There's a lot of improvements that can be made:
* Different techniques for shadowmapping
* Non linear animation interpolation
* Modern ways of implementing a skybox
* ...

But it was a great learning experience, opening the path of graphics programming!

