/*
 * s3c-dma-wrapper.c  --  S3C DMA Platform Wrapper Driver
 *
 * Copyright (c) 2010 Samsung Electronics Co. Ltd
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <sound/soc.h>
#include "dma.h"
#include "s3c-idma.h"

static int s3c_wrpdma_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_platform_driver *platform;

#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		platform = &idma_soc_platform;
	else
#endif
		platform = &samsung_asoc_platform;

	if (platform->ops->hw_params)
		return platform->ops->hw_params(substream, params);
	else
		return 0;
}

static int s3c_wrpdma_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_platform_driver *platform;

#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		platform = &idma_soc_platform;
	else
#endif
		platform = &samsung_asoc_platform;

	if (platform->ops->hw_free)
		return platform->ops->hw_free(substream);
	else
		return 0;
}

static int s3c_wrpdma_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_platform_driver *platform;

#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		platform = &idma_soc_platform;
	else
#endif
		platform = &samsung_asoc_platform;

	if (platform->ops->prepare)
		return platform->ops->prepare(substream);
	else
		return 0;
}

static int s3c_wrpdma_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_platform_driver *platform;

#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		platform = &idma_soc_platform;
	else
#endif
		platform = &samsung_asoc_platform;

	if (platform->ops->trigger)
		return platform->ops->trigger(substream, cmd);
	else
		return 0;
}

static snd_pcm_uframes_t s3c_wrpdma_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_platform_driver *platform;

#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		platform = &idma_soc_platform;
	else
#endif
		platform = &samsung_asoc_platform;

	if (platform->ops->pointer)
		return platform->ops->pointer(substream);
	else
		return 0;
}

static int s3c_wrpdma_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_platform_driver *platform;

#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		platform = &idma_soc_platform;
	else
#endif
		platform = &samsung_asoc_platform;

	if (platform->ops->open)
		return platform->ops->open(substream);
	else
		return 0;
}

static int s3c_wrpdma_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_platform_driver *platform;

#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		platform = &idma_soc_platform;
	else
#endif
		platform = &samsung_asoc_platform;

	if (platform->ops->close)
		return platform->ops->close(substream);
	else
		return 0;
}

static int s3c_wrpdma_ioctl(struct snd_pcm_substream *substream,
		unsigned int cmd, void *arg)
{
	struct snd_soc_platform_driver *platform;

#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		platform = &idma_soc_platform;
	else
#endif
		platform = &samsung_asoc_platform;

	if (platform->ops->ioctl)
		return platform->ops->ioctl(substream, cmd, arg);
	else
		return 0;
}

static int s3c_wrpdma_mmap(struct snd_pcm_substream *substream,
		struct vm_area_struct *vma)
{
	struct snd_soc_platform_driver *platform;

#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		platform = &idma_soc_platform;
	else
#endif
		platform = &samsung_asoc_platform;

	if (platform->ops->mmap)
		return platform->ops->mmap(substream, vma);
	else
		return 0;
}

static struct snd_pcm_ops s3c_wrpdma_ops = {
	.open		= s3c_wrpdma_open,
	.close		= s3c_wrpdma_close,
	.ioctl		= s3c_wrpdma_ioctl,
	.hw_params	= s3c_wrpdma_hw_params,
	.hw_free	= s3c_wrpdma_hw_free,
	.prepare	= s3c_wrpdma_prepare,
	.trigger	= s3c_wrpdma_trigger,
	.pointer	= s3c_wrpdma_pointer,
	.mmap		= s3c_wrpdma_mmap,
};

static void s3c_wrpdma_pcm_free(struct snd_pcm *pcm)
{
	struct snd_soc_platform_driver *gdma_platform;
#ifdef CONFIG_S5P_INTERNAL_DMA
	struct snd_soc_platform_driver *idma_platform;
#endif

#ifdef CONFIG_S5P_INTERNAL_DMA
	idma_platform = &idma_soc_platform;
	if (idma_platform->pcm_free)
		idma_platform->pcm_free(pcm);
#endif
	gdma_platform = &samsung_asoc_platform;
	if (gdma_platform->pcm_free)
		gdma_platform->pcm_free(pcm);
}

static int s3c_wrpdma_pcm_new(struct snd_card *card,
		struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
	struct snd_soc_platform_driver *gdma_platform;
#ifdef CONFIG_S5P_INTERNAL_DMA
	struct snd_soc_platform_driver *idma_platform;
#endif

	/* sec_fifo i/f always use internal h/w buffers
	 * irrespective of the xfer method (iDMA or SysDMA) */

#ifdef CONFIG_S5P_INTERNAL_DMA
	idma_platform = &idma_soc_platform;
	if (idma_platform->pcm_new)
		idma_platform->pcm_new(card, dai, pcm);
#endif
	gdma_platform  = &samsung_asoc_platform;
	if (gdma_platform->pcm_new)
		gdma_platform->pcm_new(card, dai, pcm);

	return 0;
}

static struct snd_soc_platform_driver s3c_dma_wrapper = {
	.ops		= &s3c_wrpdma_ops,
	.pcm_new	= s3c_wrpdma_pcm_new,
	.pcm_free	= s3c_wrpdma_pcm_free,
};
//EXPORT_SYMBOL_GPL(s3c_dma_wrapper);

static int __devinit s3c_dma_wrapper_platform_probe(struct platform_device *pdev)
{
	return snd_soc_register_platform(&pdev->dev, &s3c_dma_wrapper);
}

static int __devexit s3c_dma_wrapper_platform_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static struct platform_driver s3c_dma_wrapper_driver = {
	.driver = {
		.name = "samsung-audio",
		.owner = THIS_MODULE,
	},

	.probe = s3c_dma_wrapper_platform_probe,
	.remove = __devexit_p(s3c_dma_wrapper_platform_remove),
};

static int __init s3c_dma_wrapper_init(void)
{
	return platform_driver_register(&s3c_dma_wrapper_driver);
}
module_init(s3c_dma_wrapper_init);

static void __exit s3c_dma_wrapper_exit(void)
{
	platform_driver_unregister(&s3c_dma_wrapper_driver);
}
module_exit(s3c_dma_wrapper_exit);

MODULE_AUTHOR("Jaswinder Singh, <jassi.brar@samsung.com>");
MODULE_DESCRIPTION("Audio DMA wrapper module");
MODULE_LICENSE("GPL");
