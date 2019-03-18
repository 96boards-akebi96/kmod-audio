#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <pthread.h>

#include <alsa/asoundlib.h>

#define max(a, b)    ( ((a) > (b)) ? (a) : (b) )
#define min(a, b)    ( ((a) < (b)) ? (a) : (b) )

enum STATE {
	IDLE  = 0, 
	START = 1, 
	RUN   = 2, 
	STOP  = 3, 
};

struct time_hms {
	int hour;
	int minute;
	int sec;
	int usec;
};

struct playback_info {
	snd_pcm_t *pcm;
	unsigned int sampling_rate;
	unsigned int latency;
	size_t frame_size;

	pthread_mutex_t th_mutex;
	pthread_cond_t th_cond;

	off_t filesize;
	char *buf;
	size_t pos;

	enum STATE state;
	int should_stop;
	int f_loop;
};

struct wav_info {
	unsigned int rate;
	unsigned int bits;
	unsigned int ch;
	off_t start;
	size_t size;
};

struct riff_chunk_info_rec {
	uint32_t id;
	uint32_t size;
	uint32_t type;
};

struct riff_chunk_info {
	uint32_t id;
	uint32_t size;
};

struct fmt_chunk_info {
	struct riff_chunk_info h;
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;
	uint16_t cbSize;
};

void *playback_main(void *arg);

void usage(void)
{
	printf("Usage: command number\n"
		"Commands: \n"
		"  h(or ?): Show this help.\n"
		"  i      : Show information of the WAV file.\n"
		"  s      : Start playing from current position.\n"
		"  t      : Stop playing.\n"
		"  p      : Print current position(in [sec]).\n"
		"  k num  : Seek to 'num[sec]'.\n"
		"  l      : Toggle the mode of loop play.\n"
		"  q      : Quit.\n");
}

int analyze_wav(const char *buf, size_t size, struct wav_info *wavinf)
{
	struct riff_chunk_info_rec riff_chk;
	struct fmt_chunk_info fmt_chk;
	struct riff_chunk_info chk_h;
	int found_fmt, found_data;
	size_t pos;

	if (!buf || !wavinf) {
		fprintf(stderr, "Illegal arguments.\n");
		return -1;
	}

	pos = 0;
	wavinf->rate = 0;
	wavinf->bits = 0;
	wavinf->ch = 0;
	wavinf->start = 0;
	wavinf->size = 0;
	found_fmt = 0;
	found_data = 0;

	//read 'RIFF' chunk
	memmove(&riff_chk, &buf[pos], sizeof(riff_chk));
	pos += sizeof(riff_chk);
	if (riff_chk.id != 0x46464952) {
		fprintf(stderr, "Not found 'RIFF' chunk.\n");
		return -1;
	}

	while (pos < size - sizeof(struct riff_chunk_info)) {
		//read ahead chunk header
		memmove(&chk_h, &buf[pos], sizeof(chk_h));

		if (chk_h.id == 0x20746d66) {
			//found 'fmt ' chunk
			memmove(&fmt_chk, &buf[pos], sizeof(fmt_chk));
			wavinf->rate = fmt_chk.nSamplesPerSec;
			wavinf->bits = fmt_chk.wBitsPerSample;
			wavinf->ch = fmt_chk.nChannels;

			//printf("'fmt ' chunk\n"
			//	"  rate    : %d\n"
			//	"  bits    : %d\n"
			//	"  channels: %d\n", 
			//	wavinf->rate, wavinf->bits, wavinf->ch);

			found_fmt = 1;
		} else if (chk_h.id == 0x61746164) {
			//found 'data' chunk
			wavinf->start = pos + sizeof(chk_h);
			wavinf->size = chk_h.size;

			//printf("'data' chunk\n"
			//	"  start   : %d\n"
			//	"  size    : %d\n", 
			//	(int)wavinf->start, (int)wavinf->size);

			found_data = 1;
		} else {
			//printf("'%c%c%c%c' chunk(ignored)\n", 
			//	buf[pos + 0], buf[pos + 1], 
			//	buf[pos + 2], buf[pos + 3]);
		}

		//goto next chunk
		pos += sizeof(chk_h) + chk_h.size;
	}

	if (!found_fmt) {
		fprintf(stderr, "Not found 'fmt ' chunk.\n");
		return -1;
	}
	if (!found_data) {
		fprintf(stderr, "Not found 'data' chunk.\n");
		return -1;
	}

	return 0;
}

void start(struct playback_info *rwinfo)
{
	pthread_mutex_lock(&rwinfo->th_mutex);
	if (rwinfo->state != IDLE) {
		pthread_mutex_unlock(&rwinfo->th_mutex);
		printf("already started.\n");

		return;
	}

	rwinfo->state = START;
	pthread_cond_broadcast(&rwinfo->th_cond);

	while (rwinfo->state == START 
		&& !rwinfo->should_stop) {
		pthread_cond_wait(&rwinfo->th_cond, 
			&rwinfo->th_mutex);
	}
	pthread_mutex_unlock(&rwinfo->th_mutex);
}

void stop(struct playback_info *rwinfo)
{
	pthread_mutex_lock(&rwinfo->th_mutex);
	if (rwinfo->state != RUN) {
		pthread_mutex_unlock(&rwinfo->th_mutex);
		printf("already stopped.\n");

		return;
	}

	rwinfo->state = STOP;
	pthread_cond_broadcast(&rwinfo->th_cond);

	while (rwinfo->state == STOP 
		&& !rwinfo->should_stop) {
		pthread_cond_wait(&rwinfo->th_cond, 
			&rwinfo->th_mutex);
	}
	pthread_mutex_unlock(&rwinfo->th_mutex);
}

void seek(struct playback_info *rwinfo, int sec)
{
	size_t newpos;

	newpos = sec * rwinfo->sampling_rate * rwinfo->frame_size;
	newpos = min(newpos, rwinfo->filesize);

	pthread_mutex_lock(&rwinfo->th_mutex);
	rwinfo->pos = newpos;
	pthread_mutex_unlock(&rwinfo->th_mutex);
}

void bytes_to_hms(const struct playback_info *rwinfo, size_t pos, struct time_hms *hms)
{
	size_t bytes_sec;

	bytes_sec = rwinfo->sampling_rate * rwinfo->frame_size;

	hms->hour   = pos / bytes_sec / 3600;
	hms->minute = (pos / bytes_sec / 60) % 60;
	hms->sec    = (pos / bytes_sec) % 60;
	hms->usec   = (size_t)((double)pos * 1000000 / bytes_sec) % 1000000;
}

int main(int argc, char *argv[])
{
	const char *device = "default";
	snd_pcm_format_t format;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	unsigned int sampling_rate;
	unsigned int channels;
	int soft_resample = 1;
	unsigned int latency = 50000;

	size_t frame_size;

	snd_pcm_t *pcm = NULL;
	char *filename;
	int fd = -1;
	off_t filesize;
	char *buf = NULL, *bufline = NULL, *buf_lpcm;
	struct wav_info wavinf;
	size_t pos, linesize;
	ssize_t readn;
	pthread_t th_playback;
	pthread_attr_t attr_playback;
	struct playback_info rwinfo;
	char cmd;
	int num;
	struct time_hms hmsn, hmsa;
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

	//allocate line buffer
	linesize = 1024;
	bufline = malloc(linesize);
	if (bufline == NULL) {
		perror("malloc(bufline)");
		result = -1;
		goto err_out;
	}

	//get size of LPCM file
	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("open");
		result = -1;
		goto err_out;
	}
	filesize = lseek(fd, 0, SEEK_END);
	if (filesize < 0) {
		perror("lseek");
		result = -1;
		goto err_out;
	}
	lseek(fd, 0, SEEK_SET);

	//read the WAV file
	buf = malloc(filesize);
	if (buf == NULL) {
		perror("malloc(buf)");
		result = -1;
		goto err_out;
	}

	readn = read(fd, buf, filesize);
	if (readn != filesize) {
		perror("read(LPCM)");
		result = -1;
		goto err_out;
	}

	//analyze the WAV file
	memset(&wavinf, 0, sizeof(wavinf));
	result = analyze_wav(buf, filesize, &wavinf);
	if (result != 0) {
		fprintf(stderr, "Not found the WAV header.\n");
		goto err_out;
	}

	//overwrite ALSA settings
	sampling_rate = wavinf.rate;
	switch (wavinf.bits) {
	case 8:
		format = SND_PCM_FORMAT_U8;
		break;
	case 16:
		format = SND_PCM_FORMAT_S16_LE;
		break;
	case 24:
		format = SND_PCM_FORMAT_S24_3LE;
		break;
	case 32:
		format = SND_PCM_FORMAT_S32_LE;
		break;
	default:
		fprintf(stderr, "Unknown LPCM %d bits.\n", wavinf.bits);
		result = -1;
		goto err_out;
	}
	channels = wavinf.ch;
	frame_size = wavinf.ch * wavinf.bits / 8;
	filesize = wavinf.size;
	buf_lpcm = &buf[wavinf.start];
	pos = 0;

	//open the playback ALSA device(non-blocking mode)
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

	result = snd_pcm_nonblock(pcm, 1);
	if (result < 0) {
		perror("snd_pcm_nonblock()");
		goto err_out;
	}

	//start player thread
	memset(&rwinfo, 0, sizeof(rwinfo));
	rwinfo.pcm = pcm;
	rwinfo.sampling_rate = sampling_rate;
	rwinfo.latency = latency;
	rwinfo.frame_size = frame_size;
	pthread_mutex_init(&rwinfo.th_mutex, NULL);
	pthread_cond_init(&rwinfo.th_cond, NULL);
	rwinfo.filesize = filesize;
	rwinfo.buf = buf_lpcm;
	rwinfo.pos = pos;
	rwinfo.state = IDLE;
	rwinfo.should_stop = 0;
	rwinfo.f_loop = 0;

	pthread_attr_init(&attr_playback);
	//pthread_attr_setschedpolicy(&attr_playback, SCHED_RR);
	pthread_create(&th_playback, &attr_playback, playback_main, &rwinfo);

	while (!rwinfo.should_stop) {
		cmd = '?';
		num = 0;
		printf("cmd> ");
		result = getline(&bufline, &linesize, stdin);
		if (linesize > 0) {
			cmd = bufline[0];
		}

		switch (cmd) {
		case 'i':
			//info
			printf("info.\n"
				"  rate    : %5d[samples/s]\n"
				"  bits    : %5d[bits]\n"
				"  channels: %5d[ch]\n"
				"  start   : %5d[bytes]\n"
				"  size    : %5d[bytes]\n", 
				wavinf.rate, wavinf.bits, wavinf.ch, 
				(int)wavinf.start, (int)wavinf.size);
			break;
		case 's':
			//start
			printf("start.\n");
			start(&rwinfo);
			printf("start done.\n");

			break;
		case 't':
			//stop
			printf("stop.\n");
			stop(&rwinfo);
			printf("stop done.\n");

			break;
		case 'p':
			//print
			pthread_mutex_lock(&rwinfo.th_mutex);
			bytes_to_hms(&rwinfo, rwinfo.pos, &hmsn);
			bytes_to_hms(&rwinfo, rwinfo.filesize, &hmsa);
			printf("print.\n"
				"  time: %02d:%02d:%02d.%06d/"
				"%02d:%02d:%02d.%06d\n"
				"  pos : %d/%d[bytes]\n", 
				hmsn.hour, hmsn.minute, hmsn.sec, hmsn.usec, 
				hmsa.hour, hmsa.minute, hmsa.sec, hmsa.usec, 
				(int)rwinfo.pos, (int)rwinfo.filesize);
			pthread_mutex_unlock(&rwinfo.th_mutex);

			break;
		case 'k':
			//seek
			printf("seek.\n");

			printf("where> ");
			result = getline(&bufline, &linesize, stdin);
			if (linesize > 0) {
				num = strtol(bufline, NULL, 0);
			}

			stop(&rwinfo);
			seek(&rwinfo, num);
			start(&rwinfo);
			printf("seek done.\n");

			break;
		case 'l':
			//loop on/off
			printf("loop.\n"
				"  mode %s -> %s.\n", 
				(rwinfo.f_loop) ? " on" : "off", 
				(!rwinfo.f_loop) ? " on" : "off");

			pthread_mutex_lock(&rwinfo.th_mutex);
			rwinfo.f_loop = !rwinfo.f_loop;
			pthread_mutex_unlock(&rwinfo.th_mutex);

			break;
		case 'q':
			//quit
			printf("quit.\n");

			pthread_mutex_lock(&rwinfo.th_mutex);
			rwinfo.should_stop = 1;
			pthread_cond_broadcast(&rwinfo.th_cond);
			pthread_mutex_unlock(&rwinfo.th_mutex);

			break;
		case 'h':
		case '?':
			//help
			usage();
			break;
		default:
			//unknown
			printf("unknown command '%c'.\n", 
				cmd);
			usage();
			break;
		}
	}

	pthread_join(th_playback, NULL);

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

	free(bufline);
	bufline = NULL;

	return result;
}

void *playback_main(void *arg)
{
	struct playback_info *rwinfo = 
		(struct playback_info *)arg;
	size_t pos;
	snd_pcm_sframes_t writen, frames;
	snd_pcm_sframes_t param_frms;
	int param_sleep;
	struct time_hms hms;
	struct timeval t_now, t_inter, t_target;
	struct timespec ts;
	int result;

restart:
	//wait for the start trigger
	pthread_mutex_lock(&rwinfo->th_mutex);
	while (rwinfo->state == IDLE && !rwinfo->should_stop) {
		pthread_cond_wait(&rwinfo->th_cond, 
			&rwinfo->th_mutex);
	}
	rwinfo->state = RUN;
	pthread_cond_broadcast(&rwinfo->th_cond);
	pthread_mutex_unlock(&rwinfo->th_mutex);

	//get start position
	pos = rwinfo->pos;

	//display interval: 333[ms]
	t_inter.tv_sec = 0;
	t_inter.tv_usec = 333000;
	gettimeofday(&t_target, NULL);

	//write size: half of the buffer
	param_frms = (double)rwinfo->sampling_rate 
		* rwinfo->latency / 1000000 / 2;

	//sleep time: half of the latency, '-10000' is safety margin
	param_sleep = 0;
	if (rwinfo->latency / 2 > 10000) {
		param_sleep = rwinfo->latency / 2 - 10000;
	}

	printf("parameters.\n"
		"  start : %d[bytes]\n"
		"  frames: %d[frames]\n"
		"  sleep : %d[us]\n", 
		(int)pos, (int)param_frms, param_sleep);

	while (pos < rwinfo->filesize && !rwinfo->should_stop) {
		//display current position
		gettimeofday(&t_now, NULL);
		if (timercmp(&t_target, &t_now, <)) {
			bytes_to_hms(rwinfo, pos, &hms);
			printf("\r%02d:%02d:%02d.%06d(%d/%d[bytes])", 
				hms.hour, hms.minute, hms.sec, hms.usec, 
				(int)pos, (int)rwinfo->filesize);
			fflush(stdout);

			timeradd(&t_now, &t_inter, &t_target);
		}

		//write LPCM frames
		frames = min(param_frms, 
			(rwinfo->filesize - pos) / rwinfo->frame_size);
		writen = snd_pcm_writei(rwinfo->pcm, &rwinfo->buf[pos], 
			frames);
		if (writen == -EAGAIN) {
			writen = 0;
		} else if (writen < 0) {
			perror("snd_pcm_writei()");

			//try to recover from under-run
			result = snd_pcm_recover(rwinfo->pcm, writen, 0);
			if (result < 0) {
				perror("snd_pcm_recover()");
				goto err_out;
			}
			writen = 0;
		}

		//update current position
		pos += writen * rwinfo->frame_size;

		pthread_mutex_lock(&rwinfo->th_mutex);
		rwinfo->pos = pos;
		pthread_mutex_unlock(&rwinfo->th_mutex);

		//poll the stop trigger
		if (rwinfo->state == STOP) {
			break;
		}

		//sleep, if buffer is full
		if (writen == 0) {
			ts.tv_sec = 0;
			ts.tv_nsec = param_sleep * 1000;
			nanosleep(&ts, NULL);
		}
	}

	if (rwinfo->pos >= rwinfo->filesize) {
		printf("\nreach the EOF.\n");
	}

	if (!rwinfo->should_stop) {
		pthread_mutex_lock(&rwinfo->th_mutex);
		if (rwinfo->pos >= rwinfo->filesize && rwinfo->f_loop) {
			printf("return to the first.\n");

			rwinfo->pos = 0;
			rwinfo->state = RUN;
			pthread_cond_broadcast(&rwinfo->th_cond);
		} else {
			printf("stop playing.\n");

			rwinfo->state = IDLE;
			pthread_cond_broadcast(&rwinfo->th_cond);
		}
		pthread_mutex_unlock(&rwinfo->th_mutex);

		goto restart;
	}

	return NULL;

err_out:
	printf("ERROR: abort player thread...\n");

	return NULL;
}
