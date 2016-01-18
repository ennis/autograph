#ifndef CAMERA_HPP
#define CAMERA_HPP
#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <common.hpp>
#include <transform.hpp>
#include <application.hpp>

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

  // TODO unproject, etc.

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

class TrackballCameraControl {
public:
  TrackballCameraControl(Application& app_, glm::vec3 wEye_, float fieldOfView_,
                         float nearPlane_, float farPlane_, double sensitivity_)
      : app(app_), vEye(wEye_), fieldOfView(fieldOfView_),
        nearPlane(nearPlane_), farPlane(farPlane_), sensitivity(sensitivity_),
        sceneRotX(0.0), sceneRotY(0.0), lastWheelOffset(0.0),
        curMode(Mode::Idle), lastMousePosX(0.0), lastMousePosY(0.0) {}

  Camera getCamera();
  glm::vec3 getRotationCenter() const;

protected:
  enum class Mode { Idle, Rotate, Pan };

  Application& app;
  // world coordinates after scene rotation
  glm::vec3 vEye;
  float fieldOfView;
  float nearPlane;
  float farPlane;
  double sensitivity;
  double sceneRotX;
  double sceneRotY;
  double lastWheelOffset;
  Mode curMode;
  double lastMousePosX;
  double lastMousePosY;
};

#endif

#endif // !CAMERA_HPP
