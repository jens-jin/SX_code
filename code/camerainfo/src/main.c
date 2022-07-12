#include "camera.h"
#include "touch.h"
#include "mySQLite.h"

int SCREEN_W, SCREEN_H;
int CAMERA_W, CAMERA_H;
int redoffset  ;
int greenoffset;
int blueoffset ;
struct fb_var_screeninfo lcdinfo;
uint8_t *fb;
uint8_t *gYUV;

#define MIN(a, b) \
	({ \
		typeof(a) _a = a; \
		typeof(b) _b = b; \
		(void)(&_a==&_b); \
		_a < _b ? _a : _b; \
	})

	
//实时监控显示
void display(uint8_t *yuv)
{
	static uint32_t shown = 0;

	int R0, G0, B0;
	int R1, G1, B1;	

	uint8_t Y0, U;
	uint8_t Y1, V;

	int w = MIN(SCREEN_W, CAMERA_W);
	int h = MIN(SCREEN_H, CAMERA_H);


	uint8_t *fbtmp = fb;

	int yuv_offset, lcd_offset;
	for(int y=0; y<h; y++)
	{
		for(int x=0; x<w; x+=2)
		{
			yuv_offset = ( CAMERA_W*y + x ) * 2;
			lcd_offset = ( SCREEN_W*y + x ) * 4;
			
			Y0 = *(yuv + yuv_offset + 0);
			U  = *(yuv + yuv_offset + 1);
			Y1 = *(yuv + yuv_offset + 2);
			V  = *(yuv + yuv_offset + 3);

			*(fbtmp + lcd_offset + redoffset  +0) = R[Y0][V];
			*(fbtmp + lcd_offset + greenoffset+0) = G[Y0][U][V];
			*(fbtmp + lcd_offset + blueoffset +0) = B[Y0][U];

			*(fbtmp + lcd_offset + redoffset  +4) = R[Y1][V];
			*(fbtmp + lcd_offset + greenoffset+4) = G[Y1][U][V];
			*(fbtmp + lcd_offset + blueoffset +4) = B[Y1][U];
		}
	}
	shown++;
}

int x, y;				//触摸屏坐标
pid_t pid;				//车牌识别进程的进程号
int flag = 0;			//能否进行拍照识别的标志位    0是可以拍照并识别    1是不能
void *task(void *arg)
{
	while(1)
	{
		wait4leave(&x, &y);
		
		printf("x:%d, y:%d\n", x, y);
		
		
		if(x>640)	//拍照按键按下
		{
			if(flag == 0)
			{
				printf("正在拍照\n");
				//拍照
				yuv2jpg(gYUV);
				printf("拍照完成\n");
				
				//给车牌识别的进程发送一个信号，让他进行识别
				printf("启动识别\n");
				kill(pid, SIGUSR1);
				flag  = 1;
			}
			else
				printf("正在进行车牌识别，请稍后。。。\n");
		}
	}
	
}

void fun(int sig)
{
	printf("车牌识别完成！\n");
	flag = 0;
}
void EXIT(int sig)
{
	printf("recv sig=%d\nexit",sig);
	exit(0);
}
int main(int argc, char **argv)
{
	// 0，确保合法参数
	if(argc != 2)
	{
		printf("参数错误，请指定摄像头设备文件名\n");
		exit(0);
	}
	
	//接受车牌识别完成信号
	signal(SIGUSR2, fun);signal(SIGINT,EXIT);
	
	// 1，打开摄像头
	int camfd = open(argv[1], O_RDWR);	
	if(camfd == -1)
	{
		perror("打开摄像头失败");
		exit(0);
	}

	// 2，查看摄像头信息
	get_camcap(camfd);
	get_camfmt(camfd); // 显示摄像头当前配置的格式

	// 3，设置摄像头信息
	get_caminfo(camfd);// 罗列当前摄像头所支持的所有的格式: JPEG/YUYV/MJPG
	set_camfmt(camfd);	//设置摄像头为yuyv格式，像素为640*480
	
	
	//准备映射表
	convert();
	
	// 打开LCD设备
	int lcd = open("/dev/fb0", O_RDWR);
	if(lcd == -1)
	{
		perror("open \"/dev/fb0\" failed");
		exit(0);
	}
	
	// 获取LCD显示器的设备参数
	ioctl(lcd, FBIOGET_VSCREENINFO, &lcdinfo);

	SCREEN_W = lcdinfo.xres;
	SCREEN_H = lcdinfo.yres;
	
	fb = mmap(NULL, lcdinfo.xres* lcdinfo.yres * lcdinfo.bits_per_pixel/8,
				    PROT_READ | PROT_WRITE, MAP_SHARED, lcd, 0);
	if(fb == MAP_FAILED)
	{
		perror("mmap failed");
		exit(0);
	}
	
	// 清屏
	// 显存双缓冲优化
	// bzero(fb, 2 * lcdinfo.xres * lcdinfo.yres * 4);
	bzero(fb,  lcdinfo.xres * lcdinfo.yres * 4);

	
	// 获取RGB偏移量
	redoffset  = lcdinfo.red.offset/8;
	greenoffset= lcdinfo.green.offset/8;
	blueoffset = lcdinfo.blue.offset/8;
	
	CAMERA_W = fmt.fmt.pix.width;
	CAMERA_H = fmt.fmt.pix.height;

	// 设置即将要申请的摄像头缓存的参数
	int nbuf = 3;

	struct v4l2_requestbuffers reqbuf;
	bzero(&reqbuf, sizeof (reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = nbuf;

	// 使用该参数reqbuf来申请缓存
	ioctl(camfd, VIDIOC_REQBUFS, &reqbuf);

	// 根据刚刚设置的reqbuf.count的值，来定义相应数量的struct v4l2_buffer
	// 每一个struct v4l2_buffer对应内核摄像头驱动中的一个缓存
	struct v4l2_buffer buffer[nbuf];
	int length[nbuf];
	uint8_t *start[nbuf];

	for(int i=0; i<nbuf; i++)
	{
		bzero(&buffer[i], sizeof(buffer[i]));
		buffer[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer[i].memory = V4L2_MEMORY_MMAP;
		buffer[i].index = i;
		ioctl(camfd, VIDIOC_QUERYBUF, &buffer[i]);

		length[i] = buffer[i].length;
		start[i] = mmap(NULL, buffer[i].length,	PROT_READ | PROT_WRITE,
				        MAP_SHARED,	camfd, buffer[i].m.offset);

		ioctl(camfd , VIDIOC_QBUF, &buffer[i]);
	}

	// 启动摄像头数据采集
	enum v4l2_buf_type vtype= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(camfd, VIDIOC_STREAMON, &vtype);

	struct v4l2_buffer v4lbuf;
	bzero(&v4lbuf, sizeof(v4lbuf));
	v4lbuf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4lbuf.memory= V4L2_MEMORY_MMAP;
	
	
	//创建一个新的线程，不断的获取触摸屏坐标
	pthread_t tid;
	pthread_create(&tid, NULL, task, NULL);
	
	//创建一个新的线程，完成数据库操作
	pthread_t tid1;
	pthread_create(&tid1, NULL, fun_sqlite, NULL);
	
	
	//创建一个进程，让该进程去执行车牌识别的程序
	pid = fork();
	if(pid == 0)
	{//新创建的进程需要执行的
		
		printf("正在启动车牌识别！\n");
		execl("./alpr", "alpr", NULL);
		printf("启动车牌失败失败！\n");
		exit(0);
	}
	

	// 开始抓取摄像头数据并在屏幕播放视频
	int i=0;
	while(1)
	{
		//wait4touch(); //如果没有点击触摸屏会阻塞，导致后面的代码无法执行
		
		// 从队列中取出填满数据的缓存，让摄像头不再写入该空间
		v4lbuf.index = i%nbuf;
		ioctl(camfd , VIDIOC_DQBUF, &v4lbuf);
	
		//获取摄像头的yuv数据，转换后显示到开发板屏幕
		display(gYUV = start[i%nbuf]);

	 	// 将已经读取过数据的缓存块重新置入队列中，让摄像头将数据写入该空间
		v4lbuf.index = i%nbuf;
		ioctl(camfd , VIDIOC_QBUF, &v4lbuf);

		i++;
	}

	
	// 4，关闭文件
	close(camfd);
}
