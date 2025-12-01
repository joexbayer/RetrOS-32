include ../../make/common.mk

TARGET := http
CXXFLAGS := -std=c++11 -I../../apps/utils -I../../include

.PHONY: all run clean
all: run

run: $(TARGET)
	./$(TARGET)

$(TARGET): http.cpp
	$(QUIET)$(CXX) $(CXXFLAGS) $< -o $@

clean:
	$(QUIET)$(RM_F) $(TARGET)
