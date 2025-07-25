# Niji Engine ***WIP***

### Toy Renderer written in C++ with `Vulkan`. Other dependencies: `CMake` `GLFW` `GLM` `STB_Image` `EnTT` `VMA` `fastgltf` `mikktspace` `ImGui`

<br>

## Feature Checklist
- [x] Triangle Rendering :)
- [x] (Vertex) Buffers
- [x] Texturing
- [x] ECS System
- [x] Model Loading
- [x] Abstracted Renderer with Pass System
- [x] Shaders written in Slang
- [x] Dear ImGUI Implementation
- [x] PBR Lighting (Directional Light + IBL)
- [x] Skybox
- [x] Mipmapping
- [ ] Clustered Forward+ Rendering

and more...

## Latest Render
<img width="1918" height="1115" alt="image" src="https://github.com/user-attachments/assets/1710a775-e355-4b23-970a-d6121c1d772a" />

Details:
- Lights: 1 Direction Light
- IBL: 1 *Distant Light Probe*, Diffuse 1024x1024 cubemap, Specular 1024x1024 cubemap with 7 mips
- Performance: ~3ms @ 1920x1080p

## How to build
`Windows`
- Download and Install the latest [vulkan sdk](https://www.lunarg.com/vulkan-sdk/)
- Download and Install the latest [CMake release](https://cmake.org/download/)
- In your IDE of choice, open and build the project!
