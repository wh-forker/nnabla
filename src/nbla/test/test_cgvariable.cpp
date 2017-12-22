// Copyright (c) 2017 Sony Corporation. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// test_cgvariable.cpp

#include "gtest/gtest.h"
#include <functional>
#include <nbla/computation_graph/computation_graph.hpp>
#include <nbla/computation_graph/variable.hpp>
#include <nbla/function.hpp>
#include <nbla/function/callback.hpp>
#include <vector>

namespace nbla {

using std::vector;
using std::make_shared;

auto ignore1 = [](void *obj, const Variables &, const Variables &) {};
auto ignore2 = [](void *obj) {};

class CgVariableTest : public ::testing::Test {
public:
protected:
  virtual void SetUp() {
    init_cpu();
    this->ctx_.array_class = "CpuArray";
  }
  Context ctx_;
};

TEST_F(CgVariableTest, OrderOfBackwardForGraphWithoutBranches) {
  vector<std::string> order;
  auto generate_backward = [&order](std::string name) {
    return [name, &order](void *obj, const Variables &inputs,
                          const Variables &outputs,
                          const vector<bool> &propagate_down,
                          const vector<bool> &accum) { order.push_back(name); };
  };

  /* Generate network */
  auto a = make_shared<Callback>(this->ctx_, nullptr, ignore1, ignore1,
                                 generate_backward("A"), ignore2);
  auto b = make_shared<Callback>(this->ctx_, nullptr, ignore1, ignore1,
                                 generate_backward("B"), ignore2);
  auto c = make_shared<Callback>(this->ctx_, nullptr, ignore1, ignore1,
                                 generate_backward("C"), ignore2);
  auto input = make_shared<CgVariable>(Shape_t{1, 1, 1}, true);
  auto h1 = connect(make_shared<CgFunction>(a), {input}, 1);
  auto h2 = connect(make_shared<CgFunction>(b), h1, 1);
  auto h3 = connect(make_shared<CgFunction>(c), h2, 1);
  EXPECT_EQ(1, h3.size());

  /* backward and check the order */
  h3[0]->backward();
  EXPECT_EQ(vector<std::string>({"C", "B", "A"}), order);
}

TEST_F(CgVariableTest, OrderOfBackwardForGraphWithBranches) {
  vector<std::string> order;
  auto generate_backward = [&order](std::string name) {
    return [name, &order](void *obj, const Variables &inputs,
                          const Variables &outputs,
                          const vector<bool> &propagate_down,
                          const vector<bool> &accum) { order.push_back(name); };
  };

  /* Generate network */
  auto a = make_shared<Callback>(this->ctx_, nullptr, ignore1, ignore1,
                                 generate_backward("A"), ignore2);
  auto b = make_shared<Callback>(this->ctx_, nullptr, ignore1, ignore1,
                                 generate_backward("B"), ignore2);
  auto c = make_shared<Callback>(this->ctx_, nullptr, ignore1, ignore1,
                                 generate_backward("C"), ignore2);
  auto d = make_shared<Callback>(this->ctx_, nullptr, ignore1, ignore1,
                                 generate_backward("D"), ignore2);
  auto e = make_shared<Callback>(this->ctx_, nullptr, ignore1, ignore1,
                                 generate_backward("E"), ignore2);
  auto input = make_shared<CgVariable>(Shape_t{1, 1, 1}, true);
  auto h1 = connect(make_shared<CgFunction>(a), {input}, 1);
  auto h2 = connect(make_shared<CgFunction>(b), h1, 1);
  auto h3 = connect(make_shared<CgFunction>(c), h1, 1);
  auto h4 = connect(make_shared<CgFunction>(d), h2, 1);
  auto h5 = connect(make_shared<CgFunction>(e), {h3[0], h4[0]}, 1);
  EXPECT_EQ(1, h5.size());

  /* backward and check the order */
  h5[0]->backward();
  EXPECT_EQ(vector<std::string>({"E", "D", "C", "B", "A"}), order);
}
}