# Niji Engine ***WIP***

### Vulkan Renderer written in `C++` and `Slang`. Other dependencies: `CMake` `GLFW` `GLM` `STB_Image` `EnTT` `VMA` `fastgltf` `mikktspace` `ImGui` `nlohmann/json`

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
- [x] PBR Lighting (Directional and Point Lights + IBL)
- [x] Skybox
- [x] Mipmapping
- [x] Lights Editor via ImGui and ImGuizmo
- [x] Basic Lights Serialization via nlohmann/json
- [x] Debug Line Rendering
- [x] Shader Hot Reload
- [ ] Shader Printf
- [ ] Tiled Forward+ Rendering (WIP...)
- [ ] Clustered Forward+ Rendering

and more...

## Latest Render
<img width="1921" height="1120" alt="image" src="https://github.com/user-attachments/assets/0abdc686-6607-4550-97b1-be3c76b66864" />

Details:
- Lights: 96 moving Point Lights
- IBL: 1 *Distant Light Probe*, Diffuse 1024x1024 cubemap, Specular 1024x1024 cubemap with 7 mips
- Performance: ~3.4ms @ 1920x1080p on RTX4060 laptop edition. Depth Pre-Pass saved about 4ms of frame time

## How to build
`Windows`
- Download and Install the latest [vulkan sdk](https://www.lunarg.com/vulkan-sdk/)
- Download and Install the latest [CMake release](https://cmake.org/download/)
- In your IDE of choice, open and build the project!
