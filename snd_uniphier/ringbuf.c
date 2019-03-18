/*
 * ALSA driver for Panasonic UniPhier series.
 *
 * Copyright (c) 2013 Panasonic corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
 
#define SND_UNIPHIER_DRVNAME    "snd-uniphier"

#define pr_fmt(fmt) SND_UNIPHIER_DRVNAME ": " fmt

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>

#include "ringbuf.h"

int memcpy_wrap(void *dest, const void *src, size_t n, size_t *ndst, size_t *nsrc)
{
	if (!dest || !src || !ndst || !nsrc) {
		return -1;
	}

	memcpy(dest, src, n);

	*ndst = n;
	*nsrc = n;

	return 0;
}

/**
 * Byte swap.
 *
 * v      : 0x00010203
 * return : 0x03020100
 *
 * @param v  any value
 * @return  swapped value
 */
u32 rev32(u32 v)
{
	v = ((v & 0x00ff00ff) <<  8) | ((v & 0xff00ff00) >>  8);
	v = ((v & 0x0000ffff) << 16) | ((v & 0xffff0000) >> 16);

	return v;
}

/**
 * Copy data with byte swap.
 *
 *   src : 00 01 02 03  04 05 06 07  ...
 *   dest: 03 02 01 00  07 06 05 04  ...
 *
 * @param dest destination buffer
 * @param src  source buffer
 * @param n    size of data (multiples of 4)
 * @param ndst return the size of written data
 * @param nsrc return the size of read data
 * @return 0 is success, -1 is error
 */
int memcpy_rev(void *dest, const void *src, size_t n, size_t *ndst, size_t *nsrc)
{
	u32 *d;
	const u32 *s;
	size_t i;

	if (!dest || !src || !ndst || !nsrc) {
		return -1;
	}

	if ((n & 3) != 0) {
		pr_warning("maybe overrun...(n:%d)\n", (int)n);
	}

	d = dest;
	s = src;
	for (i = 0; i < n / 4; i++) {
		d[i] = rev32(s[i]);
	}

	*ndst = n;
	*nsrc = n;

	return 0;
}

/**
 * Copy data with channel swap.
 *
 * interleave format of 7.1 ch as follows,
 *
 * ALSA    : FL, FR, RL, RR,  C, LFE, SL, SR
 * UniPhier: FL, SL,  C, BL, FR, SR, LFE, BR
 *
 * @param dest destination buffer (UniPhier format)
 * @param src  source buffer (ALSA interleave format)
 * @param n    size of data (multiples of 4)
 * @param ndst return the size of written data
 * @param nsrc return the size of read data
 * @return 0 is success, -1 is error
 */
int memcpy_alsa_to_uniphier(void *dest, const void *src, size_t n, size_t *ndst, size_t *nsrc)
{
	static const int trans_8ch[] = {
		0, /* AIO 0( FL) <- ALSA 0( FL) */
		4, /* AIO 4( FR) <- ALSA 1( FR) */
		2, /* AIO 2( BL) <- ALSA 2( RL) */
		6, /* AIO 6( BR) <- ALSA 3( RR) */
		3, /* AIO 3(  C) <- ALSA 4(  C) */
		7, /* AIO 7(LFE) <- ALSA 5(LFE) */
		1, /* AIO 1( SL) <- ALSA 6( SL) */
		5, /* AIO 5( SR) <- ALSA 7( SR) */
	};

	u32 *aiobuf;
	const u32 *alsabuf;
	const int *table;
	int dstch, srcch;
	size_t dstsmplen, srcsmplen, dstfrmlen, srcfrmlen;
	size_t d, s, i;

	if (!dest || !src || !ndst || !nsrc) {
		return -1;
	}

	/* dest: 7.1 ch, 32 bits */
	dstch = 8;
	dstsmplen = 4;
	dstfrmlen = dstch * dstsmplen;
	/* src : 7.1 ch, 32 bits */
	srcch = 8;
	srcsmplen = 4;
	srcfrmlen = srcch * srcsmplen;

	if (n % srcfrmlen != 0) {
		pr_warning("avoid overrun...(n:%d)\n", (int)n);
		n -= n % srcfrmlen;
	}

	aiobuf = (u32 *)dest;
	alsabuf = (const u32 *)src;

	for (d = 0, s = 0; s < n / srcfrmlen; d += dstch, s += srcch) {
		switch (srcch) {
		case 8:
			table = trans_8ch;
			break;
		default:
			table = NULL;
			break;
		}

		if (table == NULL) {
			return -1;
		}

		for (i = 0; i < srcch; i++) {
			aiobuf[d + table[i]]  = rev32(alsabuf[s + i]);
		}
	}

	*ndst = d * dstsmplen;
	*nsrc = s * srcsmplen;

	return 0;
}

/**
 * Initialize the ring buffer.
 *
 * NOTE: Capacity of ringbuffer is len - 1.
 *
 * @param r   ring buffer
 * @param buf back buffer of this ring buffer
 * @param len size of back buffer
 * @return 0 is success, -1 is error
 */
int init_ringbuf(struct ringbuf *r, unsigned char *buf, size_t len)
{
	if (r == NULL) {
		pr_err("cannot init the ring buffer(NULL).\n");
		return -1;
	}

	r->start = buf;
	r->len = len;
	r->rd = 0;
	r->wr = 0;

	return 0;
}

/**
 * Get read position of ring buffer.
 *
 * Read position range is [0, len - 1]
 *
 * @param r ring buffer
 * @return current read position, -1 is error
 */
loff_t get_rp_ringbuf(const struct ringbuf *r)
{
	if (r == NULL) {
		pr_err("cannot get the read pos of ring buffer(NULL).\n");
		return -1;
	}

	return r->rd;
}

/**
 * Set read position of ring buffer.
 *
 * @param r ring buffer
 * @param pos new read position
 * @return 0 is success, -1 is error
 */
int set_rp_ringbuf(struct ringbuf *r, loff_t pos)
{
	if (r == NULL) {
		pr_err("cannot set the read pos of ring buffer(NULL).\n");
		return -1;
	}

	r->rd = pos % r->len;

	return 0;
}

/**
 * Get write position of ring buffer.
 *
 * Write position range is [0, len - 1]
 *
 * @param r ring buffer
 * @return current write position, -1 is error
 */
loff_t get_wp_ringbuf(const struct ringbuf *r)
{
	if (r == NULL) {
		pr_err("cannot get the write pos of ring buffer(NULL).\n");
		return -1;
	}

	return r->wr;
}

/**
 * Set write position of ring buffer.
 *
 * @param r ring buffer
 * @param pos new write position
 * @return 0 is success, -1 is error
 */
int set_wp_ringbuf(struct ringbuf *r, loff_t pos)
{
	if (r == NULL) {
		pr_err("cannot set the write pos of ring buffer(NULL).\n");
		return -1;
	}

	r->wr = pos % r->len;

	return 0;
}

/**
 * Get size of remaining data in ring buffer.
 *
 * @param r ring buffer
 * @return current remaining data size, -1 is error
 */
size_t get_remain_ringbuf(const struct ringbuf *r)
{
	if (r == NULL) {
		pr_err("cannot get the remains of ring buffer(NULL).\n");
		return -1;
	}

	return get_remain(r->rd, r->wr, r->len);
}

/**
 * Get size of spece in ring buffer.
 *
 * @param r ring buffer
 * @return current spece size, -1 is error
 */
size_t get_space_ringbuf(const struct ringbuf *r)
{
	if (r == NULL) {
		pr_err("cannot get the space of ring buffer(NULL).\n");
		return -1;
	}

	return get_space(r->rd, r->wr, r->len);
}

/**
 * Fill the ring buffer.
 *
 * @param r  ring buffer
 * @param d  a byte value to fill
 * @return 0 is success, -1 is error
 */
int fill_ringbuf(struct ringbuf *r, unsigned char d)
{
	if (r == NULL) {
		pr_err("cannot fill the ring buffer(NULL).\n");
		return -1;
	}

	memset(r->start, d, r->len);

	return 0;
}

/**
 * Read data from ring buffer with translation.
 *
 * @param r     ring buffer
 * @param buf   buffer
 * @param count size of data
 * @param trans pointer of translate function
 * @return size of read data, -1 is error
 */
ssize_t read_ringbuf_trans(struct ringbuf *r, void *buf, size_t count, trans_copy_t trans)
{
	unsigned char *cbuf = buf;
	size_t n, ndst, nsrc;
	ssize_t result;
	int res;

	if (r == NULL || buf == NULL) {
		return -1;
	}

	result = 0;
	while (result < count) {
		if (get_remain(r->rd, r->wr, r->len) == 0) {
			/* underflow */
			break;
		}

		n = count - result;
		n = min(n, get_remain_continuous(r->rd, r->wr, r->len));

		res = trans(&cbuf[result], &r->start[r->rd],
			n, &ndst, &nsrc);
		if (res != 0) {
			break;
		}

		result += ndst;
		r->rd += nsrc;
		if (r->rd >= r->len) {
			r->rd = 0;
		}
	}

	return result;
}

/**
 * Write data to ring buffer with translation.
 *
 * @param r     ring buffer
 * @param buf   buffer
 * @param count size of data
 * @param trans pointer of translate function
 * @return size of written data, -1 is error
 */
ssize_t write_ringbuf_trans(struct ringbuf *r, const void *buf, size_t count, trans_copy_t trans)
{
	const unsigned char *cbuf = buf;
	size_t n, ndst, nsrc;
	ssize_t result;
	int res;

	if (r == NULL || buf == NULL) {
		return -1;
	}

	result = 0;
	while (result < count) {
		if (get_space(r->rd, r->wr, r->len) == 0) {
			/* overflow */
			break;
		}

		n = count - result;
		n = min(n, get_space_continuous(r->rd, r->wr, r->len));

		res = trans(&r->start[r->wr], &cbuf[result],
			n, &ndst, &nsrc);
		if (res != 0) {
			break;
		}

		result += nsrc;
		r->wr += ndst;
		if (r->wr >= r->len) {
			r->wr = 0;
		}
	}

	return result;
}

/**
 * Write silent data to ring buffer.
 *
 * @param r     ring buffer
 * @param count size of data to fill
 * @return size of written data, -1 is error
 */
ssize_t write_silent_ringbuf(struct ringbuf *r, size_t count)
{
	size_t n;
	ssize_t result;

	if (r == NULL) {
		return -1;
	}

	result = 0;
	while (result < count) {
		if (get_space(r->rd, r->wr, r->len) == 0) {
			//over
			break;
		}

		n = count - result;
		n = min(n, get_space_continuous(r->rd, r->wr, r->len));

		memset(&r->start[r->wr], 0, n);

		result += n;
		r->wr += n;
		if (r->wr >= r->len) {
			r->wr = 0;
		}
	}

	return result;
}

/**
 * Copy data to ring buffer from other ring.
 *
 * @param dest  destination ring buffer
 * @param src   source ring buffer
 * @param count size of data
 * @param trans pointer of tranlate function
 * @return size of written data, -1 is error
 */
ssize_t copy_ringbuf(struct ringbuf *dest, struct ringbuf *src, size_t count)
{
	return copy_ringbuf_trans(dest, src, count, memcpy_wrap);
}

/**
 * Copy data to ring buffer from other ring with translation.
 *
 * @param dest  destination ring buffer
 * @param src   source ring buffer
 * @param count size of data
 * @param trans pointer of tranlate function
 * @return size of written data, -1 is error
 */
ssize_t copy_ringbuf_trans(struct ringbuf *dest, struct ringbuf *src, size_t count, trans_copy_t trans)
{
	size_t n, ndst, nsrc;
	ssize_t result;
	int res;

	if (dest == NULL || src == NULL) {
		return -1;
	}

	result = 0;
	while (result < count) {
		if (get_space(dest->rd, dest->wr, dest->len) == 0) {
			/* overflow */
			break;
		}
		if (get_remain(src->rd, src->wr, src->len) == 0) {
			/* underflow */
			break;
		}

		n = count - result;
		n = min(n, get_space_continuous(dest->rd, dest->wr, dest->len));
		n = min(n, get_remain_continuous(src->rd, src->wr, src->len));

		res = trans(&dest->start[dest->wr], &src->start[src->rd],
			n, &ndst, &nsrc);
		if (res != 0) {
			break;
		}

		result += ndst;
		dest->wr += ndst;
		if (dest->wr >= dest->len) {
			dest->wr = 0;
		}

		src->rd += nsrc;
		if (src->rd >= src->len) {
			src->rd = 0;
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

void timeradd(struct timeval *a, struct timeval *b, struct timeval *res)
{
	res->tv_sec = a->tv_sec + b->tv_sec;
	res->tv_usec = a->tv_usec + b->tv_usec;

	if (res->tv_usec >= 1000000) {
		res->tv_sec += 1;
		res->tv_usec -= 1000000;
	}
}

void timersub(struct timeval *a, struct timeval *b, struct timeval *res)
{
	res->tv_sec = a->tv_sec - b->tv_sec;
	res->tv_usec = a->tv_usec - b->tv_usec;

	if (res->tv_usec < 0) {
		res->tv_sec -= 1;
		res->tv_usec += 1000000;
	}
}

void timerclear(struct timeval *tvp)
{
	tvp->tv_sec = 0;
	tvp->tv_usec = 0;
}
