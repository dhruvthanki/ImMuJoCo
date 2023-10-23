cmake_minimum_required(VERSION 3.5)
project(imgui_example_glfw_vulkan C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 11)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# To install GLFW3: sudo apt install libglfw3-dev
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
# find_package(implot CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
# find_package(imguizmo CONFIG REQUIRED)

# To install MuJoCo: pip install mujoco
# To find path to mujoco: pip show mujoco
set(MJC /home/dhruv/.local/lib/python3.10/site-packages/mujoco)
message("Mujoco Path : " ${MJC})
include_directories( ${MJC}/include )

# Dear ImGui
set(IMGUI_DIR /home/dhruv/Documents/Github/imgui/)
set(IMGUI imgui)
add_library(${IMGUI} STATIC 
${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp 
${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp
)
target_include_directories(${IMGUI} PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

set(IMPLOT_DIR /home/dhruv/Documents/Github/ImMuJoCo/external/implot)
set(IMPLOT implot)
add_library(${IMPLOT} STATIC 
${IMPLOT_DIR}/implot.cpp
${IMPLOT_DIR}/implot_demo.cpp
${IMPLOT_DIR}/implot_items.cpp 
)
target_include_directories(${IMPLOT} PUBLIC ${IMPLOT_DIR})
target_link_libraries(${IMPLOT} PUBLIC ${IMGUI})


# App
file(GLOB sources *.cpp)
# add_executable(app ${sources})
add_executable(app main.cpp)
target_link_libraries(app libglfw.so.3 Vulkan::Vulkan ${IMGUI} ${OPENGL_opengl_LIBRARY} ${IMPLOT} ${MJC}/libmujoco.so.3.0.0)