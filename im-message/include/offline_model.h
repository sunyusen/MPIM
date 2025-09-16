#pragma once
#include "db.h"
#include <vector>
#include <string>
#include <memory>

class OfflineModel {
public:
  OfflineModel();
  bool insert(int64_t uid, const std::string& payload);
  std::vector<std::string> query(int64_t uid);
  bool remove(int64_t uid);
  
  // 群组消息相关
  bool insertGroupMessage(int64_t group_id, const std::string& payload);
  std::vector<std::string> queryGroupMessages(int64_t group_id);
  bool removeGroupMessages(int64_t group_id);
private:
  std::unique_ptr<MySQL> db_;
};
