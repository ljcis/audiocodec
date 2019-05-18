/*
 * ffmpegInc.h
 *
 *  Created on: May 11, 2019
 *      Author: fly
 */

#ifndef FFMPEGINC_H_
#define FFMPEGINC_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavformat/avio.h"

#include "libavcodec/avcodec.h"

#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"

#include "libswresample/swresample.h"


#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/pixdesc.h>
#ifdef __cplusplus
}
#endif



#endif /* FFMPEGINC_H_ */
