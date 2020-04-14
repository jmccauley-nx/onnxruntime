// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "gtest/gtest.h"
#include "test/providers/provider_test_utils.h"
#include "core/util/math.h"

namespace onnxruntime {
namespace test {

TEST(InverseOpTest, two_by_two_float) {
  OpTester test("Inverse", 12);
  test.AddInput<float>("X", {2, 2}, {4, 7, 2, 6});
  test.AddOutput<float>("Y", {2, 2}, {0.6f, -0.7f, -0.2f, 0.4f});
  test.Run();
}

TEST(InverseOpTest, two_by_two_double) {
  OpTester test("Inverse", 12);
  test.AddInput<double>("X", {2, 2}, {4, 7, 2, 6});
  test.AddOutput<double>("Y", {2, 2}, {0.6f, -0.7f, -0.2f, 0.4f});
  test.Run();
}

TEST(InverseOpTest, two_by_two_float16) {
  OpTester test("Inverse", 12);

  auto input_float = {4.f, 7.f, 2.f, 6.f};
  std::vector<MLFloat16> input;
  std::transform(
      input_float.begin(), input_float.end(), std::back_inserter(input),
      [](float v) {
        return MLFloat16(math::floatToHalf(v));
      });

  auto output_float = {0.6f, -0.7f, -0.2f, 0.4f};
  std::vector<MLFloat16> output;
  std::transform(
      output_float.begin(), output_float.end(), std::back_inserter(output), [](float v) {
        return MLFloat16(math::floatToHalf(v));
      });

  test.AddInput<MLFloat16>("X", {2, 2}, input);
  test.AddOutput<MLFloat16>("Y", {2, 2}, output);
  test.Run();
}

TEST(InverseOpTest, four_by_four_float) {
  OpTester test("Inverse", 12);
  test.AddInput<float>("X", {4, 4}, 
    {4.f, 0.f, 0.f, 0.f,
     0.f, 0.f, 2.f, 0.f,
     0.f, 1.f, 2.f, 0.f,
     1.f, 0.f, 0.f, 1.f
    });
  test.AddOutput<float>("Y", {4, 4}, {
      0.25f, 0.f, 0.f, 0.f,
      0.f,  -1.f, 1.f, 0.f,
      0.f,  0.5f, 0.f, 0.f,
     -0.25f, 0.f, 0.f, 1.f});
  test.Run();
}

}  // namespace test
}  // namespace onnxruntime