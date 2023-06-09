#include "Application.h"
#include <Windows.h>

int main(int argc, char** argv)
{
    Photoxel::Application* app = new Photoxel::Application();
    app->Run();
    delete app;
}

int WINAPI WinMain(
    HINSTANCE _In_ hInstance, 
    HINSTANCE _In_opt_ hPrev, 
    LPSTR _In_ lpszCmdLine, 
    int _In_ nCmdShow
) 
{
    Photoxel::Application* app = new Photoxel::Application();
    app->Run();
    delete app;
}