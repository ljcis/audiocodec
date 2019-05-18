/*
 * Transcode.h
 *
 *  Created on: May 11, 2019
 *      Author: fly
 */

#ifndef TRANSCODE_H_
#define TRANSCODE_H_

#include "ffmpegInc.h"
#include <string>


class Transcode{
	static const int kDefaultChannel;
	static const int kDefaultBitRate;
public:
	static int open_output_file(const std::string& filename,
						 AVCodecContext *input_codec_context,
						 AVFormatContext **output_format_context,
						 AVCodecContext **output_codec_context,
						 AVCodecID format,
						 int bitRate,
						 int channels);

	static int open_input_file(const std::string& filename,
					    AVFormatContext **input_format_context,
					    AVCodecContext **input_codec_context);

	static void init_packet(AVPacket *packet);

	static int init_input_frame(AVFrame **frame);

	static int init_resampler(AVCodecContext *input_codec_context,
					   AVCodecContext *output_codec_context,
					   SwrContext **resample_context);

	static int init_fifo(AVAudioFifo **fifo, AVCodecContext *output_codec_context);

	static int write_output_file_header(AVFormatContext *output_format_context);

	static int decode_audio_frame(AVFrame *frame,
						   AVFormatContext *input_format_context,
						   AVCodecContext *input_codec_context,
						   int *data_present, int *finished);

	static int init_converted_samples(uint8_t ***converted_input_samples,
							   AVCodecContext *output_codec_context,
							   int frame_size);

	static int convert_samples(const uint8_t **input_data,
					    uint8_t **converted_data, const int frame_size,
					    SwrContext *resample_context);

	static int add_samples_to_fifo(AVAudioFifo *fifo,
						    uint8_t **converted_input_samples,
						    const int frame_size);

	static int read_decode_convert_and_store(AVAudioFifo *fifo,
									  AVFormatContext *input_format_context,
									  AVCodecContext *input_codec_context,
									  AVCodecContext *output_codec_context,
									  SwrContext *resampler_context,
									  int *finished);

	static int init_output_frame(AVFrame **frame,
						  AVCodecContext *output_codec_context,
						  int frame_size);

	static int encode_audio_frame(AVFrame *frame,
						   AVFormatContext *output_format_context,
						   AVCodecContext *output_codec_context,
						   int *data_present);

	static int load_encode_and_write(AVAudioFifo *fifo,
							  AVFormatContext *output_format_context,
							  AVCodecContext *output_codec_context);

	static int write_output_file_trailer(AVFormatContext *output_format_context);
};

#endif /* TRANSCODE_H_ */
