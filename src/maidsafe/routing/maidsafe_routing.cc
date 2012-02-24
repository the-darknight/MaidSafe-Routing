/* Copyright (c) 2009 maidsafe.net limited
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    * Neither the name of the maidsafe.net limited nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <memory>
#include <queue>
#include <deque>

#include "boost/thread/locks.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"

#include "maidsafe/routing/routing.pb.h"
#include "maidsafe/routing/routing_table.h"
#include "maidsafe/routing/maidsafe_routing.h"
#include "maidsafe/routing/log.h"

#include "maidsafe/transport/rudp_transport.h"
#include "maidsafe/transport/transport.h"
#include "maidsafe/transport/utils.h"

#include "maidsafe/common/rsa.h"
#include "maidsafe/common/utils.h"


namespace maidsafe {
  // utility for iterable queue - should go in common
  // just a wrapper to get at deque iterators
  // a better/safer method would be to inherit form dqueue
  // and create our own unasoci
template<typename T, typename Container=std::deque<T>>
class iqueue : public std::queue<T,Container>
{
public:
    typedef typename Container::iterator iterator;
    typedef typename Container::const_iterator const_iterator;

    iterator begin() { return this->c.begin(); }
    iterator end() { return this->c.end(); }
    const_iterator begin() const { return this->c.begin(); }
    const_iterator end() const { return this->c.end(); }
    void Resize(int t) { return this->c.resize(t); } // TODO FIXME TESTME
};

namespace routing {

  namespace bfs = boost::filesystem3;
  typedef bfs::ifstream ifs;
  typedef bfs::ofstream ofs;
  typedef protobuf::Contact Contact;
 // typedef protobuf::Message Message;
  
class RoutingImpl {
 public:
   RoutingImpl();
   bool ReadConfigFile();
   bool WriteConfigFile();
   transport::Endpoint GetLocalEndpoint();
   void RecieveMessage(std::string &message);
   void ProcessMessage(protobuf::Message &message);
   void doFindNodeResponse(protobuf::Message &message);
   void doFindNodeRequest(protobuf::Message &message);
   void doConnectResponse(protobuf::Message &message);
   void doConnectRequest(protobuf::Message &message);
   void SendOn(const protobuf::Message &message, NodeId &node);
   boost::asio::io_service service_;
   int16_t cache_size_hint_;
   std::vector<Contact> bootstrap_nodes_;
   NodeId my_node_id_;
   asymm::PrivateKey my_private_key_;
   Contact my_contact_;
   RoutingTable routing_table_;
   transport::RudpTransport transport_;
   bfs::path config_file_;
   bool private_key_is_set_;
   bool node_is_set_;
   std::map<NodeId, asymm::PublicKey> public_keys_;
   maidsafe::iqueue<std::pair<std::string, std::string> > cache_chunks_;
};

RoutingImpl::RoutingImpl() :
          
  service_(),
  cache_size_hint_(kNumChunksToCache),
  bootstrap_nodes_(),
  my_node_id_(),
  my_private_key_(),
  my_contact_(),
  routing_table_(my_contact_), // TODO FIXME contact is empty here
  transport_(service_),
  config_file_("dht_config"),
  private_key_is_set_(false),
  node_is_set_(false),
  public_keys_(), 
  cache_chunks_()
  {}

bool RoutingImpl::ReadConfigFile() {
  protobuf::ConfigFile protobuf;
  if (!bfs::exists(config_file_) || !bfs::is_regular_file(config_file_))
    return false;
  try {
    ifs config_file_stream(config_file_);
    if (!protobuf.ParseFromString(config_file_.c_str()))
      return false;
    if(!private_key_is_set_) {
      if(!protobuf.has_private_key()) {
        return false;
      } else {
        asymm::DecodePrivateKey(protobuf.private_key(), &my_private_key_);
      }
    }
    if (!node_is_set_) {
      if(protobuf.has_node_id()) {
         my_node_id_ = NodeId(protobuf.node_id());
       } else {
        return false;
       }
    }
    for (auto i = 0; i != protobuf.contact_size(); ++i) 
       bootstrap_nodes_.push_back(protobuf.contact(i));
  }  catch(const std::exception &e) {
    // TODO fix DLOG - need to update common log types
//     DLOG(WARNING) << "Exception: " << e.what();
    return false;
  }
  return true;
}

bool RoutingImpl::WriteConfigFile() {
  // TODO implement 
return false;
}

Routing::Routing() : pimpl_(new RoutingImpl())  {}

bool Routing::setConfigFile(boost::filesystem3::path &config_file) {
   if (bfs::exists(config_file) && !bfs::is_regular_file(config_file))
    return false;
   pimpl_->config_file_ = config_file;
}

bool Routing::setMyPrivateKey(asymm::PrivateKey& key) {
  pimpl_->my_private_key_ = key;
  pimpl_->private_key_is_set_ = true;
  return true;
}

asymm::PrivateKey Routing::MyPrivateKey() {
  return pimpl_->my_private_key_;
}

bool Routing::setMyNodeId(NodeId& node) {
  pimpl_->my_node_id_ = node;
  pimpl_->node_is_set_ = true;
  return true;
}

NodeId Routing::MyNodeID() {
  return pimpl_->my_node_id_;
}

transport::Endpoint RoutingImpl::GetLocalEndpoint() {
//   std::vector<IP> local_ips(transport::GetLocalAddresses());
//   transport::Port  port = RandomInt32() % 1600 + 30000;
// // TODO we must only listen on the correct local port
//   // this is a very old issue.
//   bool breakme(false);
//   for (uint16_t i = port; i < 35000; ++i) {
//     for (auto it = local_ips.begin(); it != local_ips.end(); ++it) {
//       transport::Endpoint ep;
//       ep.ip = *it;
//       ep.port = i;
//       if (pimpl_->transport_.StartListening(ep) == transport::kSuccess) {
//         // TODO check we can get to at least a bootsrap node !!! then we
//         // have the correct ep
// //         if (send and recieve)  // maybe connect is enough !!
// //          break; ou of both loops - set a var
//             breakme = true;
// //         else
// //           pimpl_->transport_.StopListening();
//       }
//       if (breakme)
//         break;
//     }
//   }
}

bool Routing::StartVault(boost::asio::io_service& service) { // NOLINT
   if (! pimpl_->ReadConfigFile())
     return false;
   pimpl_->transport_ = (transport::RudpTransport(service));
   pimpl_->routing_table_ = RoutingTable(pimpl_->my_contact_);
   return false; // not implemented need to start network and routing table
}

bool Routing::StartClient(boost::asio::io_service& service) {
  //TODO client will join network using pmid BUT will request a
  // relay conenction. Vaults (i.e. routing table) will accept a range
  // of these, Initially set to 64 but we shoudl make this dynamic later
}

void Routing::Send(const protobuf::Message &msg) {
 NodeId next_node =
        pimpl_->routing_table_.GetClosestNode(NodeId(msg.destination_id()));
 pimpl_->SendOn(msg, next_node);
}

void RoutingImpl::RecieveMessage(std::string &message) {
  protobuf::Message msg;
  if(msg.ParseFromString(message))
    ProcessMessage(msg);
}

bool isCacheable(protobuf::Message &message) {
 return (message.has_cachable() && message.cachable());
}

bool isDirect(protobuf::Message &message) {
  return (message.has_direct() && message.direct());
}

void RoutingImpl::ProcessMessage(protobuf::Message& message) {
  // handle cache data
  if (isCacheable(message)) {
    if (message.response()) {
      std::pair<std::string, std::string> data;
      try {
        // check data is valid TODO FIXME - ask CAA
        if (crypto::Hash<crypto::SHA512>(message.data()) != message.source_id())
          return;
        data = std::make_pair(message.source_id(), message.data());
        cache_chunks_.push(data);
        if (cache_chunks_.size() > cache_size_hint_)
          cache_chunks_.pop();
      } catch (std::exception &e) {
        // oohps reduce cache size quickly
//         for (int16_t i = 0; i < (cache_size_hint_ / 2) ; ++i)
//           cache_chunks_.pop();
        cache_size_hint_ = cache_size_hint_ / 2;
        cache_chunks_.Resize(cache_size_hint_);
      }
    }
  } else  { // request
     for(auto it = cache_chunks_.begin(); it != cache_chunks_.end(); ++it) {
       if ((*it).first == message.source_id()) {
          message.set_destination_id(message.source_id());
          message.set_cachable(true);
          message.set_data((*it).second);
          message.set_source_id(my_node_id_.String());
          message.set_direct(true);
          message.set_response(false);
          NodeId next_node =
              routing_table_.GetClosestNode(NodeId(message.destination_id()));
          SendOn(message, next_node);
          return;
       }
     }
  //     if (cache_chunks_.search(key)protobuf::Message
  //       SendOn(message); // TODO back to source_id
    // TODO check our cache and send back response
  }
  // is it for us ??
  if (!routing_table_.AmIClosestNode(NodeId(message.destination_id()))) {
    NodeId next_node =
              routing_table_.GetClosestNode(NodeId(message.destination_id()));
    SendOn(message, next_node);
  } else { // I am closest
    if (isDirect(message)) {
      if (message.destination_id() != my_node_id_.String()) {
      // TODO send back a failure I presume !!
      } else {
        //Signal up
      }
    }
    if (message.type() == 1) // find_nodes
      if (message.has_response() && message.response()) {
        doFindNodeResponse(message);
        return;
      } else {
        doFindNodeRequest(message);
      }
    // TODO again with the signals we are closest node
    // could check with next line
    if (routing_table_.IsMyNodeInRange(NodeId
                                            (message.destination_id()))){
      // signal we are in closest if it helps logic
    }
  }
}

void RoutingImpl::SendOn(const protobuf::Message& message, NodeId& node) {
  // TODO managed connections get this !! post to asio_service !!
}

void RoutingImpl::doFindNodeResponse(protobuf::Message& message)
{
   protobuf::FindNodesResponse find_nodes;
   if (! find_nodes.ParseFromString(message.data()))
     return;
   for (uint i = 0; i < find_nodes.close_contacts().size(); ++i)
     routing_table_.AddNode(NodeId(find_nodes.close_contacts().at(i)));
   
}

void RoutingImpl::doConnectResponse(protobuf::Message& message)
{
  // TODO - check contact for direct conencted node - i.e try a
  // quick connect / ping to the remote endpoint and if reachable
  // store in bootstrap_nodes_ and do a WriteConfigFile()
  // this may be where we need a ping command to iterate and remove
  // any long dead nodes from the table.
  // Keep at least 1000 nodes in table and drop any dead beyond this
}


// TODO get messages from transport

}  // namespace routing
}  // namespace maidsafe