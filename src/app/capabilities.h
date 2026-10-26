// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
namespace plexium {
// Deliberately absent from the destination registry. No cloud endpoint probing,
// official-client impersonation, placeholder playback or hidden L1/R1 route.
inline constexpr bool free_live_tv_enabled=false;
inline constexpr const char *free_live_tv_decision="omit: authorized third-party interface and real native playback unverified";
}
