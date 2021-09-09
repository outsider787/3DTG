// Copyright 2020 The Khronos® Group Inc.
#pragma once

namespace GLTF {
namespace Constants {
enum class WebGL {
  BYTE = 5120,
  UNSIGNED_BYTE = 5121,
  SHORT = 5122,
  UNSIGNED_SHORT = 5123,
  FLOAT = 5126,
  UNSIGNED_INT = 5125,

  FLOAT_VEC2 = 35664,
  FLOAT_VEC3 = 35665,
  FLOAT_VEC4 = 35666,
  INT_VEC2 = 35667,
  INT_VEC3 = 35668,
  INT_VEC4 = 35669,
  BOOL = 35670,
  BOOL_VEC2 = 35671,
  BOOL_VEC3 = 35672,
  BOOL_VEC4 = 35673,
  FLOAT_MAT2 = 35674,
  FLOAT_MAT3 = 35675,
  FLOAT_MAT4 = 35676,
  SAMPLER_2D = 35678,
  SAMPLER_CUBE = 35680,

  CULL_FACE = 2884,
  DEPTH_TEST = 2929,

  ARRAY_BUFFER = 34962,
  ELEMENT_ARRAY_BUFFER = 34963,

  FRAGMENT_SHADER = 35632,
  VERTEX_SHADER = 35633,

  ALPHA = 6406,
  RGB = 6407,
  RGBA = 6408,
  LUMINANCE = 6409,
  LUMINANCE_ALPHA = 6410,

  TEXTURE_2D = 3553,

  NEAREST = 9728,
  LINEAR = 9729,
  NEAREST_MIPMAP_NEAREST = 9984,
  LINEAR_MIPMAP_NEAREST = 9985,
  NEAREST_MIPMAP_LINEAR = 9986,
  LINEAR_MIPMAP_LINEAR = 9987,

  CLAMP_TO_EDGE = 33071,
  MIRRORED_REPEAT = 33648,
  REPEAT = 10497,

  FUNC_ADD = 32774,
  ONE = 1,
  ONE_MINUS_SRC_ALPHA = 771
};
}  // namespace Constants
}  // namespace GLTF
