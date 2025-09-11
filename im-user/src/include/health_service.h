#pragma once
#include "health.pb.h"
#include <chrono>

inline int64_t NowMs()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

class HealthServiceImpl : public mpim::HealthService
{
public:
	void Ping(google::protobuf::RpcController *controller,
			  const mpim::PingReq *request,
			  mpim::PingResp *response,
			  google::protobuf::Closure *done) override
	{
		response->set_echo(request->payload());
		response->set_ts_server_ms(NowMs());
		if (done)
			done->Run();
	}
};
