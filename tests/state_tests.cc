#include "check.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "runtime/state.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace plexium;
int main(){
 auto root="/tmp/plexium-state-"+std::to_string(getpid());std::filesystem::create_directory(root);test_posix_result(chmod(root.c_str(),0700),"chmod");
 auto old=root+"/old",fresh=root+"/new";TEST_REQUIRE(plex_private_directory(old.c_str()));
 TEST_REQUIRE(plex::save_credentials(old+"/account.json","device","fixture-token"));
 TEST_REQUIRE(plex::save_credentials(old+"/device.json","device",""));
 TEST_REQUIRE(plex::save_credentials(old+"/server-42.json","42","machine-id"));
 std::ofstream(old+"/unrelated.txt")<<"not migrated";
 TEST_REQUIRE(migrate_state(old,fresh));std::string id,value;
 TEST_REQUIRE(plex::load_credentials(fresh+"/account.json",id,value)&&id=="device"&&value=="fixture-token");
 TEST_REQUIRE(plex::load_credentials(fresh+"/server-42.json",id,value)&&value=="machine-id");
 TEST_REQUIRE(!std::filesystem::exists(fresh+"/unrelated.txt"));
 test_posix_result(unlink((fresh+"/account.json").c_str()),"unlink");TEST_REQUIRE(migrate_state(old,fresh));TEST_REQUIRE(!std::filesystem::exists(fresh+"/account.json"));
 TEST_REQUIRE(std::filesystem::exists(old+"/account.json")); // rollback left intact
 auto existing=root+"/existing";TEST_REQUIRE(plex_private_directory(existing.c_str()));TEST_REQUIRE(plex::save_credentials(existing+"/account.json","new-device","new-token"));
 TEST_REQUIRE(migrate_state(old,existing));TEST_REQUIRE(plex::load_credentials(existing+"/account.json",id,value)&&value=="new-token");
 auto unsafe=root+"/unsafe";std::filesystem::create_directory(unsafe);test_posix_result(chmod(unsafe.c_str(),0755),"chmod");TEST_REQUIRE(!migrate_state(old,unsafe));
 auto linked=root+"/linked";TEST_REQUIRE(plex_private_directory(linked.c_str()));int link_result=symlink((old+"/account.json").c_str(),(linked+"/account.json").c_str());test_posix_result(link_result,"create unsafe symlink fixture");TEST_REQUIRE(!migrate_state(linked,root+"/rejected"));
 TEST_REQUIRE(!migration_name("server-../../file.json")&&!migration_name("route-.json"));
 struct stat st{};TEST_REQUIRE(lstat((existing+"/account.json").c_str(),&st)==0&&(st.st_mode&0777)==0600);
 std::filesystem::remove_all(root);std::cout<<"PASS: private state migration, stable identity/token/server selection, no overwrite, no token resurrection after logout, symlink/permission rejection\n";
}
