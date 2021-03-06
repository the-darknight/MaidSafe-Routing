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

#ifndef MAIDSAFE_ROUTING_TESTS_TEST_UTILS_H_
#define MAIDSAFE_ROUTING_TESTS_TEST_UTILS_H_

#include <cstdint>
#include <vector>

#include "boost/asio/ip/address.hpp"
#include "boost/asio/ip/udp.hpp"

#include "maidsafe/common/rsa.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/routing/node_info.h"
#include "maidsafe/routing/routing_table.h"


namespace maidsafe {

namespace routing {

namespace test {

struct NodeInfoAndPrivateKey {
  NodeInfoAndPrivateKey()
      : node_info(),
        private_key() {}
  NodeInfo node_info;
  asymm::PrivateKey private_key;
};

NodeInfoAndPrivateKey MakeNodeInfoAndKeys();
NodeInfoAndPrivateKey MakeNodeInfoAndKeysWithPmid(passport::Pmid pmid);
NodeInfoAndPrivateKey MakeNodeInfoAndKeysWithMaid(passport::Maid maid);

passport::Maid MakeMaid();
passport::Pmid MakePmid();

// Fob GetFob(const NodeInfoAndPrivateKey& node);

NodeInfo MakeNode();

NodeId GenerateUniqueRandomId(const NodeId& holder, const uint16_t& pos);
NodeId GenerateUniqueRandomId(const uint16_t& pos);

int NetworkStatus(const bool& client, const int& status);

void SortFromTarget(const NodeId& target, std::vector<NodeInfo>& nodes);

void PartialSortFromTarget(const NodeId& target,
                           std::vector<NodeInfo>& nodes,
                           size_t num_to_sort);

void SortIdsFromTarget(const NodeId& target, std::vector<NodeId>& nodes);

void SortNodeInfosFromTarget(const NodeId& target, std::vector<NodeInfo>& nodes);

bool CompareListOfNodeInfos(const std::vector<NodeInfo>& lhs, const std::vector<NodeInfo>& rhs);

}  // namespace test

}  // namespace routing

}  // namespace maidsafe

#endif  // MAIDSAFE_ROUTING_TESTS_TEST_UTILS_H_
