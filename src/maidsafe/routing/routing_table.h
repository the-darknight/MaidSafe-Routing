/*******************************************************************************
 *  Copyright 2012 maidsafe.net limited                                        *
 *                                                                             *
 *  The following source code is property of maidsafe.net limited and is not   *
 *  meant for external use.  The use of this code is governed by the licence   *
 *  file licence.txt found in the root of this directory and also on           *
 *  www.maidsafe.net.                                                          *
 *                                                                             *
 *  You are not free to copy, amend or otherwise use this source code without  *
 *  the explicit written permission of the board of directors of maidsafe.net. *
 ******************************************************************************/

#ifndef MAIDSAFE_ROUTING_ROUTING_TABLE_H_
#define MAIDSAFE_ROUTING_ROUTING_TABLE_H_

#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "boost/asio/ip/udp.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/rsa.h"

#include "maidsafe/private/utils/fob.h"

#include "maidsafe/routing/api_config.h"


namespace maidsafe {

namespace routing {

namespace test { class GenericNode; }

namespace protobuf { class Contact; }

struct NodeInfo;

class RoutingTable {
 public:
  RoutingTable(const Fob& fob, bool client_mode);
  void InitialiseFunctors(NetworkStatusFunctor network_status_functor,
                          std::function<void(const NodeInfo&, bool)> remove_node_functor,
                          CloseNodeReplacedFunctor close_node_replaced_functor,
                          RemoveFurthestUnnecessaryNode remove_furthest_node);
  bool AddNode(const NodeInfo& peer);
  bool CheckNode(const NodeInfo& peer);
  NodeInfo DropNode(const NodeId &node_to_drop, bool routing_only);
  bool GetNodeInfo(const NodeId& node_id, NodeInfo& node_info) const;
  bool IsThisNodeInRange(const NodeId& target_id, uint16_t range);
  bool IsThisNodeClosestTo(const NodeId& target_id, bool ignore_exact_match = false);
  bool IsConnected(const NodeId& node_id) const;
  bool IsRemovable(const NodeId& node_id);
  bool ConfirmGroupMembers(const NodeId& node1, const NodeId& node2);
  // Returns default-constructed NodeId if routing table size is zero
  NodeInfo GetClosestNode(const NodeId& target_id, bool ignore_exact_match = false);
  NodeInfo GetClosestNode(const NodeId& target_id,
                          const std::vector<std::string>& exclude,
                          bool ignore_exact_match = false);
  // Returns max NodeId if routing table size is less than requested node_number
  NodeInfo GetNthClosestNode(const NodeId& target_id, uint16_t node_number);
  std::vector<NodeId> GetClosestNodes(const NodeId& target_id, uint16_t number_to_get);
  NodeInfo GetRemovableNode(std::vector<std::string> attempted = std::vector<std::string>());
  size_t size() const;
  Fob kFob() const { return kFob_; }
  NodeId kNodeId() const { return kNodeId_; }
  NodeId kConnectionId() const { return kConnectionId_; }
  bool client_mode() const { return kClientMode_; }
  friend class test::GenericNode;

 private:
  RoutingTable(const RoutingTable&);
  RoutingTable& operator=(const RoutingTable&);
  bool AddOrCheckNode(NodeInfo node, bool remove);
  void SetBucketIndex(NodeInfo& node_info) const;
  bool CheckPublicKeyIsUnique(const NodeInfo& node, std::unique_lock<std::mutex>& lock) const;
  NodeInfo ResolveConnectionDuplication(const NodeInfo& new_duplicate_node,
                                        bool local_endpoint,
                                        NodeInfo& existing_node);
  std::vector<NodeInfo> CheckGroupChange(std::unique_lock<std::mutex>& lock);
  bool MakeSpaceForNodeToBeAdded(const NodeInfo& node,
                                 bool remove,
                                 NodeInfo& removed_node,
                                 std::unique_lock<std::mutex>& lock);
  uint16_t PartialSortFromTarget(const NodeId& target,
                                 uint16_t number,
                                 std::unique_lock<std::mutex>& lock);
  void NthElementSortFromTarget(const NodeId& target,
                                uint16_t nth_element,
                                std::unique_lock<std::mutex>& lock);
  NodeId FurthestCloseNode();
  std::vector<NodeInfo> GetClosestNodeInfo(const NodeId& target_id,
                                           uint16_t number_to_get,
                                           bool ignore_exact_match = false);
  std::pair<bool, std::vector<NodeInfo>::iterator> Find(const NodeId& node_id,
                                                        std::unique_lock<std::mutex>& lock);
  std::pair<bool, std::vector<NodeInfo>::const_iterator> Find(
      const NodeId& node_id,
      std::unique_lock<std::mutex>& lock) const;
  void UpdateNetworkStatus(uint16_t size) const;
  std::string PrintRoutingTable();

  const uint16_t kMaxSize_;
  const bool kClientMode_;
  const Fob kFob_;
  const NodeId kNodeId_;
  const NodeId kConnectionId_;
  mutable std::mutex mutex_;
  NodeId furthest_group_node_id_;
  std::function<void(const NodeInfo&, bool)> remove_node_functor_;
  NetworkStatusFunctor network_status_functor_;
  CloseNodeReplacedFunctor close_node_replaced_functor_;
  RemoveFurthestUnnecessaryNode remove_furthest_node_;
  std::vector<NodeInfo> nodes_;
};

}  // namespace routing

}  // namespace maidsafe

#endif  // MAIDSAFE_ROUTING_ROUTING_TABLE_H_
