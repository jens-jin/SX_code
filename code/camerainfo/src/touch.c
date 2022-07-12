#include "touch.h"

int wait4touch(int *x, int *y)
{
	// 1���򿪴��������豸�ļ�
	int fd = open("/dev/input/event0", O_RDWR);
	if (-1 == fd)
	{
		printf("open file err!\n");
		return -1;
	}

	// 2.���ո����ĸ�ʽȥѭ����ȡ����
	struct input_event ts;
	bool flag_x = false;
	bool flag_y = false;
	while (1)
	{
		//��սṹ�����
		bzero(&ts, sizeof(ts));

		//���û�д��������ݣ��������ȴ���ֱ��������Ϊֹ
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

		//����
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
	// 1���򿪴��������豸�ļ�
	int fd = open("/dev/input/event0", O_RDWR);
	if (-1 == fd)
	{
		printf("open file err!\n");
		return -1;
	}
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK); //������ģʽ

	int epollfd = epoll_create(1);

	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = EPOLLIN | EPOLLET; //��Ե����
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);

	struct epoll_event evs[10]; //���ؾ��
	// 2.���ո����ĸ�ʽȥѭ����ȡ����
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

				//�ɿ�
				if (ts.type == EV_KEY && ts.code == BTN_TOUCH && ts.value == 0)
				{
					if (flag_y == true && flag_x == true)
						break;
				}
			}
		}
		close(fd);
	}