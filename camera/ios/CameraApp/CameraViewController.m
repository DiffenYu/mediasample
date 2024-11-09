#import "CameraViewController.h"
#import <AVFoundation/AVFoundation.h>

@interface CameraViewController ()
@property (nonatomic, strong) AVCaptureSession *captureSession;
@property (nonatomic, strong) AVCaptureDeviceInput *currentInput;
@property (nonatomic, strong) AVCaptureVideoPreviewLayer *previewLayer;
@property (nonatomic, strong) UIButton *switchCameraButton;
@property (nonatomic, strong) UIButton *toggleCameraButton;
@property (nonatomic, assign) BOOL isCameraRunning;
@end

@implementation CameraViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor blackColor];
    
    // 配置捕捉会话
    self.captureSession = [[AVCaptureSession alloc] init];
    self.captureSession.sessionPreset = AVCaptureSessionPresetHigh;
    
    // 设置相机预览
    self.previewLayer = [AVCaptureVideoPreviewLayer layerWithSession:self.captureSession];
    self.previewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
    self.previewLayer.frame = self.view.bounds;
    [self.view.layer addSublayer:self.previewLayer];
    
    // 添加切换相机按钮
    self.switchCameraButton = [UIButton buttonWithType:UIButtonTypeSystem];
    self.switchCameraButton.frame = CGRectMake(20, 50, 100, 50);
    [self.switchCameraButton setTitle:@"Switch Camera" forState:UIControlStateNormal];
    [self.switchCameraButton addTarget:self action:@selector(switchCamera) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:self.switchCameraButton];
    
    // 添加开启/关闭相机按钮
    self.toggleCameraButton = [UIButton buttonWithType:UIButtonTypeSystem];
    self.toggleCameraButton.frame = CGRectMake(150, 50, 100, 50);
    [self.toggleCameraButton setTitle:@"Start Camera" forState:UIControlStateNormal];
    [self.toggleCameraButton addTarget:self action:@selector(toggleCamera) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:self.toggleCameraButton];
    
    // 默认使用后置摄像头
    [self setupCamera:AVCaptureDevicePositionBack];
}

- (void)setupCamera:(AVCaptureDevicePosition)position {
    NSError *error = nil;
    
    // 获取设备
    AVCaptureDevice *device = [self cameraWithPosition:position];
    if (!device) {
        NSLog(@"无法访问指定的摄像头设备。");
        return;
    }
    
    // 清除之前的输入
    [self.captureSession beginConfiguration];
    if (self.currentInput) {
        [self.captureSession removeInput:self.currentInput];
    }
    
    // 创建新的输入并添加到会话
    self.currentInput = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
    if ([self.captureSession canAddInput:self.currentInput]) {
        [self.captureSession addInput:self.currentInput];
    } else {
        NSLog(@"无法添加摄像头输入: %@", error);
    }
    
    [self.captureSession commitConfiguration];
}

- (AVCaptureDevice *)cameraWithPosition:(AVCaptureDevicePosition)position {
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in devices) {
        if (device.position == position) {
            return device;
        }
    }
    return nil;
}

- (void)switchCamera {
    AVCaptureDevicePosition newPosition = (self.currentInput.device.position == AVCaptureDevicePositionBack) ? AVCaptureDevicePositionFront : AVCaptureDevicePositionBack;
    [self setupCamera:newPosition];
}

- (void)toggleCamera {
    if (self.isCameraRunning) {
        [self.captureSession stopRunning];
        [self.toggleCameraButton setTitle:@"Start Camera" forState:UIControlStateNormal];
    } else {
        [self.captureSession startRunning];
        [self.toggleCameraButton setTitle:@"Stop Camera" forState:UIControlStateNormal];
    }
    self.isCameraRunning = !self.isCameraRunning;
}

@end
