#ifndef SHADING_CURVES_HPP
#define SHADING_CURVES_HPP

#include "canvas.hpp"
#include "pipelines.hpp"
#include "ui.hpp"

// compute shading curve from the base color layer, alse update UI
void computeShadingCurve(Device& device, Pipelines& pipelines, Canvas& canvas,
                         Ui& ui) {
  // workaround
  ag::ClearColorInt clear = {{0, 0, 0, 0}};
  ag::clearInteger(device, canvas.texHistH, clear);
  ag::clearInteger(device, canvas.texHistS, clear);
  ag::clearInteger(device, canvas.texHistV, clear);
  ag::clearInteger(device, canvas.texHistAccum, clear);

  ag::compute(
      device, pipelines.ppComputeShadingCurveHSV,
      ag::makeThreadGroupCount2D(canvas.width, canvas.height, 16, 16),
      glm::vec2 {canvas.width, canvas.height},
      glm::normalize(glm::vec3{ui.lightPosXY[0], ui.lightPosXY[1], -2.0f}),
      canvas.texNormals, canvas.texStencil, canvas.texBaseColorUV,
      RWTextureUnit(0, canvas.texHistH), RWTextureUnit(1, canvas.texHistS),
      RWTextureUnit(2, canvas.texHistV), RWTextureUnit(3, canvas.texHistAccum));

  // read back histograms
  std::vector<uint32_t> histH(kShadingCurveSamplesSize),
      histS(kShadingCurveSamplesSize), histV(kShadingCurveSamplesSize),
      histAccum(kShadingCurveSamplesSize);
  std::vector<ag::RGBA8> HSVCurve(kShadingCurveSamplesSize);
  ag::copySync(device, canvas.texHistH, gsl::as_span(histH));
  ag::copySync(device, canvas.texHistS, gsl::as_span(histS));
  ag::copySync(device, canvas.texHistV, gsl::as_span(histV));
  ag::copySync(device, canvas.texHistAccum, gsl::as_span(histAccum));

  for (unsigned i = 0; i < kShadingCurveSamplesSize; ++i) {
    if (histAccum[i]) {
      ui.histH[i] = float(histH[i]) / (255.0f * float(histAccum[i]));
      ui.histS[i] = float(histS[i]) / (255.0f * float(histAccum[i]));
      ui.histV[i] = float(histV[i]) / (255.0f * float(histAccum[i]));
    } else {
      ui.histH[i] = 0.0f;
      ui.histS[i] = 0.0f;
      ui.histV[i] = 0.0f;
    }
  }

  static constexpr float gaussK[] = {
      0.000027f, 0.00006f,  0.000125f, 0.000251f, 0.000484f, 0.000898f,
      0.001601f, 0.002743f, 0.004515f, 0.007141f, 0.010853f, 0.01585f,
      0.022243f, 0.029995f, 0.038867f, 0.048396f, 0.057906f, 0.066577f,
      0.073554f, 0.078087f, 0.079659f, 0.078087f, 0.073554f, 0.066577f,
      0.057906f, 0.048396f, 0.038867f, 0.029995f, 0.022243f, 0.01585f,
      0.010853f, 0.007141f, 0.004515f, 0.002743f, 0.001601f, 0.000898f,
      0.000484f, 0.000251f, 0.000125f, 0.00006f,  0.000027f};

  // smooth them (a lot)
  for (int i = 0; i < kShadingCurveSamplesSize; ++i) {
    float ah = 0.0f;
    float as = 0.0f;
    float av = 0.0f;
    for (int w = -20; w < +20; w++) {
      int x = glm::clamp(i + w, 0, (int)kShadingCurveSamplesSize - 1);
      ah += ui.histH[x] * gaussK[w + 20];
      as += ui.histS[x] * gaussK[w + 20];
      av += ui.histV[x] * gaussK[w + 20];
    }
    ui.histH[i] = ah;
    ui.histS[i] = as;
    ui.histV[i] = av;
    HSVCurve[i][0].value = (uint8_t)(ah * 255.0f);
    HSVCurve[i][1].value = (uint8_t)(as * 255.0f);
    HSVCurve[i][2].value = (uint8_t)(av * 255.0f);
    HSVCurve[i][3].value = 255;
  }

  // write back
  ag::copy(device, gsl::span<const ag::RGBA8>((const ag::RGBA8*)HSVCurve.data(),
                                              HSVCurve.size()),
           canvas.texShadingProfileLN);
}

#endif
