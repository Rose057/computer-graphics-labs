#include <cstdint>
#include <climits>

#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include "graphics_internal.hpp"
#include "application.hpp"

namespace {

constexpr int32_t default_window_width = 1280;
constexpr int32_t default_window_height = 720;

constexpr char default_window_title[] = "Vulkan Starter App";

GLFWwindow* glfw_window;

} // namespace

int main() {
	int status = EXIT_SUCCESS;

	// инициализация GLFM
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW\n";
		return EXIT_FAILURE;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// создание окна
	glfw_window = glfwCreateWindow(default_window_width, default_window_height,
	                               default_window_title, nullptr, nullptr);
	if (glfw_window == nullptr) {
		status = EXIT_FAILURE;
		goto err_null_window;
	}

	// изменение размера окна
	glfwSetFramebufferSizeCallback(glfw_window, [](GLFWwindow*, int width, int height){
		if (width == 0 || height == 0) {
			return;
		}

		graphics::internal::resize(width, height);
	});

	// инициализация ImGUI
	if (ImGui::CreateContext() == nullptr) {
		std::cerr << "Failed to create ImGUI context\n";
		status = EXIT_FAILURE;
		goto err_imgui_init;
	}

	if (!ImGui_ImplGlfw_InitForVulkan(glfw_window, true)) {
		std::cerr << "Failed to initialize ImGUI GLFW backend for Vulkan renderer\n";
		status = EXIT_FAILURE;
		goto err_imgui_glfw_init;
	}

	// инициализация Vulkan (создание instance, device, render pass и тд)
	if (!graphics::internal::initialize(glfw_window)) {
		std::cerr << "Failed to initialize graphics\n";
		status = EXIT_FAILURE;
		goto err_graphics_init;
	}

	// инициализация приложения (создание вершинных/индексных/uniform буферов, дескрипторов, пайплайнов)
	if (!application::initialize()) {
		std::cerr << "Failed to initialize application\n";
		status = EXIT_FAILURE;
		goto err_application_init;
	}

	// главный цикл
	while (!glfwWindowShouldClose(glfw_window)) {
		const double time = glfwGetTime(); // для анимации (время в сек от старта GLFM)

		glfwPollEvents(); // обработка событий окна (закрытие, ввод, ресайз)
		ImGui_ImplGlfw_NewFrame(); // сообщение о новом кадре

		ImGui::NewFrame(); // начало нового кадра
		application::update(time); // рисование UI, обновление матрицы в uniform-буферах
		ImGui::Render(); // рендеринг кадра

		// получение framebuffer и command buffer текущего кадра
		graphics::internal::FrameData fd = graphics::internal::prepare();
		application::render(fd); // запись команды отрисовки в fd.command_buffer
		graphics::internal::submitAndPresent(); // подготовка command buffers и отправка
	}

	// очистка ресурсов
	application::shutdown();
err_application_init:
	graphics::internal::shutdown();
err_graphics_init:
	ImGui_ImplGlfw_Shutdown();
err_imgui_glfw_init:
	ImGui::DestroyContext();
err_imgui_init:
	glfwDestroyWindow(glfw_window);
err_null_window:
	glfwTerminate();

	return 0;
}