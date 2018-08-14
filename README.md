`LookupTable` 是一个线程安全的哈希表，支持快速查询、插入及删除操作。

`LookupTable` 内部由固定大小的`Bucket`数组实现，并基于链表处理哈希冲突。

由于数组大小固定，我们只需要保证每个`Bucket`的同步即可。对不同`Bucket`的操作是线程安全的。

来源 Cpp Concurrency In Action 6.3.1章节
