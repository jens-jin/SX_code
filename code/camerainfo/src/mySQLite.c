#include "mySQLite.h"


bool first = true;

sqlite3 *db = NULL; // 数据库的操作句柄
char *err;          // 报错信息
int fifoIn;
int fifoOut;


// 每当你使用SELECT得到N条记录时，就会自动调用N次以下函数
// 参数：
// arg: 用户自定义参数
// len: 列总数
// col_val: 每一列的值
// col_name: 每一列的名称（标题)
int showDB(void *arg, int len, char **col_val, char **col_name)
{
    // 显示标题(只显示一次)
    if(first)
    {
        printf("\n");
        for(int i=0; i<len; i++)
        {
            printf("%s\t", col_name[i]);
        }
        printf("\n==============");
        printf("==============\n");
        first = false;
    }

    // 显示内容(一行一行输出)
    for(int i=0; i<len; i++)
    {
        printf("%s\t", col_val[i]);
    }
    printf("\n");

    // 返回0: 继续针对下一条记录调用本函数
    // 返回非0: 停止调用本函数
    return 0;
}

void *carIn(void *arg)
{
    char SQL[100];

    // 静静地等待LPR发来的车牌号
    char carplate[20];
    while(1)
    {
		//读取lpr发送的车牌号
        bzero(carplate, 20);
        read(fifoIn, carplate, 20);

		//利用车牌号和时间完成sql语句的拼接
        bzero(SQL, 100);
        snprintf(SQL, 100, "INSERT INTO info VALUES"
			   "('%s', '%lu');", carplate, time(NULL));
	
		//执行sql语句，完成数据的插入
        sqlite3_exec(db, SQL, NULL, NULL, &err);
        if(err != SQLITE_OK)
        {
			printf("车辆入场失败\n");
        }
        else
        {
            printf("车辆入场成功\n");
			bzero(SQL, 100);
			snprintf(SQL, 100, "SELECT * FROM info;");

			first = true;
			sqlite3_exec(db, SQL, showDB, NULL/*用户自定义参数*/, &err);
        }
    }
}

void *carOut(void *arg)
{
	char SQL[100];

    // 静静地等待LPR发来的车牌号
    char carplate[20];
    while(1)
    {
		//读取lpr发送的车牌号
        bzero(carplate, 20);
        read(fifoOut, carplate, 20);
		
		printf("出库车牌为%s\n", carplate);
		
		/* 查询数据库，是否有出库车辆数据 */
		
		/* 如果有 就删除并获取时间 */
		
	}
	
}

void *fun_sqlite(void *arg)
{
    printf("启动 SQLite 成功\n");
	
	 
	//创建进程间通信通道   有名管道
	mkfifo("/tmp/LPR2SQLitIn", 0666);
	mkfifo("/tmp/LPR2SQLitOut", 0666);
	
	fifoIn = open("/tmp/LPR2SQLitIn", O_RDWR);
	fifoOut = open("/tmp/LPR2SQLitOut", O_RDWR);
	
	
    // 1，创建、打开一个数据库文件*.db
    int ret = sqlite3_open_v2("parking.db", &db,
                              SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, NULL);
    if(ret != SQLITE_OK)
    {
        printf("创建数据库文件失败:%s\n", sqlite3_errmsg(db));
        exit(0);
    }

    // 2，创建表Table
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS info"
		     "(车牌 TEXT PRIMARY KEY, 时间 TEXT);",
                 NULL, NULL, &err);

    

    // 4，创建入库线程
    pthread_t t1;
    pthread_create(&t1, NULL, carIn,  NULL);
	
	// 4，创建出库线程
    pthread_t t2;
    pthread_create(&t2, NULL, carOut,  NULL);

    // 5，主线程退出
    pthread_exit(NULL);
}

