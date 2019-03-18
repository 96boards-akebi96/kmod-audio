/*
 * Socionext UniPhier AIO ALSA driver.
 *
 * Copyright (c) 2016-2017 Socionext Inc.
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

#ifndef SND_UNIPHIER_AIO_JACK_H__
#define SND_UNIPHIER_AIO_JACK_H__

#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/hrtimer.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <sound/core.h>
#include <sound/jack.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include "aio.h"

int uniphier_jack_dai_probe(struct snd_soc_dai *dai);
int uniphier_jack_dai_resume(struct snd_soc_dai *dai);
int uniphier_jack_probe(struct platform_device *pdev);

#endif /* SND_UNIPHIER_AIO_JACK_H__ */
