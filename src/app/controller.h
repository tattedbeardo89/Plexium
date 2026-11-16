// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
namespace plexium {
enum Button { Up=1,Down=2,Left=4,Right=8,Cross=16,Circle=32,L1=64,R1=128,Options=256,Triangle=512,Square=1024 };
// Modifiers are committed alone on release. A chord consumes their solo action,
// so holding Square to reorder cannot first remove a queue entry.
class ControllerEvents {
 unsigned previous=0,consumed=0;int screen=-1;
 public:
 unsigned update(unsigned held,int current_screen,bool blocked=false){
  constexpr unsigned modifiers=Square|Triangle|Options|L1|R1;
  auto down=held&~previous,up=previous&~held;
  if(blocked||screen!=current_screen){consumed=held&modifiers;previous=held;screen=current_screen;return down&Circle;}
  auto event=down&~modifiers;auto held_modifiers=held&modifiers;
  if((event&&held_modifiers)||((down&modifiers)&&(held_modifiers&(held_modifiers-1)))){event|=held_modifiers;consumed|=held_modifiers;}
  event|=up&modifiers&~consumed;consumed&=held;previous=held;return event;
 }
};
}
