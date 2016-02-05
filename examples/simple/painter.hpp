#ifndef PAINTER_HPP
#define PAINTER_HPP

#include <fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

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

#include "brush_path.hpp"
#include "canvas.hpp"
#include "pipelines.hpp"
#include "ui.hpp"

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
    input = std::make_unique<input::Input>();
    input->registerEventSource(
        std::make_unique<input::GLFWInputEventSource>(gl.getWindow()));
    ui = std::make_unique<Ui>(gl.getWindow(), *input);
    texBrushTip = image_io::loadTexture2D(
        *device, (samplesRoot / kDefaultBrushTip).str().c_str());
    samples::Vertex2D vboQuadData[] = {
        {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0, 1.0},
        {1.0f, 0.0f, 1.0, 0.0},   {0.0f, 1.0f, 0.0, 1.0},
        {1.0f, 1.0f, 1.0, 1.0},   {1.0f, 0.0f, 1.0, 0.0},
    };
    vboQuad = device->createBuffer(vboQuadData);
    setupInput();
  }

  void makeSceneData() {
    const auto aspectRatio = (float)canvas->width / (float)canvas->height;
    const auto eyePos = glm::vec3(0, 2, -3);
    const auto lookAt =
        glm::lookAt(eyePos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    const auto proj = glm::perspective(45.0f, aspectRatio, 0.01f, 100.0f);
    uniforms::Scene scene;
    scene.viewMatrix = lookAt;
    scene.projMatrix = proj;
    scene.viewProjMatrix = proj * lookAt;
    scene.viewportSize.x = (float)canvas->width;
    scene.viewportSize.y = (float)canvas->height;
    sceneData = device->pushDataToUploadBuffer(scene);
  }

  void makeCanvasData() {
    uniforms::CanvasData uCanvasData;
    uCanvasData.size.x = (float)canvas->width;
    uCanvasData.size.y = (float)canvas->height;
    canvasData = device->pushDataToUploadBuffer(uCanvasData);
  }

  void render() {
    using namespace glm;
    makeSceneData();
    makeCanvasData();
    auto out = device->getOutputSurface();
    ag::clear(*device, out, ag::ClearColor{1.0f, 0.0f, 1.0f, 1.0f});
    ui->render(*device);
  }

  void onBrushPointerEvent(const BrushProperties& props, unsigned x,
                           unsigned y) {
    brushPath.addPointerEvent(
        PointerEvent{x, y, 1.0f}, props,
        [this](auto splat) { this->drawBrushSplat(*canvas, splat); });
  }

  void setupInput() {
    // on key presses
    input->keys().subscribe(
        [this](auto ev) { fmt::print("Key event: {}\n", (int)ev.code); });
    // on mouse button clicks
    ui->canvasMouseButtons.subscribe([this](auto ev) {
      if (ev.button == GLFW_MOUSE_BUTTON_LEFT &&
          ev.state == input::MouseButtonState::Pressed) {
        isMakingStroke = true;   // go into stroke mode
        brushPath = BrushPath(); // reset brush path
        unsigned x, y;
        ui->getPointerPosition(x, y);
        onBrushPointerEvent(this->brushPropsFromUi(), x, y);
        // beginStroke();
      } else if (ev.button == GLFW_MOUSE_BUTTON_LEFT &&
                 ev.state == input::MouseButtonState::Released &&
                 isMakingStroke) {
        unsigned x, y;
        ui->getPointerPosition(x, y);
        onBrushPointerEvent(this->brushPropsFromUi(), x, y);
        isMakingStroke = false; // end stroke mode
      }
    });

    // on mouse move
    ui->canvasMousePointer.subscribe([this](auto ev) {
      // brush tool selected
      if (isMakingStroke == true && ui->activeTool == Tool::Brush) {
        // on mouse move, add splats with the current brush
        onBrushPointerEvent(this->brushPropsFromUi(), ev.positionX,
                            ev.positionY);
      } else {
        // TODO other tools (blur, smudge, etc.)
      }
    });
  }

  BrushProperties brushPropsFromUi() {
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
    return props;
  }

  void drawBrushSplat(Canvas& canvas, const SplatProperties& splat) {
    fmt::print(std::clog, "splat(({},{}),{},{})\n", splat.center.x,
               splat.center.y, splat.width, splat.rotation);
    uniforms::Splat uSplat;
    glm::vec2 scale{1.0f};
    if (ui->brushTip == BrushTip::Textured)
      if (texBrushTip.info.dimensions.x < texBrushTip.info.dimensions.y)
        scale = glm::vec2{1.0f, (float)texBrushTip.info.dimensions.y /
                                    (float)texBrushTip.info.dimensions.x};
      else
        scale = glm::vec2{(float)texBrushTip.info.dimensions.x /
                              (float)texBrushTip.info.dimensions.y,
                          1.0f};
    scale *= splat.width;

    uSplat.center = splat.center;
    uSplat.transform = glm::mat2x3(
        glm::scale(glm::rotate(glm::translate(glm::mat3x3(1.0f), splat.center),
                               splat.rotation),
                   scale));
    if (ui->brushTip == BrushTip::Round)
      ag::draw(*device, canvas.texMask, pipelines->ppDrawRoundSplatToStrokeMask,
               ag::DrawArrays(ag::PrimitiveType::Triangles, vboQuad), canvasData, uSplat);
    else if (ui->brushTip == BrushTip::Textured)
      ag::draw(
          *device, canvas.texMask, pipelines->ppDrawTexturedSplatToStrokeMask,
		  ag::DrawArrays(ag::PrimitiveType::Triangles, vboQuad),
          ag::TextureUnit(0, texBrushTip, samLinearClamp), canvasData, uSplat);
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
  std::unique_ptr<input::Input> input;
  // brush tip texture
  Texture2D<ag::RGBA8> texBrushTip;
  // is the user making a stroke
  bool isMakingStroke;
  // active tool
  Tool activeTool;

  Buffer<samples::Vertex2D[]> vboQuad;

  ///////////
  RawBufferSlice canvasData;
  RawBufferSlice sceneData;

  /////////// Brush tool state
  // current brush path
  BrushPath brushPath;
};

#endif
