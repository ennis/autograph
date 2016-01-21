#include <glm/glm.hpp>

namespace uniforms {
struct Scene {
  glm::mat4 viewMatrix;
  glm::mat4 projMatrix;
  glm::mat4 viewProjMatrix;
  glm::vec2 viewportSize;
};

struct Object {
  glm::mat4 modelMatrix;
};
}
