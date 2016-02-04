#ifndef PAINTER_HPP
#define PAINTER_HPP

#include <fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <autograph/backend/opengl/backend.hpp>
#include <autograph/device.hpp>
#include <autograph/draw.hpp>
#include <autograph/pipeline.hpp>
#include <autograph/pixel_format.hpp>
#include <autograph/surface.hpp>

#include <extra/input/input.hpp>
#include <extra/input/input_glfw.hpp>
#include <extra/image_io/load_image.hpp>
#include <extra/input/input.hpp>
#include <extra/input/input_glfw.hpp>

#include "../common/sample.hpp"
#include "../common/uniforms.hpp"

#include "pipelines.hpp"
#include "canvas.hpp"
#include "ui.hpp"
#include "brush_path.hpp"

namespace input = ag::extra::input;
namespace image_io = ag::extra::image_io;

static constexpr const char kDefaultBrushTip[] = "simple/img/brush_tip.png";

class Painter : public samples::GLSample<Painter> {
public:
  Painter(unsigned width, unsigned height)
      : GLSample(width, height, "Painter") {
    pipelines = std::make_unique<Pipelines>(*device, samplesRoot);
    // 1000x1000 canvas
    canvas = std::make_unique<Canvas>(*device, 1000, 1000);
<<<<<<< HEAD
    input = std::make_unique<input::Input>();
    input->registerEventSource(
        std::make_unique<input::GLFWInputEventSource>(gl.getWindow()));
    ui = std::make_unique<Ui>(gl.getWindow(), *input);
    texBrushTip = image_io::loadTexture2D(
        *device, (samplesRoot / kDefaultBrushTip).str().c_str());
=======
    input = std::make_unique<ag::extra::input::Input>();
    input->registerEventSource(
        std::make_unique<ag::extra::input::GLFWInputEventSource>(gl.getWindow()));
    ui = std::make_unique<Ui>(gl.getWindow(), input);
>>>>>>> 25fe19c9b14fafc15101894b880361787addcac6
  }

  auto makeSceneData() {
    const auto aspectRatio = (float)canvas->width / (float)canvas->height;
    const auto eyePos = glm::vec3(0, 2, -3);
<<<<<<< HEAD
    const auto lookAt =
        glm::lookAt(eyePos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
=======
    const auto lookAt = glm::lookAt(eyePos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
>>>>>>> 25fe19c9b14fafc15101894b880361787addcac6
    const auto proj = glm::perspective(45.0f, aspectRatio, 0.01f, 100.0f);
    uniforms::Scene scene;
    scene.viewMatrix = lookAt;
    scene.projMatrix = proj;
    scene.viewProjMatrix = proj * lookAt;
    scene.viewportSize.x = (float)canvas->width;
    scene.viewportSize.y = (float)canvas->height;
    return device->pushDataToUploadBuffer(scene);
  }

  void render() {
    using namespace glm;
    auto out = device->getOutputSurface();
    ag::clear(*device, out, ag::ClearColor{1.0f, 0.0f, 1.0f, 1.0f});
  }

  void setupInput() {
    // on mouse button clicks
    ui->canvasMouseButtons.subscribe([this](auto ev) {
      if (ev.button == GLFW_MOUSE_BUTTON_LEFT && ev.state == input::MouseButtonState::Pressed) {
        isMakingStroke = true; // go into stroke mode
        brushPath = BrushPath(); // reset brush path
        // beginStroke();
      } else if (ev.button == GLFW_MOUSE_BUTTON_LEFT &&
                 ev.state == input::MouseButtonState::Released) {
        isMakingStroke = false; // end stroke mode
        // endStroke();
      }
    });

    // on mouse move
    ui->canvasMousePointer.subscribe([this](auto ev) {
      // brush tool selected
      if (isMakingStroke == true && ui->activeTool == Tool::Brush) {
        auto props = this->brushPropsFromUi();
        // on mouse move, add splats with the current brush
        brushPath.addPointerEvent(ev, props, [this](auto splat) {
          this->drawBrushSplat(*canvas, splat);
        });
      } else {
        // TODO other tools (blur, smudge, etc.)
      }
    });
  }

  BrushProperties brushPropsFromUi()
  {
      BrushProperties props;
      props.color[0] = ui->strokeColor[0];
      props.color[1] = ui->strokeColor[1];
      props.color[2] = ui->strokeColor[2];
      props.opacity = ui->strokeOpacity;
      props.opacityJitter = ui->strokeOpacityJitter;
      props.width = ui->strokeWidth;
      props.widthJitter = ui->brushWidthJitter;
      props.spacing = ui->brushSpacing;
      props.spacingJitter = ui->brushSpacingJitter;
      props.rotation = 0.0f;
      props.rotationJitter = ui->brushRotationJitter;
  }

  void drawBrushSplat(Canvas &canvas, const SplatProperties &pos) {
    if (ui->brushTip == BrushTip::Round)
      ag::draw(*device, pipelines->ppDrawRoundSplatToStrokeMask, ...);
    else if (ui->brushTip == BrushTip::Textured)
      ag::draw(*device, pipelines->ppDrawTexturedSplatToStrokeMask, ...);
  }

  // create textures
  // load pipelines

  // render normal map
  // render isolines

private:
  // shaders
  std::unique_ptr<Pipelines> pipelines;
  // canvas
  std::unique_ptr<Canvas> canvas;
  // mesh
  std::unique_ptr<Mesh> mesh;
  // UI
  std::unique_ptr<Ui> ui;
  // input
<<<<<<< HEAD
  std::unique_ptr<input::Input> input;
  // brush tip texture
  Texture2D<ag::RGBA8> texBrushTip;
=======
  std::unique_ptr<ag::extra::input::Input> input;
>>>>>>> 25fe19c9b14fafc15101894b880361787addcac6

  // is the user making a stroke
  bool isMakingStroke;
  // active tool
  Tool activeTool;

  /////////// Brush tool state
  // current brush path
  BrushPath brushPath;
};

#endif
