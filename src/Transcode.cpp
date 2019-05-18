/*
 * Transcode.cpp
 *
 *  Created on: May 11, 2019
 *      Author: fly
 */


#include <stdio.h>
#include "Transcode.h"
#include <string>

const int Transcode::kDefaultChannel = 2;
const int Transcode::kDefaultBitRate = 96000;
static int64_t pts = 0;

int Transcode::open_output_file(const std::string& filename,
		AVCodecContext* input_codec_context,
		AVFormatContext** output_format_context,
		AVCodecContext** output_codec_context,
		AVCodecID format,
		int bitRate,
	    int channels) {
	AVCodecContext *avctx          = NULL;
	AVIOContext *output_io_context = NULL;
	AVStream *stream               = NULL;
	AVCodec *output_codec          = NULL;
	int error;

	/* Open the output file to write to it. */
	if ((error = avio_open(&output_io_context, filename.c_str(),
						   AVIO_FLAG_WRITE)) < 0) {
		fprintf(stderr, "Could not open output file '%s' (error)\n",
				filename.c_str());
		return error;
	}

	/* Create a new format context for the output container format. */
	if (!(*output_format_context = avformat_alloc_context())) {
		fprintf(stderr, "Could not allocate output format context\n");
		return AVERROR(ENOMEM);
	}

	/* Associate the output file (pointer) with the container format context. */
	(*output_format_context)->pb = output_io_context;

	/* Guess the desired container format based on the file extension. */
	if (!((*output_format_context)->oformat = av_guess_format(NULL, filename.c_str(),
															  NULL))) {
		fprintf(stderr, "Could not find output file format\n");
		goto cleanup;
	}

	if (!((*output_format_context)->url = av_strdup(filename.c_str()))) {
		fprintf(stderr, "Could not allocate url.\n");
		error = AVERROR(ENOMEM);
		goto cleanup;
	}

	/* Find the encoder to be used by its name. */
	if (!(output_codec = avcodec_find_encoder(format))) {
		fprintf(stderr, "Could not find an encoder.\n");
		goto cleanup;
	}

	/* Create a new audio stream in the output file container. */
	if (!(stream = avformat_new_stream(*output_format_context, NULL))) {
		fprintf(stderr, "Could not create new stream\n");
		error = AVERROR(ENOMEM);
		goto cleanup;
	}

	avctx = avcodec_alloc_context3(output_codec);
	if (!avctx) {
		fprintf(stderr, "Could not allocate an encoding context\n");
		error = AVERROR(ENOMEM);
		goto cleanup;
	}

	/* Set the basic encoder parameters.
	 * The input file's sample rate is used to avoid a sample rate conversion. */
	avctx->channels       = channels;
	avctx->channel_layout = av_get_default_channel_layout(kDefaultChannel);
	avctx->sample_rate    = input_codec_context->sample_rate;
	avctx->sample_fmt     = output_codec->sample_fmts[0];
	avctx->bit_rate       = bitRate;

	/* Allow the use of the experimental AAC encoder. */
	avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	/* Set the sample rate for the container. */
	stream->time_base.den = input_codec_context->sample_rate;
	stream->time_base.num = 1;

	/* Some container formats (like MP4) require global headers to be present.
	 * Mark the encoder so that it behaves accordingly. */
	if ((*output_format_context)->oformat->flags & AVFMT_GLOBALHEADER)
		avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	/* Open the encoder for the audio stream to use it later. */
	if ((error = avcodec_open2(avctx, output_codec, NULL)) < 0) {
		fprintf(stderr, "Could not open output codec (error)\n");
		goto cleanup;
	}

	error = avcodec_parameters_from_context(stream->codecpar, avctx);
	if (error < 0) {
		fprintf(stderr, "Could not initialize stream parameters\n");
		goto cleanup;
	}

	/* Save the encoder context for easier access later. */
	*output_codec_context = avctx;

	return 0;

cleanup:
	avcodec_free_context(&avctx);
	avio_closep(&(*output_format_context)->pb);
	avformat_free_context(*output_format_context);
	*output_format_context = NULL;
	return error < 0 ? error : AVERROR_EXIT;
}

int Transcode::open_input_file(const std::string& filename,
		AVFormatContext** input_format_context,
		AVCodecContext** input_codec_context) {
    AVCodecContext *avctx = NULL;
    AVCodec *input_codec = NULL;
    int ret = 0;
    int errornum = 0;

    /* Open the input file to read from it. */
    if ((errornum = avformat_open_input(input_format_context, filename.c_str(), NULL,
                                     NULL)) < 0) {
        fprintf(stderr, "Could not open input file '%s' (error)\n",
                filename.c_str());
        *input_format_context = NULL;
        goto cleanup;
    }

    /* Get information on the input file (number of streams etc.). */
    if ((errornum = avformat_find_stream_info(*input_format_context, NULL)) < 0) {
        fprintf(stderr, "Could not open find stream info (error)\n");
        goto cleanup;
    }

    for(uint i = 0; i < (*input_format_context)->nb_streams; i++){
    	AVStream *stream = (*input_format_context)->streams[i];
    	if(stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
    		input_codec = avcodec_find_decoder(stream->codecpar->codec_id);
    		if(!input_codec){
    			fprintf(stderr, "Could not open find decoder (error)\n");
    			goto cleanup;
    		}

    		avctx = avcodec_alloc_context3(input_codec);
    		if (!avctx) {
    			fprintf(stderr, "Could not alloc context (error)\n");
    			goto cleanup;
    		}

    		ret = avcodec_parameters_to_context(avctx, stream->codecpar);
			if (ret < 0) {
				fprintf(stderr, "Failed to copy decoder parameters to input decoder context "
					   "for stream #%u\n", i);
				goto cleanup;
			}

			if ((ret = avcodec_open2(avctx, input_codec, NULL)) < 0) {
				goto cleanup;
			}

			*input_codec_context = avctx;
			break;
    	}
    }

    return 0;

cleanup:
	if(input_format_context){
		avformat_close_input(input_format_context);
		*input_format_context = NULL;
	}

	if(avctx){
		avcodec_free_context(&avctx);
	}

	return errornum;
}

void Transcode::init_packet(AVPacket* packet) {
	av_init_packet(packet);
	/* Set the packet data and size so that it is recognized as being empty. */
	packet->data = NULL;
	packet->size = 0;
}

int Transcode::init_input_frame(AVFrame** frame) {
	if (!(*frame = av_frame_alloc())) {
		fprintf(stderr, "Could not allocate input frame\n");
		return AVERROR(ENOMEM);
	}
	return 0;
}

int Transcode::init_resampler(AVCodecContext* input_codec_context,
		AVCodecContext* output_codec_context, SwrContext** resample_context) {
	int error;

	/*
	 * Create a resampler context for the conversion.
	 * Set the conversion parameters.
	 * Default channel layouts based on the number of channels
	 * are assumed for simplicity (they are sometimes not detected
	 * properly by the demuxer and/or decoder).
	 */
	*resample_context = swr_alloc_set_opts(NULL,
										  av_get_default_channel_layout(output_codec_context->channels),
										  output_codec_context->sample_fmt,
										  output_codec_context->sample_rate,
										  av_get_default_channel_layout(input_codec_context->channels),
										  input_codec_context->sample_fmt,
										  input_codec_context->sample_rate,
										  0, NULL);
	if (!*resample_context) {
		fprintf(stderr, "Could not allocate resample context\n");
		return AVERROR(ENOMEM);
	}
	/*
	* Perform a sanity check so that the number of converted samples is
	* not greater than the number of samples to be converted.
	* If the sample rates differ, this case has to be handled differently
	*/
	av_assert0(output_codec_context->sample_rate <= input_codec_context->sample_rate);

	/* Open the resampler with the specified parameters. */
	if ((error = swr_init(*resample_context)) < 0) {
		fprintf(stderr, "Could not open resample context\n");
		swr_free(resample_context);
		return error;
	}
    return 0;
}

int Transcode::init_fifo(AVAudioFifo** fifo,
		AVCodecContext* output_codec_context) {
    /* Create the FIFO buffer based on the specified output sample format. */
    if (!(*fifo = av_audio_fifo_alloc(output_codec_context->sample_fmt,
                                      output_codec_context->channels, 1))) {
        fprintf(stderr, "Could not allocate FIFO\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}

int Transcode::write_output_file_header(
		AVFormatContext* output_format_context) {
    int error;
    if ((error = avformat_write_header(output_format_context, NULL)) < 0) {
        fprintf(stderr, "Could not write output file header (error)\n");
        return error;
    }
    return 0;
}

int Transcode::decode_audio_frame(AVFrame* frame,
		AVFormatContext* input_format_context,
		AVCodecContext* input_codec_context, int* data_present, int* finished) {
    AVPacket input_packet;
	int error;
	init_packet(&input_packet);

	/* Read one audio frame from the input file into a temporary packet. */
	if ((error = av_read_frame(input_format_context, &input_packet)) < 0) {
		/* If we are at the end of the file, flush the decoder below. */
		if (error == AVERROR_EOF)
			*finished = 1;
		else {
			fprintf(stderr, "Could not read frame (error )\n");
			return error;
		}
	}

	/* Send the audio frame stored in the temporary packet to the decoder.
	 * The input audio stream decoder is used to do this. */
	if ((error = avcodec_send_packet(input_codec_context, &input_packet)) < 0) {
		fprintf(stderr, "Could not send packet for decoding (error)\n");
		return error;
	}

	/* Receive one frame from the decoder. */
	error = avcodec_receive_frame(input_codec_context, frame);
	/* If the decoder asks for more data to be able to decode a frame,
	 * return indicating that no data is present. */
	if (error == AVERROR(EAGAIN)) {
		error = 0;
		goto cleanup;
	/* If the end of the input file is reached, stop decoding. */
	} else if (error == AVERROR_EOF) {
		*finished = 1;
		error = 0;
		goto cleanup;
	} else if (error < 0) {
		fprintf(stderr, "Could not decode frame (error)\n");
		goto cleanup;
	/* Default case: Return decoded data. */
	} else {
		*data_present = 1;
		goto cleanup;
	}

cleanup:
	av_packet_unref(&input_packet);
	return error;
}

int Transcode::init_converted_samples(uint8_t*** converted_input_samples,
		AVCodecContext* output_codec_context, int frame_size) {
    int error;

    /* Allocate as many pointers as there are audio channels.
     * Each pointer will later point to the audio samples of the corresponding
     * channels (although it may be NULL for interleaved formats).
     */
    if (!(*converted_input_samples = static_cast<uint8_t**> (calloc(output_codec_context->channels,
                                            sizeof(**converted_input_samples))))) {
        fprintf(stderr, "Could not allocate converted input sample pointers\n");
        return AVERROR(ENOMEM);
    }

    /* Allocate memory for the samples of all channels in one consecutive
     * block for convenience. */
    if ((error = av_samples_alloc(*converted_input_samples, NULL,
                                  output_codec_context->channels,
                                  frame_size,
                                  output_codec_context->sample_fmt, 0)) < 0) {
        fprintf(stderr,
                "Could not allocate converted input samples (error)\n");
        av_freep(&(*converted_input_samples)[0]);
        free(*converted_input_samples);
        return error;
    }
    return 0;
}

int Transcode::convert_samples(const uint8_t** input_data,
		uint8_t** converted_data, const int frame_size,
		SwrContext* resample_context) {
    int error;

    /* Convert the samples using the resampler. */
    if ((error = swr_convert(resample_context,
                             converted_data, frame_size,
                             input_data    , frame_size)) < 0) {
        fprintf(stderr, "Could not convert input samples (error )\n");
        return error;
    }

    return 0;
}

int Transcode::add_samples_to_fifo(AVAudioFifo* fifo,
		uint8_t** converted_input_samples, const int frame_size) {
    int error;

    /* Make the FIFO as large as it needs to be to hold both,
     * the old and the new samples. */
    if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size)) < 0) {
        fprintf(stderr, "Could not reallocate FIFO\n");
        return error;
    }

    /* Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(fifo, (void **)converted_input_samples,
                            frame_size) < frame_size) {
        fprintf(stderr, "Could not write data to FIFO\n");
        return AVERROR_EXIT;
    }
    return 0;
}

int Transcode::read_decode_convert_and_store(AVAudioFifo* fifo,
		AVFormatContext* input_format_context,
		AVCodecContext* input_codec_context,
		AVCodecContext* output_codec_context, SwrContext* resampler_context,
		int* finished) {
    /* Temporary storage of the input samples of the frame read from the file. */
    AVFrame *input_frame = NULL;
    /* Temporary storage for the converted input samples. */
    uint8_t **converted_input_samples = NULL;
    int data_present = 0;
    int ret = AVERROR_EXIT;

    /* Initialize temporary storage for one input frame. */
    if (init_input_frame(&input_frame))
        goto cleanup;
    /* Decode one frame worth of audio samples. */
    if (decode_audio_frame(input_frame, input_format_context,
                           input_codec_context, &data_present, finished))
        goto cleanup;
    /* If we are at the end of the file and there are no more samples
     * in the decoder which are delayed, we are actually finished.
     * This must not be treated as an error. */
    if (*finished) {
        ret = 0;
        goto cleanup;
    }
    /* If there is decoded data, convert and store it. */
    if (data_present) {
        /* Initialize the temporary storage for the converted input samples. */
        if (init_converted_samples(&converted_input_samples, output_codec_context,
                                   input_frame->nb_samples))
            goto cleanup;

        /* Convert the input samples to the desired output sample format.
         * This requires a temporary storage provided by converted_input_samples. */
        if (convert_samples((const uint8_t**)input_frame->extended_data, converted_input_samples,
                            input_frame->nb_samples, resampler_context))
            goto cleanup;

        /* Add the converted input samples to the FIFO buffer for later processing. */
        if (add_samples_to_fifo(fifo, converted_input_samples,
                                input_frame->nb_samples))
            goto cleanup;
        ret = 0;
    }
    ret = 0;

cleanup:
    if (converted_input_samples) {
        av_freep(&converted_input_samples[0]);
        free(converted_input_samples);
    }
    av_frame_free(&input_frame);

    return ret;
}

int Transcode::init_output_frame(AVFrame** frame,
		AVCodecContext* output_codec_context, int frame_size) {
    int error;

    /* Create a new frame to store the audio samples. */
    if (!(*frame = av_frame_alloc())) {
        fprintf(stderr, "Could not allocate output frame\n");
        return AVERROR_EXIT;
    }

    /* Set the frame's parameters, especially its size and format.
     * av_frame_get_buffer needs this to allocate memory for the
     * audio samples of the frame.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity. */
    (*frame)->nb_samples     = frame_size;
    (*frame)->channel_layout = output_codec_context->channel_layout;
    (*frame)->format         = output_codec_context->sample_fmt;
    (*frame)->sample_rate    = output_codec_context->sample_rate;

    /* Allocate the samples of the created frame. This call will make
     * sure that the audio frame can hold as many samples as specified. */
    if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
        fprintf(stderr, "Could not allocate output frame samples (error)\n");
        av_frame_free(frame);
        return error;
    }

    return 0;
}

int Transcode::encode_audio_frame(AVFrame* frame,
		AVFormatContext* output_format_context,
		AVCodecContext* output_codec_context, int* data_present) {
    /* Packet used for temporary storage. */
    AVPacket output_packet;
    int error;
    init_packet(&output_packet);

    /* Set a timestamp based on the sample rate for the container. */
    if (frame) {
        frame->pts = pts;
        pts += frame->nb_samples;
    }

    /* Send the audio frame stored in the temporary packet to the encoder.
     * The output audio stream encoder is used to do this. */
    error = avcodec_send_frame(output_codec_context, frame);
    /* The encoder signals that it has nothing more to encode. */
    if (error == AVERROR_EOF) {
        error = 0;
        goto cleanup;
    } else if (error < 0) {
        fprintf(stderr, "Could not send packet for encoding (error)\n");
        return error;
    }

    /* Receive one encoded frame from the encoder. */
    error = avcodec_receive_packet(output_codec_context, &output_packet);
    /* If the encoder asks for more data to be able to provide an
     * encoded frame, return indicating that no data is present. */
    if (error == AVERROR(EAGAIN)) {
        error = 0;
        goto cleanup;
    /* If the last frame has been encoded, stop encoding. */
    } else if (error == AVERROR_EOF) {
        error = 0;
        goto cleanup;
    } else if (error < 0) {
        fprintf(stderr, "Could not encode frame (error)\n");
        goto cleanup;
    /* Default case: Return encoded data. */
    } else {
        *data_present = 1;
    }

    /* Write one audio frame from the temporary packet to the output file. */
    if (*data_present &&
        (error = av_write_frame(output_format_context, &output_packet)) < 0) {
        fprintf(stderr, "Could not write frame (error )\n");
        goto cleanup;
    }

cleanup:
    av_packet_unref(&output_packet);
    return error;
}

int Transcode::load_encode_and_write(AVAudioFifo* fifo,
		AVFormatContext* output_format_context,
		AVCodecContext* output_codec_context) {
   /* Temporary storage of the output samples of the frame written to the file. */
	AVFrame *output_frame;
	/* Use the maximum number of possible samples per frame.
	 * If there is less than the maximum possible frame size in the FIFO
	 * buffer use this number. Otherwise, use the maximum possible frame size. */
	const int frame_size = FFMIN(av_audio_fifo_size(fifo),
								 output_codec_context->frame_size);
	int data_written;
	int ret = 0;
	/* Initialize temporary storage for one output frame. */
	if (init_output_frame(&output_frame, output_codec_context, frame_size))
		return AVERROR_EXIT;

	/* Read as many samples from the FIFO buffer as required to fill the frame.
	 * The samples are stored in the frame temporarily. */
	if (av_audio_fifo_read(fifo, (void **)output_frame->data, frame_size) < frame_size) {
		fprintf(stderr, "Could not read data from FIFO\n");
		ret = AVERROR_EXIT;
		goto cleanup;
	}

	/* Encode one frame worth of audio samples. */
	if (encode_audio_frame(output_frame, output_format_context,
						   output_codec_context, &data_written)) {
		ret = AVERROR_EXIT;
		goto cleanup;
	}

cleanup:
	av_frame_free(&output_frame);
	return ret;
}

int Transcode::write_output_file_trailer(
		AVFormatContext* output_format_context) {
    int error;
    if ((error = av_write_trailer(output_format_context)) < 0) {
        fprintf(stderr, "Could not write output file trailer (error)\n");
        return error;
    }
    return 0;
}
