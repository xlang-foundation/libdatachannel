/**
 * Copyright (c) 2020 Filip Klembara (in2core)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#if RTC_ENABLE_MEDIA

#include "rtppacketizer.hpp"

#include <cmath>
#include <cstring>

namespace rtc {

RtpPacketizer::RtpPacketizer(shared_ptr<RtpPacketizationConfig> rtpConfig) : rtpConfig(rtpConfig) {}

RtpPacketizer::~RtpPacketizer() {}

std::vector<binary> RtpPacketizer::fragment(binary data) {
	// Default implementation
	return {std::move(data)};
}

message_ptr RtpPacketizer::packetize(const binary &payload, bool mark) {
	size_t rtpExtHeaderSize = 0;

	const bool setVideoRotation = (rtpConfig->videoOrientationId != 0) &&
	                              (rtpConfig->videoOrientationId <
	                               15) && // needs fixing if longer extension headers are supported
	                              mark &&
	                              (rtpConfig->videoOrientation != 0);

	if (setVideoRotation)
		rtpExtHeaderSize += 2;

	const bool setPlayoutDelay = (rtpConfig->playoutDelayId > 0 && rtpConfig->playoutDelayId < 15);

	if (setPlayoutDelay)
		rtpExtHeaderSize += 4;

	if (rtpConfig->mid.has_value())
		rtpExtHeaderSize += (1 + rtpConfig->mid->length());

	if (rtpConfig->rid.has_value())
		rtpExtHeaderSize += (1 + rtpConfig->rid->length());

	if (rtpExtHeaderSize != 0)
		rtpExtHeaderSize += 4;

	rtpExtHeaderSize = (rtpExtHeaderSize + 3) & ~3;

	auto message = make_message(RtpHeaderSize + rtpExtHeaderSize + payload.size());
	auto *rtp = (RtpHeader *)message->data();
	rtp->setPayloadType(rtpConfig->payloadType);
	rtp->setSeqNumber(rtpConfig->sequenceNumber++); // increase sequence number
	rtp->setTimestamp(rtpConfig->timestamp);
	rtp->setSsrc(rtpConfig->ssrc);

	if (mark) {
		rtp->setMarker(true);
	}

	if (rtpExtHeaderSize) {
		rtp->setExtension(true);

		auto extHeader = rtp->getExtensionHeader();
		extHeader->setProfileSpecificId(0xbede);

		auto headerLength = static_cast<uint16_t>(rtpExtHeaderSize / 4) - 1;

		extHeader->setHeaderLength(headerLength);
		extHeader->clearBody();

		size_t offset = 0;
		if (setVideoRotation) {
			extHeader->writeCurrentVideoOrientation(offset, rtpConfig->videoOrientationId,
			                                        rtpConfig->videoOrientation);
			offset += 2;
		}

		if (rtpConfig->mid.has_value()) {
			extHeader->writeOneByteHeader(
			    offset, rtpConfig->midId,
			    reinterpret_cast<const std::byte *>(rtpConfig->mid->c_str()),
			    rtpConfig->mid->length());
			offset += (1 + rtpConfig->mid->length());
		}

		if (rtpConfig->rid.has_value()) {
			extHeader->writeOneByteHeader(
			    offset, rtpConfig->ridId,
			    reinterpret_cast<const std::byte *>(rtpConfig->rid->c_str()),
			    rtpConfig->rid->length());
		}

		if (setPlayoutDelay) {
			uint16_t min = rtpConfig->playoutDelayMin & 0xFFF;
			uint16_t max = rtpConfig->playoutDelayMax & 0xFFF;

			// 12 bits for min + 12 bits for max
			byte data[] = {byte((min >> 4) & 0xFF), byte(((min & 0xF) << 4) | ((max >> 8) & 0xF)),
			               byte(max & 0xFF)};

			extHeader->writeOneByteHeader(offset, rtpConfig->playoutDelayId, data, 3);
			offset += 4;
		}
	}

	rtp->preparePacket();

	std::memcpy(message->data() + RtpHeaderSize + rtpExtHeaderSize, payload.data(), payload.size());

	return message;
}

message_ptr RtpPacketizer::packetize(shared_ptr<binary> payload, bool mark) {
	return packetize(*payload, mark);
}

void RtpPacketizer::media([[maybe_unused]] const Description::Media &desc) {}

void RtpPacketizer::outgoing(message_vector &messages,
                             [[maybe_unused]] const message_callback &send) {
	message_vector result;
	for (const auto &message : messages) {
		if (const auto &frameInfo = message->frameInfo) {
			if (frameInfo->payloadType && frameInfo->payloadType != rtpConfig->payloadType)
				continue;

			if (frameInfo->timestampSeconds)
				rtpConfig->timestamp =
				    rtpConfig->startTimestamp +
				    rtpConfig->secondsToTimestamp(
				        std::chrono::duration<double>(*frameInfo->timestampSeconds).count());
			else
				rtpConfig->timestamp = frameInfo->timestamp;
		}

		auto payloads = fragment(std::move(*message));
		if (payloads.size() > 0) {
			for (size_t i = 0; i < payloads.size() - 1; i++)
				result.push_back(packetize(payloads[i], false));

			result.push_back(packetize(payloads[payloads.size() - 1], true));
		}
	}

	messages.swap(result);
}

} // namespace rtc

#endif /* RTC_ENABLE_MEDIA */
