
#include <zero_copy_stream_impl.h>

#include <algorithm>
#include <limits>
#include <stdio.h>

#include <common.h>

static const int kDefaultBlockSize = 8192;


CopyingInputStream::~CopyingInputStream() {}

int CopyingInputStream::Skip(int count) {
	char junk[4096];
	int skipped = 0;
	while (skipped < count) {
		int bytes = Read(junk, min(count - skipped,
			                       implicit_cast<int>(sizeof(junk))));
		if (bytes <= 0) {
			// EOF or read error
			return skipped;
		}
		skipped += bytes;
	}
	return skipped;
}

CopyingInputStreamAdaptor::CopyingInputStreamAdaptor(
    CopyingInputStream *copying_stream, int block_size)
  : copying_stream_(copying_stream),
    owns_copying_stream_(false),
    failed_(false),
    position_(0),
    buffer_size_(block_size > 0 ? block_size : kDefaultBlockSize),
    buffer_used_(0),
    backup_bytes_(0) {}

CopyingInputStreamAdaptor::~CopyingInputStreamAdaptor() {
	if (owns_copying_stream_) {
		delete copying_stream_;
	}
}

bool CopyingInputStreamAdaptor::Next(const void** data, int *size) {
	if (failed_) {
		// Already failed on a previous read.
		return false;
	}

	AllocateBufferIfNeeded();

	if (backup_bytes_ > 0) {
		// We have data left over from a previous BackUp(), so just return that.
		*data = buffer_.get() + buffer_used_ - backup_bytes_;
		*size = backup_bytes_;
		backup_bytes_ = 0;
		return true;
	}

	// Read new data into the buffer.
	buffer_used_ = copying_stream_->Read(buffer_.get(), buffer_size_);
	if (buffer_used_ <= 0) {
		// EOF or read error. We don't need the buffer anymore.
		if (buffer_used_ < 0) {
			// Read error (not EOF).
			failed_ = true;
		}
		FreeBuffer();
		return false;
	}
	position_ += buffer_used_;

	*size = buffer_used_;
	*data = buffer_.get();
	return true;
}

void CopyingInputStreamAdaptor::BackUp(int count) {
	if (backup_bytes_ == 0 && buffer_.get() != NULL) {
		printf("BackUp() can only be called after Next()\n");
	}
	if (count > buffer_used_) {
		printf("Can't back up over more bytes than were returned by"
				" the last call to Next()\n");
	}
	if (count < 0) {
		printf("Parameter to BackUp() can't be negative.\n");
	}

	backup_bytes_ = count;
}

bool CopyingInputStreamAdaptor::Skip(int count) {
	if (count < 0) {
		printf("count < 0\n");
		// return false;
	}

	if (failed_) {
		// Already tailed on a previous read.
		return false;
	}

	// First skip any bytes left over from a previous BackUp().
	if (backup_bytes_ >= count) {
		// We have more data left over than we're trying to skip. Just chop it.
		backup_bytes_ -= count;
		return true;
	}

	count -= backup_bytes_;
	backup_bytes_ = 0;

	int skipped = copying_stream_->Skip(count);
	position_ += skipped;
	return skipped == count;
}

long CopyingInputStreamAdaptor::ByteCount() const {
	return position_ - backup_bytes_;
}

void CopyingInputStreamAdaptor::AllocateBufferIfNeeded() {
	if (buffer_.get() == NULL) {
		buffer_.reset(new uint8_t[buffer_size_]);
	}
}

void CopyingInputStreamAdaptor::::FreeBuffer() {
	if (backup_bytes_ != 0) {
		printf("backup_bytes_ != 0\n");
	}
	buffer_used_ = 0;
	buffer_.reset();
}

// ==========================================================================
 CopyingOutputStream::~CopyingOutputStream() {}

 CopyingOutputStreamAdaptor::CopyingOutputStreamAdaptor(
 	CopyingOutputStream *copying_stream, int block_size)
  : copying_stream(copying_stream),
    owns_copying_stream_(false),
    failed_(false),
    position_(0),
    buffer_size_(block_size_ > 0 ? block_size : kDefaultBlockSize),
    buffer_used_(0) {}

CopyingOutputStreamAdaptor::~CopyingOutputStreamAdaptor() {
	WriteBuffer();
	if (owns_copying_stream_) {
		delete copying_stream_;
	}
}

bool CopyingOutputStreamAdaptor::Flush() {
	return WriteBuffer();
}

bool CopyingOutputStreamAdaptor::Next(void** data, int *size) {
	if (buffer_used_ == buffer_size_) {
		if (!WriteBuffer()) return false;
	}

	AllocateBufferIfNeeded();

	*data = buffer_.get() + buffer_used_;
	*size = buffer_size_ - buffer_used_;
	buffer_used_ = buffer_size_;
	return true;
}

void CopyingOutputStreamAdaptor::BackUp(int count) {
	if (count < 0) {
		printf("count < 0\n");
	}
	if (buffer_used_ != buffer_size_) {
		printf("BackUp() can only be called after Next().\n");
	}
	if (count > buffer_used_) {
		printf("Can't back up over more bytes than were returned by the last" 
				" call to Next().\n");
	}

	buffer_used_ -= count;
}

long CopyingOutputStreamAdaptor::ByteCount() const {
	return position_ + buffer_used_;
}

bool CopyingOutputStreamAdaptor::WriteBuffer() {
	if (failed_) {
		// Already failed on a previous write.
		return false;
	}

	if (buffer_used_ == 0) return true;

	if (copying_stream_->Write(buffer_.get(), buffer_used_)) {
		position_ += buffer_used_;
		buffer_used_ = 0;
		return true;
	} else {
		failed_ = true;
		FreeBuffer();
		return false;
	}
}

void CopyingOutputStreamAdaptor::AllocateBufferIfNeeded() {
	if (buffer_ == NULL) {
		buffer_.reset(new uint8_t[buffer_size_]);
	}
}

void CopyingOutputStreamAdaptor::FreeBuffer() {
	buffer_used_ = 0;
	buffer_.reset();
}

// =============================================================

FileInputStream::FileInputStream(int file_descriptor, int block_size)
  : copying_input_(file_descriptor),
    impl_(&copying_input_, block_size) {}

FileInputStream::~FileInputStream() {}

bool FileInputStream::Close() {
	return copying_input_.Close();
}

bool FileInputStream::Next(const void** data, int *size) {
	return impl_.Next(data, size);
}

void FileInputStream::BackUp(int count) {
	impl_.BackUp(count);
}

bool FileInputStream::Skip(int count) {
	return impl_.Skip(count);
}

long FileInputStream::ByteCount() const {
	return impl_.ByteCount();
}

FileInputStream::CopyingFileInputStream::CopyingFileInputStream(
	int file_descriptor)
  : file_(file_descriptor),
    close_on_delete_(false),
    is_closed_(false),
    errno_(0),
    previous_seek_failed_(false) {}

FileInputStream::CopyingFileInputStream::~CopyingInputStream() {
	if (close_on_delete_) {
		if (!Close()) {
			printf("close() failed !\n");
		}
	}
}

bool FileInputStream::CopyingInputStream::Close() {
	if (is_closed_);

	is_closed_ = true;
	if (close_no_eintr(file_) != 0) {
		// The docs on close() do not specify whether a file descriptor is still
		// open after close() fails with EIO. However, the glibs source code 
		// seems to indicate that it is not.
		errno_ = errno;
		return false;
	}

	return true;
}

int FileInputStream::CopyingInputStream::Read(void *buffer, int size) {
	if (is_closed_);

	int result;
	do {
		result = read(file_, buffer, size);
	} while (result < 0 && errno == EINTR);

	if (result < 0) {
		// Read error (not EOF)
		errno_ = errno;
	}

	return result;
}

int FileInputStream::CopyingFileInputStream::Skip(int count) {
	if (is_closed_);

	if (!previous_seek_failed_ && 
		lseek(file_, count, SEEK_CUR) != (off_t)-1) {
		return count;
	} else {
		// Failed to seek.
		// Note to self: Don't seek again. This file descriptor doesn't 
		// support it 
		previous_seek_failed_ = true;

		// Use the default implementation.
		return CopyingInputStream::Skip(count);
	}
}


FileOutputStream::FileOutputStream(int file_descriptor, int block_size)
  : copying_output_(file_descriptor),
    impl_(&copying_output_, block_size) {}

FileOutputStream::~FileOutputStream() {
	impl_.Flush();
}

bool FileOutputStream::Close() {
	bool flush_succeeded = impl_.Flush();
	return copying_output_.Close() && flush_succeeded;
}

bool FileOutputStream::Flush() {
	return impl_.Flush();
}

bool FileOutputStream::Next(void** data, int *size) {
	return impl_.Next(data, size);
}

void FileOutputStream::BackUp(int count) {
	impl_.BackUp(count);
}

long FileOutputStream::ByteCount() const {
	return impl_.ByteCount();
}

FileOutputStream::CopyingFileOutputStream::CopyingFileOutputStream(
	int file_descriptor)
  : file_(file_descriptor),
  close_on_delete_(false),
  is_closed_(false),
  errno_(0) {}

FileOutputStream::CopyingFileOutputStream::~CopyingFileOutputStream() {
	if (close_on_delete_) {
		if (!Close()) {
			printf("error: close() failed: %d\n", errno_);
		}
	}
}

bool FileOutputStream::CopyingFileOutputStream::Close() {
	if (is_closed_) {
		printf("error: file already closed\n");
		return false;
	}

	is_closed_ = true;
	if (close_no_eintr(file_) != 0) {
		// The docs on close() do not specify whether a file descriptor is still
		// open after close() fails with EIO. However, the glibc source code 
		// seems to indicate that it is not.
		error_ = errno;
		return false;
	}

	return true;
}

bool FileOutputStream::CopyingFileOutputStream::Write(
	const void *buffer, int size) {
	if (is_closed_) {
		printf("error: file already closed\n");
		return false;
	}

	int total_written = 0;
	const uint8_t *buffer_base = reinterpret_cast<const uint8_t *>(buffer);

	while(total_written < size) {
		int bytes;
		do {
			bytes = write(file_, buffer_base + total_written, size - total_written);
		} while (bytes < 0 && errno == EINTR);

		if (bytes <= 0) {
			// Write error.

			if (bytes < 0) {
				errno_ = errno;
			}
			return false;
		}
		total_written += bytes;
	}

	return true;
}

