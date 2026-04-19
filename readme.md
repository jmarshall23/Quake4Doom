# RTCW - Wolfenstein DXR (D3D12)

![Preview](https://i.ibb.co/cSWMdyj9/image-2.webp)

## Overview

This project brings **Return to Castle Wolfenstein** into the modern era using **Direct3D 12** and **DXR ray tracing**, while preserving the original game's look and feel.

The engine has been ported to **x64**, with legacy **VM code removed and rewritten in C++**, allowing for deeper control over the engine and rendering pipeline.

Rendering runs through an OpenGL-to-D3D12 shim layer called **IceBridge**, which translates legacy fixed-function calls into a modern backend. Because the shim layer maintains the OpenGL interface, the original renderer remains largely untouched.

The result is a unique visual style:
- Classic early-2000s aesthetics remain intact  
- Lighting behaves physically with ray tracing  
- Shadows are softer and more natural  
- Reflections and depth add subtle realism  

It still *feels like RTCW* — just with more accurate light.

---

## Key Features

- ⚡ **Direct3D 12 backend**
- 🌍 **DXR ray traced lighting & shadows**
- 🧱 **OpenGL shim layer (IceBridge)**
- 🧬 **x64 port with VM removed and rewritten in C++**
- 🎮 **Original gameplay and renderer preserved**
- 🔥 **Modern GPU acceleration**

---

## Technology

- Direct3D 12 (D3D12)
- DXR (DirectX Raytracing)
- OpenGL 1.x style emulation via IceBridge
- id Tech 3–based rendering pipeline

---

## Media

Preview image:  
https://ibb.co/C3jNFwfy

---

## Status

🚧 Work in progress

---

## Tags

`rtcw` `wolfenstein` `dxr` `d3d12` `raytracing` `opengl` `idtech3` `graphics` `rendering`
