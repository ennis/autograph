#ifndef PAINTER_HPP
#define PAINTER_HPP

#include <fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

#include <autograph/backend/opengl/backend.hpp>
#include <autograph/compute.hpp>
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

constexpr const char kDefaultBrushTip[] = "simple/img/brush_tip.png";
constexpr unsigned kCSThreadGroupSizeX = 16;
constexpr unsigned kCSThreadGroupSizeY = 16;
constexpr unsigned kCSThreadGroupSizeZ = 1;

int roundUp(int numToRound, int multiple) {
  return ((numToRound + multiple - 1) / multiple) * multiple;
}

// Stroke task
// start on canvas touch event
// wait for canvas event
/*[[coroutine]]
void strokeTask(Device& device, Painter& painter, Canvas& canvas, Texture2D<RGBA8>& texStrokeMask, uvec2 position)
{
	BrushPath path;

	while (last_event is not mouse released)
	{
		auto ev = await(...);
		yield ag::draw(...);
	}
}*/

class Painter : public samples::GLSample<Painter> {
public:
  Painter(unsigned width, unsigned height)
      : GLSample(width, height, "Painter") {
    pipelines = std::make_unique<Pipelines>(*device, samplesRoot);
    // 1000x1000 canvas
    canvas = std::make_unique<Canvas>(*device, 1000, 1000);
    texEvalCanvas = device->createTexture2D<ag::RGBA8>(glm::uvec2{1000, 1000});
    input = std::make_unique<input::Input>();
    input->registerEventSource(
        std::make_unique<input::GLFWInputEventSource>(gl.getWindow()));
    ui = std::make_unique<Ui>(gl.getWindow(), *input);
    texBrushTip = image_io::loadTexture2D(
        *device, (samplesRoot / kDefaultBrushTip).str().c_str());
    samples::Vertex2D vboQuadData[] = {
        {-1.0f, -1.0f, 0.0f, 0.0f}, {-1.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, -1.0f, 1.0f, 0.0f},  {-1.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},   {1.0f, -1.0f, 1.0f, 0.0f},
    };
    vboQuad = device->createBuffer(vboQuadData);
    surfOut = device->getOutputSurface();
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
    sceneData = device->pushDataToUploadBuffer(
        scene, GL::kUniformBufferOffsetAlignment);
  }

  void makeCanvasData() {
    uniforms::CanvasData uCanvasData;
    uCanvasData.size.x = (float)canvas->width;
    uCanvasData.size.y = (float)canvas->height;
    canvasData = device->pushDataToUploadBuffer(
        uCanvasData, GL::kUniformBufferOffsetAlignment);
  }

  void render() {
    using namespace glm;
    ag::clear(*device, texEvalCanvas, ag::ClearColor{0.0f, 0.0f, 0.0f, 1.0f});
    ag::clearDepth(*device, surfOut, 1.0f);
    input->poll();
    makeSceneData();
    makeCanvasData();
    renderCanvas();
    ui->render(*device);
  }

  void renderCanvas() {
    copyTex(canvas->texStrokeMask, surfOut, 1000, 1000, glm::vec2{0.0f, 0.0f},
            1.0f);
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
    /*fmt::print(std::clog, "splat(({},{}),{},{})\n", splat.center.x,
               splat.center.y, splat.width, splat.rotation);*/
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
    uSplat.width = splat.width;
    uSplat.transform = glm::mat3x4(
        glm::scale(glm::rotate(glm::translate(glm::mat3x3(1.0f), splat.center),
                               splat.rotation),
                   scale));
    if (ui->brushTip == BrushTip::Round)
      ag::draw(*device, canvas.texStrokeMask,
               pipelines->ppDrawRoundSplatToStrokeMask,
               ag::DrawArrays(ag::PrimitiveType::Triangles, vboQuad),
               canvasData, uSplat);
    else if (ui->brushTip == BrushTip::Textured)
      ag::draw(*device, canvas.texStrokeMask,
               pipelines->ppDrawTexturedSplatToStrokeMask,
               ag::DrawArrays(ag::PrimitiveType::Triangles, vboQuad),
               ag::TextureUnit(0, texBrushTip, samLinearClamp), canvasData,
               uSplat);
  }

  void applyStroke(Canvas& canvas, const BrushProperties& brushProps) {
    ag::compute(*device, pipelines->ppFlattenStroke,
                ag::ThreadGroupCount{
                    (unsigned)roundUp(canvas.width, kCSThreadGroupSizeX),
                    (unsigned)roundUp(canvas.height, kCSThreadGroupSizeY), 1u},
                canvasData, glm::vec4{brushProps.color[0], brushProps.color[1],
                                      brushProps.color[2], brushProps.opacity},
                canvas.texStrokeMask,
                ag::RWTextureUnit(0, canvas.texHSVOffsetXY));
  }

  void evaluate(Canvas& canvas)
  {
	  //ag::compute(*device, pipelines->ppEvaluate,
  }

  // create textures
  // load pipelines

  // render normal map
  // render isolines

private:
  ag::Surface<GL, float, ag::RGBA8> surfOut;
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

  // evaluated canvas
  Texture2D<ag::RGBA8> texEvalCanvas;

  Buffer<samples::Vertex2D[]> vboQuad;

  ///////////
  RawBufferSlice canvasData;
  RawBufferSlice sceneData;

  /////////// Brush tool state
  // current brush path
  BrushPath brushPath;
};

#endif
