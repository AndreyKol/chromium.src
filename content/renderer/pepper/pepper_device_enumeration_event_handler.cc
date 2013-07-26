// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_device_enumeration_event_handler.h"

#include "base/logging.h"
#include "content/renderer/media/media_stream_dispatcher.h"
#include "content/renderer/render_view_impl.h"
#include "ppapi/shared_impl/ppb_device_ref_shared.h"

namespace content {

namespace {

ppapi::DeviceRefData FromStreamDeviceInfo(const StreamDeviceInfo& info) {
  ppapi::DeviceRefData data;
  data.id = info.device.id;
  data.name = info.device.name;
  data.type = PepperDeviceEnumerationEventHandler::FromMediaStreamType(
      info.device.type);
  return data;
}

}  // namespace

PepperDeviceEnumerationEventHandler*
    PepperDeviceEnumerationEventHandler::GetForRenderView(
        RenderView* render_view) {
  PepperDeviceEnumerationEventHandler* handler =
      PepperDeviceEnumerationEventHandler::Get(render_view);
  if (!handler)
    handler = new PepperDeviceEnumerationEventHandler(render_view);
  return handler;
}

PepperDeviceEnumerationEventHandler::PepperDeviceEnumerationEventHandler(
    RenderView* render_view)
    : RenderViewObserver(render_view),
      RenderViewObserverTracker<PepperDeviceEnumerationEventHandler>(
          render_view),
      next_id_(1) {
}

PepperDeviceEnumerationEventHandler::~PepperDeviceEnumerationEventHandler() {
  DCHECK(enumerate_callbacks_.empty());
  DCHECK(open_callbacks_.empty());
}

int PepperDeviceEnumerationEventHandler::EnumerateDevices(
    PP_DeviceType_Dev type,
    const EnumerateDevicesCallback& callback) {
  enumerate_callbacks_[next_id_] = callback;
  int request_id = next_id_++;

#if defined(ENABLE_WEBRTC)
  GetRenderViewImpl()->media_stream_dispatcher()->EnumerateDevices(
      request_id, AsWeakPtr(),
      PepperDeviceEnumerationEventHandler::FromPepperDeviceType(type),
      GURL());
#else
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(
          &PepperDeviceEnumerationEventHandler::OnDevicesEnumerationFailed,
          AsWeakPtr(),
          request_id));
#endif

  return request_id;
}

void PepperDeviceEnumerationEventHandler::StopEnumerateDevices(int request_id) {
  enumerate_callbacks_.erase(request_id);

#if defined(ENABLE_WEBRTC)
  // Need to post task since this function might be called inside the callback
  // of EnumerateDevices.
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&MediaStreamDispatcher::StopEnumerateDevices,
                 GetRenderViewImpl()->media_stream_dispatcher()->AsWeakPtr(),
                 request_id,
                 AsWeakPtr()));
#endif
}

int PepperDeviceEnumerationEventHandler::OpenDevice(
    PP_DeviceType_Dev type,
    const std::string& device_id,
    const GURL& document_url,
    const OpenDeviceCallback& callback) {
  open_callbacks_[next_id_] = callback;
  int request_id = next_id_++;

#if defined(ENABLE_WEBRTC)
  GetRenderViewImpl()->media_stream_dispatcher()->
      OpenDevice(
          request_id,
          AsWeakPtr(),
          device_id,
          PepperDeviceEnumerationEventHandler::FromPepperDeviceType(type),
          document_url.GetOrigin());
#else
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&PepperDeviceEnumerationEventHandler::OnDeviceOpenFailed,
                 AsWeakPtr(),
                 request_id));
#endif

  return request_id;
}

void PepperDeviceEnumerationEventHandler::CancelOpenDevice(int request_id) {
  open_callbacks_.erase(request_id);

#if defined(ENABLE_WEBRTC)
  GetRenderViewImpl()->media_stream_dispatcher()->CancelOpenDevice(
      request_id, AsWeakPtr());
#endif
}

void PepperDeviceEnumerationEventHandler::CloseDevice(
    const std::string& label) {
#if defined(ENABLE_WEBRTC)
  GetRenderViewImpl()->media_stream_dispatcher()->CloseDevice(label);
#endif
}

int PepperDeviceEnumerationEventHandler::GetSessionID(
    PP_DeviceType_Dev type,
    const std::string& label) {
#if defined(ENABLE_WEBRTC)
  switch (type) {
    case PP_DEVICETYPE_DEV_AUDIOCAPTURE:
      return GetRenderViewImpl()->media_stream_dispatcher()->audio_session_id(
          label, 0);
    case PP_DEVICETYPE_DEV_VIDEOCAPTURE:
      return GetRenderViewImpl()->media_stream_dispatcher()->video_session_id(
          label, 0);
    default:
      NOTREACHED();
      return 0;
  }
#else
  return 0;
#endif
}

void PepperDeviceEnumerationEventHandler::OnStreamGenerated(
    int request_id,
    const std::string& label,
    const StreamDeviceInfoArray& audio_device_array,
    const StreamDeviceInfoArray& video_device_array) {
}

void PepperDeviceEnumerationEventHandler::OnStreamGenerationFailed(
    int request_id) {
}

void PepperDeviceEnumerationEventHandler::OnDevicesEnumerated(
    int request_id,
    const StreamDeviceInfoArray& device_array) {
  NotifyDevicesEnumerated(request_id, true, device_array);
}

void PepperDeviceEnumerationEventHandler::OnDevicesEnumerationFailed(
    int request_id) {
  NotifyDevicesEnumerated(request_id, false, StreamDeviceInfoArray());
}

void PepperDeviceEnumerationEventHandler::OnDeviceOpened(
    int request_id,
    const std::string& label,
    const StreamDeviceInfo& device_info) {
  NotifyDeviceOpened(request_id, true, label);
}

void PepperDeviceEnumerationEventHandler::OnDeviceOpenFailed(int request_id) {
  NotifyDeviceOpened(request_id, false, std::string());
}

// static
MediaStreamType PepperDeviceEnumerationEventHandler::FromPepperDeviceType(
    PP_DeviceType_Dev type) {
  switch (type) {
    case PP_DEVICETYPE_DEV_INVALID:
      return MEDIA_NO_SERVICE;
    case PP_DEVICETYPE_DEV_AUDIOCAPTURE:
      return MEDIA_DEVICE_AUDIO_CAPTURE;
    case PP_DEVICETYPE_DEV_VIDEOCAPTURE:
      return MEDIA_DEVICE_VIDEO_CAPTURE;
    default:
      NOTREACHED();
      return MEDIA_NO_SERVICE;
  }
}

// static
PP_DeviceType_Dev PepperDeviceEnumerationEventHandler::FromMediaStreamType(
    MediaStreamType type) {
  switch (type) {
    case MEDIA_NO_SERVICE:
      return PP_DEVICETYPE_DEV_INVALID;
    case MEDIA_DEVICE_AUDIO_CAPTURE:
      return PP_DEVICETYPE_DEV_AUDIOCAPTURE;
    case MEDIA_DEVICE_VIDEO_CAPTURE:
      return PP_DEVICETYPE_DEV_VIDEOCAPTURE;
    default:
      NOTREACHED();
      return PP_DEVICETYPE_DEV_INVALID;
  }
}

void PepperDeviceEnumerationEventHandler::NotifyDevicesEnumerated(
    int request_id,
    bool succeeded,
    const StreamDeviceInfoArray& device_array) {
  EnumerateCallbackMap::iterator iter = enumerate_callbacks_.find(request_id);
  if (iter == enumerate_callbacks_.end()) {
    // This might be enumerated result sent before StopEnumerateDevices is
    // called since EnumerateDevices is persistent request.
    return;
  }

  EnumerateDevicesCallback callback = iter->second;

  std::vector<ppapi::DeviceRefData> devices;
  if (succeeded) {
    devices.reserve(device_array.size());
    for (StreamDeviceInfoArray::const_iterator info =
             device_array.begin(); info != device_array.end(); ++info) {
      devices.push_back(FromStreamDeviceInfo(*info));
    }
  }
  callback.Run(request_id, succeeded, devices);
}

void PepperDeviceEnumerationEventHandler::NotifyDeviceOpened(
    int request_id,
    bool succeeded,
    const std::string& label) {
  OpenCallbackMap::iterator iter = open_callbacks_.find(request_id);
  if (iter == open_callbacks_.end()) {
    // The callback may have been unregistered.
    return;
  }

  OpenDeviceCallback callback = iter->second;
  open_callbacks_.erase(iter);

  callback.Run(request_id, succeeded, label);
}

RenderViewImpl* PepperDeviceEnumerationEventHandler::GetRenderViewImpl() {
  return static_cast<RenderViewImpl*>(render_view());
}

}  // namespace content
