#include <cstdio>
#include <cstring>
#include <vector>
#include <iostream>

#include <imgui.h>
#include "implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <mujoco/mujoco.h>

// MuJoCo data structures
mjModel* m = NULL;                  // MuJoCo model
mjData* d = NULL;                   // MuJoCo data
mjvCamera cam;                      // abstract camera
mjvOption opt;                      // visualization options
mjvScene scn;                       // abstract scene
mjrContext con;                     // custom GPU context

// mouse interaction
bool button_left = false;
bool button_middle = false;
bool button_right =  false;
double lastx = 0;
double lasty = 0;

const char* XML_PATH = "/home/dhruv/Documents/Github/mujoco_menagerie/agility_cassie/scene.xml";


// keyboard callback
void keyboard(GLFWwindow* window, int key, int scancode, int act, int mods) {
  ImGuiIO &io = ImGui::GetIO();
  if (!io.WantCaptureKeyboard){

  // backspace: reset simulation
  if (act==GLFW_PRESS && key==GLFW_KEY_BACKSPACE) {
    mj_resetData(m, d);
    mj_forward(m, d);
  }
  }
}


// mouse button callback
void mouse_button(GLFWwindow* window, int button, int act, int mods) {
  ImGuiIO &io = ImGui::GetIO();
  if (!io.WantCaptureMouse){

  // update button state
  button_left = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS);
  button_middle = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE)==GLFW_PRESS);
  button_right = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS);

  // update mouse position
  glfwGetCursorPos(window, &lastx, &lasty);
  }
}


// mouse move callback
void mouse_move(GLFWwindow* window, double xpos, double ypos) {
  ImGuiIO &io = ImGui::GetIO();
  if (!io.WantCaptureMouse){

  // no buttons down: nothing to do
  if (!button_left && !button_middle && !button_right) {
    return;
  }

  // compute mouse displacement, save
  double dx = xpos - lastx;
  double dy = ypos - lasty;
  lastx = xpos;
  lasty = ypos;

  // get current window size
  int width, height;
  glfwGetWindowSize(window, &width, &height);

  // get shift key state
  bool mod_shift = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS);

  // determine action based on mouse button
  mjtMouse action;
  if (button_right) {
    action = mod_shift ? mjMOUSE_MOVE_H : mjMOUSE_MOVE_V;
  } else if (button_left) {
    action = mod_shift ? mjMOUSE_ROTATE_H : mjMOUSE_ROTATE_V;
  } else {
    action = mjMOUSE_ZOOM;
  }

  // move camera
  mjv_moveCamera(m, action, dx/height, dy/height, &scn, &cam);
  }
}


// scroll callback
void scroll(GLFWwindow* window, double xoffset, double yoffset) {
  ImGuiIO &io = ImGui::GetIO();
  if (!io.WantCaptureMouse){
    // emulate vertical mouse motion = 5% of window height
    mjv_moveCamera(m, mjMOUSE_ZOOM, 0, -0.05*yoffset, &scn, &cam);
  }
}


// utility structure for realtime plot
struct ScrollingBuffer {
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 2000) {
        MaxSize = max_size;
        Offset  = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y) {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x,y));
        else {
            Data[Offset] = ImVec2(x,y);
            Offset =  (Offset + 1) % MaxSize;
        }
    }
    void Erase() {
        if (Data.size() > 0) {
            Data.shrink(0);
            Offset  = 0;
        }
    }
};


template <typename T>
inline T RandomRange(T min, T max) {
    T scale = rand() / (T) RAND_MAX;
    return min + scale * ( max - min );
}


ImVec4 RandomColor() {
    ImVec4 col;
    col.x = RandomRange(0.0f,1.0f);
    col.y = RandomRange(0.0f,1.0f);
    col.z = RandomRange(0.0f,1.0f);
    col.w = 1.0f;
    return col;
}


// main function
int main(int argc, const char** argv) {
  // load and compile model
  char error[1000] = "Could not load binary model";
  m = mj_loadXML(XML_PATH, 0, error, 1000);
  if (!m) {
    mju_error("Load model error: %s", error);
  }

  // make data
  d = mj_makeData(m);

  // init GLFW
  if (!glfwInit()) {
    mju_error("Could not initialize GLFW");
  }

  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // create window, make OpenGL context current, request v-sync
  GLFWwindow* window = glfwCreateWindow(1200, 900, "Demo", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // initialize visualization data structures
  mjv_defaultCamera(&cam);
  mjv_defaultOption(&opt);
  mjv_defaultScene(&scn);
  mjr_defaultContext(&con);

  // create scene and context
  mjv_makeScene(m, &scn, 2000);
  mjr_makeContext(m, &con, mjFONTSCALE_150);

  // install GLFW mouse and keyboard callbacks
  glfwSetKeyCallback(window, keyboard);
  glfwSetCursorPosCallback(window, mouse_move);
  glfwSetMouseButtonCallback(window, mouse_button);
  glfwSetScrollCallback(window, scroll);
  
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplGlfw_SetCallbacksChainForAllWindows(true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // run main loop, target real-time simulation and 60 fps rendering
  while (!glfwWindowShouldClose(window)) {
    // process pending GUI events, call GLFW callbacks
    glfwPollEvents();
	  
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // advance interactive simulation for 1/60 sec
    //  Assuming MuJoCo can simulate faster than real-time, which it usually can,
    //  this loop will finish on time for the next frame to be rendered at 60 fps.
    //  Otherwise add a cpu timer and exit this loop when it is time to render.
    mjtNum simstart = d->time;
    while (d->time - simstart < 1.0/60.0) {
      mj_step(m, d);
    }

    // get framebuffer viewport
    mjrRect viewport = {0, 0, 0, 0};
    glfwGetFramebufferSize(window, &viewport.width, &viewport.height);

    // update scene and render
    mjv_updateScene(m, d, &opt, NULL, &cam, mjCAT_ALL, &scn);
    mjr_render(viewport, &scn, &con);

    if(ImGui::Begin("My Window")){

    // convenience struct to manage DND items; do this however you like
    struct MyDndItem {
        int              Idx;
        int              Plt;
        ImAxis           Yax;
        char             Label[16];
        ImVector<ImVec2> Data;
        ImVec4           Color;
        MyDndItem()        {
            static int i = 0;
            Idx = i++;
            Plt = 0;
            Yax = ImAxis_Y1;
            snprintf(Label, sizeof(Label), "%02d Hz", Idx+1);
            Color = RandomColor();
            Data.reserve(1001);
            for (int k = 0; k < 1001; ++k) {
                float t = k * 1.0f / 999;
                Data.push_back(ImVec2(t, 0.5f + 0.5f * sinf(2*3.14f*t*(Idx+1))));
            }
        }
        void Reset() { Plt = 0; Yax = ImAxis_Y1; }
    };

    const int         k_dnd = 20;
    static MyDndItem  dnd[k_dnd];
    static MyDndItem* dndx = NULL; // for plot 2
    static MyDndItem* dndy = NULL; // for plot 2

    // child window to serve as initial source for our DND items
    ImGui::BeginChild("DND_LEFT",ImVec2(100,400));
    if (ImGui::Button("Reset Data")) {
        for (int k = 0; k < k_dnd; ++k)
            dnd[k].Reset();
        dndx = dndy = NULL;
    }
    for (int k = 0; k < k_dnd; ++k) {
        if (dnd[k].Plt > 0)
            continue;
        ImPlot::ItemIcon(dnd[k].Color); ImGui::SameLine();
        ImGui::Selectable(dnd[k].Label, false, 0, ImVec2(100, 0));
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("MY_DND", &k, sizeof(int));
            ImPlot::ItemIcon(dnd[k].Color); ImGui::SameLine();
            ImGui::TextUnformatted(dnd[k].Label);
            ImGui::EndDragDropSource();
        }
    }
    ImGui::EndChild();
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
            int i = *(int*)payload->Data; dnd[i].Reset();
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    ImGui::BeginChild("DND_RIGHT",ImVec2(-1,400));
    // plot 1 (time series)
    ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoHighlight;
    if (ImPlot::BeginPlot("##DND1", ImVec2(-1,195))) {
        ImPlot::SetupAxis(ImAxis_X1, NULL, flags|ImPlotAxisFlags_Lock);
        ImPlot::SetupAxis(ImAxis_Y1, "[drop here]", flags);
        ImPlot::SetupAxis(ImAxis_Y2, "[drop here]", flags|ImPlotAxisFlags_Opposite);
        ImPlot::SetupAxis(ImAxis_Y3, "[drop here]", flags|ImPlotAxisFlags_Opposite);

        for (int k = 0; k < k_dnd; ++k) {
            if (dnd[k].Plt == 1 && dnd[k].Data.size() > 0) {
                ImPlot::SetAxis(dnd[k].Yax);
                ImPlot::SetNextLineStyle(dnd[k].Color);
                ImPlot::PlotLine(dnd[k].Label, &dnd[k].Data[0].x, &dnd[k].Data[0].y, dnd[k].Data.size(), 0, 0, 2 * sizeof(float));
                // allow legend item labels to be DND sources
                if (ImPlot::BeginDragDropSourceItem(dnd[k].Label)) {
                    ImGui::SetDragDropPayload("MY_DND", &k, sizeof(int));
                    ImPlot::ItemIcon(dnd[k].Color); ImGui::SameLine();
                    ImGui::TextUnformatted(dnd[k].Label);
                    ImPlot::EndDragDropSource();
                }
            }
        }
        // allow the main plot area to be a DND target
        if (ImPlot::BeginDragDropTargetPlot()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dnd[i].Plt = 1; dnd[i].Yax = ImAxis_Y1;
            }
            ImPlot::EndDragDropTarget();
        }
        // allow each y-axis to be a DND target
        for (int y = ImAxis_Y1; y <= ImAxis_Y3; ++y) {
            if (ImPlot::BeginDragDropTargetAxis(y)) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                    int i = *(int*)payload->Data; dnd[i].Plt = 1; dnd[i].Yax = y;
                }
                ImPlot::EndDragDropTarget();
            }
        }
        // allow the legend to be a DND target
        if (ImPlot::BeginDragDropTargetLegend()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dnd[i].Plt = 1; dnd[i].Yax = ImAxis_Y1;
            }
            ImPlot::EndDragDropTarget();
        }
        ImPlot::EndPlot();
    }
    // plot 2 (Lissajous)
    if (ImPlot::BeginPlot("##DND2", ImVec2(-1,195))) {
        ImPlot::PushStyleColor(ImPlotCol_AxisBg, dndx != NULL ? dndx->Color : ImPlot::GetStyle().Colors[ImPlotCol_AxisBg]);
        ImPlot::SetupAxis(ImAxis_X1, dndx == NULL ? "[drop here]" : dndx->Label, flags);
        ImPlot::PushStyleColor(ImPlotCol_AxisBg, dndy != NULL ? dndy->Color : ImPlot::GetStyle().Colors[ImPlotCol_AxisBg]);
        ImPlot::SetupAxis(ImAxis_Y1, dndy == NULL ? "[drop here]" : dndy->Label, flags);
        ImPlot::PopStyleColor(2);
        if (dndx != NULL && dndy != NULL) {
            ImVec4 mixed((dndx->Color.x + dndy->Color.x)/2,(dndx->Color.y + dndy->Color.y)/2,(dndx->Color.z + dndy->Color.z)/2,(dndx->Color.w + dndy->Color.w)/2);
            ImPlot::SetNextLineStyle(mixed);
            ImPlot::PlotLine("##dndxy", &dndx->Data[0].y, &dndy->Data[0].y, dndx->Data.size(), 0, 0, 2 * sizeof(float));
        }
        // allow the x-axis to be a DND target
        if (ImPlot::BeginDragDropTargetAxis(ImAxis_X1)) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dndx = &dnd[i];
            }
            ImPlot::EndDragDropTarget();
        }
        // allow the x-axis to be a DND source
        if (dndx != NULL && ImPlot::BeginDragDropSourceAxis(ImAxis_X1)) {
            ImGui::SetDragDropPayload("MY_DND", &dndx->Idx, sizeof(int));
            ImPlot::ItemIcon(dndx->Color); ImGui::SameLine();
            ImGui::TextUnformatted(dndx->Label);
            ImPlot::EndDragDropSource();
        }
        // allow the y-axis to be a DND target
        if (ImPlot::BeginDragDropTargetAxis(ImAxis_Y1)) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dndy = &dnd[i];
            }
            ImPlot::EndDragDropTarget();
        }
        // allow the y-axis to be a DND source
        if (dndy != NULL && ImPlot::BeginDragDropSourceAxis(ImAxis_Y1)) {
            ImGui::SetDragDropPayload("MY_DND", &dndy->Idx, sizeof(int));
            ImPlot::ItemIcon(dndy->Color); ImGui::SameLine();
            ImGui::TextUnformatted(dndy->Label);
            ImPlot::EndDragDropSource();
        }
        // allow the plot area to be a DND target
        if (ImPlot::BeginDragDropTargetPlot()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dndx = dndy = &dnd[i];
            }
        }
        // allow the plot area to be a DND source
        if (ImPlot::BeginDragDropSourcePlot()) {
            ImGui::TextUnformatted("Yes, you can\ndrag this!");
            ImPlot::EndDragDropSource();
        }
        ImPlot::EndPlot();
    }
    ImGui::EndChild();
    ImGui::End();
    }

    if(ImGui::Begin("My Window"))
    {
      static ScrollingBuffer sdata2;
      static float t = 0;
      t += ImGui::GetIO().DeltaTime;
      // t += (float) d->time;
      sdata2.AddPoint(t, (float)d->qvel[5]);

      static float history = 10.0f;
      ImGui::SliderFloat("History",&history,1,30,"%.1f s");

      static ImPlotAxisFlags flags = ImPlotAxisFlags_AutoFit;
      //  = ImPlotAxisFlags_NoTickLabels;

      if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1,150))) {
        ImPlot::SetupAxes("time (sec)", "position (rad)", flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1,t - history, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1,-1,1);
        // ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL,0.5f);
        ImPlot::PlotLine("qpos 6", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), 0, sdata2.Offset, 2*sizeof(float));
        ImPlot::EndPlot();
      }
      ImGui::End();
    }

	  // Rendering
    ImGui::Render();
    // int display_w, display_h;
    glfwGetFramebufferSize(window, &viewport.width, &viewport.height);
    // glViewport(0, 0, &viewport.width, display_h);
    // glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    // glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
		
    // swap OpenGL buffers (blocking call due to v-sync)
    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  //free visualization storage
  mjv_freeScene(&scn);
  mjr_freeContext(&con);

  // free MuJoCo model and data
  mj_deleteData(d);
  mj_deleteModel(m);

  // terminate GLFW (crashes with Linux NVidia drivers)
#if defined(__APPLE__) || defined(_WIN32)
  glfwTerminate();
#endif

  return 1;
}