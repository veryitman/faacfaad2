//
//  MZCodec.cpp
//  MZAudioCodec
//
//  Created by mark.zhang on 25/01/2018.
//  Copyright © 2018 mark.zhang. All rights reserved.
//

#include "MZCodec.hpp"

#include "faac.h"
#include "faad.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//http://blog.csdn.net/gavinr/article/details/6959198
//http://blog.csdn.net/guofengpu/article/details/52026791
//http://blog.51cto.com/13136504/2056948

struct MZWavAudioFileHeader
{
    char       riff[4];       // 字符串 "RIFF"
    uint32_t   totalLength; // 文件总大小, 包括PCM 数据大小和该文件头大小
    char       wave[4];     // 字符串 "WAVE"
    char       fmt[4];      // 字符串 "fmt "
    uint32_t   format;      // WAV 头大小, 固定为值 16
    uint16_t   pcm;         // PCM 编码方式, 固定值为 1
    uint16_t   channels;    // 声道数量, 为 2
    uint32_t   frequency;   // 采样频率
    uint32_t   bytes_per_second; // 每秒字节数(码率), 其值=采样率x通道数x位深度/8
    uint16_t   bytes_by_capture; // 采样块大小
    uint16_t   bits_per_sample; // 采样点大小, 这里是 16 位
    char       data[4];         // 字符串 "data"
    uint32_t   bytes_in_pcmdata;  // pcm 数据长度
};

int codeWAV(const char *srcFilePath, const char *destPath) {
    
    unsigned long nSampleRate   = 44100; //采样率
    unsigned int nChannels      = 2; //声道数
    unsigned int nPCMBitSize    = 16; //单样本位数
    
    unsigned long nInputSamples   = 0;
    unsigned long nMaxOutputBytes = 0;
    
    int nRet;
    faacEncHandle hEncoder;
    faacEncConfigurationPtr pConfiguration;
    
    size_t nBytesRead;
    unsigned long nPCMBufferSize;
    unsigned char *pbPCMBuffer;
    unsigned char *pbAACBuffer;
    
    FILE *fpIn; // 输入 WAV 文件
    FILE *fpOut; // 输出 AAC 文件
    
    /// 获取 faac 版本信息
    {
        char *version;
        char *copyright;
        faacEncGetVersion(&version, &copyright);
        printf("FAAC version: %s, copyright: %s\n", version, copyright);
    }
    
    fpIn = fopen(srcFilePath, "rb");
    
    if (NULL == fpIn) {
        return -2;
    }
    
    fpOut = fopen(destPath, "wb");
    
    /// 1. 打开 FAAC
    hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples, &nMaxOutputBytes);
    
    if (NULL == hEncoder) {
        
        printf("[ERROR] Failed to call faacEncOpen()\n");
        return -1;
    }
    
    nPCMBufferSize = nInputSamples * nPCMBitSize / 8;
    pbPCMBuffer = new unsigned char[nPCMBufferSize];
    pbAACBuffer = new unsigned char[nMaxOutputBytes];
    
    /// 2.1. 获取当前的编码器配置
    pConfiguration = faacEncGetCurrentConfiguration(hEncoder);
    
    pConfiguration->inputFormat = FAAC_INPUT_16BIT;
    // 对象类型只有为 LOW, iOS 的 AVAudioPlayer 才能播放
    pConfiguration->aacObjectType = LOW;
    // 0 = Raw; 1 = ADTS
    pConfiguration->outputFormat = 1;
    pConfiguration->mpegVersion = MPEG4;
    pConfiguration->useTns = 1;
    pConfiguration->bitRate = 30;
    
    /// 2.2. 配置编码器
    nRet = faacEncSetConfiguration(hEncoder, pConfiguration);
    
    //是wav格式, 先读取前面的
    fseek(fpIn, 58, SEEK_SET);
    
    do {
        
        //读入的实际字节数，最大不会超过 nPCMBufferSize
        nBytesRead = fread(pbPCMBuffer, 1, nPCMBufferSize, fpIn);
        
        //输入样本数，用实际读入字节数计算
        //一般只有读到文件尾时才不是 nPCMBufferSize/(nPCMBitSize/8)
        nInputSamples = nBytesRead / (nPCMBitSize / 8);
        
        /// 3. 编码
        nRet = faacEncEncode(hEncoder,
                             (int *)pbPCMBuffer,
                             (unsigned int)nInputSamples,
                             pbAACBuffer,
                             (unsigned int)nMaxOutputBytes);
        
        fwrite(pbAACBuffer, 1, nRet, fpOut);
        
        printf("FaacEncEncode returns %d\n", nRet);
        
    } while (nBytesRead > 0);
    
    /// 4. 关闭 FAAC
    nRet = faacEncClose(hEncoder);
    
    delete[] pbPCMBuffer;
    delete[] pbAACBuffer;
    
    fclose(fpIn);
    fclose(fpOut);
    
    return 0;
}


/**
 * 写入 wav 头数据.
 *
 *  @param file  wav 文件指针.
 *  @param total_samples_per_channel 每个声道的采样数.
 *  @param samplerate 采样率.
 *  @param channels 声道数.
 */
void mz_write_wav_header(FILE *file, int total_samples_per_channel, int samplerate, int channels) {
    
    if (NULL == file) {
        return;
    }
    
    if (total_samples_per_channel <= 0) {
        return;
    }
    
    printf("FAAD. total_samples_per_channel: %i, samplerate: %i, channels: %i\n",
           total_samples_per_channel, samplerate, channels);
    
    struct MZWavAudioFileHeader wavHeader;
    
    // 写入 RIFF
    strcpy(wavHeader.riff, "RIFF");
    
    wavHeader.bits_per_sample = 16;
    
    wavHeader.totalLength = (total_samples_per_channel * channels * wavHeader.bits_per_sample/8) + sizeof(wavHeader) - 8;
    
    // 写入 WAVE 和 fmt
    strcpy(wavHeader.wave, "WAVE");
    strcpy(wavHeader.fmt, "fmt ");
    
    wavHeader.format = 16;
    wavHeader.pcm = 1;
    wavHeader.channels = channels;
    wavHeader.frequency = samplerate;
    
    // 每秒的字节数(码率)=采样率x通道数x位深度/8
    wavHeader.bytes_per_second = wavHeader.channels * wavHeader.frequency * wavHeader.bits_per_sample/8;
    
    wavHeader.bytes_by_capture = wavHeader.channels*wavHeader.bits_per_sample/8;
    
    wavHeader.bytes_in_pcmdata = total_samples_per_channel * wavHeader.channels * wavHeader.bits_per_sample/8;
    
    // 写入 data
    strcpy(wavHeader.data, "data");
    
    fwrite(&wavHeader, 1, sizeof(wavHeader), file);
}

int decodeAAC(const char *aacfile, const char *wavename) {
    
    // 获取句柄
    NeAACDecHandle faadhandle = NeAACDecOpen();
    
    if (faadhandle) {
    
        FILE *inputFile = fopen(aacfile, "rb");
        
        if (NULL == inputFile) {
            return -2;
        }
        
        // 移动文件的读写指针到文件末尾
        fseek(inputFile, 0, SEEK_END);
        // 获取文件长度
        long filelen = ftell(inputFile);
        // 重新移动到文件开头
        fseek(inputFile, 0, SEEK_SET);
        
        unsigned char *filebuf = (unsigned char *)malloc(filelen);
        size_t len = fread(filebuf, 1, filelen, inputFile);
        fclose(inputFile);
        
        unsigned long samplerate = 0;
        unsigned char channel = 0;
        
        // 初始化解码器
        long ret = NeAACDecInit(faadhandle, filebuf, len, &samplerate, &channel);
        if (ret >= 0) {
            
            printf("FAAD. aac init: sam = %lu, chn = %d\n", samplerate, channel);
            
            NeAACDecFrameInfo frameinfo;
            unsigned char *curbyte = filebuf;
            unsigned long leftsize = len;
            
            FILE *outputFile = fopen(wavename, "wb");
            if (NULL == outputFile) {
                return -3;
            }

            int wavheadsize = sizeof(struct MZWavAudioFileHeader);
            printf("FAAD. The WAV file header size: %i\n", wavheadsize);
            fseek(outputFile, wavheadsize, SEEK_SET);
            
            int totalsmp_per_chl = 0;
            void *out = NULL;
            
            while (1) {
                
                // 解码(一帧音频)
                out = NeAACDecDecode(faadhandle, &frameinfo, curbyte, leftsize);
                
                if (NULL == out) {
                    break;
                }
                
                printf("FAAD. decode a frame: samplerate:%ld, \
                       channels=%d, \
                       samplecount=%ld, \
                       obj_type=%d, \
                       header_type=%d, \
                       bytesconsumed=%ld\n",
                       frameinfo.samplerate,
                       frameinfo.channels,
                       frameinfo.samples,
                       frameinfo.object_type,
                       frameinfo.header_type,
                       frameinfo.bytesconsumed);
                
                curbyte += frameinfo.bytesconsumed;
                leftsize -= frameinfo.bytesconsumed;
                
                fwrite(out, 1, frameinfo.samples*2, outputFile);
                
                // 每个声道的采样总数
                totalsmp_per_chl += frameinfo.samples / frameinfo.channels;
            }
            
            fseek(outputFile, 0, SEEK_SET);
            
            mz_write_wav_header(outputFile, totalsmp_per_chl, (int)samplerate, (int)channel);
            
            fclose(outputFile);
        }
        
        free(filebuf);
        
        // 关闭句柄
        NeAACDecClose(faadhandle);
    }
    
    return 0;
}
