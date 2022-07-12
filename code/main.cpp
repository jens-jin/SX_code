#include "include/Pipeline.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

using namespace std;

int fifoIn;
int fifoOut;

int flag = 0;
char aa=0;
void *fun(void *arg)
{
	while (1)
	{
		if ((aa=getchar()) != 'r')
			{putchar(aa); continue;}
		else
		{
			if (flag == 0)
			{
				flag = 1; //切换到出库模式
				cout << "切换到出库模式" << endl;
			}
			else
			{
				flag = 0; //切换到入库模式
				cout << "切换到入库模式" << endl;
			}
		}
	}
}

void func(int sig)
{
	cout << "正在进行车牌识别" << endl;

	// 加载数据模型
	pr::PipelinePR prc("model/cascade.xml",
					   "model/HorizonalFinemapping.prototxt",
					   "model/HorizonalFinemapping.caffemodel",
					   "model/Segmentation.prototxt",
					   "model/Segmentation.caffemodel",
					   "model/CharacterRecognization.prototxt",
					   "model/CharacterRecognization.caffemodel",
					   "model/SegmenationFree-Inception.prototxt",
					   "model/SegmenationFree-Inception.caffemodel");

	string pn;
	// 读取一张图片（支持BMP、JPG、PNG等）
	cv::Mat image = cv::imread("a.jpg");
	std::vector<pr::PlateInfo> res;

	// 尝试识别图片中的车牌信息
	// 若图片中不含车牌信息，接口会抛出异常
	try
	{
		res = prc.RunPiplineAsImage(image, pr::SEGMENTATION_FREE_METHOD);
	}
	catch (...)
	{
		cout << "未检测到车牌" << endl;
	}

	for (auto st : res)
	{
		pn = st.getPlateName();
		if (pn.length() == 9)
		{
			cout << "检测到车牌: " << pn.data();
			cout << "，确信率: " << st.confidence << endl;

			if (flag == 0)
				//将识别到的车牌号发送到数据库
				write(fifoIn, pn.data(), pn.length());
			else
				//将识别到的车牌号发送到数据库
				write(fifoOut, pn.data(), pn.length());
		}
	}

	//发送识别完成信号
	cout << "车牌识别完成" << endl;
	kill(getppid(), SIGUSR2);
}

int main(int argc, char **argv)
{
	/*
	if(argc != 2)
	{
		printf("请指定一张图片\n");
		exit(0);
	}
	*/

	printf("车牌识别启动\n");
	printf("现在是入库模式\n");

	//设置收到SIGUSR1信号之后去调用函数func，该函数实现车牌识别
	signal(SIGUSR1, func);

	fifoIn = open("/tmp/LPR2SQLitIn", O_RDWR);
	fifoOut = open("/tmp/LPR2SQLitOut", O_RDWR);

	//创建一个新的线程，用户的模式切换
	pthread_t tid;
	pthread_create(&tid, NULL, fun, NULL);

    pthread_join(tid,NULL);

	return 0;
}