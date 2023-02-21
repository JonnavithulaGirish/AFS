#ifdef __cplusplus
extern "C"
#endif
int afsGetAttr(const char* path, struct stat *buf);
int afsOpen(const char* path, int flags);
int afsClose(int fh);
int afsMkdir(const char* path, int flags);
int afsRmdir(const char* path);
int64_t afsOpendir(const char* path);
int afsReleasedir(int64_t fh);