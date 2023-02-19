#ifdef __cplusplus
extern "C"
#endif
int afsGetAttr(const char* path, struct stat *buf);
int afsOpen(const char* path, int flags);
int afsClose(int fh);
