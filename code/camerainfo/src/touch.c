#include "touch.h"

int wait4touch(int *x, int *y)
{
	// 1、打开触摸屏的设备文件
	int fd = open("/dev/input/event0", O_RDWR);
	if (-1 == fd)
	{
		printf("open file err!\n");
		return -1;
	}

	// 2.按照给定的格式去循环读取数据
	struct input_event ts;
	bool flag_x = false;
	bool flag_y = false;
	while (1)
	{
		//清空结构体变量
		bzero(&ts, sizeof(ts));

		//如果没有触摸屏数据，会阻塞等待，直到有数据为止
		read(fd, &ts, sizeof(ts));

		// printf("ts.type:%d, ts.code:%d, ts.value:%d\n",ts.type, ts.code, ts.value);
		if (ts.type == EV_ABS && ts.code == ABS_X)
		{
			// printf("x:%d\n", ts.value);
			*x = ts.value;
			flag_x = true;
		}

		if (ts.type == EV_ABS && ts.code == ABS_Y)
		{
			// printf("y:%d\n", ts.value);
			*y = ts.value;
			flag_y = true;
		}

		//按下
		if (ts.type == EV_KEY && ts.code == BTN_TOUCH && ts.value == 1)
		{
			if (flag_y == true && flag_x == true)
				break;
		}
	}
	close(fd);
}

int wait4leave(int *x, int *y)
{
	// 1、打开触摸屏的设备文件
	int fd = open("/dev/input/event0", O_RDWR);
	if (-1 == fd)
	{
		printf("open file err!\n");
		return -1;
	}
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK); //非阻塞模式

	int epollfd = epoll_create(1);

	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = EPOLLIN | EPOLLET; //边缘触发
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);

	struct epoll_event evs[10]; //返回句柄
	// 2.按照给定的格式去循环读取数据
	struct input_event ts;
	bool flag_x = false;
	bool flag_y = false;
	while (1)
	{
		int infds = epoll_wait(epollfd, evs, 10, 5000);

		if (infds < 0)
		{
			perror("epoll failed\n");
			break;
		}

		if (infds == 0)
		{
			printf("timeout \n");
			continue;
		}

		for (int ii = 0; ii < infds; ii++)
		{
			if (evs[ii].data.fd == fd)
			{
				bzero(&ts, sizeof(ts));
				char buffer[1024];
				memset(buffer, 0, sizeof(buffer));
				// disconnect
				int iret = 0;
				char *ptr = buffer;
				while (true)
				{
					if ((iret = read(fd, &ts, sizeof(ts)))) <= 0)
						{
							if (errno == EAGAIN)
								break;
						}
					ptr += iret;
				}
				if (ts.type == EV_ABS && ts.code == ABS_X)
				{
					// printf("x:%d\n", ts.value);
					*x = ts.value;
					flag_x = true;
				}

				if (ts.type == EV_ABS && ts.code == ABS_Y)
				{
					// printf("y:%d\n", ts.value);
					*y = ts.value;
					flag_y = true;
				}

				//松开
				if (ts.type == EV_KEY && ts.code == BTN_TOUCH && ts.value == 0)
				{
					if (flag_y == true && flag_x == true)
						break;
				}
			}
		}
		close(fd);
	}