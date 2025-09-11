#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "mprpcapplication.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"
#include "health.pb.h"

using namespace std::chrono;

// 简单统计工具
struct Stats
{
	std::vector<double> lat_ms;
	std::atomic<uint64_t> ok{0}, fail{0};
	void add(double ms, bool success)
	{
		if (success)
			ok.fetch_add(1, std::memory_order_relaxed);
		else
			fail.fetch_add(1, std::memory_order_relaxed);
		lat_ms.push_back(ms);
	}
};

static double percentile(std::vector<double> &v, double p)
{
	if (v.empty())
		return 0;
	size_t k = std::max<size_t>(0, std::min<size_t>(v.size() - 1, (size_t)std::llround((p / 100.0) * (v.size() - 1))));
	std::nth_element(v.begin(), v.begin() + k, v.end());
	return v[k];
}

struct Cmd
{
	std::string zk_conf = "test.conf"; // 你的 MprpcApplication::Init 用到的 conf
	int threads = 4;
	int qps_per_thread = 500;
	int seconds = 10;
	int payload = 64;
};

int main(int argc, char **argv)
{
	Cmd cmd;
	// 极简参数解析
	for (int i = 1; i < argc; i++)
	{
		std::string a = argv[i];
		auto eat = [&](const char *key, auto &dst)
		{
			if (a.rfind(key, 0) == 0)
			{
				dst = std::stoi(a.substr(std::string(key).size()));
				return true;
			}
			return false;
		};
		if (a.rfind("--threads=", 0) == 0)
			eat("--threads=", cmd.threads);
		else if (a.rfind("--qps=", 0) == 0)
			eat("--qps=", cmd.qps_per_thread);
		else if (a.rfind("--seconds=", 0) == 0)
			eat("--seconds=", cmd.seconds);
		else if (a.rfind("--payload=", 0) == 0)
			eat("--payload=", cmd.payload);
		else if (a.rfind("--conf=", 0) == 0)
		{
			cmd.zk_conf = a.substr(7);
		}
	}

	// 伪造 argv 兼容你框架的 Init
	std::string arg0 = "bench_rpc";
	std::string arg1 = "-i";
	std::string arg2 = cmd.zk_conf;
	std::vector<char *> av{&arg0[0], &arg1[0], &arg2[0]};
	MprpcApplication::Init((int)av.size(), av.data());

	std::cout << "[bench] threads=" << cmd.threads
			  << " qps_per_thread=" << cmd.qps_per_thread
			  << " seconds=" << cmd.seconds
			  << " payload=" << cmd.payload << "B\n";

	std::string payload(cmd.payload, 'x');
	std::atomic<bool> stop{false};
	Stats stats;
	std::mutex mtx;

	auto worker = [&]()
	{
		MprpcChannel channel; // channel 会用 ZK 发现 mpim.HealthService
		mpim::HealthService_Stub stub(&channel);
		int interval_ns = (cmd.qps_per_thread > 0) ? (1000000000 / cmd.qps_per_thread) : 0;
		auto next_tick = steady_clock::now();

		while (!stop.load(std::memory_order_relaxed))
		{
			next_tick += nanoseconds(interval_ns);
			mpim::PingReq req;
			req.set_payload(payload);
			mpim::PingResp resp;
			MprpcController ctrl;

			auto t0 = steady_clock::now();
			stub.Ping(&ctrl, &req, &resp, nullptr);
			auto t1 = steady_clock::now();

			bool ok = !ctrl.Failed() && resp.echo().size() == (size_t)cmd.payload;
			double ms = duration<double, std::milli>(t1 - t0).count();

			{
				std::lock_guard<std::mutex> lk(mtx);
				stats.add(ms, ok);
			}

			if (interval_ns > 0)
			{
				std::this_thread::sleep_until(next_tick);
			}
		}
	};

	std::vector<std::thread> th;
	th.reserve(cmd.threads);
	for (int i = 0; i < cmd.threads; i++)
		th.emplace_back(worker);

	std::this_thread::sleep_for(std::chrono::seconds(cmd.seconds));
	stop.store(true);
	for (auto &t : th)
		t.join();

	std::cout << "[bench] ok=" << stats.ok.load() << " fail=" << stats.fail.load() << "\n";
	auto &v = stats.lat_ms;
	if (v.empty())
	{
		std::cout << "no samples\n";
		return 0;
	}
	// 粗算 QPS
	double total_reqs = (double)stats.ok.load() + stats.fail.load();
	double qps = total_reqs / cmd.seconds;
	std::cout << "QPS=" << qps
			  << " P50=" << percentile(v, 50)
			  << " P95=" << percentile(v, 95)
			  << " P99=" << percentile(v, 99) << " ms\n";
	return 0;
}
