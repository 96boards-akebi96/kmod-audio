/*
 * Socionext UniPhier I2S driver.
 *
 * Copyright (c) 2016 Socionext Inc.
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

#ifndef SND_UNIPHIER_RINGBUF_H__
#define SND_UNIPHIER_RINGBUF_H__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>

struct ringbuf {
	unsigned char *start;
	size_t len;
	loff_t rd, wr;
};

typedef int (* trans_copy_t)(void *dest, const void *src, size_t n, size_t *ndst, size_t *nsrc);

int memcpy_wrap(void *dest, const void *src, size_t n, size_t *ndst, size_t *nsrc);
u32 rev32(u32 v);
int memcpy_rev(void *dest, const void *src, size_t n, size_t *ndst, size_t *nsrc);
int memcpy_alsa_to_uniphier(void *dest, const void *src, size_t n, size_t *ndst, size_t *nsrc);

int init_ringbuf(struct ringbuf *r, unsigned char *buf, size_t len);
loff_t get_rp_ringbuf(const struct ringbuf *r);
int set_rp_ringbuf(struct ringbuf *r, loff_t pos);
loff_t get_wp_ringbuf(const struct ringbuf *r);
int set_wp_ringbuf(struct ringbuf *r, loff_t pos);
size_t get_remain_ringbuf(const struct ringbuf *r);
size_t get_space_ringbuf(const struct ringbuf *r);
int fill_ringbuf(struct ringbuf *r, unsigned char d);
ssize_t read_ringbuf_trans(struct ringbuf *r, void *buf, size_t count, trans_copy_t trans);
ssize_t write_ringbuf_trans(struct ringbuf *r, const void *buf, size_t count, trans_copy_t trans);
ssize_t write_silent_ringbuf(struct ringbuf *r, size_t count);
ssize_t copy_ringbuf(struct ringbuf *dest, struct ringbuf *src, size_t count);
ssize_t copy_ringbuf_trans(struct ringbuf *dest, struct ringbuf *src, size_t count, trans_copy_t trans);

size_t get_remain(loff_t rd, loff_t wr, size_t len);
size_t get_remain_continuous(loff_t rd, loff_t wr, size_t len);
size_t get_space(loff_t rd, loff_t wr, size_t len);
size_t get_space_continuous(loff_t rd, loff_t wr, size_t len);

void timeradd(struct timeval *a, struct timeval *b, struct timeval *res);
void timersub(struct timeval *a, struct timeval *b, struct timeval *res);
void timerclear(struct timeval *tvp);

#endif /* SND_UNIPHIER_RINGBUF_H__ */
