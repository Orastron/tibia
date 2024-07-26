/*
 * Tibia
 *
 * Copyright (C) 2024 Orastron Srl unipersonale
 *
 * Tibia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Tibia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tibia.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File author: Stefano D'Angelo
 */

#include <stdlib.h>
#include <stdint.h>

#include "callbacks.h"

#include "data.h"
#include "plugin.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#if NUM_CHANNELS_IN + NUM_CHANNELS_OUT > 0
# include <tinywav.h>
#endif
#if NUM_MIDI_INPUTS > 0
# include <midi-parser.h>
#endif

#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <pmmintrin.h>
#endif

plugin			instance;
void *			mem;
#if NUM_NON_OPT_CHANNELS_IN > NUM_CHANNELS_IN
float *			zero;
#endif
#if NUM_CHANNELS_IN > 0
float *			x_buf;
float *			x_in[NUM_CHANNELS_IN];
#endif
#if NUM_ALL_CHANNELS_IN > 0
const float *		x[NUM_ALL_CHANNELS_IN];
#else
const float **		x;
#endif
#if NUM_NON_OPT_CHANNELS_OUT > 0
float *			y_buf;
#endif
#if NUM_CHANNELS_OUT > 0
float *			y_out[NUM_CHANNELS_OUT];
#endif
#if NUM_ALL_CHANNELS_OUT > 0
float *			y[NUM_ALL_CHANNELS_OUT];
#else
float **		y;
#endif
float			fs = 44100.f;
size_t			bufsize = 128;
#if NUM_CHANNELS_IN == 0
float			length = 1.f;
#endif
#if PARAMETERS_N > 0
float			param_values[PARAMETERS_N];
#endif
#if NUM_CHANNELS_IN > 0
const char *		infile = NULL;
#endif
#if NUM_CHANNELS_OUT > 0
const char *		outfile = NULL;
#endif
#if NUM_MIDI_INPUTS > 0
const char *		midifile = NULL;
void *			midi_data = NULL;
struct midi_parser	midi_parser;
int16_t			midi_ticks = 0;
uint32_t		midi_tempo = 500000; // microseconds per quarter-note -> 120 bpm
enum midi_parser_status	midi_status;
double			midi_next;
char			midi_next_read;
#endif

void usage(const char * argv0) {
#if NUM_CHANNELS_IN > 0
	fprintf(stderr, "Usage: %s [bufsize=value] infile", argv0);
#else
	fprintf(stderr, "Usage: %s [fs=value] [bufsize=value] [length=value]", argv0);
#endif
#if NUM_CHANNELS_OUT > 0
	fprintf(stderr, " outfile");
#endif
#if NUM_MIDI_INPUTS > 0
	fprintf(stderr, " [midifile]");
#endif
#if PARAMETERS_N > 0
	fprintf(stderr, " [param=value] ...");
#endif
	fprintf(stderr, "\n");

#if NUM_CHANNELS_IN > 0
	fprintf(stderr, " defaults: bufsize=128");
#else
	fprintf(stderr, " defaults: fs=44100, bufsize=128");
#endif
#if PARAMETERS_N > 0
	for (size_t i = 0; i < PARAMETERS_N; i++)
		if (!param_data[i].out)
			fprintf(stderr, ", %s=%g", param_data[i].id, param_data[i].def);
#endif
	fprintf(stderr, "\n");
}

#if NUM_MIDI_INPUTS > 0
char * read_file(const char * filename, int32_t * size) {
	FILE * fp = fopen(filename, "r");
	if (fp == NULL)
		return NULL;
	if (fseek(fp, 0, SEEK_END) != 0) {
		fclose(fp);
		return NULL;
	}
	*size = ftell(fp);
	if (*size < 0) {
		fclose(fp);
		return NULL;
	}
	void * mem = malloc(*size);
	if (mem == NULL) {
		fclose(fp);
		return NULL;
	}
	if (fseek(fp, 0L, SEEK_SET) != 0) {
		free(mem);
		fclose(fp);
		return NULL;
	}
	size_t n = fread(mem, 1, *size, fp);
	if (n != (uint32_t)(*size)) {
		free(mem);
		fclose(fp);
		return NULL;
	}
	fclose(fp);
	return mem;
}
#endif

float clampf(float x, float m, float M) {
	return x < m ? m : (x > M ? M : x);
}

int main(int argc, char * argv[]) {
#if PARAMETERS_N > 0
	for (size_t i = 0; i < PARAMETERS_N; i++)
		param_values[i] = param_data[i].def;
#endif

	char parsingState = 0; // 0 = fs/bufsize/length, 1 = filenames, 2 = params
	for (int i = 1; i < argc; i++) {
		switch (parsingState) {
		case 0:
		{
			char * c = strchr(argv[i], '=');
			if (c == NULL) {
				parsingState = 1;
				i--;
				continue;
			}
			if (strncmp(argv[i], "bufsize", 7) == 0) {
				char * e;
				ssize_t v = strtol(c + 1, &e, 10);
				if (errno || v <= 0 || *e != '\0') {
					fprintf(stderr, "invalid format of argument '%s'\n", argv[i]);
					usage(argv[0]);
					return EXIT_FAILURE;
				}
				bufsize = v;
#if NUM_CHANNELS_IN == 0
			} else if (strncmp(argv[i], "fs", 2) == 0) {
				char * e;
				float v = strtof(c + 1, &e);
				if (errno || !isfinite(v) || v <= 0.f || *e != '\0') {
					fprintf(stderr, "invalid format of argument '%s'\n", argv[i]);
					usage(argv[0]);
					return EXIT_FAILURE;
				}
				fs = v;
			} else if (strncmp(argv[i], "length", 6) == 0) {
				char * e;
				float v = strtof(c + 1, &e);
				if (errno || !isfinite(v) || v <= 0.f || *e != '\0') {
					fprintf(stderr, "invalid format of argument '%s'\n", argv[i]);
					usage(argv[0]);
					return EXIT_FAILURE;
				}
				length = v;
#endif
			} else {
				fprintf(stderr, "invalid format of argument '%s'\n", argv[i]);
				usage(argv[0]);
				return EXIT_FAILURE;
			}
		}
			break;
		case 1:
		{
			char * c = strchr(argv[i], '=');
			if (c != NULL) {
#if PARAMETERS_N == 0
				fprintf(stderr, "invalid format of argument '%s'\n", argv[i]);
				usage(argv[0]);
				return EXIT_FAILURE;
#endif
				parsingState = 2;
				i--;
				continue;
			}
			const char ** next = NULL;
#if NUM_CHANNELS_IN > 0
			if (infile == NULL)
				next = &infile;
#endif
#if NUM_CHANNELS_OUT > 0
			if (next == NULL && outfile == NULL)
				next = &outfile;
#endif
#if NUM_MIDI_INPUTS > 0
			if (next == NULL && midifile == NULL)
				next = &midifile;
#endif
			if (next == NULL) {
				fprintf(stderr, "invalid argument '%s' (in/out files already specified)\n", argv[i]);
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			*next = argv[i];
		}
			break;
#if PARAMETERS_N > 0
		case 2:
		{
			char * c = strchr(argv[i], '=');
			if (c == NULL) {
				fprintf(stderr, "invalid format of argument '%s'\n", argv[i]);
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			char * e;
			float v = strtof(c + 1, &e);
			if (errno || !isfinite(v) || *e != '\0') {
				fprintf(stderr, "invalid format of argument '%s'\n", argv[i]);
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			int len = c - argv[i];
			int j = 0;
			for (; j < PARAMETERS_N; j++) {
				if (strncmp(argv[i], param_data[j].id, len) == 0 && param_data[j].id[len] == '\0')
					break;
			}
			if (j == PARAMETERS_N) {
				fprintf(stderr, "parameter for '%s' not found\n", argv[i]);
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			param_values[j] = v;

		}
			break;
#endif
		}
	}
#if NUM_CHANNELS_IN > 0
	if (infile == NULL) {
		fprintf(stderr, "infile not specified\n");
		usage(argv[0]);
		return EXIT_FAILURE;
	}
#endif
#if NUM_CHANNELS_OUT > 0
	if (outfile == NULL) {
		fprintf(stderr, "outfile not specified\n");
		usage(argv[0]);
		return EXIT_FAILURE;
	}
#endif

#if defined(__aarch64__)
	uint64_t fpcr;
	__asm__ __volatile__ ("mrs %0, fpcr" : "=r"(fpcr));
	__asm__ __volatile__ ("msr fpcr, %0" :: "r"(fpcr | 0x1000000)); // enable FZ
#elif defined(__i386__) || defined(__x86_64__)
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

#if PARAMETERS_N > 0
	for (size_t i = 0; i < PARAMETERS_N; i++) {
		if (param_data[i].out)
			continue;
		float v = param_values[i];
		if (param_data[i].flags & (PARAM_BYPASS | PARAM_TOGGLED))
			v = v > 0.5f ? 1.f : 0.f;
		else if (param_data[i].flags & PARAM_INTEGER)
			v = (int32_t)(v + (v >= 0.f ? 0.5f : -0.5f));

		param_values[i] = clampf(v, param_data[i].min, param_data[i].max);
	}
#endif

	int exit_code = EXIT_FAILURE;

#if NUM_CHANNELS_IN > 0
	TinyWav tw_in;
	if (tinywav_open_read(&tw_in, infile, TW_SPLIT) != 0)
		return EXIT_FAILURE;
	if (tw_in.h.NumChannels != NUM_CHANNELS_IN) {
		fprintf(stderr, "input file has %d channels but %d channels were expected\n", tw_in.h.NumChannels, NUM_CHANNELS_IN);
		goto err_num_channels_in;
	}
	fs = tw_in.h.SampleRate;
#endif

	printf(" fs: %g\n", fs);
	printf(" bufsize: %zu\n", bufsize);
#if NUM_CHANNELS_IN > 0
	printf(" length: %g\n", (double)tw_in.numFramesInHeader / (double)fs);
	printf(" infile: %s\n", infile);
#else
	printf(" length: %g\n", length);
#endif
#if NUM_CHANNELS_OUT > 0
	printf(" outfile: %s\n", outfile);
#endif
#if NUM_MIDI_INPUTS > 0
	printf(" midifile: %s\n", midifile ? midifile : "[none]");
#endif
#if PARAMETERS_N > 0
	for (size_t i = 0; i < PARAMETERS_N; i++)
		if (!param_data[i].out)
			printf(" %s: %g\n", param_data[i].id, param_values[i]);
#endif

	plugin_callbacks cbs = {
		/* .handle		= */ NULL,
		/* .format		= */ "cmd",
		/* .get_bindir		= */ NULL,
		/* .get_datadir		= */ NULL
	};
	plugin_init(&instance, &cbs);

#if PARAMETERS_N > 0
	for (size_t i = 0; i < PARAMETERS_N; i++)
		if (!param_data[i].out)
			plugin_set_parameter(&instance, i, param_values[i]);
#endif

	plugin_set_sample_rate(&instance, fs);
	size_t req = plugin_mem_req(&instance);
	if (req != 0) {
		mem = malloc(req);
		if (mem == NULL) {
			fprintf(stderr, "Out of memory\n");
			goto err_mem_alloc;
		}
		plugin_mem_set(&instance, mem);
	} else
	       mem = NULL;	

	plugin_reset(&instance);

#if NUM_NON_OPT_CHANNELS_IN > NUM_CHANNELS_IN
	zero = malloc(bufsize * sizeof(float));
	if (zero == NULL) {
		fprintf(stderr, "Out of memory\n");
		goto err_zero;
	}
	memset(zero, 0, bufsize * sizeof(float));
#endif

#if NUM_CHANNELS_IN > 0
	x_buf = malloc(NUM_CHANNELS_IN * bufsize * sizeof(float));
	if (x_buf == NULL) {
		fprintf(stderr, "Out of memory\n");
		goto err_x_buf;
	}
#endif
#if NUM_ALL_CHANNELS_IN > 0
	for (size_t i = 0, j = 0, k = 0; i < NUM_AUDIO_BUSES_IN + NUM_AUDIO_BUSES_OUT; i++) {
		if (audio_bus_data[i].out)
			continue;
		for (int l = 0; l < audio_bus_data[i].channels; l++, j++) {
			if (AUDIO_BUS_IN == audio_bus_data[i].index) {
				float * b = x_buf + bufsize * k;
				x[j] = b;
				x_in[l] = b;
				k++;
			} else
#if NUM_NON_OPT_CHANNELS_IN > NUM_CHANNELS_IN
				x[j] = audio_bus_data[i].optional ? NULL : zero;
#else
				x[j] = NULL;
#endif
		}
	}
#else
	x = NULL;
#endif

#if NUM_NON_OPT_CHANNELS_OUT > 0
	y_buf = malloc(NUM_NON_OPT_CHANNELS_OUT * bufsize * sizeof(float));
	if (y_buf == NULL) {
		fprintf(stderr, "Out of memory\n");
		goto err_y_buf;
	}
#endif
#if NUM_ALL_CHANNELS_OUT > 0
	for (size_t i = 0, j = 0, k = 0; i < NUM_AUDIO_BUSES_IN + NUM_AUDIO_BUSES_OUT; i++) {
		if (!audio_bus_data[i].out)
			continue;
		for (int l = 0; l < audio_bus_data[i].channels; l++, j++) {
			if (AUDIO_BUS_OUT == audio_bus_data[i].index) {
				y[j] = y_buf + bufsize * k;
				y_out[l] = y[j];
				k++;
			} else if (!audio_bus_data[i].optional) {
				y[j] = y_buf + bufsize * k;
				k++;
			} else
				y[j] = NULL; 
		}
	}
#else
	y = NULL;
#endif

#if NUM_MIDI_INPUTS > 0
	if (midifile != NULL) {
		int32_t midi_data_size;
		midi_data = read_file(midifile, &midi_data_size);
		if (midi_data == NULL)
			goto err_midi_read;

		midi_parser.state = MIDI_PARSER_INIT;
		midi_parser.size = midi_data_size;
		midi_parser.in = midi_data;

		midi_status = midi_parse(&midi_parser);
		if (midi_status != MIDI_PARSER_HEADER) {
			fprintf(stderr, "Header not found in MIDI file\n");
			goto err_midi_parse;
		}
		if (midi_parser.header.format != 0) {
			fprintf(stderr, "Only MIDI file format 0 is supported\n");
			goto err_midi_parse;
		}
		if ((midi_parser.header.time_division & 0x80) != 0x80) {
			fprintf(stderr, "Only ticks per quarter-note time division is supported when reading MIDI files\n");
			goto err_midi_parse;
		}
		if (midi_parser.header.time_division == 0) {
			fprintf(stderr, "Invalid 0 tick per quarter-note in MIDI file\n");
			goto err_midi_parse;
		}

		midi_ticks = midi_parser.header.time_division;
		midi_next = 0.0;
		midi_next_read = 1;
	} else {
		midi_status = MIDI_PARSER_EOB;
		midi_next = 0.0;
		midi_next_read = 0;
	}
#endif

#if NUM_CHANNELS_OUT > 0
	TinyWav tw_out;
	if (tinywav_open_write(&tw_out, NUM_CHANNELS_OUT, fs, TW_FLOAT32, TW_SPLIT, outfile) != 0)
		goto err_outfile;
#endif

	size_t i = 0;
#if NUM_CHANNELS_IN > 0
	size_t len = tw_in.numFramesInHeader;
#else
	size_t len = (size_t)(fs * length + 0.5f);
#endif
	while (i < len) {
		size_t left = len - i;
		size_t n = left > bufsize ? bufsize : left;
#if NUM_CHANNELS_IN > 0
		n = tinywav_read_f(&tw_in, x_in, n);
		if (n == 0)
			break;
#endif
#if NUM_MIDI_INPUTS > 0
		while (1) {
			if (midi_next > 0.0)
				break;
			else if (midi_next_read == 0) {
				if (midi_status == MIDI_PARSER_TRACK_META && midi_parser.meta.type == MIDI_META_SET_TEMPO)
					midi_tempo = (midi_parser.meta.bytes[0] << 16) | (midi_parser.meta.bytes[1] << 8) | midi_parser.meta.bytes[2];
				else if (midi_status == MIDI_PARSER_TRACK_MIDI) {
					uint8_t data[3] = { (midi_parser.midi.status << 4) | midi_parser.midi.channel, midi_parser.midi.param1, midi_parser.midi.param2 };
					plugin_midi_msg_in(&instance, MIDI_BUS_IN, data);
				}
				midi_next_read = 1;
			}

			if (midi_status == MIDI_PARSER_EOB)
				break;

			midi_status = midi_parse(&midi_parser);
			switch (midi_status) {
			case MIDI_PARSER_ERROR:
			case MIDI_PARSER_HEADER:
				fprintf(stderr, "Error while parsing MIDI file\n");
				goto err_midi_parse;
				break;
			case MIDI_PARSER_TRACK_META:
			case MIDI_PARSER_TRACK_MIDI:
			case MIDI_PARSER_TRACK_SYSEX:
				midi_next += ((double)midi_tempo / (double)midi_ticks) * midi_parser.vtime;
				midi_next_read = 0;
				break;
			default:
				break;
			}
		}
		midi_next -= 1e6 * ((double)n / (double)fs);
#endif

		plugin_process(&instance, x, y, n);

#if PARAMETERS_N > 0
		for (size_t j = 0; j < PARAMETERS_N; j++) {
			if (!param_data[j].out)
				continue;
			param_values[j] = plugin_get_parameter(&instance, j);
			printf("  %s: %g\n", param_data[j].id, param_values[j]);
		}
#endif

#if NUM_CHANNELS_OUT > 0
		tinywav_write_f(&tw_out, y_out, n);
#endif

		i += n;
	}

	exit_code = EXIT_SUCCESS;

#if NUM_CHANNELS_OUT > 0
	tinywav_close_write(&tw_out);
#endif
err_outfile:
#if NUM_MIDI_INPUTS > 0
err_midi_parse:
	if (midi_data != NULL)
		free(midi_data);
err_midi_read:
#endif
#if NUM_CHANNELS_OUT > 0
	free(y_buf);
err_y_buf:
#endif
#if NUM_CHANNELS_IN > 0
	free(x_buf);
err_x_buf:
#endif
#if NUM_NON_OPT_CHANNELS_IN > NUM_CHANNELS_IN
	free(zero);
err_zero:
#endif
	if (mem != NULL)
		free(mem);
err_mem_alloc:
	plugin_fini(&instance);
#if NUM_CHANNELS_IN > 0
err_num_channels_in:
	tinywav_close_read(&tw_in);
#endif

	return exit_code;
}
