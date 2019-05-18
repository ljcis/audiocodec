# audiocodec
### 音频文件的转码
所谓转码，就是将音频文件解码，再使用相应的编码器重新编码并写入文件的过程。参考FFmpeg提供的例子，给出了一个将其他类型的音频文件转码为aac的样例。  
将样例跑起来后，遇到了一些问题：
- 将编码的格式改成MP3，就无法编码了
- 用一个电脑中的MP3，在运行中直接崩溃了

> **问题原因：**  
MP3格式由于版权的原因，FFmpeg只支持解码，却不支持编码，需要支持编码就要使用第三方的MP3的编码库，常用的就是libmp3lame，自己在网上一搜就找得到。  

##### FFmpeg支持MP3编码
查看FFmpeg支持的所有的编解码类型可以用自有的命令：
```
ffmpeg -codecs
```
得到的结果如下：
```
 D..... = Decoding supported
 .E.... = Encoding supported
 ..V... = Video codec
 ..A... = Audio codec
 ..S... = Subtitle codec
 ...I.. = Intra frame-only codec
 ....L. = Lossy compression
 .....S = Lossless compression
 -------
 D.VI.S 012v                 Uncompressed 4:2:2 10-bit
 D.V.L. 4xm                  4X Movie
 D.VI.S 8bps                 QuickTime 8BPS video
 .EVIL. a64_multi            Multicolor charset for Commodore 64 (encoders: a64multi )
 .EVIL. a64_multi5           Multicolor charset for Commodore 64, extended with 5th color (colram) (encoders: a64multi5 )
 D.V..S aasc                 Autodesk RLE
 D.V.L. agm                  Amuse Graphics Movie
 D.VIL. aic                  Apple Intermediate Codec
 DEVI.S alias_pix            Alias/Wavefront PIX image
 DEVIL. amv                  AMV Video
 D.V.L. anm                  Deluxe Paint Animation
 D.V.L. ansi                 ASCII/ANSI art

```
在此时，MP3格式是不支持编码的，只支持解码，在安装了libmp3lame后，重新编译FFmpeg，并用下面的configure：
```
./configure --disable-x86asm --enable-libmp3lame
```
然后再查看，可见支持了MP3编码，且使用的编码器是libmp3lame：
```
DEA.L. mp3                  MP3 (MPEG audio layer 3) (decoders: mp3float mp3 ) (encoders: libmp3lame )
```
此时已经可以将其他的音频格式用MP3重新编码并输出，但之前的MP3文件转AAC一样的崩溃。  

> **问题原因：**  
普通的AAC文件中只有一个媒体流，而MP3文件可以有两个媒体流，其中一个是普通的MP3类型，另一个是AVMEDIA_TYPE_VIDEO类型，codecID为AV_CODEC_ID_MJPEG。  

#### 设计
在编码和解码过程中，主要需要涉及到以下几个结构：
- AVFormatContext：文件格式的上下文信息
- AVCodecContext ：文件编码或解码的上下文信息
- SwrContext ： 转码的上下文信息
- AVAudioFifo ：处理队列

```
graph TB
A(初始化输入文件的上下文信息)
B(初始化输出文件的上下文信息)
C(初始化转码上下文)
D(初始化队列)
E(按frame转码并写入文件)

A-->B
B-->C
C-->D
D-->E
```

##### 打开输入上下文
这里只选择获取文件的多个流中的音频流的信息，暂时不考虑一个文件中包含多个音频流的情况。
```c++
//<summary></summary>
int openInputFile(const std::string& filePath, 
                  AVFormatContext** inputFormatContext,
                  AVCodecContext** inputCodecContext)
```
在这个接口中主要需要涉及以下几个FFmpeg提供的接口：
- avformat_open_input ： 打开输入文件的AVFormatContext
- avformat_find_stream_info：获取音频中的流信息
- avcodec_find_decoder ： 获取解码器
- avcodec_alloc_context3 ： 申请输入文件的AVCodecContext的内存
- avcodec_parameters_to_context：将参数传给AVCodecContext
- avformat_close_input：当失败时，清理AVFormatContext
- avcodec_free_context：失败时清理AVCodecContext
##### 打开输出上下文
```c++
int openOutputFile(const std::string& filename,
            		AVCodecContext* input_codec_context,
            		AVFormatContext** output_format_context,
            		AVCodecContext** output_codec_context,
            		AVCodecID format,
            		int bitRate,
            	    int channels)
```
