// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/renderer_save_password_progress_logger.h"

#include "components/autofill/content/common/autofill_messages.h"
#include "ipc/ipc_sender.h"

namespace autofill {

RendererSavePasswordProgressLogger::RendererSavePasswordProgressLogger(
    IPC::Sender* sender,
    int routing_id)
    : sender_(sender), routing_id_(routing_id) {
  DCHECK(sender_);
}

RendererSavePasswordProgressLogger::~RendererSavePasswordProgressLogger() {}

void RendererSavePasswordProgressLogger::SendLog(const std::string& log) {
  sender_->Send(
      new AutofillHostMsg_RecordSavePasswordProgress(routing_id_, log));
}

}  // namespace autofill
