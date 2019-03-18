/*
 * hw_params.c - print hardware capabilities
 *
 * compile with: gcc -o hw_params hw_params.c -lasound
 *
 * Based on:
 * http://www.mail-archive.com/alsa-user@lists.sourceforge.net/msg24794.html
 */

#include <stdio.h>
#include <alsa/asoundlib.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof *(a))

static const unsigned int rates[] = {
	5512,
	8000,
	11025,
	16000,
	22050,
	32000,
	44100,
	48000,
	64000,
	88200,
	96000,
	176400,
	192000,
};

static void print_hw_params(const char *device_name, snd_pcm_stream_t stream)
{
	snd_pcm_t *pcm;
	snd_pcm_hw_params_t *hw_params;
	unsigned int i;
	unsigned int min, max;
	int any_rate;
	int err;

	err = snd_pcm_open(&pcm, device_name, stream,
		SND_PCM_NONBLOCK);
	if (err < 0)
		return;

	snd_pcm_hw_params_alloca(&hw_params);
	err = snd_pcm_hw_params_any(pcm, hw_params);
	if (err < 0) {
		fprintf(stderr, "cannot get hardware parameters: %s\n", 
			snd_strerror(err));
		snd_pcm_close(pcm);
		return;
	}

	printf("\tDevice: %s (type: %s)\n", device_name, 
		snd_pcm_type_name(snd_pcm_type(pcm)));

	printf("\tAccess types:");
	for (i = 0; i < SND_PCM_ACCESS_LAST; ++i) {
		if (!snd_pcm_hw_params_test_access(pcm, hw_params, i))
			printf(" %s", snd_pcm_access_name(i));
	}
	putchar('\n');

	printf("\tFormats:");
	for (i = 0; i < SND_PCM_FORMAT_LAST; ++i) {
		if (!snd_pcm_hw_params_test_format(pcm, hw_params, i))
			printf(" %s", snd_pcm_format_name(i));
	}
	putchar('\n');

	err = snd_pcm_hw_params_get_channels_min(hw_params, &min);
	if (err < 0) {
		fprintf(stderr, "cannot get minimum channels count: %s\n", 
			snd_strerror(err));
		snd_pcm_close(pcm);
		return;
	}
	err = snd_pcm_hw_params_get_channels_max(hw_params, &max);
	if (err < 0) {
		fprintf(stderr, "cannot get maximum channels count: %s\n", 
			snd_strerror(err));
		snd_pcm_close(pcm);
		return;
	}
	printf("\tChannels:");
	for (i = min; i <= max; ++i) {
		if (!snd_pcm_hw_params_test_channels(pcm, hw_params, i))
			printf(" %u", i);
	}
	putchar('\n');

	err = snd_pcm_hw_params_get_rate_min(hw_params, &min, NULL);
	if (err < 0) {
		fprintf(stderr, "cannot get minimum rate: %s\n", 
			snd_strerror(err));
		snd_pcm_close(pcm);
		return;
	}
	err = snd_pcm_hw_params_get_rate_max(hw_params, &max, NULL);
	if (err < 0) {
		fprintf(stderr, "cannot get maximum rate: %s\n", 
			snd_strerror(err));
		snd_pcm_close(pcm);
		return;
	}
	printf("\tSample rates:");
	if (min == max)
		printf(" %u", min);
	else if (!snd_pcm_hw_params_test_rate(pcm, hw_params, min + 1, 0))
		printf(" %u-%u", min, max);
	else {
		any_rate = 0;
		for (i = 0; i < ARRAY_SIZE(rates); ++i) {
			if (!snd_pcm_hw_params_test_rate(pcm, hw_params,
							 rates[i], 0)) {
				any_rate = 1;
				printf(" %u", rates[i]);
			}
		}
		if (!any_rate)
			printf(" %u-%u", min, max);
	}
	putchar('\n');

	err = snd_pcm_hw_params_get_period_time_min(hw_params, &min, NULL);
	if (err < 0) {
		fprintf(stderr, "cannot get minimum period time: %s\n", 
			snd_strerror(err));
		snd_pcm_close(pcm);
		return;
	}
	err = snd_pcm_hw_params_get_period_time_max(hw_params, &max, NULL);
	if (err < 0) {
		fprintf(stderr, "cannot get maximum period time: %s\n", 
			snd_strerror(err));
		snd_pcm_close(pcm);
		return;
	}
	printf("\tInterrupt interval: %u-%u us\n", min, max);

	err = snd_pcm_hw_params_get_buffer_time_min(hw_params, &min, NULL);
	if (err < 0) {
		fprintf(stderr, "cannot get minimum buffer time: %s\n", 
			snd_strerror(err));
		snd_pcm_close(pcm);
		return;
	}
	err = snd_pcm_hw_params_get_buffer_time_max(hw_params, &max, NULL);
	if (err < 0) {
		fprintf(stderr, "cannot get maximum buffer time: %s\n", 
			snd_strerror(err));
		snd_pcm_close(pcm);
		return;
	}
	printf("\tBuffer time: %u-%u us\n", min, max);
	putchar('\n');

	snd_pcm_close(pcm);
}

static void hw_params(snd_pcm_stream_t stream)
{
	snd_ctl_t *handle;
	int card, err;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);
	int sub;

	card = -1;
	if (snd_card_next(&card) < 0 || card < 0) {
		fprintf(stderr, "no soundcards found...");
		return;
	}
	printf("%s\n", snd_pcm_stream_name(stream));
	do {
		char name[32];
		sprintf(name, "hw:%d", card);

		err = snd_ctl_open(&handle, name, 0);
		if (err < 0) {
			fprintf(stderr, "control open (%i): %s", card, snd_strerror(err));
			goto next_card;
		}

		err = snd_ctl_card_info(handle, info);
		if (err < 0) {
			fprintf(stderr, "control hardware info (%i): %s", card, snd_strerror(err));
			snd_ctl_close(handle);
			goto next_card;
		}

		for (sub = 0; sub < 16; sub++) {
			sprintf(name, "hw:%d,%d", card, sub);
			print_hw_params(name, stream);
		}

		snd_ctl_close(handle);
	next_card:
		if (snd_card_next(&card) < 0) {
			fprintf(stderr, "snd_card_next");
			break;
		}
	} while (card >= 0);
}

int main(int argc, char *argv[])
{
	int i;

	for (i = 0; i <= SND_PCM_STREAM_LAST; i++)
		hw_params(i);

	return 0;
}
