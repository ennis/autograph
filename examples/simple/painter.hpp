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
#include "smudge.hpp"
#include "ui.hpp"

#include <boost/filesystem.hpp>

namespace input = ag::extra::input;
namespace image_io = ag::extra::image_io;

constexpr const char kDefaultBrushTip[] = "simple/img/brushes/brush_tip.png";

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
    texEvalCanvasBlur =
        device->createTexture2D<ag::RGBA8>(glm::uvec2{width, height});

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

    toolResources = std::make_unique<ToolResources>(ToolResources{
        *device, *pipelines, *canvas, *ui, samLinearRepeat, samLinearClamp,
        samNearestRepeat, samNearestClamp, vboQuad});

	toolInstance = std::make_unique<ColorBrushTool>(*toolResources);
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
		ag::makeThreadGroupCount2D(canvas.width, canvas.height, 16, 16),
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
        BlurParams{{(float)canvas.width, (float)canvas.height}, 3, 1.0f};
    auto lightPos =
        glm::normalize(glm::vec3{ui->lightPosXY[0], ui->lightPosXY[1], -2.0f});
    ag::draw(
        *device, canvas.texShadingTerm, pipelines->ppShadingOverlay,
        ag::DrawArrays(ag::PrimitiveType::Triangles, 0, 3), canvas.texNormals,
        canvasData,
        glm::normalize(glm::vec3{ui->lightPosXY[0], ui->lightPosXY[1], -2.0f}));
    ag::compute(*device, pipelines->ppBlurH,
                ag::makeThreadGroupCount2D(canvas.width, canvas.height, 16, 16), params,
                RWTextureUnit(0, canvas.texShadingTerm),
                RWTextureUnit(1, canvas.texShadingTermSmooth0));
    ag::compute(*device, pipelines->ppBlurV,
		ag::makeThreadGroupCount2D(canvas.width, canvas.height, 16, 16), params,
                RWTextureUnit(0, canvas.texShadingTermSmooth0),
                RWTextureUnit(1, canvas.texShadingTermSmooth));
  }

  void updateActiveTool() {
    if (activeTool != ui->activeTool) {
      activeTool = ui->activeTool;
      switch (activeTool) {
      case Tool::Brush:
        toolInstance = std::make_unique<ColorBrushTool>(*toolResources);
        break;
      case Tool::Smudge:
        toolInstance = std::make_unique<SmudgeTool>(*toolResources);
        break;
      case Tool::Blur:
		  
      case Tool::Select:
      case Tool::None:
      default:
        // do nothing
        break;
      }
    }
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
    updateActiveTool();
	if (!ui->overrideShadingCurve)
		computeHistograms(*canvas);
	else
		loadShadingCurve(*canvas);
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

    /*if (isMakingStroke && ui->activeTool == Tool::Brush) {
      auto brushProps = this->brushPropsFromUi();
      // preview canvas
      previewCanvas(*canvas, texEvalCanvasBlur,
                    pipelines->ppEvaluatePreviewBaseColorUV, canvasData,
                    lightPos, canvas->texStrokeMask,
                    glm::vec4{brushProps.color[0], brushProps.color[1],
                              brushProps.color[2], brushProps.opacity});
    } else*/

    previewCanvas(*device, *canvas, texEvalCanvasBlur, pipelines->ppEvaluate,
                  canvasData, samLinearClamp, lightPos);
    // blur pass
    previewCanvas(*device, *canvas, texEvalCanvas, pipelines->ppEvaluateBlurPass,
                  canvasData, samLinearClamp,
                  RWTextureUnit(1, texEvalCanvasBlur));

    copyTex(canvas->texNormals, surfOut, width, height, glm::vec2{0.0f, 0.0f},
            1.0f);
    copyTex(texEvalCanvas, surfOut, width, height, glm::vec2{0.0f, 0.0f}, 1.0f);
  }

  /*void blur(Canvas& canvas, const BrushProperties& brushProps,
            const SplatProperties& splat) {
    ag::Box2D footprintBox;
    footprintBox = getSplatFootprint(splat.width, splat.width, splat);

    struct BlurUniforms {
      glm::uvec2 origin;
      glm::uvec2 size;
      float intensity;
    };

    BlurUniforms u{{footprintBox.xmin, footprintBox.ymin},
                   {footprintBox.width(), footprintBox.height()}};

    applyComputeShaderOverRect(
        canvas, footprintBox, pipelines->ppBlur, canvasData,
        RWTextureUnit(0, canvas.texBaseColorUV),
        RWTextureUnit(1, texSmudgeFootprint), u,
        ui->brushTipTextures[ui->selectedBrushTip].tex);
  }*/

  void setupInput() {
    // on key presses
    /*input->keys().subscribe(
        [this](auto ev) { fmt::print("Key event: {}\n", (int)ev.code); });*/
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
		ag::makeThreadGroupCount2D(canvas.width, canvas.height, 16, 16),
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
        int x = glm::clamp(i + w, 0, (int)kShadingCurveSamplesSize - 1);
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
      HSVCurve[i][3].value = 255;
    }

    // write back
    ag::copy(*device, gsl::span<const ag::RGBA8>(
                          (const ag::RGBA8*)HSVCurve.data(), HSVCurve.size()),
             canvas.texShadingProfileLN);
  }

  void loadShadingCurve(Canvas& canvas)
  {
	  std::vector<ag::RGBA8> HSVCurve(kShadingCurveSamplesSize);
	  for (unsigned i = 0; i < kShadingCurveSamplesSize; ++i)
	  {
		  HSVCurve[i][0].value = 0;
		  HSVCurve[i][1].value = 0;
		  HSVCurve[i][2].value = (uint8_t)(glm::smoothstep(0.2f, 0.8f, (float)i / (float)kShadingCurveSamplesSize)*255.0f);
		  HSVCurve[i][3].value = 255;
		}
	  ag::copy(*device, gsl::span<const ag::RGBA8>(
		  (const ag::RGBA8*)HSVCurve.data(), HSVCurve.size()),
		  canvas.texShadingProfileLN);
  }

  void drawShadingOverlay(Canvas& canvas) {
    copyTex(canvas.texShadingTermSmooth, surfOut, width, height,
            glm::vec2{0.0f, 0.0f}, 1.0f);
  }
  
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

  // Pointers to resources shared between tools
  std::unique_ptr<ToolResources> toolResources;

  Tool activeTool = Tool::Brush;
  std::unique_ptr<ToolInstance> toolInstance;

  // evaluated canvas
  Texture2D<ag::RGBA8> texEvalCanvasBlur;
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
