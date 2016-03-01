#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

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

struct TrackballCameraSettings {
  TrackballCameraSettings() = default;
  glm::vec3 eye = glm::vec3{0.0f, 0.0f, 0.0f};
  float fieldOfView = 70.0f;
  float nearPlane = 0.05f;
  float farPlane = 100.0f;
  float sensitivity = 0.1f;
};

struct TrackballCamera {
  enum class Mode { Pan, Rotate, Idle };
  enum class ScrollMode { Zoom, MoveForward };

  TrackballCamera(const TrackballCameraSettings &init) : vEye(init.eye) {}

  glm::mat4 getLookAt() {
    auto lookAt = glm::lookAt(glm::vec3(0, 0, -2), glm::vec3(0, 0, 0), CamUp) *
                  glm::rotate(glm::rotate((float)sceneRotX, CamRight),
                              (float)sceneRotY, CamUp);
    return lookAt;
  }

  void updatePanVectors() {
    auto invLookAt = glm::inverse(getLookAt());
    wCamRight = glm::vec3(invLookAt * glm::vec4(CamRight, 0.0f));
    wCamUp = glm::vec3(invLookAt * glm::vec4(-CamUp, 0.0f));
    wCamFront = glm::vec3(invLookAt * glm::vec4(CamFront, 0.0f));
  }

  void onMouseMove(double raw_dx, double raw_dy, Mode mode) {
    auto dx = settings.sensitivity * raw_dx;
    auto dy = settings.sensitivity * raw_dy;
    if (mode == Mode::Rotate) {
      auto rot_speed = 0.1;
      auto twopi = glm::pi<double>() * 2.0;
      sceneRotX += std::fmod(rot_speed * dy, twopi);
      sceneRotY += std::fmod(rot_speed * dx, twopi);
      updatePanVectors();
    } else if (mode == Mode::Pan) {
      auto pan_speed = 1.0;
      vEye += (float)(dx * pan_speed) * wCamRight +
              (float)(dy * pan_speed) * wCamUp;
    }
  }

  void onScrollWheel(double delta, ScrollMode scrollMode) {
    auto scroll = delta * settings.sensitivity;
    auto scroll_speed = 10.0;
    if (scrollMode == ScrollMode::MoveForward)
      vEye += (float)(scroll * scroll_speed) * wCamFront;
    else
      zoom *= std::exp2(scroll);
  }

  Camera getCamera(float aspect_ratio) {
    Camera cam;
    auto lookAt = getLookAt();
    cam.mode = Camera::Mode::Perspective;
    cam.viewMat = lookAt * glm::translate(vEye);
    cam.projMat = glm::scale(glm::vec3{(float)zoom, (float)zoom, 1.0f}) *
                  glm::perspective(settings.fieldOfView, aspect_ratio,
                                   settings.nearPlane, settings.farPlane);
    cam.wEye = glm::vec3(glm::inverse(cam.viewMat) *
                         glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    return cam;
  }

  bool panning = false;
  bool rotating = false;

  TrackballCameraSettings settings;
  double sceneRotX = 0.0;
  double sceneRotY = 0.0;
  double zoom = 1.0;
  glm::vec3 vEye;
  glm::vec3 wCamRight;
  glm::vec3 wCamUp;
  glm::vec3 wCamFront;
};

// Tool that moves the camera
class TrackballCameraController : public ToolInstance {
public:
  TrackballCameraController(const ToolResources &resources,
                            TrackballCamera &trackball_)
      : res(resources), trackball(trackball_) {

    resources.ui.canvasMouseButtons.subscribe(
        subscription, [this](const input::mouse_button_event &ev) {
          if (ev.button == GLFW_MOUSE_BUTTON_LEFT) {
            if (ev.state == input::mouse_button_state::pressed) {
              this->rotating = true;
            } else if (ev.state == input::mouse_button_state::released) {
              this->rotating = false;
            }
          } else if (ev.button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (ev.state == input::mouse_button_state::pressed) {
              this->panning = true;
            } else if (ev.state == input::mouse_button_state::released) {
              this->panning = false;
            }
          }
        });

    resources.ui.canvasMousePointer.subscribe(
        subscription, [this](const input::cursor_event &ev) {
          this->trackball.onMouseMove(
              (double)ev.positionX - (double)lastPosX,
              (double)ev.positionY - (double)lastPosY,
              panning ? TrackballCamera::Mode::Pan
                      : (rotating ? TrackballCamera::Mode::Rotate
                                  : TrackballCamera::Mode::Idle));
          lastPosX = (double)ev.positionX;
          lastPosY = (double)ev.positionY;
        });

    /*input::input_filter<input::key_event>(resources.input.events())
        .filter([](auto ev) { return ev.code == GLFW_KEY_LEFT_CONTROL; })
        .subscribe(subscription, [this](const input::key_event &ev) {
          if (ev.state == input::key_state::pressed)
            this->ctrl_down = true;
          else if (ev.state == input::key_state::released)
            this->ctrl_down = false;
        });*/

    ctrl_down =
        input::get_key_state(resources.input.events(), GLFW_KEY_LEFT_CONTROL);

    resources.ui.canvasMouseScroll.subscribe(
        subscription, [this](const input::mouse_scroll_event &ev) {
          this->trackball.onScrollWheel(
              ev.dy, this->ctrl_down.get_value() == input::key_state::pressed
                         ? TrackballCamera::ScrollMode::Zoom
                         : TrackballCamera::ScrollMode::MoveForward);
        });
  }

  ~TrackballCameraController() { subscription.unsubscribe(); }

private:
  ToolResources res;
  TrackballCamera &trackball;
  bool panning = false;
  bool rotating = false;
  rxcpp::rxsub::behavior<input::key_state> ctrl_down{
      input::key_state::released};
  double lastPosX = 0.0;
  double lastPosY = 0.0;
  rxcpp::composite_subscription subscription;
};

#endif
