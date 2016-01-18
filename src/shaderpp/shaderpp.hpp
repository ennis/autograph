#ifndef SHADERPP_HPP
#define SHADERPP_HPP

#include <string>
#include <gsl.h>

namespace shaderpp {
enum class PipelineStage { Vertex, Geometry, Pixel, Domain, Hull, Compute };

class ShaderSource {
public:
  ShaderSource(const char* path_);

  const std::string& getOriginalSource() const { return source; }

  std::string preprocess(PipelineStage stage, gsl::span<const char*> defines,
                         gsl::span<const char*> includePaths);

private:
  std::string source;
  std::string path;
};
}

#endif
