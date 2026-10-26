CXX := g++
CC := gcc
OUT := build$(if $(filter 1,$(SANITIZE)),/san,)
OBJ := build/obj$(if $(filter 1,$(SANITIZE)),-san,)
CXXFLAGS := -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation -Isrc -Ithird_party -Ithird_party/imgui -pthread
LDFLAGS := -pthread
ifeq ($(SANITIZE),1)
 CXXFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
 LDFLAGS += -fsanitize=address,undefined
endif
CORE := $(OBJ)/src/plex/client.o $(OBJ)/third_party/tinyxml2/tinyxml2.o
APP := $(OBJ)/src/app/session.o
IMGUI := $(addprefix $(OBJ)/third_party/imgui/,$(addsuffix .o,imgui imgui_draw imgui_widgets imgui_tables))
SUITES := runtime-tests session-tests protocol-tests auth-tests home-tests home-feed-tests
.PHONY: all test clean
all: $(addprefix $(OUT)/,$(SUITES)) $(OUT)/plexium-render $(OUT)/transport-probe $(OUT)/plexium-cli
$(OBJ)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
$(OBJ)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
$(OBJ)/qrcodegen.o: third_party/qrcodegen/qrcodegen.c
	@mkdir -p $(dir $@)
	$(CC) -O1 -g -c $< -o $@
$(OUT)/runtime-tests: $(CORE) $(APP) $(OBJ)/tests/runtime_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/session-tests: $(CORE) $(APP) $(OBJ)/tests/session_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/protocol-tests: $(CORE) $(OBJ)/tests/plex_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/auth-tests: $(CORE) $(OBJ)/qrcodegen.o $(OBJ)/tests/auth_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/home-tests: $(CORE) $(OBJ)/tests/home_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/home-feed-tests: $(CORE) $(OBJ)/tests/home_feed_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/transport-probe: $(CORE) $(OBJ)/src/plex/transport_curl.o $(OBJ)/tests/transport_probe.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) $(shell pkg-config --libs libcurl) -o $@
$(OUT)/plexium-cli: $(CORE) $(APP) $(OBJ)/src/plex/transport_curl.o $(OBJ)/tools/plexium-cli.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) $(shell pkg-config --libs libcurl) -o $@
$(OUT)/plexium-render: $(CORE) $(APP) $(IMGUI) $(OBJ)/src/app/view.o $(OBJ)/tools/render-host.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OBJ)/src/plex/transport_curl.o: CXXFLAGS += $(shell pkg-config --cflags libcurl)
-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)
