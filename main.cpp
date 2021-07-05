// STL
#include <string>
#include <cstdlib>
#include <fstream>
// UNIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
// fmt
#include <fmt/color.h>
#include <fmt/printf.h>
// imgui
#include <TextEditor.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
// SDL2
#include <SDL2/SDL.h>
// glbinding
#define GLFW_INCLUDE_NONE
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
using namespace gl;

namespace SDL2 {
inline constexpr uint32_t SUCCESS = 0;
} // namespace SDL2

static int CreateGUI(int frontendToGDB[2], int gdbToFrontend[2], pid_t childProcess) {
  int pidStatus;
  if(waitpid(childProcess, &pidStatus, WNOHANG) == childProcess) {
    int wout = write(STDOUT_FILENO, "Gdb exitted.", 11);
    (void)wout;
    return EXIT_FAILURE;
  }

  if (SDL_Init(SDL_INIT_VIDEO) != SDL2::SUCCESS) {
    fmt::print(fg(fmt::color::red), "ERROR: Can not initialize SDL2\n");
    return EXIT_FAILURE;
  }

  const char *glsl_version = "#version 130";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  // Create window with graphics context
  // SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  auto *pWindow =
      SDL_CreateWindow("GdbImgui", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
  if (pWindow == nullptr) {
    fmt::print(fg(fmt::color::red), "ERROR: Can not create SDL2 window\n");
    return EXIT_FAILURE;
  }
  auto context = SDL_GL_CreateContext(pWindow);

  SDL_GL_MakeCurrent(pWindow, context);
  SDL_GL_SetSwapInterval(1);

  glbinding::initialize(
      [](const char *name) {
        return reinterpret_cast<glbinding::ProcAddress>(
            SDL_GL_GetProcAddress(name));
      },
      false);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(pWindow, context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  TextEditor editor;
  auto lang = TextEditor::LanguageDefinition::CPlusPlus();
  editor.SetLanguageDefinition(lang);

  // Close stdin
  //close(fd[STDIN_FILENO]);
  constexpr size_t CommandSize = 512;
  char command[CommandSize] = {};

  constexpr size_t HistorySize = 512;
  char history[HistorySize] = {};

  bool bRunning = true;
  while (bRunning) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) {
        bRunning = false;
      }
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(pWindow)) {
        bRunning = false;
      }
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(pWindow);
    ImGui::NewFrame();

    {
      // auto cpos = editor.GetCursorPosition();
      ImGui::Begin("Text Editor Demo", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
      ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
      if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
          if (ImGui::MenuItem("Save")) {
            auto textToSave = editor.GetText();
            /// save text....
          }
          if (ImGui::MenuItem("Quit", "Alt-F4"))
            break;
          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
          bool ro = editor.IsReadOnly();
          if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
            editor.SetReadOnly(ro);
          ImGui::Separator();

          if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr,
                              !ro && editor.CanUndo()))
            editor.Undo();
          if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr,
                              !ro && editor.CanRedo()))
            editor.Redo();

          ImGui::Separator();

          if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
            editor.Copy();
          if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr,
                              !ro && editor.HasSelection()))
            editor.Cut();
          if (ImGui::MenuItem("Delete", "Del", nullptr,
                              !ro && editor.HasSelection()))
            editor.Delete();
          if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr,
                              !ro && ImGui::GetClipboardText() != nullptr))
            editor.Paste();

          ImGui::Separator();

          if (ImGui::MenuItem("Select all", nullptr, nullptr))
            editor.SetSelection(
                TextEditor::Coordinates(),
                TextEditor::Coordinates(editor.GetTotalLines(), 0));

          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
          if (ImGui::MenuItem("Dark palette"))
            editor.SetPalette(TextEditor::GetDarkPalette());
          if (ImGui::MenuItem("Light palette"))
            editor.SetPalette(TextEditor::GetLightPalette());
          if (ImGui::MenuItem("Retro blue palette"))
            editor.SetPalette(TextEditor::GetRetroBluePalette());
          ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
      }

      editor.Render("TextEditor");

      read(gdbToFrontend[0], history, HistorySize);
      //write(s_frontend_to_gdb[1], exitString, (strlen(exitString) + 1));

      ImGui::Begin("GDB", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
        ImGui::Text("%s", history);
        fmt::print("History: {}\n", history);
        if(ImGui::InputText("$", command, CommandSize)) {
          fmt::print("Command: {}\n", command);
          // write(s_frontend_to_gdb[1], command, (strlen(command) + 1));
        }
      ImGui::End();

      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    glViewport(0, 0, static_cast<int>(io.DisplaySize.x),
               static_cast<int>(io.DisplaySize.y));
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(pWindow);
  }

  fmt::print("1. Quit\n");

  const char *exitString = "quit\n";
  write(frontendToGDB[1], exitString, (strlen(exitString) + 1));
  // int wout = write(s_frontend_to_gdb[1], s_cmd_buff, written);

  fmt::print("2. Quit\n");

  fmt::print("1. waitpid\n");

  int cstatus;
  waitpid(childProcess, &cstatus, 0);

  fmt::print("2. waitpid\n");

  // Close stdin
  // close(fd[0]);
  // Close stdout
  //close(fd[1]);

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(pWindow);
  SDL_Quit();
  return EXIT_SUCCESS;
}

[[noreturn]] static void DebugProcess(int frontendToGDB[2], int gdbToFrontend[2]) {
  dup2(frontendToGDB[0], STDIN_FILENO);
  dup2(gdbToFrontend[1], STDOUT_FILENO);

  char *argsGDB[] = {"/usr/bin/gdb", "--interpreter=mi", "--quiet", nullptr};
  execvp(argsGDB[0], argsGDB);

  _exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fmt::print(stderr, fg(fmt::color::yellow), "Usage: {} exec_path args\n",
               argv[0]);
    return EXIT_FAILURE;
  }

  int frontendToGDB[2];
  int gdbToFrontend[2];
  if(pipe(frontendToGDB) != 0 &&
     pipe(gdbToFrontend) != 0) {
    fmt::print(stderr, fg(fmt::color::red), "Can not create pipe\n");
    return EXIT_FAILURE;
  }

  // frontend
  int fstate = fcntl(frontendToGDB[1], F_GETFL, 0);
  fstate     = fstate | O_NONBLOCK;
  fcntl(frontendToGDB[1], F_SETFL, fstate);

  fstate = fcntl(gdbToFrontend[0], F_GETFL, 0);
  fstate = fstate | O_NONBLOCK;
  fcntl(gdbToFrontend[0], F_SETFL, fstate);

  // gdb
  fstate = fcntl(frontendToGDB[0], F_GETFL, 0);
  fstate = fstate | O_NONBLOCK;
  fcntl(frontendToGDB[0], F_SETFL, fstate);

  const auto processID = fork();
  switch (processID) {
  case -1:
    fmt::print(stderr, fg(fmt::color::red),
               "ERROR: Can not create a new process\n.");
    return EXIT_FAILURE;
  case 0: // Child
    DebugProcess(frontendToGDB, gdbToFrontend);
    return EXIT_SUCCESS;
  }
  return CreateGUI(frontendToGDB, gdbToFrontend, processID);
}
