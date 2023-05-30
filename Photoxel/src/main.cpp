#include "Application.h"
#include <Windows.h>

int main() {
    Photoxel::Application* app = new Photoxel::Application();
    app->Run();
    delete app;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpszCmdLine, int nCmdShow) {
    Photoxel::Application* app = new Photoxel::Application();
    app->Run();
    delete app;
}