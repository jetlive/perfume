/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2014 PeterXu uskee521@gmail.com
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "peer.h"
#include "error.h"

namespace xrtc {

class DummySetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:
    static DummySetSessionDescriptionObserver* Create() {
        return new talk_base::RefCountedObject<DummySetSessionDescriptionObserver>();
    }
    virtual void OnSuccess() {
    }
    virtual void OnFailure(const std::string& error) {
    }

protected:
    DummySetSessionDescriptionObserver() {}
    ~DummySetSessionDescriptionObserver() {}
};


//
//> for CRTCPeerConnection
bool CRTCPeerConnection::Init(talk_base::scoped_refptr<webrtc::PeerConnectionFactoryInterface> pc_factory)
{
    returnb_assert (pc_factory.get() != NULL);

    m_observer = new talk_base::RefCountedObject<CRTCPeerConnectionObserver>();
    m_observer->Put_EventHandler(this->Get_EventHandler());

    webrtc::PeerConnectionInterface::IceServers servers;
    webrtc::PeerConnectionInterface::IceServer server;
    server.uri = kDefaultIceServer;
    servers.push_back(server);

    m_conn = pc_factory->CreatePeerConnection(servers, NULL, NULL, (webrtc::PeerConnectionObserver *)m_observer);
    returnb_assert(m_conn.get() != NULL);
    return m_observer->Init(this, m_conn);
}
 
CRTCPeerConnection::CRTCPeerConnection ()
{
    m_conn = NULL;
    m_observer = NULL; 
}

CRTCPeerConnection::~CRTCPeerConnection ()
{
    m_conn = NULL;
    m_observer = NULL;
    m_local_streams.clear();
    m_remote_streams.clear();
}

void * CRTCPeerConnection::getptr() 
{
    return m_conn.get();
}

void CRTCPeerConnection::setParams (const RTCConfiguration & configuration, const MediaConstraints & constraints)
{}

void CRTCPeerConnection::createOffer (const MediaConstraints & constraints)
{
    return_assert(m_conn.get());
    m_conn->CreateOffer((webrtc::CreateSessionDescriptionObserver *)m_observer, NULL);
}

void CRTCPeerConnection::createAnswer (const MediaConstraints & constraints)
{
    return_assert(m_conn.get());
    m_conn->CreateAnswer((webrtc::CreateSessionDescriptionObserver *)m_observer, NULL);
}

void CRTCPeerConnection::setLocalDescription (const RTCSessionDescription & description)
{
    return_assert(m_conn.get());
    webrtc::SessionDescriptionInterface* desp(webrtc::CreateSessionDescription(description.type, description.sdp));
    if (desp) {
        m_conn->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desp);
    }
}

void CRTCPeerConnection::setRemoteDescription (const RTCSessionDescription & description)
{
    return_assert(m_conn.get());
    webrtc::SessionDescriptionInterface* desp(webrtc::CreateSessionDescription(description.type, description.sdp));
    if (desp) {
        m_conn->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(), desp);
    }
}

void CRTCPeerConnection::updateIce (const RTCConfiguration & configuration, const MediaConstraints & constraints)
{
}

void CRTCPeerConnection::addIceCandidate (const RTCIceCandidate & ice)
{
    return_assert(m_conn.get());
    talk_base::scoped_ptr<webrtc::IceCandidateInterface> candidate(
            webrtc::CreateIceCandidate(ice.sdpMid, ice.sdpMLineIndex, ice.candidate));
    m_conn->AddIceCandidate(candidate.get());
}

sequence<MediaStreamPtr> & CRTCPeerConnection::getLocalStreams ()
{
    return m_local_streams;
}

sequence<MediaStreamPtr> & CRTCPeerConnection::getRemoteStreams ()
{
    return m_remote_streams;
}

MediaStreamPtr CRTCPeerConnection::getStreamById (DOMString streamId)
{
    sequence<MediaStreamPtr>::iterator iter;
    sequence<MediaStreamPtr> streams = getLocalStreams();
    for (iter=streams.begin(); iter != streams.end(); iter++) {
        if ((*iter)->Get_id() == streamId) {
            return *iter;
        }
    }

    streams = getRemoteStreams();
    for (iter=streams.begin(); iter != streams.end(); iter++) {
        if ((*iter)->Get_id() == streamId) {
            return *iter;
        }
    }

    return NULL;
}

void CRTCPeerConnection::addStream (MediaStreamPtr stream, const MediaConstraints & constraints)
{
    return_assert(m_conn.get());
    if (stream && stream->getptr()) {
        webrtc::MediaConstraintsInterface* constraints = NULL;
        m_conn->AddStream((webrtc::MediaStreamInterface *)stream->getptr(), constraints);
        m_local_streams.push_back(stream);
    }
}

void CRTCPeerConnection::removeStream (MediaStreamPtr stream)
{
    return_assert(m_conn.get());
    if (stream && stream->getptr()) {
        m_conn->RemoveStream((webrtc::MediaStreamInterface *)stream->getptr());
        m_local_streams.clear(); // TODO: now only support one local stream       
    }
}

void CRTCPeerConnection::close ()
{
    return_assert(m_conn.get());
    m_conn->Close();
}


//
//> for create interface
ubase::zeroptr<RTCPeerConnection> CreatePeerConnection(talk_base::scoped_refptr<webrtc::PeerConnectionFactoryInterface> pc_factory) {
    ubase::zeroptr<CRTCPeerConnection> pc = new ubase::RefCounted<CRTCPeerConnection>();
    if (!pc.get() || !pc->Init(pc_factory)) {
        pc = NULL;
    }
    return pc;
}


} //namespace xrtc
