//
//  MZCodec.cpp
//  MZAudioCodec
//
//  Created by mark.zhang on 25/01/2018.
//  Copyright © 2018 mark.zhang. All rights reserved.
//

#include "MZCodec.hpp"

#include "faac.h"
#include <stdio.h>

int codeWAV(const char *srcFilePath,  const char *destPath) {
    
    unsigned long nSampleRate   = 44100;//采样率
    unsigned int nChannels      = 2;//声道数
    unsigned int nPCMBitSize    = 16;//单样本位数
    
    unsigned long nInputSamples   = 0;
    unsigned long nMaxOutputBytes = 0;
    
    int nRet;
    faacEncHandle hEncoder;
    faacEncConfigurationPtr pConfiguration;
    
    size_t nBytesRead;
    unsigned long nPCMBufferSize;
    unsigned char *pbPCMBuffer;
    unsigned char *pbAACBuffer;
    
    FILE *fpIn; // WAV file for input
    FILE *fpOut; // AAC file for output
    
    /// 获取 faac 版本信息
    {
        char *version;
        char *copyright;
        faacEncGetVersion(&version, &copyright);
        printf("FAAC version: %s, copyright: %s", version, copyright);
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
