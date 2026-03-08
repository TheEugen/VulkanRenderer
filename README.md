# 🎮 VulkanRenderer

A C++ Vulkan renderer built from scratch as a learning project, implementing the full rendering pipeline from instance creation to per-frame presentation. Loads `.obj` meshes and textures, supports multiple rendering modes, and renders in real time via a GLFW event loop. Inspired by [niagara](https://github.com/zeux/niagara) and the *Vulkan Cookbook* by Pawel Lapinski.

---

## Features

- **Full Vulkan bootstrap** — `VulkanBase` handles instance creation, physical device selection, surface creation (via GLFW), and capability/format querying with diagnostic output
- **Swapchain & render pass** — Swapchain creation with image views, framebuffers, and a render pass with colour, depth, and MSAA resolve attachments
- **Configurable graphics pipeline** — `Pipeline` encapsulates all pipeline state (vertex input, rasterization, multisampling, colour blending, dynamic viewport/scissor, push constants) with a `setPolygonMode()` override for wireframe rendering
- **Multiple render modes** — Standard, wireframe, and cel shading fragment shaders, selectable via `VulkanMainConfig`; splitscreen mode renders two pipelines side by side in a single pass
- **GLSL shaders compiled to SPIR-V** — Four shader variants (`vertShader`, `fragShader`, `wireframeFragShader`, `cel_shading`) compiled via the included `SPIRVCompiler.bat`
- **OBJ mesh loading** — Meshes loaded via `Mesh`, with vertex and index buffers uploaded to device-local memory via a staging buffer and dedicated transfer queue
- **Texture loading** — Images loaded and uploaded to the GPU via `EasyImage`, bound as a combined image sampler descriptor
- **MVP + lighting UBO** — Per-frame uniform buffer containing model, view, projection matrices and a dynamic light position, updated each frame with time-based animation
- **Push constants** — Phong toggle and texture presence flag pushed to the fragment shader per draw call
- **MSAA** — Maximum supported sample count auto-detected from the physical device; sample rate shading enabled
- **Depth buffering** — Depth image managed by `DepthImage` with format support queries
- **Swapchain recreation** — Window resize handled via GLFW callback, triggering full swapchain, render pass, and framebuffer recreation
- **Keyboard camera** — WASD/QE camera movement via GLFW key callback, updating the view matrix each frame
- **Separate transfer queue** — Vertex, index, and texture uploads use a dedicated transfer queue and command pool, separate from the graphics queue

---

## Project Structure

```
VulkanRenderer/
├── Main.cpp              # Entry point, config setup, event loop
├── VulkanBase.h          # Instance, surface, physical device, GLFW init
├── VulkanMain.h          # Core renderer: swapchain, pipelines, draw loop
├── VulkanMainConfig.h    # Config struct (paths, flags, dimensions)
├── VulkanUtils.h         # Shared helpers: buffer/image creation, layout transitions
├── Pipeline.h            # Graphics pipeline encapsulation
├── Mesh.h                # OBJ mesh loading
├── Vertex.h              # Vertex struct and input descriptions
├── EasyImage.h           # Texture loading and GPU upload
├── DepthImage.h          # Depth buffer image and format selection
├── shader/               # GLSL vertex and fragment shaders
└── SPIRVCompiler.bat     # Compiles shaders to SPIR-V via glslc
```

---

## Getting Started

### Prerequisites

- Windows
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) (includes `glslc`)
- [GLFW](https://www.glfw.org/)
- [GLM](https://github.com/g-truc/glm)
- Visual Studio (MSVC)

### Shader Compilation

Before building, compile the GLSL shaders to SPIR-V:

```bash
SPIRVCompiler.bat
```

### Build & Configure

Open the project in Visual Studio and configure the Vulkan SDK, GLFW, and GLM include/lib paths. Mesh and texture paths are set directly in `Main.cpp`:

```cpp
const char* objPath = "meshes//dragon.obj";
const char* texPath = "images//skull_diffuse.jpg";
```

Rendering mode flags are set via `VulkanMainConfig`:

```cpp
vulkanMainConfig.useSplitscreen = false;  // render two pipelines side by side
vulkanMainConfig.useWireframe   = false;  // wireframe fragment shader
vulkanMainConfig.useCelShading  = false;  // cel shading fragment shader
```

---

## Controls

| Key | Action |
|-----|--------|
| `W` / `S` | Strafe left / right |
| `A` / `D` | Strafe up / down |
| `Q` / `E` | Move forward / backward |

---

## References

- [niagara](https://github.com/zeux/niagara) by Arseny Kapoulkine
- *Vulkan Cookbook* by Pawel Lapinski

---

## License

This project is licensed under the [MIT License](LICENSE).