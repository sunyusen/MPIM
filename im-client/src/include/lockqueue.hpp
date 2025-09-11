// im-client/src/include/lockqueue.hpp
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class LockQueue
{
public:
	bool Push(T v)
	{
		std::lock_guard<std::mutex> lg(m_);
		if (closed_)
			return false;
		q_.push(std::move(v));
		cv_.notify_one();
		return true;
	}
	// 阻塞直到有数据或被关闭。返回 false 表示队列已关闭且为空。
	bool Pop(T &out)
	{
		std::unique_lock<std::mutex> lk(m_);
		cv_.wait(lk, [&]
				 { return closed_ || !q_.empty(); });
		if (q_.empty())
			return false;
		out = std::move(q_.front());
		q_.pop();
		return true;
	}
	void Close()
	{
		std::lock_guard<std::mutex> lg(m_);
		closed_ = true;
		cv_.notify_all();
	}

private:
	std::queue<T> q_;
	std::mutex m_;
	std::condition_variable cv_;
	bool closed_{false};
};
