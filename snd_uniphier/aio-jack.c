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

#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include "aio.h"
//#include "aio-regctrl.h"
#include "aio-jack.h"

static struct snd_soc_jack_pin uniphier_jack_pins[] = {
	{
		.pin  = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};

int uniphier_jack_dai_probe(struct snd_soc_dai *dai)
{
	struct uniphier_aio *aio = uniphier_priv(dai);
	struct uniphier_aio_chip *chip = aio->chip;
	struct device *dev = &aio->chip->pdev->dev;
	size_t sz;
	int ret;

	if (chip->irq_jack != 0 && !chip->enable_jack) {
		chip->enable_jack = 1;

		chip->num_jack_pins = ARRAY_SIZE(uniphier_jack_pins);
		sz = sizeof(struct snd_soc_jack_pin) * chip->num_jack_pins;
		chip->jack_pins = devm_kzalloc(dev, sz, GFP_KERNEL);
		memcpy(chip->jack_pins, uniphier_jack_pins, sz);

		ret = snd_soc_card_jack_new(dai->component->card,
			"Headphones", SND_JACK_HEADPHONE, &chip->jack,
			chip->jack_pins, chip->num_jack_pins);
		if (ret < 0) {
			dev_err(dev, "Setup Headphone jack failed.\n");
			return ret;
		}
	}

	/* check jack status at first */
	schedule_delayed_work(&chip->jack_work, chip->delay_jack);

	return 0;
}

int uniphier_jack_dai_resume(struct snd_soc_dai *dai)
{
	struct uniphier_aio *aio = uniphier_priv(dai);
	struct uniphier_aio_chip *chip = aio->chip;

	schedule_delayed_work(&chip->jack_work, chip->delay_jack);

	return 0;
}

static int uniphier_jack_request_irq(struct platform_device *pdev,
				    const char *name, int i,
				    irq_handler_t handler, void *data,
				    int *irq)
{
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
	if (IS_ERR_OR_NULL(res))
		return PTR_ERR(res);

	ret = devm_request_irq(&pdev->dev, res->start, handler, 0, name, data);
	if (ret != 0)
		return ret;

	*irq = res->start;

	return ret;
}

static void uniphier_jack_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct uniphier_aio_chip *chip = container_of(dwork,
		struct uniphier_aio_chip, jack_work);
	void __iomem *addr;
	int report;
	u32 val;

	if (!chip->enable_jack)
		return;

	//val = gpiod_get_value_cansleep(aio->chip->gpio_jack);

	/* FIXME: Read PORT150 from SG directly */
	addr = ioremap(0x55000000, 0x1000);
	val = readl(addr + 0x80) & 1;
	iounmap(addr);

	/* H level: plug is absent, L level: plug is present */
	if (val)
		report = 0;
	else
		report = SND_JACK_HEADPHONE;

	snd_soc_jack_report(&chip->jack, report, SND_JACK_HEADPHONE);
}

static irqreturn_t uniphier_jack_handler(int irq, void *p)
{
	struct platform_device *pdev = p;
	struct uniphier_aio_chip *chip = dev_get_drvdata(&pdev->dev);

	schedule_delayed_work(&chip->jack_work, chip->delay_jack);

	return IRQ_HANDLED;
}

int uniphier_jack_probe(struct platform_device *pdev)
{
	struct uniphier_aio_chip *chip = dev_get_drvdata(&pdev->dev);
	struct device *dev = &pdev->dev;
	int ret;

	INIT_DELAYED_WORK(&chip->jack_work, uniphier_jack_worker);
	chip->delay_jack = HZ / 2;

	chip->gpio_jack = devm_gpiod_get_optional(dev, "jack", GPIOD_IN);
	if (IS_ERR(chip->gpio_jack)) {
		dev_err(dev, "Get gpio 'jack' failed.\n");
		return PTR_ERR(chip->gpio_jack);
	}

	ret = uniphier_jack_request_irq(pdev, "aio-jack", 1,
		uniphier_jack_handler, pdev, &chip->irq_jack);
	if (ret != 0)
		dev_warn(dev, "Request IRQ 'aio-jack' failed, ignored.\n");

	return 0;
}
