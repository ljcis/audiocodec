/*
 * AudioConvert.cpp
 *
 *  Created on: May 10, 2019
 *      Author: fly
 */

#include "AudioConvert.h"
#include "Transcode.h"
#include <stdio.h>


AudioConvert::AudioConvert(const std::string& inputFile,
		const std::string& outputFile,
		AudioFormat format,
		BitRate outputBitRate,
		int outputSampleChannel) :
									outputFile_(outputFile),
									inputFormatContext_(NULL),
									outputFormatContext_(NULL),
									inputCodecContext_(NULL),
									outputCodecContext_(NULL),
									resampleContext_(NULL),
									fifo_(NULL),
									outputBitRate_(int(outputBitRate)),
									outputSampleChannel_(outputSampleChannel)
{
	Transcode::open_input_file(inputFile,
					&inputFormatContext_,
					&inputCodecContext_);

	if(getInputSampleRate() <= outputBitRate_){
		outputBitRate_ = getInputSampleRate();
	}
	Transcode::open_output_file(outputFile,
					inputCodecContext_,
					&outputFormatContext_,
					&outputCodecContext_,
					AVCodecID(format),
					outputBitRate_,
					outputSampleChannel_);

	Transcode::init_resampler(inputCodecContext_, outputCodecContext_, &resampleContext_);

	Transcode::init_fifo(&fifo_, outputCodecContext_);

	Transcode::write_output_file_header(outputFormatContext_);
}

AudioConvert::~AudioConvert() {
	// TODO Auto-generated destructor stub
    if (fifo_)
        av_audio_fifo_free(fifo_);
    swr_free(&resampleContext_);
    if (outputCodecContext_)
        avcodec_free_context(&outputCodecContext_);
    if (outputFormatContext_) {
        avio_closep(&outputFormatContext_->pb);
        avformat_free_context(outputFormatContext_);
    }
    if (inputCodecContext_)
        avcodec_free_context(&inputCodecContext_);
    if (inputFormatContext_)
        avformat_close_input(&inputFormatContext_);
}

int AudioConvert::convert(void* handle, P_ProcessCallback cb) {
	int frameNum = inputCodecContext_->frame_number;
	while (1) {
		/* Use the encoder's desired frame size for processing. */
		const int output_frame_size = outputCodecContext_->frame_size;
		int finished                = 0;
		int frameCount				= 0;
		/* Make sure that there is one frame worth of samples in the FIFO
		 * buffer so that the encoder can do its work.
		 * Since the decoder's and the encoder's frame size may differ, we
		 * need to FIFO buffer to store as many frames worth of input samples
		 * that they make up at least one frame worth of output samples. */
		while (av_audio_fifo_size(fifo_) < output_frame_size) {
			/* Decode one frame worth of audio samples, convert it to the
			 * output sample format and put it into the FIFO buffer. */
			if (Transcode::read_decode_convert_and_store(fifo_, inputFormatContext_,
											  inputCodecContext_,
											  outputCodecContext_,
											  resampleContext_, &finished))
			{
				continue;
			}else{
				frameCount ++;
				if(cb){
					cb(handle, frameCount*100/frameNum);
				}
//				return -1;
			}
			/* If we are at the end of the input file, we continue
			 * encoding the remaining audio samples to the output file. */
			if (finished)
				break;
		}

		/* If we have enough samples for the encoder, we encode them.
		 * At the end of the file, we pass the remaining samples to
		 * the encoder. */
		while (av_audio_fifo_size(fifo_) >= output_frame_size ||
			   (finished && av_audio_fifo_size(fifo_) > 0))
			/* Take one frame worth of audio samples from the FIFO buffer,
			 * encode it and write it to the output file. */
			if (Transcode::load_encode_and_write(fifo_, outputFormatContext_,
										outputCodecContext_))
				return -1;

		/* If we are at the end of the input file and have encoded
		 * all remaining samples, we can exit this loop and finish. */
		if (finished) {
			int data_written;
			/* Flush the encoder as it may have delayed frames. */
			do {
				data_written = 0;
				if (Transcode::encode_audio_frame(NULL, outputFormatContext_,
						outputCodecContext_, &data_written))
					return -1;
			} while (data_written);
			break;
		}
	}

	/* Write the trailer of the output file container. */
	if (Transcode::write_output_file_trailer(outputFormatContext_))
		return -1;
	return  0;
}

const std::string AudioConvert::getDefaultOutputFile(
		const std::string& inputFile, AudioFormat format) {
	size_t pos = inputFile.find(".");
	if(pos != std::string::npos){
		std::string fileName = inputFile.substr(0, pos);
		std::string formatName;
		switch(format){
		case FORMAT_ID_MP3:
			formatName = ".mp3";
			break;
		case FORMAT_ID_AAC:
			formatName = ".aac";
			break;
		case FORMAT_ID_FLAC:
			formatName = ".flac";
			break;
		case FORMAT_ID_WAV:
			formatName = ".wav";
			break;
		default:
			break;
		}

		return fileName + formatName;
	}
	return inputFile + "_1";
}
