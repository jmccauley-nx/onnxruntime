// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/framework/allocatormgr.h"
#include "core/framework/allocator.h"
#include "core/optimizer/insert_cast_transformer.h"
#include "core/graph/model.h"
#include "gtest/gtest.h"
#include "test_utils.h"
#include "test/test_environment.h"

using namespace ONNX_NAMESPACE;
namespace onnxruntime {
namespace test {

#define MODEL_FOLDER ORT_TSTR("testdata/transform/")

typedef std::vector<onnxruntime::NodeArg*> ArgMap;
TEST(TransformerTest, InsertCastGPUTest) {
  auto model = std::make_shared<onnxruntime::Model>("test", false, DefaultLoggingManager().DefaultLogger());
  onnxruntime::Graph& graph = model->MainGraph();

  TypeProto tensor_float_16;
  tensor_float_16.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT16);
  onnxruntime::NodeArg i1_def("I1", &tensor_float_16),
      i2_def("I2", &tensor_float_16),
      i3_def("I3", &tensor_float_16),
      o1_def("O1", &tensor_float_16),
      o2_def("O2", &tensor_float_16),
      o3_def("O3", &tensor_float_16);

  auto& node1 = graph.AddNode("node1", "MatMul", "cpu operator1", ArgMap{&i1_def, &i2_def}, ArgMap{&o1_def});
  auto& node2 = graph.AddNode("node2", "MatMul", "gpu operator1", ArgMap{&o1_def, &i3_def}, ArgMap{&o2_def});
  node2.SetExecutionProviderType(onnxruntime::kCudaExecutionProvider);
  auto& node3 = graph.AddNode("node3", "Clip", "cpu operator2", ArgMap{&o2_def}, ArgMap{&o3_def});

  auto status = graph.Resolve();
  ASSERT_TRUE(status.IsOK()) << status.ErrorMessage();
  InsertCastTransformer transformer("Test");

  bool modified = true;
  status = transformer.Apply(graph, modified, DefaultLoggingManager().DefaultLogger());
  EXPECT_TRUE(status.IsOK());
  status = graph.Resolve();
  EXPECT_TRUE(status.IsOK()) << status.ErrorMessage();
  EXPECT_EQ(graph.NumberOfNodes(), 8);
  for (auto it = node1.InputNodesBegin(); it != node1.InputNodesEnd(); ++it) {
    EXPECT_EQ((*it).OpType(), "Cast");
  }
  for (auto it = node1.OutputNodesBegin(); it != node1.OutputNodesEnd(); ++it) {
    EXPECT_EQ((*it).OpType(), "Cast");
  }
  for (auto it = node2.InputNodesBegin(); it != node2.InputNodesEnd(); ++it) {
    EXPECT_EQ((*it).OpType(), "Cast");
  }
  for (auto it = node2.OutputNodesBegin(); it != node2.OutputNodesEnd(); ++it) {
    EXPECT_EQ((*it).OpType(), "Cast");
  }
  for (auto it = node3.InputNodesBegin(); it != node3.InputNodesEnd(); ++it) {
    EXPECT_EQ((*it).OpType(), "Cast");
  }
  for (auto it = node3.OutputNodesBegin(); it != node3.OutputNodesEnd(); ++it) {
    EXPECT_EQ((*it).OpType(), "Cast");
  }
}

TEST(TransformerTest, InsertCastAllCPUTest) {
  auto model = std::make_shared<onnxruntime::Model>("test", false, DefaultLoggingManager().DefaultLogger());
  onnxruntime::Graph& graph = model->MainGraph();

  TypeProto tensor_float_16;
  tensor_float_16.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT16);
  onnxruntime::NodeArg i1_def("I1", &tensor_float_16),
      i2_def("I2", &tensor_float_16),
      i3_def("I3", &tensor_float_16),
      o1_def("O1", &tensor_float_16),
      o2_def("O2", &tensor_float_16),
      o3_def("O3", &tensor_float_16);

  auto& node1 = graph.AddNode("node1", "MatMul", "cpu operator1", ArgMap{&i1_def, &i2_def}, ArgMap{&o1_def});
  auto& node2 = graph.AddNode("node2", "MatMul", "gpu operator1", ArgMap{&o1_def, &i3_def}, ArgMap{&o2_def});
  auto& node3 = graph.AddNode("node3", "Clip", "cpu operator2", ArgMap{&o2_def}, ArgMap{&o3_def});

  auto status = graph.Resolve();
  ASSERT_TRUE(status.IsOK()) << status.ErrorMessage();

  InsertCastTransformer transformer("Test");

  bool modified = true;
  EXPECT_TRUE(transformer.Apply(graph, modified, DefaultLoggingManager().DefaultLogger()).IsOK());
  status = graph.Resolve();
  EXPECT_TRUE(status.IsOK()) << status.ErrorMessage();
  EXPECT_EQ(graph.NumberOfNodes(), 7);
  for (auto it = node1.InputNodesBegin(); it != node1.InputNodesEnd(); ++it) {
    EXPECT_EQ((*it).OpType(), "Cast");
  }
  for (auto it = node1.OutputNodesBegin(); it != node1.OutputNodesEnd(); ++it) {
    EXPECT_NE((*it).OpType(), "Cast");
  }
  for (auto it = node2.OutputNodesBegin(); it != node2.OutputNodesEnd(); ++it) {
    EXPECT_NE((*it).OpType(), "Cast");
  }
  for (auto it = node3.InputNodesBegin(); it != node3.InputNodesEnd(); ++it) {
    EXPECT_NE((*it).OpType(), "Cast");
  }
  for (auto it = node3.OutputNodesBegin(); it != node3.OutputNodesEnd(); ++it) {
    EXPECT_EQ((*it).OpType(), "Cast");
  }
}

// test that when there are 3 Cast ops in a row we remove the correct ones
TEST(TransformerTest, ThreeInARowRemoval) {
  auto model_uri = MODEL_FOLDER ORT_TSTR("triple-cast.onnx");
  std::shared_ptr<Model> model;
  auto status = Model::Load(model_uri, model, nullptr, DefaultLoggingManager().DefaultLogger());
  ASSERT_TRUE(status.IsOK()) << status;

  Graph& graph = model->MainGraph();
  std::map<std::string, int> op_to_count = CountOpsInGraph(graph);
  // there are 3 in a row prior to a Transpose, and one post-Transpose.
  // we want to remove 2 of the first 3
  ASSERT_TRUE(op_to_count["Cast"] == 4);

  InsertCastTransformer transformer("Test");

  bool modified = false;
  status = transformer.Apply(graph, modified, DefaultLoggingManager().DefaultLogger());
  EXPECT_TRUE(status.IsOK()) << status;
  EXPECT_TRUE(modified) << "Transformer should have removed some Cast nodes";
  status = graph.Resolve();
  EXPECT_TRUE(status.IsOK()) << status;

  op_to_count = CountOpsInGraph(graph);
  ASSERT_TRUE(op_to_count["Cast"] == 2);
}

}  // namespace test
}  // namespace onnxruntime
