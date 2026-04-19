# Quake4Doom - D3D12 With The Awakening Support

## Overview

This project brings **Quake 4** into the modern era through **Quake4Doom**, making it the **first port to bring Quake 4 into open source form** while preserving the original game's look and feel.

The engine has been ported to **x64**, with legacy **VM code removed and rewritten in C++**, allowing for deeper control over the engine and rendering pipeline.

Rendering runs through an OpenGL-to-D3D12 shim layer called **IceBridge**, which translates legacy fixed-function calls into a modern backend. Because the shim layer maintains the OpenGL interface, the original renderer remains largely untouched.

This project is significant not just as a rendering upgrade, but as a preservation and accessibility milestone. By opening up the codebase through this porting effort, **Quake 4 is now available in an open-source form for the first time**, creating new opportunities for study, extension, modernization, and long-term maintenance.

The result is a unique visual style:
- Classic early-2000s aesthetics remain intact  
- Original rendering style is preserved  
- Modern graphics backend improves compatibility and control  
- It still *feels like Quake 4*  

---

## Key Features

- ⚡ **Direct3D 12 backend**
- 🧱 **OpenGL shim layer (IceBridge)**
- 🧬 **x64 port with VM removed and rewritten in C++**
- 🎮 **Original gameplay and renderer preserved**
- 🔥 **Modern GPU acceleration**
- 🌌 **The Awakening support**
- 🚀 **First port to bring Quake 4 into open source form**

---

## Technology

- Direct3D 12 (D3D12)
- OpenGL 1.x style emulation via IceBridge
- id Tech–based rendering pipeline

---

## Status

🚧 Work in progress

---

## Tags

`quake4doom` `quake4` `d3d12` `opensource` `opengl` `graphics` `rendering` `x64` `icebridge`