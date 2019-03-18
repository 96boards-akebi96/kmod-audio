#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <pthread.h>

#include <alsa/asoundlib.h>

#define max(a, b)    ( ((a) > (b)) ? (a) : (b) )
#define min(a, b)    ( ((a) < (b)) ? (a) : (b) )

struct ringbuf {
	unsigned char *start;
	loff_t rd, wr;
	size_t len;
};

struct capture_playback_info {
	snd_pcm_t *c_pcm;
	snd_pcm_t *p_pcm;

	pthread_mutex_t ring_mutex;
	pthread_cond_t ring_cond;
	struct ringbuf snd_ring;
	size_t bufsize;

	int should_stop;
};

int init_ringbuf(struct ringbuf *r, unsigned char *buf, size_t len);
size_t get_remain_ringbuf(const struct ringbuf *r);
size_t get_space_ringbuf(const struct ringbuf *r);
ssize_t read_ringbuf(struct ringbuf *r, void *buf, size_t count);
ssize_t write_ringbuf(struct ringbuf *r, const void *buf, size_t count);

size_t get_remain(loff_t rd, loff_t wr, size_t len);
size_t get_remain_continuous(loff_t rd, loff_t wr, size_t len);
size_t get_space(loff_t rd, loff_t wr, size_t len);
size_t get_space_continuous(loff_t rd, loff_t wr, size_t len);

void *capture_main(void *arg);
void *playback_main(void *arg);

/**
 * Initialize the ring buffer.
 * The length of the ring buffer is set to (len - 1).
 * 
 * @param r   ring buffer
 * @param buf back buffer of ring
 * @param len length of back buffer
 * @return 0 is success, -1 is error.
 */
int init_ringbuf(struct ringbuf *r, unsigned char *buf, size_t len)
{
	if (r == NULL) {
		printf("cannot init the ring buffer(NULL).\n");
		return -1;
	}

	r->start = buf;
	r->len = len;
	r->rd = 0;
	r->wr = 0;

	return 0;
}

/**
 * Get the remain bytes of ring buffer.
 * 
 * @param r ring buffer
 * @return the remain bytes of buffer, -1 is error.
 */
size_t get_remain_ringbuf(const struct ringbuf *r)
{
	if (r == NULL) {
		printf("cannot get the remains of ring buffer(NULL).\n");
		return -1;
	}

	return get_remain(r->rd, r->wr, r->len);
}

/**
 * Get the space bytes of ring buffer.
 * 
 * @param r ring buffer
 * @return the space bytes of buffer, -1 is error.
 */
size_t get_space_ringbuf(const struct ringbuf *r)
{
	if (r == NULL) {
		printf("cannot get the space of ring buffer(NULL).\n");
		return -1;
	}

	return get_space(r->rd, r->wr, r->len);
}

/**
 * Read the data from ring buffer up to count bytes.
 * 
 * @param r     ring buffer
 * @param buf   the buffer to store the data read
 * @param count the number of the data
 * @return the read bytes from ring buffer, -1 is error.
 */
ssize_t read_ringbuf(struct ringbuf *r, void *buf, size_t count)
{
	unsigned char *cbuf = buf;
	size_t tran;
	ssize_t result;

	if (r == NULL || buf == NULL) {
		return -1;
	}

	result = 0;
	while (result < count) {
		if (get_remain(r->rd, r->wr, r->len) == 0) {
			//under
			break;
		}

		tran = count - result;
		tran = min(tran, 
			get_remain_continuous(r->rd, r->wr, r->len));

		memcpy(&cbuf[result], &r->start[r->rd], tran);

		result += tran;
		r->rd += tran;
		if (r->rd >= r->len) {
			r->rd = 0;
		}
	}

	return result;
}

/**
 * Write the data to ring buffer up to count bytes.
 * 
 * @param r     ring buffer
 * @param buf   the buffer that holds data
 * @param count the number of the data
 * @return the written bytes to ring buffer, -1 is error.
 */
ssize_t write_ringbuf(struct ringbuf *r, const void *buf, size_t count)
{
	const unsigned char *cbuf = buf;
	size_t tran;
	ssize_t result;

	if (r == NULL || buf == NULL) {
		return -1;
	}

	result = 0;
	while (result < count) {
		if (get_space(r->rd, r->wr, r->len) == 0) {
			//over
			break;
		}

		tran = count - result;
		tran = min(tran, 
			get_space_continuous(r->rd, r->wr, r->len));

		memcpy(&r->start[r->wr], &cbuf[result], tran);

		result += tran;
		r->wr += tran;
		if (r->wr >= r->len) {
			r->wr = 0;
		}
	}

	return result;
}

/**
 * Get the remain bytes of buffer.
 * 
 * @param rd  read pointer of buffer
 * @param wr  write pointer of buffer
 * @param len length of buffer
 * @return the remain bytes of buffer.
 */
size_t get_remain(loff_t rd, loff_t wr, size_t len)
{
	size_t rest;

	if (rd <= wr) {
		rest = wr - rd;
	} else {
		rest = len - (rd - wr);
	}

	return rest;
}

/**
 * Get the continuous remain bytes of buffer.
 * 
 * @param rd  read pointer of buffer
 * @param wr  write pointer of buffer
 * @param len length of buffer
 * @return the remain bytes of buffer.
 */
size_t get_remain_continuous(loff_t rd, loff_t wr, size_t len)
{
	size_t rest;

	if (rd <= wr) {
		rest = wr - rd;
	} else {
		rest = len - rd;
	}

	return rest;
}

/**
 * Get the space bytes of buffer.
 * 
 * @param rd  read pointer of buffer
 * @param wr  write pointer of buffer
 * @param len length of buffer
 * @return the space bytes of buffer.
 */
size_t get_space(loff_t rd, loff_t wr, size_t len)
{
	size_t rest;

	if (rd <= wr) {
		rest = len - (wr - rd) - 1;
	} else {
		rest = rd - wr - 1;
	}

	return rest;
}

/**
 * Get the continuous space bytes of buffer.
 * 
 * @param rd  read pointer of buffer
 * @param wr  write pointer of buffer
 * @param len length of buffer
 * @return the continuous space bytes of buffer.
 */
size_t get_space_continuous(loff_t rd, loff_t wr, size_t len)
{
	size_t rest;

	if (rd > wr) {
		rest = rd - wr - 1;
	} else if (rd > 0) {
		rest = len - wr;
	} else {
		rest = len - wr - 1;
	}

	return rest;
}

int main(int argc, char *argv[])
{
	const char *c_device = "default";
	snd_pcm_format_t c_format = SND_PCM_FORMAT_S32_LE;
	snd_pcm_access_t c_access = SND_PCM_ACCESS_RW_INTERLEAVED;
	unsigned int c_sampling_rate = 48000;
	unsigned int c_channels = 2;
	int c_soft_resample = 1;
	unsigned int c_latency = 20000;

	const char *p_device = "default";
	snd_pcm_format_t p_format = SND_PCM_FORMAT_S32_LE;
	snd_pcm_access_t p_access = SND_PCM_ACCESS_RW_INTERLEAVED;
	unsigned int p_sampling_rate = 48000;
	unsigned int p_channels = 2;
	int p_soft_resample = 1;
	unsigned int p_latency = 20000;

	snd_pcm_t *c_pcm = NULL, *p_pcm = NULL;
	char *buf;
	size_t bufsize;
	pthread_t th_capture, th_playback;
	pthread_attr_t attr_capture, attr_playback;
	struct capture_playback_info rwinfo;
	int result = -1;

	if (argc < 1) {
		fprintf(stderr, "usage: \n%s [capture_dev] [playback_dev]\n", 
			argv[0]);
		return 1;
	}

	if (argc >= 2) {
		c_device = argv[1];
	}
	if (argc >= 3) {
		p_device = argv[2];
	}

	bufsize = 8 * 1024;

	buf = malloc(bufsize);
	if (buf == NULL) {
		perror("malloc");
		result = -1;
		goto err_out;
	}

	//open the capture and playback devices of ALSA
	result = snd_pcm_open(&c_pcm, c_device, SND_PCM_STREAM_CAPTURE, 0);
	if (result < 0) {
		perror("snd_pcm_open(capture)");
		goto err_out;
	}
	result = snd_pcm_open(&p_pcm, p_device, SND_PCM_STREAM_PLAYBACK, 0);
	if (result < 0) {
		perror("snd_pcm_open(playback)");
		goto err_out;
	}

	//set sampling rate, frame format, buffer size of device
	result = snd_pcm_set_params(c_pcm, c_format, c_access, c_channels, 
		c_sampling_rate, c_soft_resample, c_latency);
	if (result < 0) {
		perror("snd_pcm_set_params(capture)");
		goto err_out;
	}
	result = snd_pcm_start(c_pcm);
	if (result < 0) {
		perror("snd_pcm_start(capture)");
		goto err_out;
	}
	result = snd_pcm_set_params(p_pcm, p_format, p_access, p_channels, 
		p_sampling_rate, p_soft_resample, p_latency);
	if (result < 0) {
		perror("snd_pcm_set_params(playback)");
		goto err_out;
	}

	pthread_attr_init(&attr_capture);
	//pthread_attr_setschedpolicy(&attr_capture, SCHED_RR);
	pthread_attr_init(&attr_playback);
	//pthread_attr_setschedpolicy(&attr_playback, SCHED_RR);

	memset(&rwinfo, 0, sizeof(rwinfo));
	rwinfo.c_pcm = c_pcm;
	rwinfo.p_pcm = p_pcm;
	pthread_mutex_init(&rwinfo.ring_mutex, NULL);
	pthread_cond_init(&rwinfo.ring_cond, NULL);
	init_ringbuf(&rwinfo.snd_ring, (unsigned char *)buf, bufsize);
	rwinfo.bufsize = bufsize;
	rwinfo.should_stop = 0;
	pthread_create(&th_capture, &attr_capture, capture_main, &rwinfo);
	pthread_create(&th_playback, &attr_playback, playback_main, &rwinfo);

	pthread_join(th_playback, NULL);
	pthread_join(th_capture, NULL);

	result = 0;

err_out:
	if (p_pcm) {
		snd_pcm_close(p_pcm);
		p_pcm = NULL;
	}
	if (c_pcm) {
		snd_pcm_close(c_pcm);
		c_pcm = NULL;
	}

	free(buf);
	buf = NULL;

	return result;
}

void *capture_main(void *arg)
{
	struct capture_playback_info *rwinfo = 
		(struct capture_playback_info *)arg;
	char *buf = NULL;
	ssize_t readn, writen;
	size_t pos, len;
	size_t period;
	int result;

	buf = malloc(rwinfo->bufsize);
	if (buf == NULL) {
		perror("malloc(capture buf)");
		goto err_out;
	}

	//10.6ms = 512frames, 1frame = 8bytes(32bit, stereo)
	period = min(rwinfo->bufsize / 8, 512);

	while (!rwinfo->should_stop) {
		readn = snd_pcm_readi(rwinfo->c_pcm, &buf[0],
			period);
		if (readn < 0) {
			perror("snd_pcm_readi()");

			//try to recover from over-run
			result = snd_pcm_recover(rwinfo->c_pcm, readn, 0);
			if (result < 0) {
				perror("snd_pcm_recover(capture)");
				goto err_out;
			}
			result = snd_pcm_drain(rwinfo->c_pcm);
			if (result < 0) {
				perror("snd_pcm_drain(capture)");
				goto err_out;
			}
			while (snd_pcm_start(rwinfo->c_pcm) < 0) {
				perror("snd_pcm_start(capture)");
				goto err_out;
			}

			continue;
		}

		//1frame = 8bytes(32bit, stereo)
		pos = 0;
		len = readn * 8;
		while (pos < len && !rwinfo->should_stop) {
			pthread_mutex_lock(&rwinfo->ring_mutex);

			//wait for the playback
			while (get_space_ringbuf(&rwinfo->snd_ring) == 0
					&& !rwinfo->should_stop) {
				pthread_cond_wait(&rwinfo->ring_cond, 
					&rwinfo->ring_mutex);
			}
			writen = write_ringbuf(&rwinfo->snd_ring, 
				&buf[pos], len - pos);

			//wakeup the playback
			pthread_cond_broadcast(&rwinfo->ring_cond);

			pthread_mutex_unlock(&rwinfo->ring_mutex);

			pos += writen;
		}
	}

err_out:
	//abort all threads
	rwinfo->should_stop = 1;
	snd_pcm_nonblock(rwinfo->c_pcm, 2);
	snd_pcm_nonblock(rwinfo->p_pcm, 2);
	pthread_cond_broadcast(&rwinfo->ring_cond);

	free(buf);
	buf = NULL;

	return NULL;
}

void *playback_main(void *arg)
{
	struct capture_playback_info *rwinfo = 
		(struct capture_playback_info *)arg;
	char *buf = NULL;
	ssize_t readn, writen;
	size_t pos, len;
	size_t period;
	int result;

	buf = malloc(rwinfo->bufsize);
	if (buf == NULL) {
		perror("malloc(playback buf)");
		goto err_out;
	}

	//10.6ms = 512frames, 1frame = 8bytes(32bit, stereo)
	period = min(rwinfo->bufsize / 8, 512);

	while (!rwinfo->should_stop) {
		pos = 0;
		len = period * 8;
		while (pos < len && !rwinfo->should_stop) {
			pthread_mutex_lock(&rwinfo->ring_mutex);

			//wait for the capture
			while (get_remain_ringbuf(&rwinfo->snd_ring) == 0
					&& !rwinfo->should_stop) {
				pthread_cond_wait(&rwinfo->ring_cond, 
					&rwinfo->ring_mutex);
			}
			readn = read_ringbuf(&rwinfo->snd_ring, 
				&buf[pos], len - pos);

			//wakeup the capture
			pthread_cond_broadcast(&rwinfo->ring_cond);

			pthread_mutex_unlock(&rwinfo->ring_mutex);

			pos += readn;
		}

		writen = snd_pcm_writei(rwinfo->p_pcm, &buf[0], 
			period);
		if (writen < 0) {
			perror("snd_pcm_writei()");

			//try to recover from under-run
			result = snd_pcm_recover(rwinfo->p_pcm, writen, 0);
			if (result < 0) {
				perror("snd_pcm_recover(playback)");
				goto err_out;
			}
		}
	}

err_out:
	//abort all threads
	rwinfo->should_stop = 1;
	snd_pcm_nonblock(rwinfo->c_pcm, 2);
	snd_pcm_nonblock(rwinfo->p_pcm, 2);
	pthread_cond_broadcast(&rwinfo->ring_cond);

	free(buf);
	buf = NULL;

	return NULL;
}
