#include <fstream>
#include <iostream>

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

#include "../common/sample.hpp"
#include "../common/uniforms.hpp"

using GL = ag::opengl::OpenGLBackend;

class InputSample : public samples::GLSample<InputSample> {
public:
  InputSample(unsigned width, unsigned height)
      : GLSample(width, height, "Input") {
    texDefault = loadTexture2D("common/img/tonberry.jpg");

	auto key = rxcpp::observable<>::range(1, 10);
	key.subscribe([](int v) { fmt::print("next: {}\n", v); },
		[]() { fmt::print("End\n"); });

	auto sub = rxcpp::rxsub::subject<int>();
	auto sub_push = sub.get_subscriber();

	auto recv = sub.get_observable().subscribe([](auto v) {fmt::print("received: {}\n", v);});
	sub_push.on_next(5);
	sub_push.on_next(5);
	sub_push.on_next(4);
  }

  void render() {
    using namespace glm;
    samples::uniforms::Object objectData;
    auto out = device->getOutputSurface();
    ag::clear(*device, out, ag::ClearColor{1.0f, 0.0f, 1.0f, 1.0f});
    copyTex(texDefault, out, width, height, {20, 20}, 1.0);
  }

private:
  ag::Texture2D<ag::RGBA8, GL> texDefault;
};

int main() {
  InputSample sample(1000, 800);
  return sample.run();
}
