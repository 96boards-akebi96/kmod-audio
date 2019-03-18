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
	unsigned int latency = 20000000;

	snd_pcm_t *pcm = NULL;
	char *filename;
	int fd = -1;
	off_t fsize;
	char *buf1 = NULL;
	char *buf2 = NULL;
	ssize_t readn, size1, size2, pos1, pos2, before_pa;
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
	size1 = fsize;
	size2 = fsize * 2;

	buf1 = malloc(size1);
	if (buf1 == NULL) {
		perror("malloc1");
		result = -1;
		goto err_out;
	}

	buf2 = malloc(size2);
	if (buf2 == NULL) {
		perror("malloc2");
		result = -1;
		goto err_out;
	}

	readn = read(fd, buf1, size1);
	if (readn != size1) {
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

	pos1 = 0;
	pos2 = 0;
	before_pa = 0;
	while (pos1 < size1) {
		buf2[pos2] = buf1[pos1];
		buf2[pos2 + 1] = buf1[pos1 + 1];
		if (buf1[pos1] == 0xf8 && buf1[pos1 + 1] == 0x72) {
			if (pos1 - before_pa < 6144) {
				//emulation?
				buf2[pos2 + 2] = 0x00;
				buf2[pos2 + 3] = 0x00;
			} else if (pos1 - before_pa > 6144) {
				printf("%d\n", pos1 - before_pa);
				buf2[pos2 + 2] = 0x00;
				buf2[pos2 + 3] = 0x00;
			} else {
				buf2[pos2 + 2] = 0x00;
				buf2[pos2 + 3] = 0x01;
				before_pa = pos1;
			}
		} else {
			buf2[pos2 + 2] = 0x00;
			buf2[pos2 + 3] = 0x00;
		}

		pos1 += 2;
		pos2 += 4;
	}

	pos2 = 0;
	while (pos2 < size2) {
		printf("\r%d/%d[bytes] playing...", (int)pos2, (int)size2);
		fflush(stdout);

		//1frame = 8bytes(32bit, stereo)
		frames = min(32768, (size2 - pos2) / 8);

		writen = snd_pcm_writei(pcm, &buf2[pos2], 
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

		//1frame = 4bytes(32bit, stereo)
		pos2 += writen * 8;
	}
	printf("\r%d/%d[bytes] finished\n", (int)pos2, (int)size2);

	result = 0;

err_out:
	if (pcm) {
		snd_pcm_close(pcm);
		pcm = NULL;
	}

	free(buf2);
	buf2 = NULL;

	free(buf1);
	buf1 = NULL;

	if (fd != -1) {
		close(fd);
		fd = -1;
	}

	return result;
}
