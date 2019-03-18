#include <stdlib.h>
#include <stdint.h>

#include <alsa/asoundlib.h>

#define min(a, b)    ( (a < b) ? (a) : (b) )

int main(int argc, char *argv[])
{
	const char *device = "default";
	snd_pcm_format_t format = SND_PCM_FORMAT_S32_LE;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	unsigned int sampling_rate = 48000;
	unsigned int channels = 2;
	int soft_resample = 1;
	unsigned int latency = 50000;

	snd_pcm_t *pcm = NULL;
	char *filename;
	int fd = -1;
	off_t fsize;
	char *buf = NULL;
	ssize_t readn, pos;
	snd_pcm_sframes_t writen, frames;
	int result = -1;

	if (argc < 2) {
		fprintf(stderr, "usage: \n%s filename [device]\n", 
			argv[0]);
		return 1;
	}

	filename = argv[1];
	if (argc >= 3) {
		device = argv[2];
	}

	//read the whole LPCM file
	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("open");
		result = -1;
		goto err_out;
	}
	fsize = lseek(fd, 0, SEEK_END);
	if (fsize < 0) {
		perror("lseek");
		result = -1;
		goto err_out;
	}
	lseek(fd, 0, SEEK_SET);

	buf = malloc(fsize);
	if (buf == NULL) {
		perror("malloc");
		result = -1;
		goto err_out;
	}

	readn = read(fd, buf, fsize);
	if (readn != fsize) {
		perror("read");
		result = -1;
		goto err_out;
	}

	//open the playback ALSA device
	result = snd_pcm_open(&pcm, device, SND_PCM_STREAM_PLAYBACK, 0);
	if (result < 0) {
		perror("snd_pcm_open()");
		goto err_out;
	}

	result = snd_pcm_set_params(pcm, format, access,
		channels, sampling_rate, soft_resample,
		latency);
	if (result < 0) {
		perror("snd_pcm_set_params()");
		goto err_out;
	}

	pos = 0;
	while (pos < fsize) {
		printf("\r%d/%d[bytes] playing...", (int)pos, (int)fsize);
		fflush(stdout);

		//1frame = 4bytes(16bit, stereo)
		frames = min(32768, (fsize - pos) / 4);

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
	printf("\r%d/%d[bytes] finished\n", (int)pos, (int)fsize);

	result = 0;

err_out:
	if (pcm) {
		snd_pcm_close(pcm);
		pcm = NULL;
	}

	free(buf);
	buf = NULL;

	if (fd != -1) {
		close(fd);
		fd = -1;
	}

	return result;
}
