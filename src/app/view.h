// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "session.h"
#include "imgui.h"
#include <optional>
namespace plexium {
enum Button { Up=1,Down=2,Left=4,Right=8,Cross=16,Circle=32,L1=64,R1=128,Options=256 };
class View {
 Screen previous=Screen::Welcome;
 int row=-1,column=0,side=0;
 bool sidebar=false;
 std::string selected_profile,entry;
 std::map<std::string,std::pair<int,int>> focus;
 float playback_idle=0;
 bool playback_controls=true;
 std::string detail_focus;
 std::string listing_focus;
 int detail_panel=3;
 float detail_scroll=0;
 public:
 std::function<ImTextureID(const plex::Item&,bool)> image;
 std::function<float(ImTextureID)> aspect;
 std::function<ImTextureID(const std::string&)> avatar;
 bool controls_visible()const{return playback_controls;}
 std::optional<Command> frame(const Model&,unsigned pressed,float dt);
 bool sensitive(const Model&m)const{return m.screen==Screen::Pin;}
};
}
