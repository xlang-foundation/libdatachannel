#include "DataChannelBinding.h"
#include "xhost.h" // Host definitions for invoking callbacks

namespace X {
namespace LibDataChannel {

// DataChannel implementation
DataChannel::DataChannel(){}

void DataChannel::Set(std::shared_ptr<rtc::DataChannel> dc) {
	m_dc = dc;
	m_dc->onMessage([this](rtc::message_variant message) {
		if (!m_messageCallback.IsObject())
			return;
		// Handle string messages
		if (std::holds_alternative<rtc::string>(message)) {
			auto &s = std::get<rtc::string>(message);
			X::ARGS args(1);
			args.push_back(X::Value(s));
			m_messageCallback.ObjCall(args);
		}
		// Handle binary messages
		else if (std::holds_alternative<rtc::binary>(message)) {
			auto &b = std::get<rtc::binary>(message);
			std::string msgStr(reinterpret_cast<const char *>(b.data()), b.size());
			X::ARGS args(1);
			args.push_back(msgStr);
			m_messageCallback.ObjCall(args);
		}
	});
}

DataChannel::~DataChannel() {
	if (m_dc) {
		m_dc->close();
		m_dc = nullptr;
	}
}

bool DataChannel::Send(const std::string &message) {
	if (!m_dc)
		return false;
	m_dc->send(message);
	return true;
}

void DataChannel::Close() {
	if (m_dc) {
		m_dc->close();
	}
}

void DataChannel::OnMessageCallback(X::XRuntime *rt, X::XObj *pContext, X::Value callback) {
	m_messageCallback = callback;
}

// Manager implementation
X::Value Manager::Create(X::XRuntime *rt, X::XObj *pContext, std::string label, X::Value options) {
	rtc::Configuration config;

	// If options is a dict, use its methods
	if (options.IsValid() && options.IsDict()) {
		X::Dict dict(options);
		if (dict->Has("iceServers")) {
			X::Value servers = dict["iceServers"];
			if (servers.IsList()) {
				X::List listServers(servers);
				for (auto &sv : *listServers) {
					if (sv.IsString()) {
						config.iceServers.emplace_back(sv.ToString());
					} else if (sv.IsObject()) {
						X::Dict d(sv);
						if (d->Has("urls")) {
							X::List urls(d["urls"]);
							for (auto &u : *urls)
								config.iceServers.emplace_back(u.ToString());
						}
					}
				}
			}
		}
		// Additional options can be added here
	}

	auto pc = std::make_shared<rtc::PeerConnection>(config);
	auto dc = pc->createDataChannel(label);

	X::XPackageValue<DataChannel> valDC;
	DataChannel& x_dc = *valDC;
	x_dc.Set(dc);
	return X::Value(valDC);
}

void Manager::SetDefaultOptions(X::XRuntime *rt, X::XObj *pContext, X::Value options) {
	//rt->SetGlobalVar("__ldc_default_opts__", options);
}

} // namespace LibDataChannel
} // namespace X
