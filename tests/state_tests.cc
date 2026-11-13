// SPDX-License-Identifier: GPL-3.0-or-later
#include "runtime/state.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace plexium;
int main(){
 auto root="/tmp/plexium-state-"+std::to_string(getpid());std::filesystem::create_directory(root);chmod(root.c_str(),0700);
 auto old=root+"/old",fresh=root+"/new";assert(plex_private_directory(old.c_str()));
 assert(plex::save_credentials(old+"/account.json","device","fixture-token"));
 assert(plex::save_credentials(old+"/device.json","device",""));
 assert(plex::save_credentials(old+"/server-42.json","42","machine-id"));
 std::ofstream(old+"/unrelated.txt")<<"not migrated";
 assert(migrate_state(old,fresh));std::string id,value;
 assert(plex::load_credentials(fresh+"/account.json",id,value)&&id=="device"&&value=="fixture-token");
 assert(plex::load_credentials(fresh+"/server-42.json",id,value)&&value=="machine-id");
 assert(!std::filesystem::exists(fresh+"/unrelated.txt"));
 unlink((fresh+"/account.json").c_str());assert(migrate_state(old,fresh));assert(!std::filesystem::exists(fresh+"/account.json"));
 assert(std::filesystem::exists(old+"/account.json")); // rollback left intact
 auto existing=root+"/existing";assert(plex_private_directory(existing.c_str()));assert(plex::save_credentials(existing+"/account.json","new-device","new-token"));
 assert(migrate_state(old,existing));assert(plex::load_credentials(existing+"/account.json",id,value)&&value=="new-token");
 auto unsafe=root+"/unsafe";std::filesystem::create_directory(unsafe);chmod(unsafe.c_str(),0755);assert(!migrate_state(old,unsafe));
 auto linked=root+"/linked";assert(plex_private_directory(linked.c_str()));symlink((old+"/account.json").c_str(),(linked+"/account.json").c_str());assert(!migrate_state(linked,root+"/rejected"));
 assert(!migration_name("server-../../file.json")&&!migration_name("route-.json"));
 struct stat st{};assert(lstat((existing+"/account.json").c_str(),&st)==0&&(st.st_mode&0777)==0600);
 std::filesystem::remove_all(root);std::cout<<"PASS: private state migration, stable identity/token/server selection, no overwrite, no token resurrection after logout, symlink/permission rejection\n";
}
