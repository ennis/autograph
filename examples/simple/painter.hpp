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
#include "camera.hpp"

#include <boost/filesystem.hpp>

namespace input = ag::extra::input;
namespace image_io = ag::extra::image_io;

constexpr const char kDefaultBrushTip[] = "simple/img/brushes/brush_tip.png";
constexpr unsigned kCSThreadGroupSizeX = 16;
constexpr unsigned kCSThreadGroupSizeY = 16;
constexpr unsigned kCSThreadGroupSizeZ = 1;

int divRoundUp(int numToRound, int multiple) {
  return (numToRound + multiple - 1) / multiple;
}

// Stroke task
// start on canvas touch event
// wait for canvas event
/*[[coroutine]]
void strokeTask(Device& device, Painter& painter, Canvas& canvas,
Texture2D<RGBA8>& texStrokeMask, uvec2 position)
{
        BrushPath path;

        while (last_event is not mouse released)
        {
                auto ev = await(...);
                yield ag::draw(...);
        }
}*/

namespace fs = boost::filesystem;

class Painter : public samples::GLSample<Painter> {
public:
  Painter(unsigned width, unsigned height)
      : GLSample(width, height, "Painter") {
    pipelines = std::make_unique<Pipelines>(*device, samplesRoot);
    // 1000x1000 canvas
    mesh = loadMesh("common/meshes/skull.obj");
    canvas = std::make_unique<Canvas>(*device, 1000, 1000);
    texEvalCanvas = device->createTexture2D<ag::RGBA8>(glm::uvec2{1000, 1000});
    input = std::make_unique<input::Input>();
    input->registerEventSource(
        std::make_unique<input::GLFWInputEventSource>(gl.getWindow()));
    ui = std::make_unique<Ui>(gl.getWindow(), *input);
    samples::Vertex2D vboQuadData[] = {
        {-1.0f, -1.0f, 0.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, -1.0f, 1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, -1.0f, 1.0f, 0.0f},
    };
    vboQuad = device->createBuffer(vboQuadData);
    surfOut = device->getOutputSurface();
    setupInput();
    loadBrushTips();
  }

  template <typename... Resources>
  void previewCanvas(Canvas& canvas, Texture2D<ag::RGBA8>& out,
                     ComputePipeline& pipeline, RawBufferSlice& canvasData,
                     Resources&&... resources) {
    ag::compute(
        *device, pipeline,
        ag::ThreadGroupCount{
            (unsigned)divRoundUp(canvas.width, kCSThreadGroupSizeX),
            (unsigned)divRoundUp(canvas.height, kCSThreadGroupSizeY), 1u},
        canvasData,
        ag::TextureUnit(0, canvas.texShadingProfileLN, samLinearClamp),
        ag::TextureUnit(1, canvas.texBlurParametersLN, samLinearClamp),
        ag::TextureUnit(2, canvas.texDetailMaskLN, samLinearClamp),
        ag::TextureUnit(3, canvas.texBaseColorUV, samLinearClamp),
        ag::TextureUnit(4, canvas.texHSVOffsetUV, samLinearClamp),
        ag::TextureUnit(5, canvas.texBlurParametersUV, samLinearClamp),
        ag::RWTextureUnit(0, out), std::forward<Resources>(resources)...);
  }

  // load brush tips from img directories
  void loadBrushTips() {
    using namespace fs;
    auto path1 = fs::path(samplesRoot.str().c_str()) / "simple/img/brushes";
    auto path2 = fs::path(samplesRoot.str().c_str()) / "common/img/brushes";

    if (exists(path1) && is_directory(path1))
      for (auto it = fs::directory_iterator(path1);
           it != fs::directory_iterator(); ++it)
        loadBrushTip((*it).path());

    if (exists(path2) && is_directory(path2))
      for (auto it = fs::directory_iterator(path2);
           it != fs::directory_iterator(); ++it)
        loadBrushTip((*it).path());
  }

  void loadBrushTip(const fs::path& path) {
    // filter by extension...
    if (path.extension() != ".png")
      return;
    ui->brushTipTextures.emplace_back(BrushTipTexture{
        path.stem().string(), image_io::loadTexture2D(*device, path.c_str())});
  }

  void makeSceneData() {
    const auto aspectRatio = (float)canvas->width / (float)canvas->height;
    const auto eyePos = glm::vec3(0, 0, 3);
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
      makeSceneData();
      makeCanvasData();
    ag::clear(*device, surfOut, ag::ClearColor{0.0f, 0.0f, 0.0f, 1.0f});
    ag::clearDepth(*device, surfOut, 1.0f);
    ag::clearDepth(*device, canvas->texDepth, 1.0f);
    ag::clear(*device, canvas->texNormals, ag::ClearColor{0.0f, 0.0f, 0.0f, 1.0f});
    ag::clear(*device, canvas->texStencil, ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    renderMesh(*canvas);
    input->poll();
    renderCanvas();
    computeHistograms(*canvas);
    ui->render(*device);
  }

  void renderCanvas() {
    ag::clear(*device, texEvalCanvas, ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    if (isMakingStroke) {
      auto brushProps = this->brushPropsFromUi();
      // preview canvas
      previewCanvas(*canvas, texEvalCanvas,
                    pipelines->ppEvaluatePreviewBaseColorUV, canvasData,
                    canvas->texStrokeMask,
                    glm::vec4{brushProps.color[0], brushProps.color[1],
                              brushProps.color[2], brushProps.opacity});
    } else
      previewCanvas(*canvas, texEvalCanvas, pipelines->ppEvaluate, canvasData,
                    canvas->texStrokeMask);
    copyTex(canvas->texNormals, surfOut, 1000, 1000, glm::vec2{0.0f, 0.0f}, 1.0f);
    copyTex(texEvalCanvas, surfOut, 1000, 1000, glm::vec2{0.0f, 0.0f}, 1.0f);
  }

  void onBrushPointerEvent(const BrushProperties& props, unsigned x,
                           unsigned y) {
    brushPath.addPointerEvent(PointerEvent{x, y, 1.0f}, props,
                              [this, props](auto splat) {
                                this->drawBrushSplat(*canvas, props, splat);
                              });
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
        ag::clear(*device, canvas->texStrokeMask,
                  ag::ClearColor{0.0f, 0.0f, 0.0f, 1.0f});
        unsigned x, y;
        ui->getPointerPosition(x, y);
        this->onBrushPointerEvent(this->brushPropsFromUi(), x, y);
        // beginStroke();
      } else if (ev.button == GLFW_MOUSE_BUTTON_LEFT &&
                 ev.state == input::MouseButtonState::Released &&
                 isMakingStroke) {
        unsigned x, y;
        ui->getPointerPosition(x, y);
        auto brushProps = this->brushPropsFromUi();
        this->onBrushPointerEvent(brushProps, x, y);
        this->applyStroke(*canvas, brushProps);
        isMakingStroke = false; // end stroke mode
      }
    });

    // on mouse move
    ui->canvasMousePointer.subscribe([this](auto ev) {
      // brush tool selected
      if (isMakingStroke == true && ui->activeTool == Tool::Brush) {
        // on mouse move, add splats with the current brush
        this->onBrushPointerEvent(this->brushPropsFromUi(), ev.positionX,
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

  void drawBrushSplat(Canvas& canvas, const BrushProperties& brushProps,
                      const SplatProperties& splat) {
    /*fmt::print(std::clog, "splat(({},{}),{},{})\n", splat.center.x,
               splat.center.y, splat.width, splat.rotation);*/
    uniforms::Splat uSplat;
    glm::vec2 scale{1.0f};
    if (ui->brushTip == BrushTip::Textured) {
      auto dim = ui->brushTipTextures[ui->selectedBrushTip].tex.info.dimensions;
      if (dim.x < dim.y)
        scale = glm::vec2{1.0f, (float)dim.y / (float)dim.x};
      else
        scale = glm::vec2{(float)dim.x / (float)dim.y, 1.0f};
    }
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
               ag::TextureUnit(0,
                               ui->brushTipTextures[ui->selectedBrushTip].tex,
                               samLinearClamp),
               canvasData, uSplat);
  }

  void applyStroke(Canvas& canvas, const BrushProperties& brushProps) {
    ag::compute(
        *device, pipelines->ppFlattenStroke,
        ag::ThreadGroupCount{
            (unsigned)divRoundUp(canvas.width, kCSThreadGroupSizeX),
            (unsigned)divRoundUp(canvas.height, kCSThreadGroupSizeY), 1u},
        canvasData, glm::vec4{brushProps.color[0], brushProps.color[1],
                              brushProps.color[2], brushProps.opacity},
        canvas.texStrokeMask, ag::RWTextureUnit(0, canvas.texBaseColorUV));
  }

  auto getCamera() {
      const auto aspect_ratio = (float)canvas->width / (float)canvas->height;
    const auto eyePos = glm::vec3(0, 2, -3);
    auto lookAt = glm::lookAt(eyePos, glm::vec3(0, 0, 0), CamUp);
    auto proj = glm::perspective(45.0f, aspect_ratio, 0.01f, 100.0f);
    Camera cam;
    cam.projMat = proj;
    cam.viewMat = lookAt;
    cam.wEye = eyePos;
    cam.mode = Camera::Mode::Perspective;
    return cam;
  }

  void renderMesh(Canvas& canvas) {
    drawMesh(mesh, *device, ag::SurfaceRT(canvas.texDepth, canvas.texNormals,
                                           canvas.texStencil),
             pipelines->ppRenderGbuffers, sceneData, glm::scale(glm::mat4{1.0f}, glm::vec3{0.3f}));
  }

  // compute histograms
  void computeHistograms(Canvas& canvas) {
    ag::compute(*device, pipelines->ppComputeShadingCurveHSV,
       ag::ThreadGroupCount{
            (unsigned)divRoundUp(canvas.width, kCSThreadGroupSizeX),
            (unsigned)divRoundUp(canvas.height, kCSThreadGroupSizeY), 1u},
            canvasData, glm::vec3{0.0f},
                canvas.texNormals,
                canvas.texStencil,
                canvas.texBaseColorUV,
            RWTextureUnit(0, canvas.texHistH),
                RWTextureUnit(0, canvas.texHistS),
                RWTextureUnit(0, canvas.texHistV),
                RWTextureUnit(0, canvas.texHistAccum));

    // read back histograms
    std::vector<uint32_t> histH(kShadingCurveSamplesSize), histS(kShadingCurveSamplesSize), histV(kShadingCurveSamplesSize), histAccum(kShadingCurveSamplesSize);
    ag::copySync(*device, canvas.texHistH, gsl::as_span(histH));
    ag::copySync(*device, canvas.texHistS, gsl::as_span(histS));
    ag::copySync(*device, canvas.texHistV, gsl::as_span(histV));
    ag::copySync(*device, canvas.texHistAccum, gsl::as_span(histAccum));

    for (unsigned i = 0; i < kShadingCurveSamplesSize; ++i) {
        ui->histH[i] = float(histH[i]) / float(histAccum[i]);
        ui->histS[i] = float(histS[i]) / float(histAccum[i]);
        ui->histV[i] = float(histV[i]) / float(histAccum[i]);
    }

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
  Mesh mesh;
  // UI
  std::unique_ptr<Ui> ui;
  // input
  std::unique_ptr<input::Input> input;
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
