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

def onLocalDescription(description):
	print("Local Description (Paste this to the other peer):\r\n", description)
def onLocalCandidate(candidate):
	print("Local Candidate (Paste this to the other peer after the local description):\r\n", candidate)
def onStateChange(state):
	print("State:", state)
def onGatheringStateChange(state):
	print("Gathering State:", state)

dc = webrtc.create("test", options,onLocalDescription,onLocalCandidate,onStateChange,onGatheringStateChange)

def OnMessage(evt):
	print("Received message:", evt)
	# print("Received message:", evt.data)
	# Send a response back
	# dc.send("Hello, client!")

dc.onmessage(OnMessage)

def setRemoteDescription():
	# 等待用户输入一些文本
	text = input("请输入Description：")
	while True:
		line = input()  # 默认为读取一行，直到用户按下回车键为止
		print("line:", line)
		if line == 'exit':  # 假设用户输入'exit'作为结束信号
			break
		text += "\r\n" + line  # 将每一行添加到文本中，并在行尾添加换行符
	print("你输入的Description是：", text)
	dc.setRemoteDescription(text)

setRemoteDescription()

def addRemoteCandidate():
	# 等待用户输入一些文本
	text1 = input("请输入Candidate：")
	print("你输入的Candidate是：", text1)
	dc.addRemoteCandidate(text1)

#addRemoteCandidate()

def sendMsg():
	while True:
		line1 = input("请输入Message：")  # 默认为读取一行，直到用户按下回车键为止
		if line1 == 'exit':  # 假设用户输入'exit'作为结束信号
			break
		print("你输入的Message是：", line1)
		dc.send(line1)

sendMsg()

dc.send("Hello, world!")
dc.close()
print("Done")
