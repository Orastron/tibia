ifeq ($(TEMPLATE), cmd)
	TINYWAV_DIR := ../../../tinywav
	MIDI_PARSER_DIR := ../../../midi-parser
endif

ifeq ($(TEMPLATE), lv2)
	CFLAGS_EXTRA := $(shell pkg-config --cflags pugl-cairo-0 pugl-0 cairo)
	LDFLAGS_EXTRA := $(shell pkg-config --libs pugl-cairo-0 pugl-0 cairo) -Wl,-rpath,$(shell pkg-config --variable=libdir pugl-cairo-0),-rpath,$(shell pkg-config --variable=libdir pugl-0),-rpath,$(shell pkg-config --variable=libdir cairo)
endif

ifeq ($(TEMPLATE), vst3)
	CFLAGS_EXTRA := -I../../../vst3_c_api $(shell pkg-config --cflags pugl-cairo-0)
	LDFLAGS_EXTRA := $(shell pkg-config --libs pugl-cairo-0 pugl-0 cairo) -Wl,-rpath,$(shell pkg-config --variable=libdir pugl-cairo-0),-rpath,$(shell pkg-config --variable=libdir pugl-0),-rpath,$(shell pkg-config --variable=libdir cairo)
endif
