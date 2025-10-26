# RTPM

**Real-Time Path Tracing Renderer implemented in OpenGL**

This project is a real-time path tracing renderer built entirely with **OpenGL**,  
designed to understand the full process of ray tracing without relying on DXR or Vulkan hardware ray tracing APIs.

---

## 🧱 Build Instructions

### 1. Clone the repository
```bash
git clone https://github.com/iduxinyu/RTPM.git
cd RTPM
```

### 2. Prepare build directory
```bash
rm -r build
mkdir build
cd build
```

### 3. Build with CMake
```bash
cmake ..
cmake --build .
```

### 4. Run the executable
```bash
cd ../out
./main
```

---

## ⚙️ Requirements
- C++17 or later  
- OpenGL 4.5 or higher  
- CMake 3.10+  
- GPU with 4GB+ VRAM (recommended)

---

## 🧩 Features
- Custom BVH construction and texture packing  
- Real-time ray tracing pipeline on GPU  
- Single-sample rendering with spatial & temporal denoising  
- Optimized using G-Buffer data to skip redundant intersection tests

---

© 2025 Xinyu Du. All rights reserved.
