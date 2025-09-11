#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <semaphore.h>
#include <iostream>
#include <future>

// 全局的watcher观察器		zkserver给zkclient的通知
void global_watcher(zhandle_t *zh, int type,
					int state, const char *path, void *watcherCtx)
{
	if (type == ZOO_SESSION_EVENT)	//回调的消息类型是和会话相关的消息类型
	{
		if (state == ZOO_CONNECTED_STATE)	// zkclient和zkserver连接建立成功
		{
			// sem_t *sem = (sem_t *)zoo_get_context(zh);	
			// sem_post(sem);
			auto* prom = static_cast<std::promise<void>*>(watcherCtx);
			if(prom){
				prom->set_value();
			}
		}
	}
}

ZkClient::ZkClient()
	: m_zhandle(nullptr)
{
}

ZkClient::~ZkClient()
{
	if (m_zhandle != nullptr)
	{
		zookeeper_close(m_zhandle); // 关闭句柄，释放资源
	}
}

// 连接zkserver
void ZkClient::Start()
{
	// std::cout << "ZkClient::Start()......" << std::endl;
	std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
	std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
	std::string connstr = host + ":" + port;

	zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);	//设置zookeeper客户端的日志级别,不打印普通日志

	std::promise<void> prom;
	auto fut = prom.get_future();
	/*
	zookeeper_mt: 多线程版本
	zookeeper的API客户端程序提供了三个线程
	API调用线程
	网络IO线程 pthread_create poll
	watcher回调线程
	*/
	// 异步
	m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, &prom, 0);
	// std::cout << "connstr: " << connstr << std::endl;
	if (nullptr == m_zhandle)
	{
		std::cout << "zookeeper_init error!" << std::endl;
		exit(EXIT_FAILURE);
	}

	/*
	zooKeeper 客户端初始化是异步的，zookeeper_init 会启动多个线程（包括网络 IO 线程和 watcher 回调线程）来完成初始化。
	sem 信号量用于同步主线程和 ZooKeeper 的 watcher 回调线程：
	主线程调用 sem_wait(&sem) 阻塞，等待 ZooKeeper 初始化完成。
	当 ZooKeeper 初始化完成时，回调函数 global_watcher 会被触发，回调函数中可以调用 sem_post(&sem) 唤醒主线程。
	这样可以确保 ZooKeeper 客户端在初始化完成后，主线程才会继续执行后续逻辑。
	*/

	fut.get();	//阻塞直到set_value()
	// sem_t sem;
	// sem_init(&sem, 0, 0);
	// zoo_set_context(m_zhandle, this);	//将sem交给m_zhandle句柄，句柄帮忙传递给global_watcher，通过以上达到同步的目的(global_watcher调用之后，sem_post(&sem);)

	// sem_wait(&m_sem);
	// std::cout << "zookeeper_init success!" << std::endl;
}
// 在zkserver上根据指定的path创建znode节点
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
	char path_buffer[128];
	int bufferlen = sizeof(path_buffer);
	int flag;
	// 先判断path表示的znode节点是否存在，如果存在，就不再重复创建了
	flag = zoo_exists(m_zhandle, path, 0, nullptr);
	if (ZNONODE == flag)	//表示path的znode节点不存在
	{
		// 创建指定path的znode节点
		flag = zoo_create(m_zhandle, path, data, datalen,
				&ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
		if (flag == ZOK)
		{
			std::cout << "znode create success... path: " << path << std::endl;
		}
		else
		{
			std::cout << "flag:" << flag << std::endl;
			std::cout << "znode create error... path: " << path << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}

// 根据参数指定的znode节点路径，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
	char buffer[64];
	int bufferlen = sizeof(buffer);
	int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
	if (flag != ZOK)
	{
		std::cout << "get znode error... path: " << path << std::endl;
		return "";
	}
	else
	{
		return buffer;
	}
}