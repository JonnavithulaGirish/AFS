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
int afsRename(const char* oldPath, const char*newPath);
int afsTruncate(const char* path, int len);
int afsMknod(const char* path, mode_t len, dev_t dev);
int64_t afsReaddir(int64_t dp, int *sz, char ***dnames);
int afsCreat(const char *path, int flags, mode_t mode);
int afsUnlink(const char *path);