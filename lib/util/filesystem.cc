// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/modular/lib/util/filesystem.h"

#include <fcntl.h>
#include <magenta/device/vfs.h>
#include <magenta/syscalls.h>
#include <unistd.h>
#include <memory>

#include "lib/ftl/files/file.h"
#include "lib/ftl/files/unique_fd.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/macros.h"
#include "lib/ftl/strings/string_printf.h"
#include "lib/ftl/strings/string_view.h"
#include "lib/ftl/time/time_delta.h"
#include "lib/ftl/time/time_point.h"

namespace modular {

// For polling minfs.
constexpr ftl::StringView kPersistentFileSystem = "/data";
constexpr ftl::StringView kMinFsName = "minfs";
constexpr ftl::TimeDelta kMaxPollingDelay = ftl::TimeDelta::FromSeconds(10);

void WaitForMinfs() {
  auto delay = ftl::TimeDelta::FromMilliseconds(10);
  ftl::TimePoint now = ftl::TimePoint::Now();
  while (ftl::TimePoint::Now() - now < kMaxPollingDelay) {
    ftl::UniqueFD fd(open(kPersistentFileSystem.data(), O_RDONLY));
    if (fd.is_valid()) {
      char buf[sizeof(vfs_query_info_t) + MAX_FS_NAME_LEN + 1];
      auto* info = reinterpret_cast<vfs_query_info_t*>(buf);
      ssize_t len = ioctl_vfs_query_fs(fd.get(), info, sizeof(buf) - 1);
      FTL_DCHECK(len > (ssize_t)sizeof(vfs_query_info_t));
      ftl::StringView fs_name(info->name, len - sizeof(vfs_query_info_t));
      if (fs_name == kMinFsName) {
        return;
      }
    }

    usleep(delay.ToMicroseconds());
    delay = delay * 2;
  }

  FTL_LOG(WARNING) << kPersistentFileSystem
                   << " is not persistent. Did you forget to configure it?";
}

}  // namespace modular
