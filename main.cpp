#include <iostream>
#include "lookuptable.h"

LookupTable<std::string, int> table;
void TableAdd(std::string s, char base) {
  for (auto c : s) {
    table.Insert(std::string(1, c), (c - base));
  }
}

void TableDelete1() {
  std::string s("AacD");
  for (auto c : s) {
    table.Delete(std::string(1, c));
  }
  for (auto c : s) {
    int v = table.Query(std::string(1, c), -1);
    assert(v == -1);
  }
}

void TableDelete2() {
  std::string s("BbCad");
  for (auto c : s) {
    table.Delete(std::string(1, c));
  }
  for (auto c : s) {
    int v = table.Query(std::string(1, c), -1);
    assert(v == -1);
  }
}

std::mutex cout_mutex;
void PrintTable() {
  std::map<std::string, int> m = table.ToMap();
  std::lock_guard<std::mutex> lock(cout_mutex);
  std::cout << "{";
  for (auto &i : m) {
    std::cout << i.first << ":" << i.second << ", ";
  }
  std::cout << "}\n";
}

struct User {
  std::string id;
  std::string name;
  User(std::string id, std::string name) : id(std::move(id)), name(std::move(name)) {}
};

using UserTable = LookupTable<std::string, std::shared_ptr<User>>;

void InsertUser(UserTable *table, std::string id, std::string name) {
  table->Insert(id, std::make_shared<User>(id, name));
}

void QueryUser(UserTable *table, std::string id) {
  auto user = table->Query(id, nullptr);
  if (user) {
    printf("User (%s) found: %s %s\n", id.c_str(), user->id.c_str(), user->name.c_str());
  } else {
    printf("User (%s) not found\n", id.c_str());
  }
}

int main() {
  std::thread t1{TableAdd, "abcdefghijklmn", 'a'};
  std::thread t2{TableAdd, "opqrstuvwxyz", 'a'};
  std::thread t3{TableAdd, "ABCDEFGHIJKLMN", 'A'};
  std::thread t4{TableAdd, "OPQRSTUVWXYZ", 'A'};
  std::thread t5{TableAdd, "0123456789", '0'};
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();

  std::thread t7{TableDelete1};
  std::thread t8{TableDelete2};
  t7.join();
  t8.join();

  std::thread t9{PrintTable};
  std::thread t10{PrintTable};
  t9.join();
  t10.join();

  LookupTable<std::string, std::shared_ptr<User>> user_table;
  std::vector<std::thread> user_threads;

  for (int i = 0; i < 10; i++) {
    std::string s = std::to_string(i);
    user_threads.emplace_back(InsertUser, &user_table, std::to_string(i), "user" + s);
  }

  for (int i = 0; i < 10; i++) {
    user_threads[i].join();
  }

  QueryUser(&user_table, "1");
  QueryUser(&user_table, "3");
  QueryUser(&user_table, "11");

  return 0;
}
