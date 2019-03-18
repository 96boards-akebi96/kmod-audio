/*
 * ALSA SoC Thru io codec driver.
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

#define DRV_NAME    "thru-io"

#define pr_fmt(fmt) DRV_NAME "" fmt

#include <linux/module.h>
#include <linux/slab.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <linux/of.h>

#define THRU_RATES      SNDRV_PCM_RATE_8000_192000
#define THRU_FORMATS    (SNDRV_PCM_FMTBIT_S16_LE | \
	SNDRV_PCM_FMTBIT_S20_3LE | \
	SNDRV_PCM_FMTBIT_S24_LE  | \
	SNDRV_PCM_FMTBIT_S32_LE | \
	SNDRV_PCM_FMTBIT_S16_BE | \
	SNDRV_PCM_FMTBIT_S20_3BE | \
	SNDRV_PCM_FMTBIT_S24_BE  | \
	SNDRV_PCM_FMTBIT_S32_BE)

static const struct snd_soc_dapm_widget thru_widgets[] = {
	SND_SOC_DAPM_OUTPUT("thru-out"),
	SND_SOC_DAPM_INPUT("thru-in"),
};

static const struct snd_soc_dapm_route thru_routes[] = {
	{ "thru-out", NULL, "Playback" },
	{ "Capture", NULL, "thru-in" },
};

static struct snd_soc_codec_driver soc_codec_thru = {
	.component_driver = {
		.dapm_widgets = thru_widgets,
		.num_dapm_widgets = ARRAY_SIZE(thru_widgets),
		.dapm_routes = thru_routes,
		.num_dapm_routes = ARRAY_SIZE(thru_routes),
	},
};

static struct snd_soc_dai_driver soc_dai_thru[] = {
	{
		.name     = DRV_NAME,
		.playback = {
			.stream_name  = "Playback",
			.formats      = THRU_FORMATS,
			.rates        = THRU_RATES,
			.channels_min = 1,
			.channels_max = 128,
		},
		.capture = {
			.stream_name  = "Capture",
			.formats      = THRU_FORMATS,
			.rates        = THRU_RATES,
			.channels_min = 1,
			.channels_max = 128,
		},
	},
};

static int thru_io_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev, &soc_codec_thru,
		soc_dai_thru, ARRAY_SIZE(soc_dai_thru));
}

static int thru_io_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id thru_io_of_match[] = {
	{ .compatible = "socionext," DRV_NAME, },
	{}
};
MODULE_DEVICE_TABLE(of, thru_io_of_match);
#endif

static struct platform_driver thru_io_codec_driver = {
	.driver = {
		.name = DRV_NAME,
		.of_match_table = of_match_ptr(thru_io_of_match),
	},
	.probe  = thru_io_probe,
	.remove = thru_io_remove,
};
module_platform_driver(thru_io_codec_driver);

MODULE_AUTHOR("Katsuhiro Suzuki <suzuki.katsuhiro@socionext.com>");
MODULE_DESCRIPTION("Thru io dummy codec driver");
MODULE_LICENSE("GPL v2");
