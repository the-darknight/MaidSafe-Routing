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

#include <string>
#include <algorithm>
#include <vector>

#include "maidsafe/routing/utils.h"

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/rudp/return_codes.h"

#include "maidsafe/routing/client_routing_table.h"
#include "maidsafe/routing/message_handler.h"
#include "maidsafe/routing/network_utils.h"
#include "maidsafe/routing/return_codes.h"
#include "maidsafe/routing/routing.pb.h"
#include "maidsafe/routing/routing_table.h"
#include "maidsafe/routing/rpcs.h"

namespace maidsafe {

namespace routing {

int AddToRudp(NetworkUtils& network,
              const NodeId& this_node_id,
              const NodeId& this_connection_id,
              const NodeId& peer_id,
              const NodeId& peer_connection_id,
              rudp::EndpointPair peer_endpoint_pair,
              const bool& requestor,
              const bool& client) {
  LOG(kVerbose) << "AddToRudp. peer_id : " << DebugId(peer_id)
                << " , connection id : " << DebugId(peer_connection_id);
  protobuf::Message connect_success(
      rpcs::ConnectSuccess(peer_id, this_node_id, this_connection_id, requestor, client));
  int result = network.Add(peer_connection_id, peer_endpoint_pair,
                           connect_success.SerializeAsString());
  if (result != rudp::kSuccess) {
    LOG(kError) << "rudp add failed for peer node [" << DebugId(peer_id) << "]. Connection id : "
                << DebugId(peer_connection_id) << ". result : " << result;
  } else {
    LOG(kVerbose) << "rudp.Add succeeded for peer node ["
                  << DebugId(peer_id) << "]. Connection id : "
                  << DebugId(peer_connection_id);
  }
  return result;
}

bool ValidateAndAddToRoutingTable(NetworkUtils& network,
                                  RoutingTable& routing_table,
                                  ClientRoutingTable& client_routing_table,
                                  const NodeId& peer_id,
                                  const NodeId& connection_id,
                                  const asymm::PublicKey& public_key,
                                  const bool& client) {
  if (network.MarkConnectionAsValid(connection_id) != kSuccess) {
    LOG(kError) << "[" << DebugId(routing_table.kNodeId()) << "] "
                << ". Rudp failed to validate connection with  Peer id : "
                << DebugId(peer_id)
                << " , Connection id : "
                << DebugId(connection_id);
    return false;
  }

  NodeInfo peer;
  peer.node_id = peer_id;
  peer.public_key = public_key;
  peer.connection_id = connection_id;
  bool routing_accepted_node(false);
  if (client) {
    NodeId furthest_close_node_id =
        routing_table.GetNthClosestNode(NodeId(routing_table.kNodeId()),
                                        2 * Parameters::closest_nodes_size).node_id;

    if (client_routing_table.AddNode(peer, furthest_close_node_id))
      routing_accepted_node = true;
  } else {  // Vaults
    if (routing_table.AddNode(peer))
      routing_accepted_node = true;
  }

  if (routing_accepted_node) {
    LOG(kVerbose) << "[" << DebugId(routing_table.kNodeId()) << "] "
                  << "added " << (client ? "client-" : "") << "node to "
                  << (client ? "non-" : "") << "routing table.  Node ID: "
                  << HexSubstr(peer_id.string());
    return true;
  }

  LOG(kInfo) << "[" << DebugId(routing_table.kNodeId()) << "] "
             << "failed to add " << (client ? "client-" : "") << "node to "
             << (client ? "non-" : "") << "routing table.  Node ID: "
             << HexSubstr(peer_id.string())
             << ". Added rudp connection will be removed.";
  network.Remove(connection_id);
  return false;
}

// FIXME
void HandleSymmetricNodeAdd(RoutingTable& /*routing_table*/, const NodeId& /*peer_id*/,
                            const asymm::PublicKey& /*public_key*/) {
//  if (routing_table.Contains(peer_id)) {
//    LOG(kVerbose) << "[" << HexSubstr(routing_table.kKeys().identity) << "] "
//                  << "already added node to routing table.  Node ID: "
//                  << HexSubstr(peer_id.string())
//                  << "Node is behind symmetric router but connected on local endpoint";
//    return;
//  }
//  NodeInfo peer;
//  peer.node_id = peer_id;
//  peer.public_key = public_key;
////  peer.endpoint = rudp::kNonRoutable;
//  peer.nat_type = rudp::NatType::kSymmetric;

//  if (routing_table.AddNode(peer)) {
//    LOG(kVerbose) << "[" << HexSubstr(routing_table.kKeys().identity) << "] "
//                  << "added node to routing table.  Node ID: " << HexSubstr(peer_id.string())
//                  << "Node is behind symmetric router !";
//  } else {
//    LOG(kVerbose) << "Failed to add node to routing table.  Node id : "
//                  << HexSubstr(peer_id.string());
//  }
}

GroupRangeStatus GetProximalRange(const NodeId& target_id,
                                  const NodeId& node_id,
                                  const NodeId& this_node_id,
                                  const crypto::BigInt& proximity_radius,
                                  const std::vector<NodeId>& holders)  {
  assert((std::find(holders.begin(), holders.end(), target_id) == holders.end()) &&
         "Ensure to remove target id entry from holders, if present");
  assert(std::is_sorted(holders.begin(),
                        holders.end(),
                        [target_id](const NodeId& lhs, const NodeId& rhs) {
                          return NodeId::CloserToTarget(lhs, rhs, target_id);
                        }) && "Ensure to sort holders in order of distance to targer_id");
  assert(holders.size() <= Parameters::node_group_size);

  if ((target_id == node_id) || (target_id == this_node_id))
    return GroupRangeStatus::kOutwithRange;

  if (std::find(holders.begin(), holders.end(), node_id) != holders.end()) {
    return GroupRangeStatus::kInRange;
  }

  NodeId distance_id(node_id ^ target_id);
  crypto::BigInt distance((distance_id.ToStringEncoded(NodeId::kHex) + 'h').c_str());
  return (distance < proximity_radius) ? GroupRangeStatus::kInProximalRange
                                       : GroupRangeStatus::kOutwithRange;
}


bool IsRoutingMessage(const protobuf::Message& message) {
  return message.routing_message();
}

bool IsNodeLevelMessage(const protobuf::Message& message) {
  return !IsRoutingMessage(message);
}

bool IsRequest(const protobuf::Message& message) {
  return (message.request());
}

bool IsResponse(const protobuf::Message& message) {
  return !IsRequest(message);
}

bool IsDirect(const protobuf::Message& message) {
  return message.direct();
}

bool IsCacheableGet(const protobuf::Message& message) {
  return (message.has_cacheable() &&
           (static_cast<Cacheable>(message.cacheable()) == Cacheable::kGet));
}

bool IsCacheablePut(const protobuf::Message& message) {
  return (message.has_cacheable() &&
           (static_cast<Cacheable>(message.cacheable()) == Cacheable::kPut));
}

bool CheckId(const std::string& id_to_test) {
  return id_to_test.size() == NodeId::kSize;
}

bool ValidateMessage(const protobuf::Message &message) {
  if (!message.IsInitialized()) {
    LOG(kWarning) << "Uninitialised message dropped.";
    return false;
  }

  // Message has traversed more hops than expected
  if (message.hops_to_live() <= 0) {
    std::string route_history;
    for (const auto& route : message.route_history())
      route_history += HexSubstr(route) + ", ";
    LOG(kError) << "Message has traversed more hops than expected. "
                <<  Parameters::max_route_history << " last hops in route history are: "
                 << route_history
                 << " \nMessage source: " << HexSubstr(message.source_id())
                 << ", \nMessage destination: " << HexSubstr(message.destination_id())
                 << ", \nMessage type: " << message.type()
                 << ", \nMessage id: " << message.id();
    return false;
  }
  // Invalid destination id, unknown message
  if (!CheckId(message.destination_id())) {
    LOG(kWarning) << "Stray message dropped, need destination ID for processing."
                  << " id: " << message.id();
    return false;
  }


  if (!(message.has_source_id() || (message.has_relay_id() &&
                                    message.has_relay_connection_id()))) {
    LOG(kWarning) << "Message should have either src id or relay information.";
    assert(false && "Message should have either src id or relay information.");
    return false;
  }

  if (message.has_source_id() && !CheckId(message.source_id())) {
      LOG(kWarning) << "Invalid source id field.";
      return false;
  }

  if (message.has_relay_id() && NodeId(message.relay_id()).IsZero()) {
    LOG(kWarning) << "Invalid relay id field.";
    return false;
  }

  if (message.has_relay_connection_id() && NodeId(message.relay_connection_id()).IsZero()) {
    LOG(kWarning) << "Invalid relay connection id field.";
    return false;
  }

  if (static_cast<MessageType>(message.type()) == MessageType::kConnect)
    if (!message.direct()) {
      LOG(kWarning) << "kConnectRequest type message must be direct.";
      return false;
    }

  if (static_cast<MessageType>(message.type()) == MessageType::kFindNodes &&
      (message.request() == false))
    if ((!message.direct())) {
      LOG(kWarning) << "kFindNodesResponse type message must be direct.";
      return false;
    }

  return true;
}

void SetProtobufEndpoint(const boost::asio::ip::udp::endpoint& endpoint,
                         protobuf::Endpoint* pb_endpoint) {
  if (pb_endpoint) {
    pb_endpoint->set_ip(endpoint.address().to_string().c_str());
    pb_endpoint->set_port(endpoint.port());
  }
}

boost::asio::ip::udp::endpoint GetEndpointFromProtobuf(const protobuf::Endpoint& pb_endpoint) {
  return boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(pb_endpoint.ip()),
                                        static_cast<uint16_t>(pb_endpoint.port()));
}

std::string MessageTypeString(const protobuf::Message& message) {
  std::string message_type;
  switch (static_cast<MessageType>(message.type())) {
    case MessageType::kPing :
      message_type = "kPing     ";
      break;
    case MessageType::kConnect :
      message_type = "kConnect  ";
      break;
    case MessageType::kFindNodes :
      message_type = "kFindNodes";
      break;
    case MessageType::kConnectSuccess :
      message_type = "kC-Success";
      break;
    case MessageType::kConnectSuccessAcknowledgement :
      message_type = "kC-Suc-Ack";
      break;
    case MessageType::kRemove :
      message_type = "kRemove";
      break;
    case MessageType::kClosestNodesUpdate :
      message_type = "kCloses_Nodes_Update";
      break;
    case MessageType::kGetGroup :
      message_type = "kGetGroup";
      break;
    case MessageType::kNodeLevel :
      message_type = "kNodeLevel";
      break;
    default:
      message_type = "Unknown  ";
  }
  if (message.request())
    message_type = message_type + " Req";
  else
    message_type = message_type + " Res";
  return message_type;
}

std::vector<boost::asio::ip::udp::endpoint> OrderBootstrapList(
                    std::vector<boost::asio::ip::udp::endpoint> peer_endpoints) {
  if (peer_endpoints.empty())
    return peer_endpoints;
  auto copy_vector(peer_endpoints);
  for (auto& endpoint : copy_vector) {
    endpoint.port(kLivePort);
  }
  auto it = std::unique(copy_vector.begin(), copy_vector.end());
  copy_vector.resize(it - copy_vector.begin());
  std::reverse(peer_endpoints.begin(), peer_endpoints.end());
  peer_endpoints.resize(peer_endpoints.size() + copy_vector.size());
  for (const auto& i : copy_vector)
    peer_endpoints.push_back(i);
  std::reverse(peer_endpoints.begin(), peer_endpoints.end());
  return peer_endpoints;
}

protobuf::NatType NatTypeProtobuf(const rudp::NatType& nat_type) {
  switch (nat_type) {
    case rudp::NatType::kSymmetric :
      return protobuf::NatType::kSymmetric;
      break;
    case rudp::NatType::kOther :
      return protobuf::NatType::kOther;
      break;
    default :
      return protobuf::NatType::kUnknown;
      break;
  }
}

rudp::NatType NatTypeFromProtobuf(const protobuf::NatType& nat_type_proto) {
  switch (nat_type_proto) {
    case protobuf::NatType::kSymmetric :
      return rudp::NatType::kSymmetric;
      break;
    case protobuf::NatType::kOther :
      return rudp::NatType::kOther;
      break;
    default :
      return rudp::NatType::kUnknown;
      break;
  }
}

std::string PrintMessage(const protobuf::Message& message) {
  std::string s = "\n\n Message type : ";
  s += MessageTypeString(message);
  std::string direct((message.direct() ? "direct" : "group"));
  s += std::string("\n direct : " + direct);
  if (message.has_source_id())
      s += std::string("\n source_id : " + HexSubstr(message.source_id()));
  if (message.has_destination_id())
    s += std::string("\n destination_id : " + HexSubstr(message.destination_id()));
  if (message.has_relay_id())
    s += std::string("\n relay_id : " + HexSubstr(message.relay_id()));
  if (message.has_relay_connection_id())
    s += std::string("\n relay_connection_id : " + HexSubstr(message.relay_connection_id()));
  std::stringstream id;
  id << message.id();
  if (message.has_id())
    s += std::string("\n id : "+ id.str());
  s += "\n\n";
  return s;
}

std::vector<NodeId> DeserializeNodeIdList(const std::string &node_list_str) {
  std::vector<NodeId> node_list;
  protobuf::NodeIdList node_list_msg;
  node_list_msg.ParseFromString(node_list_str);
  for (int i = 0; i < node_list_msg.node_id_list_size(); ++i)
    node_list.push_back(NodeId(node_list_msg.node_id_list(i).node_id()));
  return node_list;
}

std::string SerializeNodeIdList(const std::vector<NodeId> &node_list) {
  protobuf::NodeIdList node_list_msg;
  for (const auto& node_id : node_list) {
    auto entry = node_list_msg.add_node_id_list();
    entry->set_node_id(node_id.string());
  }
  return node_list_msg.SerializeAsString();
}

}  // namespace routing

}  // namespace maidsafe
