from datachannel import webrtc
options = {
	iceServers = [
		{
			urls: "stun:stun.l.google.com:19302"
		},
		{
			urls: "turn:turn.example.com:3478",
			username: "user",
			credential: "pass"
		}
	]
}

dc = webrtc.create("test", options)
def OnMessage(evt):
	print("Received message:", evt.data)
	# Send a response back
	dc.send("Hello, client!")

dc.onmessage(OnMessage)

dc.send("Hello, world!")

dc.close()
print("Done")
