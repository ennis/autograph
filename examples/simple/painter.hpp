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

#include <extra/image_io/load_image.hpp>

#include "../common/sample.hpp"
#include "../common/uniforms.hpp"

#include "pipelines.hpp"
#include "canvas.hpp"
#include "ui.hpp"
#include "brush_path.hpp"


class Painter : public samples::GLSample<Painter> {
public:
  Painter(unsigned width, unsigned height)
      : GLSample(width, height, "Painter") {
    pipelines = std::make_unique<Pipelines>(*device, samplesRoot);
    // 1000x1000 canvas
    canvas = std::make_unique<Canvas>(*device, 1000, 1000);
    input = std::make_unique<Input>();
    input.registerEventSource(
        std::make_unique<GLFWInputEventSource>(gl.getWindow()));
    ui = std::make_unique<UI>(gl.getWindow(), input);
  }

  auto makeSceneData() {
    const auto aspectRatio = (float)canvas->width / (float)canvas->height;
    const auto eyePos = glm::vec3(0, 2, -3);
    const auto lookAt = glm::lookAt(eyePos, glm::vec3(0, 0, 0), CamUp);
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
    ag::clear(*device, texRender, ag::ClearColor{0.0f, 1.0f, 0.0f, 1.0f});
    /*samples::drawMesh(bunnyMesh, *device, out, pipeline, cbSceneData,
                      glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)));*/

    copyTex(texDefault, out, width, height, {20, 20}, 1.0);
  }

  void setupInput() {
    // on mouse button clicks
    ui->canvasMouseButtons.subscribe([this](auto ev) {
      if (ev.button == GLFW_MOUSE_BUTTON_LEFT && ev.type == GLFW_PRESS) {
        isMakingStroke = true;  // go into stroke mode
        beginStroke();
      }
      else if (ev.button == GLFW_MOUSE_BUTTON_LEFT && ev.type == GLFW_RELEASE) {
        isMakingStroke = false; // end stroke mode
        endStroke();
      }
    });

    // on mouse move
    ui->canvasMousePointer.get_observable().subscribe([this](auto ev) {
      // brush tool selected
      if (isMakingStroke == true && activeTool = Tool::Brush) {
        // on mouse move, add splats with the current brush
        brushPath.addPointerEvent(ev, [this](auto pos) {
          drawBrushSplat(*canvas, pos);
        });
      } else {
        // TODO other tools (blur, smudge, etc.)
      }
    });
  }

  // create textures
  // load pipelines

  // render normal map
  // render isolines

  enum class Tool 
  {
    None,
    Brush,
    Blur,
    Smudge,
    Select
  };

private:
  // shaders
  std::unique_ptr<Pipelines> pipelines;
  // canvas
  std::unique_ptr<Canvas> canvas;
  // mesh
  std::unique_ptr<Mesh> mesh;
  // UI
  std::unique_ptr<UI> ui;
  // input
  std::unique_ptr<Input> input;

  // is the user making a stroke
  bool isMakingStroke;

  /////////// Brush tool state
  // current brush path
  BrushPath brushPath;

};

#endif