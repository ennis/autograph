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
#include "camera.hpp"
#include "canvas.hpp"
#include "pipelines.hpp"
#include "ui.hpp"

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

namespace fs = boost::filesystem;

class Painter : public samples::GLSample<Painter> {
public:
  Painter(unsigned width, unsigned height)
      : GLSample(width, height, "Painter") {
    pipelines = std::make_unique<Pipelines>(*device, samplesRoot);
    // 1000x1000 canvas
    mesh = loadMesh("common/meshes/skull.obj");
    canvas = std::make_unique<Canvas>(*device, width, height);
    texEvalCanvas =
        device->createTexture2D<ag::RGBA8>(glm::uvec2{width, height});
    texSmudgeFootprint =
        device->createTexture2D<ag::RGBA8>(glm::uvec2{512, 512});
    input = std::make_unique<input::Input>();
    input->registerEventSource(
        std::make_unique<input::GLFWInputEventSource>(gl.getWindow()));
    ui = std::make_unique<Ui>(gl.getWindow(), *input);
    samples::Vertex2D vboQuadData[] = {
        {-1.0f, -1.0f, 0.0f, 0.0f}, {-1.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, -1.0f, 1.0f, 0.0f},  {-1.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},   {1.0f, -1.0f, 1.0f, 0.0f},
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
        ag::TextureUnit(6, canvas.texShadingTermSmooth, samLinearClamp),
        ag::TextureUnit(7, canvas.texStencil, samLinearClamp),
        ag::RWTextureUnit(0, out), std::forward<Resources>(resources)...);
  }

  // apply a CS over a region of the canvas
  template <typename... Resources>
  void applyComputeShaderOverRect(Canvas& canvas, const ag::Box2D& rect,
                                  ComputePipeline& pipeline,
                                  RawBufferSlice& canvasData,
                                  Resources&&... resources) {
    ag::compute(*device, pipeline,
                ag::ThreadGroupCount{
                    (unsigned)divRoundUp(rect.width(), kCSThreadGroupSizeX),
                    (unsigned)divRoundUp(rect.height(), kCSThreadGroupSizeY),
                    1u},
                canvasData, std::forward<Resources>(resources)...);
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
        path.stem().string(),
        image_io::loadTexture2D(*device, path.string().c_str())});
  }

  void makeSceneData() {
    const auto aspectRatio = (float)canvas->width / (float)canvas->height;
    const auto eyePos = glm::vec3(0, 1, 1);
    const auto lookAt =
        glm::lookAt(eyePos, glm::vec3(0, 1, 0), glm::vec3(0, 1, 0));
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

  void baseColorToShadingOffset(Canvas& canvas) {
    auto lightPos =
        glm::normalize(glm::vec3{ui->lightPosXY[0], ui->lightPosXY[1], -2.0f});
    ag::compute(*device, pipelines->ppBaseColorToOffset,
                ag::ThreadGroupCount{
                    (unsigned)divRoundUp(canvas.width, kCSThreadGroupSizeX),
                    (unsigned)divRoundUp(canvas.height, kCSThreadGroupSizeY),
                    1u},
                canvasData, lightPos, canvas.texShadingProfileLN,
                canvas.texBaseColorUV, canvas.texShadingTermSmooth,
                canvas.texStencil, ag::RWTextureUnit(0, canvas.texHSVOffsetUV));
  }

  void renderShading(Canvas& canvas) {
    struct BlurParams {
      glm::vec2 size;
      int blurSize;
      float sigma;
    };
    auto params =
        BlurParams{{(float)canvas.width, (float)canvas.height}, 121, 26.0f};
    auto lightPos =
        glm::normalize(glm::vec3{ui->lightPosXY[0], ui->lightPosXY[1], -2.0f});
    ag::draw(
        *device, canvas.texShadingTerm, pipelines->ppShadingOverlay,
        ag::DrawArrays(ag::PrimitiveType::Triangles, 0, 3), canvas.texNormals,
        canvasData,
        glm::normalize(glm::vec3{ui->lightPosXY[0], ui->lightPosXY[1], -2.0f}));
    ag::compute(*device, pipelines->ppBlurH,
                ag::ThreadGroupCount{
                    (unsigned)divRoundUp(canvas.width, kCSThreadGroupSizeX),
                    (unsigned)divRoundUp(canvas.height, kCSThreadGroupSizeY),
                    1u},
                params, RWTextureUnit(0, canvas.texShadingTerm),
                RWTextureUnit(1, canvas.texShadingTermSmooth0));
    ag::compute(*device, pipelines->ppBlurV,
                ag::ThreadGroupCount{
                    (unsigned)divRoundUp(canvas.width, kCSThreadGroupSizeX),
                    (unsigned)divRoundUp(canvas.height, kCSThreadGroupSizeY),
                    1u},
                params, RWTextureUnit(0, canvas.texShadingTermSmooth0),
                RWTextureUnit(1, canvas.texShadingTermSmooth));
  }

  void render() {
    using namespace glm;
    makeSceneData();
    makeCanvasData();
    ag::clear(*device, surfOut, ag::ClearColor{0.0f, 0.0f, 0.0f, 1.0f});
    ag::clearDepth(*device, surfOut, 1.0f);
    ag::clearDepth(*device, canvas->texDepth, 1.0f);
    ag::clear(*device, canvas->texNormals,
              ag::ClearColor{0.0f, 0.0f, 0.0f, 1.0f});
    ag::clear(*device, canvas->texStencil,
              ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    renderMesh(*canvas);
    renderShading(*canvas);
    input->poll();
    renderCanvas();
    if (ui->showReferenceShading)
      drawShadingOverlay(*canvas);
    if (ui->showHSVOffset)
      copyTex(canvas->texHSVOffsetUV, surfOut, width, height,
              glm::vec2{0.0f, 0.0f}, 1.0f);
    if (ui->showBaseColor)
      copyTex(canvas->texBaseColorUV, surfOut, width, height,
              glm::vec2{0.0f, 0.0f}, 1.0f);
    ui->render(*device);
  }

  void renderCanvas() {
    auto lightPos =
        glm::normalize(glm::vec3{ui->lightPosXY[0], ui->lightPosXY[1], -2.0f});
    ag::clear(*device, texEvalCanvas, ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    if (isMakingStroke && ui->activeTool == Tool::Brush) {
      auto brushProps = this->brushPropsFromUi();
      // preview canvas
      previewCanvas(*canvas, texEvalCanvas,
                    pipelines->ppEvaluatePreviewBaseColorUV, canvasData,
                    lightPos, canvas->texStrokeMask,
                    glm::vec4{brushProps.color[0], brushProps.color[1],
                              brushProps.color[2], brushProps.opacity});
    } else
      previewCanvas(*canvas, texEvalCanvas, pipelines->ppEvaluate, canvasData,
                    lightPos, canvas->texStrokeMask);
    copyTex(canvas->texNormals, surfOut, width, height, glm::vec2{0.0f, 0.0f},
            1.0f);
    copyTex(texEvalCanvas, surfOut, width, height, glm::vec2{0.0f, 0.0f}, 1.0f);
  }

  void onBrushPointerEvent(const BrushProperties& props, unsigned x,
                           unsigned y) {
    brushPath.addPointerEvent(PointerEvent{x, y, 1.0f}, props,
                              [this, props](auto splat) {
                                this->drawBrushSplat(*canvas, props, splat);
                              });
  }

  void onSmudgePointerEvent(const BrushProperties& props, unsigned x,
                            unsigned y, bool first) {
    brushPath.addPointerEvent(PointerEvent{x, y, 1.0f}, props,
                              [this, props, first](auto splat) {
                                this->smudge(*canvas, props, splat, first);
                              });
  }

  void setupInput() {
    // on key presses
    input->keys().subscribe(
        [this](auto ev) { fmt::print("Key event: {}\n", (int)ev.code); });
    // on mouse button clicks
    ui->canvasMouseButtons.subscribe([this](auto ev) {
      if (ui->activeTool == Tool::Brush) {
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
          this->computeHistograms(*canvas);
          this->baseColorToShadingOffset(*canvas);
        }
      } else if (ui->activeTool == Tool::Smudge) {
        if (ev.button == GLFW_MOUSE_BUTTON_LEFT &&
            ev.state == input::MouseButtonState::Pressed) {
          isMakingStroke = true;   // go into stroke mode
          brushPath = BrushPath(); // reset brush path
          ag::clear(*device, texSmudgeFootprint,
                    ag::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
          unsigned x, y;
          ui->getPointerPosition(x, y);
          this->onSmudgePointerEvent(this->brushPropsFromUi(), x, y, true);
        } else if (ev.button == GLFW_MOUSE_BUTTON_LEFT &&
                   ev.state == input::MouseButtonState::Released &&
                   isMakingStroke) {
          unsigned x, y;
          ui->getPointerPosition(x, y);
          auto brushProps = this->brushPropsFromUi();
          this->onSmudgePointerEvent(brushProps, x, y, false);
          isMakingStroke = false; // end stroke mode
          this->computeHistograms(*canvas);
          this->baseColorToShadingOffset(*canvas);
        }
      }
    });

    // on mouse move
    ui->canvasMousePointer.subscribe([this](auto ev) {
      // brush tool selected
      if (isMakingStroke == true && ui->activeTool == Tool::Brush) {
        // on mouse move, add splats with the current brush
        this->onBrushPointerEvent(this->brushPropsFromUi(), ev.positionX,
                                  ev.positionY);
      } else if (isMakingStroke == true && ui->activeTool == Tool::Smudge) {
        this->onSmudgePointerEvent(this->brushPropsFromUi(), ev.positionX,
                                   ev.positionY, false);
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

  glm::mat3x4 getSplatTransform(unsigned tipWidth, unsigned tipHeight,
                                const SplatProperties& splat) {
    glm::vec2 scale{1.0f};
    if (tipWidth < tipHeight)
      scale = glm::vec2{1.0f, (float)tipHeight / (float)tipWidth};
    else
      scale = glm::vec2{(float)tipWidth / (float)tipHeight, 1.0f};

    scale *= splat.width;

    return glm::mat3x4(
        glm::scale(glm::rotate(glm::translate(glm::mat3x3(1.0f), splat.center),
                               splat.rotation),
                   scale));
  }

  ag::Box2D getSplatFootprint(unsigned tipWidth, unsigned tipHeight,
                              const SplatProperties& splat) {
    auto transform = getSplatTransform(tipWidth, tipHeight, splat);
    auto topleft = transform * glm::vec3{-1.0f, -1.0f, 1.0f};
    auto bottomright = transform * glm::vec3{1.0f, 1.0f, 1.0f};
    return ag::Box2D{(unsigned)topleft.x, (unsigned)topleft.y,
                     (unsigned)bottomright.x, (unsigned)bottomright.y};
  }

  // smudge tool operation:
  // take a snapshot of the canvas under the brush footprint
  // when the mouse move, apply snapshot with opacity, repeat
  // no need for a stroke mask
  void smudge(Canvas& canvas, const BrushProperties& brushProps,
              const SplatProperties& splat, bool first) {
    ag::Box2D footprintBox;
    if (ui->brushTip == BrushTip::Round) {
      auto dim = ui->brushTipTextures[ui->selectedBrushTip].tex.info.dimensions;
      footprintBox = getSplatFootprint(dim.x, dim.y, splat);
    } else
      footprintBox = getSplatFootprint(splat.width, splat.width, splat);

    struct SmudgeUniforms {
      glm::uvec2 origin;
      glm::uvec2 size;
      float opacity;
    };

    SmudgeUniforms u{{footprintBox.xmin, footprintBox.ymin},
                     {footprintBox.width(), footprintBox.height()},
                     first ? 0.0f : ui->strokeOpacity};

    if (ui->brushTip == BrushTip::Textured)
      applyComputeShaderOverRect(
          canvas, footprintBox, pipelines->ppSmudge, canvasData,
          RWTextureUnit(0, canvas.texBaseColorUV),
          RWTextureUnit(1, texSmudgeFootprint), u,
          ui->brushTipTextures[ui->selectedBrushTip].tex);
  }

  void drawBrushSplat(Canvas& canvas, const BrushProperties& brushProps,
                      const SplatProperties& splat) {
    /*fmt::print(std::clog, "splat(({},{}),{},{})\n", splat.center.x,
               splat.center.y, splat.width, splat.rotation);*/
    uniforms::Splat uSplat;
    uSplat.center = splat.center;
    uSplat.width = splat.width;
    if (ui->brushTip == BrushTip::Round) {
      auto dim = ui->brushTipTextures[ui->selectedBrushTip].tex.info.dimensions;
      uSplat.transform = getSplatTransform(dim.x, dim.y, splat);
    } else
      uSplat.transform = getSplatTransform(splat.width, splat.width, splat);

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

  /*auto getCamera() {
      const auto aspect_ratio = (float)canvas->width / (float)canvas->height;
    const auto eyePos = glm::vec3(0, 10, -3);
    auto lookAt = glm::lookAt(eyePos, glm::vec3(0, 10, 0), CamUp);
    auto proj = glm::perspective(45.0f, aspect_ratio, 0.01f, 100.0f);
    Camera cam;
    cam.projMat = proj;
    cam.viewMat = lookAt;
    cam.wEye = eyePos;
    cam.mode = Camera::Mode::Perspective;
    return cam;
  }*/

  void renderMesh(Canvas& canvas) {
    drawMesh(mesh, *device, ag::SurfaceRT(canvas.texDepth, canvas.texNormals,
                                          canvas.texStencil),
             pipelines->ppRenderGbuffers, sceneData,
             glm::scale(glm::mat4{1.0f}, glm::vec3{0.2f}));
  }

  // compute histograms
  void computeHistograms(Canvas& canvas) {
    // workaround
    ag::ClearColorInt clear = {{0, 0, 0, 0}};
    ag::clearInteger(*device, canvas.texHistH, clear);
    ag::clearInteger(*device, canvas.texHistS, clear);
    ag::clearInteger(*device, canvas.texHistV, clear);
    ag::clearInteger(*device, canvas.texHistAccum, clear);

    ag::compute(
        *device, pipelines->ppComputeShadingCurveHSV,
        ag::ThreadGroupCount{
            (unsigned)divRoundUp(canvas.width, kCSThreadGroupSizeX),
            (unsigned)divRoundUp(canvas.height, kCSThreadGroupSizeY), 1u},
        canvasData,
        glm::normalize(glm::vec3{ui->lightPosXY[0], ui->lightPosXY[1], -2.0f}),
        canvas.texNormals, canvas.texStencil, canvas.texBaseColorUV,
        RWTextureUnit(0, canvas.texHistH), RWTextureUnit(1, canvas.texHistS),
        RWTextureUnit(2, canvas.texHistV),
        RWTextureUnit(3, canvas.texHistAccum));

    // read back histograms
    std::vector<uint32_t> histH(kShadingCurveSamplesSize),
        histS(kShadingCurveSamplesSize), histV(kShadingCurveSamplesSize),
        histAccum(kShadingCurveSamplesSize);
    std::vector<ag::RGBA8> HSVCurve(kShadingCurveSamplesSize);
    ag::copySync(*device, canvas.texHistH, gsl::as_span(histH));
    ag::copySync(*device, canvas.texHistS, gsl::as_span(histS));
    ag::copySync(*device, canvas.texHistV, gsl::as_span(histV));
    ag::copySync(*device, canvas.texHistAccum, gsl::as_span(histAccum));

    for (unsigned i = 0; i < kShadingCurveSamplesSize; ++i) {
      if (histAccum[i]) {
        ui->histH[i] = float(histH[i]) / (255.0f * float(histAccum[i]));
        ui->histS[i] = float(histS[i]) / (255.0f * float(histAccum[i]));
        ui->histV[i] = float(histV[i]) / (255.0f * float(histAccum[i]));
      } else {
        ui->histH[i] = 0.0f;
        ui->histS[i] = 0.0f;
        ui->histV[i] = 0.0f;
      }
    }

    static constexpr float gaussK[] = {
        0.000027, 0.00006,  0.000125, 0.000251, 0.000484, 0.000898, 0.001601,
        0.002743, 0.004515, 0.007141, 0.010853, 0.01585,  0.022243, 0.029995,
        0.038867, 0.048396, 0.057906, 0.066577, 0.073554, 0.078087, 0.079659,
        0.078087, 0.073554, 0.066577, 0.057906, 0.048396, 0.038867, 0.029995,
        0.022243, 0.01585,  0.010853, 0.007141, 0.004515, 0.002743, 0.001601,
        0.000898, 0.000484, 0.000251, 0.000125, 0.00006,  0.000027};

    // smooth them (a lot)
    for (int i = 0; i < kShadingCurveSamplesSize; ++i) {
      float ah = 0.0f;
      float as = 0.0f;
      float av = 0.0f;
      for (int w = -20; w < +20; w++) {
        int x = glm::clamp(i + w, 0, (int)kShadingCurveSamplesSize-1);
        ah += ui->histH[x] * gaussK[w + 20];
        as += ui->histS[x] * gaussK[w + 20];
        av += ui->histV[x] * gaussK[w + 20];
      }
      ui->histH[i] = ah;
      ui->histS[i] = as;
      ui->histV[i] = av;
      HSVCurve[i][0].value = (uint8_t)(ah * 255.0f);
      HSVCurve[i][1].value = (uint8_t)(as * 255.0f);
      HSVCurve[i][2].value = (uint8_t)(av * 255.0f);
      HSVCurve[i][3].value = 1.0f;
    }

    // write back
    ag::copy(*device, gsl::span<const ag::RGBA8>(
                          (const ag::RGBA8*)HSVCurve.data(), HSVCurve.size()),
             canvas.texShadingProfileLN);
  }

  void drawShadingOverlay(Canvas& canvas) {
    copyTex(canvas.texShadingTermSmooth, surfOut, width, height,
            glm::vec2{0.0f, 0.0f}, 1.0f);
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

  // splat footprint (for smudge)
  Texture2D<ag::RGBA8> texSmudgeFootprint;

  Buffer<samples::Vertex2D[]> vboQuad;

  ///////////
  RawBufferSlice canvasData;
  RawBufferSlice sceneData;

  /////////// Brush tool state
  // current brush path
  BrushPath brushPath;
};

#endif
