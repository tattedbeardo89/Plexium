// SPDX-License-Identifier: GPL-3.0-or-later
// Small texture upload boundary. No file browser or media scanner is included.
#pragma once
#include "imgui.h"
ImTextureID library_upload_rgba(const unsigned char*,int,int);
void library_free_image(ImTextureID);
void textures_shutdown();
