// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_OPERATIONS_CREATE_DIRECTORY_H_
#define CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_OPERATIONS_CREATE_DIRECTORY_H_

#include "base/files/file.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/chromeos/file_system_provider/operations/operation.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/file_system_provider/request_value.h"
#include "storage/browser/fileapi/async_file_util.h"

namespace base {
class FilePath;
}  // namespace base

namespace extensions {
class EventRouter;
}  // namespace extensions

namespace chromeos {
namespace file_system_provider {
namespace operations {

// Creates a directory. If |recursive| is set to true, then creates also all
// non-existing directories on the path. The operation will fail if the
// directory already exists. Created per request.
class CreateDirectory : public Operation {
 public:
  CreateDirectory(extensions::EventRouter* event_router,
                  const ProvidedFileSystemInfo& file_system_info,
                  const base::FilePath& directory_path,
                  bool recursive,
                  const storage::AsyncFileUtil::StatusCallback& callback);
  virtual ~CreateDirectory();

  // Operation overrides.
  virtual bool Execute(int request_id) override;
  virtual void OnSuccess(int request_id,
                         scoped_ptr<RequestValue> result,
                         bool has_more) override;
  virtual void OnError(int request_id,
                       scoped_ptr<RequestValue> result,
                       base::File::Error error) override;

 private:
  base::FilePath directory_path_;
  bool recursive_;
  const storage::AsyncFileUtil::StatusCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(CreateDirectory);
};

}  // namespace operations
}  // namespace file_system_provider
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_OPERATIONS_CREATE_DIRECTORY_H_
