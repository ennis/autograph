#include <fstream>
#include <iostream>
#include <unordered_map>

#include <format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <rxcpp/rx.hpp>

#include <rxcpp/rx-subjects.hpp>

#include <autograph/backend/opengl/backend.hpp>
#include <autograph/device.hpp>
#include <autograph/draw.hpp>
#include <autograph/pipeline.hpp>
#include <autograph/pixel_format.hpp>
#include <autograph/surface.hpp>

#include <extra/image_io/load_image.hpp>
#include <extra/input/input.hpp>
#include <extra/input/input_glfw.hpp>

#include "../common/sample.hpp"
#include "../common/uniforms.hpp"


using GL = ag::opengl::OpenGLBackend;

namespace input = ag::extra::input;

class InputSample : public samples::GLSample<InputSample> {
public:
  InputSample(unsigned width, unsigned height)
      : GLSample(width, height, "Input"), last_key(0)
  {
    glfw_input_source = std::make_unique<input::GLFWInputEventSource>(gl.getWindow());
    texDefault = loadTexture2D("common/img/tonberry.jpg");

    the_a_key = input->keys().filter([](auto k) { return k == GLFW_KEY_A; });
    the_a_key.subscribe([](auto k) { fmt::print("Pressed A!\n"); });
    input->mouse_pos().subscribe(
        [](auto v) { fmt::print("Mouse pos: {},{}\n", v.x, v.y); });
    last_key = input->key_state(GLFW_KEY_3);
    // fmt::print("Mouse pos: {} {}", input->mouse_pos().;
  }

  void render() {
    using namespace glm;
    samples::uniforms::Object objectData;
    auto out = device->getOutputSurface();
    ag::clear(*device, out, ag::ClearColor{1.0f, 0.0f, 1.0f, 1.0f});
    copyTex(texDefault, out, width, height, {20, 20}, 1.0);
    frame_call = 0;
    fmt::print("Last key is: {}\n", last_key.get_value());
  }

private:
  rxcpp::observable<int> the_a_key;
  rxcpp::subjects::behavior<int> last_key;
  input::Input input;
  std::unique_ptr<input::GLFWInputEventSource> glfw_input_source;
  ag::Texture2D<ag::RGBA8, GL> texDefault;
};

int main() {
  InputSample sample(1000, 800);
  return sample.run();
}
