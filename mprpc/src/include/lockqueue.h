#pragma once
#include <queue>
#include <thread>
#include <mutex>	//pthread_mutex_t
#include <condition_variable>	//pthread_condition_t

// 异步写日志的日志队列
template<typename T>
class LockQueue
{
public:
	LockQueue() :m_closed(false) {}

	// 关闭队列：唤醒所有等待者，之后的push直接失败
	void Close()
	{
		std::lock_guard<std::mutex> lg(m_mutex);
		m_closed = true;
		m_cond.notify_all(); // 唤醒所有等待的线程,通知他们队列关闭了
	}

	bool IsClosed() const {
		std::lock_guard<std::mutex> lg(m_mutex);
		return m_closed;
	}

	// 多个worker线程都会写日志queue
	bool Push(const T &data)
	{
		std::lock_guard<std::mutex> lg(m_mutex);
		if(m_closed) return false;
		m_queue.push(data);
		m_cond.notify_one(); // 唤醒一个等待的线程
		return true;
	}

	// 右值Push
	bool Push(T&& data)
	{
		std::lock_guard<std::mutex> lg(m_mutex);
		if(m_closed) return false;
		m_queue.push(std::move(data));
		m_cond.notify_one(); // 唤醒一个等待的线程
		return true;
	}

	// 阻塞Pop: 返回false表示队列已关闭且为空
	bool Pop(T& out)
	{
		std::unique_lock<std::mutex> ul(m_mutex);
		m_cond.wait(ul, [this]() { return !m_queue.empty() || m_closed; });
		if(m_queue.empty()) return false; // 队列为空且关闭
		out = std::move(m_queue.front());
		m_queue.pop();
		return true;
	}

	// 带超时的Pop。超时或关闭且空 => false
	bool TryPop(T& out, std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> ul(m_mutex);
		if(!m_cond.wait_for(ul, timeout, [this]() { return !m_queue.empty() || m_closed; }))
			return false; // 超时
		if(m_queue.empty()) return false; // 队列为空且关闭
		out = std::move(m_queue.front());
		m_queue.pop();
		return true;
	}

	bool Empty()
	{
		std::lock_guard<std::mutex> lg(m_mutex);
		return m_queue.empty();
	}

private:
	std::queue<T> m_queue;
	mutable std::mutex m_mutex;	//为了使const方法里面也能加锁，添加mutable
	std::condition_variable m_cond;
	bool m_closed;
};