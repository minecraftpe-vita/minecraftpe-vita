#include <GLFW/glfw3.h>

#include "App.h"
#include "AppPlatform_Linux.h"
#include "platform/input/Multitouch.h"

static unsigned char transformKey(int key) {
	// Handle ALL keys here. If not handled -> return 0 ("invalid")
    if(key == GLFW_KEY_UP) return Keyboard::KEY_UP;
    if(key == GLFW_KEY_DOWN) return Keyboard::KEY_DOWN;
    if(key == GLFW_KEY_RIGHT) return Keyboard::KEY_RIGHT;
    if(key == GLFW_KEY_LEFT) return Keyboard::KEY_LEFT;
	if(key == GLFW_KEY_LEFT_SHIFT) return Keyboard::KEY_LSHIFT;
    if(key == GLFW_KEY_SPACE) return Keyboard::KEY_SPACE;
    if(key == GLFW_KEY_E) return Keyboard::KEY_E;
    if(key == GLFW_KEY_Q) return Keyboard::KEY_Q;
    if(key == GLFW_KEY_F) return Keyboard::KEY_F;
    if(key == GLFW_KEY_T) return Keyboard::KEY_T;
    if(key == GLFW_KEY_W) return Keyboard::KEY_W;
    if(key == GLFW_KEY_A) return Keyboard::KEY_A;
    if(key == GLFW_KEY_S) return Keyboard::KEY_S;
    if(key == GLFW_KEY_D) return Keyboard::KEY_D;
    if(key == GLFW_KEY_ESCAPE) return Keyboard::KEY_ESCAPE;
	if(key == GLFW_KEY_ENTER) return Keyboard::KEY_RETURN;
	if(key == GLFW_KEY_TAB) return Keyboard::KEY_TAB;
	if(key >= 'a' && key <= 'z') return key - 32;
	if(key >= GLFW_KEY_0 && key <= GLFW_KEY_9) return '0' + (key - GLFW_KEY_0);
	return 0;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    unsigned char transformed = transformKey(key);
    if(transformed == 0) return;
    if(action == GLFW_PRESS) Keyboard::feed(transformed, 1);
    if(action == GLFW_RELEASE) Keyboard::feed(transformed, 0);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    static double prev_x = 0;
    static double prev_y = 0;
    Multitouch::feed(0, 0, xpos, ypos, 0);
    Mouse::feed(0, 0, xpos, ypos, xpos - prev_x, ypos - prev_y);
    prev_x = xpos;
    prev_y = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    char mc_button = 0;
    if(button == GLFW_MOUSE_BUTTON_LEFT) mc_button = 1;
    if(button == GLFW_MOUSE_BUTTON_RIGHT) mc_button = 2;
    if(button == GLFW_MOUSE_BUTTON_MIDDLE) mc_button = 3;

    if(mc_button == 3) {
        if(action == GLFW_PRESS) {
            Mouse::feed(3, 1, xpos, ypos, 0, 1);
        }
        if(action == GLFW_RELEASE) {
            Mouse::feed(3, 0, xpos, ypos, 0, -1);
        }
    } else {
        if(action == GLFW_PRESS) {
            Mouse::feed(mc_button, 1, xpos, ypos);
            Multitouch::feed(mc_button, 1, xpos, ypos, 0);
        }
        if(action == GLFW_RELEASE) {
            Mouse::feed(mc_button, 0, xpos, ypos);
            Multitouch::feed(mc_button, 0, xpos, ypos, 0);
        }
    }
}

int main(int argc, char** argv) {
    if (!glfwInit())
    {
        // Handle initialization failure
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(854, 480, "Minecraft", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    MAIN_CLASS* app = new MAIN_CLASS();

	app->externalStoragePath = "savedata";
	app->externalCacheStoragePath = "savedata";

    AppContext context;
	AppPlatform_Linux platform(window);
	context.doRender = true;
	context.platform = &platform;

	app->init_ctx(context);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
	app->setSize(width, height);

	while (!glfwWindowShouldClose(window)) {
		app->update();
        glfwSwapBuffers(window);
        glfwPollEvents();
	}

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}