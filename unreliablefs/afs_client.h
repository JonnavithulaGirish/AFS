#ifdef __cplusplus
extern "C"
#endif
int afsGetAttr(char* path, struct stat *buf);
int afsOpen(char* path, int flags);