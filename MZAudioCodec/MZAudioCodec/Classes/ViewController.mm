//
//  ViewController.mm
//  MZAudioCodec
//
//  Created by mark.zhang on 22/01/2018.
//  Copyright © 2018 mark.zhang. All rights reserved.
//

#import "ViewController.h"
#import "MZCodec.hpp"
#import <AVFoundation/AVFoundation.h>

@interface ViewController ()

@property (strong, nonatomic) IBOutlet UITextView *disAearText;

@property (nonatomic, strong) AVAudioPlayer *audioPlayer;

@property (weak, nonatomic) IBOutlet UIButton *covertBtn;

@property (weak, nonatomic) IBOutlet UIButton *playBtn;

@property (weak, nonatomic) IBOutlet UIButton *covertWAVToAACBtn;

@property (weak, nonatomic) IBOutlet UIButton *playWAVBtn;

@property (nonatomic, copy) NSString *destFilePath;

@end


@implementation ViewController

- (void)viewDidLoad {
    
    [super viewDidLoad];
    
    self.navigationItem.title = @"音频编解码";
    
    self.disAearText.text = @"";
    self.playBtn.enabled = NO;
    self.playBtn.alpha = 0.7f;
    self.playWAVBtn.enabled = NO;
    self.playWAVBtn.alpha = 0.7f;
}

- (IBAction)doWAV2AACAction:(id)sender {
    
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *resPath = [bundle pathForResource:@"m" ofType:@"wav"];
    NSLog(@"The path of wav file: %@", resPath);
    
    NSArray<NSString *> *docPath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *destPath = [[docPath lastObject] stringByAppendingString:@"/out.aac"];
    NSLog(@"The path of aac file: %@", destPath);
    
    self.disAearText.text = @"正在转换...";
    self.covertBtn.alpha = 0.7f;
    self.covertBtn.enabled = NO;
    
    dispatch_async(dispatch_get_global_queue(0, 0), ^{
        
        codeWAV([resPath UTF8String], [destPath UTF8String]);
        
        dispatch_async(dispatch_get_main_queue(), ^{
            
            self.disAearText.text = @"转换完成...";
            self.playBtn.enabled = YES;
            self.playBtn.alpha = 1.f;
            self.destFilePath = destPath;
        });
    });
}

- (IBAction)doAAC2OtherAction:(id)sender {
    
    if (nil == self.destFilePath) {
        return;
    }
    
    self.disAearText.text = @"正在播放 AAC 音频文件...";
    
    [self _playAAC];
}

- (void)_playAAC {
    
    NSURL *soundUrl = [NSURL fileURLWithPath:self.destFilePath isDirectory:NO];
    
    AVAudioSession *audioSession = [AVAudioSession sharedInstance];
    [audioSession setActive: YES error: nil];
    
    if (nil == _audioPlayer) {
        
        NSError *error;
        _audioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:soundUrl
                                                              error:&error];
        
        if (nil != error) {
            
            NSLog(@"Init AVAudioPlayer error: %@", error);
            return;
        }
    }
    
    [self.audioPlayer prepareToPlay];
    [self.audioPlayer play];
}

- (IBAction)doCovertAACToWAVAction:(id)sender {
    
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *resPath = [bundle pathForResource:@"mm" ofType:@"aac"];
    NSLog(@"The path of aac file: %@", resPath);
    
    NSArray<NSString *> *docPath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *destPath = [[docPath lastObject] stringByAppendingString:@"/out.wav"];
    NSLog(@"The path of wav file: %@", destPath);
    
    self.disAearText.text = @"正在转换...";
    self.covertWAVToAACBtn.alpha = 0.7f;
    self.covertWAVToAACBtn.enabled = NO;
    
    dispatch_async(dispatch_get_global_queue(0, 0), ^{
        
        decodeAAC([resPath UTF8String], [destPath UTF8String]);
        
        dispatch_async(dispatch_get_main_queue(), ^{
            
            self.disAearText.text = @"转换完成...";
            self.playWAVBtn.enabled = YES;
            self.playWAVBtn.alpha = 1.f;
            self.destFilePath = destPath;
        });
    });
}

- (IBAction)playWAVAction:(id)sender {
    
    if (nil == self.destFilePath) {
        return;
    }
    
    self.disAearText.text = @"正在播放 WAV 音频文件...";
    
    [self _playAAC];
}

@end
