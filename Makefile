# ファイル概要: CanSatカメラ・GPS処理をC++17でビルド・テストするMakefile
# 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
# 日付: YYYY-MM-DD
CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -Iinclude
CATCH2_PREFIX ?= $(shell brew --prefix catch2 2>/dev/null)
ifeq ($(CATCH2_PREFIX),)
CATCH2_CPPFLAGS :=
CATCH2_LDFLAGS := -lCatch2Main -lCatch2
else
CATCH2_CPPFLAGS := -I$(CATCH2_PREFIX)/include
CATCH2_LDFLAGS := -L$(CATCH2_PREFIX)/lib -lCatch2Main -lCatch2
endif
BUILD_DIR := build
SRC_DIR := src
TEST_DIR := tests

APP_SOURCES := $(filter-out $(SRC_DIR)/main.cpp,$(wildcard $(SRC_DIR)/*.cpp))
APP_MAIN := $(SRC_DIR)/main.cpp
TEST_SOURCES := $(TEST_DIR)/testRunner.cpp

.PHONY: all test clean

all: $(BUILD_DIR)/cansatController

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/cansatController: $(APP_SOURCES) $(APP_MAIN) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(APP_SOURCES) $(APP_MAIN) -o $@

$(BUILD_DIR)/cansatTests: $(APP_SOURCES) $(TEST_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CATCH2_CPPFLAGS) $(CXXFLAGS) $(APP_SOURCES) $(TEST_SOURCES) $(CATCH2_LDFLAGS) -o $@

test: $(BUILD_DIR)/cansatTests
	$(BUILD_DIR)/cansatTests

clean:
	rm -rf $(BUILD_DIR)
