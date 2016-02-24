#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include "brush_tool.hpp"

struct Frustum {
  float left;
  float right;
  float top;
  float bottom;
  // near clip plane position
  float nearPlane;
  // far clip plane position
  float farPlane;
};

// All that defines a camera (view matrix + frustum parameters)
struct Camera {
  enum class Mode { Perspective, Orthographic };

  // Camera mode
  Mode mode;
  // Projection parameters
  // frustum (for culling)
  Frustum frustum;
  // view matrix
  // (World -> View)
  glm::mat4 viewMat;
  // inverse view matrix
  // (View -> World)
  // glm::mat4 invViewMat;
  // projection matrix
  // (View -> clip?)
  glm::mat4 projMat;
  // Eye position in world space (camera center)
  glm::vec3 wEye;
};

const glm::vec3 CamFront = glm::vec3(0, 0, 1);
const glm::vec3 CamRight = glm::vec3(1, 0, 0);
const glm::vec3 CamUp = glm::vec3(0, 1, 0);

/*class TrackballCameraController : public ToolInstance
{
public:
  TrackballCameraController(const ToolResources& resources) : res(resources)
  {
    // subscribe on key and mouse events
    canvasMouseButtons = resources.ui.canvasMouseButtons;
    canvasMousePointer = resources.ui.canvasMousePointer;
    canvasMouseButtons.subscribe([this](auto ev) {});
    canvasMousePointer.subscribe([this](auto ev) {});

  }

  void updateCamera()
  {

  }

  virtual ~TrackballCameraController()
  {}

private:
  enum class Mode {
    Idle,
    Rotate,
    Pan
  };

  Mode curMode;
  ToolResources resources;
  rxcpp::observable<input::MouseButtonEvent> canvasMouseButtons;
  rxcpp::observable<input::MousePointerEvent> canvasMousePointer;
};*/

#endif