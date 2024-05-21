# vulkage

This project was setup by following the [mesh shading implementation](https://www.youtube.com/playlist?list=PL0JVLUVCkk-l7CWCn3-cdftR0oajugYvd) streaming via Arseny Kapoukine

Now mostly following the **bgfx** style with my own rough understanding to do the abstraction.

It is a playground to understand modern graphics programming better.

----------

#### Depends

- volk
- meshoptimizer
- tracy
- glfw
- imgui
- bx
- bimg
- bgfx_common
- glm

----------

#### niagara part(The streaming by Arseny Kapoukine):

Supports:

- mesh-shading
- task-submission
- Lod
- mesh level occlusion
- mesh-let level occlusion
- frustum culling
- back-face culling

-----------

#### vulkage part:

`` leash the vulkan in cage ``

- frame graph
  - sort and clip unnecessary passes and resources based on the contribution to the final result.
  - auto barrier: intend to record the resource state and transform states and layouts atomically.
    - == todo ==: cover the custom render func situation
  - auto alias: alias transition buffers if they don't overlap in the resource lifetime. 
    - == todo ==: implement for images
- gfx api abstraction
  - vulkan
- profiling
  - based on tracy

----------

#### reference

- [Niagara streaming](https://www.youtube.com/playlist?list=PL0JVLUVCkk-l7CWCn3-cdftR0oajugYvd)  - Arseny Kapoukine
- [FrameGraph: Extensible Rendering Architecture in Frostbite](https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in)  - Yuriy O'Donnell
- [Advanced Graphics Tech: Moving to DirectX 12: Lessons Learned ](https://www.gdcvault.com/play/1024656/Advanced-Graphics-Tech-Moving-to) - Tiago Rodrigues
- [Render graphs](https://apoorvaj.io/render-graphs-1/) - Apoorva Joshi 
- [Vulkan Barriers Explained](https://gpuopen.com/learn/vulkan-barriers-explained/) - Matthäus Chajdas
- [Organizing GPU Work with Directed Acyclic Graphs](https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3) - Pavlo Muratov
- And Vulkan documents :)

---------

#### screen shots

![near](.\screenshot\near.png)

![avg](.\screenshot\avg.png)

![far](.\screenshot\far.png)















