/*
 * libdatachannel example web client
 * Copyright (C) 2020 Lara Mackey
 * Copyright (C) 2020 Paul-Louis Ageneau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

window.addEventListener('load', () => {

  const config = {
    iceServers: [{
      urls: 'stun:stun.l.google.com:19302', // change to your STUN server
    }],
  };

  pc = createPeerConnection();

  async function waitGatheringComplete() {
    return new Promise((resolve) => {
      if (pc.iceGatheringState === 'complete') {
        resolve();
      } else {
        pc.addEventListener('icegatheringstatechange', () => {
          if (pc.iceGatheringState === 'complete') {
            resolve();
          }
        });
      }
    });
  }
  async function sendAnswer(pc) {
    await pc.setLocalDescription(await pc.createAnswer());
    await waitGatheringComplete();

    const answer = pc.localDescription;
    console.log('answer-sdp:' + answer.sdp);
    document.getElementById('answer-sdp').textContent = answer.sdp;
  }

  // Create and setup a PeerConnection
  function createPeerConnection() {

    const pc = new RTCPeerConnection(config);
    RDBtn.onclick = () => {
      pc.setRemoteDescription({
        sdp: RDId.value + "\r\n",
        type: "offer"
      });
      sendAnswer(pc);
    }
    pc.oniceconnectionstatechange = () =>
      console.log(`Connection state: ${pc.iceConnectionState}`);
    pc.onicegatheringstatechange = () =>
      console.log(`Gathering state: ${pc.iceGatheringState}`);
    pc.onicecandidate = (e) => {
      if (e.candidate && e.candidate.candidate) {
        // Send candidate
      }
    };

    pc.ondatachannel = (e) => {
      const dc = e.channel;
      let id = dc.label;
      console.log(`"DataChannel from "${dc.label}"`);
      setupDataChannel(dc,id);

    };

    return pc;
  }

  // Setup a DataChannel
  function setupDataChannel(dc,id) {
    dc.onopen = () => {
      console.log(`DataChannel from ${id} open`);

      sendMsg.disabled = false;
      sendBtn.disabled = false;
      sendBtn.onclick = () => dc.send(sendMsg.value);
    };
    dc.onclose = () => { console.log(`DataChannel from ${id} closed`); };
    dc.onmessage = (e) => {
      if (typeof (e.data) != 'string')
        return;
      console.log(`Message from ${id} received: ${e.data}`);
      document.body.appendChild(document.createTextNode(e.data));
    };

    return dc;
  }


});
