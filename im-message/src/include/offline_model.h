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
private:
  std::unique_ptr<MySQL> db_;
};
