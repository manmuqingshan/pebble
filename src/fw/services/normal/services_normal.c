/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "services_normal.h"

#include <string.h>

#include "process_management/app_install_manager.h" // FIXME: This should really be in services/
#include "process_management/launcher_app_message.h" // FIXME: This should really be in services/
#include "services/normal/activity/activity.h"
#include "services/normal/alarms/alarm.h"
#include "services/normal/app_cache.h"
#include "services/normal/app_fetch_endpoint.h"
#include "services/normal/app_glances/app_glance_service.h"
#include "services/normal/blob_db/api.h"
#include "services/normal/blob_db/endpoint_private.h"
#include "services/normal/data_logging/data_logging_service.h"
#include "services/normal/filesystem/pfs.h"
#include "services/normal/protobuf_log/protobuf_log.h"
#include "services/normal/music_endpoint.h"
#include "services/normal/music_internal.h"
#include "services/normal/notifications/alerts_private.h"
#include "services/normal/notifications/notifications.h"
#include "services/normal/persist.h"
#include "services/normal/phone_call.h"
#include "services/normal/process_management/app_order_storage.h"
#include "services/normal/send_text_service.h"
#include "services/normal/stationary.h"
#include "services/normal/timeline/event.h"
#include "services/normal/wakeup.h"
#include "services/normal/weather/weather_service.h"
#include "services/runlevel_impl.h"

#ifndef PLATFORM_TINTIN
#include "services/normal/activity/activity.h"
#include "services/normal/voice/voice.h"
#endif

#include "util/size.h"

void services_normal_early_init(void) {
  pfs_init(true);
}

void services_normal_init(void) {
  persist_service_init();

  app_install_manager_init();

  blob_db_init_dbs();
  app_cache_init();
  phone_call_service_init();
  music_init();
  alarm_init();
  timeline_event_init();
  dls_init();

  wakeup_init();

  app_order_storage_init();

#if CAPABILITY_HAS_HEALTH_TRACKING
  activity_init();
#endif

  notifications_init();
  alerts_init();
  send_text_service_init();
  protobuf_log_init();

#if CAPABILITY_HAS_WEATHER
  weather_service_init();
#endif

#if CAPABILITY_HAS_MICROPHONE
  voice_init();
#endif

#if CAPABILITY_HAS_APP_GLANCES
  app_glance_service_init();
#endif
}

static struct ServiceRunLevelSetting s_runlevel_settings[] = {
  {
    .set_enable_fn = wakeup_enable,
    .enable_mask = R_Stationary | R_Normal,
  },
  {
    .set_enable_fn = alarm_service_enable_alarms,
    .enable_mask = R_LowPower | R_Stationary | R_Normal,
  },
#if CAPABILITY_HAS_HEALTH_TRACKING
  {
    .set_enable_fn = activity_set_enabled,
    .enable_mask = R_Stationary | R_Normal,
  },
#endif
  {
    .set_enable_fn = stationary_run_level_enable,
    .enable_mask = R_Stationary | R_Normal,
  },
  {
    .set_enable_fn = dls_set_send_enable_run_level,
    .enable_mask = R_Normal,
  },
  {
    .set_enable_fn = blob_db_enabled,
    .enable_mask = R_Normal,
  }
};

void services_normal_set_runlevel(RunLevel runlevel) {
  for (size_t i = 0; i < ARRAY_LENGTH(s_runlevel_settings); ++i) {
    struct ServiceRunLevelSetting *service = &s_runlevel_settings[i];
    service->set_enable_fn(((1 << runlevel) & service->enable_mask) != 0);
  }
}
