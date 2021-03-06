/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/common/test.h"
#include "maidsafe/routing/tests/routing_network.h"

int main(int argc, char **argv) {
//  testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(new maidsafe::routing::test::NodesEnvironment(
      maidsafe::routing::test::kServerSize, maidsafe::routing::test::kClientSize,
      static_cast<size_t>(maidsafe::routing::test::kServerSize * 0.25),
      static_cast<size_t>(maidsafe::routing::test::kClientSize / 2)));
//  return RUN_ALL_TESTS();
  return maidsafe::test::ExecuteMain(argc, argv);
}
