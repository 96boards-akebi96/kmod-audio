#include <stdlib.h>
#include <stdint.h>

#include <alsa/asoundlib.h>

#define min(a, b)    ( (a < b) ? (a) : (b) )

void usage(int argc, char *argv[])
{
	fprintf(stderr, "usage: %s [L | R | LR] [device]\n", 
		argv[0]);
}

int main(int argc, char *argv[])
{
	const char *chan = "LR";
	const char *device = "default";
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_BE;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	unsigned int sampling_rate = 48000;
	unsigned int channels = 2;
	int soft_resample = 1;
	unsigned int latency = 50000;

	snd_pcm_t *pcm = NULL;
	off_t fsize;
	char *buf = NULL;
	ssize_t pos;
	snd_pcm_sframes_t writen, frames;
	size_t i, n;
	int ch_left, ch_right;
	uint8_t lpcm_val;
	int result = -1;

	if (argc < 1) {
		usage(argc, argv);
		return 1;
	}

	if (argc >= 2) {
		chan = argv[1];
	}

	if (argc >= 3) {
		device = argv[2];
	}

	ch_left = 0;
	ch_right = 0;
	if (strncasecmp(chan, "LR", 2) == 0) {
		printf("Channel: Left and Right.\n");
		ch_left = 1;
		ch_right = 1;
	} else if (strncasecmp(chan, "L", 1) == 0) {
		printf("Channel: Left only.\n");
		ch_left = 1;
	} else if (strncasecmp(chan, "R", 1) == 0) {
		printf("Channel: Right only.\n");
		ch_right = 1;
	} else {
		//unknown command
		fprintf(stderr, "unknown channel '%s'.\n", chan);
		result = -EINVAL;
		goto err_out;
	}

	fsize = 2 * 1024 * 1024;

	buf = malloc(fsize);
	if (buf == NULL) {
		perror("malloc");
		result = -1;
		goto err_out;
	}

	//generate the square wave
	for (i = 0, n = 0; i < fsize; i += 4) {
		uint8_t lh, ll, rh, rl;

		lh = ll = rh = rl = 0x00;

		//48000[sample/s] / 256[sample] = 187.5Hz
		if (n % 256 < 128) {
			lpcm_val = -120;
		} else {
			lpcm_val = 120;
		}
		n += 1;

		if (ch_left) {
			lh = lpcm_val;
		}
		if (ch_right) {
			rh = lpcm_val;
		}

		buf[i + 0] = lh;
		buf[i + 1] = ll;
		buf[i + 2] = rh;
		buf[i + 3] = rl;
	}

	//open the playback ALSA device
	result = snd_pcm_open(&pcm, device, SND_PCM_STREAM_PLAYBACK, 0);
	if (result < 0) {
		perror("snd_pcm_open()");
		goto err_out;
	}

	result = snd_pcm_set_params(pcm, format, access, channels, 
		sampling_rate, soft_resample, latency);
	if (result < 0) {
		perror("snd_pcm_set_params()");
		goto err_out;
	}

	pos = 0;
	while (pos < fsize) {
		printf("press return to beep...\n");
		fflush(stdout);
		getchar();

		//1frame = 4bytes(16bit, stereo)
		//4096[frames] / 48000 = 85.3[ms]
		frames = min(4096, (fsize - pos) / 4);

		writen = snd_pcm_writei(pcm, &buf[pos], 
			frames);
		if (writen < 0) {
			perror("snd_pcm_writei()");

			//try to recover from under-run
			result = snd_pcm_recover(pcm, writen, 0);
			if (result < 0) {
				perror("snd_pcm_recover()");
				goto err_out;
			}
			writen = 0;
		}

		//1frame = 4bytes(16bit, stereo)
		pos += writen * 4;
	}

	result = 0;

err_out:
	if (pcm) {
		snd_pcm_close(pcm);
		pcm = NULL;
	}

	free(buf);
	buf = NULL;

	usage(argc, argv);

	return result;
}
