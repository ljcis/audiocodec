/*
 * AudioConvert.h
 *
 *  Created on: May 10, 2019
 *      Author: fly
 */

#ifndef AUDIOCONVERT_H_
#define AUDIOCONVERT_H_
#include <string>
#include "ffmpegInc.h"

enum AudioFormat{
	FORMAT_ID_MP3 = AV_CODEC_ID_MP3,
	FORMAT_ID_AAC = AV_CODEC_ID_AAC,
	FORMAT_ID_FLAC = AV_CODEC_ID_FLAC,
	FORMAT_ID_WAV = AV_CODEC_ID_WAVPACK
};

enum BitRate{
	BIT_RATE_16K = 16000,
	BIT_RATE_32K = 32000,
	BIT_RATE_64K = 64000,
	BIT_RATE_96K = 96000,
	BIT_RATE_128K = 128000,
	BIT_RATE_256K = 256000
};
struct AVFormatContext;
struct AVCodecContext;
struct SwrContext;
struct AVAudioFifo;

typedef void (*P_ProcessCallback)(void* handle, int process); // 界面进度回调

class AudioConvert {
public:
	AudioConvert(const std::string& inputFile,
				const std::string& outputFile,
				AudioFormat outputFormat,
				BitRate outputSampleRate,
				int outputSampleChannel);

	virtual ~AudioConvert();
	int convert(void* handle, P_ProcessCallback cb);
	static const std::string getDefaultOutputFile(const std::string& inputFile, AudioFormat format);
	int getInputSampleRate() const {
		if(inputCodecContext_)
			return inputCodecContext_->bit_rate;
		else
			return 0;
	}
	int getInputChannels() const{
		if(inputCodecContext_)
			return inputCodecContext_->channels;
		else
			return 0;
	}

private:

	std::string 		outputFile_;
    AVFormatContext* 	inputFormatContext_;
    AVFormatContext*	outputFormatContext_;
    AVCodecContext* 	inputCodecContext_;
    AVCodecContext*		outputCodecContext_;
    SwrContext*			resampleContext_;
    AVAudioFifo*		fifo_;
    int 				outputBitRate_;
    int 				outputSampleChannel_;
};

#endif /* AUDIOCONVERT_H_ */
