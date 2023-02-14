#include "Application.h"

int main() {
    Photoxel::Application* app = new Photoxel::Application();
    app->Run();
    delete app;
}