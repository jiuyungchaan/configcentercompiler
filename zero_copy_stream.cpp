#include <zero_copy_stream.h>
#include <common.h>

ZeroCopyInputStream::~ZeroCopyInputStream() {}
ZeroCopyOutputStream::~ZeroCopyOutputStream() {}

bool ZeroCopyOutputStream::WriteAliasedRaw(const void* data, int size){
	return false;
}