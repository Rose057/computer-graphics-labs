#pragma once

#include "graphics_internal.hpp"

#include <glm/glm.hpp>

namespace application {
    // данные одной вершины
    struct Vertex {
        glm::vec3 position; // локальная позиция x, y, z
        glm::vec3 color; // цвет
    };

    bool initialize(); // создает все ресурсы
    void shutdown(); // уничтожает все созданные ресурсы

    void update(double time); // вызывает каждый кадр до отрисовки (для UI и матриц)
    void render(const graphics::internal::FrameData& fd); // вызывается каждый кадр после update, для команд отрисовки

} // namespace application